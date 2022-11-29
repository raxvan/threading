#pragma once

#include "thread_primitives.h"

namespace threading
{
	//used by async pipe
	struct locked_wait
	{
	public:
		void wait(threading::spin_lock& root_mutex, std::mutex& wait_mutex);
		
		bool awake_one(std::mutex& parent_mutex);
		uint32_t awake_all(std::mutex& parent_mutex);

	protected:
		int_fast32_t            m_await_counter = 0;
		std::condition_variable m_sleep_trigger;
	};



}
