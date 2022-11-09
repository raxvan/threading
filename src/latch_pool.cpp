#include "../incl/latch_pool.h"

namespace threading
{
	void unique_latch_events::arrive_and_wait(const dev::static_string_hash32& s)
	{
		m_container_lock.lock();

		auto itr = std::find_if(m_elements.begin(), m_elements.end(), [&](const latch_entry& e) { return e.key == s; });

		// setup latch
		if (itr == m_elements.end())
		{
			// first to arrive
			latch_entry e;
			e.key = s;
			e.latch = alloc_latch();

			m_elements.insert(m_elements.end(), e);
			// is new entry, wait
			auto* latch_ptr = e.latch;

			{
				std::unique_lock<std::mutex> lk(latch_ptr->mutex);
				m_container_lock.unlock();
				latch_ptr->cv.wait(lk);
			}

			free_latch(latch_ptr);
		}
		else
		{
			// second to arrive
			{
				auto*						 latch_ptr = itr->latch;
				std::unique_lock<std::mutex> _(latch_ptr->mutex);
				latch_ptr->cv.notify_one();
			}

			itr = std::find_if(m_elements.begin(), m_elements.end(), [&](const latch_entry& e) { return e.key == s; });
			m_elements.erase(itr);

			m_container_lock.unlock();
		}
	}

	unique_latch_events::latch_info* unique_latch_events::alloc_latch()
	{
		return new unique_latch_events::latch_info;
	}
	void unique_latch_events::free_latch(unique_latch_events::latch_info* e)
	{
		delete e;
	}

}