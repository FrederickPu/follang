cmake_minimum_required(VERSION 3.27)
project(follang)

set(CMAKE_CXX_STANDARD 17)

add_executable(follang main.cpp
        parserlexer.cpp
        parserlexer.h)
