
#include "../incl/thread_primitives.h"

namespace threading
{
	barrier::barrier(const uint32_t group_size)
	{
		DEV_ASSERT(group_size > 0);
		m_group_size = group_size;
	}

	void barrier::reset_group_size(const uint32_t group_size)
	{
		DEV_ASSERT(group_size > 0);

		{
			std::unique_lock<std::mutex> lk(m_lock);
			m_count = 0;
			m_group_size = group_size;
		}
	}

	barrier::~barrier()
	{
		std::size_t original_generataion = std::numeric_limits<uint32_t>::max();
		{
			std::unique_lock<std::mutex> lk(m_lock);

			if (m_count > 0)
			{
				original_generataion = m_generation;
				m_cv.notify_all();
			}
		}
		while (m_generation.load() == original_generataion)
			std::this_thread::yield();
	}

	void barrier::arrive_and_wait()
	{
		std::size_t base_generation;
		{
			std::unique_lock<std::mutex> lk(m_lock);

			base_generation = m_generation;

			m_count++;

			if (m_count == m_group_size)
			{
				m_generation++;
				m_cv.notify_all();
			}
			else
			{
				m_cv.wait(lk, [this, base_generation]() { return base_generation != m_generation; });
			}

			if ((--m_count) == 0)
			{
				m_generation++;
				return;
			}
		}
		while (m_generation.load() != (base_generation + 2))
			std::this_thread::yield();
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	latch::latch(const uint32_t group_size)
		: m_group_size(group_size)
	{
	}

	latch::~latch()
	{
		{
			std::unique_lock<std::mutex> lk(m_lock);
			if (m_count == m_group_size)
				m_cv.notify_all();
			else
				m_cv.wait(lk, [this]() { return m_count == m_group_size; });
		}
		while (m_entered_count.load() != 0)
			std::this_thread::yield();
	}

	void latch::arrive_and_wait()
	{
		m_entered_count++;
		{
			std::unique_lock<std::mutex> lk(m_lock);
			m_count++;
			if (m_count == m_group_size)
				m_cv.notify_all();
			else
				m_cv.wait(lk, [this]() { return m_count == m_group_size; });
		}
		m_entered_count--;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	swap_barrier::swap_barrier(const uint32_t group_size)
		: m_group_size(group_size)
	{
	}

	swap_barrier::~swap_barrier()
	{
		break_locks();
		while (m_entered_count.load() != 0)
			std::this_thread::yield();
	}

	void swap_barrier::break_locks()
	{
		std::unique_lock<std::mutex> lk(m_lock);
		m_count = (std::numeric_limits<uint32_t>::max() / 2);
		m_cv_wait.notify_all();
		m_cv_lock.notify_all();
	}
	bool swap_barrier::broken() const
	{
		return m_count >= (std::numeric_limits<uint32_t>::max() / 2);
	}

	void swap_barrier::arrive_and_wait()
	{
		m_entered_count++;
		{
			std::unique_lock<std::mutex> lk(m_lock);
			m_count++;

			if (m_count == m_group_size)
				m_cv_lock.notify_all();

			m_cv_wait.wait(lk, [this]() { return m_count == 0 || broken(); });
		}
		m_entered_count--;
	}

	void swap_barrier::arrive_and_lock()
	{
		m_entered_count++;

		std::unique_lock<std::mutex> lk(m_lock);
		m_count++;

		if (m_count != m_group_size && broken() == false)
			m_cv_lock.wait(lk, [this]() { return m_count == m_group_size || broken(); });

		lk.release();
	}
	void swap_barrier::unlock()
	{
		if (broken() == false)
		{
			DEV_ASSERT(m_count == m_group_size);
			m_count = 0;
			m_cv_wait.notify_all();
		}
		m_lock.unlock();
		m_entered_count--;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

}
