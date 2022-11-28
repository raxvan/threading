
#include <threading.h>

#include <iostream>

void test_async_pipe1()
{
	using namespace std::chrono_literals;

	threading::async_pipe<uint64_t> p;

	std::atomic<std::size_t> count_values { 0 };

	std::mutex				create_lock;
	std::condition_variable cv;

	std::array<std::thread, 32> threads;

	create_lock.lock();
	for (std::size_t i = 0; i < threads.size(); i++)
	{
		auto t = std::thread([&]() {
			{
				std::unique_lock<std::mutex> lk(create_lock);
				cv.wait(lk);
			}
			auto start = std::chrono::high_resolution_clock::now();
			p.consume_or_wait(true, [&](const uint64_t value) {
				std::this_thread::sleep_for(100ms);
				if (value < 20)
					p.push_back(value + 1);
				else
					count_values++;
			});
			auto									  end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> elapsed = end - start;
			TEST_ASSERT(elapsed.count() > 1500);
		});

		threads[i].swap(t);
	}
	create_lock.unlock();
	std::this_thread::sleep_for(500ms);
	cv.notify_all();

	std::size_t vc = threads.size() / 2;
	for (std::size_t i = 0; i < vc; i++)
		p.push_back(0);

	auto start = std::chrono::high_resolution_clock::now();
	p.wait_empty();
	auto									  end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = end - start;
	TEST_ASSERT(elapsed.count() > 1500);

	TEST_ASSERT(count_values.load() == vc);
	for (std::size_t i = 0; i < threads.size(); i++)
		threads[i].join();
}

void test_async_pipe2()
{
	using namespace std::chrono_literals;

	threading::async_pipe<uint64_t> p;

	std::atomic<std::size_t> count_values { 0 };

	std::mutex				create_lock;
	std::condition_variable cv;

	std::array<std::thread, 32> threads;

	create_lock.lock();
	for (std::size_t i = 0; i < threads.size(); i++)
	{
		auto t = std::thread([&]() {
			{
				std::unique_lock<std::mutex> lk(create_lock);
				cv.wait(lk);
			}
			auto start = std::chrono::high_resolution_clock::now();
			p.consume_or_wait(true, [&](const uint64_t value) {
				std::this_thread::sleep_for(100ms);
				if (value < 20)
					p.push_back(value + 1);
				else
					count_values++;
			});
			auto									  end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> elapsed = end - start;
			TEST_ASSERT(elapsed.count() > 1500);
		});

		threads[i].swap(t);
	}
	create_lock.unlock();
	std::this_thread::sleep_for(500ms);

	std::size_t vc = threads.size() / 2;
	for (std::size_t i = 0; i < vc; i++)
		p.push_back(0);

	cv.notify_all();

	auto start = std::chrono::high_resolution_clock::now();
	p.consume_or_wait(true, [&](const uint64_t value) {
		std::this_thread::sleep_for(75ms);
		if (value < 20)
			p.push_back(value + 1);
		else
			count_values++;
	});
	auto									  end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = end - start;
	TEST_ASSERT(elapsed.count() > 1500);

	TEST_ASSERT(count_values.load() == vc);
	for (std::size_t i = 0; i < threads.size(); i++)
		threads[i].join();
}

void test_async_pipe3()
{
	using namespace std::chrono_literals;

	threading::async_pipe<uint64_t> p;

	std::atomic<std::size_t> count { 0 };

	std::mutex				create_lock;
	std::condition_variable cv;

	std::array<std::thread, 32> threads;

	create_lock.lock();
	for (std::size_t i = 0; i < threads.size(); i++)
	{
		auto t = std::thread([&]() {
			{
				std::unique_lock<std::mutex> lk(create_lock);
				cv.wait(lk);
			}
			auto start = std::chrono::high_resolution_clock::now();
			p.consume_or_wait(true, [&](const uint64_t value) {
				std::this_thread::sleep_for(100ms);
				if (value < 20)
					p.push_back(value + 1);
			});
			auto									  end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> elapsed = end - start;
			TEST_ASSERT(elapsed.count() < 150);
		});

		threads[i].swap(t);
	}
	create_lock.unlock();
	std::this_thread::sleep_for(500ms);
	cv.notify_all();

	std::size_t vc = threads.size() / 2;
	for (std::size_t i = 0; i < vc; i++)
		p.push_back(0);
	std::this_thread::sleep_for(50ms);
	auto start = std::chrono::high_resolution_clock::now();
	p.clear(10);
	auto									  end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = end - start;
	TEST_ASSERT(elapsed.count() < 150);

	for (std::size_t i = 0; i < threads.size(); i++)
		threads[i].join();
}
