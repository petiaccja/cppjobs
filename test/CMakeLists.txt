set(sources
	main.cpp
	)

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(test ${sources} 
	test_future.cpp
	test_mutex.cpp)
target_link_libraries(test cppjobs)