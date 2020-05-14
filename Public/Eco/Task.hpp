#pragma once

#include "Eco/Coro.hpp"
#include "Eco/Executor.hpp"
#include "Eco/Unit.hpp"

#include <atomic>

#include <assert.h>

namespace Eco {

template<typename T>
class Task;

template<typename T>
class TaskPromise;

template<typename T>
class TaskPromiseBase;

class TaskContext
{
	Executor* m_executor;
	void* m_buffer = nullptr;

	std::atomic<bool> m_debugIsInUse;

public:
	constexpr TaskContext(Executor& executor)
	{
		m_executor = &executor;
	}

	~TaskContext()
	{
#if Eco_CONFIG_DEBUG
		{
			bool isInUse = m_debugIsInUse.load(std::memory_order_relaxed);
			assert(!isInUse);
		}
#endif
	}

	TaskContext(TaskContext&&) = delete;
	TaskContext(const TaskContext&) = delete;
	
	TaskContext& operator=(TaskContext&&) = delete;
	TaskContext& operator=(const TaskContext&) = delete;

private:
	template<typename T>
	friend class TaskPromiseBase;
};

class TaskFinalAwaiter
{
public:
	bool await_ready() const
	{
		return false;
	}

	template<typename T>
	stdcoro::coroutine_handle<> await_suspend(
		stdcoro::coroutine_handle<TaskPromise<T>> continuation) const
	{
		Coro<TaskPromise<T>> coro(continuation);
		TaskPromise<T>& promise = continuation.promise();

		if (Executor* executor = promise.m_continuation.executor)
		{
			executor->Schedule(promise.m_continuation.object);
		}
		else if (void* coro = promise.m_continuation.coro)
		{
			return stdcoro::coroutine_handle<>::from_address(coro);
		}

		return stdcoro::noop_coroutine();
	}

	void await_resume() const
	{
	}

private:
	TaskFinalAwaiter() = default;

	template<typename>
	friend class TaskPromiseBase;
};

template<typename T>
class TaskCrossAwaiter;

template<typename T>
class TaskPromiseBase : public ExecutorObject
{
public:
	typedef LiftUnit<T> ResultType;

private:
	Executor* m_executor;

	struct {
		Executor* executor;
		union {
			void* coro;
			ExecutorObject* object;
		};
	} m_continuation;

	std::aligned_storage_t<
		sizeof(ResultType),
		alignof(ResultType)
	> m_result;

#if Eco_CONFIG_DEBUG
	TaskContext* m_debugContext;
#endif

public:
	~TaskPromiseBase()
	{
		if (m_executor == nullptr)
		{
			((ResultType*)&m_result)->~ResultType();
		}

		if (m_continuation.executor == nullptr)
		{
			if (void* coro = m_continuation.coro)
			{
				stdcoro::coroutine_handle<>::from_address(coro).destroy();
			}
		}

#if Eco_CONFIG_DEBUG
		m_debugContext->m_debugIsInUse.store(false, std::memory_order_release);
#endif
	}

	Task<T> get_return_object();

	stdcoro::suspend_always initial_suspend()
	{
		return {};
	}

	TaskFinalAwaiter final_suspend()
	{
		return {};
	}

	template<typename TAwaiter>
	TAwaiter&& await_transform(TAwaiter&& awaiter)
	{
		return static_cast<TAwaiter&&>(awaiter);
	}

	template<typename TIn>
	TaskCrossAwaiter<TIn> await_transform(Task<TIn> task)
	{
		TaskCrossAwaiter<TIn>(std::move(task.m_coro));
	}

	template<typename... TArgs>
	void* operator new(size_t size, TaskContext& context, TArgs&&...)
	{
#if Eco_CONFIG_DEBUG
		{
			bool isInUse = context.m_debugIsInUse.exchange(true, std::memory_order_acquire);
			assert(!isInUse);
		}
#endif

		if (context.m_buffer == nullptr)
		{
			context.m_buffer = malloc(size);
		}
		return context.m_buffer;
	}

	void operator delete(void*)
	{
	}

