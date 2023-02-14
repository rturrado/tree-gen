cmake_minimum_required(VERSION 3.20)
project(PackageTest CXX)

find_package(tree-gen CONFIG REQUIRED)

add_executable(example src/example.cpp)
target_link_libraries(example tree-gen::tree-gen)
