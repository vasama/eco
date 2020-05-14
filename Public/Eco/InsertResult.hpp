#pragma once

namespace Eco {

template<typename T>
struct InsertResult
{
	/// @brief Matching element in the container.
	T* Element;

	/// @brief True if a new element was inserted.
	bool Inserted;
};

} // namespace Eco
