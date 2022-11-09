#pragma once

#include "thread_primitives.h"

namespace threading
{

	struct unique_latch_events
	// when two threads meet with the same string they both continue;
	// the "unique" stands for "slow", one time use
	{
	public:
		// TODO: arrive_and_continue(const dev::static_string& s)

		void arrive_and_wait(const dev::static_string_hash32& s);

	protected:
		struct latch_info
		{
			std::mutex				mutex;
			std::condition_variable cv;
		};

		struct latch_entry
		{
			dev::static_string_hash32 key;
			latch_info*		   latch = nullptr;
		};

		latch_info* alloc_latch();
		void		 free_latch(latch_info* e);

	protected:
		threading::spin_lock	 m_container_lock;
		std::vector<latch_entry> m_elements;
	};

}
