#pragma once
#include "dispatcher.h"
#include "HitBanStrategy.cpp"
#include <type_traits>

void Dispatcher::init(Lev2MdSpi* quoter_ptr,TradeSpi* trader_ptr, LoggerPtr logger_ptr) 
{
	// bind the callback function of strategies or other moduels
	std::cout<<"[Dispatcher] init, binding functions"<< std::endl;
	
	this->bind_Callback(Eventtype::L2TICK, &StrategyBase::on_tick);
	this->bind_Callback(Eventtype::ORDER_DETIAL, &StrategyBase::on_orderDetial);
	this->bind_Callback(Eventtype::TRANSACTION, &StrategyBase::on_transac);

	this->bind_Callback(Eventtype::TRADE, &StrategyBase::on_trade);
	this->bind_Callback(Eventtype::ORDER_SUCCESS, &StrategyBase::on_order_success);
	this->bind_Callback(Eventtype::ORDER_ERROR, &StrategyBase::on_order_error);
	this->bind_Callback(Eventtype::CANCEL_ERROR, &StrategyBase::on_cancel_error);
	this->bind_Callback(Eventtype::CANCEL_SUCCESS, &StrategyBase::on_cancel_success);

	this->trader_ptr = trader_ptr;
	this->L2_quoter_ptr = quoter_ptr;
	this->m_logger_ptr = logger_ptr;
	//this->bind_Callback(Eventtype::, &TaskBase::on_cus_event);
	// !! test add a simple strategy
	//std::shared_ptr<Strategy> temp_strategy = std::make_shared<Strategy>(1);
	//_sID_strategyPtr_map[1] = temp_strategy;
}

void Dispatcher::Start()
{
	
	if (_poolsize <= 0)
	{
		std::cout << " pool size cannot be 0" << std::endl;
		return;
	}

    {
        std::lock_guard<std::mutex> lock(_pool_mutex);
        if (!_tpool.empty() )
        {
            std::cout << " pool is has started or the pool is not empty" << std::endl;
            return;
        }

        _is_running = true;
        std::cout<<"[Dispatcher] creating threads"<< std::endl;
        //Start pool first
        for (int i = 0; i < _poolsize; i++)
        {
            auto th = new std::thread(&Dispatcher::worker_main, this);
            _tpool.push_back(th);
        }
    }
	//Start dispatcher later
	_dispatcher_th = std::thread(&Dispatcher::dispatch, this);
	std::cout<<"[Dispatcher] threads created"<< std::endl;
}

void Dispatcher::Stop()
{
	_is_running = false;
	for ( auto& th : _tpool)
	{
		th->join();
	}

    if (_dispatcher_th.joinable())
	{
		_dispatcher_th.join();
	}
}

void Dispatcher::dispatch()
{
	std::cout<<"[Dispatcher::dispatch] starting dispatch"<< std::endl;
	while (_is_running)
	{
		//std::cout<<"[Dispatcher::dispatch] dispatch running"<< std::endl;
		auto start = std::chrono::high_resolution_clock::now();
		SEEvent temp_event; // SSEvent is small e_type(4 bytes) + shared_prt (16 bytes)
		if (_event_q.try_dequeue(temp_event)) 
		{
			auto func_to_call = _cb_mapping.find(temp_event.e_type);
			if (func_to_call == _cb_mapping.end()) 
			{
				// Handle the case where there is no callback for this event type
				std::cout << "No callback found for this event type, Your etype is : " << temp_event.e_type << std::endl;
				return;
			}

			// Those events should only be dispatched to matching Strategies, thus the 'if' check
			if (temp_event.e_type == Eventtype::CANCEL_ERROR ||
			   temp_event.e_type == Eventtype::CANCEL_SUCCESS ||
			   temp_event.e_type == Eventtype::ORDER_SUCCESS ||
			   temp_event.e_type == Eventtype::ORDER_ERROR ||
			   temp_event.e_type == Eventtype::TRADE )
			   {
					if (temp_event.S_id[0] == '\0') // S_id is SInfo, if its empty, then the order could be placed by another system or manuelly
					{
						std::cout<<"Sinfo is empty "<<temp_event.e_type<<temp_event.event<<std::endl;
						return;
					}

					std::map<int, std::shared_ptr<StrategyBase>>::iterator temp_strategy_iter;
					{
						std::shared_lock<std::shared_mutex> lock(_strategy_mutex);
						temp_strategy_iter = _sID_strategyPtr_map.find(std::stoi(temp_event.S_id));
						// SInfo is not empty but strategy not found, stategy could be removed
						if (temp_strategy_iter == _sID_strategyPtr_map.end()) {
							std::cout<<"Strategy might be removed s_id: "<< temp_event.S_id << temp_event.e_type <<std::endl;}
					}
					SETask task( temp_event.event, func_to_call->second,  temp_strategy_iter->second );
					_task_q.enqueue(std::move(task));
			   }
			
			// case for all the quoter events, no need to match strategy
			else 
			{
				std::shared_lock<std::shared_mutex> lock(_strategy_mutex);
				for (const auto& pair : _sID_strategyPtr_map) 
				{
					SETask task( temp_event.event, func_to_call->second,  pair.second );
					_task_q.enqueue(std::move(task));
				}
			}
		}
		auto stop = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
		//std::cout << "[Dispatcher]: "
        //      << duration.count() << " nanoseconds" << std::endl;
		//std::cout << "[Dispatch] temp_event id" << &temp_event;
		//std::cout << " payload," << temp_event.payload << " type : " << temp_event.e_type << std::endl;
	}
}

