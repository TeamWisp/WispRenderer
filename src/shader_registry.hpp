#pragma once

#include "registry.hpp"
#include "d3d12/d3d12_enums.hpp"
#include "util/named_type.hpp"

namespace wr
{
	using Shader = void;

	struct ShaderDescription
	{
		std::string path;
		std::string entry;
		ShaderType type;
		std::vector<std::pair<std::wstring, std::wstring>> defines;
	};

	class ShaderRegistry : public internal::Registry<ShaderRegistry, Shader, ShaderDescription>
	{
	public:
		ShaderRegistry();
		virtual ~ShaderRegistry() = default;

		RegistryHandle Register(ShaderDescription description);
	};

} /* wr */
