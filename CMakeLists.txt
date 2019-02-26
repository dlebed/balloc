cmake_minimum_required (VERSION 2.6)
project (balloc)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -std=c99 -O0 -ggdb -g3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=undefined")

add_executable(balloc main.c balloc.c)