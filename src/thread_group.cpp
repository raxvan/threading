
#include "../incl/thread_group.h"

namespace threading
{

	thread_group::~thread_group()
	{
		for (auto& t : m_thread_handles)
			t.join();
	}
	void thread_group::swap(thread_group& other)
	{
		m_thread_handles.swap(other.m_thread_handles);
	}

}
