#pragma once

#include "../scheduler.hpp"

/// <summary>
/// This is a scheduler wrapper that runs some counters for testing and debugging.
/// Don't use it in dependent code.
/// </summary>
/// <typeparam name="Scheduler"> The scheduler you want to wrap. </typeparam>
template <class Scheduler>
class debug_scheduler : public Scheduler {
public:
	size_t resume_count() const {
		return m_resume_count;
	}
protected:
	void queue_for_resume(std::coroutine_handle<> handle) override {
		++m_resume_count;
		Scheduler::queue_for_resume(handle);
	}
private:
	std::atomic_size_t m_resume_count = 0;
	
};