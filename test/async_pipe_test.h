
#include <threading.h>

#include <iostream>

void test_async_pipe1()
{
	using namespace std::chrono_literals;

	threading::async_pipe<uint64_t> p;

	std::atomic<uint64_t> sum { 0 };
	std::atomic<std::size_t> count_values{ 0 };

	std::array<std::thread, 32> threads;
	std::atomic<bool> closing{ false };

	auto consume_item = [&](const uint64_t value) {

		sum += value;

		if (sum % 2 == 0)
			std::this_thread::sleep_for(13ms);
		else
			std::this_thread::sleep_for(10ms);

	};
	{
		threading::latch l{ threads.size() + 1 };

		for (std::size_t i = 0; i < threads.size(); i++)
		{
			auto t = std::thread([&]() {
				l.arrive_and_wait();
				if (i % 2 == 0)
				{
					while (p.consume_loop(consume_item) == true);
					TEST_ASSERT(closing.load() == true);
				}
				else
				{
					p.consume_loop_or_wait(consume_item);
				}
			});

			threads[i].swap(t);
		}
		l.arrive_and_wait();
	}

	std::size_t vc = 1000;
	for (std::size_t i = 0; i < vc; i++)
		p.push_back(i);

	p.wait_for_empty();

	TEST_ASSERT(sum.load() == (vc * (vc - 1)) / 2);

	closing = true;
	p.evict(10);
	for (std::size_t i = 0; i < threads.size(); i++)
		threads[i].join();
}

void test_async_pipe2()
{
	using namespace std::chrono_literals;

	threading::async_pipe<uint64_t> p;

	std::atomic<uint64_t> count{ 0 };
	std::atomic<std::size_t> count_values{ 0 };

	std::array<std::thread, 32> threads;
	std::atomic<bool> closing {false};

	auto consume_item = [&](const uint64_t value) {

		if (value < 10)
			p.push_back(value + 1);
		else
			count++;

		if (count % 2 == 0)
			std::this_thread::sleep_for(13ms);
		else
			std::this_thread::sleep_for(10ms);
	};

	{
		threading::latch l{ threads.size() + 1 };

		for (std::size_t i = 0; i < threads.size(); i++)
		{
			auto t = std::thread([&, i]() {
				l.arrive_and_wait();
				if (i % 2 == 0)
				{
					while (p.consume_loop(consume_item) == true);
					TEST_ASSERT(closing.load() == true);
				}
				else
				{
					p.consume_loop_or_wait(consume_item);
				}
			});

			threads[i].swap(t);
		}
		l.arrive_and_wait();
	}

	std::size_t vc = 100;
	for (std::size_t i = 0; i < vc; i++)
		p.push_back(0);

	p.wait_for_empty();

	TEST_ASSERT(count.load() == vc);

	closing = true;
	p.evict();

	for (std::size_t i = 0; i < threads.size(); i++)
		threads[i].join();
}

void test_async_pipe()
{
	TEST_FUNCTION(test_async_pipe1);
	TEST_FUNCTION(test_async_pipe2);
}
