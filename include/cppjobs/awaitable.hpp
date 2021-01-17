#pragma once

#include "scheduler_base.hpp"

#include <coroutine>
#include <condition_variable>


namespace cppjobs {

struct awaitable_node {
	awaitable_node* m_next = nullptr;
	void resume() {
		if (m_scheduler) {
			m_scheduler->queue_for_resume(m_waiting);
		}
		else {
			m_waiting.resume();
		}
	}
	std::coroutine_handle<> m_waiting = nullptr;
	std::shared_ptr<scheduler_base> m_scheduler = nullptr;
};


struct sync_awaitable_node {
	sync_awaitable_node* m_next = nullptr;
	std::condition_variable* m_cv = nullptr;
	void resume() {
		if (m_scheduler) {
			m_scheduler->queue_for_resume(m_waiting);
		}
		else {
			m_waiting.resume();
		}
		if (m_cv) {
			m_cv->notify_all();
		}
	}
	std::shared_ptr<scheduler_base> m_scheduler = nullptr;
	std::coroutine_handle<> m_waiting = nullptr;
};

} // namespace cppjobs