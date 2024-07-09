# Minimum required CMake version
cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0074 NEW)

# Project name
project(RSAchiver)
link_directories(${ROOT_LIBRARY_DIR})
link_directories(${CMSG_ROOT}/Linux-x86_64/lib)
link_directories(${xmsg_DIR}/../..)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/RSAchiver)


# Add executable for RSelog
add_executable(RSArchiver HTMLOutputGenerator.cc INIReader.cc PDFOutputGenerator.cc RSArchiver.cc)

# Link with libraries (assuming libRootSpy-client is built as a library)
target_link_libraries(RSArchiver RootSpy-client ${ROOT_LIBRARIES} ${CMSG_LIBRARIES} ${xmsg_LIBRARIES} ${Protobuf_LIBRARIES} ${CURL_LIBRARIES} inih)