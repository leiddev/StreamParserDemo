cmake_minimum_required(VERSION 3.10)
project(StreamParserDemo)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)
# Include External Project
include(3rdparty/asio.cmake)

add_executable(StreamParserDemo main.cc)

if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
else()
    find_package(Threads REQUIRED)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    add_compile_options(/source-charset:utf-8)
    add_compile_options(/execution-charset:utf-8)
    add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_options(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
endif()

target_sources(StreamParserDemo PRIVATE
    src/StreamParser.cc
)

target_include_directories(StreamParserDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(StreamParserDemo asio)
target_compile_definitions(StreamParserDemo PRIVATE ASIO_STANDALONE _WIN32_WINNT=0x0601)


message(STATUS "Build completed for ${CMAKE_SYSTEM_NAME}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
