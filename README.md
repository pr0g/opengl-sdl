# opengl-sdl

Simple OpenGL test scene, initially experimenting with reverse z projection.

## Prerequisites

You'll need the normal stuff like CMake, a compiler (e.g. Visual Studio/MSVC, GCC, Clang etc...) to really get started, below are the additional things you might bump into.

### Python

When trying to configure the repo initially, you may hit this error:

```bash
CMake Error at C:/Program Files/CMake/share/cmake-3.25/Modules/FindPackageHandleStandardArgs.cmake:230 (message):
  Could NOT find PythonInterp (missing: PYTHON_EXECUTABLE)
Call Stack (most recent call first):
  C:/Program Files/CMake/share/cmake-3.25/Modules/FindPackageHandleStandardArgs.cmake:600 (_FPHSA_FAILURE_MESSAGE)
  C:/Program Files/CMake/share/cmake-3.25/Modules/FindPythonInterp.cmake:169 (FIND_PACKAGE_HANDLE_STANDARD_ARGS)
  build/_deps/glad-src/cmake/CMakeLists.txt:44 (find_package)
```

This is most likely because Python can't be found on your system. You'll need to go ahead and [download]((https://www.python.org/downloads/)) it first.

If you configure again, you should then hopefully see something that looks like this...

```bash
Found PythonInterp: C:/Users/hultonha/AppData/Local/Programs/Python/Python311/python.exe (found version "3.11")
-- Glad Library 'glad_gl_core_46'
```

Configuring should now be complete, but when attempting to build it's possible you'll hit an error with a missing dependency (a Python one which is required by Glad), `Jinja2`.

To install this find wherever pip (Python's package manager) is on your system (it will have been installed with your previous Python installation, most likely in your AppData folder on Windows).

Then run `pip install jinja2` to have Python install that dependency for you (you might want to add pip to your Path environment variable too for easy access).

Once that's installed, building the project should work as expected.

## Setup and configuration

### Windows

To ensure the MSVC compiler (and programs such as Ninja) are found correctly, it's best to run the Visual Studio Developer Command Prompt (you can find this in All Programs usually). If you'd rather use a different terminal such as [cmder](https://cmder.app/) or [Microsoft Terminal](https://apps.microsoft.com/store/detail/windows-terminal/9N0DX20HK701?hl=en-gb&gl=gb) then ensure you run `VsDevCmd.bat` which sets the Visual Studio related environment variables (essentially what happens when launching the Visual Studio Developer Command Prompt). It is located here `Program Files\Microsoft Visual Studio\Version\Common7\Tools` or `Program Files (x86)\Microsoft Visual Studio\Version\Common7\Tools`.

### General

Run `configure.bat/sh` to have CMake configure the project with the required settings/arguments. A superbuild project is used to ensure third party dependencies (SDL2) are downloaded and built as part of the normal build. By default the scripts use Ninja but it's possible to use whichever generator you prefer (Ninja is selected purely because it's consistent across Window/macOS/Linux).

Note: If you wish to build SDL2 separately from the third-party folder, pass `-DCMAKE_PREFIX_PATH` with the path to the SDL2 install folder when configuring the main project so it can find SDL2 (this is handled transparently with the `-DSUPERBUILD` option and isn't explicitly required).
