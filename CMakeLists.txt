cmake_minimum_required(VERSION 3.10)
project(StreamParserDemo)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)
# Include External Project
include(3rdparty/asio.cmake)

add_executable(SerialDemo serial_main.cc)
add_executable(TcpServerDemo tcp_server_main.cc)
add_executable(TcpClientDemo tcp_client_main.cc)
add_executable(UdpDemo udp_main.cc)

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

target_sources(SerialDemo PRIVATE
    src/StreamParser.cc
)

target_sources(TcpServerDemo PRIVATE
    src/StreamParser.cc
)

target_sources(TcpClientDemo PRIVATE
    src/StreamParser.cc
)

target_sources(UdpDemo PRIVATE
    src/StreamParser.cc
)

target_include_directories(SerialDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(SerialDemo asio)
target_compile_definitions(SerialDemo PRIVATE ASIO_STANDALONE _WIN32_WINNT=0x0601)

target_include_directories(TcpServerDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(TcpServerDemo asio)
target_compile_definitions(TcpServerDemo PRIVATE ASIO_STANDALONE _WIN32_WINNT=0x0601)

target_include_directories(TcpClientDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(TcpClientDemo asio)
target_compile_definitions(TcpClientDemo PRIVATE ASIO_STANDALONE _WIN32_WINNT=0x0601)

target_include_directories(UdpDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(UdpDemo asio)
target_compile_definitions(UdpDemo PRIVATE ASIO_STANDALONE _WIN32_WINNT=0x0601)


message(STATUS "Build completed for ${CMAKE_SYSTEM_NAME}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
