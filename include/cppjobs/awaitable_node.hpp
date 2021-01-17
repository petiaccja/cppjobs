#pragma once

#include "scheduler_base.hpp"

#include <coroutine>
#include <condition_variable>


namespace cppjobs {

struct awaitable_node {
	awaitable_node* m_next = nullptr;
	template <class Promise>
	void set_waiting(std::coroutine_handle<Promise> handle) {
		m_waiting = handle;
		if constexpr (std::convertible_to<Promise, schedulable_promise>) {
			m_scheduler = handle.promise().m_scheduler;
		}
	}
	void resume() {
		if (m_waiting) {
			m_scheduler ? m_scheduler->queue_for_resume(m_waiting) : m_waiting.resume();
		}
	}
private:
	std::coroutine_handle<> m_waiting = nullptr;
	std::shared_ptr<scheduler_base> m_scheduler = nullptr;
};


struct sync_awaitable_node {
	sync_awaitable_node* m_next = nullptr;
	std::condition_variable* m_cv = nullptr;
	template <class Promise>
	void set_waiting(std::coroutine_handle<Promise> handle) {
		m_waiting = handle;
		if constexpr (std::convertible_to<Promise, schedulable_promise>) {
			m_scheduler = handle.promise().m_scheduler;
		}
	}
	void resume() {
		if (m_waiting) {
			m_scheduler ? m_scheduler->queue_for_resume(m_waiting) : m_waiting.resume();
		}
		if (m_cv) {
			m_cv->notify_all();
		}
	}
private:
	std::shared_ptr<scheduler_base> m_scheduler = nullptr;
	std::coroutine_handle<> m_waiting = nullptr;
};

} // namespace cppjobs