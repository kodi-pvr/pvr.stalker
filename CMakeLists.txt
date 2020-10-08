cmake_minimum_required(VERSION 3.5)
project(pvr.stalker)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(JsonCpp REQUIRED)

include_directories(
        ${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways
        ${PROJECT_SOURCE_DIR}/lib
        ${LIBXML2_INCLUDE_DIR}
        ${JSONCPP_INCLUDE_DIRS})

if(WIN32)
  add_compile_options(/wd4996)
endif()

add_subdirectory(lib/libstalkerclient)

set(DEPLIBS
        stalkerclient
        ${LIBXML2_LIBRARIES}
        ${JSONCPP_LIBRARIES})

set(BASE_SOURCES
        src/base/Cache.cpp)

set(SOURCES
        src/ChannelManager.cpp
        src/CWatchdog.cpp
        src/GuideManager.cpp
        src/HTTPSocket.cpp
        src/SAPI.cpp
        src/SData.cpp
        src/SessionManager.cpp
        src/Utils.cpp
        src/XMLTV.cpp)

set(STALKER_SOURCES
        ${BASE_SOURCES}
        ${SOURCES})

set(STALKER_HEADERS
        src/ChannelManager.h
        src/CWatchdog.h
        src/Error.h
        src/GuideManager.h
        src/HTTPSocket.h
        src/SAPI.h
        src/SData.h
        src/SessionManager.h
        src/Settings.h
        src/Utils.h
        src/XMLTV.h
        src/base/Cache.h
        src/base/ChannelManager.h
        src/base/GuideManager.h)

build_addon(pvr.stalker STALKER DEPLIBS)

include(CPack)
