# opengl-sdl

Simple OpenGL test scene, initially experimenting with reverse z projection.

## Prerequisites

You'll need the normal stuff like CMake, a compiler (e.g. Visual Studio/MSVC, GCC, Clang etc...) to really get started, below are the additional things you might bump into.

### Python

When trying to configure the repo initially, you may hit this error:

```
CMake Error at C:/Program Files/CMake/share/cmake-3.25/Modules/FindPackageHandleStandardArgs.cmake:230 (message):
  Could NOT find PythonInterp (missing: PYTHON_EXECUTABLE)
Call Stack (most recent call first):
  C:/Program Files/CMake/share/cmake-3.25/Modules/FindPackageHandleStandardArgs.cmake:600 (_FPHSA_FAILURE_MESSAGE)
  C:/Program Files/CMake/share/cmake-3.25/Modules/FindPythonInterp.cmake:169 (FIND_PACKAGE_HANDLE_STANDARD_ARGS)
  build/_deps/glad-src/cmake/CMakeLists.txt:44 (find_package)
```

This is most likely because Python can't be found on your system. You'll need to go ahead and download it first ([link](https://www.python.org/downloads/)).

If you configure again, you should then hopefully see something that looks like this...

```
Found PythonInterp: C:/Users/hultonha/AppData/Local/Programs/Python/Python311/python.exe (found version "3.11")
-- Glad Library 'glad_gl_core_46'
```

Configuring should now be complete, but when attempting to build it's possible you'll hit an error with a missing dependency (a Python one which is required by Glad), `Jinja2`.

To install this find wherever pip (Python's package manager) is on your system (it will have been installed with your previous Python installation, most likely in your AppData folder on Windows).

Then run `pip install jinja2` to have Python install that dependency for you (you might want to add pip to your Path environment variable too for easy access).

Once that's installed, building the project should work as expected.