#include "Eco/AvlSet.hpp"

#include <array>
#include <utility>

#include <cmath>

using namespace Eco;
using namespace Private::AvlSet_;

static_assert(sizeof(Hook) == sizeof(AvlSetLink));
static_assert(CheckCompleteTaggedPointer<Ptr<Hook>>());


static Hook* Leftmost(Hook* hook, uintptr_t const l)
{
	while (!hook->children[l].IsZero())
	{
		hook = hook->children[l].Ptr();
	}
	return hook;
}

// Rotate from left to right.
static void Rotate(Hook* const root, bool const l, bool const single, bool const rootBalance)
{
	bool const r = !l;

	Ptr<Hook>* const parent = root->parent;
	Hook* const pivot = root->children[l].Ptr();
	Hook* const child = pivot->children[r].Ptr();

	bool const balance = !pivot->children[l].Tag() & single;

	root->children[l].Set(child, balance);
	root->children[r].SetTag(rootBalance);
	root->parent = pivot->children;

	pivot->children[l].SetTag(0);
	pivot->children[r].Set(root, balance);
	pivot->parent = parent;

	if (child != nullptr) child->parent = root->children;
	parent[root != parent->Ptr()].SetPtr(pivot);
}

static void Rebalance(Ptr<Hook>* const root, Ptr<Hook>* node, bool l, bool const insert)
{
	while (node != root)
	{
		l ^= !insert;
		bool const r = !l;

		// The node is not the root, so it is a Hook.
		Hook* const parent = Eco_AVL_HOOK_FROM_CHILDREN(node);
		Hook* newParent = parent;

		// If the node is now +2 on the l side.
		if (parent->children[l].Tag())
		{
			Hook* const child = parent->children[l].Ptr();

			newParent = child;

			bool const doubleRotation = child->children[r].Tag();
			bool const removalBalance = child->children[l].Tag();

			bool balance = 0;
			if (doubleRotation)
			{
				Hook* const pivot = child->children[r].Ptr();
				balance = pivot->children[l].Tag();

				// Rotate hook from r to l to allow the
				// subsequent parent rotation to balance the parent.
				Rotate(child, r, false, pivot->children[r].Tag());

				newParent = pivot;
			}

			// Rotate parent from l to r to balance.
			Rotate(parent, l, !doubleRotation, balance);

			// On insertion a single or double rotation always balances the tree.
			// On removal a single rotation balances the tree if the pivot is balanced.
			if (insert || doubleRotation == removalBalance) break;
		}
		else
		{
			// The node is either 0 or +1 on the l side.
			// Update the node's balancing factors.
			bool const balance = parent->children[r].Tag();

			// The l side becomes 1, or cancels out with the r side.
			parent->children[l].TagProxy() |= !balance;

			// The r side either is already 0, or it becomes 0.
			parent->children[r].SetTag(0);

			// If the r side is 1, the height of this subtree does not change.
			if (balance == insert) break;
		}

		node = newParent->parent;
		l = node->Ptr() != newParent;
	}
}

static bool Invariant(const Core* const self)
{
	if (self->m_root->Tag() != 0)
		return false;

	size_t size = 0;
	if (const Hook* hook = self->m_root->Ptr())
	{
		struct Frame
		{
			uint8_t visit : 2;
			uint8_t lHeight : 6;
		};

		Frame stack[sizeof(size_t) * CHAR_BIT];
		static_assert(std::size(stack) <= 1 << 6);

		int8_t height = 0;
		stack[0].visit = 0;
		int8_t rHeight = 0;

		while (height >= 0)
		{
		next_iteration:
			Frame& frame = stack[height];

			for (int8_t visit; (visit = frame.visit++) < 2;)
			{
				frame.lHeight = std::exchange(rHeight, 0);

				if (const Hook* const child = hook->children[visit].Ptr())
				{
					if (child->parent != hook->children)
						return false;

					hook = child;
					stack[++height].visit = 0;
					goto next_iteration;
				}

				if (hook->children[visit].Tag())
					return false;
			}

			int8_t const lHeight = frame.lHeight;

			if (std::abs(lHeight - rHeight) > 1)
				return false;
			if (hook->children[0].Tag() != (lHeight > rHeight))
				return false;
			if (hook->children[1].Tag() != (rHeight > lHeight))
				return false;

			rHeight = std::max(lHeight, rHeight) + 1;

			hook = Eco_AVL_HOOK_FROM_CHILDREN(hook->parent);
			--height;

			++size;
		}
	}

	return size == self->m_size.Value;
}


