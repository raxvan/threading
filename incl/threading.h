

#pragma once

#include "thread_primitives.h"
#include "native_threads.h"
#include "async_pipe.h"
#include "latch_pool.h"

namespace threading
{

	//--------------------------------------------------------------------------------------------------------------------------------

	extern void lock_current_thread_to_core(const std::size_t core_index);
	extern void sleep_thread(const uint32_t ms_time);

	//--------------------------------------------------------------------------------------------------------------------------------

	struct utils
	{
		template <class F>
		inline static void start_native(std::thread& out, F&& _func)
		{
			std::thread tmp(std::move(_func));
			THREADING_ASSERT(out.joinable() == false);
			out.swap(tmp);
		}
	};

}