void Dispatcher::worker_main()
{
	std::cout<<"[Dispatcher::worker_main] starting worker"<< std::endl;
	while (_is_running)
	{
		//std::cout<<"[Dispatcher::worker_main] running"<< std::endl;
		SETask task;
		if (_task_q.try_dequeue(task)) 
		{
			try 
			{
				(task.s.get()->*task._cb_funcPtr)(task.event);
			} catch (const std::exception& e) {
				// Code to handle the exception
				std::cerr << "Exception caught: " << e.what() << std::endl;
			}
		}
		// call the Strategy's instance method
		//std::cout << "[Dispatcher] trying to process event of type " << item._e.e_type << " and payload: " << item._e.payload;
		//std::cout<<"func is "<< item .s << std::endl;
		//std::cout << "One task finished " << std::endl;
	}
}


void Dispatcher::add_strategy(int s_id, std::string SecurityID, const char& eid, std::shared_ptr<StrategyBase> s)
{
	std::unique_lock<std::shared_mutex> lock(_strategy_mutex);
	auto stock_to_strategy_iter = stock_to_strategy_map.find(SecurityID);
	if (stock_to_strategy_iter == stock_to_strategy_map.end()) {
        // Key does not exist, insert a new key with a list containing the value
		// subscribe to market
        stock_to_strategy_map[SecurityID] = std::list<int>{s_id};
		// convert ‘const char*’ to ‘char*’
		char nonconst_SecurityID[31];
		strcpy(nonconst_SecurityID,SecurityID.c_str());
		char* Securities[1];
		Securities[0] = nonconst_SecurityID;
		L2_quoter_ptr->Subscribe(Securities,1,eid);// 1 means only subscribe one stock

    } else {
        // Key exists, append value to the existing list
		// no need to subscribe
        stock_to_strategy_iter->second.push_back(s_id);
    }
	_sID_strategyPtr_map.emplace(s_id , s);
}

void Dispatcher::remove_strategy(int s_id,std::string SecurityID, const char& eid)
{
	std::unique_lock<std::shared_mutex> lock(_strategy_mutex);
	auto S_toRemove = _sID_strategyPtr_map.find(s_id);
	if (S_toRemove != _sID_strategyPtr_map.end()) {
		// stock_to_strategy_map< std::string(SecurityID), std::list<S_ID> >
		// S_ID is unique through out the program
		stock_to_strategy_map[SecurityID].remove(s_id);

		if (stock_to_strategy_map[SecurityID].size()==0)
		{
			char nonconst_SecurityID[31];
			strcpy(nonconst_SecurityID,SecurityID.c_str());
			char* Securities[1];
			Securities[0] = nonconst_SecurityID;
			L2_quoter_ptr->UnSubscribe(Securities,1,eid);// 1 means only subscribe one stock
		}
		auto removed_strategy_node = _sID_strategyPtr_map.extract(s_id);
		if (removed_strategy_node) 
		{
        	_removed_sID_strategyPtr_map.insert(std::move(removed_strategy_node));
    	}
	}
	else {
		// Handle the case where there is no callback for this event type
		std::cout << "No Strategy found for this event type, Your Strategy is: " << s_id << std::endl;
	}
}

