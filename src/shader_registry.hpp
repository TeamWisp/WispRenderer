#pragma once

#include "registry.hpp"
#include "d3d12/d3d12_enums.hpp"

namespace wr
{
	struct Shader
	{

	};

	struct ShaderDescription
	{
		std::string path;
		std::string entry;
		ShaderType type;
	};

	class ShaderRegistry : public internal::Registry<ShaderRegistry, Shader, ShaderDescription>
	{
	public:
		ShaderRegistry();
		virtual ~ShaderRegistry() = default;

		RegistryHandle Register(ShaderDescription description);
	};

} /* wr */