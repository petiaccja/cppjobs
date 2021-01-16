#include <cppjobs/shared_mutex.hpp>


namespace cppjobs {


future<shared_mutex::token> shared_mutex::lock() {
	co_await m_outerMutex;
	co_await m_innerMtx;
	co_return token{ this };
}

future<shared_mutex::shared_token> shared_mutex::lock_shared() {
	co_await m_outerMutex;
	auto prev = m_innerCount++;
	if (prev == 0) {
		co_await m_innerMtx;
	}
	m_outerMutex.unlock();
	co_return shared_token{ this };
}

bool shared_mutex::try_lock() {
	if (m_outerMutex.try_lock()) {
		if (m_innerMtx.try_lock()) {
			return true;
		}
		m_outerMutex.unlock();
		return false;
	}
	return false;
}

void shared_mutex::unlock() {
	m_innerMtx.unlock();
	m_outerMutex.unlock();
}

bool shared_mutex::try_lock_shared() {
	if (m_outerMutex.try_lock()) {
		auto prev = m_innerCount++;
		if (prev == 0) {
			// Inner mutex is in the process of being release by unlock_shared, so this must succeed pretty soon.
			while (!m_innerMtx.try_lock()) {}
			return true;
		}
		return true;
	}
	return false;
}

void shared_mutex::unlock_shared() {
	auto current = --m_innerCount;
	if (current == 0) {
		m_innerMtx.unlock();	
	}	
}

}
