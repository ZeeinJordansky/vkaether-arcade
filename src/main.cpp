#include "vkaether/bot.hpp"
#include "vkaether/logger.hpp"
#include <csignal>
#include <iostream>
namespace {vkaether::Bot* instance=nullptr;void stop_handler(int){if(instance)instance->stop();}}
int main(int argc,char**argv){try{auto c=vkaether::Config::load(argc>1?argv[1]:"config.json");if(c.log_level=="debug")vkaether::Logger::set_level(vkaether::LogLevel::debug);else if(c.log_level=="warning")vkaether::Logger::set_level(vkaether::LogLevel::warning);else if(c.log_level=="error")vkaether::Logger::set_level(vkaether::LogLevel::error);vkaether::Bot bot(std::move(c));instance=&bot;std::signal(SIGINT,stop_handler);std::signal(SIGTERM,stop_handler);bot.run();instance=nullptr;return 0;}catch(const std::exception&e){std::cerr<<"Fatal: "<<e.what()<<'\n';return 1;}}
