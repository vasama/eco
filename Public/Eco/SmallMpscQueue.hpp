#pragma once

#include "Eco/Std20.hpp"

#include <atomic>
#include <optional>

#include <assert.h>
#include <stdint.h>

namespace Eco {

template<typename T>
class SmallMpscQueue
{
	T* m_buffer;
	uint8_t m_size;
	std::atomic<uint8_t> m_index = 0;
	std::atomic<uint32_t> m_taken = 0;
	std::atomic<uint32_t> m_ready = 0;

public:
	static constexpr size_t MaxSize = 32;

	SmallMpscQueue(T* buffer, size_t size)
	{
		assert(size <= MaxSize);
		m_buffer = buffer;
		m_size = (uint8_t)size;
	}

	void Enqueue(std::convertible_to<T> auto&& value);

	std::optional<T> TryDequeue();

private:
};

} // namespace Eco
