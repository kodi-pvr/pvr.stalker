project(pvr.stalker.client)

cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(TinyXML REQUIRED)
find_package(JsonCpp REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/lib
                    ${TINYXML_INCLUDE_DIRS}
                    ${JSONCPP_INCLUDE_DIRS})

add_subdirectory(lib/libstalkerclient)

set(DEPLIBS ${kodiplatform_LIBRARIES} stalkerclient ${TINYXML_LIBRARIES} ${JSONCPP_LIBRARIES})

if(WIN32)
  list(APPEND DEPLIBS ws2_32)
endif()

set(SC_SOURCES src/client.cpp
               src/HTTPSocket.cpp
               src/SAPI.cpp
               src/SData.cpp)

build_addon(pvr.stalker.client SC DEPLIBS)

include(CPack)
