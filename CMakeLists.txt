cmake_minimum_required(VERSION 3.14)

project(hueCommand VERSION 0.1)

add_executable(hueCommand src/hueCommand.c src/cJSON/cJSON.c src/requestHandler.c)

target_link_libraries(hueCommand PUBLIC curl)

