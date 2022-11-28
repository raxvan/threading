
#pragma once
#include "thread_primitives.h"

namespace threading
{

	//--------------------------------------------------------------------------------------------------------------------------------
	class iThreadContext;
	class ThreadGroup
	{
	public:
		virtual ~ThreadGroup();

		virtual uint32_t size();
		// number of threads that joined this group (could not be accurate)

		virtual void start_one(const uint32_t index, iThreadContext* handler, threading::latch* init_barrier);
		virtual void join_one(iThreadContext* handler);

	public:
		inline void start_one(const uint32_t index, iThreadContext& handler, threading::latch* init_barrier)
		{
			start_one(index, &handler, init_barrier);
		}
		inline void join_one(iThreadContext& handler)
		{
			join_one(&handler);
		}

	public:
		template <class ITR>
		void start_wall(ITR _start, const ITR& _end, const uint32_t base_index = 0)
		{
			THREADING_ASSERT(std::distance(_start, _end) > 0);
			threading::latch ib { std::size_t(std::distance(_start, _end)) };
			uint32_t		 index = base_index;
			while (_start != _end)
				start_one(index++, *_start++, &ib);
		}
		template <class ITR>
		void join_all(ITR _start, const ITR& _end)
		{
			while (_start != _end)
				join_one(*_start++);
		}

	protected:
		/*
		virtual void run_handler_async(iThreadContext* handler, const uint32_t index);
		virtual void enter_native_thread(const uint32_t index);
		virtual void exit_native_thread(const uint32_t index);
		*/
	protected:
		std::atomic<uint32_t> m_joined_threads { 0 };
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	// for code that is to be executed on a thread
	class iThreadContext
	{
	public:
		iThreadContext() = default;
		iThreadContext(const iThreadContext&) = delete;
		iThreadContext& operator=(const iThreadContext&) = delete;

		iThreadContext(iThreadContext&&) noexcept;
		iThreadContext& operator=(iThreadContext&&) noexcept;

	public: // executed on thread:
		virtual void init(ThreadGroup* group, const uint32_t index) = 0;
		virtual void execute(ThreadGroup* group, const uint32_t index) = 0;

	public: // api for other threads
		virtual bool wait();

		virtual ~iThreadContext() = default;
	protected:
		void swap(iThreadContext& other);

	protected:
		friend class ThreadGroup;
		std::thread m_handle;
	};

	//--------------------------------------------------------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------------------------------------------------

}
