#pragma once
#include <iostream>
#include <thread>
#include <map>
#include <vector>
#include <list>
#include <queue>
#include <mutex>
#include "Event.h"
#include "SEObject.hpp"
#include "StrategyBase.h"
#include "Strategy.hpp"

#include "concurrentqueue.h"

struct Task
{
	typedef void (StrategyBase::*FuncPtr)(const Event e);
	FuncPtr _cb_funcPtr;
	Event _e;
	Strategy* s;

    Task(){}
    Task(FuncPtr cb_funcPtr, const Event& e, Strategy* strategy)
        : _cb_funcPtr(cb_funcPtr), _e(e), s(strategy) {}

};

class Dispatcher
{
public:
	typedef void (StrategyBase::*FuncPtr)(const std::shared_ptr<SEObject> e);

	Dispatcher() { std::cout << "[Dispatcher] Dispatcher created"<<std::endl; }
	~Dispatcher() { std::cout << "[Dispatcher] Dispatcher destoried" << std::endl; }

	void init();
	void Start();
	void Stop();
	void bind_Callback(Eventtype e_type, FuncPtr func);
	void unbind_Callback(Eventtype e_type);

	void worker_main();
	void add_strategy(int s_id,std::shared_ptr<Strategy> s);
	void remove_strategy(int s_id);
	void dispatch();
public:
	moodycamel::ConcurrentQueue<SEEvent>  _event_q;
private:
	bool _is_running;
	std::thread _dispatcher_th;

	std::mutex _strategy_mutex;
    std::mutex _event_mutex;
    std::mutex _task_mutex;
    std::mutex _pool_mutex;

	size_t _poolsize = 2;
	/// @brief 
	
	//moodycamel::ConcurrentQueue<std::shared_ptr<SEObject>>  _event_q;
	//std::queue<Task>  _task_q;
	moodycamel::ConcurrentQueue<SETask>  _task_q;
	std::map<Eventtype, FuncPtr > _cb_mapping;

	//std::map<int , Strategy*>  _strategy_map;
	std::map<int, std::shared_ptr<Strategy>> _strategy_map;
	std::vector<std::thread*> _tpool;


};