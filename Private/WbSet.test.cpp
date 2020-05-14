#include "Eco/WbSet.hpp"

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

using Set = WbSet<Element, KeySelector>;

struct TwoSets
{
	std::list<Element> list;
	std::set<int> std;
	WbSet<Element, KeySelector> eco;

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


TEST_CASE("WbSet::Insert", "[WbSet][Container]")
{
	Elements e;

	Set set;

	SECTION("123")
	{
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

	SECTION("Pathological case 1")
	{
		REQUIRE(set.Insert(e(9)).Inserted);
		REQUIRE(set.Insert(e(7)).Inserted);
		REQUIRE(set.Insert(e(5)).Inserted);
		REQUIRE(set.Insert(e(8)).Inserted);
		REQUIRE(set.Insert(e(6)).Inserted);
		REQUIRE(set.Insert(e(2)).Inserted);
		REQUIRE(set.Insert(e(4)).Inserted);
		REQUIRE(set.Insert(e(1)).Inserted);
		REQUIRE(set.Insert(e(3)).Inserted);

		REQUIRE(set.Size() == 9);
	}

	SECTION("Pathological case 2")
	{
		REQUIRE(set.Insert(e(3)).Inserted);
		REQUIRE(set.Insert(e(2)).Inserted);
		REQUIRE(set.Insert(e(7)).Inserted);
		REQUIRE(set.Insert(e(1)).Inserted);
		REQUIRE(set.Insert(e(4)).Inserted);
		REQUIRE(set.Insert(e(9)).Inserted);
		REQUIRE(set.Insert(e(6)).Inserted);
		REQUIRE(set.Insert(e(8)).Inserted);
		REQUIRE(set.Insert(e(11)).Inserted);
		REQUIRE(set.Insert(e(10)).Inserted);
		REQUIRE(set.Insert(e(5)).Inserted);

		REQUIRE(set.Size() == 11);
	}
}

TEST_CASE("WbSet::Remove", "[WbSet][Container]")
{
	TwoSets sets;

	SECTION("One element tree")
	{
		sets.Insert(0);
		REQUIRE(sets.Remove(0));
		REQUIRE(sets.Equal());
	}

	SECTION("Perfectly balanced, height 5")
	{
		auto const MaybeRemove = [&](int const value) -> bool
		{
			if (GENERATE(false, true))
			{
				sets.Remove(value);
				return true;
			}
			return false;
		};

		/*
		 *                     8
		 *                    / \
		 *                   /   \
		 *                  /     \
		 *                 /       \
		 *                /         \
		 *               /           \
		 *              /             \
		 *             /               \
		 *            /                 \
		 *           4                  12
		 *          / \                 / \
		 *         /   \               /   \
		 *        /     \             /     \
		 *       /       \           /       \
		 *      2         6         10       14
		 *     / \       / \       / \       / \
		 *    /   \     /   \     /   \     /   \
		 *   1     3   5     7   9    11   13   15
		 */

		sets.Insert(80);
		sets.Insert(40);
		sets.Insert(120);
		sets.Insert(20);
		sets.Insert(60);
		sets.Insert(100);
		sets.Insert(140);
		sets.Insert(10);
		sets.Insert(30);
		sets.Insert(50);
		sets.Insert(70);
		sets.Insert(90);
		sets.Insert(110);
		sets.Insert(130);
		sets.Insert(150);

		int const base = GENERATE(0, 80);

		SECTION("Remove a leaf")
		{
			REQUIRE(sets.Remove(base + GENERATE(1, 3, 5, 7) * 10));
			REQUIRE(sets.Equal());
		}

		SECTION("Remove an upper branch")
		{
			int const branch = base + GENERATE(2, 6) * 10;

			// Try with 0, 1 and 2 children removed.
			MaybeRemove(branch - 10);
			MaybeRemove(branch + 10);

			REQUIRE(sets.Remove(branch));
			REQUIRE(sets.Equal());
		}

		SECTION("Remove a lower branch")
		{
			auto const MaybeRemoveUpperBranch = [&](int const value) -> bool
			{
				if (MaybeRemove(value - 10) && MaybeRemove(value + 10) && GENERATE(false, true))
				{
					sets.Remove(value);
					return true;
				}
				return false;
			};

			int const branch = base + 40;

			// Try with [0, 6] children removed.
			MaybeRemoveUpperBranch(branch - 20);
			MaybeRemoveUpperBranch(branch + 20);

			REQUIRE(sets.Remove(branch));
			REQUIRE(sets.Equal());
		}

		SECTION("Remove the root")
		{
			REQUIRE(sets.Remove(80));
			REQUIRE(sets.Equal());
		}
	}
}

TEST_CASE("WbSet mass test.", "[WbSet][Container]")
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

TEST_CASE("WbSet::Clear", "[WbSet][Container]")
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

TEST_CASE("WbSet iteration.", "[WbSet][Container]")
{
	Elements e;

	Set set;

	for (int i : std::views::iota(1, 100) | std::views::reverse) set.Insert(e(i));
	REQUIRE(std::ranges::equal(std::views::iota(1, 100), Values(set)));
}

} // namespace
