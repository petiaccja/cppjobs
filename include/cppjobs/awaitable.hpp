#pragma once

#include <coroutine>


namespace std {
class condition_variable;
}


namespace cppjobs {

struct awaitable_node {
	awaitable_node* m_next = nullptr;
	std::coroutine_handle<> m_waiting = nullptr;
};


struct sync_awaitable_node {
	sync_awaitable_node* m_next = nullptr;
	std::coroutine_handle<> m_waiting = nullptr;
	std::condition_variable* m_cv = nullptr;
};

} // namespace cppjobs