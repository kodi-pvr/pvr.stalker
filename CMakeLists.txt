project(pvr.stalker)

cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)
find_package(TinyXML REQUIRED)
find_package(JsonCpp REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/lib
                    ${TINYXML_INCLUDE_DIRS}
                    ${JSONCPP_INCLUDE_DIRS})

add_subdirectory(lib/libstalkerclient)

set(DEPLIBS ${p8-platform_LIBRARIES} stalkerclient ${TINYXML_LIBRARIES} ${JSONCPP_LIBRARIES})

if(WIN32)
  list(APPEND DEPLIBS ws2_32)
endif()

set(STALKER_SOURCES src/client.cpp
                    src/CWatchdog.cpp
                    src/HTTPSocket.cpp
                    src/SAPI.cpp
                    src/SData.cpp
                    src/Utils.cpp
                    src/XMLTV.cpp)

build_addon(pvr.stalker STALKER DEPLIBS)

include(CPack)
