#pragma once

namespace Eco {

template<typename T>
class ForwardListObject;

template<>
struct ForwardListObject<void>
{
	ForwardListObject<void>* m_next = nullptr;
};

template<typename T>
struct ForwardListObject : ForwardListObject<void>
{
};

} // namespace Eco
