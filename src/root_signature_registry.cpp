#include "root_signature_registry.hpp"

namespace wr
{

	RootSignatureRegistry::RootSignatureRegistry() : Registry<RootSignatureRegistry, RootSignature, RootSignatureDescription>()
	{
	}

	RegistryHandle RootSignatureRegistry::Register(RootSignatureDescription description)
	{
		auto handle = m_next_handle;

		m_descriptions.insert({ handle, description });

		m_next_handle++;
		return handle;
	}

} /* wr */
