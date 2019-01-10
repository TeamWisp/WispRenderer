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
		static RegistryHandle mip_mapping;
		static RegistryHandle equirect_to_cubemap;
		static RegistryHandle accumulation;
	};

	struct shaders
	{
		static RegistryHandle basic_vs;
		static RegistryHandle basic_ps;
		static RegistryHandle fullscreen_quad_vs;
		static RegistryHandle deferred_composition_cs;
		static RegistryHandle rt_lib;
		static RegistryHandle mip_mapping_cs;
		static RegistryHandle equirect_to_cubemap_vs;
		static RegistryHandle equirect_to_cubemap_ps;
		static RegistryHandle accumulation;
	};

	struct pipelines
	{
		static RegistryHandle basic_deferred;
		static RegistryHandle deferred_composition;
		static RegistryHandle mip_mapping;
		static RegistryHandle equirect_to_cubemap;
		static RegistryHandle accumulation;
	};

	struct state_objects
	{
		static RegistryHandle state_object;
	};

} /* wr */