cmake_minimum_required(VERSION 3.24)
project(vkaether_arcade VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
  message(FATAL_ERROR "The current C# NativeAOT integration targets Linux x86-64 only.")
endif()

include(ExternalProject)
find_program(CARGO_EXECUTABLE cargo REQUIRED)
find_program(DOTNET_EXECUTABLE dotnet REQUIRED)

set(RUST_TARGET_DIR "${CMAKE_BINARY_DIR}/cargo-target")
if(MSVC)
  set(RUST_STATIC_LIBRARY "${RUST_TARGET_DIR}/release/vkaether_rust.lib")
else()
  set(RUST_STATIC_LIBRARY "${RUST_TARGET_DIR}/release/libvkaether_rust.a")
endif()

ExternalProject_Add(vkaether_rust_build
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rust"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND "${CARGO_EXECUTABLE}" build --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/rust/Cargo.toml" --release --target-dir "${RUST_TARGET_DIR}"
  INSTALL_COMMAND ""
  BUILD_BYPRODUCTS "${RUST_STATIC_LIBRARY}"
  UPDATE_COMMAND ""
  BUILD_ALWAYS TRUE
)
add_library(vkaether_rust STATIC IMPORTED GLOBAL)
set_target_properties(vkaether_rust PROPERTIES IMPORTED_LOCATION "${RUST_STATIC_LIBRARY}")

set(AETHER_POLICY_DIR "${CMAKE_BINARY_DIR}/csharp")
set(AETHER_POLICY_LIBRARY "${AETHER_POLICY_DIR}/libAetherPolicy.so")
add_custom_command(
  OUTPUT "${AETHER_POLICY_LIBRARY}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${AETHER_POLICY_DIR}"
  COMMAND "${DOTNET_EXECUTABLE}" publish
          "${CMAKE_CURRENT_SOURCE_DIR}/native/AetherPolicy/AetherPolicy.csproj"
          --configuration Release --runtime linux-x64 --self-contained true
          --output "${AETHER_POLICY_DIR}"
          -p:NativeLib=Shared -p:PublishAot=true
  DEPENDS native/AetherPolicy/AetherPolicy.csproj native/AetherPolicy/Policy.cs native/AetherPolicy/aether_policy.h
  VERBATIM
)
add_custom_target(aether_policy_build DEPENDS "${AETHER_POLICY_LIBRARY}")
add_library(aether_policy SHARED IMPORTED GLOBAL)
set_target_properties(aether_policy PROPERTIES IMPORTED_LOCATION "${AETHER_POLICY_LIBRARY}")

option(VKAETHER_BUILD_TESTS "Build tests" ON)
option(VKAETHER_ENABLE_LTO "Enable LTO in Release" ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(FetchContent)
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(json)
add_library(vkaether_core src/core/config.cpp src/core/logger.cpp src/core/thread_pool.cpp src/game/database.cpp src/game/commands.cpp src/game/commands_extra.cpp src/native/validation.c src/vk/api.cpp src/vk/long_poll.cpp src/bot.cpp)
add_dependencies(vkaether_core vkaether_rust_build aether_policy_build)
target_include_directories(vkaether_core PUBLIC include native/AetherPolicy)
target_link_libraries(vkaether_core PUBLIC CURL::libcurl SQLite::SQLite3 nlohmann_json::nlohmann_json vkaether_rust aether_policy)
if(UNIX AND NOT APPLE)
  target_link_libraries(vkaether_core PUBLIC dl m rt util)
endif()
if(MSVC)
  target_compile_options(vkaether_core PRIVATE /W4 /permissive- /O2)
else()
  target_compile_options(vkaether_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  target_link_libraries(vkaether_core PUBLIC pthread)
  if(VKAETHER_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported)
    if(ipo_supported)
      set_property(TARGET vkaether_core PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    endif()
  endif()
endif()
add_executable(vkaether src/main.cpp)
target_link_libraries(vkaether PRIVATE vkaether_core)
set_target_properties(vkaether PROPERTIES BUILD_RPATH "$ORIGIN/csharp")
if(VKAETHER_BUILD_TESTS)
  enable_testing()
  add_executable(vkaether_tests tests/core_tests.cpp)
  target_link_libraries(vkaether_tests PRIVATE vkaether_core)
  set_target_properties(vkaether_tests PROPERTIES BUILD_RPATH "$ORIGIN/csharp")
  add_test(NAME core COMMAND vkaether_tests)
endif()
