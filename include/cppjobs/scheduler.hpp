#pragma once

#include "future.hpp"
#include "type_traits.hpp"
#include <memory>


namespace cppjobs {


class scheduler_base : public std::enable_shared_from_this<scheduler_base> {
public:
	virtual ~scheduler_base() {}

	inline static thread_local std::shared_ptr<scheduler_base> tls_scheduler = nullptr;
protected:
	virtual void queue_for_resume(std::coroutine_handle<> handle) {
		handle.resume();
	}
};


class scheduler : public scheduler_base {
public:
	template <class Func, class... Args>
	auto schedule(Func func, Args&&... args);
	
private:
	template <class Func, class... Args>
	static awaitable auto launch(Func func, Args&&... args) requires awaitable<std::invoke_result_t<Func, Args...>> {
		return func(std::forward<Args>(args)...);
	}

	template <class Func, class... Args>
	static awaitable auto launch(Func func, Args&&... args) requires !awaitable<std::invoke_result_t<Func, Args...>> {
		using result_t = std::invoke_result_t<Func, Args...>;
		using future_t = future<result_t>;
		auto wrapper_coro = [](Func func, Args&&... args) mutable -> future_t {
			co_return func(std::forward<Args>(args)...);
		};
		return wrapper_coro(std::move(func), std::forward<Args>(args)...);
	}
};


template <class Func, class... Args>
auto scheduler::schedule(Func func, Args&&... args) {
	tls_scheduler = shared_from_this();
	auto future = this->launch(std::move(func), std::forward<Args>(args)...);
	return future;
}


} // namespace cppjobs
