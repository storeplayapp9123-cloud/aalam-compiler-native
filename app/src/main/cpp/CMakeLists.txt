cmake_minimum_required(VERSION 3.22.1)
project(aalam_compiler_native)

add_library(
        aalamcompiler
        SHARED
        native-lib.cpp
        aalam_compiler.cpp
        build_pipeline.cpp
)

find_library(log-lib log)

target_link_libraries(
        aalamcompiler
        ${log-lib}
)

target_compile_features(aalamcompiler PRIVATE cxx_std_17)
