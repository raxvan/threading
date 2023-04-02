
#include <threading.h>

#include <iostream>

void test_spin_value_lock()
{
	std::array<std::thread, 32> threads;

	threading::spin_value_lock<uint32_t> spvalue(0);
	std::size_t iterations = 4096 * 16;

	std::atomic<uint64_t> total_time{ 0 };
	for(auto & t : threads)
	threading::utils::start_native(t, [&](){

		ttf::timer timer;
		for (std::size_t i = 0; i < iterations; i++)
		{
			auto v = spvalue.lock();
			v = v + 1;
			spvalue.unlock(v);
		}
		total_time += timer.get_nanoseconds();
	});

	for(auto & t : threads)
		t.join();

	nstimer::debug_utils::print_nice_delta("Total Thread time:", double(total_time.load()) / threads.size());

	std::size_t expected = iterations * threads.size();
	
	TEST_ASSERT(spvalue.islocked() == false);
	TEST_ASSERT(spvalue.peek() == uint32_t(expected));

}




