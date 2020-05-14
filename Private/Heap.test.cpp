#include "Eco/Heap.hpp"

#include "Elements.test.hpp"

#include "catch2/catch.hpp"

#include <random>

using namespace Eco;

namespace {

struct KeySelector
{
	int operator()(const Element& node) const
	{
		return node.value;
	}
};

struct TwoHeaps
{
	std::vector<int> std;
	Heap<Element, KeySelector> eco;
	Elements elements;

	bool IsEmpty() const
	{
		bool const empty = eco.IsEmpty();
		REQUIRE(empty == std.empty());
		return empty;
	}

	void Push(int const value)
	{
		eco.Push(elements(value));

		std.push_back(value);
		std::ranges::push_heap(std);
	}

	int Pop()
	{
		int const eValue = eco.Pop()->value;

		std::ranges::pop_heap(std);
		int const sValue = std.back();
		std.pop_back();

		REQUIRE(eValue == sValue);

		return eValue;
	}

	bool Remove(int const value)
	{
		auto const sIt = std::ranges::find(std, value);
		auto const eIt = std::find_if(
			elements.list.begin(), elements.list.end(),
			[&](const auto& x) { return x.value == value; });

		bool const sFound = sIt != std.end();
		bool const eFound = eIt != elements.list.end();

		REQUIRE(sFound == eFound);

		if (eFound)
		{
			std.erase(sIt);
			std::ranges::make_heap(std);
			eco.Remove(&*eIt);
		}

		return eFound;
	}
};

TEST_CASE("Heap::Push", "[Heap][Container]")
{
	TwoHeaps heaps;

	heaps.Push(2);
	heaps.Push(1);
	heaps.Push(3);

	CHECK(heaps.Pop() == 3);
	CHECK(heaps.Pop() == 2);
	CHECK(heaps.Pop() == 1);

	CHECK(heaps.IsEmpty());
}

TEST_CASE("Heap::Remove", "[Heap][Container]")
{
	TwoHeaps heaps;

	for (int i = 7; i > 0; --i)
		heaps.Push(i);

	int const remove = GENERATE(1, 4, 7);
	CHECK(heaps.Remove(remove));

	for (int i = 7; i > 0; --i)
		if (i != remove)
			CHECK(i == heaps.Pop());

	CHECK(heaps.IsEmpty());
}

TEST_CASE("Heap mass test.", "[Heap][Container]")
{
	std::vector<int> array;
	Heap<Element, KeySelector> heap;
	Elements elements;

	auto&& rng = Catch::rng();
	auto distribution = std::uniform_int_distribution();

	for (size_t i = 0; i < 10000; ++i)
	{
		int const value = distribution(rng);

		array.push_back(value);
		heap.Push(elements(value));
	}

	std::ranges::sort(array, std::greater{});

	for (int const value : array)
	{
		REQUIRE(!heap.IsEmpty());
		CHECK(value == heap.Pop()->value);
	}

	CHECK(heap.IsEmpty());
}

} // namespace
