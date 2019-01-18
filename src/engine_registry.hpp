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
		static RegistryHandle rt_hybrid_global;
		static RegistryHandle rt_hybrid_local;
		static RegistryHandle cubemap_conversion;
		static RegistryHandle cubemap_convolution;
		static RegistryHandle post_processing;
	};

	struct shaders
	{
		static RegistryHandle basic_vs;
		static RegistryHandle basic_ps;
		static RegistryHandle fullscreen_quad_vs;
		static RegistryHandle deferred_composition_cs;
		static RegistryHandle rt_lib;
		static RegistryHandle rt_hybrid_lib;
		static RegistryHandle mip_mapping_cs;
		static RegistryHandle equirect_to_cubemap_vs;
		static RegistryHandle equirect_to_cubemap_ps;
		static RegistryHandle cubemap_convolution_ps;
		static RegistryHandle post_processing;
	};

	struct pipelines
	{
		static RegistryHandle basic_deferred;
		static RegistryHandle deferred_composition;
		static RegistryHandle mip_mapping;
		static RegistryHandle equirect_to_cubemap;
		static RegistryHandle cubemap_convolution;
		static RegistryHandle post_processing;
	};

	struct state_objects
	{
		static RegistryHandle state_object;
		static RegistryHandle rt_hybrid_state_object;
	};

} /* wr */