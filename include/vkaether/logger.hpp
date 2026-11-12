#pragma once
#include <string_view>
namespace vkaether {
enum class LogLevel { debug, info, warning, error };
class Logger { public: static void set_level(LogLevel) noexcept; static void write(LogLevel,std::string_view) noexcept; };
}