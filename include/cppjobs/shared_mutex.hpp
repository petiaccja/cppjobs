#pragma once

#include "future.hpp"
#include "mutex.hpp"

namespace cppjobs {

/// <summary>
/// I have literally no fucking clue if the implementation works. I guess I have to run a statistical test.
/// </summary>
class shared_mutex {
	template <class Mutex>
	friend class lock_guard;
	template <class Mutex>
	friend class unique_lock;

	struct token {
		template <class Mutex>
		friend class lock_guard;
		template <class Mutex>
		friend class unique_lock;
		friend class shared_mutex;

	private:
		token(shared_mutex* mtx) : m_mutex(mtx) {}
		shared_mutex* const m_mutex;
		void unlock();
		bool m_armed = false;
	};

	struct shared_token {
		template <class Mutex>
		friend class lock_guard;
		template <class Mutex>
		friend class unique_lock;
		friend class shared_mutex;

	private:
		shared_token(shared_mutex* mtx) : m_mutex(mtx) {}
		shared_mutex* const m_mutex;
		void unlock();
		bool m_armed = false;
	};
	
public:
	shared_mutex() = default;
	shared_mutex(const shared_mutex&) = delete;
	shared_mutex(shared_mutex&&) = delete;
	shared_mutex& operator=(const shared_mutex&) = delete;
	shared_mutex& operator=(shared_mutex&&) = delete;

	friend auto unique(shared_mutex& mtx) { return mtx.lock(); }
	friend auto shared(shared_mutex& mtx) { return mtx.lock_shared(); }
	bool try_lock();
	void unlock();
	bool try_lock_shared();
	void unlock_shared();
		
	bool _is_locked() const;
	bool _is_locked_shared() const;

private:
	future<token> lock();
	future<shared_token> lock_shared();

private:
	mutex m_outerMutex;
	mutex m_innerMtx;
	std::atomic_size_t m_innerCount = 0;
};

} // namespace cppjobs