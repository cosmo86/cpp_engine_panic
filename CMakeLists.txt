cmake_minimum_required(VERSION 3.8)  # Minimum version of CMake
project(cpp_engine VERSION 1.0)  # Project name and version

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)  # C++ standard
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add include directories

#set(JEMALLOC_INCLUDE_DIR "C:/Users/ROG/Desktop/vcpkg/vcpkg/installed/x64-windows/include")
#set(JEMALLOC_INCLUDE_DIR "/usr/local/include")
set(JEMALLOC_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

#include_directories(include)
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/Strategy
    ${PROJECT_SOURCE_DIR}/model
    ${PROJECT_SOURCE_DIR}/spdlog
    ${PROJECT_SOURCE_DIR}/src
    ${JEMALLOC_INCLUDE_DIR}
)


# Define an executable target and its source files
#add_executable(MyExecutable src/main.cpp)

add_library(MyLibrary SHARED src/Porter.cpp src/engine.cpp src/dispatcher.cpp "include/log_handle.cpp") 

# You can also specify additional libraries to link against
# target_link_libraries(MyExecutable PUBLIC my_library)

set_target_properties(MyLibrary PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)


set(lev2_LIB_PATH "${PROJECT_SOURCE_DIR}/include/liblev2mdapi.so")
set(trader_LIB_PATH "${PROJECT_SOURCE_DIR}/include/libtraderapi.so")
set(jemalloc_LIB_PATH "${PROJECT_SOURCE_DIR}/include/libjemalloc.so.2")
set(ivonv_LIB_PATH "${PROJECT_SOURCE_DIR}/include/libiconv.so.2")
set(dolphin_LIB_PATH "${PROJECT_SOURCE_DIR}/include/libDolphinDBAPI.so")
target_link_libraries(MyLibrary PRIVATE
    ${lev2_LIB_PATH}
    ${trader_LIB_PATH}
    ${jemalloc_LIB_PATH}
    ${ivonv_LIB_PATH}
    ${dolphin_LIB_PATH}
    stdc++fs
)
#target_link_libraries(MyLibrary PRIVATE ${lev2_LIB_PATH})
#target_link_libraries(MyLibrary PRIVATE ${jemalloc_LIB_PATH})