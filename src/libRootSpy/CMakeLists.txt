# Minimum required CMake version

cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0074 NEW)
project(libRootSpy)
#
#Root Installation location
#set(CMAKE_PREFIX_PATH /opt/usr/local/bin)

#set(ROOT_DIR /opt/usr/local/bin)

# Find required packages
find_package(ROOT REQUIRED)
find_package(xmsg REQUIRED)
#find_package(cmsg_REQUIRED)
#message(STATUS "xsmg_INCLUDE_DIRS=${xsmg_INCLUDE_DIRS}")

include_directories(${ROOT_INCLUDE_DIRS})
include_directories(${xmsg_DIR}/../../../include)
include_directories(/home/angen/work/2024.06.16.RootSpy/RootSpy/xmsgbuild/opt/usr/local/Linux-x86_64/include)
# include_directories(home/angen/work/2024.06.16.RootSpy/RootSpy/xmsgbuild/opt/usr/local/include)

# Define the library target
add_library(RootSpy DRootSpy.cc)

# Include directories for ROOT
message(STATUS "xsmg_INCLUDE_DIRS=${xsmg_INCLUDE_DIRS}")

# Iterate over all variables
get_cmake_property(all_variables VARIABLES)

# Print variables that contain "xmsg"
foreach(var ${all_variables})
    if(var MATCHES "^cmsg_")
        message(STATUS "${var} = ${${var}}")
    endif()
endforeach()

# Assuming you have a target named 'your_target_name'
target_compile_options(RootSpy PRIVATE -fpermissive)


# Optionally, set the C++ standard if needed
#target_compile_features(printmsg PUBLIC cxx_std_11)

# Optionally, specify include directories if needed
#target_include_directories(libRootSpy PRIVATE "/home/angen/work/2024.06.16.RootSpy/RootSpy/xmsgbuild/opt/usr/local/include")

