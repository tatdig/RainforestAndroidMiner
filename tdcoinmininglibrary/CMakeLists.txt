cmake_minimum_required(VERSION 3.4.1)

# Include sub directory
add_subdirectory(${CMAKE_SOURCE_DIR}/src/main/cpp/libcpuminer)

add_library(
    tdcoinmininglibrary
    SHARED
        src/main/cpp/tdcoinmininglibrary.cpp)

find_library(
    log-lib
    log)

target_link_libraries(
    tdcoinmininglibrary
    ${log-lib}
    cpuminer)

