# Minimum required CMake version
cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0074 NEW)

# Project name
project(RSAI)
link_directories(${ROOT_LIBRARY_DIR})
link_directories(${CMSG_ROOT}/Linux-x86_64/lib)
link_directories(${xmsg_DIR}/../..)


# Add executable for rsai
add_executable(RSAI RSAI.cc)

# Link with libraries (assuming libRootSpy-client is built as a library)
target_link_libraries(RSAI RootSpy-client ${ROOT_LIBRARIES} ${CMSG_LIBRARIES} ${xmsg_LIBRARIES} ${Protobuf_LIBRARIES})

