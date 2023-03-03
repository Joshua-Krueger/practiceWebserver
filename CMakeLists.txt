cmake_minimum_required(VERSION 3.24)
project(practiceWebserver C)

set(CMAKE_C_STANDARD 11)

link_libraries(ws2_32 wsock32)

add_executable(practiceWebserver main.c)
