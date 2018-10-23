#pragma once

#include <string>
#include <vector>

namespace wr::fg
{

	class TaskBase;

	class ResourceBase
	{
		friend class FrameGraph;
	public:
		explicit ResourceBase(const std::string& name, const TaskBase* creator) : name(name), creator(creator), ref_count(0)
		{
			static std::size_t id = 0;
			this->id = id++;
		}
		virtual ~ResourceBase() = default;

		ResourceBase(const ResourceBase&) = delete;
		ResourceBase(ResourceBase&&) = default;
		ResourceBase& operator=(const ResourceBase&) = delete;
		ResourceBase& operator=(ResourceBase&&) = default;

	protected:
		const TaskBase* creator;
		std::vector<const TaskBase*> readers;
		std::vector<const TaskBase*> writers;

		std::size_t id;
		std::string name;
		std::size_t ref_count;
	};

} /* wr::fg */