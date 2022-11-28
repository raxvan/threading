
#include "../incl/locked_wait.h"

namespace threading
{

	bool locked_wait::awake_one(std::mutex& parent_mutex)
	{
		if(m_await_counter > 0)
		{
			std::unique_lock<std::mutex> lk(parent_mutex);
			m_sleep_trigger.notify_one();
			return true;
		}
		return false;
	}
	uint32_t locked_wait::awake_all(std::mutex& sleep_mutex)
	{
		if(m_await_counter > 0)
		{
			auto r = m_await_counter;
			std::unique_lock<std::mutex> lk(sleep_mutex);
			m_sleep_trigger.notify_all();
			return uint32_t(r);
		}
		return 0;
	}

	void locked_wait::wait(threading::spin_lock& root_mutex, std::mutex& wait_mutex)
	{
		m_await_counter++;
		{
			std::unique_lock<std::mutex> lk(wait_mutex);
			root_mutex.unlock();
			m_sleep_trigger.wait(lk);
		}
		root_mutex.lock();
		m_await_counter--;
	}



}
