#pragma once


#include "threading_config.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace threading
{

	//--------------------------------------------------------------------------------------------------------------------------------

	struct spin_lock
	{
		std::atomic<bool> flag = { false };

		inline void lock() noexcept
		{
			while (true)
			{
				if (flag.exchange(true, std::memory_order_acquire) == false)
					break;

				while (flag.load(std::memory_order_relaxed))
				{
					// yield for hyperthreading improvements:
					//__builtin_ia32_pause
				}
			}
		}

		inline void unlock() noexcept
		{
			flag.store(false, std::memory_order_release);
		}
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	struct semaphore
	{
	public:
		semaphore(const semaphore&) = delete;
		semaphore& operator=(const semaphore&) = delete;

	public:
		inline semaphore(const std::size_t n)
			: m_available(n)
		{
		}

	public:
		inline void acquire()
		{
			std::unique_lock<std::mutex> lk(m_lock);
			if (m_available == 0)
				m_cv.wait(lk, [this] { return m_available == 0; });
			m_available--;
		}

		inline void release()
		{
			std::unique_lock<std::mutex> lock(m_lock);
			m_available++;
			m_cv.notify_one();
		}

	protected:
		std::mutex				m_lock;
		std::condition_variable m_cv;
		std::size_t				m_available;
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	//--------------------------------------------------------------------------------------------------------------------------------
	struct latch
	// like a barrier, but not reusable
	{
	public:
		latch(const latch&) = delete;
		latch(latch&&) = delete;
		latch& operator=(const latch&) = delete;
		latch& operator=(latch&&) = delete;

	public:
		explicit latch(const uint32_t group_size);

	public:
		~latch(); // safely destroys latch

		void arrive_and_wait();
		// all thread are blocked here until the number of threads waiting is equal with group size
		// does not reset anything

		inline uint32_t group_size() const
		{
			return m_group_size;
		}

	protected:
		std::atomic<uint32_t>	m_entered_count { 0 };
		std::mutex				m_lock;
		std::condition_variable m_cv;
		uint32_t				m_count = 0;
		uint32_t				m_group_size = 0;
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	struct barrier
	// similar to a latch, but barrier resets after each arrive_and_wait();
	{
	public:
		barrier(const barrier&) = delete;
		barrier& operator=(const barrier&) = delete;

	public:
		barrier() = default;
		barrier(const uint32_t group_size);

	public:
		~barrier();

		void arrive_and_wait();

	public:
		void reset_group_size(const uint32_t group_size);

	protected:
		std::mutex				m_lock;
		std::condition_variable m_cv;
		std::atomic<uint32_t>	m_generation { 0 };
		uint32_t				m_count = 0;
		uint32_t				m_group_size = 0;
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	struct swap_barrier
	// similar to a barrier, all threads need (except one) to arrive_and_wait, all threads will start when all threads wait
	// the "odd" thread will call arrive_and_lock then unlock, this combination is similar to arrive_and_wait(), but work can be done
	{
	public:
		swap_barrier(const uint32_t group_size); // max(uint32_t) / 4 max group
		~swap_barrier();

	public:
		void arrive_and_wait();

		void arrive_and_lock();
		void unlock(); // needs to be called only

		void break_locks();

		bool broken() const; // only use between arrive_and_lock() and unlock()

	public:
		template <class F>
		inline void arrive(const F& _func)
		{
			arrive_and_lock();
			_func();
			unlock();
		}

		inline uint32_t group_size() const
		{
			return m_group_size;
		}

	protected:
		std::atomic<uint32_t>	m_entered_count { 0 };
		std::mutex				m_lock;
		std::condition_variable m_cv_wait;
		std::condition_variable m_cv_lock;
		uint32_t				m_count = 0;
		uint32_t				m_group_size = 0;
	};

	//--------------------------------------------------------------------------------------------------------------------------------

}
