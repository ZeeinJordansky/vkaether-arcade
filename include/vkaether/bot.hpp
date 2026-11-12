#pragma once
#include "vkaether/commands.hpp"
#include "vkaether/config.hpp"
#include "vkaether/long_poll.hpp"
#include "vkaether/thread_pool.hpp"
#include <atomic>
namespace vkaether {
class Bot {
public: explicit Bot(Config); void run(); void stop() noexcept;
private:
 Config config_; Database database_; VkApi api_; LongPoll long_poll_; CommandRegistry commands_; ThreadPool workers_; std::atomic_bool stopping_{false};
};
}