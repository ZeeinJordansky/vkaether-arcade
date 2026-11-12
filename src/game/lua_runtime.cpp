#include "lua_runtime.hpp"

#include <lua.hpp>

#include <cstdlib>
#include <string>

namespace vkaether {
namespace {

struct LuaMemory {
    std::size_t used{};
    static constexpr std::size_t limit = 1024 * 1024;
};

void* limited_allocator(void* user_data, void* pointer, std::size_t old_size, std::size_t new_size) {
    auto& memory = *static_cast<LuaMemory*>(user_data);
    if (new_size == 0) {
        if (pointer) { memory.used -= old_size; std::free(pointer); }
        return nullptr;
    }
    const std::size_t accounted_old = pointer ? old_size : 0;
    if (new_size > accounted_old && new_size - accounted_old > LuaMemory::limit - memory.used)
        return nullptr;
    void* replacement = std::realloc(pointer, new_size);
    if (replacement) memory.used = memory.used - accounted_old + new_size;
    return replacement;
}

void instruction_limit(lua_State* state, lua_Debug*) {
    luaL_error(state, "instruction limit exceeded");
}

} // namespace

std::string execute_lua(std::string_view source) {
    LuaMemory memory;
    lua_State* state = lua_newstate(limited_allocator, &memory);
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

    // Bound CPU use so scripts cannot stall a worker indefinitely.
    lua_sethook(state, instruction_limit, LUA_MASKCOUNT, 100000);

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
