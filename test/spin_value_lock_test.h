
#include <threading.h>

#include <iostream>

void test_spin_value_lock()
{
	std::array<std::thread, 32> threads;

	threading::spin_value_lock<uint32_t> spvalue(0);
	std::size_t iterations = 4096 * 16;

	std::atomic<int64_t> total_time{ 0 };
	for(auto & t : threads)
	threading::utils::start_native(t, [&](){

		nstimer::std_timer::time_capture_t start = nstimer::std_timer::capture_now_time();
		for (std::size_t i = 0; i < iterations; i++)
		{
			auto v = spvalue.lock();
			v = v + 1;
			spvalue.unlock(v);
		}
		nstimer::std_timer::time_capture_t end = nstimer::std_timer::capture_now_time();
		int64_t	delta = nstimer::std_timer::delta_ns(start, end);
		total_time += delta;
	});

	for(auto & t : threads)
		t.join();

	nstimer::debug_utils::print_nice_delta("Total Thread time:", double(total_time.load()) / threads.size());

	std::size_t expected = iterations * threads.size();
	
	TEST_ASSERT(spvalue.islocked() == false);
	TEST_ASSERT(spvalue.peek() == uint32_t(expected));

}




