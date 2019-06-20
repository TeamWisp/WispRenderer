/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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