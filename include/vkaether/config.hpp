#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
namespace vkaether {
struct Config {
 std::string vk_token; std::int64_t group_id{}; std::string api_version{"5.199"};
 std::filesystem::path database_path{"data/vkaether.sqlite3"}; std::size_t worker_threads{}; std::string log_level{"info"};
 static Config load(const std::filesystem::path& path);
};
}