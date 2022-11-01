

#pragma once

#include "thread_primitives.h"
#include "native_threads.h"
#include "async_pipe.h"
#include "latch_pool.h"

namespace threading
{

	//--------------------------------------------------------------------------------------------------------------------------------

	extern void lock_current_thread_to_core(const std::size_t core_index);
	extern void sleep_thread(const uint32_t ms_time);

	//--------------------------------------------------------------------------------------------------------------------------------

	template <class T>
	struct atomic_ptr_iterator
	{
	public:
		atomic_ptr_iterator() = default;
		atomic_ptr_iterator(T* _begin, const std::size_t _size)
			: m_size(_size)
			, m_begin(_begin)
		{
		}

		inline void reset(T* _begin, const std::size_t _size)
		{
			m_iterator = 0;
			m_size = _size;
			m_begin = _begin;
		}
		inline std::size_t size() const
		{
			return m_iterator.load();
		}

	public:
		T* get()
		{
			std::size_t index = m_iterator++;
			if (index < m_size)
				return m_begin + index;
			return nullptr;
		}
		T* getx2()
		{
			std::size_t index = m_iterator++;
			index -= m_size;
			if (index < m_size)
				return m_begin + index;
			return nullptr;
		}

	protected:
		std::atomic<std::size_t> m_iterator = 0;
		std::size_t				 m_size = 0;
		T*						 m_begin = nullptr;
	};

}