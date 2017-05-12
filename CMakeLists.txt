project(pvr.stalker)

cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(JsonCpp REQUIRED)

include_directories(
        ${kodiplatform_INCLUDE_DIRS}
        ${p8-platform_INCLUDE_DIRS}
        ${KODI_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/lib
        ${LIBXML2_INCLUDE_DIR}
        ${JSONCPP_INCLUDE_DIRS})

add_subdirectory(lib/libstalkerclient)

set(DEPLIBS
        ${p8-platform_LIBRARIES}
        stalkerclient
        ${LIBXML2_LIBRARIES}
        ${JSONCPP_LIBRARIES})

set(BASE_SOURCES
        src/base/Cache.cpp)

set(SOURCES
        src/client.cpp
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
        src/client.h
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
