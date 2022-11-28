
#include "../incl/native_threads.h"

namespace threading
{
	ThreadGroup::~ThreadGroup()
	{
		THREADING_ASSERT(m_joined_threads.load() == 0);
	}

	uint32_t ThreadGroup::size()
	{
		return m_joined_threads.load();
	}

	void ThreadGroup::start_one(const uint32_t index, iThreadContext* handler, threading::latch* init_barrier)
	{
		THREADING_ASSERT(handler != nullptr);
		THREADING_ASSERT(handler->m_handle.joinable() == false);

		std::thread t = std::thread([this, handler, index, init_barrier]() {
			handler->init(this, index);

			if (init_barrier != nullptr)
				init_barrier->arrive_and_wait();

			this->m_joined_threads++;
			handler->execute(this, index);
			this->m_joined_threads--;
		});

		handler->m_handle.swap(t);
	}
	void ThreadGroup::join_one(iThreadContext* handler)
	{
		THREADING_ASSERT(handler != nullptr);
		if (handler->m_handle.joinable())
			handler->m_handle.join();
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	bool iThreadContext::wait()
	{
		if (m_handle.joinable())
		{
			m_handle.join();
			return true;
		}
		return false;
	}
	void iThreadContext::swap(iThreadContext& other)
	{
		m_handle.swap(other.m_handle);
	}

	iThreadContext::iThreadContext(iThreadContext&& other) noexcept
	{
		m_handle.swap(other.m_handle);
	}
	iThreadContext& iThreadContext::operator=(iThreadContext&& other) noexcept
	{
		if (m_handle.joinable())
			m_handle.join();
		std::thread tmp;
		m_handle.swap(tmp);
		m_handle.swap(other.m_handle);
		return (*this);
	}

}
