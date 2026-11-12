#pragma once
#include "vkaether/vk_api.hpp"
#include <atomic>
#include <cstdint>
#include <functional>
namespace vkaether {
class LongPoll {
public: using EventHandler=std::function<void(const nlohmann::json&)>; LongPoll(const VkApi&,std::int64_t);
 void run(const std::atomic_bool&,const EventHandler&) const;
private: const VkApi& api_; std::int64_t group_id_;
};
}