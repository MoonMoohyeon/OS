cmake_minimum_required(VERSION 3.0.0)
set(PROJECT_NAME os)

project(${PROJECT_NAME} VERSION 0.1.0)

set(SRC_FILES mcku.c mcku.h)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_compile_options(${PROJECT_NAME} PUBLIC -std=c17 -pedantic-errors)