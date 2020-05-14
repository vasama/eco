#include "Eco/WbSet.hpp"

#include <array>
#include <utility>

using namespace Eco;
using namespace Private::WbSet_;

static_assert(sizeof(Hook) == sizeof(WbSetLink));
static_assert(CheckCompleteTaggedPointer<Ptr<Hook>>());


static constexpr uintptr_t Delta = 4;
static constexpr uintptr_t Ratio = 2;


static size_t Weight(Hook* const root)
{
	return root != nullptr ? root->weight : 0;
}

static Hook* Leftmost(Hook* hook, bool const l)
{
	while (hook->children[l] != nullptr)
	{
		hook = hook->children[l];
	}
	return hook;
}

// Rotate from left to right.
static void Rotate(Hook* const root, bool const l)
{
	bool const r = !l;

	Hook** const parent = root->parent;
	Hook* const pivot = root->children[l];
	Hook* const child = pivot->children[r];

	uintptr_t const rootWeight = root->weight;
	uintptr_t const pivotWeight = pivot->weight;
	uintptr_t const childWeight = Weight(child);

	root->children[l] = child;
	root->parent = pivot->children;

	pivot->children[r] = root;
	pivot->parent = parent;

	if (child != nullptr) child->parent = root->children;
	parent[root != parent[0]] = pivot;

	root->weight += childWeight - pivotWeight;
	pivot->weight += rootWeight - pivotWeight;
}

static void Rebalance(Hook** const root, Hook** node, bool l, bool const insert)
{
	uintptr_t const weightIncrement = insert ? +1 : -1;

	while (node != root)
	{
		l ^= !insert;
		bool const r = !l;

		Hook* const parent = Eco_WB_HOOK_FROM_CHILDREN(node);
		parent->weight += weightIncrement;

		Hook* newParent = parent;

		Hook* const lChild = parent->children[l];
		Hook* const rChild = parent->children[r];

		if (parent->weight > 2 && Weight(lChild) >= Weight(rChild) * Delta)
		{
			newParent = lChild;

			Hook* const llChild = lChild->children[l];
			Hook* const lrChild = lChild->children[r];

			if (Weight(lrChild) >= Weight(llChild) * Ratio)
			{
				Rotate(lChild, r);

				// Inner rotation pivot becomes the new subtree root.
				newParent = lrChild;
			}

			Rotate(parent, l);
		}

		node = newParent->parent;
		l = *node != newParent;
	}
}

static bool Invariant(const Core* const self)
{
	if (const Hook* hook = self->m_root.Value)
	{
		struct Frame
		{
			uint32_t visit : 2;
			uint32_t lWeight : 30;
		};

		Frame stack[sizeof(size_t) * CHAR_BIT + 1];

		int32_t height = 0;
		stack[0].visit = 0;
		uint32_t rWeight = 0;

		while (height >= 0)
		{
		next_iteration:
			Frame& frame = stack[height];

			for (uint32_t visit; (visit = frame.visit++) < 2;)
			{
				frame.lWeight = std::exchange(rWeight, 0);

				if (const Hook* const child = hook->children[visit])
				{
					if (child->parent != hook->children)
						return false;

					hook = child;
					stack[++height].visit = 0;
					goto next_iteration;
				}
			}

			uint32_t const lWeight = frame.lWeight;

			if (lWeight + rWeight > 1 &&
				(lWeight >= rWeight * Delta || rWeight >= lWeight * Delta))
				return false;

			rWeight = lWeight + rWeight + 1;
			if (hook->weight != rWeight)
				return false;

			hook = Eco_WB_HOOK_FROM_CHILDREN(hook->parent);
			--height;
		}
	}

	return true;
}


Hook* Core::Select(size_t rank) const
{
	Hook* hook = m_root.Value;
	Eco_Assert(hook != nullptr);

	while (true)
	{
		Eco_Assert(rank < hook->weight);

		size_t const leftWeight = Weight(hook->children[0]);
		if (leftWeight == rank) return hook;

		hook = hook->children[rank > leftWeight];
		if (rank > leftWeight) rank -= leftWeight;
	}
}

size_t Core::Rank(const Hook* hook) const
{
	LinkCheck(*hook, *this);

	size_t rank = 0;
	while (hook->parent != &m_root.Value)
	{
		const Hook* const parent = Eco_WB_HOOK_FROM_CHILDREN(hook->parent);

		if (hook != parent->children[0])
			rank += Weight(parent->children[0]);

		hook = parent;
	}
	return rank;
}

