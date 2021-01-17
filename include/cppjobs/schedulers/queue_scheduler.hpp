#include "../scheduler.hpp"

#include <queue>


namespace cppjobs {

class queue_scheduler final : public scheduler {
	void queue_for_resume(std::coroutine_handle<> handle) override {
		m_handles.push(handle);
		while (!m_handles.empty()) {
			m_handles.front().resume();
			m_handles.pop();
		}
	}
	std::queue<std::coroutine_handle<>> m_handles;
};


} // namespace cppjobs