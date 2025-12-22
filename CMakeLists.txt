cmake_minimum_required(VERSION 3.5)
project(PCAD CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra -Werror -funroll-loops -fno-strict-aliasing -flto=auto")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SimGrid REQUIRED IMPORTED_TARGET simgrid)

include(GNUInstallDirs)

add_library(pcad SHARED create-pcad.cpp)
target_link_libraries(pcad PUBLIC PkgConfig::SimGrid)

install(TARGETS pcad DESTINATION ${CMAKE_INSTALL_LIBDIR})
