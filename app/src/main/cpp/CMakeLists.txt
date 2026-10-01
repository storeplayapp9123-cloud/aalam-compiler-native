cmake_minimum_required(VERSION 3.22)
project(aalam_native C CXX)
set(CMAKE_CXX_STANDARD 17)
add_library(aalam_native SHARED
    native-lib.cpp
    aalam_compiler.cpp
    build_pipeline.cpp)
find_library(log-lib log)
target_link_libraries(aalam_native ${log-lib})
