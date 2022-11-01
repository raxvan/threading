
#include <ttf.h>
#include <threading.h>
#include <nstimer.h>

#include <clock_counters.h>
#include <iostream>

#define THREAD_COUNT 3
#define MEMORY_SIZE 1024 * 1024 * 32 /*32 mb ram*/

class ThreadGrinder
{
public:
	std::atomic<bool> wait { true };

	std::mutex			  write_lock;
	std::vector<uint32_t> seq;

	std::array<std::thread, THREAD_COUNT> threads;
#ifdef NSTIMER_CYCLE_TIMER
	std::array<std::vector<uint32_t>, THREAD_COUNT> core_ids;
#endif

	// run 0+
	std::array<int64_t, THREAD_COUNT> avg_run_duration_all; // all runs

	// run 1+
#ifdef NSTIMER_CYCLE_TIMER
	std::array<nstimer::clock_cycle_timer::clock_count_t, THREAD_COUNT> avg_clock_cycles;
	std::array<nstimer::clock_cycle_timer::clock_count_t, THREAD_COUNT> avg_run_duration_local;
#endif

public:
	ThreadGrinder()
	{
		const uint32_t s = MEMORY_SIZE;
		seq.reserve(s);
		for (uint32_t i = 0; i < s; i++)
			seq.push_back(i);

		// start threads
		for (std::size_t i = 0; i < threads.size(); i++)
		{
			avg_run_duration_all[i] = 0;
#ifdef NSTIMER_CYCLE_TIMER
			avg_clock_cycles[i] = 0;
			avg_run_duration_local[i] = 0;
#endif
			std::thread([&, i]() { this->iterate_sequance(i, threads.size()); }).swap(threads[i]);
		}
		wait.store(false);
	}
	void wait_for_threads()
	{
		for (std::size_t i = 0; i < 3; i++)
			threads[i].join();

		std::cout << std::endl;
		for (std::size_t i = 0; i < threads.size(); i++)
		{
			std::cout << "avg exec time for thread " << i << " " << std::endl;
			nstimer::debug_utils::print_nice_delta("duration", double(avg_run_duration_all[i]));
		}

		for (std::size_t i = 1; i < threads.size(); i++)
		{
			TEST_ASSERT(avg_run_duration_all[0] < avg_run_duration_all[i]);
		}

#ifdef NSTIMER_CYCLE_TIMER

		std::cout << "core ids for first run:\n";
		for (std::size_t i = 0; i < threads.size(); i++)
		{
			const std::vector<uint32_t>& ids = core_ids[i];
			if (ids.size() == 0)
				continue;
			std::cout << "thread " << i << " [" << ids[0];
			for (std::size_t j = 1; j < ids.size(); j++)
			{
				std::cout << " " << ids[j];
			}
			std::cout << "]\n";
		}

		std::cout << "avrage clock cycles:\n";
		for (std::size_t i = 0; i < threads.size(); i++)
		{
			std::cout << "thread " << i << " clocks " << avg_clock_cycles[i] << std::endl;
		}

		auto compute_hz = [](const int64_t _duration, const uint64_t _clocks) {
			double duration = double(_duration);
			double clock = double(_clocks);
			double ratio = clock / duration;
			return ratio * 1000.0;
		};

		std::cout << "core0 estimated speed (thread 0) " << compute_hz(avg_run_duration_local[0], avg_clock_cycles[0]) << " MHz\n";
		for (std::size_t i = 1; i < threads.size(); i++)
		{
			std::cout << "core1 estimated speed (thread " << i << ") " << compute_hz(avg_run_duration_local[1], avg_clock_cycles[1]) << " MHz\n";
		}
#endif
	}

	void iterate_sequance(const std::size_t index, const std::size_t count)
	{
		// join all threads
		while (wait.load())
		{
			std::this_thread::yield();
		}

#ifdef NSTIMER_CYCLE_TIMER
		std::vector<uint32_t>& ids = core_ids[index];
#endif

		bool	 value_set = false;
		uint32_t value_current = 0;

		auto validate_value = [&](const uint32_t result) {
			if (value_set == false)
			{
				value_set = true;
				value_current = result;
			}
			TEST_ASSERT(result == value_current);
		};

		std::size_t run_count = 5;
		for (std::size_t run = 0; run < run_count; run++)
		{
			if (run > 1)
			{
				if (index == 0)
					threading::lock_current_thread_to_core(0);
				else
					threading::lock_current_thread_to_core(1);
			}

			{
				std::lock_guard<std::mutex> _(write_lock);
				std::this_thread::yield();
			}

#ifdef NSTIMER_CYCLE_TIMER
			auto start_clock = nstimer::clock_cycle_timer::clock_counter();
#endif
			nstimer::std_timer::time_capture_t start = nstimer::std_timer::capture_now_time();
			uint32_t						   xv = 0;
			{
				std::size_t read_index = 1;
				std::size_t yeld_counter = 0;
				for (std::size_t i = index; i < seq.size(); i += count)
				{
					read_index = (read_index + i) * 1106531;
					xv = xv ^ seq[read_index % seq.size()];
					yeld_counter++;
					if (yeld_counter > 1024)
					{
						yeld_counter = 0;

#ifdef NSTIMER_CYCLE_TIMER
						if (run == 0)
						{
							uint32_t cid = nstimer::clock_cycle_timer::core_id();
							auto	 itr = std::lower_bound(ids.begin(), ids.end(), cid);
							if (!(itr != ids.end() && *itr == cid))
								ids.insert(itr, cid);
						}
#endif
						std::this_thread::yield();
					}
				}
				validate_value(xv);
			}

#ifdef NSTIMER_CYCLE_TIMER
			if (run > 1)
			{
				uint32_t cid = nstimer::clock_cycle_timer::core_id();
				uint32_t expected_cid = index == 0 ? 0 : 1;
				TEST_ASSERT(expected_cid == cid);
			}
#endif
			nstimer::std_timer::time_capture_t end = nstimer::std_timer::capture_now_time();
#ifdef NSTIMER_CYCLE_TIMER
			auto end_clock = nstimer::clock_cycle_timer::clock_counter();
#endif
			{
				std::lock_guard<std::mutex> _(write_lock);
				int64_t						delta = nstimer::std_timer::delta_ns(start, end);
				std::cout << "thread index: " << index << " run itr " << run << std::endl;
				nstimer::debug_utils::print_nice_delta("duration", double(delta));
				avg_run_duration_all[index] += delta / run_count;

#ifdef NSTIMER_CYCLE_TIMER
				if (run > 1)
				{
					avg_clock_cycles[index] += (end_clock - start_clock) / (run_count - 1);
					avg_run_duration_local[index] += delta / (run_count - 1);
				}
#endif
			}
		}
	}
};

void test_thread_grind()
{
	ThreadGrinder r;
	r.wait_for_threads();
}