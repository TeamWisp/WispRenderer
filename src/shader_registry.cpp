#include "shader_registry.hpp"

namespace wr
{

	ShaderRegistry::ShaderRegistry() : Registry<ShaderRegistry, Shader, ShaderDescription>()
	{
	}

	RegistryHandle ShaderRegistry::Register(ShaderDescription description)
	{
		auto handle = m_next_handle;

		m_descriptions.insert({ handle, description });

		m_next_handle++;
		return handle;
	}

} /* wr */
