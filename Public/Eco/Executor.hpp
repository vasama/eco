#pragma once

#include "Eco/Config.hpp"
#include "Eco/ForwardList.hpp"
#include "Eco/Function.hpp"

namespace Eco {

//TODO: Turn into virtual interface?
class ExecutorObject : public ForwardListObject<ExecutorObject>
{
	void(*m_func)(ExecutorObject& object) = nullptr;

public:
	template<typename T, typename TLambda>
	static void Set(T& object, TLambda)
		requires std::is_base_of_v<ExecutorObject, T> && std::is_empty_v<TLambda>
	{
		ExecutorObject::Set(object, &LambdaCallback<T, TLambda>);
	}

	template<typename T>
	static void Set(T& object, void(*func)(T&))
		requires std::is_base_of_v<ExecutorObject, T>
	{
		static_cast<ExecutorObject&>(object).m_func = (void(*)(ExecutorObject&))func;
	}

private:
	template<typename T, typename TLambda>
	static void LambdaCallback(T& object)
	{
		Eco_CLANG_DIAG(push)
		Eco_CLANG_DIAG(ignored "-Wuninitialized")
		constexpr TLambda lambda = lambda;
		Eco_CLANG_DIAG(pop)

		lambda(object);
	}

	friend class Executor;
};

class Executor
{
public:
	Executor() = default;

	Executor(Executor&&) = delete;
	Executor(const Executor&) = delete;
	
	Executor& operator=(Executor&&) = delete;
	Executor& operator=(const Executor&) = delete;

	virtual void Schedule(ExecutorObject* object) = 0;

protected:
	static void Execute(ExecutorObject* object)
	{
		object->m_func(*object);
	}
};

} // namespace Eco
