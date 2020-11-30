set(sources
	mutex.cpp
	)

include_directories(${CMAKE_SOURCE_DIR}/include)
add_library(cppjobs STATIC ${sources} "mutex.cpp")