void Core::Insert(Hook* const hook, Ptr<Ptr<Hook>> const parentAndSide)
{
	LinkInsert(*hook, *this);

	++m_size.Value;

	Ptr<Hook>* const parent = parentAndSide.Ptr();
	bool const l = parentAndSide.Tag();

	hook->children[0] = nullptr;
	hook->children[1] = nullptr;
	hook->parent = parent;
	parent[l].SetPtr(hook);

	Rebalance(&m_root.Value, parent, l, true);

	Eco_AssertSlow(Invariant(this));
}

void Core::Remove(Hook* const hook)
{
	LinkRemove(*hook, *this);

	--m_size.Value;

	Ptr<Hook>* const parent = hook->parent;
	bool const l = parent->Ptr() != hook;

	Ptr<Hook>* balanceHook = parent;
	bool balanceL = l;

	// If hook is not a leaf.
	if (!hook->children[0].IsZero() || !hook->children[1].IsZero())
	{
		// Higher side of the tree on the left.
		bool const succL = hook->children[1].Tag();
		bool const succR = !succL;

		Hook* const lChild = hook->children[succL].Ptr();
		Hook* const rChild = hook->children[succR].Ptr();

		// Find the in-order successor of hook on the higher side of the tree.
		Hook* successor = lChild;

		balanceHook = lChild->children;
		balanceL = succL;

		if (!lChild->children[succR].IsZero())
		{
			successor = Leftmost(lChild, succR);

			Ptr<Hook>* const succParent = successor->parent;
			Hook* const succChild = successor->children[succL].Ptr();

			// Attach the successor's child to the successor's parent.
			succParent[succR].SetPtr(succChild);
			if (succChild != nullptr)
				succChild->parent = succParent;

			// Attach hook's direct child to the successor.
			successor->children[succL].Set(lChild, hook->children[succL].Tag());
			lChild->parent = successor->children;

			balanceHook = successor->parent;
			balanceL = succR;
		}

		successor->children[succL].SetTag(hook->children[succL].Tag());

		// Attach the hook's other child to the successor.
		// Tag is known to be zero on the lower side.
		successor->children[succR] = rChild;
		if (rChild != nullptr)
			rChild->parent = successor->children;

		// Attach the successor to the removed hook's parent.
		parent[l].SetPtr(successor);
		successor->parent = parent;
	}
	else
	{
		parent[l].SetPtr(nullptr);
	}

	Rebalance(&m_root.Value, balanceHook, balanceL, false);

	Eco_AssertSlow(Invariant(this));
}

void Core::Clear()
{
	if (!m_root->IsZero())
	{
		Ptr<Hook>* children = Leftmost(m_root->Ptr(), 0)->children;

		while (children != &m_root.Value)
		{
			if (!children[1].IsZero())
			{
				children = Leftmost(children[1].Ptr(), 0)->children;
			}
			else
			{
				Hook* const hook = Eco_AVL_HOOK_FROM_CHILDREN(children);

				children = std::exchange(hook->parent, nullptr);
				children[hook != children[0].Ptr()] = nullptr;

				LinkRemove(*hook, *this);
			}
		}

		m_root = nullptr;
		m_size = 0;
	}

	Eco_AssertSlow(Invariant(this));
}

Private::List_::Hook* Core::Flatten()
{
	Hook* root = m_root->Ptr();

	if (root != nullptr)
	{
		m_root = nullptr;
		m_size = 0;

		auto const FlattenSide = [](Hook* node, bool const l) -> Hook*
		{
			bool const r = !l;

			while (node->children[l].IsZero())
			{
				// Rotate the whole left subtree right to left turning it into a list.
				// It is important that each non-zero child pointer is overwritten to reset tags.

				Hook* const tail = node->children[l].Ptr();
				Hook* head = tail;

				while (!head->children[r].IsZero())
				{
					Hook* const pivot = head->children[r].Ptr();

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


Ptr<Hook>* Private::AvlSet_::IteratorBegin(Ptr<Hook>* const root)
{
	Hook* const hook = root->Ptr();
	return hook == nullptr ? root : Leftmost(hook, 0)->children;
}

Ptr<Hook>* Private::AvlSet_::IteratorAdvance(Ptr<Hook>* const children, uintptr_t const l)
{
	uintptr_t const r = l ^ 1;

	// If node has a right child: stop at its leftmost descendant.
	if (!children[r].IsZero()) return Leftmost(children[r].Ptr(), l)->children;

	Hook* hook = Eco_AVL_HOOK_FROM_CHILDREN(children);

	// Iterate through ancestors until the branch where the hook is the left child.
	while (hook != hook->parent->Ptr()) hook = Eco_AVL_HOOK_FROM_CHILDREN(hook->parent);

	return hook->parent;
}
