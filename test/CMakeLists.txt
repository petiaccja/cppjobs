set(sources
	main.cpp
	)

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(test ${sources} 
	test_future.cpp
	test_mutex.cpp
	test_shared_mutex.cpp 
	test_type_traits.cpp
	test_scheduler.cpp)
target_link_libraries(test cppjobs)