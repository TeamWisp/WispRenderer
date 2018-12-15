#pragma once

#include "registry.hpp"
#include "d3d12/d3d12_enums.hpp"
#include "util/named_type.hpp"

namespace wr
{
	struct Shader
	{

	};


	struct ShaderDescription
	{
		using Path = fluent::NamedType<std::string, ShaderDescription>;
		using Entry = fluent::NamedType<std::string, ShaderDescription>;
		using Type = fluent::NamedType<ShaderType, ShaderDescription>;

		Path path;
		Entry entry;
		Type type;
	};

	class ShaderRegistry : public internal::Registry<ShaderRegistry, Shader, ShaderDescription>
	{
	public:
		ShaderRegistry();
		virtual ~ShaderRegistry() = default;

		RegistryHandle Register(ShaderDescription description);
	};

} /* wr */