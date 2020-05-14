cmake_minimum_required(VERSION 3.15)

project(Eco)

add_compile_definitions(_DLL _SILENCE_CLANG_CONCEPTS_MESSAGE)
add_compile_options(-std=c++2a -fno-exceptions -isystem "C:/Program Files/LLVM/include/c++/v1")
add_link_options("-LC:/Program Files/LLVM/lib")

add_library(Eco
	Private/MpscListQueue.cpp
)
target_include_directories(Eco PUBLIC Public)
