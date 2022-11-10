#pragma once

#include "thread_primitives.h"

namespace threading
{
	extern void sleep_thread(const uint32_t ms_time);

	// for multiple producers/multiple consumers of data T
	template <class T>
	struct async_pipe
	{
	public:
		using consume_func_t = void(T&&);

	public:
		// for consumers:
		// consume func type: void(T&&)

		template <class F>
		void consume_or_wait(const bool exit_when_empty, const F& _func)
		{
			std::lock_guard<threading::spin_lock> _(m_main_lock);

			do
			{
				if (m_items.size() > 0)
				{
					_consume_locked(_func);

					// m_active_count == 0 is true when:
					// no items are available
					// no items are currently consumed
					if (exit_when_empty && m_active_count == 0)
					{
						// notify everyone and exit
						std::unique_lock<std::mutex> lk(m_sleep_mutex);
						if (m_sleep_count > 0)
						{
							m_sleep_count = -m_sleep_count;
							m_sleep_trigger.notify_all();
						}
						break;
					}
				}
			} while (_wait_locked());
		}

		template <class F>
		// void(T&&)
		void consume(const F& _func) // try to get, immediately return when no items are available
		{
			std::lock_guard<threading::spin_lock> _(m_main_lock);
			if (m_items.size() > 0)
				_consume_locked(_func);
		}

	public:
		// for producers

		bool push_back(T&& item)
		{
			std::lock_guard<threading::spin_lock> _(m_main_lock);
			m_items.push_back(std::forward<T>(item));
			return _on_push_back_locked();
		}
		bool push_back(const T& item)
		{
			std::lock_guard<threading::spin_lock> _(m_main_lock);
			m_items.push_back(item);
			return _on_push_back_locked();
		}

		// returns true when evicted by a call to terminate()
		bool wait_empty()
		{
			std::lock_guard<threading::spin_lock> _(m_main_lock);
			while (m_items.size() > 0 || m_active_count > 0)
			{
				if (_wait_locked())
				{
					if (m_items.size() > 0 && m_sleep_count > 0)
					{
						std::unique_lock<std::mutex> lk(m_sleep_mutex);
						m_sleep_trigger.notify_one();
					}
				}
				else
					return true;
			}
			return false;
		}

		// removes all elements and evicts all waiting threads
		void clear(const uint32_t sleep_interval_ms = 10)
		{
			std::lock_guard<threading::spin_lock> _(m_main_lock);
			{
				std::unique_lock<std::mutex> lk(m_sleep_mutex);
				if (m_sleep_count > 0)
				{
					m_sleep_count = -m_sleep_count - 1;
					m_sleep_trigger.notify_all();
				}
				else if (m_active_count == 0)
					m_sleep_count = -1;
			}

			while (m_active_count > 0)
			{
				// wait for all consumers to exit
				m_main_lock.unlock();

				threading::sleep_thread(sleep_interval_ms);

				m_main_lock.lock();
			}

			{
				std::unique_lock<std::mutex> lk(m_sleep_mutex);
				m_sleep_count++;
			}
		}

	private:
		template <class F>
		decl_force_inline void _consume_locked(const F& _func)
		{
			do
			{
				T out = std::move(m_items.back());
				m_items.pop_back();
				m_active_count++;
				m_main_lock.unlock();

				_func(std::move(out));

				m_main_lock.lock();
				m_active_count--;
			} while (m_items.size() > 0);
		}
		decl_force_inline bool _wait_locked()
		{
			bool stay = true;

			{
				std::unique_lock<std::mutex> lk(m_sleep_mutex);
				if (m_sleep_count >= 0)
				{
					m_sleep_count++;
					m_main_lock.unlock();
					m_sleep_trigger.wait(lk);
					if (m_sleep_count > 0)
						m_sleep_count--;
					else
					{
						m_sleep_count++;
						stay = false;
					}
				}
				else
				{
					return false;
				}
			}
			m_main_lock.lock();
			return stay;
		}

		bool _on_push_back_locked()
		{
			if (m_sleep_count != 0)
			{
				std::unique_lock<std::mutex> lk(m_sleep_mutex);
				if (m_sleep_count > 0)
					m_sleep_trigger.notify_one();
				else if (m_sleep_count < 0)
				{
					m_items.clear();
					return false;
				}
			}
			return true;
		}

	protected:
		threading::spin_lock m_main_lock;
		std::vector<T>		 m_items;
		uint16_t			 m_active_count = 0;
		int16_t				 m_sleep_count = 0;

		std::mutex				m_sleep_mutex;
		std::condition_variable m_sleep_trigger;
	};

}
