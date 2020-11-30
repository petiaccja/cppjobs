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

future<void> void_coro() {
	co_return;
}

future<int&> ref_coro() {
	co_return 42;
}

future<std::unique_ptr<int>> moveable_coro() {
	auto ptr = std::make_unique<int>(42);
	co_return std::move(ptr);
}

future<void> destruction_coro(std::shared_ptr<int> param) {
	co_return;
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

TEST_CASE("Future type traits") {
	REQUIRE(std::is_default_constructible_v<future<void>>);
	REQUIRE(std::is_move_constructible_v<future<void>>);
	REQUIRE(std::is_move_assignable_v<future<void>>);
	REQUIRE(!std::is_copy_constructible_v<future<void>>);
	REQUIRE(!std::is_copy_assignable_v<future<void>>);

	REQUIRE(std::is_default_constructible_v<shared_future<void>>);
	REQUIRE(std::is_move_constructible_v<shared_future<void>>);
	REQUIRE(std::is_move_assignable_v<shared_future<void>>);
	REQUIRE(std::is_copy_constructible_v<shared_future<void>>);
	REQUIRE(std::is_copy_assignable_v<shared_future<void>>);
}



TEST_CASE("Run simple coroutine") {
	auto fut = simple_coro();
	auto value = fut.get();
	REQUIRE(value == 42);
}


TEST_CASE("Run void coroutine") {
	auto fut = void_coro();
	fut.get();
}


TEST_CASE("Run ref coroutine") {
	auto fut = ref_coro();
	auto& value = fut.get();
	REQUIRE(value == 42);
	++value;
	auto& updated = fut.get();
	REQUIRE(updated == 43);
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


TEST_CASE("Promise destroyed normal") {
	auto param = std::make_shared<int>(42);
	auto weak = std::weak_ptr(param);
	{
		auto fut = destruction_coro(std::move(param));
		fut.get();
		REQUIRE(weak.lock());
	}
	REQUIRE(!weak.lock());
}


TEST_CASE("Promise destroyed abandon") {
	auto param = std::make_shared<int>(42);
	auto weak = std::weak_ptr(param);
	{
		auto fut = destruction_coro(std::move(param));
		REQUIRE(weak.lock());
	}
	REQUIRE(!weak.lock());
}


//------------------------------------------------------------------------------
// shared_future
//------------------------------------------------------------------------------


TEST_CASE("Get results shared future") {
	auto fut = moveable_coro().share();
	int rm, ra, r1, r2;
	std::thread t1([fut, &r1] { r1 = *fut.get(); });
	std::thread t2([fut, &r2] { r2 = *fut.get(); });

	std::thread ta([fut, &ra] {
		ra = [](shared_future<std::unique_ptr<int>> fut) -> future<int> {
			co_return* co_await fut;
		}(fut).get();
	});

	rm = *fut.get();
	t1.join();
	t2.join();
	ta.join();
	REQUIRE(rm == 42);
	REQUIRE(ra == 42);
	REQUIRE(r1 == 42);
	REQUIRE(r2 == 42);
}



TEST_CASE("Promise destroyed share normal") {
	auto param = std::make_shared<int>(42);
	auto weak = std::weak_ptr(param);
	std::thread t1, t2;
	{
		shared_future<void> fut = destruction_coro(std::move(param));
		t1 = std::thread([fut] { fut.get(); });
		t2 = std::thread([fut] { fut.get(); });
		fut.get();
		REQUIRE(weak.lock());
	}
	t1.join();
	t2.join();
	REQUIRE(!weak.lock());
}


TEST_CASE("Promise destroyed share abandon") {
	auto param = std::make_shared<int>(42);
	auto weak = std::weak_ptr(param);
	{
		shared_future<void> fut = destruction_coro(std::move(param));
		auto fut2 = fut;
		auto fut3 = fut2;
		REQUIRE(weak.lock());
	}
	REQUIRE(!weak.lock());
}