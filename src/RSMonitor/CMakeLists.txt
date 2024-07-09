# Minimum required CMake version
cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0074 NEW)

# Project name
project(RSMonitor)
link_directories(${ROOT_LIBRARY_DIR})
link_directories(${CMSG_ROOT}/Linux-x86_64/lib)
link_directories(${xmsg_DIR}/../..)


# Add executable for RSMonitor
add_executable(RSMonitor RSMonitor.cc rsmon_cmsg.cc)

# Link with libraries (assuming libRootSpy-client is built as a library)
target_link_libraries(RSMonitor RootSpy-client ${ROOT_LIBRARIES} ${CMSG_LIBRARIES} ${xmsg_LIBRARIES} ${Protobuf_LIBRARIES} ${CURL_LIBRARIES})
