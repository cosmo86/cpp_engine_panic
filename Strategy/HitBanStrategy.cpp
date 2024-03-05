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

enum ScoutStatus
{
    SCOUT_RUNNING = 0,
    SCOUT_ORDER_SENT = 1,
    SCOUT_ORDER_CANCELED = 2,
	SCOUT_ORDER_CANCELED_ABNORMAL = 3,
    SCOUT_ORDER_REJECTED = -1,
	SCOUT_TRADED = 4
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

	void updateDurationNanoseconds(long long newDuration) 
	{
        this->durationNanoseconds = newDuration;
		/// !! idealy , reevaluate pair_vec and totalVolume to reflect the new duration
        // This could involve removing pairs that no longer fit within the updated duration
		// However, this could cause potential issues.
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
		//Timed logic vars
		this->lower_time_limit = j["LowerTimeLimit"].get<long long>();
		this->scout_buy_trigger_cash_lim = j["ScoutBuyTriggerCashLim"].get<int>();
		this->scout_monitor_duration = j["ScoutMonitorDuration"].get<long long>();
		this->condition_2_percentage = j["Cond2Percent"].get<float>();
		this->condition_2_higher_time = j["Cond2HighTime"].get<long long>();
		this->condition_2_track_duration = j["Cond2TrackDuration"].get<long long>();
		time_volume_tracker.updateDurationNanoseconds(this->condition_2_track_duration);
		this->cancel_trigger_volume_large = j["CancelTriggerVolumeLarge"].get<int>();
		this->condition_4_low_time = j["Cond4LowTime"].get<long long>();
		this->condition_4_high_time = j["Cond4HighTime"].get<long long>();
		//

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

		nlohmann::json jsonObj = this->get_strategy_params();
		for (auto& [key, value] : jsonObj.items()) 
		{
			std::cout << "Key: " << key << ", Value: ";
			if (value.is_primitive()) {
				std::cout << value << std::endl;}
		}

		std::cout<<"strate_stock_code"<<strate_stock_code<<std::endl;
		std::cout<<"buy_trigger_volume"<<buy_trigger_volume<<std::endl;
		std::cout<<"cancel_trigger_volume"<<cancel_trigger_volume<<std::endl;
		std::cout<<"max_trigger_times"<<max_trigger_times<<std::endl;
		std::cout<<"position"<<position<<std::endl;
		std::cout<<"strate_SInfo"<<strate_SInfo<<std::endl;
		std::cout<<"strate_stock_name"<<strate_stock_name<<std::endl;
		std::cout<<"strate_exchangeID"<<strate_exchangeID<<std::endl;
		std::cout<<"target_position"<<target_position<<std::endl;
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
	double strate_curr_trade_price = 0; // 
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
	// They are atomic to avoid a dead-lock in this case:
	// 1. check_running_strategy is called first, dispatcher shared_mutex is locked, trying to get strategy's mutex
	// 2. Strategy may have traded and strategy's lock is accquired, Strategy calls dispatcher -remove_strategy 
	//    trying to get dispatcher mutex
	// Do you see the dead-lock? Thus we cannot try to get the strategy's mutex during check_running_strategy.
	std::atomic<int> current_position;
	std::atomic<int> running_status;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	int max_order_error_tolerance = 2;
	int order_error_tolerance_count = 0;

	int curr_FengBan_volume = 0;

	bool can_resend_order = true;

	bool if_formal_order_sent = false;
	bool if_cancel_sent = false;

	bool formal_order_accepted = false;
	char strate_OrderSysID[21];

	// Timed logic vars
	
	std::chrono::steady_clock::time_point formal_order_acpt_time;//this defaults to epoch start
	long long duration_from_formal_order_acpt = 0;
	std::chrono::steady_clock::time_point temp_curr_time;
	long long lower_time_limit = 2000000000;

	// condition 1 scout order
	int scout_buy_trigger_cash_lim = 0;
	bool scout_order_sent = false;
	bool scout_order_acpt = false;
	bool scout_order_traded = false;
	//Scout_status is atomic for the same reason as running_status
	std::atomic<int> scout_status{0};
	std::chrono::steady_clock::time_point scout_order_acpt_time;
	long long scout_monitor_duration = 600000000000;
	char scout_OrderSysID[21];

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

	nlohmann::json get_strategy_params()
	{
		nlohmann::json temp_json;

		temp_json["ID"] = this->strate_SInfo;
		temp_json["SecurityID"] = this->strate_stock_code;
		temp_json["ExchangeID"] = std::string(1, this->strate_exchangeID);
		temp_json["BuyTriggerVolume"] = this->buy_trigger_volume;
		temp_json["CancelVolume"] = this->cancel_trigger_volume;
		temp_json["TargetPosition"] = this->target_position;
		temp_json["CurrPosition"] = this->current_position.load();
		temp_json["Status"] = this->running_status.load();
		temp_json["MaxTriggerTimes"] = this->current_trigger_times;
		temp_json["OrderID"] = this->strate_OrderSysID;
		temp_json["SecurityName"] = this->strate_stock_name;
		// time logic vars
		temp_json["LowerTimeLimit"] = this->lower_time_limit;
		temp_json["ScoutBuyTriggerCashLim"] = this->scout_buy_trigger_cash_lim;
		temp_json["ScoutMonitorDuration"] = this->scout_monitor_duration;
		temp_json["ScoutStatus"] = this->scout_status.load();
		temp_json["Cond2Percent"] = this->condition_2_percentage;
		temp_json["Cond2HighTime"] = this->condition_2_higher_time;
		temp_json["Cond2TrackDuration"] = this->condition_2_track_duration;
		temp_json["CancelTriggerVolumeLarge"] = this->cancel_trigger_volume_large;
		temp_json["Cond4LowTime"] = this->condition_4_low_time;
		temp_json["Cond4HighTime"] = this->condition_4_high_time;
		return temp_json;
		
	}

	void reset_scout()
	{
		this->scout_order_acpt = false;
		this->scout_order_traded = false;
		this->scout_OrderSysID[0] = '\0';
	}

	void check_scout_order()
	{
		// !!! This functoin is under the condition scout is traded
		// !!!
		// No locks here because the context where check_scout_order() is called should be already locked
		long long duration_from_scout_order_acpt =  std::chrono::duration_cast<std::chrono::nanoseconds>(this->temp_curr_time - this->scout_order_acpt_time).count();
		if(duration_from_scout_order_acpt <= this->scout_monitor_duration)
		{
			if(this->if_formal_order_sent == false)// scout traded before formal order sent
			{
				this->can_resend_order = false;
				m_logger->warn("S,{}, [CHECK SCOUT] , code: {}, scout traded within {} s formal order not placed, scout ordersysID: {}, ",
							this->strate_SInfo,
							this->strate_stock_code,
							duration_from_scout_order_acpt/1000000000.0,
							this->scout_OrderSysID
							);
				this->reset_scout();
			}
			// cancel formal order
			if(this->formal_order_accepted == true && this->if_cancel_sent == false)
			{
				m_dispatcher_ptr->trader_ptr->Send_Cancle_Order(this->strate_exchangeID, 
																this->strate_OrderSysID,
																this->strate_SInfo);
				this->if_cancel_sent = true;
				m_logger->warn("S,{}, [CHECK SCOUT] , code: {}, scout traded within {} s cancel formal order {}, scout ordersysID: {}, ",
							this->strate_SInfo,
							this->strate_stock_code,
							duration_from_scout_order_acpt/1000000000.0,
							this->strate_OrderSysID,
							this->scout_OrderSysID
							);
				return;
			}

		}
		// after 10 mintes
		else
		{
			m_logger->warn("S,{}, [CHECK SCOUT] , code: {}, scout traded but after monitor duration: {} s, scout ordersysID: {}, ",
							this->strate_SInfo,
							this->strate_stock_code,
							duration_from_scout_order_acpt/1000000000.0,
							this->scout_OrderSysID
							);
			return;
		}

	}

	void action()
	{
		// No locks here because the context where action() is called should be already locked

		// Scout order
		// only place order when scout is not sent and formal order is not acpted
		if(this->scout_order_sent == false && this->formal_order_accepted == false)
		{
			if(curr_FengBan_volume * strate_limup_price >= scout_buy_trigger_cash_lim)
			{
				m_dispatcher_ptr->trader_ptr->Send_Order_LimitPrice(this->strate_exchangeID, 
																100, 
																this->strate_limup_price,
																this->strate_stock_code, 
																this->strate_SInfo,
																1 );
			}
			this->scout_order_sent = true;
			m_logger->warn("S,{}, [ACTION] , code: {}, scout order sent, curr_volume: {},trigger_volume: {}",
							this->strate_SInfo,
							this->strate_stock_code,
							this->curr_FengBan_volume,
							this->buy_trigger_volume
							);
		}

		//Send order
		bool __condition_buy = curr_FengBan_volume * strate_limup_price >= buy_trigger_volume;

		if(if_formal_order_sent == false && __condition_buy && this->can_resend_order )
		{
			m_dispatcher_ptr->trader_ptr->Send_Order_LimitPrice(this->strate_exchangeID, 
																this->target_position, 
																this->strate_limup_price,
																this->strate_stock_code, 
																this->strate_SInfo );
			this->if_formal_order_sent = true;
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

			this->duration_from_formal_order_acpt = std::chrono::duration_cast<std::chrono::nanoseconds>(this->temp_curr_time - this->formal_order_acpt_time).count();

			// cond 2
			if ( this->duration_from_formal_order_acpt <= this->condition_2_higher_time )
			{
				// <= condition_2_percentage because cancel_order is negative
				if( static_cast<double>(time_volume_tracker.getTotalVolume()) / this->curr_FengBan_volume <= this->condition_2_percentage )
				{ 
					__cancel_cond_2= true;
				}
			}

			// cond 3
			__cancel_cond_3 = this->curr_FengBan_volume * this->strate_limup_price <= this->cancel_trigger_volume;

			// cond 4
			if (this->duration_from_formal_order_acpt >= this->condition_4_low_time && this->duration_from_formal_order_acpt <= this->condition_4_high_time)
			{
				__cancel_cond_4 = this->curr_FengBan_volume * this->strate_limup_price <= this->cancel_trigger_volume_large;
			}

			if ( __cancel_cond_1 || __cancel_cond_2 || __cancel_cond_3 || __cancel_cond_4 || __cancel_cond_5)
			{
				m_dispatcher_ptr->trader_ptr->Send_Cancle_Order(this->strate_exchangeID, 
																this->strate_OrderSysID,
																this->strate_SInfo);
				this->if_cancel_sent = true;

				m_dispatcher_ptr->trader_ptr->Send_Cancle_Order(this->strate_exchangeID, 
																this->scout_OrderSysID,
																this->strate_SInfo,
																2);
				
				m_logger->info("S,{}, [ACTION] , code: {}, cancle sent, curr_volume: {}, cancle_volume: {}", 
								this->strate_SInfo,
								this->strate_stock_code,
								this->curr_FengBan_volume,
								this->cancel_trigger_volume
								);
				m_logger->info("S,{}, [ACTION] , code: {}, SCOUT cancle sent, curr_volume: {}, cancle_volume: {}", 
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

		////////////////////////////////////
		if (strcmp(temp_tick->SecurityID, strate_stock_code) == 0)
		{
			m_logger->info("S,{}, [on_transac] , RECEIVED , SecurityID , source: {}, Strategy: {},",
							this->strate_SInfo,
							temp_tick->SecurityID,
							strate_stock_code);
		}

		if (  strncmp(strate_stock_code, temp_tick->SecurityID, 6) == 0 &&  strcmp(temp_tick->SecurityID, strate_stock_code) != 0)
		{
			m_logger->warn("S,{}, [ON_TICK] , len of strate_stock_code {}, len of coming data {}",
						this->strate_SInfo,
						strlen(strate_stock_code),
						strlen(temp_tick->SecurityID)
						);
		}
		///////////////////////////////////////////////////////
		if (strcmp(temp_tick->SecurityID, strate_stock_code) != 0){
			//m_logger->warn("S,{}, [ON_TICK] , SecurityID mismatach, source: {}, Strategy: {}",
			//				this->strate_SInfo,
			//				temp_tick->SecurityID,
			//				strate_stock_code);
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
					this->scout_order_sent = false;
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

			//this->strate_curr_trade_price = temp_tick->LastPrice;
		}
	}

	void on_orderDetial(std::shared_ptr<SEObject> e) override
	{
		
		std::shared_ptr<SE_Lev2OrderDetailField> temp_orderdetial = std::static_pointer_cast<SE_Lev2OrderDetailField>(e);

		////////////////////////////////////
		if (strcmp(temp_orderdetial->SecurityID, strate_stock_code) == 0)
		{
			m_logger->info("S,{}, [on_transac] , RECEIVED , SecurityID , source: {}, Strategy: {},",
							this->strate_SInfo,
							temp_orderdetial->SecurityID,
							strate_stock_code);
		}

		if (  strncmp(strate_stock_code, temp_orderdetial->SecurityID, 6) == 0 &&  strcmp(temp_orderdetial->SecurityID, strate_stock_code) != 0)
		{
			m_logger->warn("S,{}, [ON_TICK] , len of strate_stock_code {}, len of coming data {}",
						this->strate_SInfo,
						strlen(strate_stock_code),
						strlen(temp_orderdetial->SecurityID)
						);
		}
		///////////////////////////////////////////////////////

		if (strcmp(temp_orderdetial->SecurityID, strate_stock_code) != 0)
		{
			//m_logger->warn("S,{}, [on_orderDetial] , SecurityID mismatach, source: {}, Strategy: {}",
			//				this->strate_SInfo,
			//				temp_orderdetial->SecurityID,
			//				strate_stock_code);
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
			if (temp_orderdetial->Price == this->strate_limup_price && 
				temp_orderdetial->Side == '1' && 
				temp_orderdetial->OrderStatus != 'D')
			{
				curr_FengBan_volume += temp_orderdetial->Volume;
				if (this->strate_curr_trade_price >= this->strate_limup_price )
				{
					action();
				}
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
				time_volume_tracker.insertPair( this->temp_curr_time, -temp_orderdetial->Volume );
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
		
		////////////////////////////////////
		if (strcmp(temp_transac->SecurityID, strate_stock_code) == 0)
		{
			m_logger->info("S,{}, [on_transac] , RECEIVED , SecurityID , source: {}, Strategy: {},",
							this->strate_SInfo,
							temp_transac->SecurityID,
							strate_stock_code);
		}

		if (  strncmp(strate_stock_code, temp_transac->SecurityID, 6) == 0 &&  strcmp(temp_transac->SecurityID, strate_stock_code) != 0)
		{
			m_logger->warn("S,{}, [ON_TICK] , len of strate_stock_code {}, len of coming data {}",
						this->strate_SInfo,
						strlen(strate_stock_code),
						strlen(temp_transac->SecurityID)
						);
		}
		///////////////////////////////////////////////////////
		if (strcmp(temp_transac->SecurityID, strate_stock_code) != 0)
		{
			//m_logger->warn("S,{}, [on_transac] , SecurityID mismatach, source: {}, Strategy: {}",
			//				this->strate_SInfo,
			//				temp_transac->SecurityID,
			//				strate_stock_code);
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
					this->scout_order_sent = false;
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
				time_volume_tracker.insertPair( this->temp_curr_time, -temp_transac->TradeVolume );
				this->action();
				this->strate_curr_trade_price = temp_transac->TradePrice;
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

		if (temp_orderField->IInfo == 1)
		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			strcpy(this->scout_OrderSysID,temp_orderField->OrderSysID);
			this->scout_order_acpt_time = std::chrono::steady_clock::now();
			this->scout_order_acpt = true;
			this->scout_status.store(ScoutStatus::SCOUT_ORDER_SENT);
			m_logger->info("S,{}, [ORDER_SUCCESS] ,scout order acpt, securityID:{}, SInfo:{}, IInfo: {}, OrderSysID: {}",
							this->strate_SInfo, 
							temp_orderField->SecurityID, temp_orderField->SInfo, temp_orderField->SInfo , temp_orderField->OrderSysID);
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

		if (temp_orderField->IInfo == 1)
		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->scout_order_sent  = false;
			this->scout_status.store(ScoutStatus::SCOUT_ORDER_REJECTED);
			m_logger->info("S,{}, [ORDER_ERROR] ,scout order NOT acpt, securityID:{}, SInfo:{}, IInfo: {}, OrderSysID: {}",
							this->strate_SInfo, 
							temp_orderField->SecurityID, temp_orderField->SInfo, temp_orderField->SInfo , temp_orderField->OrderSysID);
			return;
		}

		if (strcmp(temp_orderField->SInfo , strate_SInfo) != 0 ){
			m_logger->info("S,{}, [ORDER_ERROR] ,SInfo dont match {},  Strategy{}",this->strate_SInfo,temp_orderField->SInfo, this->strate_SInfo);
			return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->if_formal_order_sent = false;
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

		if (temp_orderActionField->IInfo == 2)
		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->reset_scout();
			this->scout_status.store(ScoutStatus::SCOUT_ORDER_CANCELED);
			m_logger->info("S,{}, [CANCEL SUCCESS] ,scout order canceled, securityID:{}, SInfo:{}, IInfo: {}, OrderSysID: {}",
							this->strate_SInfo, 
							this->strate_stock_code,
							temp_orderActionField->SInfo, temp_orderActionField->IInfo , temp_orderActionField->OrderSysID);
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
			this->if_formal_order_sent = false;
			this->formal_order_accepted = false;
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

		if (temp_orderActionField->IInfo == 2)
		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->scout_status.store(ScoutStatus::SCOUT_ORDER_CANCELED_ABNORMAL);
			m_logger->info("S,{}, [CANCEL ERROR] ,scout order NOT canceled, securityID:{}, SInfo:{}, IInfo: {}, OrderSysID: {}",
							this->strate_SInfo, 
							this->strate_stock_code,
							temp_orderActionField->SInfo, temp_orderActionField->SInfo , temp_orderActionField->OrderSysID);
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

		// scout order traded
		if (strcmp(temp_TradeField->OrderSysID , this->scout_OrderSysID) == 0 )
		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->scout_order_traded = true;
			this->check_scout_order();
			this->scout_status.store(ScoutStatus::SCOUT_TRADED);
			return;
		}

		{
			std::unique_lock<std::shared_mutex> lock(m_shared_mtx);
			this->current_position.fetch_add(temp_TradeField->Volume);
			if (this->current_position.load() == this->target_position)
			{
				this->running_status.store(StrategyStatus::FULLY_TRADED);
				m_logger->info("S,{}, [ON_TRADE] ,fully traded, security {}, trade_volume: {}, curr_pos {}",
				this->strate_SInfo,temp_TradeField->SecurityID, temp_TradeField->Volume, this->current_position.load());
				this->stop_strategy();
				return;
			}
			else
			{
				m_logger->info("S,{}, [ON_TRADE] ,Part traded, security {}, trade_volume: {}, curr_pos {}",
				this->strate_SInfo,temp_TradeField->SecurityID, temp_TradeField->Volume, this->current_position.load());
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

