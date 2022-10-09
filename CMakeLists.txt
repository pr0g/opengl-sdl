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
  GIT_TAG 7de421efc5f6edcdf325be91db7eafb897558f88)
FetchContent_Declare(
  as-camera-input
  GIT_REPOSITORY https://github.com/pr0g/as-camera-input.git
  GIT_TAG d47bd06a9dbcfa3f5cb92cdab146d599f0e0b978)

FetchContent_MakeAvailable(glad as as-camera-input)

list(APPEND CMAKE_MODULE_PATH ${glad_SOURCE_DIR})

glad_add_library(glad_gl_core_46 REPRODUCIBLE API gl:core=4.6)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE main.cpp)
target_link_libraries(
  ${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main glad_gl_core_46 as
                          as-camera-input)
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
