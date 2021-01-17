#pragma once

#include <concepts>
#include <coroutine>
#include <type_traits>


namespace cppjobs {

// clang-format off
template <class T>
concept direct_awaitable = requires(T a) {
	{ a.await_ready() }	-> std::same_as<bool>;
	requires requires {{ a.await_suspend(std::declval<std::coroutine_handle<void>>()) } -> std::same_as<bool>;} || requires {{ a.await_suspend() } -> std::same_as<void>;};
	a.await_resume();
};

template <class T>
concept conversion_awaitable = requires (T a) {{a.operator co_await()} -> direct_awaitable;};

template <class T>
concept awaitable = direct_awaitable<T> || conversion_awaitable<T>;
// clang-format on

template <awaitable T>
struct await_result {
	using type = typename await_result<decltype(std::declval<T>().operator co_await())>::type;
};

template <direct_awaitable T>
struct await_result<T> {
	using type = std::invoke_result_t<decltype(&T::await_resume), T>;
};

template <awaitable T>
using await_result_t = typename await_result<T>::type;


} // namespace cppjobs