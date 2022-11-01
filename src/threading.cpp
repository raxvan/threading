
#include "../incl/threading.h"

#include <thread>

#if DEV_PLATFORM_LIN()
#	include <pthread.h>
#endif

#ifdef _MSC_VER
#	include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
//^ explain yourself !!!
#	include <time.h> // for nanosleep
#else
#	include <unistd.h> // for usleep
#endif

namespace threading
{

	void lock_current_thread_to_core(const std::size_t core_index)
	{
#if DEV_PLATFORM_WIN()
		SetThreadAffinityMask(GetCurrentThread(), (std::size_t(1) << core_index));
#endif

#if DEV_PLATFORM_LIN()
		cpu_set_t cpuset;
		pthread_t thread = pthread_self();

		CPU_ZERO(&cpuset);
		int index = int(core_index);
		CPU_SET(index, &cpuset);

		pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset);
#endif
	}

	void sleep_thread(const uint32_t ms_time)
	{
		std::this_thread::yield();
#ifdef WIN32
		Sleep(int(ms_time));
#elif _POSIX_C_SOURCE >= 199309L
		struct timespec ts;
		ts.tv_sec = int(ms_time) / 1000;
		ts.tv_nsec = (int(ms_time) % 1000) * 1000000;
		nanosleep(&ts, NULL);
#else
		if (int(ms_time) >= 1000)
			sleep(int(ms_time) / 1000);
		usleep((int(ms_time) % 1000) * 1000);
#endif
	}

}
