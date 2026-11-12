#pragma once
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <sqlite3.h>
namespace vkaether {
struct UserState { std::int64_t id{},balance{},experience{},level{1}; };
class Database {
public:
 explicit Database(const std::filesystem::path&); ~Database();
 Database(const Database&)=delete; Database& operator=(const Database&)=delete;
 UserState get_user(std::int64_t); std::int64_t daily_bonus(std::int64_t,std::int64_t);
 bool work(std::int64_t,std::int64_t); bool transfer(std::int64_t,std::int64_t,std::int64_t); std::string top(std::size_t);
private: void execute(const char*); sqlite3* db_{}; std::mutex mutex_;
};
}