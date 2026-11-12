#include "lua_runtime.hpp"

#include <lua.hpp>

#include <string>

namespace vkaether {
namespace {

int lua_error(lua_State* state) {
    const char* message = lua_tostring(state, -1);
    return luaL_error(state, "%s", message ? message : "unknown Lua error");
}

} // namespace

std::string execute_lua(std::string_view source) {
    lua_State* state = luaL_newstate();
    if (!state) {
        return "Lua: не удалось создать состояние.";
    }

    // Open only deterministic, non-I/O libraries. A fresh state per invocation
    // prevents one user's script from mutating another invocation's globals.
    luaL_requiref(state, LUA_GNAME, luaopen_base, 1);
    lua_pop(state, 1);
    luaL_requiref(state, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(state, 1);
    luaL_requiref(state, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(state, 1);
    luaL_requiref(state, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(state, 1);

    // Remove unsafe base-library entry points even though io/os/package were
    // never opened explicitly.
    lua_pushnil(state); lua_setglobal(state, "dofile");
    lua_pushnil(state); lua_setglobal(state, "loadfile");
    lua_pushnil(state); lua_setglobal(state, "load");

    const std::string script(source);
    if (luaL_loadbuffer(state, script.data(), script.size(), "vk-command") != LUA_OK) {
        const char* error = lua_tostring(state, -1);
        std::string result = "Lua: ошибка компиляции: ";
        result += error ? error : "unknown error";
        lua_close(state);
        return result;
    }

    if (lua_pcall(state, 0, 1, 0) != LUA_OK) {
        const char* error = lua_tostring(state, -1);
        std::string result = "Lua: ошибка выполнения: ";
        result += error ? error : "unknown error";
        lua_close(state);
        return result;
    }

    std::string result;
    if (lua_isstring(state, -1)) {
        result = lua_tostring(state, -1);
    } else if (lua_isnumber(state, -1)) {
        result = std::to_string(lua_tonumber(state, -1));
    } else if (lua_isboolean(state, -1)) {
        result = lua_toboolean(state, -1) ? "true" : "false";
    } else if (lua_isnil(state, -1)) {
        result = "nil";
    } else {
        result = "Lua: верни строку, число, boolean или nil.";
    }

    lua_close(state);
    constexpr std::size_t max_reply_size = 3500;
    if (result.size() > max_reply_size) {
        result.resize(max_reply_size);
        result += "…";
    }
    return result;
}

} // namespace vkaether
