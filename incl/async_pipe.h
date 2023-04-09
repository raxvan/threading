#pragma once

#include "locked_wait.h"

namespace threading
{

	// for multiple producers/multiple consumers of data T & multiple waiting threads
	// low overhead when prodicing/consuming, consumers go to sleep when idle
	template <class T>
	struct async_pipe
	{
	public:
		using consume_func_t = void(T&&);

	public:
		// for consumers:
		// consume func type: void(T&&)/bool(T&&)

		template <class F>
		// returns only when evicted
		void consume_loop_or_wait(const F& _func)
		{
			std::lock_guard<threading::spin_lock> _(m_first_lock);
			do
			{
				_consume_all_locked(_func);
			} while (_wait_locked());
			_end_consumer(false);
		}

		template <class F>
		// void(T&&);
		// try to get, immediately return when no items are available
		// returns false when evicted
		bool consume_loop(const F& _func, const bool wait_for_empty = false)
		{
			std::lock_guard<threading::spin_lock> _(m_first_lock);
			_consume_all_locked(_func);
			return _end_consumer(wait_for_empty);
		}
		template <class F>
		// bool(T&&)
		// try to get, immediately return when no items are available
		// returns false when evicted
		bool consume_while(const F& _func, const bool wait_for_empty = false)
		{
			std::lock_guard<threading::spin_lock> _(m_first_lock);
			_consume_while_locked(_func);
			return _end_consumer(wait_for_empty);
		}

	public:
		// for producers; returns false if producers should stop

		bool push_back(T&& item)
		{
			std::lock_guard<threading::spin_lock> _(m_first_lock);
			m_items.push_back(std::forward<T>(item));
			return _notify_consumers();
		}
		bool push_back(const T& item)
		{
			std::lock_guard<threading::spin_lock> _(m_first_lock);
			m_items.push_back(item);
			return _notify_consumers();
		}

		~async_pipe()
		{
			std::lock_guard<threading::spin_lock> _(m_first_lock);
			THREADING_ASSERT(m_active_consumers == 0);
			THREADING_ASSERT(m_evict_count == 0);
			THREADING_ASSERT(m_sleeping_threads.awake_all(m_second_lock) == 0);
			THREADING_ASSERT(m_waiting_threads.awake_all(m_second_lock) == 0);
		}
	public: // others:

		//return strue when evicting
		bool wait_for_empty()
		{
			std::lock_guard<threading::spin_lock> _(m_first_lock);
			if ((m_items.size() > 0 || m_active_consumers > 0))
				m_waiting_threads.wait(m_first_lock, m_second_lock);
			return _check_evict();
		}
		bool empty()
		{
			std::lock_guard<threading::spin_lock> _(m_first_lock);
			if (m_items.size() == 0 && m_active_consumers == 0)
				return true;
			return false;
		}

	public:
		void evict(const std::size_t evict_count, const uint32_t sleep_interval_ms)
		{
			THREADING_ASSERT(evict_count < std::numeric_limits<int_fast16_t>::max());
			m_first_lock.lock();

			THREADING_ASSERT(m_evict_count == 0);

			m_evict_count = int_fast16_t(evict_count);
			
			while (true)
			{
				bool consumers = m_active_consumers > 0;
				bool sleepers = m_sleeping_threads.awake_all(m_second_lock) > 0;
				bool waiters = m_waiting_threads.awake_all(m_second_lock) > 0;

				if ((consumers || sleepers || waiters) && m_evict_count > 0)
				{
					m_first_lock.unlock();
					threading::utils::sleep_thread(sleep_interval_ms);
					m_first_lock.lock();
				}
				else
				{
					break;
				}
			}

			m_first_lock.unlock();
		}

	private:
		inline void _consume_one_begin()
		{
			m_items.pop_back();
			m_active_consumers++;
			m_first_lock.unlock();
		}
		inline void _consume_one_end()
		{
			m_first_lock.lock();
			m_active_consumers--;
		}
		inline bool _check_evict() const
		{
			return m_evict_count > 0;
		}
		inline bool _end_consumer(const bool wait_for_empty)
		{
			bool no_evict = (m_evict_count <= 0);
			if (no_evict == false)
			{
				m_evict_count--;
				while (m_evict_count > 0)
				{
					m_first_lock.unlock();
					std::this_thread::yield();
					m_first_lock.lock();
				}
			}
			if (no_evict && wait_for_empty)
			{
				if ((m_items.size() > 0 || m_active_consumers > 0))
					m_waiting_threads.wait(m_first_lock, m_second_lock);
			}
			return no_evict;
		}

		template <class F>
		inline void _consume_all_locked(const F& _func)
		{
			while (m_items.size() > 0 && m_evict_count == 0)
			{
				T out = std::move(m_items.back());
				_consume_one_begin();
				_func(std::move(out));
				_consume_one_end();

				if (m_items.size() == 0 && m_active_consumers == 0)
				{
					m_waiting_threads.awake_all(m_second_lock);
					break;
				}
			}
		}
		template <class F>
		inline void _consume_while_locked(const F& _func)
		{
			while (m_items.size() > 0 && m_evict_count == 0)
			{
				T out = std::move(m_items.back());
				_consume_one_begin();
				bool cond = _func(std::move(out));
				_consume_one_end();

				if (m_items.size() == 0 && m_active_consumers == 0)
				{
					m_waiting_threads.awake_all(m_second_lock);
					break;
				}
				if (cond)
					break;
			}
		}

		inline bool _wait_locked()
		{
			if (_check_evict())
				return false;
			m_sleeping_threads.wait(m_first_lock, m_second_lock);
			return true;
		}
		inline bool _notify_consumers()
		{
			if (_check_evict())
				return false;

			m_sleeping_threads.awake_one(m_second_lock);
			return true;
		}

	protected:
		threading::spin_lock m_first_lock;
		std::vector<T>		 m_items;
		int_fast16_t		 m_evict_count = 0;
		int_fast16_t		 m_active_consumers = 0;


		std::mutex	m_second_lock;
		locked_wait m_sleeping_threads;
		locked_wait m_waiting_threads;
	};

}
