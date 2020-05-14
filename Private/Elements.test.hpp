#pragma once

#include "Eco/Link.hpp"

#include <iterator>
#include <list>
#include <ranges>
#include <unordered_map>

namespace Eco {

struct Element : Link<4>
{
	int value;

	Element(int const value)
		: value(value)
	{
	}
};

struct Elements
{
	std::list<Element> list;

	Element* operator()(int const value)
	{
		return &list.emplace_back(value);
	}
};

struct UniqueElements
{
	std::unordered_map<int, Element> map;

	Element* operator()(int const value)
	{
		return &map.try_emplace(value, value).first->second;
	}
};

template<typename TRange>
auto Values(const TRange& range)
	requires std::is_same_v<std::ranges::range_value_t<TRange>, Element>
{
	return std::views::transform(range,
		[](const auto& x) -> const auto& { return x.value; });
}

} // namespace Eco
