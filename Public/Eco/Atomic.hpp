#pragma once

#include <atomic>

namespace Eco {

template<typename T>
	requires std::atomic<T>::is_always_lock_free
using atomic = std::atomic<T>;

} // namespace Eco
