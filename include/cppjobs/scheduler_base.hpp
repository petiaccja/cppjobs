#pragma once

#include <memory>
#include <coroutine>


namespace cppjobs {

class scheduler_base : public std::enable_shared_from_this<scheduler_base> {
public:
	virtual ~scheduler_base() {}
	virtual void queue_for_resume(std::coroutine_handle<> handle) = 0;

	inline static thread_local std::shared_ptr<scheduler_base> tls_scheduler = nullptr;

};

} // namespace cppjobs