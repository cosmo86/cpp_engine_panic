#pragma once
#include <iostream>
#include <vector>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <thread>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"

using LoggerPtr = std::shared_ptr<spdlog::async_logger>;

// helper function to get current timestamp as string
std::string getCurrentTimestamp();
LoggerPtr GetLogger();
