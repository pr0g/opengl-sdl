@echo off

cmake -B build/debug -G Ninja ^
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
-DCMAKE_BUILD_TYPE=Debug ^
-DCMAKE_PREFIX_PATH=%cd%/third-party/sdl/build ^
-DAS_COL_MAJOR=ON -DAS_PRECISION_FLOAT=ON

cmake -B build/release -G Ninja ^
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
-DCMAKE_BUILD_TYPE=Release ^
-DCMAKE_PREFIX_PATH=%cd%/third-party/sdl/build ^
-DAS_COL_MAJOR=ON -DAS_PRECISION_FLOAT=ON
