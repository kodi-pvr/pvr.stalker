cmake_minimum_required(VERSION 3.5)

project(libxml2 VERSION 2.9.10)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if (APPLE)
  # For some reason libxml2 requires this flag or it will create an insane amout of log output on compile
  list(APPEND libxml2_conf CFLAGS=-Wno-nullability-completeness)
endif()
list(APPEND libxml2_conf CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include)
list(APPEND libxml2_conf LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib)

include(ExternalProject)
externalproject_add(libxml2
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --without-python
                      --without-iconv
                      --without-http
                      --without-ftp
                      --without-catalog
                      --without-modules
                      --disable-shared
                      --with-pic
                      ${libxml2_conf})

externalproject_add_step(libxml2
                         RunSedCommand
                         COMMAND echo "Removing executable from makefile.in using BBB sed ${CMAKE_CURRENT_SOURCE_DIR}"
                         COMMAND bash -c "sed -ie 's|runtest$(EXEEXT)||' ${CMAKE_CURRENT_SOURCE_DIR}/Makefile.in"
                         COMMAND bash -c "sed -ie 's|testrecurse$(EXEEXT)||' ${CMAKE_CURRENT_SOURCE_DIR}/Makefile.in"
                         DEPENDERS configure)

install(CODE "Message(Done)")
