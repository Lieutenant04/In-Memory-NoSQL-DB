cmake_minimum_required(VERSION 3.14)
project(InMemoryNoSQLDB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main executable
add_executable(db_app main.cpp)

# Test executable
enable_testing()
add_executable(db_tests tests.cpp)
add_test(NAME DatabaseTests COMMAND db_tests)

# On Linux/macOS, link pthreads for std::thread support
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(db_app PRIVATE Threads::Threads)
    target_link_libraries(db_tests PRIVATE Threads::Threads)
endif()
