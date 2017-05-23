cmake_minimum_required(VERSION 2.6)

PROJECT(ubahn CXX C)

# handle includes
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake)
INCLUDE(EnableCXX11)

# build in Release-mode by default if not explicitly set
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

# get the most recent version of the Google C++ Style Guide Checker
IF (NOT EXISTS "${PROJECT_SOURCE_DIR}/tools/cpplint.py")
  FILE(DOWNLOAD https://raw.githubusercontent.com/google/styleguide/gh-pages/cpplint/cpplint.py "${PROJECT_SOURCE_DIR}/tools/cpplint.py")
ENDIF ()

#--- BOOST ---
find_package(Boost REQUIRED)

#--- XERCES ---
find_package(XERCES REQUIRED)

#--- LEDA ---
find_package(LEDA REQUIRED)

#--- Concert ---
find_package(Concert REQUIRED)

#build the source
add_subdirectory(src)

# print the value of a variable if it is set
macro(print_set VAR)
  IF(${VAR})
    MESSAGE(STATUS " ${VAR} = ${${VAR}}")
  ENDIF(${VAR})
endmacro(print_set VAR)

MESSAGE(STATUS)
MESSAGE(STATUS "-------------------------------------------------------------------------------")
MESSAGE(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "Libraries:")
PRINT_SET(XERCES_ROOT)
PRINT_SET(LEDA_ROOT)
PRINT_SET(CPLEX_ROOT)
PRINT_SET(Concert_ROOT)
MESSAGE(STATUS)
MESSAGE(STATUS "Change a value with: cmake -D<Variable>=<Value>")
MESSAGE(STATUS "-------------------------------------------------------------------------------")
MESSAGE(STATUS)
