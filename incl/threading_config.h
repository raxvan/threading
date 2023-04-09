
#pragma once

//--------------------------------------------------------------------------------------------------------------------------------

#define THREADING_ENABLE_ASSERT

//--------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------
#if defined(THREADING_TESTING)

#	include <ttf.h>
#	define THREADING_ASSERT TTF_ASSERT

#elif defined(THREADING_ENABLE_ASSERT)
//--------------------------------------------------------------------------------------------------------------------------------

#	define THREADING_ASSERT DEV_ASSERT

#endif
//--------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------

#ifndef THREADING_ASSERT

#	define THREADING_ASSERT(...) \
		do                        \
		{                         \
		} while (false)
#endif

#ifndef THREADING_ASSERT_FALSE
#	define THREADING_ASSERT_FALSE(...) \
		do                              \
		{                               \
			THREADING_ASSERT(false);    \
		} while (false)

#endif

//--------------------------------------------------------------------------------------------------------------------------------

#include <devtiny.h>

#if defined(_MSC_VER)
#	include <intrin.h>

#	if defined(_M_AMD64) || defined(_M_IX86)
#		pragma intrinsic(_mm_pause)
#		define threading_impl_spin_yield() _mm_pause()
#	elif defined(_M_IA64)
#		pragma intrinsic(__yield)
#		define threading_impl_spin_yield() __yield()
#	else
#		define threading_impl_spin_yield() \
			do                              \
			{                               \
			} while (false)
#	endif
#endif

//--------------------------------------------------------------------------------------------------------------------------------

namespace threading
{

	struct utils
	{
		static void lock_current_thread_to_core(const std::size_t core_index);
		static void sleep_thread(const uint32_t ms_time);


		template <class F>
		inline static void start_native(std::thread& out, F&& _func)
		{
			std::thread tmp(std::move(_func));
			THREADING_ASSERT(out.joinable() == false);
			out.swap(tmp);
		}
		template <class F>
		inline static void start_native(std::thread& out, const F& _func)
		{
			std::thread tmp(_func);
			THREADING_ASSERT(out.joinable() == false);
			out.swap(tmp);
		}
	};

}
