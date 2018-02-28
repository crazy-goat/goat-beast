cmake_minimum_required(VERSION 3.9)
project(goat_zero)
find_package(Threads)

set(CMAKE_CXX_STANDARD 17)

add_executable(goat_zero src/main.cpp)
target_include_directories(goat_zero PUBLIC vendor/boost)
find_library(BOOST_SYSTEM libboost_system.a HINTS vendor/boost/stage/lib)
find_library(BOOST_FILESYSTEM libboost_filesystem.a HINTS vendor/boost/stage/lib)
find_library(BOOST_THREAD libboost_thread.a HINTS vendor/boost/stage/lib)
find_library(BOOST_COROUTINE libboost_coroutine.a HINTS vendor/boost/stage/lib)

target_link_libraries(goat_zero PUBLIC
        ${BOOST_SYSTEM} ${BOOST_THREAD}
        ${BOOST_FILESYSTEM} ${BOOST_COROUTINE}
        ${CMAKE_THREAD_LIBS_INIT})
