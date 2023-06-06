cmake_minimum_required(VERSION 3.22)
project(webserver_files C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(webserver_files
#        client.c
#        output.c
        request.c
        request.h
        segel.c
        segel.h
        server.c
        connfdList.c
        connfdList.h)
