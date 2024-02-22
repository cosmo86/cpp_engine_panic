#pragma once
#include "StrategyBase.h"
#include "dispatcher.h"

class Dispatcher;

// Hitban specific Status
enum StrategyStatus
{
    RUNNING = 0,
    ORDER_SENT = 1,
    ORDER_CANCELED = 2,
    FULLY_TRADED = 3,
    PART_TRADED = 7,
    STOPPED = 4,
    ORDER_CANCELED_ABNORMAL = 5,
    PAUSED = 6,
    REJECTED = -1
};

// For Condition 2 
class TimeVolumePair {
public:
    std::chrono::steady_clock::time_point time;
    int volume;
    TimeVolumePair(std::chrono::steady_clock::time_point t, int v) : time(t), volume(v) {}
};

class TimeVolumeTracker {
private:
    std::vector<TimeVolumePair> pair_vec;
    long long durationNanoseconds;
    int totalVolume;
public:
    TimeVolumeTracker(long long duration) : durationNanoseconds(duration), totalVolume(0) {}

    void insertPair(std::chrono::steady_clock::time_point time, int volume) {
        auto it = pair_vec.begin();
        while (it != pair_vec.end()) {
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(time - it->time).count();
            if (duration > durationNanoseconds) {
                totalVolume -= it->volume;
                it = pair_vec.erase(it); // Efficient for vectors if erasing from the beginning
            } else {
                ++it;
            }
        }
        pair_vec.push_back(TimeVolumePair(time, volume));
        totalVolume += volume;
    }

    int getTotalVolume() const {
        return totalVolume;
    }

    void printPairs() const {
        for (const auto& pair : pair_vec) {
            std::cout << "Time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(pair.time.time_since_epoch()).count()
                      << ", Volume: " << pair.volume << std::endl;
        }
    }
};



class HitBanStrategy : public StrategyBase 
{
public:
	HitBanStrategy(const nlohmann::json& j, Dispatcher* ptr, LoggerPtr log_ptr) : m_dispatcher_ptr(ptr),
	  m_logger(log_ptr),
	  current_position(0), 
      running_status(0) 
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
		// this sets the securityName
		get_security_name();
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
		std::cout<<"strate_limup_price"<<strate_limup_price<<std::endl;
		std::cout<<"strate_stock_name"<<strate_stock_name<<std::endl;
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
	int current_trigger_times = 0;
	
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Please refer to dispatcher.check_running_strategy() 
	// to understand why those two values must be atomic<int>
	std::atomic<int> current_position;
	std::atomic<int> running_status;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	int max_order_error_tolerance = 2;
	int order_error_tolerance_count = 0;

	int curr_FengBan_volume = 0;

	bool can_resend_order = true;

	bool if_order_sent = false;
	bool if_cancel_sent = false;
	int delay_duration = 500; // in milliseconds

	bool formal_order_accepted = false;
	char strate_OrderSysID[21];

	// Timed logic vars
	
	std::chrono::steady_clock::time_point formal_order_acpt_time;//this defaults to epoch start
	long long duration_from_formal_order_acpt = 0;
	std::chrono::steady_clock::time_point temp_curr_time;
	long long lower_time_limit = 2000000000;

	// condition 2
	float condition_2_percentage = -0.35;
	long long condition_2_higher_time = 180000000000;
	long long condition_2_track_duration = 3000000000;
	TimeVolumeTracker time_volume_tracker{condition_2_track_duration};

	// condition 4
	int cancel_trigger_volume_large = 0;
	long long condition_4_low_time = 4000000000;
	long long condition_4_high_time = 600000000000;





public:

