#pragma once

#include "thread_primitives.h"
#include <vector>

namespace threading
{

	/*
	struct unique_latch_events
	// when two threads meet with the same string they both continue;
	// the "unique" stands for "slow", one time use
	// looking back on this... this i dumb
	{
	public:
		// TODO: arrive_and_continue(const dev::static_string& s)

		void arrive_and_wait(const sg::compiletime_identifier& s);

	protected:
		struct latch_info
		{
			std::mutex				mutex;
			std::condition_variable cv;
		};

		struct latch_entry
		{
			sg::compiletime_identifier key;
			latch_info*				   latch = nullptr;
		};

		latch_info* alloc_latch();
		void		free_latch(latch_info* e);

	protected:
		threading::spin_lock	 m_container_lock;
		std::vector<latch_entry> m_elements;
	};
	*/

}
