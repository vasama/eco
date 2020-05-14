cmake_minimum_required(VERSION 3.21)

project(Eco)


Include(FetchContent)


add_library(Eco
	Public/Eco/Assert.hpp
	Public/Eco/Atomic.hpp
	Public/Eco/AvlSet.hpp
	Public/Eco/Heap.hpp
	Public/Eco/KeySelector.hpp
	Public/Eco/Link.hpp
	Public/Eco/List.hpp
	Public/Eco/MpscQueue.hpp
	Public/Eco/TaggedPointer.hpp
	Public/Eco/WbSet.hpp

	Private/AvlSet.cpp
	Private/Heap.cpp
	Private/Link.cpp
	Private/List.cpp
	Private/MpscQueue.cpp
	Private/WbSet.cpp
)
target_include_directories(Eco
	PUBLIC
		Public
)
target_compile_features(Eco
	PUBLIC
		cxx_std_20
)

if(WIN32)
	target_sources(Eco
		PRIVATE
			Private/Assert.win32.cpp
	)
endif()

if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
	target_sources(Eco
		PRIVATE
			Extra/Eco.natvis
	)
endif()


if(PROJECT_IS_TOP_LEVEL)
	FetchContent_Declare(
		Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG        v2.13.8
	)
	FetchContent_MakeAvailable(Catch2)


	add_executable(Eco-Test
		Private/AvlSet.test.cpp
		Private/Heap.test.cpp
		Private/List.test.cpp
		Private/Main.test.cpp
		Private/WbSet.test.cpp
	)
	target_link_libraries(Eco-Test
		PRIVATE
			Eco
			Catch2::Catch2
	)
endif()
