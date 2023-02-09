cmake_minimum_required(VERSION 3.19)
project(Threads2)

set(CMAKE_CXX_STANDARD 17)

add_executable(Threads2 main.cpp myqueue.h myqueue.cpp)