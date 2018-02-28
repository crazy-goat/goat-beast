cmake_minimum_required(VERSION 3.9)
project(goat-beast)
find_package(Threads)

set(CMAKE_CXX_STANDARD 17)

add_executable(goat-beast src/main.cpp src/HttpWorker.h)
target_include_directories(goat-beast PUBLIC vendor/boost vendor/json/include)
find_library(BOOST_SYSTEM libboost_system.a HINTS vendor/boost/stage/lib)
find_library(BOOST_FILESYSTEM libboost_filesystem.a HINTS vendor/boost/stage/lib)
find_library(BOOST_THREAD libboost_thread.a HINTS vendor/boost/stage/lib)
find_library(BOOST_COROUTINE libboost_coroutine.a HINTS vendor/boost/stage/lib)

target_link_libraries(goat-beast PUBLIC
        ${BOOST_SYSTEM} ${BOOST_THREAD}
        ${BOOST_FILESYSTEM} ${BOOST_COROUTINE}
        ${CMAKE_THREAD_LIBS_INIT})
