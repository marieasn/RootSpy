cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0074 NEW)
project(RootSpy)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(xmsg REQUIRED)
find_package(ROOT REQUIRED)
find_package(Protobuf REQUIRED)
find_package(ZeroMQ REQUIRED)
find_package(CURL REQUIRED)

# cmsg
set(CMSG_ROOT "$ENV{cmsg_ROOT}")
if (NOT CMSG_ROOT)
    message(FATAL_ERROR "Environment variable cmsg_ROOT is not set.")
endif()
set(CMSG_INCLUDE_DIR "${CMSG_ROOT}/Linux-x86_64/include")
add_compile_definitions(HAVE_CMSG=1)
set(CMSG_LIBRARIES cmsgRegex  cMsg  cMsgxx)
set(xmsg_LIBRARIES xmsg)



# Iterate over all variables
get_cmake_property(all_variables VARIABLES)

# Print variables that contain "xmsg"
foreach(var ${all_variables})
    if(var MATCHES "^xmsg_")
        message(STATUS "${var} = ${${var}}")
    endif()
endforeach()

# Include directories for ROOT
include_directories(${ROOT_INCLUDE_DIRS})
include_directories(${xmsg_DIR}/../../../include)
include_directories(${CMSG_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/libRootSpy-client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/libRootSpy)

add_subdirectory(src/libRootSpy)
add_subdirectory(src/libRootSpy-client)
add_subdirectory(src/RSAI)
add_subdirectory(src/RSTimeSeries)
add_subdirectory(src/RSelog)
add_subdirectory(src/RSAggregator)
add_subdirectory(src/RSArchiver)
# add_subdirectory(src/RSMonitor)
# add_subdirectory(src/RSDumpTree)
