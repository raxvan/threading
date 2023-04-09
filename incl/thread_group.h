
#pragma once
#include "thread_primitives.h"

namespace threading
{

	struct thread_group
	{
	public:
		thread_group() = default;
		~thread_group();
		void swap(thread_group& other);

	public:

		template <class F>
		inline void spawn(const std::size_t count, const F& _func)
		{
			std::size_t sz = m_thread_handles.size();
			m_thread_handles.resize(sz + count);
			for (std::size_t i = 0; i < count; i++)
			{
				threading::utils::start_native(m_thread_handles[sz + i], _func);
			}
		}

		inline std::size_t size() const
		{
			return m_thread_handles.size();
		}

	protected:
		std::vector<std::thread> m_thread_handles;
	};

}
