#pragma once
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>
namespace vkaether {
class VkApi {
public: VkApi(std::string,std::string); nlohmann::json call(std::string_view,const nlohmann::json& = nlohmann::json::object()) const;
 static std::string http_get(std::string_view);
private: std::string token_,version_;
};
}