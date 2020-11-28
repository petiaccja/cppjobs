#pragma once

#include <coroutine>
#include <future>
#include <variant>


namespace cppjobs {

/*
Coroutine body:
{
	P p promise-constructor-arguments;
	co_await p.initial_suspend();// initial suspend point
	try {F} catch(...) {p.unhandled_exception(); }
final_suspend:
	co_await p.final_suspend(); // final suspend point
}
*/

// draft page 219

struct awaitable_node {
	awaitable_node* m_next = nullptr;
	std::coroutine_handle<> m_waiting = nullptr;
	std::condition_variable* m_cv = nullptr;
};



template <class T>
struct future {
	struct promise_type {
		auto get_return_object() { return future{ std::coroutine_handle<promise_type>::from_promise(*this) }; }
		auto initial_suspend() noexcept { return std::suspend_always{}; }
		auto final_suspend() noexcept;
		void return_value(T arg) { m_value = std::move(arg); }
		void unhandled_exception() { m_value = std::current_exception(); }
		
		T get() const;
		void start();
		bool finished() const { return m_waiting == FINISHED; }
		bool chain(awaitable_node* waiting);

	private:
		std::variant<T, std::exception_ptr> m_value;
		std::atomic_flag m_started;
		std::atomic<awaitable_node*> m_waiting = nullptr;
		static inline awaitable_node* const FINISHED = reinterpret_cast<awaitable_node*>(std::numeric_limits<size_t>::max());
	};
	using handle_type = std::coroutine_handle<promise_type>;

public:
	future() noexcept = default;
	future(future&&) noexcept;
	future& operator=(future&&) noexcept;
	future(const future&) = delete;
	future& operator=(const future&) = delete;
	~future();

	bool valid() const noexcept;
	void wait() const;
	T get();
	auto operator co_await() const;

	void share() { throw std::logic_error("not implemented"); }
	template <class Rep, class Period>
	std::future_status wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) const { throw std::logic_error("not implemented"); }
	template <class Clock, class Duration>
	std::future_status wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time) const { throw std::logic_error("not implemented"); }
private:
	future(handle_type handle) noexcept : m_handle(handle) {}

private:
	handle_type m_handle;
};


template <class T>
future<T>::future(future&& rhs) noexcept : m_handle(rhs.m_handle) {
	rhs.m_handle = nullptr;
}

template <class T>
future<T>& future<T>::operator=(future&& rhs) noexcept {
	future::~future();
	m_handle = rhs.m_handle;
}

template <class T>
future<T>::~future() {
	if (valid()) {
		wait();
		m_handle.destroy();
	}
}

template <class T>
auto future<T>::promise_type::final_suspend() noexcept {
	awaitable_node* waiting = m_waiting.exchange(FINISHED);
	while (waiting != nullptr) {
		if (waiting->m_waiting) {
			waiting->m_waiting.resume();
		}
		if (waiting->m_cv) {
			waiting->m_cv->notify_all();
		}
		waiting = waiting->m_next;
	}
	return std::suspend_always{};
}

template <class T>
T future<T>::promise_type::get() const {
	if (std::holds_alternative<T>(m_value)) {
		return std::get<T>(m_value);
	}
	if (std::holds_alternative<std::exception_ptr>(m_value)) {
		std::rethrow_exception(std::get<std::exception_ptr>(m_value));
	}
	std::terminate(); // Coroutine never finished.
}

template <class T>
void future<T>::promise_type::start() {
	auto my_handle = std::coroutine_handle<promise_type>::from_promise(*this);
	if (!m_started.test_and_set()) {
		my_handle.resume();
	}
}

template <class T>
bool future<T>::promise_type::chain(awaitable_node* waiting) {
	bool success;
	do {
		awaitable_node* next = m_waiting.load();
		waiting->m_next = next;
		if (next == FINISHED) {
			return false;
		}
		success = m_waiting.compare_exchange_strong(next, waiting);
	} while (!success);
	return true;
}


template <class T>
bool future<T>::valid() const noexcept {
	return static_cast<bool>(m_handle);
}

template <class T>
void future<T>::wait() const {
	if (!valid()) {
		throw std::future_error{ std::future_errc::no_state };
	}
	m_handle.promise().start();
	
	std::condition_variable cv;
	awaitable_node node{ .m_cv = &cv };
	if (m_handle.promise().chain(&node)) {
		std::mutex mtx;
		std::unique_lock lk(mtx);
		cv.wait(lk, [this] {
			return m_handle.promise().finished();
		});
	}
}

template <class T>
T future<T>::get() {
	wait();
	return m_handle.promise().get();
}

template <class T>
auto future<T>::operator co_await() const {
	struct awaitable : awaitable_node {
		bool await_ready() { return m_handle.promise().finished(); }
		bool await_suspend(std::coroutine_handle<> waiting) {
			m_handle.promise().start();
			m_waiting = waiting;
			return m_handle.promise().chain(this);
		}
		T await_resume() { return m_handle.promise().get(); }
		handle_type m_handle;
	};
	return awaitable{ .m_handle = m_handle };
}

} // namespace cppjobs