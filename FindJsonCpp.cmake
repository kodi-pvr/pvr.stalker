find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_JSONCPP jsoncpp)
endif()

find_path(JSONCPP_INCLUDE_DIRS json/json.h
                               PATHS ${PC_JSONCPP_INCLUDEDIR}
                               PATH_SUFFIXES jsoncpp)
find_library(JSONCPP_LIBRARIES jsoncpp
                               PATHS ${PC_JSONCPP_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JsonCpp REQUIRED_VARS JSONCPP_LIBRARIES JSONCPP_INCLUDE_DIRS)

mark_as_advanced(JSONCPP_INCLUDE_DIRS JSONCPP_LIBRARIES)
