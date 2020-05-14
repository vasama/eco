#include "Eco/AvlSet.hpp"

#include "Elements.test.hpp"

#include "catch2/catch.hpp"

#include <set>

using namespace Eco;

namespace {

struct KeySelector
{
	int operator()(const Element& node) const
	{
		return node.value;
	}
};

using Set = AvlSet<Element, KeySelector>;

struct TwoSets
{
	std::list<Element> list;
	std::set<int> std;
	AvlSet<Element, KeySelector> eco;

	auto Insert(int const value)
	{
		auto const it = list.insert(list.end(), Element(value));

		auto const rEco = eco.Insert(&*it);
		auto const rStd = std.insert(value);
		REQUIRE(rEco.Inserted == rStd.second);

		if (!rEco.Inserted)
		{
			list.erase(it);
		}

		return rEco;
	}

	bool Remove(int const value)
	{
		Element* const element = eco.Find(value);
		auto const it = std.find(value);

		REQUIRE((element != nullptr) == (it != std.end()));
		if (element == nullptr) return false;

		eco.Remove(element);
		std.erase(it);

		return true;
	}

	auto Values() const
	{
		return Eco::Values(eco);
	}

	bool Equal() const
	{
		return std::ranges::equal(std, Values());
	}
};


TEST_CASE("AvlSet::Insert", "[AvlSet][Container]")
{
	Elements e;

	Set set;

	REQUIRE(set.Insert(e(1)).Inserted);
	REQUIRE(set.Insert(e(2)).Inserted);
	REQUIRE(set.Insert(e(3)).Inserted);

	REQUIRE(!set.Insert(e(1)).Inserted);
	REQUIRE(!set.Insert(e(2)).Inserted);
	REQUIRE(!set.Insert(e(3)).Inserted);

	REQUIRE(set.Insert(e(-1)).Inserted);
	REQUIRE(set.Insert(e(-2)).Inserted);
	REQUIRE(set.Insert(e(-3)).Inserted);

	REQUIRE(!set.Insert(e(-1)).Inserted);
	REQUIRE(!set.Insert(e(-2)).Inserted);
	REQUIRE(!set.Insert(e(-3)).Inserted);

	REQUIRE(set.Size() == 6);
}

TEST_CASE("AvlSet::Remove", "[AvlSet][Container]")
{
	TwoSets sets;

	SECTION("One element tree")
	{
		sets.Insert(0);
		REQUIRE(sets.Remove(0));
		REQUIRE(sets.Equal());
	}

	SECTION("Perfectly balanced, height 3-4")
	{
		/*           4
		 *          / \
		 *         /   \
		 *        /     \
		 *       /       \
		 *      2         6
		 *     / \       / \
		 *    /   \     /   \
		 *   1     3   5     7
		 */

		sets.Insert(40);
		sets.Insert(20);
		sets.Insert(60);
		sets.Insert(10);
		sets.Insert(30);
		sets.Insert(50);
		sets.Insert(70);

		SECTION("Remove a leaf")
		{
			REQUIRE(sets.Remove(GENERATE(1, 3, 5, 7) * 10));
			REQUIRE(sets.Equal());
		}

		SECTION("Remove a branch")
		{
			int const branch = GENERATE(2, 6) * 10;

			// Try with 0, 1 and 2 children removed.
			if (GENERATE(false, true)) sets.Remove(branch - 10);
			if (GENERATE(false, true)) sets.Remove(branch + 10);

			REQUIRE(sets.Remove(branch));
			REQUIRE(sets.Equal());
		}

		SECTION("Remove the root")
		{
			int branch1 = 20;
			int branch2 = 60;

			if (GENERATE(false, true)) std::swap(branch1, branch2);

			if (GENERATE(false, true))
			{
				// Try with 0, 1, and 2 children added to the leaves of the branch.
				if (GENERATE(false, true)) sets.Insert(branch1 - 15);
				if (GENERATE(false, true)) sets.Insert(branch1 - 5);
				if (GENERATE(false, true)) sets.Insert(branch1 + 5);
				if (GENERATE(false, true)) sets.Insert(branch1 + 15);
			}
			else
			{
				// Try with 0, 1 and 2 children removed from branch1.
				if (GENERATE(false, true)) sets.Remove(branch1 - 10);
				if (GENERATE(false, true)) sets.Remove(branch1 + 10);
			}

			// Try with one of the children removed from branch2.
			if (GENERATE(false, true)) sets.Remove(branch2 + GENERATE(-1, +1) * 10);

			REQUIRE(sets.Remove(40));
			REQUIRE(sets.Equal());
		}
	}
}

TEST_CASE("AvlSet::Clear", "[AvlSet][Container]")
{
	UniqueElements e;

	Set set;

	// Insert the same elements twice.

	for (int i = 0; i < 10; ++i)
		set.Insert(e(i));
	set.Clear();
	REQUIRE(set.IsEmpty());

	for (int i = 0; i < 10; ++i)
		set.Insert(e(i));
	set.Clear();
	REQUIRE(set.IsEmpty());
}

TEST_CASE("AvlSet iteration.", "[AvlSet][Container]")
{
	Elements e;

	Set set;

	for (int i : std::views::iota(1, 100) | std::views::reverse) set.Insert(e(i));
	REQUIRE(std::ranges::equal(std::views::iota(1, 100), Values(set)));
}

TEST_CASE("AvlSet mass test.", "[AvlSet][Container]")
{
	Elements e;

	auto rng = Catch::rng();
	std::uniform_int_distribution distribution = {};

	Set set;
	std::set<int> stdSet;

	for (size_t i = 0; i < 10000; ++i)
	{
		int const value = distribution(rng);
		bool const inserted = stdSet.insert(value).second;
		REQUIRE(set.Insert(e(value)).Inserted == inserted);
	}

	REQUIRE(std::ranges::equal(stdSet, Values(set)));
}

} // namespace
