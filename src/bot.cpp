#include "vkaether/bot.hpp"
#include "vkaether/logger.hpp"
#include "native/validation.h"

#include <random>
extern "C" int aether_policy_validate_message(const unsigned char* data, int length);

namespace vkaether {
Bot::Bot(Config c):config_(std::move(c)),database_(config_.database_path),api_(config_.vk_token,config_.api_version),long_poll_(api_,config_.group_id),commands_(database_),workers_(config_.worker_threads){Logger::write(LogLevel::info,"Aether Arcade ready; commands="+std::to_string(commands_.size()));}
void Bot::run(){long_poll_.run(stopping_,[this](const nlohmann::json&e){if(e.value("type",std::string{})!="message_new")return;const auto&o=e.value("object",nlohmann::json::object());const auto&m=o.value("message",o);Message msg{m.value("peer_id",std::int64_t{}),m.value("from_id",std::int64_t{}),m.value("text",std::string{})};if(!vkaether_validate_event(msg.peer_id,msg.from_id,static_cast<std::int64_t>(msg.text.size()))||aether_policy_validate_message(reinterpret_cast<const unsigned char*>(msg.text.data()),static_cast<int>(msg.text.size()))!=0)return;workers_.submit([this,msg=std::move(msg)]{try{auto r=commands_.dispatch(msg);if(r.text.empty())return;static thread_local std::mt19937_64 rng(std::random_device{}());api_.call("messages.send",{{"peer_id",msg.peer_id},{"random_id",static_cast<std::int32_t>(rng()&0x7fffffff)},{"message",r.text}});}catch(const std::exception&x){Logger::write(LogLevel::error,x.what());}});});}
void Bot::stop()noexcept{stopping_.store(true);}
}