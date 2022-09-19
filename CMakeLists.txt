cmake_minimum_required(VERSION 3.15)
project(opengl-sdl)

find_package(SDL2 REQUIRED CONFIG)

include(FetchContent)

FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG b1051403cf81b984f9fc11cc5916adb5afd34766
  SOURCE_SUBDIR cmake)
FetchContent_MakeAvailable(glad)

list(APPEND CMAKE_MODULE_PATH ${glad_SOURCE_DIR})

glad_add_library(glad_gl_core_41 REPRODUCIBLE API gl:core=4.1)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main
                                              glad_gl_core_41)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
