cmake_minimum_required(VERSION 3.5)
project(stalkerclient)

find_package(ZLIB REQUIRED)
find_package(LibXml2 REQUIRED)

include_directories(${LIBXML2_INCLUDE_DIR})

set(LIBSTALKER_SOURCES
        identity.c
        itv.c
        list.c
        param.c
        request.c
        stb.c
        util.c
        watchdog.c
        xmltv.c)

add_library(stalkerclient STATIC ${LIBSTALKER_SOURCES})
target_link_libraries(stalkerclient ${ZLIB_LIBRARIES} ${LIBXML2_LIBRARIES})
set_property(TARGET stalkerclient PROPERTY POSITION_INDEPENDENT_CODE ON)
