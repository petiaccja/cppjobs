#pragma once

#include <coroutine>
#include <future>
#include <new>
#include <variant>
#include "awaitable.hpp"


namespace cppjobs {


template <class T>
class shared_future;

template <class T>
class future {
	struct promise_storage_void {
		using stored_t = std::monostate;
		void return_void() { m_value = std::monostate{}; }
		std::variant<std::monostate, std::exception_ptr> m_value;
	};
	struct promise_storage_full {
		using stored_t = std::remove_reference_t<T>;
		void return_value(stored_t value) { m_value = std::move(value); }
		std::variant<std::monostate, stored_t, std::exception_ptr> m_value;
	};
	using promise_storage = std::conditional_t<std::is_void_v<T>, promise_storage_void, promise_storage_full>;

public:
	struct promise_type : promise_storage {
		using typename promise_storage::stored_t;

		auto get_return_object() { return future{ std::coroutine_handle<promise_type>::from_promise(*this) }; }
		auto initial_suspend() noexcept { return std::suspend_always{}; }
		auto final_suspend() noexcept;
		void unhandled_exception() { this->m_value = std::current_exception(); }

		auto get() -> stored_t&;
		void start();
		bool finished() const { return m_waiting == FINISHED; }
		bool chain(sync_awaitable_node* waiting);

		void add_ref() { m_refcount.fetch_add(1); }
		bool remove_ref() { return 1 == m_refcount.fetch_sub(1); }
		void wait_destroy();

	private:
		std::atomic_flag m_started;
		std::atomic<sync_awaitable_node*> m_waiting = nullptr;
		static inline sync_awaitable_node* const FINISHED = reinterpret_cast<sync_awaitable_node*>(std::numeric_limits<size_t>::max());
		std::atomic_size_t m_refcount = 0;
		volatile bool m_can_destroy = false;
	};
	using handle_type = std::coroutine_handle<promise_type>;

public:
	future() noexcept = default;
	future(future&&) noexcept;
	future& operator=(future&&) noexcept;
	~future();

	bool valid() const noexcept;
	void wait() const;
	T get();
	auto operator co_await() const;

	shared_future<T> share();

	template <class Rep, class Period>
	std::future_status wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) const { throw std::logic_error("not implemented"); }
	template <class Clock, class Duration>
	std::future_status wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time) const { throw std::logic_error("not implemented"); }

protected:
	future(const future&) noexcept;
	future& operator=(const future&) noexcept;
	future(handle_type handle) noexcept;
protected:
	handle_type m_handle = nullptr;
};


template <class T>
class shared_future : public future<T> {
public:
	shared_future(future<T>&& fut) : future<T>(std::move(fut)) {}
	using future<T>::future;

	auto get() const -> std::conditional_t<std::is_void_v<T>, void, std::add_lvalue_reference_t<T>>;
	auto operator co_await() const;
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
		auto& promise = m_handle.promise();
		bool destroy = promise.remove_ref();
		if (destroy) {
			promise.wait_destroy();
			m_handle.destroy();
		}
	}
}

template <class T>
auto future<T>::promise_type::final_suspend() noexcept {
	// Set state to finished.
	sync_awaitable_node* waiting = m_waiting.exchange(FINISHED);
	// Continue chains.
	while (waiting != nullptr) {
		if (waiting->m_waiting) {
			waiting->m_waiting.resume();
		}
		if (waiting->m_cv) {
			waiting->m_cv->notify_all();
		}
		waiting = waiting->m_next;
	}
	// Cleanup awaiter.
	struct awaitable {
		bool await_ready() const noexcept {
			bool destroy = m_promise->remove_ref();
			return destroy;
		}
		void await_suspend(std::coroutine_handle<>) const noexcept {
			m_promise->m_can_destroy = true;
		}
		constexpr void await_resume() const noexcept {}
		promise_type* m_promise;
	};
	return awaitable{ this };
}

template <class T>
auto future<T>::promise_type::get() -> stored_t& {
	if (std::holds_alternative<std::exception_ptr>(this->m_value)) {
		std::rethrow_exception(std::get<std::exception_ptr>(this->m_value));
	}
	if (std::holds_alternative<stored_t>(this->m_value)) {
		return std::get<stored_t>(this->m_value);
	}
	assert(false);
	std::terminate();
}

template <class T>
void future<T>::promise_type::start() {
	auto my_handle = std::coroutine_handle<promise_type>::from_promise(*this);
	if (!m_started.test_and_set()) {
		add_ref();
		my_handle.resume();
	}
}

template <class T>
bool future<T>::promise_type::chain(sync_awaitable_node* waiting) {
	bool success;
	do {
		sync_awaitable_node* next = m_waiting.load();
		waiting->m_next = next;
		if (next == FINISHED) {
			return false;
		}
		success = m_waiting.compare_exchange_strong(next, waiting);
	} while (!success);
	return true;
}

template <class T>
void future<T>::promise_type::wait_destroy() {
	if (m_started.test()) {
		while (!m_can_destroy)
			;
	}
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
	sync_awaitable_node node{ .m_cv = &cv };
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
	if constexpr (std::is_void_v<T>) {
		return;
	}
	else if constexpr (std::is_reference_v<T>) {
		return m_handle.promise().get();
	}
	else {
		return std::move(m_handle.promise().get());
	}
}

template <class T>
auto future<T>::operator co_await() const {
	struct awaitable : sync_awaitable_node {
		bool await_ready() { return m_handle.promise().finished(); }
		bool await_suspend(std::coroutine_handle<> waiting) {
			m_handle.promise().start();
			m_waiting = waiting;
			return m_handle.promise().chain(this);
		}
		T await_resume() { return std::move(m_handle.promise().get()); }
		handle_type m_handle;
	};
	return awaitable{ .m_handle = m_handle };
}

template <class T>
shared_future<T> future<T>::share() {
	return shared_future(std::move(*this));
}

template <class T>
future<T>::future(const future& rhs) noexcept : future(rhs.m_handle) {}

template <class T>
future<T>& future<T>::operator=(const future& rhs) noexcept {
	future::~future();
	new (this) future(rhs);
}

template <class T>
future<T>::future(handle_type handle) noexcept
	: m_handle(handle) {
	m_handle.promise().add_ref();
}

template <class T>
auto shared_future<T>::get() const -> std::conditional_t<std::is_void_v<T>, void, std::add_lvalue_reference_t<T>> {
	this->wait();
	if constexpr (!std::is_void_v<T>) {
		return this->m_handle.promise().get();
	}
}

template <class T>
auto shared_future<T>::operator co_await() const {
	struct awaitable : sync_awaitable_node {
		bool await_ready() { return m_handle.promise().finished(); }
		bool await_suspend(std::coroutine_handle<> waiting) {
			m_handle.promise().start();
			m_waiting = waiting;
			return m_handle.promise().chain(this);
		}
		T& await_resume() { return m_handle.promise().get(); }
		typename future<T>::handle_type m_handle;
	};
	return awaitable{ .m_handle = this->m_handle };
}


} // namespace cppjobs