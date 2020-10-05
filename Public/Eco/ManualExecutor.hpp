#pragma once

#include "Eco/Executor.hpp"
#include "Eco/MpscQueue.hpp"
#include "Eco/Private/Config.hpp"

namespace Eco {
inline namespace Eco_NS {

class ManualExecutor final : public Executor
{
	MpscQueue<Executable> m_queue;

public:
	virtual void Schedule(Executable* object) override
	{
		m_queue.Enqueue(object);
	}

	size_t Execute()
	{
		size_t count = 0;
		for (Executable* executable; (executable = m_queue.TryDequeue()) != nullptr; ++count)
		{
			executable->Execute();
		}
		return count;
	}
};

} // inline namespace Eco_NS
} // namespace Eco
