//
// Created by 沈昊 on 2022/4/3.
//

#ifndef TOOLKIT_LOGGER_HPP
#define TOOLKIT_LOGGER_HPP
#include <string>
#ifdef ENABLE_LOG
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
std::shared_ptr<spdlog::async_logger>& get_logger();
namespace logger{
    void initialize(const std::string& base_name, const spdlog::level::level_enum&, int hours = 23, int minutes = 0);
}
#define Trace(...)		SPDLOG_LOGGER_CALL(get_logger(), spdlog::level::trace, __VA_ARGS__)
#define Debug(...)		SPDLOG_LOGGER_CALL(get_logger(), spdlog::level::debug, __VA_ARGS__)
#define Info(...)		SPDLOG_LOGGER_CALL(get_logger(), spdlog::level::info, __VA_ARGS__)
#define Warn(...)		SPDLOG_LOGGER_CALL(get_logger(), spdlog::level::warn, __VA_ARGS__)
#define Error(...)		SPDLOG_LOGGER_CALL(get_logger(), spdlog::level::err, __VA_ARGS__)
#define Critical(...)  SPDLOG_LOGGER_CALL(get_logger(), spdlog::level::critical, __VA_ARGS__)
#else
namespace spdlog{
    namespace level{
        enum level_enum{trace,debug,info,warn,error,critical};
    };
};
namespace logger{
    void initialize(const std::string& base_name, const spdlog::level::level_enum&, int hours = 23, int minutes = 0);
};
#define Trace(...)		;
#define Debug(...)		;
#define Info(...)		  ;
#define Warn(...)		  ;
#define Error(...)	  ;
#define Critical(...) ;
#endif
#endif//TOOLKIT_LOGGER_HPP
