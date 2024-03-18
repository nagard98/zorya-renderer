cmake_minimum_required(VERSION 3.12)
project(preprocessor)

add_executable(preprocessor PreProcessor.cpp)

target_include_directories(preprocessor PRIVATE "${PROJECT_SOURCE_DIR}/../src")