#pragma once

#include "awaitable.hpp"

#include <atomic>


namespace cppjobs {


class mutex {
	template <class Mutex>
	friend class lock_guard;
	template <class Mutex>
	friend class unique_lock;
	
	struct lock_token {
		template <class Mutex>
		friend class lock_guard;
		template <class Mutex>
		friend class unique_lock;
		friend class mutex;
	private:
		lock_token(mutex* mtx) : m_mutex(mtx) {}
		mutex* const m_mutex;
		void unlock();
		bool m_armed = false;
	};
	
	struct awaitable : awaitable_node {
		bool await_ready() const;
		bool await_suspend(std::coroutine_handle<> waiting);
		lock_token await_resume() const;
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
	explicit lock_guard(typename Mutex::lock_token&& token) : m_token(std::move(token)) { m_token.m_armed = true; }
	lock_guard(const lock_guard&) = delete;
	lock_guard& operator=(const lock_guard&) = delete;
	~lock_guard() { m_token.unlock(); }

private:
	typename Mutex::lock_token m_token;
};



} // namespace cppjobs
