#include <catch.hpp>
#include <cppjobs/future.hpp>
#include <iostream>
#include <thread>

using namespace cppjobs;

struct spawn_thread {
	bool await_ready() const { return false; }
	void await_suspend(std::coroutine_handle<> waiting) const {
		std::thread([waiting] {
			waiting.resume();
		}).detach();
	}
	void await_resume() {}
};


future<int> simple_coro() {
	co_return 42;
}

future<int> switch_thread_coro() {
	using namespace std::chrono_literals;
	co_await spawn_thread{};
	std::this_thread::sleep_for(300ms);
	co_return 42;
}

future<int> wait_future(future<int> fut) {
	int ret = co_await fut;
	co_return ret;
}


TEST_CASE("Run simple coroutine") {
	auto fut = simple_coro();
	auto value = fut.get();
	REQUIRE(value == 42);
}


TEST_CASE("Await simple coroutine") {
	auto fut = wait_future(simple_coro());
	auto value = fut.get();
	REQUIRE(value == 42);
}


TEST_CASE("Abuse future") {
	auto fut = simple_coro();
	fut.wait();
	auto value = fut.get();
	REQUIRE(value == 42);
	value = wait_future(std::move(fut)).get();
	REQUIRE(value == 42);
}


TEST_CASE("Future blocking wait") {
	auto fut = switch_thread_coro();
	auto value = fut.get();
	REQUIRE(value == 42);
}
