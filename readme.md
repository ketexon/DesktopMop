# DesktopMop

DesktopMop is a simple WinAPI GUI Tool that cleans the desktop of certain file patterns. The primary purpose of this is to remove any unwanted shortcuts  (`.lnk` files) created by applications during their installation.

## Building

There will be prebuilt binaries in the releases section.

DesktopMop only works on Windows, and it is only tested on Windows 11 64 bit (I'm not sure if it supports 32 bit, but it should work on Windows 10). You will need a C++ compiler supporting C++20 and CMake (the CMakeLists only supports MSVC and maybe clang-cl due to the implicit linking of like `kernel32.lib`, though feel free to PR). I'm not sure if you need to install the Windows SDK, but I don't think so.

Then, build the CMake project however you would usually. I use VSCode's CMake integration. Make sure the generator is VisualStudio if you are using MSVC. 

There are a few compile options that, right now, you'd have to manually change in CMake. They are all specified near the bottom in `target_compile_definitions`:

- `DESKTOPMOP_DEBUG` does nothing
- `DESKTOPMOP_SHOW_CONSOLE`: if 0, disables console creation
- `DESKTOPMOP_CLEAR_CONFIG`: if 1, deletes the `settings.cfg` file. This treats each launch as a new launch.
- `DESKTOPMOP_LOG_LEVEL`: what level of logs to show (in console only). Levels are 0: nothing, 1: error, 2: warning, 3: info, 4: debug

## Usage

The app will first prompt you if you want to add DesktopMop to Start Menu, Startup, and (ironically) Desktop folders. At any time, you can get these popups again by deleting the "initialized=1" in the `settings.cfg` file in the data folder (File -> Open Data Folder).

Then, you can add and delete from the two lists, Blacklisted files and Whitelisted files, by right clicking and pressing add. You can delete any of these by right clicking and pressing delete. The UI is a little scuffed (raw WinAPI moment). The format for these entries is regex (see below if you need help). 

Note that files are delete *only if* they are blacklisted and not whitelisted. If you love DesktopMop very much, for example, you can safely do something like blacklist `.*\.lnk` but whitelist `DesktopMop\.lnk` to keep DesktopMop on the desktop.

### Regex

Both of the blacklisted and whitelisted files use Regex format (if you need help, [this website]([url](https://regexr.com/)) is a cool playground for regex. Some basic patterns that I would use are:

- `filename`
- `filename\.exe`
- `.*\.lnk`

## License

The license is MIT. Note that the icon may not be MIT. I honestly don't know why the icon is something that *\*cough\* \*cough\* looks like* Miku, there was probably a gas leak in my house.