	void unhandled_exception() = delete;

private:
	template<typename... TArgs>
	TaskPromiseBase(TaskContext& context, TArgs&&...)
	{
		m_executor = context.m_executor;
		m_continuation.executor = nullptr;
		m_continuation.coro = nullptr;
		m_debugContext = &context;
	}

	template<typename TIn>
	void SetValue(TIn&& value)
	{
		assert(m_executor != nullptr);
		::new (&m_result) ResultType(static_cast<TIn&&>(value));
		m_executor = nullptr;
	}

	friend class Task<T>;

	friend class TaskPromise<T>;

	friend class TaskFinalAwaiter;

	template<typename>
	friend class TaskCrossAwaiter;
};

template<typename T>
class TaskPromise : public TaskPromiseBase<T>
{
public:
	template<typename... TArgs>
	TaskPromise(TaskContext& context, TArgs&&... args)
		: TaskPromiseBase<T>(context, static_cast<TArgs&&>(args)...)
	{
	}

	void return_value(T&& value)
	{
		this->SetValue(std::move(value));
	}

	void return_value(const T& value)
	{
		this->SetValue(value);
	}
};

template<>
class TaskPromise<void> : public TaskPromiseBase<void>
{
public:
	template<typename... TArgs>
	TaskPromise(TaskContext& context, TArgs&&... args)
		: TaskPromiseBase<void>(context, static_cast<TArgs&&>(args)...)
	{
	}

	void return_void()
	{
		this->SetValue(typename TaskPromiseBase<void>::ResultType{});
	}
};

template<typename T>
class Task
{
	Coro<TaskPromise<T>> m_coro;

public:
	typedef TaskPromise<T> promise_type;

	void Start() &&
	{
		TaskPromise<T>& promise = *m_coro;
		m_coro.Release();

		//TODO: executor should also be able to destroy the coroutine?
		ExecutorObject::Set(promise, [](TaskPromise<T>& promise) {
			stdcoro::coroutine_handle<TaskPromise<T>>::from_promise(promise).resume();
		});

		promise.m_executor->Schedule(&promise);
	}

	template<typename TCallback>
	void Start(TCallback&& callback) &&
	{
		m_coro->m_callback = static_cast<TCallback&&>(callback);
		Start();
	}

	void StartInlineUnsafe() &&
	{
		m_coro.Release().resume();
	}

	template<typename TCallback>
	void StartInlineUnsafe(TCallback&& callback) &&
	{
		m_coro->m_callback = static_cast<TCallback&&>(callback);
		StartInlineUnsafe();
	}

private:
	Task(TaskPromise<T>& promise)
		: m_coro(stdcoro::coroutine_handle<TaskPromise<T>>::from_promise(promise))
	{
	}

	friend class TaskPromiseBase<T>;
};

template<typename T>
class TaskCrossAwaiter
{
	Coro<TaskPromise<T>> m_coro;

public:
	bool await_ready() const
	{
		return false;
	}

	template<typename TIn>
	stdcoro::coroutine_handle<> await_suspend(
		stdcoro::coroutine_handle<TaskPromise<TIn>> continuation)
	{
		TaskPromise<TIn>& promise = continuation.promise();
		Executor* executor = promise.m_executor;

		if (m_coro->m_executor == executor)
		{
			m_coro->m_continuation.coro = continuation.address();
			return m_coro.Release();
		}
		else
		{
			ExecutorObject::Set(promise, [](TaskPromise<T>& promise) {
				stdcoro::coroutine_handle<TaskPromise<T>>::from_promise(promise).resume();
			});

			m_coro->m_continuation.executor = executor;
			m_coro->m_continuation.object = &promise;
		}

		return stdcoro::noop_coroutine();
	}

	typename TaskPromise<T>::ResultType await_resume()
	{
		typedef typename TaskPromise<T>::ResultType ResultType;
		return std::move(*((ResultType*)&m_coro->m_result));
	}

private:
	TaskCrossAwaiter(Coro<TaskPromise<T>>&& coro)
		: m_coro(std::move(coro))
	{
	}

	template<typename>
	friend class TaskPromiseBase;
};

template<typename T>
Task<T> TaskPromiseBase<T>::get_return_object()
{
	return Task<T>(static_cast<TaskPromise<T>&>(*this));
}

} // namespace Eco
