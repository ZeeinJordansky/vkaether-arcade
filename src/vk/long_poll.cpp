#include "vkaether/long_poll.hpp"
#include "vkaether/logger.hpp"
#include <chrono>
#include <thread>
namespace vkaether {
LongPoll::LongPoll(const VkApi&a,std::int64_t id):api_(a),group_id_(id){}
void LongPoll::run(const std::atomic_bool& stop,const EventHandler& handler)const{
 std::string server,key;std::int64_t ts=0;
 auto refresh=[&]{auto j=api_.call("groups.getLongPollServer",{{"group_id",group_id_}});server=j.at("server").get<std::string>();key=j.at("key").get<std::string>();ts=std::stoll(j.at("ts").get<std::string>());};
 while(!stop.load()&&server.empty()){try{refresh();}catch(const std::exception&e){Logger::write(LogLevel::error,e.what());std::this_thread::sleep_for(std::chrono::seconds(3));}}
 while(!stop.load()){try{if(server.empty())refresh();auto url=server+"?act=a_check&key="+key+"&ts="+std::to_string(ts)+"&wait=25&mode=2&version=3";auto j=nlohmann::json::parse(VkApi::http_get(url));if(j.contains("failed")){int f=j["failed"].get<int>();if(f==1&&j.contains("ts"))ts=std::stoll(j["ts"].get<std::string>());else{server.clear();key.clear();refresh();}continue;}if(j.contains("ts"))ts=std::stoll(j["ts"].get<std::string>());if(j.contains("updates"))for(const auto&e:j["updates"])handler(e);}catch(const std::exception&e){Logger::write(LogLevel::warning,std::string("Long Poll reconnect: ")+e.what());std::this_thread::sleep_for(std::chrono::milliseconds(500));}}
}
}