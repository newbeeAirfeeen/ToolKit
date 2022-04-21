//
// Created by 沈昊 on 2022/4/3.
//
#include "logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/formatter.h>
#include <fmt/bundled/color.h>
#include <mutex>
static constexpr const char* logger_name = "logger";
static std::shared_ptr<spdlog::async_logger> async_logger;
std::shared_ptr<spdlog::async_logger>& get_logger(){
    return std::ref(async_logger);
}

namespace logger{

    class basic_formatter : public spdlog::formatter{
    public:
        ~basic_formatter() override = default;
        void format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest) override{
            using namespace spdlog;
            /*%Y-%m-%d %H:%M:%S*/
            constexpr const char* format_pattern = "[{:%Y-%m-%d %H:%M:%S}.{:0>3}] [thread {}] [{}] [{}:{}] {}\n";
            /* level */
            std::string level = "Trace";
            std::string line = msg.source.filename;
            auto pos = line.find_last_of("/\\");
            line = line.substr(pos + 1);
            switch (msg.level) {
                case level::level_enum::trace:													                      level = "Trace";	 break;
                case level::level_enum::debug:   type = fmt::terminal_color::blue;				    level = "Debug";	 break;
                case level::level_enum::info:    type = fmt::terminal_color::green;				    level = "Info";		 break;
                case level::level_enum::warn:    type = fmt::terminal_color::yellow;			    level = "Warn";		 break;
                case level::level_enum::err:     type = fmt::terminal_color::red;	            level = "Error";	 break;
                case level::level_enum::critical:type = fmt::terminal_color::bright_red;		  level = "Critical";	 break;
                default:break;
            }
            time_t tt = std::chrono::system_clock::to_time_t(msg.time);
            tm* local_time = ::localtime(&tt);
            auto index = msg.payload.size() - 1;
            int64_t miliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(msg.time.time_since_epoch()).count() % 1000;
            fmt::format_to(std::back_inserter(dest), format_pattern, *local_time, miliseconds, msg.thread_id, level, line, msg.source.line, msg.payload);
        }
        std::unique_ptr<spdlog::formatter> clone() const override{
            std::unique_ptr<basic_formatter> basic_formatter_(new basic_formatter);
            return std::move(basic_formatter_);
        }
        inline fmt::detail::color_type& get_color_type(){
            return this->type;
        }
    private:
        fmt::detail::color_type type = fmt::terminal_color::white;
    };

    class console_formatter : public basic_formatter {
    public:
        ~console_formatter() override = default;
        void format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest) override{
            using namespace spdlog;
            basic_formatter::format(msg, dest);
            fmt::text_style style = fmt::fg(get_color_type());
            if (level::level_enum::err == msg.level || msg.level == level::level_enum::critical) {
                style |= fmt::emphasis::bold;
            }
            std::string fmt_str = std::move(fmt::format(style, spdlog::string_view_t(dest.data(), dest.size())));
            dest.clear();
            dest.append(std::move(fmt_str));
        }
        std::unique_ptr<spdlog::formatter> clone() const override{
            std::unique_ptr<console_formatter> console_formatter_(new console_formatter);
            return std::move(console_formatter_);
        }
    };


    void initialize(const std::string& base_name, const spdlog::level::level_enum& lvl, int hours, int minutes){
        static std::once_flag flag;
        auto func = [&](){
#ifdef WIN32
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD dwMode = 0;
            GetConsoleMode(hOut, &dwMode);
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
#endif
            spdlog::init_thread_pool(4096, 1);

            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            auto daily_sink = std::make_shared<spdlog::sinks::daily_file_format_sink_mt>(base_name, hours, minutes, true);

            std::unique_ptr<basic_formatter> basic_formatter_(new basic_formatter);
            std::unique_ptr<console_formatter> console_formatter_(new console_formatter);
            console_sink->set_formatter(std::move(console_formatter_));
            daily_sink->set_formatter(std::move(basic_formatter_));
            async_logger = std::make_shared<spdlog::async_logger>(logger_name, spdlog::sinks_init_list{console_sink, daily_sink},
                                                                  spdlog::thread_pool());
            async_logger->set_level(lvl);
        };
        std::call_once(flag, func);
    }
}