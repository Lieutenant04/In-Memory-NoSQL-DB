cmake_minimum_required(VERSION 3.14)
project(InMemoryNoSQLDB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main executable
add_executable(db_app main.cpp)

# On Linux/macOS, link pthreads for std::thread support
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(db_app PRIVATE Threads::Threads)
endif()
