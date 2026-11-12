# Lua command scripts

Lua is available through the `lua <expression>` VK command. Examples:

```text
lua return 6 * 7
lua return string.upper("aether")
lua return math.floor(math.pi * 100) / 100
```

Each invocation receives a fresh Lua state. The runtime exposes base, math,
string and table libraries, but does not expose filesystem, process, package
loading or debug APIs. Scripts are expressions/chunks whose final return value
is sent as the bot reply. Keep scripts short: execution is synchronous with
the command handler and should not contain expensive loops.
