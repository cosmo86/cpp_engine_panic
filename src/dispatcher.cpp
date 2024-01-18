#include "dispatcher.h"
#include <shared_mutex>

void Dispatcher::init() 
{
	// bind the callback function of strategies or other moduels
	std::cout<<"[Dispatcher] init, binding functions"<< std::endl;
	this->bind_Callback(Eventtype::L2TICK, &StrategyBase::on_tick);
	this->bind_Callback(Eventtype::ORDER_DETIAL, &StrategyBase::on_orderDetial);
	this->bind_Callback(Eventtype::TRANSACTION, &StrategyBase::on_transac);
	
	//this->bind_Callback(Eventtype::, &TaskBase::on_cus_event);
	std::shared_ptr<Strategy> temp_strategy = std::make_shared<Strategy>(1);
	_strategy_map[1] = temp_strategy;
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

			if (temp_event.e_type == Eventtype::CANCEL_ERROR ||
			   temp_event.e_type == Eventtype::CANCEL_SUCCESS ||
			   temp_event.e_type == Eventtype::ORDER_SUCCESS ||
			   temp_event.e_type == Eventtype::ORDER_ERROR ||
			   temp_event.e_type == Eventtype::TRADE )
			   {
					if (temp_event.S_id == '\0')
					{
						std::cout<<"Sinfo is empty "<<temp_event.e_type<<temp_event.event<<std::endl;
						return;
					}
					auto temp_strategy_iter = _strategy_map.find(std::stoi(temp_event.S_id));
					if (temp_strategy_iter == _strategy_map.end()) {std::cout<<}
					SETask task( temp_event.event, func_to_call->second,  temp_strategy_iter->second );
					_task_q.enqueue(std::move(task));
			   }
			// lock the task_q
			{
				std::shared_lock<std::shared_mutex> lock(_strategy_mutex);
				for (const auto& pair : _strategy_map) 
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


void Dispatcher::add_strategy(int s_id, std::shared_ptr<Strategy> s)
{
	std::unique_lock<std::shared_mutex> lock(_strategy_mutex);
	_strategy_map.emplace(s_id , s);
}

void Dispatcher::remove_strategy(int s_id)
{
	std::unique_lock<std::shared_mutex> lock(_strategy_mutex);
	auto S_toRemove = _strategy_map.find(s_id);
	if (S_toRemove != _strategy_map.end()) {
		_strategy_map.erase(s_id);
	}
	else {
		// Handle the case where there is no callback for this event type
		std::cout << "No Strategy found for this event type, Your Strategy is: " << s_id << std::endl;
	}
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