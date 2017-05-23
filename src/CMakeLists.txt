# Name of the executable
SET(NAME_EXECUTABLE ubahn)

# Add basic source Files
SET(SOURCE_FILES
	ubahn.cpp
	graph_builder.cpp
	io/xml_reader.cpp
	solver/euler.cpp
	solver/cplex_solver.cpp
	solver/station_solver.cpp
)

ADD_EXECUTABLE(${NAME_EXECUTABLE} ${SOURCE_FILES})

# all Language should output all warnings
ADD_DEFINITIONS(-Wall -Wextra)
# switch off some annoying warnings
ADD_DEFINITIONS(-Wno-unused-parameter -Wno-sign-compare -Wno-ignored-attributes -Wno-misleading-indentation)

# Set C++ flags
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG -pipe")
SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

# strip executable in Release
IF(CMAKE_BUILD_TYPE STREQUAL "Release")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
ENDIF()

# listing header files as descendants of this directory should be possible
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# add the libraries and includes
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(${NAME_EXECUTABLE} ${Boost_LIBRARIES})

INCLUDE_DIRECTORIES(${LEDA_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(${NAME_EXECUTABLE} ${LEDA_LIBRARIES})

INCLUDE_DIRECTORIES(${XERCES_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(${NAME_EXECUTABLE} ${XERCES_LIBRARY})

INCLUDE_DIRECTORIES(${Concert_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${NAME_EXECUTABLE} ${Concert_LIBRARIES})
