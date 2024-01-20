#pragma once
#include "StrategyBase.h"

class Dispatcher;

class HitBanStrategy : public StrategyBase 
{
private:
    Dispatcher* m_dispatcher_ptr = nullptr;
    json m_stratParams;
	std::shared_mutex m_shared_mtx

public:
	char strate_SInfo[33];
	char strate_stock_code[31];
	char strate_stock_name[81];
	char strate_exchangeID;
	float strate_limup_price;
	int buy_trigger_volume;
	int cancel_trigger_volume;
	int max_trigger_times;
    // TODO :!!!!!!!!!! fix position
	int position;
	int target_position = (static_cast<int>(position / strate_limup_price) / 100) * 100;;
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
	char order_sys_id[21];

public:

	void action()
	{

		bool __condition_buy = curr_FengBan_volume * strate_limup_price >= buy_trigger_volume;
        bool __condition_cancel = curr_FengBan_volume * strate_limup_price <= cancel_trigger_volume;

		if(if_order_sent==false || __condition_buy )
		{
			m_dispatcher_ptr->trader_ptr->Send_Order_LimitPrice(this->strate_exchangeID, target_position, strate_limup_price,strate_stock_code )
			if_order_sent = true;
			m_logger->info("order sent")
			std::this_thread::sleep_for(std::chrono::milliseconds(delay_duration));

		}

		if (order_created != false || if_cancel_sent == false || __condition_cancel)
		{
			m_dispatcher_ptr->trader_ptr->Send_Cancle_Order(strate_exchangeID,order_sys_id);
			m_logger->info("cancel request sent")
		}

	}                                                                         

	void on_tick(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_Lev2MarketDataField> temp_tick = std::static_pointer_cast<SE_Lev2MarketDataField>(e);
		if (strcmp(temp_tick->SecurityID, strate_stock_code) != 0){
			return;
		}

		// unique_lock scope
		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			if(temp_tick->BidPrice1 < strate_limup_price){return;}
			curr_FengBan_volume += temp_tick->BidVolume1;
			action();
		}
		m_logger.info("{}, {}, {}",temp_tick->DataTimeStamp, temp_tick->BidPrice1, temp_tick->BidVolume1);


	}

	void on_orderDetial(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_Lev2OrderDetailField> temp_orderdetial = std::static_pointer_cast<SE_Lev2OrderDetailField>(e);
		if (strcmp(temp_orderdetial->SecurityID, strate_stock_code) != 0){
			return;
		}

		//unique share-lock
		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			if (temp_orderdetial->Price < strate_limup_price){return;}

			//buy order not cancled, add buy volume to fengban_volume
			if (temp_orderdetial->Price == self.limup_price || temp_orderdetial->Side == '1' || temp_orderdetial->OrderStatus != 'D')
			{
				curr_FengBan_volume += temp_orderdetial->Volume;
				action();
				m_logger->info("{},curr_FengBan_volume{}",temp_orderdetial->OrderTime, curr_FengBan_volume);
				return;
			}
			// buy order and cancled, subtract volume from fengban,ExchangeID == '1' is SSE
			if (temp_orderdetial->ExchangeID == '1' || temp_orderdetial->Price == self.limup_price || 
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
		if (strcmp(temp_transac->SecurityID, strate_stock_code) != 0){
			return;
		}

		//unique share-lock
		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);

			if (temp_transac->TradePrice < strate_limup_price){return;}
			curr_FengBan_volume -= temp_transac->TradeVolume;
			self.action();
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
			temp_orderField->SecurityID,temp_orderField->SInfo temp_orderField->OrderSysID);
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
				m_logger.info("on_order ERROR, max order error reached, please check");
				return;
			}
			// Update running status and increment order_error_tolerance_count by 1
			this->running_status = StrategyStatus::REJECTED;
            this->order_error_tolerance_count += 1 ;
			m_logger->info("order_error, securityID:{}, SInfo:{}, OrderSysID: {}", 
			temp_orderField->SecurityID,temp_orderField->SInfo temp_orderField->OrderSysID);
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
				m_logger.info("max trigger time reached, stategy stopped");
				return;
			}
			this->running_status = StrategyStatus::ORDER_CANCELED;
		}
		m_logger.info("ORDER CANCELED, SIfo:{}, OrderSysID:{}",temp_orderActionField->SInfo,temp_orderActionField->OrderSysID);
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
		m_logger.info("cancel_error  OrderSysID:{} ", temp_orderActionField->OrderSysID);
	}

	void on _trade(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_InputOrderActionField> temp_orderActionField = std::static_pointer_cast<SE_InputOrderActionField>(e);
		if (strcmp(temp_orderActionField->OrderSysID , strate_OrderSysID) != 0 ){
		m_logger->info("OrderSysID dont match OrderSysID:{},  strate_OrderSysID{}",temp_orderActionField->OrderSysID, this->strate_OrderSysID);
		return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			if (temp_orderActionField->Volume == this->target_position)
			{
				current_position += temp_orderActionField->Volume;
				this->running_status = StrategyStatus::FULLY_TRADED;
				this->stop_strategy(fully_traded = True);
				m_logger.info("fully traded, security {}, trade_volume: {}",temp_orderActionField->SecurityID, temp_orderActionField->Volume);
				return;
			}
			else
			{
				m_logger.info("Part traded, security {}, trade_volume: {}",temp_orderActionField->SecurityID, temp_orderActionField->Volume);
				this->running_status = StrategyStatus::PART_TRADED
				return;
			}
		}
	}

	void stop_strategy(){}
};

