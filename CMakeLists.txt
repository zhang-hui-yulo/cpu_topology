cmake_minimum_required(VERSION 3.12)
project(CpuTopology LANGUAGES CXX)

add_executable(main
    src/main.cpp
    src/CpuTopology.cpp)

if (MSVC)
    target_compile_options(main PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:/W4>"
        "$<$<CXX_COMPILER_ID:MSVC>:/WX>")
endif()
