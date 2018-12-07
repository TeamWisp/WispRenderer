#pragma once

#include "registry.hpp"

namespace wr
{

	struct root_signatures
	{
		static RegistryHandle basic;
		static RegistryHandle deferred_composition;
		static RegistryHandle rt_test_global;
		static RegistryHandle rt_test_local;
	};

	struct shaders
	{
		static RegistryHandle basic_vs;
		static RegistryHandle basic_ps;
		static RegistryHandle fullscreen_quad_vs;
		static RegistryHandle deferred_composition_ps;
		static RegistryHandle rt_lib;
	};

	struct pipelines
	{
		static RegistryHandle basic_deferred;
		static RegistryHandle deferred_composition;
	};

	struct state_objects
	{
		static RegistryHandle state_object;
	};

} /* wr */