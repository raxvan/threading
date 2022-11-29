

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

}