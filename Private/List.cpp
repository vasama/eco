#include "Eco/List.hpp"

using namespace Eco;
using namespace Private::List_;

static bool Invariant(const Core* const self)
{
	size_t size = 0;

	const Hook* hare = &self->m_root;
	const Hook* tortoise = &self->m_root;

	while (true)
	{
		hare = hare->siblings[0]->siblings[0];

		const Hook* const next = tortoise->siblings[0];
		if (next->siblings[1] != tortoise) return false;
		tortoise = next;

		if (tortoise == hare) break;

		++size;
	}

	return size == self->m_size;
}


Core::Core(Core&& src)
{
	Hook* root = &m_root;

	if ((m_size = src.m_size) > 0)
	{
		root = &src.m_root;

		Hook* const next = root->siblings[0];
		Hook* const prev = root->siblings[1];

		m_root.siblings[0] = next;
		m_root.siblings[1] = prev;

		prev->siblings[0] = &m_root;
		next->siblings[1] = &m_root;
	}

	root->siblings[0] = root;
	root->siblings[1] = root;
}

void Core::Adopt(Hook* const head, size_t const size)
{
	Eco_Assert(m_size == 0);
	m_size = size;

	Hook* const tail = m_root.siblings[1];
	Eco_Assert(tail->siblings[0] == head);

	m_root.siblings[0] = head;
	head->siblings[1] = &m_root;
	
	m_root.siblings[1] = tail;
	tail->siblings[0] = &m_root;

	Eco_AssertSlow(Invariant(this));
}

void Core::Insert(Hook* const prev, Hook* const hook, bool const before)
{
	LinkInsert(*hook, *this);

	++m_size;

	Hook* const next = prev->siblings[before];

	hook->siblings[before] = next;
	hook->siblings[!before] = prev;

	prev->siblings[before] = hook;
	next->siblings[!before] = hook;

	Eco_AssertSlow(Invariant(this));
}

void Core::Remove(Hook* const hook)
{
	LinkRemove(*hook, *this);

	--m_size;

	Hook* const next = hook->siblings[0];
	Hook* const prev = hook->siblings[1];

	prev->siblings[0] = next;
	next->siblings[1] = prev;

	Eco_AssertSlow(Invariant(this));
}
