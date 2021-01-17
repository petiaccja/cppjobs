#include "../scheduler.hpp"


namespace cppjobs {

class immediate_scheduler final : public scheduler {
	void queue_for_resume(std::coroutine_handle<> handle) override {
		handle.resume();
	}
};


}