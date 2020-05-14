#include "Eco/List.hpp"

#include "Elements.test.hpp"

#include "catch2/catch.hpp"

using namespace Eco;

namespace {

using List = Eco::List<Element>;

TEST_CASE("List::Append", "[List][Container]")
{
	List list;
	Elements e;

	list.Append(e(1));
	CHECK(list.Size() == 1);
	CHECK(list.First()->value == 1);
	CHECK(list.Last()->value == 1);

	list.Append(e(2));
	CHECK(list.Size() == 2);
	CHECK(list.First()->value == 1);
	CHECK(list.Last()->value == 2);

	list.Append(e(3));
	CHECK(list.Size() == 3);
	CHECK(list.First()->value == 1);
	CHECK(list.Last()->value == 3);
}

TEST_CASE("List::Prepend", "[List][Container]")
{
	List list;
	Elements e;

	list.Prepend(e(1));
	CHECK(list.Size() == 1);
	CHECK(list.First()->value == 1);
	CHECK(list.Last()->value == 1);

	list.Prepend(e(2));
	CHECK(list.Size() == 2);
	CHECK(list.First()->value == 2);
	CHECK(list.Last()->value == 1);

	list.Prepend(e(3));
	CHECK(list.Size() == 3);
	CHECK(list.First()->value == 3);
	CHECK(list.Last()->value == 1);
}

TEST_CASE("List iteration.", "[List][Container]")
{
	List list;
	Elements e;

	for (int i = 0; i < 100; ++i)
		list.Append(e(i));

	auto beg = std::ranges::begin(list);
	auto const end = std::ranges::end(list);

	for (int i = 0; i < 100; ++i, ++beg)
	{
		REQUIRE(beg != end);
		CHECK(beg->value == i);
	}

	CHECK(beg == end);
}

TEST_CASE("List::Remove" "[List][Container]")
{
	List list;
	Elements e;

	Element* elements[5];
	for (int i = 0; i < 5; ++i)
		list.Append(elements[i] = e(i));

	int const remove = GENERATE(0, 2, 4);
	list.Remove(elements[remove]);

	int expected[4];
	for (int i = 0, j = 0; i < 5; ++i)
		if (i != remove)
			expected[j++] = i;

	CHECK(std::ranges::equal(expected, Values(list)));
}

TEST_CASE("List::Remove during iteration.", "[List][Container]")
{
	List list;
	Elements e;

	list.Append(e(1));
	list.Append(e(2));
	list.Append(e(3));

	auto beg = std::ranges::begin(list);
	auto const end = std::ranges::end(list);

	REQUIRE(beg != end);
	CHECK(beg->value == 1);
	{
		auto next = std::next(beg);
		REQUIRE(next != end);
		CHECK(next->value == 2);
		list.Remove(&*next);
	}
	REQUIRE(beg != end);
	CHECK(beg->value == 1);
	{
		auto next = std::next(beg);
		REQUIRE(next != end);
		CHECK(next->value == 3);
		list.Remove(&*next);
	}
	REQUIRE(beg != end);
	CHECK(beg->value == 1);

	CHECK(++beg == end);
}

} // namespace
