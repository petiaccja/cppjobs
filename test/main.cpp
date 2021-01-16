#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <cppjobs/future.hpp>
#include <iostream>


struct typed_awaitable {
	bool await_ready() const { return false; }
	template <class Promise>
	bool await_suspend(std::coroutine_handle<Promise> waiting) {
		m_waiting = waiting;
		m_resume_fun = [](std::coroutine_handle<> handle) {
			auto typed_handle = static_cast<std::coroutine_handle<Promise>>(handle);
			std::cout << typeid(decltype(typed_handle)).name() << std::endl;
		};
	}
	void await_resume() const { resume(); }
	void resume() const {
		assert(resume_fun);
		m_resume_fun(m_waiting);
	}
	std::coroutine_handle<> m_waiting = nullptr;
	void (*m_resume_fun)(std::coroutine_handle<>) = nullptr;
};

int main(int argc, char* argv[]) {
	auto coro = []() mutable -> cppjobs::future<void> {
		co_await typed_awaitable{};
	};
	return 0;
	
	int result = Catch::Session().run(argc, argv);
	return result;
}