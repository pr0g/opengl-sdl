cmake_minimum_required(VERSION 3.15)
project(opengl-sdl)

find_package(SDL2 REQUIRED CONFIG)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE main.cpp third-party/glad/src/gl.c)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE third-party/glad/include)
