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
		using lock_guard = std::lock_guard<spin_lock>;

		std::atomic<bool> flag = { false };

		inline void lock() noexcept
		{
			while (true)
			{
				if (flag.exchange(true, std::memory_order_acquire) == false)
					break;

				while (flag.load(std::memory_order_relaxed))
				{
					// threading_impl_spin_yield();
				}
			}
		}

		inline void unlock() noexcept
		{
			flag.store(false, std::memory_order_release);
		}

		inline lock_guard guard() noexcept
		{
			return lock_guard(*this);
		}

	};

	//--------------------------------------------------------------------------------------------------------------------------------

	//one writer, multiple readers lock (limited to 2B readers, and 1 writer)
	struct mr_spin_lock
	{
	public:
		void write_lock();
		void write_unlock();

	public:
		//for readers
		void lock();
		void unlock();
	protected:
		std::atomic<uint32_t> m_readers{0};
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	template <class T>
	struct spin_value_lock
	{
	public:
		static_assert(std::is_integral<T>::value, "The provided type must be an integral number (no floating-point types allowed).");

		using value_t = T;

		inline static bool value_set(const value_t data_value)
		{
			return data_value != std::numeric_limits<value_t>::max();
		}
		inline static bool value_locked(const value_t data_value)
		{
			return data_value == std::numeric_limits<value_t>::max();
		}
		inline static bool value_default(const value_t data_value)
		{
			THREADING_ASSERT(value_locked(data_value) == false);
			return data_value == (std::numeric_limits<value_t>::max() - 1);
		}
	public:
		struct lock_guard
		{
			spin_value_lock<T>& 	lock;
			value_t 				value;
			lock_guard(spin_value_lock<T>& spi)
				:lock(spi)
				,value(spi.lock())
			{
			}
			~lock_guard()
			{
				lock.unlock(value);
			}
		};
	public:
		spin_value_lock() = default;
		spin_value_lock(const value_t index)
			:data{index}
		{
			THREADING_ASSERT(value_set(index));
		}

		inline value_t lock() noexcept
		{
			while (true)
			{
				value_t data_value = data.exchange(std::numeric_limits<value_t>::max(), std::memory_order_acquire);
				if(value_set(data_value))
					return data_value;
				do
				{
					//threading_impl_spin_yield();
				}
				while (value_locked(data.load(std::memory_order_relaxed)));
			}
		}

		template <class F>
		//bool _func(value_t); will wait until _func() returns true, _func is called in in locked state;
		inline value_t lock_if(const F& _func) noexcept
		{
			while (true)
			{
				value_t data_value = data.exchange(std::numeric_limits<value_t>::max(), std::memory_order_acquire);
				if(value_locked(data_value))
				{
					do
					{
						// yield for hyperthreading improvements:
						//__builtin_ia32_pause
					}
					while (value_locked(data.load(std::memory_order_relaxed)));
				}
				else if(_func(data_value))
				{
					THREADING_ASSERT(value_set(data.load(std::memory_order_relaxed)));
					return data_value;
				}
				else
				{
					THREADING_ASSERT(value_set(data.load(std::memory_order_relaxed)));
					data.store(data_value, std::memory_order_release);
				}
			}
		}


		inline value_t trylock() noexcept
		{
			return data.exchange(std::numeric_limits<value_t>::max(), std::memory_order_acquire);
		}

		inline void unlock(const value_t data_value) noexcept
		{
			data.store(data_value, std::memory_order_release);
		}

		inline value_t peek() const noexcept
		{
			while (true)
			{
				value_t data_value = data.load(std::memory_order_relaxed);
				if(value_set(data_value))
					return data_value;
			}
		}

		inline bool islocked() const noexcept
		{
			return value_locked(data.load(std::memory_order_relaxed));
		}

		inline lock_guard guard() noexcept
		{
			return lock_guard(*this);
		}
	public:
		std::atomic<value_t> data = { std::numeric_limits<value_t>::max() - 1 };
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
		explicit latch(const std::size_t group_size);

	public:
		~latch(); // safely destroys latch

		void arrive_and_wait();
		// all thread are blocked here until the number of threads waiting is equal with group size
		// does not reset anything

		inline uint_fast32_t group_size() const
		{
			return m_group_size;
		}

	protected:
		std::atomic<uint_fast32_t>	m_entered_count { 0 };
		std::mutex					m_lock;
		std::condition_variable		m_cv;
		uint_fast32_t				m_count = 0;
		uint_fast32_t				m_group_size = 0;
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	struct barrier
	// similar to a latch, but barrier resets after group reaches arrive_and_wait();
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
	// the "odd" thread will call arrive_and_lock then unlock, this call sequance is similar to arrive_and_wait(), in between
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
