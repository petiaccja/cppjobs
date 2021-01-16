#include <catch.hpp>
#include <cppjobs/future.hpp>
#include <cppjobs/mutex.hpp>
#include <cppjobs/shared_mutex.hpp>
#include <iostream>
#include <thread>

using namespace cppjobs;


TEST_CASE("Mutex lock/unlock cycle", "[Mutex]") {
	auto task = []() mutable -> future<void> {
		mutex mtx;
		REQUIRE(!mtx._is_locked());
		co_await mtx;
		REQUIRE(mtx._is_locked());
		mtx.unlock();
		REQUIRE(!mtx._is_locked());
	}();
	task.get();
}


TEST_CASE("Mutex try_lock/unlock cycle", "[Mutex]") {
	mutex mtx;
	REQUIRE(!mtx._is_locked());
	mtx.try_lock();
	REQUIRE(mtx._is_locked());
	mtx.unlock();
	REQUIRE(!mtx._is_locked());
}


TEST_CASE("Lock_guard", "[Mutex]") {
	mutex mtx;
	auto task = [](mutex& mtx) -> future<void> {
		REQUIRE(!mtx._is_locked());
		lock_guard<mutex> lk{ co_await mtx };
		REQUIRE(mtx._is_locked());
	}(mtx);
	task.get();
	REQUIRE(!mtx._is_locked());
}


TEST_CASE("Mutex hammer", "[Mutex]") {
	mutex mtx;
	volatile bool run = true;
	volatile size_t value = 0;
	std::atomic_size_t control = 0;
	auto increment = [](mutex& mtx, volatile size_t& value, volatile bool& run) mutable -> future<size_t> {
		size_t internal = 0;
		while (run) {
			++internal;
			lock_guard<mutex> lk{ co_await mtx };
			++value;
		}
		co_return internal;
	};

	std::vector<std::thread> threads;
	for (size_t i = 0; i<std::thread::hardware_concurrency(); ++i) {
		threads.push_back(std::thread([&mtx, &value, &run, &control, increment] {
			size_t internal = increment(mtx, value, run).get();
			control += internal;
		}));
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(350));
	run = false;
	
	REQUIRE(control == control.load());

	std::ranges::for_each(threads, [](std::thread& thread) { thread.join(); });
}