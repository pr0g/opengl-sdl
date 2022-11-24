cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0135 NEW)

if(NOT SUPERBUILD)
  project(sdl2)
endif()

include(ExternalProject)

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT isMultiConfig)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE
        Debug
        CACHE STRING "" FORCE)
  endif()
  set(build_type_dir ${CMAKE_BUILD_TYPE})
  set(build_type_arg -DCMAKE_BUILD_TYPE=$<CONFIG>)
endif()

if(SUPERBUILD)
  set(PREFIX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/sdl/build)
else()
  set(PREFIX_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

ExternalProject_Add(
  SDL2
  URL https://www.libsdl.org/release/SDL2-2.0.20.tar.gz
  URL_HASH MD5=a53acc02e1cca98c4123229069b67c9e
  PREFIX ${PREFIX_DIR}
  BINARY_DIR ${PREFIX_DIR}/src/SDL2-build/${build_type_dir}
  CMAKE_ARGS ${build_type_arg} -D CMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
  INSTALL_COMMAND cmake --build <BINARY_DIR> --target install
                  ${build_config_arg})
