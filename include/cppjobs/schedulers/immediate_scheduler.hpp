#include "../scheduler.hpp"


namespace cppjobs {

class immediate_scheduler : public scheduler {
protected:
	void queue_for_resume(std::coroutine_handle<> handle) override {
		handle.resume();
	}
};


}