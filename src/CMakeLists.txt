set(sources
	fence.cpp
	)

include_directories(${CMAKE_SOURCE_DIR}/include)
add_library(cppjobs STATIC ${sources})