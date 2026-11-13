# Polyglot runtime

Aether Arcade is built as one application with explicit language boundaries:

| Language | Responsibility | Integration |
|---|---|---|
| C++20 | VK Long Poll, command dispatch, worker pool, game orchestration | Main native executable |
| Rust | Checked arithmetic and rock-paper-scissors game rules | Static library, stable C ABI |
| C11 | Tiny low-level ingress/identity primitives | Compiled into the C++ core |
| C# / .NET 8 NativeAOT | UTF-8 and message-ingress policy validation | Native shared library, stable C ABI |
| Go | Process supervision, graceful shutdown, health endpoints | Production entrypoint |

The network-facing bot remains a single native process. Go supervises it rather than
creating an extra network hop for every VK event. C and C# validators run in-process
through C ABI calls. Rust domain code is statically linked. This avoids a distributed
microservice deployment for components that do not need independent scaling.

## Build prerequisites

CMake 3.24+, C11/C++20 compilers, Rust stable, Go 1.22+, .NET 8 SDK with NativeAOT
toolchain, libcurl and SQLite3.

## Health

The Go supervisor exposes `/healthz` and `/readyz` on port 8080. It returns
HTTP 200 while the child bot process is alive and 503 after it exits. It forwards
SIGINT/SIGTERM and gives the bot up to ten seconds to stop gracefully.

## Native boundaries

- C event validation rejects non-positive VK peer/user IDs, empty payloads and payloads larger than 4096 bytes before queue submission.
- C# policy validation rejects invalid UTF-8, embedded NUL/control characters and
  payloads over 4096 bytes before the C++ command dispatcher sees them.
- Rust arithmetic/game functions return explicit status codes for null output pointers, invalid domains, division by zero and overflow; the command layer calls these functions directly, and the FFI boundaries are covered by Rust and C++ tests.
