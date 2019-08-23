cmake_minimum_required(VERSION 3.5)
project(jsoncpp)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES src/lib_json/json_reader.cpp
            src/lib_json/json_value.cpp
            src/lib_json/json_writer.cpp)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(jsoncpp ${SOURCES})

install(TARGETS jsoncpp DESTINATION ${CMAKE_INSTALL_PREFIX}/lib) 
install(DIRECTORY include/json DESTINATION ${CMAKE_INSTALL_PREFIX}/include/jsoncpp)
