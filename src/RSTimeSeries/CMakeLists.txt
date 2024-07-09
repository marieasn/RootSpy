
# Minimum required CMake version
cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0074 NEW)

# Project name
project(RSTimeSeries)
link_directories(${ROOT_LIBRARY_DIR})
link_directories(${CMSG_ROOT}/Linux-x86_64/lib)
link_directories(${xmsg_DIR}/../..)


# Add executable for RSTimeSeries
add_executable(RSTimeSeries RSTimeSeries.cc)

# Link with libraries (assuming libRootSpy-client is built as a library)
target_link_libraries(RSTimeSeries RootSpy-client ${ROOT_LIBRARIES} ${CMSG_LIBRARIES} ${xmsg_LIBRARIES} ${Protobuf_LIBRARIES} ${CURL_LIBRARIES})
