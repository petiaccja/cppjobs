#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <cppjobs/mutex.hpp>


namespace cppjobs {

mutex::awaitable mutex::operator co_await() {
	return awaitable{ .m_mutex = this };
}

bool mutex::try_lock() {
	awaitable_node* hoped = nullptr;
	awaitable_node* const tag = reinterpret_cast<awaitable_node*>(std::numeric_limits<uintptr_t>::max());
	bool locked = m_waiting.compare_exchange_strong(hoped, tag);
	if (locked) {
		m_holder = tag;
	}
	return locked;	
}

void mutex::token::unlock() {
	if (m_armed) {
		m_mutex->unlock();
	}
}

bool mutex::awaitable::await_ready() const {
	return m_mutex->try_lock();
}

bool mutex::awaitable::await_suspend(std::coroutine_handle<> waiting) {
	bool success;
	awaitable_node* previous_in_line;
	do {
		previous_in_line = m_mutex->m_waiting;
		m_next = previous_in_line;
		m_waiting = waiting;
		success = m_mutex->m_waiting.compare_exchange_weak(previous_in_line, const_cast<awaitable*>(this));
	} while (!success);

	if (previous_in_line == nullptr) {
		m_mutex->m_holder = const_cast<awaitable*>(this);
	}
	
	return previous_in_line != nullptr;
}

mutex::token mutex::awaitable::await_resume() const {
	return token{ m_mutex };
}

void mutex::unlock() {
	awaitable_node* holder = m_holder;

	// If the head of the list (m_waiting) equals holder, nobody is else is waiting.
	// In this case the mutex is freed by writing nullptr as the holder.
	const bool nobody_waited = m_waiting.compare_exchange_strong(holder, nullptr);

	// If somebody was waiting, we have now have the head of the list in holder.
	if (!nobody_waited) {
		if (holder == nullptr) {
			throw std::logic_error("mutex is not locked!");
		}
		awaitable_node* next_in_line = nullptr;
		while (holder != m_holder) {
			next_in_line = holder;
			holder = holder->m_next;
		}
		// Resume next in line.
		next_in_line->m_next = nullptr;
		m_holder = next_in_line;
		next_in_line->m_waiting.resume();
	}
}

bool mutex::_is_locked() const {
	return m_waiting != nullptr;
}


} // namespace cppjobs
