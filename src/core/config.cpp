#include "vkaether/config.hpp"
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>
namespace vkaether {
Config Config::load(const std::filesystem::path& path){
 std::ifstream in(path); if(!in) throw std::runtime_error("Cannot open config: "+path.string());
 nlohmann::json j; in>>j; Config c; c.vk_token=j.value("vk_token",std::string{}); c.group_id=j.value("group_id",std::int64_t{});
 c.api_version=j.value("api_version",std::string{"5.199"}); c.database_path=j.value("database_path",std::string{"data/vkaether.sqlite3"});
 c.worker_threads=j.value("worker_threads",std::size_t{}); c.log_level=j.value("log_level",std::string{"info"});
 if(c.vk_token.empty()||c.vk_token=="PUT_VK_COMMUNITY_TOKEN_HERE") throw std::runtime_error("Set vk_token in config.json");
 if(c.group_id<=0) throw std::runtime_error("group_id must be a positive community ID");
 return c;
}
}