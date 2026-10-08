cmake_minimum_required(VERSION 3.20)

project(OS_Labs C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
)

add_subdirectory(lab02)
