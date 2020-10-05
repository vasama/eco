#include "Eco/Private/Config.hpp"

namespace Eco {
inline namespace Eco_NS {
namespace _ {

template<size_t TIndex>
struct PriorityTag : PriorityTag<TIndex - 1> {};

template<>
struct PriorityTag<0> {};

} // namespace _
} // namespace Eco_NS
} // namespace Eco
