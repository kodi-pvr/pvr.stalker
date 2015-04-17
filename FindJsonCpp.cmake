find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules (JSONCPP jsoncpp)
endif()

if(NOT JSONCPP_FOUND)
  find_path(JSONCPP_INCLUDE_DIRS json/json.h
            PATH_SUFFIXES jsoncpp)
  find_library(JSONCPP_LIBRARIES jsoncpp)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JsonCpp DEFAULT_MSG JSONCPP_LIBRARIES JSONCPP_INCLUDE_DIRS)

mark_as_advanced(JSONCPP_INCLUDE_DIRS JSONCPP_LIBRARIES)
