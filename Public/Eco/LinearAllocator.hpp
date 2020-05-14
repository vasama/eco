#include <cstddef>
#include <memory>

namespace Eco {
// inline namespace Eco_NS {

class LinearAllocator
{
	void* m_buffer;
	size_t m_space;

public:
	LinearAllocator(void* buffer, size_t size)
	{
		m_buffer = buffer;
		m_space = size;
	}

	void* Allocate(size_t size, size_t align = alignof(std::max_align_t))
	{
		return std::align(align, size, m_buffer, m_space);
	}
};

// } // inline namespace Eco_NS
} // namespace Eco
