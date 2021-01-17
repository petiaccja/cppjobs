#include "catch.hpp"

#include <cppjobs/scheduler.hpp>

using namespace cppjobs;


TEST_CASE("Schedule regular function", "[Scheduler]") {
	auto fun = []() { return 42; };
	scheduler sched;
	future<int> fut = sched.schedule(fun);
	int val = fut.get();
	REQUIRE(val == 42);
}

TEST_CASE("Schedule coroutine", "[Scheduler]") {
	auto coro = []() mutable -> future<int> { co_return 42; };
	scheduler sched;
	future<int> fut = sched.schedule(coro);
	int val = fut.get();
	REQUIRE(val == 42);	
}