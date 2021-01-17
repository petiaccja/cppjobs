#pragma once

#include "awaitable_node.hpp"

#include <atomic>


namespace cppjobs {


class mutex {
	template <class Mutex>
	friend class lock_guard;
	template <class Mutex>
	friend class unique_lock;

	struct token {
		template <class Mutex>
		friend class lock_guard;
		template <class Mutex>
		friend class unique_lock;
		friend class mutex;

	private:
		token(mutex* mtx) : m_mutex(mtx) {}
		mutex* const m_mutex;
		void unlock();
		bool m_armed = false;
	};

	struct awaitable : awaitable_node {
		bool await_ready() const;
		template <class Promise>
		bool await_suspend(std::coroutine_handle<Promise> waiting);
		token await_resume() const;
		mutex* const m_mutex;
	};

public:
	mutex() = default;
	mutex(const mutex&) = delete;
	mutex(mutex&&) = delete;
	mutex& operator=(const mutex&) = delete;
	mutex& operator=(mutex&&) = delete;

	awaitable operator co_await();
	bool try_lock();
	void unlock();
	bool _is_locked() const;

private:
	std::atomic<awaitable_node*> m_waiting = nullptr;
	/// <summary> Always a dangling pointer. The last element of the m_waiting linked list. </summary>
	/// <remarks> Modify this variable only from holder context! </remarks>
	awaitable_node* m_holder = nullptr;
};



template <class Mutex>
class lock_guard {
public:
	explicit lock_guard(typename Mutex::token&& token) : m_token(std::move(token)) { m_token.m_armed = true; }
	lock_guard(const lock_guard&) = delete;
	lock_guard& operator=(const lock_guard&) = delete;
	~lock_guard() { m_token.unlock(); }

private:
	typename Mutex::token m_token;
};


template <class Promise>
bool mutex::awaitable::await_suspend(std::coroutine_handle<Promise> waiting) {
	bool success;
	awaitable_node* previous_in_line;
	do {
		previous_in_line = m_mutex->m_waiting;
		m_next = previous_in_line;
		set_waiting(waiting);
		success = m_mutex->m_waiting.compare_exchange_weak(previous_in_line, const_cast<awaitable*>(this));
	} while (!success);

	if (previous_in_line == nullptr) {
		m_mutex->m_holder = const_cast<awaitable*>(this);
	}

	return previous_in_line != nullptr;
}

} // namespace cppjobs
