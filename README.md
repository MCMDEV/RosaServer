<img src="https://i.imgur.com/4N3PMTS.png" width="600">

A linux server scripting API for [Sub Rosa](http://subrosagame.com/).

RosaServer uses [LuaJIT](http://luajit.org/); this means there's no hit to performance while being able to create anything from moderation tools to complex custom games with easy-to-write version agnostic code.

# Getting Started
1. Create a normal Sub Rosa Dedicated Server
2. Compile the Server as it is described in [#Building](https://github.com/RosaServer/RosaServer#Building)
3. Copy both `./RosaServer/librosaserver.so` and `./LuaJIT/src/libluajit.so` into your normal Sub Rosa vanilla server folder
4. Create or put your code in the `./main/init.lua` file in your Sub Rosa vanilla server folder
5. Start your with by running `LD_PRELOAD="$(pwd)/libluajit.so $(pwd)/librosaserver.so" ./subrosadedicated.x64`

# Documentation
For complete reference on using the Lua API, go to the [wiki](https://github.com/RosaServer/RosaServer/wiki).

# Building
If you want to build RosaServer yourself and contribute, you can use Visual Studio and WSL, or use CMake on linux itself. You'll have to run `make` inside ./LuaJit first.

---

Thanks to these open source libraries:
- [Sol3](https://github.com/ThePhD/sol2)
- [SubHook](https://github.com/Zeex/subhook)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [TinyCon](https://github.com/unix-ninja/hackersandbox)
