#pragma once

#include <array>

#include "registry.hpp"
#include "d3d12/d3dx12.hpp"

namespace wr
{

	namespace rs_layout
	{
		enum class Type
		{
			SRV,
			SRV_RANGE,
			CBV_OR_CONST,
			CBV_RANGE,
			UAV,
			UAV_RANGE,
		};

		struct Entry
		{
			int name;
			int size;
			Type type;
		};

		template<typename T, typename E>
		constexpr unsigned int GetStart(const T data, const E name)
		{
			Type type = Type::CBV_OR_CONST;
			unsigned int start = 0;

			// Find Type
			for (std::size_t i = 0; i < data.size(); i++)
			{
				auto entry = data[i];
				if (static_cast<E>(entry.name) == name)
				{
					type = entry.type;
				}
			}
			
			// Find Start
			for (std::size_t i = 0; i < data.size(); i++)
			{
				auto entry = data[i];
				if (static_cast<E>(entry.name) == name)
				{
					break;
				}
				else if (entry.type == type)
				{
					start += entry.size;
				}
			}

			return start;
		}

		template<typename T, typename E>
		constexpr unsigned int GetHeapLoc(const T data, const E name)
		{
			unsigned int start = 0;

			// Find Start
			for (std::size_t i = 0; i < data.size(); i++)
			{
				auto entry = data[i];
				if (static_cast<E>(entry.name) == name)
				{
					break;
				}
				else if (entry.type == Type::CBV_RANGE || entry.type == Type::SRV_RANGE || entry.type == Type::UAV_RANGE)
				{
					start += entry.size;
				}
			}

			return start;
		}

		template<typename T, typename E>
		constexpr unsigned int GetSize(const T data, const E name)
		{
			for (std::size_t i = 0; i < data.size(); i++)
			{
				auto entry = data[i];
				if (static_cast<E>(entry.name) == name)
				{
					return entry.size;
				}
			}

			return 0;
		}

		template<typename T, typename E>
		constexpr CD3DX12_DESCRIPTOR_RANGE GetRange(const T data, const Type type, const E name)
		{
			unsigned int start = 0;
			unsigned int size = 0;

			// Find Start & Size
			for (std::size_t i = 0; i < data.size(); i++)
			{
				auto entry = data[i];
				if (static_cast<E>(entry.name) == name)
				{
					size = entry.size;
					break;
				}
				else if (entry.type == type)
				{
					start += entry.size;
				}
			}

			D3D12_DESCRIPTOR_RANGE_TYPE d3d12_range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			switch (type)
			{
			case Type::SRV_RANGE:
				d3d12_range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				break;
			case Type::UAV_RANGE:
				d3d12_range_type = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				break;
			case Type::CBV_RANGE:
				d3d12_range_type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
				break;
			default:
				LOGW("Unknown range type");
				break;
			}

			CD3DX12_DESCRIPTOR_RANGE retval;
			retval.Init(d3d12_range_type, size, start);
			return retval;
		}

		template<typename T, typename E>
		constexpr CD3DX12_ROOT_PARAMETER GetCBV(const T data, const E name, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			const Type type = Type::CBV_OR_CONST;
			unsigned int start = 0;
			unsigned int size = 0;

			// Find Start & Size
			for (std::size_t i = 0; i < data.size(); i++)
			{
				auto entry = data[i];
				if (static_cast<E>(entry.name) == name)
				{
					size = entry.size;
					break;
				}
				else if (entry.type == type)
				{
					start += entry.size;
				}
			}

			CD3DX12_ROOT_PARAMETER retval;
			retval.InitAsConstantBufferView(start, 0, visibility);

			return retval;
		}

		template<typename T, typename E>
		constexpr CD3DX12_ROOT_PARAMETER GetConstants(const T data, const E name, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			const Type type = Type::CBV_OR_CONST;
			unsigned int start = 0;
			unsigned int size = 0;

			// Find Start
			for (std::size_t i = 0; i < data.size(); i++)
			{
				auto entry = data[i];
				if (static_cast<E>(entry.name) == name)
				{
					size = entry.size;
					break;
				}
				else if (entry.type == type)
				{
					start += entry.size;
				}
			}

			CD3DX12_ROOT_PARAMETER retval;
			retval.InitAsConstants(size, start, 0, visibility);

			return retval;
		}

	} /* rs_layout */

	namespace srv
	{

		enum class BasicE
		{
			CAMERA_PROPERTIES,
			OBJECT_PROPERTIES,
			ALBEDO,
			NORMAL,
			ROUGHNESS,
			METALLIC,
		};

		constexpr std::array<rs_layout::Entry, 6> basic = {
			rs_layout::Entry{(int)BasicE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)BasicE::OBJECT_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)BasicE::ALBEDO, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BasicE::NORMAL, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BasicE::ROUGHNESS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BasicE::METALLIC, 1, rs_layout::Type::SRV_RANGE}
		};

		enum class DeferredCompositionE
		{
			CAMERA_PROPERTIES,
			GBUFFER_ALBEDO_ROUGHNESS,
			GBUFFER_NORMAL_METALLIC,
			GBUFFER_DEPTH,
			LIGHT_BUFFER,
			SKY_BOX,
			IRRADIANCE_MAP,
			OUTPUT,
		};

		constexpr std::array<rs_layout::Entry, 8> deferred_composition = {
			rs_layout::Entry{(int)DeferredCompositionE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)DeferredCompositionE::GBUFFER_ALBEDO_ROUGHNESS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::GBUFFER_NORMAL_METALLIC, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::GBUFFER_DEPTH, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::LIGHT_BUFFER, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::SKY_BOX, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::IRRADIANCE_MAP, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::OUTPUT, 1, rs_layout::Type::UAV_RANGE}
		};

		enum class MipMappingE
		{
			SOURCE,
			DEST,
			TEXEL_SIZE,
		};

		constexpr std::array<rs_layout::Entry, 7> mip_mapping = {
			rs_layout::Entry{(int)MipMappingE::SOURCE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)MipMappingE::DEST, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)MipMappingE::TEXEL_SIZE, 2, rs_layout::Type::CBV_OR_CONST},
		};

	} /* srv */

	struct root_signatures
	{
		static RegistryHandle basic;
		static RegistryHandle deferred_composition;
		static RegistryHandle rt_test_global;
		static RegistryHandle mip_mapping;
		static RegistryHandle rt_hybrid_global;
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