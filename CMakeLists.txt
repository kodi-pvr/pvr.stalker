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
        src/stalker/base/Cache.cpp)

set(SOURCES
        src/addon.cpp
        src/StalkerInstance.cpp
        src/stalker/AddonSettings.cpp
        src/stalker/ChannelManager.cpp
        src/stalker/CWatchdog.cpp
        src/stalker/GuideManager.cpp
        src/stalker/HTTPSocket.cpp
        src/stalker/InstanceSettings.cpp
        src/stalker/SAPI.cpp
        src/stalker/SessionManager.cpp
        src/stalker/SettingsMigration.cpp
        src/stalker/Utils.cpp
        src/stalker/XMLTV.cpp)

set(STALKER_SOURCES
        ${BASE_SOURCES}
        ${SOURCES})

set(STALKER_HEADERS
        src/addon.h
        src/StalkerInstance.h
        src/stalker/AddonSettings.h
        src/stalker/ChannelManager.h
        src/stalker/CWatchdog.h
        src/stalker/Error.h
        src/stalker/GuideManager.h
        src/stalker/HTTPSocket.h
        src/stalker/InstanceSettings.h
        src/stalker/SAPI.h
        src/stalker/SessionManager.h
        src/stalker/SettingsMigration.h
        src/stalker/Utils.h
        src/stalker/XMLTV.h
        src/stalker/base/Cache.h
        src/stalker/base/ChannelManager.h
        src/stalker/base/GuideManager.h)

build_addon(pvr.stalker STALKER DEPLIBS)

include(CPack)
