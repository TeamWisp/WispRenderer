#include "id_factory.hpp"

namespace wr
{

	IDFactory::IDFactory()
		: m_id(0)
	{
	}

	void IDFactory::MakeIDAvailable(std::uint32_t unused_id)
	{
		m_unused_ids.push_back(unused_id);
	}

	std::uint32_t IDFactory::GetUnusedID()
	{
		std::uint32_t ret_id;

		if (m_unused_ids.empty())
		{
			ret_id = m_id;
			m_id++;
		}
		else
		{
			ret_id = *(m_unused_ids.end() - 1);
			m_unused_ids.pop_back();
		}

		return ret_id;
	}

} /* wr */