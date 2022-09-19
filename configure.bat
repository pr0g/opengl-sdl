cmake -S . -B build-win ^
-DCMAKE_PREFIX_PATH=%cd%/third-party/sdl/build ^
-DAS_COL_MAJOR=ON -DAS_PRECISION_FLOAT=ON
