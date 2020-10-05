#pragma once

#include "Eco/Assert.hpp"
#include "Eco/Coroutine.hpp"
#include "Eco/Executor.hpp"
#include "Eco/Private/Config.hpp"
#include "Eco/Unit.hpp"

#include <atomic>

namespace Eco {
inline namespace Eco_NS {

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

#if Eco_CONFIG_ASSERT
	std::atomic<bool> m_isInUse;
#endif

public:
	constexpr TaskContext(Executor& executor)
		: m_executor(&executor)
	{
	}

#if Eco_CONFIG_ASSERT
	~TaskContext()
	{
		Eco_Assert(!m_isInUse.load(std::memory_order_relaxed));
	}
#endif

	TaskContext(const TaskContext&) = delete;
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
#ifdef __clang__
	__attribute__((noinline))
#endif
	std20::coroutine_handle<> await_suspend(
		std20::coroutine_handle<TaskPromise<T>> continuation) const
	{
		Coro<TaskPromise<T>> coro(continuation);
		TaskPromise<T>& promise = continuation.promise();

		if (Executor* executor = promise.m_continuation.executor)
		{
			executor->Schedule(promise.m_continuation.executable);
		}
		else if (void* coro = promise.m_continuation.coro)
		{
			return std20::coroutine_handle<>::from_address(coro);
		}

		return std20::noop_coroutine();
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
class TaskPromiseBase : public Executable
{
public:
	typedef LiftUnit<T> ResultType;

private:
	Executor* m_executor;

	struct {
		Executor* executor;
		union {
			void* coro;
			Executable* executable;
		};
	} m_continuation;

	std::aligned_storage_t<
		sizeof(ResultType),
		alignof(ResultType)
	> m_result;

#if Eco_CONFIG_ASSERT
	TaskContext* m_context;
#endif

public:
	~TaskPromiseBase()
	{
		if (m_executor == nullptr)
		{
			reinterpret_cast<ResultType&>(m_result).~ResultType();
		}

		if (m_continuation.executor == nullptr)
		{
			if (void* coro = m_continuation.coro)
			{
				std20::coroutine_handle<>::from_address(coro).destroy();
			}
		}

#if Eco_CONFIG_ASSERT
		m_context->m_isInUse.store(false, std::memory_order_release);
#endif
	}

	Task<T> get_return_object();

	std20::suspend_always initial_suspend()
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
#if Eco_CONFIG_ASSERT
		Eco_Verify(!context.m_isInUse.exchange(true, std::memory_order_acquire));
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

#if Eco_CONFIG_ASSERT
		m_context = &context;
#endif
	}

	template<typename TIn>
	void SetValue(TIn&& value)
	{
		Eco_Assert(m_executor != nullptr);
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
class TaskPromise final : public TaskPromiseBase<T>
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

	void Execute() override
	{
		std20::coroutine_handle<TaskPromise<T>>::from_promise(*this).resume();
	}
};

template<>
class TaskPromise<void> final : public TaskPromiseBase<void>
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

	void Execute() override
	{
		std20::coroutine_handle<TaskPromise<void>>::from_promise(*this).resume();
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
		: m_coro(std20::coroutine_handle<TaskPromise<T>>::from_promise(promise))
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
	std20::coroutine_handle<> await_suspend(
		std20::coroutine_handle<TaskPromise<TIn>> continuation)
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
			m_coro->m_continuation.executor = executor;
			m_coro->m_continuation.executable = &promise;
		}

		return std20::noop_coroutine();
	}

	typename TaskPromise<T>::ResultType await_resume()
	{
		typedef typename TaskPromise<T>::ResultType ResultType;
		return reinterpret_cast<ResultType&&>(m_coro->m_Result);
	}

private:
	TaskCrossAwaiter(Coro<TaskPromise<T>>&& coro)
		: m_coro(static_cast<Coro<TaskPromise<T>>&&>(coro))
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

} // inline namespace Eco_NS
} // namespace Eco
