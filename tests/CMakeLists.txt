cmake_minimum_required(VERSION 3.10)
project(PackageTest CXX)

find_package(logrescuer REQUIRED)

add_executable(test_package test_package.cpp)
target_link_libraries(test_package logrescuer::logrescuer)
target_compile_features(test_package PRIVATE cxx_std_17)
