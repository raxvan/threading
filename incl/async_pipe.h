#pragma once

#include "locked_wait.h"

namespace threading
{
	extern void sleep_thread(const uint32_t ms_time);

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
		}

		template <class F>
		// void(T&&);
		// try to get, immediately return when no items are available
		// returns false when evicted
		bool consume_loop(const F& _func)
		{
			std::lock_guard<threading::spin_lock> _(m_first_lock);
			_consume_all_locked(_func);
			return m_evict == false;
		}
		template <class F>
		// bool(T&&)
		// try to get, immediately return when no items are available
		// returns false when evicted
		bool consume_while(const F& _func)
		{
			std::lock_guard<threading::spin_lock> _(m_first_lock);
			_consume_while_locked(_func);
			return m_evict == false;
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

	public: // others:
		bool wait_for_empty()
		{
			std::lock_guard<threading::spin_lock> _(m_first_lock);
			if ((m_items.size() > 0 || m_active_consumers > 0) && m_evict == false)
				m_waiting_threads.wait(m_first_lock, m_second_lock);
			return m_evict == false;
		}
		bool empty()
		{
			std::lock_guard<threading::spin_lock> _(m_first_lock);
			if (m_items.size() == 0 && m_active_consumers == 0)
				return true;
			if (m_evict)
				return true;
			return false;
		}

	public:
		void evict(const uint32_t sleep_interval_ms = 0)
		{
			m_first_lock.lock();
			m_evict = true;
			while (true)
			{
				bool consumers = m_active_consumers > 0;
				bool sleepers = m_sleeping_threads.awake_all(m_second_lock) > 0;
				bool waiters = m_waiting_threads.awake_all(m_second_lock) > 0;

				if (consumers || sleepers || waiters)
				{
					m_first_lock.unlock();
					threading::sleep_thread(sleep_interval_ms);
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
		template <class F>
		inline void _consume_all_locked(const F& _func)
		{
			while (m_items.size() > 0 && m_evict == false)
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
			while (m_items.size() > 0 && m_evict == false)
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
			if (m_evict)
				return false;
			m_sleeping_threads.wait(m_first_lock, m_second_lock);
			return true;
		}
		inline bool _notify_consumers()
		{
			if (m_evict)
				return false;

			m_sleeping_threads.awake_one(m_second_lock);
			return true;
		}

	protected:
		threading::spin_lock m_first_lock;
		std::vector<T>		 m_items;
		int_fast16_t		 m_active_consumers = 0;
		bool				 m_evict = false;

		std::mutex	m_second_lock;
		locked_wait m_sleeping_threads;
		locked_wait m_waiting_threads;
	};

}
