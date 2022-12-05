
#include <ttf.h>
#include <threading.h>
#include <array>


void test_multi_read_spin_lock()
{
	std::array<std::thread, 64> threads;

	std::array<int64_t, 512> data;
	for (std::size_t i = 0; i < data.size(); i += 2)
	{
		data[i + 0] = -1;
		data[i + 1] = 1;
	}

	threading::mr_spin_lock ml;
	std::atomic<bool> spin{ true };

	auto checksum = [&](std::size_t index) {
		int64_t r = 0;
		for (std::size_t i = 0; i < data.size(); i++)
		{
			std::size_t ix = ((i + index) * 4261) % data.size();
			r += data[ix];
		}
		return r;
	};

	for(std::size_t i = 0; i < threads.size();i++)
		TEST_ASSERT(checksum(i) == 0);

	{
		threading::latch l{ uint32_t(threads.size() + 1) };

		auto thread_job = [&](const std::size_t index) {
			
			while (spin.load())
			{
				uint64_t r = 0;
				{
					std::lock_guard<threading::mr_spin_lock> _(ml);
					r = checksum(index);
				}
				TEST_ASSERT(r == 0);
			}

		};

		for (std::size_t i = 0; i < threads.size(); i++)
		{
			auto t = std::thread([&, index = i]() {
				l.arrive_and_wait();
				thread_job(index);
				});
			threads[i].swap(t);
		}

		l.arrive_and_wait();

		std::size_t x = 0;
		std::size_t y = 0;
		for (std::size_t itr = 0; itr < 1024; itr++)
		{
			ml.write_lock();
			for (std::size_t i = 0; i < 128; i++)
				data[((x++) * 3659) % data.size()]++;
			for (std::size_t i = 0; i < 128; i++)
				data[((y++) * 5227) % data.size()]--;
			ml.write_unlock();

			TEST_ASSERT(checksum(0) == 0);
		}

		spin = false;

		for (std::size_t i = 0; i < threads.size(); i++)
			threads[i].join();
	}



}