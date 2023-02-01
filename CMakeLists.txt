cmake_minimum_required(VERSION 3.16)
project(FtpProxy)

set(CMAKE_CXX_STANDARD 17)


aux_source_directory(Ftp COMPILER_FILE)
aux_source_directory(Thread COMPILER_FILE)
aux_source_directory(Util COMPILER_FILE)
aux_source_directory(Reactor COMPILER_FILE)
aux_source_directory(. COMPILER_FILE)

include_directories(.)
include_directories(Ftp)
include_directories(Thread)
include_directories(Util)
include_directories(Reactor)

add_executable(FtpProxy ${COMPILER_FILE})

target_link_libraries(FtpProxy pthread)