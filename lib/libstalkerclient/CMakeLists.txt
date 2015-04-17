project(stalkerclient)

cmake_minimum_required(VERSION 2.6)

enable_language(C)

set(SOURCES itv.c
            param.c
            request.c
            stb.c
            util.c)

add_library(stalkerclient STATIC ${SOURCES})

if(NOT WIN32)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
else()
  target_link_libraries(stalkerclient ws2_32)
endif()
