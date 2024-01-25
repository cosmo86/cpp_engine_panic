#pragma once
#include "StrategyBase.h"
#include "dispatcher.h"

class Dispatcher;

class HitBanStrategy : public StrategyBase 
{
public:
	HitBanStrategy(const nlohmann::json& j, Dispatcher* ptr, LoggerPtr log_ptr) : m_dispatcher_ptr(ptr) , m_logger(log_ptr)
	{
		std::cout<<"[HitBanStrategy] One instance created"<<std::endl;
		this->buy_trigger_volume = j["BuyTriggerVolume"].get<int>();
		this->cancel_trigger_volume = j["CancelVolume"].get<int>();
		this->max_trigger_times = j["MaxTriggerTimes"].get<int>();
		this->position = j["Position"].get<int>();
		this->delay_duration = j["DelayTime"].get<int>();
		

		std::string tempStr_securityID = j["SecurityID"].get<std::string>();
		std::strcpy(this->strate_stock_code, tempStr_securityID.c_str());

		std::string tempStr_sinfo = j["ID"].get<std::string>();
		std::strcpy(this->strate_SInfo, tempStr_sinfo.c_str());

		std::string tempStr_sname = j["SecurityName"].get<std::string>();
		std::strcpy(this->strate_stock_name, tempStr_sname.c_str());

		this->strate_exchangeID = j["ExchangeID"].get<std::string>()[0];



		this->strate_limup_price = get_limup_price();
		this->target_position = (static_cast<int>(this->position / this->strate_limup_price) / 100) * 100;

		std::cout<<"strate_stock_code"<<strate_stock_code<<std::endl;
		std::cout<<"buy_trigger_volume"<<buy_trigger_volume<<std::endl;
		std::cout<<"cancel_trigger_volume"<<cancel_trigger_volume<<std::endl;
		std::cout<<"max_trigger_times"<<max_trigger_times<<std::endl;
		std::cout<<"position"<<position<<std::endl;
		std::cout<<"strate_SInfo"<<strate_SInfo<<std::endl;
		std::cout<<"strate_stock_name"<<strate_stock_name<<std::endl;
		std::cout<<"strate_exchangeID"<<strate_exchangeID<<std::endl;
		std::cout<<"target_position"<<target_position<<std::endl;
		std::cout<<"delay_duration"<<delay_duration<<std::endl;
	}

	~HitBanStrategy(){std::cout<<"[HitBanStrategy] destoryed"<<std::endl;}
private:
    Dispatcher* m_dispatcher_ptr = nullptr;
    nlohmann::json m_stratParams;
	std::shared_mutex m_shared_mtx;
	LoggerPtr m_logger;

public:
	char strate_SInfo[33];
	char strate_stock_code[31];
	char strate_stock_name[81];
	char strate_exchangeID;
	double strate_limup_price;
	int buy_trigger_volume;
	int cancel_trigger_volume;
	int max_trigger_times;
    // TODO :!!!!!!!!!! fix position
	int position;
	int target_position;
	int current_position;

	int current_trigger_times = 0;
	StrategyStatus running_status;

	int max_order_error_tolerance = 2;
	int order_error_tolerance_count = 0;

	int curr_FengBan_volume = 0;

	bool if_order_sent = false;
	bool if_cancel_sent = false;
	int delay_duration; // in milliseconds

	bool order_created = false;
	char strate_OrderSysID[21];

public:

	void action()
	{

		bool __condition_buy = curr_FengBan_volume * strate_limup_price >= buy_trigger_volume;
        bool __condition_cancel = curr_FengBan_volume * strate_limup_price <= cancel_trigger_volume;

		if(if_order_sent==false || __condition_buy )
		{
			m_dispatcher_ptr->trader_ptr->Send_Order_LimitPrice(this->strate_exchangeID, target_position, strate_limup_price,strate_stock_code );
			if_order_sent = true;
			m_logger->info("order sent");
			std::this_thread::sleep_for(std::chrono::milliseconds(delay_duration));
			return;
		}

		if (order_created != false || if_cancel_sent == false || __condition_cancel)
		{
			m_dispatcher_ptr->trader_ptr->Send_Cancle_Order(strate_exchangeID,strate_OrderSysID);
			m_logger->info("cancel request sent");
			return;
		}

	}                                                                         

