cmake_minimum_required(VERSION 3.15)
project(opengl-sdl)

find_package(SDL2 REQUIRED CONFIG)

include(FetchContent)

FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG b1051403cf81b984f9fc11cc5916adb5afd34766
  SOURCE_SUBDIR cmake)
FetchContent_Declare(
  as
  GIT_REPOSITORY https://github.com/pr0g/as.git
  GIT_TAG 82c15519e8d9701a04359495639322dd49c03c8c)
FetchContent_Declare(
  as-camera-input-sdl
  GIT_REPOSITORY https://github.com/pr0g/as-camera-input-sdl.git
  GIT_TAG a2c53f7ef12cea081b632d559477f1549055a080)
FetchContent_Declare(
  imgui.cmake
  GIT_REPOSITORY https://github.com/pr0g/imgui.cmake.git
  GIT_TAG e74e32c2673e7e5cf559d63292a74e1df5725f02)

FetchContent_MakeAvailable(glad as as-camera-input-sdl imgui.cmake)

list(APPEND CMAKE_MODULE_PATH ${glad_SOURCE_DIR})

glad_add_library(glad_gl_core_46 REPRODUCIBLE API gl:core=4.6)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE main.cpp imgui/imgui_impl_opengl3.cpp
                                       imgui/imgui_impl_sdl.cpp)
target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
target_link_libraries(
  ${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main glad_gl_core_46 as
                          as-camera-input-sdl imgui.cmake)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_definitions(
  ${PROJECT_NAME}
  PRIVATE $<$<BOOL:${AS_PRECISION_FLOAT}>:AS_PRECISION_FLOAT>
          $<$<BOOL:${AS_PRECISION_DOUBLE}>:AS_PRECISION_DOUBLE>
          $<$<BOOL:${AS_COL_MAJOR}>:AS_COL_MAJOR>
          $<$<BOOL:${AS_ROW_MAJOR}>:AS_ROW_MAJOR>)

if(WIN32)
  # copy the SDL2.dll to the same folder as the executable
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    VERBATIM)
endif()
