cmake_minimum_required(VERSION 2.8)
include_directories(${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/..)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fsanitize=address")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
add_executable(testshared test.cpp)
