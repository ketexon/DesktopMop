# DesktopMop

DesktopMop is a simple WinAPI GUI Tool that cleans the desktop of certain filetypes. The primary purpose of this is to remove any `.lnk` files created by applications if you do not want Desktop Shortcuts.

## Building

This application only works on Windows, and it is only tested on windows 11. You will need a C++ compiler supporting C++20 and CMake (the CMakeLists only supports MSVC and maybe clang-cl, though feel free to PR). I'm not sure if you need to install Windows SDK, but I don't think so.

Then, build the CMake project however you would usually. I use VSCode's CMake integration.