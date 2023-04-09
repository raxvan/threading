
#include <threading.h>

#include <iostream>

void test_async_pipe1()
{
	using namespace std::chrono_literals;

	threading::async_pipe<uint64_t> p;

	std::atomic<uint64_t>	 sum { 0 };
	std::atomic<std::size_t> count_values { 0 };

	threading::thread_group threads;

	auto consume_item = [&](const uint64_t value) {
		sum += value;

		if (sum % 2 == 0)
			std::this_thread::sleep_for(13ms);
		else
			std::this_thread::sleep_for(10ms);
	};
	{
		threads.spawn(32, [&]() {
			p.consume_loop_or_wait(consume_item);
		});
	}

	std::size_t vc = 1000;
	for (std::size_t i = 0; i < vc; i++)
		p.push_back(i);

	p.wait_for_empty();

	TEST_ASSERT(sum.load() == (vc * (vc - 1)) / 2);

	p.evict(threads.size(), 0);

}

void test_async_pipe2()
{
	using namespace std::chrono_literals;

	threading::async_pipe<uint64_t> p;

	std::atomic<uint64_t>	 count { 0 };
	std::atomic<std::size_t> count_values { 0 };

	threading::thread_group threads;

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
		threads.spawn(32, [&]() {
			p.consume_loop_or_wait(consume_item);
		});
	}

	std::size_t vc = 100;
	for (std::size_t i = 0; i < vc; i++)
		p.push_back(0);

	p.wait_for_empty();

	TEST_ASSERT(count.load() == vc);

	p.evict(threads.size(), 0);
}

void test_async_pipe3()
{
	using namespace std::chrono_literals;

	threading::async_pipe<uint64_t> p;

	std::atomic<uint64_t>	 sum{ 0 };
	
	threading::thread_group threads;
	std::atomic<bool>			closing{ false };

	auto consume_item = [&](const uint64_t value) {
		sum += value;
	};

	std::atomic<std::size_t> count_check{ 0 };
	{

		threads.spawn(32, [&]() {
			p.consume_loop_or_wait(consume_item);
			count_check++;
			p.consume_loop_or_wait(consume_item);
		});
	}
	std::size_t vc = 512;
	for (std::size_t i = 0; i < vc; i++)
		p.push_back(1);

	p.evict(threads.size(), 0);
	threading::utils::sleep_thread(250);
	TEST_ASSERT(count_check.load() == threads.size());
	
	for (std::size_t i = 0; i < vc; i++)
		p.push_back(1);
	
	p.wait_for_empty();
	TEST_ASSERT(sum.load() == 1024);
	
	p.evict(threads.size(), 0);
}

void test_async_pipe()
{
	TEST_FUNCTION(test_async_pipe1);
	TEST_FUNCTION(test_async_pipe2);
	TEST_FUNCTION(test_async_pipe3);
}
