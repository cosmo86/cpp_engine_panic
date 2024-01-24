#pragma once
#include <iostream>
#include <string>
#include <thread>
#include <map>
#include <vector>
#include <list>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include "Event.h"
#include "SEObject.hpp"
#include "StrategyBase.h"
#include "L2_quoter.cpp"
#include "trader.cpp"

#include "concurrentqueue.h"


class Dispatcher
{
public:
	typedef void (StrategyBase::*FuncPtr)(const std::shared_ptr<SEObject> e);

	Dispatcher() { std::cout << "[Dispatcher] Dispatcher created"<<std::endl; }
	~Dispatcher() { std::cout << "[Dispatcher] Dispatcher destoried" << std::endl; }

	void init(Lev2MdSpi* quoter_ptr,TradeSpi* trader_ptr,LoggerPtr logger_ptr );
	void Start();
	void Stop();
	void bind_Callback(Eventtype e_type, FuncPtr func);
	void unbind_Callback(Eventtype e_type);

	void worker_main();
	void add_strategy(int s_id, std::string, const char&, std::shared_ptr<StrategyBase> s);
	void remove_strategy(int s_id,std::string SecurityID, const char& eid);
	void dispatch();
	int get_event_q_size();
	nlohmann::json check_running_strategy();
	nlohmann::json check_removed_strategy();
	void update_delay_duration(int s_id, int new_delay_duration);

public:
	moodycamel::ConcurrentQueue<SEEvent>  _event_q;
	Lev2MdSpi* L2_quoter_ptr;
    TradeSpi* trader_ptr;
private:
	bool _is_running;
	std::thread _dispatcher_th;

	LoggerPtr m_logger_ptr;

	std::shared_mutex _strategy_mutex;
    std::mutex _event_mutex;
    std::mutex _task_mutex;
    std::mutex _pool_mutex;

	size_t _poolsize = 2;
	
	
	//moodycamel::ConcurrentQueue<std::shared_ptr<SEObject>>  _event_q;
	//std::queue<Task>  _task_q;
	moodycamel::ConcurrentQueue<SETask>  _task_q;
	std::map<Eventtype, FuncPtr > _cb_mapping;

	// use case for stock_to_strategy_map
	// 1. iterate through to assign task for quoter-related Tasks, no need to use index to get elements
	// 2. keep track of which stocks to subscribe/unsubscribe
	std::map<std::string, std::list<int>> stock_to_strategy_map; 
	std::map<int, std::shared_ptr<StrategyBase>> _sID_strategyPtr_map;
	std::map<int, std::shared_ptr<StrategyBase>> _removed_sID_strategyPtr_map;
	std::vector<std::thread*> _tpool;


};