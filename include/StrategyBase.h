#pragma once
#include "Lv2dataModel.hpp"
#include "OrderModels.hpp"
#include "json.hpp"
#include <mutex>
#include <stdexcept>
#include <shared_mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <atomic>
#include <memory>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include "SEObject.hpp"

using LoggerPtr = std::shared_ptr<spdlog::async_logger>;

class SEObject;

class StrategyBase
{
public:
	StrategyBase() = default;

	virtual void on_tick(std::shared_ptr<SEObject> e) = 0;
	virtual void on_orderDetial(std::shared_ptr<SEObject> e) = 0;
	virtual void on_transac(std::shared_ptr<SEObject> e) = 0;
	virtual void on_ngstick(std::shared_ptr<SEObject> e){};

	virtual void on_order_success(std::shared_ptr<SEObject> e) = 0;
	virtual void on_order_error(std::shared_ptr<SEObject> e) = 0;

	virtual void on_cancel_success(std::shared_ptr<SEObject> e) = 0;
	virtual void on_cancel_error(std::shared_ptr<SEObject> e) = 0;
	
	virtual void on_trade(std::shared_ptr<SEObject> e) = 0;

	//tual void on_cus_event(std::shared_ptr<SEObject> e) {};
	int test_var = 1;
private:


};