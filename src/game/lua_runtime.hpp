#pragma once

#include <string>
#include <string_view>

namespace vkaether {

/// Executes a single Lua expression in a restricted, fresh Lua state.
/// Standard I/O, OS, package loading and debug facilities are unavailable.
std::string execute_lua(std::string_view source);

} // namespace vkaether