void Core::Insert(Hook* const hook, Ptr<Hook*> const parentAndSide)
{
	LinkInsert(*hook, *this);

	Hook** const parent = parentAndSide.Ptr();
	bool const l = parentAndSide.Tag();

	hook->children[0] = nullptr;
	hook->children[1] = nullptr;
	hook->parent = parent;
	hook->weight = 1;
	parent[l] = hook;
	
	Rebalance(&m_root.Value, parent, l, true);

	Eco_AssertSlow(Invariant(this));
}

void Core::Remove(Hook* const hook)
{
	LinkRemove(*hook, *this);

	Hook** const parent = hook->parent;
	bool const l = *parent != hook;

	Hook** balanceHook = parent;
	bool balanceL = l;

	Hook* lChild = hook->children[0];
	Hook* rChild = hook->children[1];

	// If hook is not a leaf.
	if (lChild != nullptr || rChild != nullptr)
	{
		// Heavier side of the tree on the left.
		bool const succL = Weight(rChild) > Weight(lChild);
		bool const succR = !succL;

		if (succL) std::swap(lChild, rChild);

		// Find the in-order successor of the hook on the heavier side of the tree.
		Hook* successor = lChild;

		balanceHook = lChild->children;
		balanceL = succL;

		if (lChild->children[succR] != nullptr)
		{
			successor = Leftmost(lChild, succR);

			Hook** const succParent = successor->parent;
			Hook* const succChild = successor->children[succL];

			// Attach the successor's child to the successor's parent.
			succParent[succR] = succChild;
			if (succChild != nullptr)
				succChild->parent = succParent;

			// Attach hook's direct child to the successor.
			successor->children[succL] = lChild;
			lChild->parent = successor->children;

			balanceHook = successor->parent;
			balanceL = succR;
		}

		successor->weight = hook->weight;

		// Attach the hook's other child to the successor.
		successor->children[succR] = rChild;
		if (rChild != nullptr)
			rChild->parent = successor->children;

		// Attach the successor to the removed hook's parent.
		parent[l] = successor;
		successor->parent = parent;
	}
	else
	{
		parent[l] = nullptr;
	}

	Rebalance(&m_root.Value, balanceHook, balanceL, false);

	Eco_AssertSlow(Invariant(this));
}

void Core::Clear()
{
	if (m_root.Value != nullptr)
	{
		Hook** children = Leftmost(m_root.Value, 0)->children;

		while (children != &m_root.Value)
		{
			if (children[1] != nullptr)
			{
				children = Leftmost(children[1], 0)->children;
			}
			else
			{
				Hook* const hook = Eco_WB_HOOK_FROM_CHILDREN(children);

				children = std::exchange(hook->parent, nullptr);
				children[hook != children[0]] = nullptr;

				LinkRemove(*hook, *this);
			}
		}

		m_root = nullptr;
	}

	Eco_AssertSlow(Invariant(this));
}

Private::List_::Hook* Core::Flatten()
{
	Hook* root = m_root.Value;

	if (root != nullptr)
	{
		m_root = nullptr;

		auto const FlattenSide = [](Hook* node, bool const l) -> Hook*
		{
			bool const r = !l;

			while (node->children[l] != nullptr)
			{
				// Rotate the whole left subtree right to left turning it into a list.

				Hook* const tail = node->children[l];
				Hook* head = tail;

				while (head->children[r] != nullptr)
				{
					Hook* const pivot = head->children[r];

					pivot->children[l] = head;
					head->children[r] = pivot;

					head = pivot;
				}

				node->children[l] = head;
				head->children[r] = node;

				node = tail;
			}

			return node;
		};

		Hook* const head = FlattenSide(root, 0);
		Hook* const tail = FlattenSide(root, 1);

		head->children[0] = tail;
		tail->children[1] = head;

		root = head;
	}

	Eco_AssertSlow(Invariant(this));

	return reinterpret_cast<List_::Hook*>(root);
}


Hook** Private::WbSet_::IteratorBegin(Hook** const root)
{
	Hook* const hook = *root;
	return hook == nullptr ? root : Leftmost(hook, 0)->children;
}

Hook** Private::WbSet_::IteratorAdvance(Hook** const children, bool const l)
{
	bool const r = !l;

	// If node has a right child: stop at its leftmost descendant.
	if (children[r] != nullptr) return Leftmost(children[r], l)->children;

	Hook* hook = Eco_WB_HOOK_FROM_CHILDREN(children);

	// Iterate through ancestors until the branch where the hook is the left child.
	while (hook != *hook->parent) hook = Eco_WB_HOOK_FROM_CHILDREN(hook->parent);

	return hook->parent;
}
