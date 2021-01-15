#include <catch.hpp>
#include <cppjobs/future.hpp>
#include <cppjobs/mutex.hpp>
#include <cppjobs/shared_mutex.hpp>
#include <iostream>
#include <thread>

using namespace cppjobs;


TEST_CASE("Mutex lock/unlock cycle", "[Mutex]") {
	auto task = []() mutable -> future<void>
	{
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
	auto task = [&]() mutable -> future<void> {
		REQUIRE(!mtx._is_locked());
		lock_guard<mutex> lk{ co_await mtx };
		REQUIRE(mtx._is_locked());
	}();
	task.get();
	REQUIRE(!mtx._is_locked());
}