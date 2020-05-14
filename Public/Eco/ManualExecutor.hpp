#pragma once

#include "Eco/Config.hpp"
#include "Eco/Executor.hpp"

#include "Eco/MpscListQueue.hpp"

namespace Eco {

class ManualExecutor final : public Executor
{
	MpscListQueue<ExecutorObject> m_queue;

public:
	virtual void Schedule(ExecutorObject* object) override
	{
		m_queue.Enqueue(object);
	}

	size_t Execute()
	{
		size_t count = 0;
		for (ExecutorObject* object; (object = m_queue.TryDequeue()) != nullptr; ++count)
		{
			Executor::Execute(object);
		}
		return count;
	}
};

} // namespace Eco

