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
#pragma once

#include <map>
#include <mutex>

#include "util/log.hpp"

#undef GetObject // Prevent macro collision

namespace wr
{
	using RegistryHandle = std::uint64_t;
}

namespace wr::internal
{
	template<typename C, typename TO, typename TD>
	class Registry
	{
	protected:
		Registry() : m_next_handle(0) { }

	public:
		virtual ~Registry() = default;

		TO* Find(RegistryHandle handle)
		{
			auto it = m_objects.find(handle);
			if (it != m_objects.end())
			{
				return (*it).second;
			}
			else
			{
				LOGE("Failed to find registry object related to registry handle.");
				return nullptr;
			}
		}

		static C& Get()
		{
			static C instance = {};
			return instance;
		}

		void Lock()
		{
			m_mutex.lock();
		}

		void Unlock()
		{
			m_mutex.unlock();
		}

		void ClearReloadRequests()
		{
			m_requested_reload.clear();
		}

		std::vector<RegistryHandle> const & GetReloadRequests()
		{
			return m_requested_reload;
		}

		void RequestReload(RegistryHandle handle)
		{
			Lock();
			m_requested_reload.push_back(handle);
			Unlock();
		}

		std::map<RegistryHandle, TD> m_descriptions;
		std::map<RegistryHandle, TO*> m_objects;

	protected:
		std::vector<RegistryHandle> m_requested_reload;
		std::mutex m_mutex;
		RegistryHandle m_next_handle;
	};

} /* wr::internal */