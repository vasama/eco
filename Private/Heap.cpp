#include "Eco/Heap.hpp"

#include <array>
#include <bit>
#include <limits>
#include <utility>

using namespace Eco;
using namespace Private::Heap_;

static std::pair<Hook**, Hook**> FindLast(Hook** const root, size_t const size)
{
	static constexpr size_t HighBitIndex = sizeof(size_t) * CHAR_BIT - 1;

	Eco_Assert(size > 0);

	Hook** parent = root;
	Hook** child = root;

	if (size > 1)
	{
		for (size_t i = HighBitIndex - std::countl_zero(size); i-- > 0;)
		{
			parent = (*child)->children;
			child = parent + (size >> i & 1);
		}
	}

	return { parent, child };
}

static void Swap(Hook* const parent, Hook* const child)
{
	Eco_Assert(child->parent == parent->children);

	// Attach child to grandparent.
	Hook** const grandparent = parent->parent;
	grandparent[grandparent[0] != parent] = child;

	// Attach parent to granchildren.
	for (Hook* const grandchild : child->children)
		if (grandchild != nullptr) grandchild->parent = parent->children;

	bool const side = parent->children[0] != child;

	// Attach child to its sibling as a parent.
	if (Hook* const sibling = parent->children[side ^ 1])
		sibling->parent = child->children;

	// Swap the children of parent and child.
	std::swap(parent->children, child->children);

	// Attach parent to child as a child on the appropriate side.
	child->children[side] = parent;

	// Attach child to parent as a parent.
	parent->parent = child->children;

	// attach grandparent to child as a parent.
	child->parent = grandparent;
}

// Walk towards the root and restore the heap property along the way.
static void PercolateToRoot(Core* const self, Hook* const hook, Comparator* const comparator)
{
	while (true)
	{
		Hook** const parent = hook->parent;

		// When the min element is reached, the heap property is restored.
		if (parent == &self->m_root) break;

		// If the parent is not the root, it is a Hook.
		Hook* const parentHook = reinterpret_cast<Hook*>(parent);

		// If the last node is not less than its parent, the heap property is restored.
		if (!comparator(self, parentHook, hook)) break;

		// Swap last with its parent.
		Swap(parentHook, hook);
	}
}

static bool Invariant(const Core* const self)
{
	size_t size = 0;
	if (const Hook* hook = self->m_root)
	{
		int8_t minHeight = std::numeric_limits<int8_t>::max();
		int8_t maxHeight = 0;

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

			for (uint8_t visit; (visit = frame.visit++) < 2;)
			{
				frame.lHeight = std::exchange(rHeight, 0);

				if (const Hook* const child = hook->children[visit])
				{
					if (child->parent != hook->children)
						return false;

					hook = child;
					stack[++height].visit = 0;
					goto next_iteration;
				}
			}

			int8_t const lHeight = frame.lHeight;
			if (lHeight < rHeight)
				return false;

			if (lHeight == 0)
			{
				minHeight = std::min(minHeight, height);
				maxHeight = std::max(maxHeight, height);
			}

			rHeight = std::max(lHeight, rHeight) + 1;

			hook = reinterpret_cast<const Hook*>(hook->parent);
			--height;

			++size;
		}

		Eco_Assert(minHeight <= maxHeight);
		if (maxHeight - minHeight > 1)
			return false;
	}
	return size == self->m_size;
}


void Core::Push(Hook* const hook, Comparator* const comparator)
{
	LinkInsert(*hook, *this);

	// Find the parent of, and the pointer to the last node.
	auto const [lastParent, lastParentChild] = FindLast(&m_root, ++m_size);

	// Clear hook's children and attach its new parent.
	hook->children[0] = nullptr;
	hook->children[1] = nullptr;
	hook->parent = lastParent;

	// Attach hook to its new parent.
	*lastParentChild = hook;

	// Percolate hook toward the root.
	PercolateToRoot(this, hook, comparator);

	Eco_AssertSlow(Invariant(this));
}

void Core::Remove(Hook* const hook, Comparator* const comparator)
{
	LinkRemove(*hook, *this);

	// Find the pointer to the last node.
	Hook** const lastParentChild = FindLast(&m_root, m_size--).second;

	// Remove the last node from the tree.
	Hook* const last = std::exchange(*lastParentChild, nullptr);

	// If the last node is being removed, exit.
	if (last == hook) return;

	// Replace the node being removed with the last node.
	*last = *hook;

	// Attach last to hook's children as parent.
	for (Hook* const child : hook->children)
		if (child != nullptr) child->parent = last->children;

	// Attach last to hook's parent as a child.
	hook->parent[hook->parent[0] != hook] = last;

	// Walk towards the leaves and restore the heap property along the way.
	while (true)
	{
		Hook* max = last;

		// Find the maximum of last and its children.
		for (Hook* const child : last->children)
			if (child != nullptr && comparator(this, max, child)) max = child;

		// If the last is ordered before its children, the heap property is restored.
		if (last == max) break;

		// Swap last with the maximum of its children.
		Swap(last, max);
	}

	PercolateToRoot(this, last, comparator);

	Eco_AssertSlow(Invariant(this));
}

Hook* Core::Pop(Comparator* const comparator)
{
	Eco_Assert(m_size > 0);
	Hook* const hook = m_root;
	Remove(hook, comparator);
	return hook;
}
