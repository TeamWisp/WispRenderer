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
			static C* instance = new C();
			return *instance;
			//static C instance;
			//return instance;
		}

		void RequestReload(RegistryHandle handle)
		{
			m_reload_request_mutex.lock();
			m_requested_reload.push_back(handle);
			m_reload_request_mutex.unlock();
		}

		std::map<RegistryHandle, TD> m_descriptions;
		std::map<RegistryHandle, TO*> m_objects;
		std::vector<RegistryHandle> m_requested_reload;
		std::mutex m_reload_request_mutex;

	protected:
		RegistryHandle m_next_handle;
	};

} /* wr::internal */