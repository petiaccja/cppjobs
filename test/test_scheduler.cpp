#include "catch.hpp"
#include "cppjobs/schedulers/debug_scheduler.hpp"
#include "cppjobs/schedulers/immediate_scheduler.hpp"

using namespace cppjobs;

using dbg_sched = debug_scheduler<immediate_scheduler>;

TEST_CASE("Schedule regular function", "[Scheduler]") {
	auto fun = []() { return 42; };
	auto sched = std::make_shared<dbg_sched>();
	future<int> fut = sched->schedule(fun);
	int val = fut.get();
	REQUIRE(val == 42);
	REQUIRE(sched->resume_count() == 1);
}


TEST_CASE("Schedule coroutine", "[Scheduler]") {
	auto coro = []() mutable -> future<int> { co_return 42; };
	auto sched = std::make_shared<dbg_sched>();
	future<int> fut = sched->schedule(coro);
	int val = fut.get();
	REQUIRE(val == 42);
	REQUIRE(sched->resume_count() == 1);
}


TEST_CASE("Awaits", "[Scheduler]") {
	auto factorial = [](future<int> chain, int count) mutable -> future<int>
	{
		int mul = 1;
		if (chain.valid()) {
			mul = co_await chain;
		}
		co_return count * mul;
	};
	
	auto sched = std::make_shared<dbg_sched>();

	future<int> fut;
	fut = sched->schedule(factorial, std::move(fut), 1);
	fut = sched->schedule(factorial, std::move(fut), 2);
	fut = sched->schedule(factorial, std::move(fut), 3);
	fut = sched->schedule(factorial, std::move(fut), 4);
	fut = sched->schedule(factorial, std::move(fut), 5);
	fut = sched->schedule(factorial, std::move(fut), 6);
	fut = sched->schedule(factorial, std::move(fut), 7);
	fut = sched->schedule(factorial, std::move(fut), 8);	
	
	int val = fut.get();
	REQUIRE(val == 40320);
	REQUIRE(sched->resume_count() == 8);
}

