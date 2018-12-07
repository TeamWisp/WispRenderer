#pragma once

#include "registry.hpp"

namespace wr
{

	struct root_signatures
	{
		static RegistryHandle basic;
		static RegistryHandle deferred_composition;
	};

	struct shaders
	{
		static RegistryHandle basic_vs;
		static RegistryHandle basic_ps;
		static RegistryHandle fullscreen_quad_vs;
		static RegistryHandle deferred_composition_cs;
	};

	struct pipelines
	{
		static RegistryHandle basic_deferred;
		static RegistryHandle deferred_composition;
	};

} /* wr */