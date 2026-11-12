#pragma once
#include "vkaether/database.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
namespace vkaether {
struct Message { std::int64_t peer_id{},from_id{}; std::string text; };
struct Reply { std::string text; };
class CommandRegistry {
public: explicit CommandRegistry(Database&); Reply dispatch(const Message&) const; std::size_t size() const noexcept{return commands_.size();}
private:
 using Handler=std::function<Reply(const Message&,const std::vector<std::string>&)>;
 void add(std::string,Handler); void register_extended_commands(); Database& database_; std::unordered_map<std::string,Handler> commands_;
};
}