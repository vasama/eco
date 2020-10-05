cmake_minimum_required(VERSION 3.15)

project(Eco)

add_compile_definitions(_DLL _SILENCE_CLANG_CONCEPTS_MESSAGE)
add_compile_options(-std=c++2a -fno-exceptions -isystem "C:/Program Files/LLVM/include/c++/v1")
add_link_options("-LC:/Program Files/LLVM/lib")

add_library(Eco
	Private/MpscQueue.cpp
	Private/Mutex.cpp
)

target_include_directories(Eco
	PUBLIC
		Public
)

if(WIN32)
	target_sources(Eco
		PRIVATE
			Private/Assert.win32.cpp
	)
endif()


add_executable(Test1
	Test/Test1.cpp
)

target_link_libraries(Test1
	PRIVATE
		Eco
)
