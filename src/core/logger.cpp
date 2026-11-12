#include "vkaether/logger.hpp"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
namespace vkaether {
namespace { std::mutex output_mutex; LogLevel threshold=LogLevel::info; const char* name(LogLevel l){switch(l){case LogLevel::debug:return "DEBUG";case LogLevel::info:return "INFO";case LogLevel::warning:return "WARN";case LogLevel::error:return "ERROR";}return "?";} }
void Logger::set_level(LogLevel l) noexcept{threshold=l;}
void Logger::write(LogLevel l,std::string_view msg) noexcept{if(static_cast<int>(l)<static_cast<int>(threshold))return;try{std::lock_guard lock(output_mutex);auto now=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());std::tm tm{};
#ifdef _WIN32
localtime_s(&tm,&now);
#else
localtime_r(&now,&tm);
#endif
std::cerr<<std::put_time(&tm,"%F %T")<<" ["<<name(l)<<"] "<<msg<<'\\n';}catch(...){}}
}