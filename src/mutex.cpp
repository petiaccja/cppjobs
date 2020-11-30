#include <cppjobs/mutex.hpp>


namespace cppjobs {

auto mutex::operator co_await() {
	return awaitable{ .m_mutex = this };
}

bool mutex::awaitable::await_ready() const {
	return false; // return try_lock()
}

bool mutex::awaitable::await_suspend(std::coroutine_handle<> waiting) const {
	awaitable_node* expected = nullptr;
	m_mutex->m_waiting.compare_exchange_strong(expected, LOCKED);
	return true;
}

mutex::lock_token mutex::awaitable::await_result() const {
	return {};
}


} // namespace cppjobs
