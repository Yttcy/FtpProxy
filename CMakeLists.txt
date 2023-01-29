cmake_minimum_required(VERSION 3.16)
project(FTP_Proxy)

set(CMAKE_CXX_STANDARD 17)


aux_source_directory(Client COMPILER_FILE)
aux_source_directory(. COMPILER_FILE)

include_directories(.)
include_directories(./Client)

add_executable(FTP_Proxy ${COMPILER_FILE})

target_link_libraries(FTP_Proxy pthread)