int Dispatcher::get_event_q_size()
{
	return _event_q.size_approx();
}

void Dispatcher::bind_Callback(Eventtype e_type, FuncPtr func)
{
	_cb_mapping[e_type] =func;
}

void Dispatcher::unbind_Callback(Eventtype e_type)
{
	auto it = _cb_mapping.find(e_type);
	if (it != _cb_mapping.end()) {
		_cb_mapping.erase(e_type);
	}
	else {
		// Handle the case where there is no callback for this event type
		std::cout << "No callback found for this event type, Your etype is: " << e_type << std::endl;
	}
}

nlohmann::json Dispatcher::check_running_strategy()
{
	nlohmann::json combinedJson = nlohmann::json::array();

	std::shared_lock<std::shared_mutex> lock(_strategy_mutex);
	// shared lock scope
	{
		for (const auto& pair : _sID_strategyPtr_map) 
		{
			nlohmann::json temp_json;
			//std::shared_ptr<StrategyBase> temp_base_ptr = pair.second;
			//std::cout<<temp_base_ptr->test_var<<std::endl;
			std::shared_ptr<HitBanStrategy> temp_strategy = std::static_pointer_cast<HitBanStrategy>(pair.second);

			temp_json["ID"] = temp_strategy->strate_SInfo;
			temp_json["SecurityID"] = temp_strategy->strate_stock_code;
			temp_json["ExchangeID"] = std::string(1, temp_strategy->strate_exchangeID);
			temp_json["BuyTriggerVolume"] = temp_strategy->buy_trigger_volume;
			temp_json["CancelVolume"] = temp_strategy->cancel_trigger_volume;
			temp_json["TargetPosition"] = temp_strategy->target_position;
			temp_json["CurrPosition"] = temp_strategy->current_position;
			temp_json["MaxTriggerTimes"] = temp_strategy->current_trigger_times;
			temp_json["OrderID"] = temp_strategy->strate_OrderSysID;
			temp_json["SecurityName"] = temp_strategy->strate_stock_name;
			combinedJson.push_back(temp_json);
		}
	}
    return combinedJson;
}

nlohmann::json Dispatcher::check_removed_strategy()
{
	nlohmann::json combinedJson = nlohmann::json::array();
	std::shared_lock<std::shared_mutex> lock(_strategy_mutex);
	// shared lock scope
	{
		for (const auto& pair : _removed_sID_strategyPtr_map) 
		{
			nlohmann::json temp_json;
			//std::shared_ptr<StrategyBase> temp_base_ptr = pair.second;
			//std::cout<<temp_base_ptr->test_var<<std::endl;
			std::shared_ptr<HitBanStrategy> temp_strategy = std::static_pointer_cast<HitBanStrategy>(pair.second);

			temp_json["ID"] = temp_strategy->strate_SInfo;
			temp_json["SecurityID"] = temp_strategy->strate_stock_code;
			temp_json["ExchangeID"] =  std::string(1, temp_strategy->strate_exchangeID);
			temp_json["BuyTriggerVolume"] = temp_strategy->buy_trigger_volume;
			temp_json["CancelVolume"] = temp_strategy->cancel_trigger_volume;
			temp_json["TargetPosition"] = temp_strategy->target_position;
			temp_json["CurrPosition"] = temp_strategy->current_position;
			temp_json["MaxTriggerTimes"] = temp_strategy->current_trigger_times;
			temp_json["OrderID"] = temp_strategy->strate_OrderSysID;
			temp_json["SecurityName"] = temp_strategy->strate_stock_name;
			combinedJson.push_back(temp_json);
		}
	}
    return combinedJson;
}

void Dispatcher::update_delay_duration(int s_id, int new_delay_duration)
{
	auto S_toUpdate_iter = _sID_strategyPtr_map.find(s_id);
	if (S_toUpdate_iter != _sID_strategyPtr_map.end())
	{
		std::shared_ptr<HitBanStrategy> temp_strategy = std::static_pointer_cast<HitBanStrategy>(S_toUpdate_iter->second);
		temp_strategy->delay_duration = new_delay_duration;
	}	
	else
	{
		m_logger_ptr->warn("[update_delay_duration] s_id not found, input s_id is {}",s_id );
	}
}
