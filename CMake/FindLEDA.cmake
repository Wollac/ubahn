# - Try to find LEDA
# http://www.ibm.com/software/products/us/en/ibmilogcpleoptistud/
#
# Once done, this will define
#
#  LEDA_INCLUDE_DIRS   - where to find ilcplex/ilocplex.h, etc.
#  LEDA_LIBRARIES      - List of libraries when using LEDA.
#  LEDA_FOUND          - True if LEDA found.
#
# An includer may set LEDA_ROOT to a LEDA installation root to tell
# this module where to look.
#
# Author:
# Wolfgang A. Welz <welz@math.tu-berlin.de>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

include(LibFindMacros)

# If LEDA_ROOT is not set, look for the environment variable
if(NOT LEDA_ROOT AND NOT "$ENV{LEDA_ROOT}" STREQUAL "")
  set(LEDA_ROOT $ENV{LEDA_ROOT})
endif()

set(_LEDA_SEARCHES)

# Search LEDA_ROOT first if it is set.
if(LEDA_ROOT)
  set(_LEDA_SEARCH_ROOT PATHS ${LEDA_ROOT} NO_DEFAULT_PATH)
  list(APPEND _LEDA_SEARCHES _LEDA_SEARCH_ROOT)
endif()

# Normal search.
set(_LEDA_SEARCH_NORMAL
  PATHS ""
)
list(APPEND _LEDA_SEARCHES _LEDA_SEARCH_NORMAL)

# Try each search configuration.
foreach(search ${_LEDA_SEARCHES})
  FIND_PATH(LEDA_INCLUDE_DIR NAMES LEDA/graph/graph.h ${${search}} PATH_SUFFIXES incl include)
  FIND_LIBRARY(LEDAG_LIBRARY NAMES G ${${search}} PATH_SUFFIXES lib)
  FIND_LIBRARY(LEDAL_LIBRARY NAMES L ${${search}} PATH_SUFFIXES lib)
endforeach()

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(LEDA_PROCESS_INCLUDES LEDA_INCLUDE_DIR)
set(LEDA_PROCESS_LIBS LEDAG_LIBRARY LEDAL_LIBRARY)
libfind_process(LEDA)

ADD_DEFINITIONS(-fno-strict-aliasing)