	void on_tick(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_Lev2MarketDataField> temp_tick = std::static_pointer_cast<SE_Lev2MarketDataField>(e);

		m_logger->info("on_tick, security id {} limup {} curr price {}", temp_tick->SecurityID, this->strate_limup_price,temp_tick->BidPrice1 );
		//std::cout<<"on_orderDetial, security id "<< temp_tick->SecurityID<<std::endl;


		if (strcmp(temp_tick->SecurityID, strate_stock_code) != 0){
			return;
		}

		// unique_lock scope
		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			if(temp_tick->BidPrice1 < this->strate_limup_price){return;}
			curr_FengBan_volume += temp_tick->BidVolume1;
			action();
		}
		m_logger->info("on_tick {}, {}, {}",temp_tick->DataTimeStamp, temp_tick->BidPrice1, temp_tick->BidVolume1);


	}

	void on_orderDetial(std::shared_ptr<SEObject> e) override
	{

		std::shared_ptr<SE_Lev2OrderDetailField> temp_orderdetial = std::static_pointer_cast<SE_Lev2OrderDetailField>(e);

		m_logger->info("on_orderDetial, security id {} limup price {} price {}", temp_orderdetial->SecurityID,this->strate_limup_price, temp_orderdetial->Price);
		//std::cout<<"on_orderDetial, security id "<< temp_orderdetial->SecurityID<<std::endl;

		if (strcmp(temp_orderdetial->SecurityID, strate_stock_code) != 0){
			return;
		}

		//unique share-lock
		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			if (temp_orderdetial->Price < this->strate_limup_price){return;}

			//buy order not cancled, add buy volume to fengban_volume
			if (temp_orderdetial->Price == this->strate_limup_price || temp_orderdetial->Side == '1' || temp_orderdetial->OrderStatus != 'D')
			{
				curr_FengBan_volume += temp_orderdetial->Volume;
				action();
				m_logger->info("{},curr_FengBan_volume{}",temp_orderdetial->OrderTime, curr_FengBan_volume);
				return;
			}
			// buy order and cancled, subtract volume from fengban,ExchangeID == '1' is SSE
			if (temp_orderdetial->ExchangeID == '1' || temp_orderdetial->Price == this->strate_limup_price || 
				temp_orderdetial->Side == '1' || temp_orderdetial->OrderStatus == 'D')
			{
				curr_FengBan_volume -= temp_orderdetial->Volume;
				action();
				m_logger->info("{},curr_FengBan_volume{}",temp_orderdetial->OrderTime, curr_FengBan_volume);
				return;
			}
		}
	}

	void on_transac(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_Lev2TransactionStruct> temp_transac = std::static_pointer_cast<SE_Lev2TransactionStruct>(e);

		m_logger->info("on_transac, security id {},limup price {} trade_price {}", temp_transac->SecurityID,this->strate_limup_price, temp_transac->TradePrice);
		//std::cout<<"on_transac, security id "<< temp_transac->SecurityID<<std::endl;

		if (strcmp(temp_transac->SecurityID, strate_stock_code) != 0){
			return;
		}

		//unique share-lock
		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);

			if (temp_transac->TradePrice < strate_limup_price){return;}
			curr_FengBan_volume -= temp_transac->TradeVolume;
			this->action();
			return;
		}
		// ExchangeID == '2' SZSE , ExecType == '2' cancel order  
		if (temp_transac->ExchangeID == '2' and temp_transac->ExecType == '2')
		{
			m_logger->info("SZSE cancel {} price {} volume[{}] BuyNo[{}] SellNo[{}]", temp_transac->TradeTime,
			temp_transac->TradePrice,temp_transac->TradeVolume,temp_transac->BuyNo,temp_transac->SellNo);
		}		
		m_logger->info("SZSE transaction {} Exectype {} curr_FengBan_volume[{}] ", temp_transac->TradeTime,
			temp_transac->ExecType,curr_FengBan_volume);
	}

	void on_order_success(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_OrderField> temp_orderField = std::static_pointer_cast<SE_OrderField>(e);
		if (strcmp(temp_orderField->SecurityID, strate_stock_code) != 0){
			return;
		}
		if (strcmp(temp_orderField->SInfo , strate_SInfo) != 0 ){
			m_logger->info("SInfo dont match event:{},  Strategy{}",temp_orderField->SInfo, this->strate_SInfo);
			return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->running_status = StrategyStatus::ORDER_SENT;
			m_logger->info("order_success, securityID:{}, SInfo:{}, OrderSysID: {}", 
			temp_orderField->SecurityID, temp_orderField->SInfo , temp_orderField->OrderSysID);
		}
	}

	void on_order_error(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_OrderField> temp_orderField = std::static_pointer_cast<SE_OrderField>(e);
		if (strcmp(temp_orderField->SecurityID, strate_stock_code) != 0){
			return;
		}
		if (strcmp(temp_orderField->SInfo , strate_SInfo) != 0 ){
			m_logger->info("SInfo dont match event:{},  Strategy{}",temp_orderField->SInfo, this->strate_SInfo);
			return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			// Reached max order reject time, stop stategy
			if (this->order_error_tolerance_count > this->max_order_error_tolerance)
			{
				this->stop_strategy();
				m_logger->info("on_order ERROR, max order error reached, please check");
				return;
			}
			// Update running status and increment order_error_tolerance_count by 1
			this->running_status = StrategyStatus::REJECTED;
            this->order_error_tolerance_count += 1 ;
			m_logger->info("order_error, securityID:{}, SInfo:{}, OrderSysID: {}", 
			temp_orderField->SecurityID,temp_orderField->SInfo, temp_orderField->OrderSysID);
		}
	}


	void on_cancel_success(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_InputOrderActionField> temp_orderActionField = std::static_pointer_cast<SE_InputOrderActionField>(e);
		if (strcmp(temp_orderActionField->SInfo , strate_SInfo) != 0 ){
		m_logger->info("SInfo dont match event:{},  Strategy{}",temp_orderActionField->SInfo, this->strate_SInfo);
		return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->current_trigger_times += 1;
			if (this->current_trigger_times >= this->max_trigger_times ){
				this->stop_strategy();
				m_logger->info("max trigger time reached, stategy stopped");
				return;
			}
			this->running_status = StrategyStatus::ORDER_CANCELED;
		}
		m_logger->info("ORDER CANCELED, SIfo:{}, OrderSysID:{}",temp_orderActionField->SInfo,temp_orderActionField->OrderSysID);
	}

	void on_cancel_error(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_InputOrderActionField> temp_orderActionField = std::static_pointer_cast<SE_InputOrderActionField>(e);
		if (strcmp(temp_orderActionField->SInfo , strate_SInfo) != 0 ){
		m_logger->info("SInfo dont match event:{},  Strategy{}",temp_orderActionField->SInfo, this->strate_SInfo);
		return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->running_status = StrategyStatus::ORDER_CANCELED_ABNORMAL;
		}
		m_logger->info("cancel_error  OrderSysID:{} ", temp_orderActionField->OrderSysID);
	}

	void on_trade(std::shared_ptr<SEObject> e) override //SE_TradeField
	{
		std::shared_ptr<SE_TradeField> temp_TradeField = std::static_pointer_cast<SE_TradeField>(e);
		if (strcmp(temp_TradeField->OrderSysID , strate_OrderSysID) != 0 ){
		m_logger->info("OrderSysID dont match OrderSysID:{},  strate_OrderSysID{}",temp_TradeField->OrderSysID, this->strate_OrderSysID);
		return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			if (temp_TradeField->Volume == this->target_position)
			{
				current_position += temp_TradeField->Volume;
				this->running_status = StrategyStatus::FULLY_TRADED;
				this->stop_strategy();
				m_logger->info("fully traded, security {}, trade_volume: {}",temp_TradeField->SecurityID, temp_TradeField->Volume);
				return;
			}
			else
			{
				m_logger->info("Part traded, security {}, trade_volume: {}",temp_TradeField->SecurityID, temp_TradeField->Volume);
				this->running_status = StrategyStatus::PART_TRADED;
				return;
			}
		}
	}

	void stop_strategy()
	{
		std::string SecurityID(this->strate_stock_code);
		this->m_dispatcher_ptr->remove_strategy(std::stoi(strate_SInfo), SecurityID,this->strate_exchangeID);
	}

	double get_limup_price()
	{
		std::string stock_str(this->strate_stock_code);
		return m_dispatcher_ptr->trader_ptr->get_limup_price(stock_str);
	}
};

