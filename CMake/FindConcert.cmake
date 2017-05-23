# - Try to find Concert
# http://www.ibm.com/software/products/us/en/ibmilogcpleoptistud/
#
# Once done, this will define
#
#  Concert_INCLUDE_DIRS   - where to find ilcplex/ilocplex.h, etc.
#  Concert_LIBRARIES      - List of libraries when using Concert.
#  Concert_FOUND          - True if Concert found.
#
#  Concert_VERSION        - The version of Concert found
#
# An includer may set Concert_ROOT to a Concert installation root to tell
# this module where to look.
#
# Author:
# Wolfgang A. Welz <welz@math.tu-berlin.de>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

include(LibFindMacros)

# Depends on Threads
libfind_package(Concert CPLEX)

# If Concert_ROOT is not set, look for the environment variable
if(NOT Concert_ROOT AND NOT "$ENV{Concert_ROOT}" STREQUAL "")
  set(Concert_ROOT $ENV{Concert_ROOT})
endif()

set(_Concert_SEARCHES)

# Search Concert_ROOT first if it is set.
if(Concert_ROOT)
  set(_Concert_SEARCH_ROOT PATHS ${Concert_ROOT} NO_DEFAULT_PATH)
  list(APPEND _Concert_SEARCHES _Concert_SEARCH_ROOT)
endif()

# Normal search.
set(_Concert_SEARCH_NORMAL
  PATHS ""
)
list(APPEND _Concert_SEARCHES _Concert_SEARCH_NORMAL)

# Try each search configuration.
foreach(search ${_Concert_SEARCHES})
  FIND_PATH(Concert_INCLUDE_DIR NAMES ilconcert/iloenv.h ${${search}} PATH_SUFFIXES include)
  FIND_LIBRARY(Concert_LIBRARY NAMES concert ${${search}} PATH_SUFFIXES lib lib/x86-64_sles10_4.1/static_pic)
endforeach()

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Concert_PROCESS_INCLUDES Concert_INCLUDE_DIR CPLEX_INCLUDE_DIRS)
set(Concert_PROCESS_LIBS Concert_LIBRARY CPLEX_LIBRARIES)
libfind_process(Concert)

ADD_DEFINITIONS(-DILOUSESTL -DILOSTRINGSTL -DIL_STD)
