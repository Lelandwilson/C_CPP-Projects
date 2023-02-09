cmake_minimum_required(VERSION 3.19)
project(Threads3)

set(CMAKE_CXX_STANDARD 17)

add_executable(Threads3 main.cpp myqueue.h myqueue.cpp)