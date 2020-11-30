#pragma once

#include "awaitable.hpp"

#include <atomic>
#include <limits>


namespace cppjobs {


class mutex {
	struct lock_token {
		void unlock() { m_mutex->unlock(); }
		mutex* m_mutex;
	};
	struct awaitable : awaitable_node {
		bool await_ready() const;
		bool await_suspend(std::coroutine_handle<> waiting) const;
		lock_token await_result() const;
		mutex* m_mutex;
	};

public:
	auto operator co_await();
	void unlock();
private:
	std::atomic<awaitable_node*> m_waiting;
	inline static awaitable_node* const LOCKED = reinterpret_cast<awaitable_node*>(std::numeric_limits<size_t>::max());
};



template <class Mutex>
class lock_guard {
public:
	lock_guard(typename Mutex::lock_token&& token) : m_token(std::move(token)) {}
	lock_guard(const lock_guard&) = delete;
	lock_guard& operator=(const lock_guard&) = delete;
	~lock_guard() { m_token.unlock(); }

private:
	typename Mutex::lock_token m_token;
};



} // namespace cppjobs