	void action()
	{
		// No locks here because the context where action() is called should be already locked

		//Send order
		bool __condition_buy = curr_FengBan_volume * strate_limup_price >= buy_trigger_volume;

		if(if_order_sent == false && __condition_buy && this->can_resend_order )
		{
			m_dispatcher_ptr->trader_ptr->Send_Order_LimitPrice(this->strate_exchangeID, 
																this->target_position, 
																this->strate_limup_price,
																this->strate_stock_code, 
																this->strate_SInfo );
			this->if_order_sent = true;
			// Set to false to avoid resend while limup and above trigger_volume
			this->can_resend_order = false;
			m_logger->info("S,{}, [ACTION] , code: {}, order sent, curr_volume: {},trigger_volume: {}", 
							this->strate_SInfo,
							this->strate_stock_code,
							this->curr_FengBan_volume,
							this->buy_trigger_volume
							);
			return;
		}

		//Cancel order

		if (this->formal_order_accepted == true && this->if_cancel_sent == false && this->duration_from_formal_order_acpt >= this->lower_time_limit)
		{
			bool __cancel_cond_1 = false;
			bool __cancel_cond_2 = false;
			bool __cancel_cond_3 = false;
			bool __cancel_cond_4 = false;
			bool __cancel_cond_5 = false;

			this->duration_from_formal_order_acpt = std::chrono::duration_cast<std::chrono::nanoseconds(this->temp_curr_time - this->formal_order_acpt_time).count();

			if ( this->duration_from_formal_order_acpt <= this->condition_2_higher_time )
			{
				if( static_cast<double>(time_volume_tracker.getTotalVolume()) / this->curr_FengBan_volume <= this->condition_2_percentage ){ 
					__cancel_cond_2= true}
			}

			__cancel_cond_3 = this->curr_FengBan_volume * this->strate_limup_price <= this->cancel_trigger_volume;

			if (this->duration_from_formal_order_acpt >= this->condition_4_low_time && this->duration_from_formal_order_acpt <= this->condition_4_high_time){
				__cancel_cond_4 = this->curr_FengBan_volume * this->strate_limup_price <= this->cancel_trigger_volume_large;
			}

			if ( __cancel_cond_1 || __cancel_cond_2 || __cancel_cond_3 || __cancel_cond_4 || __cancel_cond_5)
			{
				m_dispatcher_ptr->trader_ptr->Send_Cancle_Order(this->strate_exchangeID, 
																this->strate_OrderSysID,
																this->strate_SInfo);
				this->if_cancel_sent = true;
				m_logger->info("S,{}, [ACTION] , code: {}, cancle sent, curr_volume: {}, cancle_volume: {}", 
								this->strate_SInfo,
								this->strate_stock_code,
								this->curr_FengBan_volume,
								this->cancel_trigger_volume
								);
				return;
			}
		}

	}                                                                         

	void on_tick(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_Lev2MarketDataField> temp_tick = std::static_pointer_cast<SE_Lev2MarketDataField>(e);

		
		//std::cout<<"on_orderDetial, security id "<< temp_tick->SecurityID<<std::endl;

		if (strcmp(temp_tick->SecurityID, strate_stock_code) != 0){
			return;
		}

		// unique_lock scope
		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);

			this->temp_curr_time = std::chrono::steady_clock::now();

