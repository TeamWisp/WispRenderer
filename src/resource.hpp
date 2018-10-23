#pragma once

#include <variant>
#include "resource_base.hpp"

namespace wr::fg
{

	template<typename T>
	class Resource : public ResourceBase
	{
		friend class FrameGraph;
	public:
		explicit Resource(std::string const & name, const TaskBase* creator, T* actual = nullptr)
			: ResourceBase(name, creator), actual(actual)
		{
		}
		virtual ~Resource() = default;

		T* Get() const // If transient, only valid through the realized interval of the resource.
		{
			return actual;
		}

	private:
		T* actual;
	};

} /* wr::fg */