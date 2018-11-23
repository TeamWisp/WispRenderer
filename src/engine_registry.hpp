#pragma once

#include "registry.hpp"

namespace wr
{

	struct root_signatures
	{
		static RegistryHandle basic;
		static RegistryHandle lighting;
	};

	struct shaders
	{
		static RegistryHandle basic_vs;
		static RegistryHandle basic_ps;
		static RegistryHandle lighting_vs;
		static RegistryHandle lighting_ps;
	};

	struct pipelines
	{
		static RegistryHandle basic;
		static RegistryHandle lighting;
	};

} /* wr */