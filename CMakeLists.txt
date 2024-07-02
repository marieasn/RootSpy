cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0074 NEW)
project(RootSpy)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(xmsg REQUIRED)
find_package(ROOT REQUIRED)

# Include directories for ROOT
include_directories(${ROOT_INCLUDE_DIRS})

# Library: libRootSpy
add_library(RootSpy_lib
    src/libRootSpy/YourSourceFilesHere.cpp
)

target_include_directories(RootSpy_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libRootSpy
)

# Library: libRootSpy-client
add_library(RootSpy_client_lib
    src/libRootSpy-client/YourSourceFilesHere.cpp
)

target_include_directories(RootSpy_client_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libRootSpy-client
)

# Programs
set(PROGRAMS
    RSAI
    RSAggregator
    RSTimeSeries
    RSMonitor
    RSDumpTree
    RSArchiver
    RootSpy
)

foreach(PROGRAM ${PROGRAMS})
    add_executable(${PROGRAM}
        src/${PROGRAM}/YourSourceFilesHere.cpp
    )

    target_link_libraries(${PROGRAM}
        PRIVATE
            RootSpy_lib
            RootSpy_client_lib
            xmsg-cpp
            ${ROOT_LIBRARIES}
    )

    target_include_directories(${PROGRAM}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/${PROGRAM}
    )
endforeach()

