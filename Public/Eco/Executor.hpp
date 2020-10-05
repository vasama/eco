#pragma once

#include "Eco/Function.hpp"
#include "Eco/Link.hpp"
#include "Eco/Private/Config.hpp"
#include "Eco/Private/Std20.hpp"

namespace Eco {
inline namespace Eco_NS {

struct Executable : DoubleLink<Executable>
{
	virtual void Execute() = 0;
};


class Executor
{
public:
	virtual void Schedule(Executable* object) = 0;
};


template<typename T>
class AnyExecutable final : Executable
{
	T m_value;

public:
	template<typename... TArgs>
	AnyExecutable(TArgs&&... args)
		requires(requires{ T(static_cast<TArgs&&>(args)...); })
		: m_value(static_cast<TArgs&&>(args)...)
	{
	}

	void Execute() override
	{
		m_value();
	}
};

template<typename T>
AnyExecutable(T) -> AnyExecutable<T>;

} // inline namespace Eco_NS
} // namespace Eco
