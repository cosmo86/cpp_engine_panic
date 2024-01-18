#pragma once
#include "log_handle.h"

// helper function to get current timestamp as string
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now = *std::localtime(&time_t_now);

    std::stringstream ss;
    ss << std::put_time(&tm_now, "%Y%m%d_%H%M%S");
    return ss.str();
}

LoggerPtr GetLogger()
{
    static LoggerPtr logger = nullptr;
    if (!logger) {
        // Initialize spdlog's global thread pool
        size_t queue_size = 65536;  // Max number of messages in the queue
        size_t thread_count = 1;    // Number of threads in the thread pool
        spdlog::init_thread_pool(queue_size, thread_count);

        std::string filename = "log_" + getCurrentTimestamp() + ".log";

        // Create stdout color sink (console logging)
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);

        // Create a basic file sink (multi-threaded)
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("%v");

        std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };
        logger = std::make_shared<spdlog::async_logger>("async_logger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
        logger->flush_on(spdlog::level::info);
    }
    return logger;
}