			if(temp_tick->LastPrice < this->strate_limup_price)
			{
				// If true remain true to enable send order
				if (this->can_resend_order){return;}
				else{
				// If false, set tp true to enable send order
					this->can_resend_order = true;
					m_logger->warn("S,{}, [ON_TICK] , limup price is {},Last price is {}, Bitsetting can_resend_order  to true. ",
									this->strate_SInfo,
									this->strate_limup_price,
									temp_tick->LastPrice);
					return;
				}
			}
			// Stock is limup
			this->curr_FengBan_volume = temp_tick->BidVolume1;
			action();
			m_logger->info("S,{}, [ON_TICK] , code: {} limup: {} curr price: {}, curr_volume: {}, trigger_volume:{},cancle_volume: {} ",
						 this->strate_SInfo,
						 temp_tick->SecurityID, 
						 this->strate_limup_price,
						 temp_tick->BidPrice1,
						 this->curr_FengBan_volume,
						 this->buy_trigger_volume,
						 this->cancel_trigger_volume );
		}
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

			this->temp_curr_time = std::chrono::steady_clock::now();

			if (temp_orderdetial->Price < this->strate_limup_price)
			{
				// Not updating can_resend_order because order price can be lower than limup while still limup
				return;
			}

			//buy order not cancled, add buy volume to fengban_volume
			if (temp_orderdetial->Price == this->strate_limup_price && temp_orderdetial->Side == '1' && temp_orderdetial->OrderStatus != 'D')
			{
				curr_FengBan_volume += temp_orderdetial->Volume;
				time_volume_tracker.insertPair( this->temp_curr_time,temp_orderdetial->Volume );
				action();
				m_logger->info("S,{}, [ON_ORDERDETIAL] , code: {} limup: {} curr price: {}, curr_volume: {}, trigger_volume:{},cancle_volume: {} ",
						 this->strate_SInfo,
						 temp_orderdetial->SecurityID, 
						 this->strate_limup_price,
						 temp_orderdetial->Price,
						 this->curr_FengBan_volume,
						 this->buy_trigger_volume,
						 this->cancel_trigger_volume );
				return;
			}
			// buy order and cancled, subtract volume from fengban,ExchangeID == '1' is SSE
			if (temp_orderdetial->ExchangeID == '1' && temp_orderdetial->Price == this->strate_limup_price && 
				temp_orderdetial->Side == '1' && temp_orderdetial->OrderStatus == 'D')
			{
				curr_FengBan_volume -= temp_orderdetial->Volume;
				time_volume_tracker.insertPair( this->temp_curr_time,temp_orderdetial->Volume );
				action();
				m_logger->info("S,{}, [ON_ORDERDETIAL] , code: {} limup: {} curr price: {}, curr_volume: {}, trigger_volume:{},cancle_volume: {} ",
				         this->strate_SInfo,
						 temp_orderdetial->SecurityID, 
						 this->strate_limup_price,
						 temp_orderdetial->Price,
						 this->curr_FengBan_volume,
						 this->buy_trigger_volume,
						 this->cancel_trigger_volume );
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

			this->temp_curr_time = std::chrono::steady_clock::now();

			if (temp_transac->TradePrice < this->strate_limup_price)
			{
				// If true remain true to enable send order
				if (this->can_resend_order){return;}
				else if(temp_transac->ExecType == '1'){
				// If false, set tp true to enable send order
					this->can_resend_order = true;
					m_logger->warn("S,{}, [ON_TRANSAC] , limup price is {},Trade price is {}, Bitsetting can_resend_order  to true. strate_limup_price and this->strate_limup_price {} ",
									this->strate_SInfo,
									this->strate_limup_price,
									temp_transac->TradePrice,
									this->strate_limup_price);
					return;
				}
			}
			else
			{
				this->curr_FengBan_volume -= temp_transac->TradeVolume;
				time_volume_tracker.insertPair( this->temp_curr_time,temp_transac->TradeVolume );
				this->action();
			}
		}
		// ExchangeID == '2' SZSE , ExecType == '2' cancel order  
		if (temp_transac->ExchangeID == '2' and temp_transac->ExecType == '2')
		{
			m_logger->info("S,{}, [ON_TRANSAC] SZSE cancel, tradetime {}, price {}, volume {}, BuyNo {}. SellNo {}", 
			this->strate_SInfo,
			temp_transac->TradeTime,
			temp_transac->TradePrice,
			temp_transac->TradeVolume,
			temp_transac->BuyNo,
			temp_transac->SellNo);
		}		
		m_logger->info("S,{}, [ON_TRANSAC] ,SZSE transaction, tradetime {}, trade_price {}, volume {}, Exectype {}, curr_FengBan_volume {}", 
					this->strate_SInfo,
					temp_transac->TradeTime,
					temp_transac->TradePrice,
					temp_transac->TradeVolume,
					temp_transac->ExecType,
					this->curr_FengBan_volume);
	}

	void on_order_success(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_OrderField> temp_orderField = std::static_pointer_cast<SE_OrderField>(e);
		if (strcmp(temp_orderField->SecurityID, strate_stock_code) != 0){
			return;
		}
		if (strcmp(temp_orderField->SInfo , strate_SInfo) != 0 ){
			m_logger->info("S,{}, [ORDER_SUCCESS] ,SInfo dont match{},  Strategy{}",
							this->strate_SInfo,
							temp_orderField->SInfo, this->strate_SInfo);
			return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->running_status.store(StrategyStatus::ORDER_SENT);
			strcpy(this->strate_OrderSysID,temp_orderField->OrderSysID);
			this->formal_order_accepted = true;
			this->if_cancel_sent = false;
			m_logger->info("S,{}, [ORDER_SUCCESS] , securityID:{}, SInfo:{}, OrderSysID: {}",
							this->strate_SInfo, 
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
			m_logger->info("S,{}, [ORDER_ERROR] ,SInfo dont match {},  Strategy{}",this->strate_SInfo,temp_orderField->SInfo, this->strate_SInfo);
			return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->if_order_sent = false;
			// Reached max order reject time, stop stategy
			if (this->order_error_tolerance_count > this->max_order_error_tolerance)
			{
				this->stop_strategy();
				m_logger->info("S,{}, [ORDER_ERROR] , max order error reached, please check",this->strate_SInfo);
				return;
			}
			// Update running status and increment order_error_tolerance_count by 1
			this->running_status.store(StrategyStatus::REJECTED);
            this->order_error_tolerance_count += 1 ;
			m_logger->info("S,{}, [ORDER_ERROR] , securityID:{}, SInfo:{}, OrderSysID: {}",
							this->strate_SInfo, 
							temp_orderField->SecurityID,temp_orderField->SInfo, temp_orderField->OrderSysID);
		}
	}


	void on_cancel_success(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_InputOrderActionField> temp_orderActionField = std::static_pointer_cast<SE_InputOrderActionField>(e);
		if (strcmp(temp_orderActionField->SInfo , strate_SInfo) != 0 ){
		m_logger->info("S,{}, [CANCEL_SUCCESS] ,SInfo dont match {},  Strategy{}",this->strate_SInfo,temp_orderActionField->SInfo, this->strate_SInfo);
		return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->current_trigger_times += 1;
			if (this->current_trigger_times >= this->max_trigger_times ){
				this->stop_strategy();
				m_logger->info("S,{}, [CANCEL_SUCCESS] ,max trigger time reached, stategy stopped",this->strate_SInfo);
				return;
			}
			this->running_status.store(StrategyStatus::ORDER_CANCELED);
			this->if_order_sent = false;
		}
		m_logger->info("S,{}, [CANCEL_SUCCESS] , SIfo:{}, OrderSysID:{}",this->strate_SInfo,temp_orderActionField->SInfo,temp_orderActionField->OrderSysID);
	}

	void on_cancel_error(std::shared_ptr<SEObject> e) override
	{
		std::shared_ptr<SE_InputOrderActionField> temp_orderActionField = std::static_pointer_cast<SE_InputOrderActionField>(e);
		if (strcmp(temp_orderActionField->SInfo , strate_SInfo) != 0 ){
		m_logger->info("S,{}, [CANCLE_ERROR] , SInfo dont match:{},  Strategy{}",this->strate_SInfo,temp_orderActionField->SInfo, this->strate_SInfo);
		return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->if_cancel_sent = false;
			this->running_status.store(StrategyStatus::ORDER_CANCELED_ABNORMAL);
		}
		m_logger->info("S,{}, [CANCLE_ERROR] ,  OrderSysID:{} ",this->strate_SInfo, temp_orderActionField->OrderSysID);
	}

	void on_trade(std::shared_ptr<SEObject> e) override //SE_TradeField
	{
		std::shared_ptr<SE_TradeField> temp_TradeField = std::static_pointer_cast<SE_TradeField>(e);
		if (strcmp(temp_TradeField->OrderSysID , strate_OrderSysID) != 0 ){
		m_logger->info("S,{}, [ON_TRADE] ,OrderSysID dont match OrderSysID:{},  strate_OrderSysID{}",this->strate_SInfo,temp_TradeField->OrderSysID, this->strate_OrderSysID);
		return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			if (temp_TradeField->Volume == this->target_position)
			{
				this->current_position.fetch_add(temp_TradeField->Volume);
				this->running_status.store(StrategyStatus::FULLY_TRADED);
				this->stop_strategy();
				m_logger->info("S,{}, [ON_TRADE] ,fully traded, security {}, trade_volume: {}",this->strate_SInfo,temp_TradeField->SecurityID, temp_TradeField->Volume);
				return;
			}
			else
			{
				m_logger->info("S,{}, [ON_TRADE] ,Part traded, security {}, trade_volume: {}",this->strate_SInfo,temp_TradeField->SecurityID, temp_TradeField->Volume);
				this->current_position.fetch_add(temp_TradeField->Volume);
				this->running_status.store(StrategyStatus::PART_TRADED);
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
		double res = m_dispatcher_ptr->trader_ptr->get_limup_price(stock_str);
		if(res<0)
		{
			throw std::invalid_argument("limup price must be greater than 0");
		}
		return res;
	}

	void get_security_name()
	{	
		std::string stock_str(this->strate_stock_code);
		std::string securityName = m_dispatcher_ptr->trader_ptr->get_security_name(stock_str);
		if(securityName=="NOTFOUND")
		{
			throw std::invalid_argument("Security not found");
		}
		strcpy(this->strate_stock_name, securityName.c_str());
	}
};

