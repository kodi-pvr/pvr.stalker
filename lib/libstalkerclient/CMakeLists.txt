project(stalkerclient)

cmake_minimum_required(VERSION 2.6)

enable_language(C)

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

if (NOT WIN32)
    set_target_properties(stalkerclient PROPERTIES COMPILE_FLAGS "-fPIC")
endif ()
