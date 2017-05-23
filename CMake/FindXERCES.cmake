# - Try to find Xerces-C
# Once done this will define
#
# XERCES_FOUND - system has Xerces-C
# XERCES_INCLUDE_DIR - the Xerces-C include directory
# XERCES_LIBRARY - link these to use Xerces-C
# XERCES_VERSION - Xerces-C found version
#
# An includer may set XERCES_ROOT to a Xerrces-C installation root to tell
# this module where to look.
#
# Author:
# Wolfgang A. Welz <welz@math.tu-berlin.de>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

include(LibFindMacros)

# If XERCES_ROOT is not set, look for the environment variable
if(NOT XERCES_ROOT AND NOT "$ENV{XERCES_ROOT}" STREQUAL "")
  set(XERCES_ROOT $ENV{XERCES_ROOT})
endif()

set(_XERCES_SEARCHES)

# Search XERCES_ROOT first if it is set
if(XERCES_ROOT)
  set(_XERCES_SEARCH_ROOT PATHS ${XERCES_ROOT} NO_DEFAULT_PATH)
  list(APPEND _XERCES_SEARCHES _XERCES_SEARCH_ROOT)
endif()

# Normal search
set(_XERCES_SEARCH_NORMAL
  PATHS ""
)
list(APPEND _XERCES_SEARCHES _XERCES_SEARCH_NORMAL)

# Try each search configuration
foreach(search ${_XERCES_SEARCHES})
  FIND_PATH(XERCES_INCLUDE_DIR NAMES xercesc/util/XercesVersion.hpp ${${search}} PATH_SUFFIXES include)
  FIND_LIBRARY(XERCES_LIBRARY NAMES xerces-c ${${search}} PATH_SUFFIXES lib)
endforeach()

IF(XERCES_INCLUDE_DIR AND EXISTS "${XERCES_INCLUDE_DIR}/xercesc/util/XercesVersion.hpp")
  FILE(READ "${XERCES_INCLUDE_DIR}/xercesc/util/XercesVersion.hpp" XVERHPP)

  STRING(REGEX MATCHALL "\n *#define XERCES_VERSION_MAJOR +[0-9]+" XVERMAJ ${XVERHPP})
  STRING(REGEX MATCH "\n *#define XERCES_VERSION_MINOR +[0-9]+" XVERMIN ${XVERHPP})
  STRING(REGEX MATCH "\n *#define XERCES_VERSION_REVISION +[0-9]+" XVERREV ${XVERHPP})

  STRING(REGEX REPLACE "\n *#define XERCES_VERSION_MAJOR +" "" XVERMAJ ${XVERMAJ})
  STRING(REGEX REPLACE "\n *#define XERCES_VERSION_MINOR +" "" XVERMIN ${XVERMIN})
  STRING(REGEX REPLACE "\n *#define XERCES_VERSION_REVISION +" "" XVERREV ${XVERREV})

  SET(XERCES_VERSION ${XVERMAJ}.${XVERMIN}.${XVERREV})
ENDIF()

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(XERCES_PROCESS_INCLUDES XERCES_INCLUDE_DIR)
set(XERCES_PROCESS_LIBS XERCES_LIBRARY)
libfind_process(XERCES)
