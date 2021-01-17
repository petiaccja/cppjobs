#pragma once

#include "future.hpp"
#include "type_traits.hpp"
#include "scheduler_base.hpp"

namespace cppjobs {


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
	struct atexit {
		~atexit() { tls_scheduler = nullptr; }
	} _atexit;
	auto future = this->launch(std::move(func), std::forward<Args>(args)...);
	return future;
}


} // namespace cppjobs
