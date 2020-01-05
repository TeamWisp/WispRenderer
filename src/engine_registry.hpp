/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <array>

#include "registry.hpp"
#include "d3d12/d3dx12.hpp"
#include "d3d12/d3d12_settings.hpp"

#define COMPILATION_EVAL(e) (std::integral_constant<decltype(e), e>::value)

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

			// Find its range equivelant or visa versa
            Type other_type = Type::SRV;
			switch (type)
			{
			case Type::SRV:
				other_type = Type::SRV_RANGE;
				break;
			case Type::SRV_RANGE:
				other_type = Type::SRV;
				break;
			case Type::CBV_OR_CONST:
				other_type = Type::CBV_RANGE;
				break;
			case Type::CBV_RANGE:
				other_type = Type::CBV_OR_CONST;
				break;
			case Type::UAV:
				other_type = Type::UAV_RANGE;
				break;
			case Type::UAV_RANGE:
				other_type = Type::UAV;
				break;
			}

			// Find Start & Size
			for (std::size_t i = 0; i < data.size(); i++)
			{
				auto entry = data[i];
				if (static_cast<E>(entry.name) == name)
				{
					size = entry.size;
					break;
				}
				else if (entry.type == type || entry.type == other_type)
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
			unsigned int start = 0;

			// Find Start & Size
			for (std::size_t i = 0; i < data.size(); i++)
			{
				auto entry = data[i];
				if (static_cast<E>(entry.name) == name)
				{
					break;
				}
				else if (entry.type == Type::CBV_OR_CONST || entry.type == Type::CBV_RANGE)
				{
					start += entry.size;
				}
			}

			CD3DX12_ROOT_PARAMETER retval;
			retval.InitAsConstantBufferView(start, 0, visibility);

			return retval;
		}

		template<typename T, typename E>
		constexpr CD3DX12_ROOT_PARAMETER GetSRV(const T data, const E name, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			unsigned int start = 0;

			// Find Start & Size
			for (std::size_t i = 0; i < data.size(); i++)
			{
				auto entry = data[i];
				if (static_cast<E>(entry.name) == name)
				{
					break;
				}
				else if (entry.type == Type::SRV || entry.type == Type::SRV_RANGE)
				{
					start += entry.size;
				}
			}

			CD3DX12_ROOT_PARAMETER retval;
			retval.InitAsShaderResourceView(start, 0, visibility);

			return retval;
		}

		template<typename T, typename E>
		constexpr CD3DX12_ROOT_PARAMETER GetConstants(const T data, const E name, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
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
				else if (entry.type == Type::CBV_OR_CONST || entry.type == Type::CBV_RANGE)
				{
					start += entry.size;
				}
			}

			CD3DX12_ROOT_PARAMETER retval;
			retval.InitAsConstants(size, start, 0, visibility);

			return retval;
		}

	} /* rs_layout */

	namespace params
	{
		enum class BRDF_LutE
		{
			OUTPUT,
		};

		constexpr std::array<rs_layout::Entry, 1> brdf_lut = {
			rs_layout::Entry{(int)BRDF_LutE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},
		};


		enum class DeferredMainE
		{
			CAMERA_PROPERTIES,
			OBJECT_PROPERTIES,
			ALBEDO,
			NORMAL,
			ROUGHNESS,
			METALLIC,
			AMBIENT_OCCLUSION,
			EMISSIVE,
			MATERIAL_PROPERTIES,
		};

		constexpr std::array<rs_layout::Entry, 9> deferred_main = {
			rs_layout::Entry{(int)DeferredMainE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)DeferredMainE::OBJECT_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)DeferredMainE::ALBEDO, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredMainE::NORMAL, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredMainE::ROUGHNESS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredMainE::METALLIC, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredMainE::AMBIENT_OCCLUSION, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredMainE::EMISSIVE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredMainE::MATERIAL_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
		};

		enum class SVGFDenoiserE
		{
			INPUT,
			MOTION,
			NORMAL,
			DEPTH,

			IN_HIST_LENGTH,

			PREV_INPUT,
			PREV_MOMENTS,
			PREV_NORMAL,
			PREV_DEPTH,

			OUT_COLOR,
			OUT_MOMENTS,
			OUT_HIST_LENGTH,

			SVGF_PROPERTIES,

			PING_PONG_UAV,
			PING_PONG_SRV,
			OUTPUT_SRV,
		};

		constexpr std::array<rs_layout::Entry, 16> svgf_denoiser = {
			rs_layout::Entry{(int)SVGFDenoiserE::INPUT, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)SVGFDenoiserE::MOTION, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)SVGFDenoiserE::NORMAL, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)SVGFDenoiserE::DEPTH, 1, rs_layout::Type::SRV_RANGE},

			rs_layout::Entry{(int)SVGFDenoiserE::IN_HIST_LENGTH, 1, rs_layout::Type::SRV_RANGE},

			rs_layout::Entry{(int)SVGFDenoiserE::PREV_INPUT, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)SVGFDenoiserE::PREV_MOMENTS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)SVGFDenoiserE::PREV_NORMAL, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)SVGFDenoiserE::PREV_DEPTH, 1, rs_layout::Type::SRV_RANGE},

			rs_layout::Entry{(int)SVGFDenoiserE::OUT_COLOR, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)SVGFDenoiserE::OUT_MOMENTS, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)SVGFDenoiserE::OUT_HIST_LENGTH, 1, rs_layout::Type::UAV_RANGE},

			rs_layout::Entry{(int)SVGFDenoiserE::SVGF_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},

			rs_layout::Entry{(int)SVGFDenoiserE::PING_PONG_UAV, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)SVGFDenoiserE::PING_PONG_SRV, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)SVGFDenoiserE::OUTPUT_SRV, 1, rs_layout::Type::SRV_RANGE},
		};

		enum class DeferredCompositionE
		{
			CAMERA_PROPERTIES,
			GBUFFER_ALBEDO_ROUGHNESS,
			GBUFFER_NORMAL_METALLIC,
			GBUFFER_EMISSIVE_AO,
			GBUFFER_DEPTH,
			LIGHT_BUFFER,
			SKY_BOX,
			IRRADIANCE_MAP,
			PREF_ENV_MAP,
			BRDF_LUT,
			BUFFER_REFLECTION,
			BUFFER_SHADOW,
			BUFFER_SCREEN_SPACE_IRRADIANCE,
			BUFFER_AO,
			OUTPUT,
		};

		constexpr std::array<rs_layout::Entry, 15> deferred_composition = {
			rs_layout::Entry{(int)DeferredCompositionE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)DeferredCompositionE::GBUFFER_ALBEDO_ROUGHNESS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::GBUFFER_NORMAL_METALLIC, 1, rs_layout::Type::SRV_RANGE},			
			rs_layout::Entry{(int)DeferredCompositionE::GBUFFER_EMISSIVE_AO, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::GBUFFER_DEPTH, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::LIGHT_BUFFER, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::SKY_BOX, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::IRRADIANCE_MAP, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::PREF_ENV_MAP, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::BRDF_LUT, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::BUFFER_REFLECTION, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::BUFFER_SHADOW, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::BUFFER_SCREEN_SPACE_IRRADIANCE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::BUFFER_AO, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DeferredCompositionE::OUTPUT, 1, rs_layout::Type::UAV_RANGE}
		};

		enum class MipMappingE
		{
			SOURCE,
			DEST,
			CBUFFER,
		};

		constexpr std::array<rs_layout::Entry, 3> mip_mapping = {
			rs_layout::Entry{(int)MipMappingE::SOURCE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)MipMappingE::DEST, 4, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)MipMappingE::CBUFFER, 6, rs_layout::Type::CBV_OR_CONST},
		};

		enum class CubemapConversionE
		{
			EQUIRECTANGULAR_TEXTURE,
			IDX,
			CAMERA_PROPERTIES,
		};

		constexpr std::array<rs_layout::Entry, 3> cubemap_conversion = {
			rs_layout::Entry{(int)CubemapConversionE::EQUIRECTANGULAR_TEXTURE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)CubemapConversionE::IDX, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)CubemapConversionE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
		};

		enum class CubemapConvolutionE
		{
			ENVIRONMENT_CUBEMAP,
			IDX,
			CAMERA_PROPERTIES,
		};

		constexpr std::array<rs_layout::Entry, 3> cubemap_convolution = {
			rs_layout::Entry{(int)CubemapConvolutionE::ENVIRONMENT_CUBEMAP, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)CubemapConvolutionE::IDX, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)CubemapConvolutionE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
		};

		enum class CubemapPrefilteringE
		{
			SOURCE,
			DEST,
			CBUFFER,
		};

		constexpr std::array<rs_layout::Entry, 3> cubemap_prefiltering = {
			rs_layout::Entry{(int)CubemapPrefilteringE::SOURCE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)CubemapPrefilteringE::DEST, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)CubemapPrefilteringE::CBUFFER, 6, rs_layout::Type::CBV_OR_CONST},
		};

		enum class PostProcessingE
		{
			SOURCE,
			DEST,
			HDR_SUPPORT,
		};

		constexpr std::array<rs_layout::Entry, 3> post_processing = {
			rs_layout::Entry{(int)PostProcessingE::SOURCE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)PostProcessingE::DEST, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)PostProcessingE::HDR_SUPPORT, 2, rs_layout::Type::CBV_OR_CONST},
		};

		enum class AccumulationE
		{
			SOURCE,
			DEST,
			FRAME_IDX,
		};

		constexpr std::array<rs_layout::Entry, 3> accumulation = {
			rs_layout::Entry{(int)AccumulationE::SOURCE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)AccumulationE::DEST, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)AccumulationE::FRAME_IDX, 2, rs_layout::Type::CBV_OR_CONST},
		};

		enum class FullRaytracingE
		{
			CAMERA_PROPERTIES,
			ACCELERATION_STRUCTURE,
			OUTPUT,
			INDICES,
			VERTICES,
			LIGHTS,
			MATERIALS,
			OFFSETS,
			SKYBOX,
			BRDF_LUT,
			IRRADIANCE_MAP,
			TEXTURES,
			FALLBACK_PTRS
		};

		constexpr std::array<rs_layout::Entry, 13> full_raytracing = {
			rs_layout::Entry{(int)FullRaytracingE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)FullRaytracingE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)FullRaytracingE::ACCELERATION_STRUCTURE, 1, rs_layout::Type::SRV},
			rs_layout::Entry{(int)FullRaytracingE::INDICES, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)FullRaytracingE::LIGHTS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)FullRaytracingE::VERTICES, 1, rs_layout::Type::SRV},
			rs_layout::Entry{(int)FullRaytracingE::MATERIALS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)FullRaytracingE::OFFSETS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)FullRaytracingE::SKYBOX, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)FullRaytracingE::BRDF_LUT, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)FullRaytracingE::IRRADIANCE_MAP, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)FullRaytracingE::TEXTURES, d3d12::settings::num_max_rt_textures, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)FullRaytracingE::FALLBACK_PTRS, 5, rs_layout::Type::SRV_RANGE},
		};

		enum class RTHybridE
		{
			CAMERA_PROPERTIES,
			ACCELERATION_STRUCTURE,
			OUTPUT,
			INDICES,
			VERTICES,
			LIGHTS,
			MATERIALS,
			OFFSETS,
			SKYBOX,
			PREF_ENV_MAP,
			BRDF_LUT,
			IRRADIANCE_MAP,
			TEXTURES,
			GBUFFERS,
			FALLBACK_PTRS
		};

		constexpr std::array<rs_layout::Entry, 20> rt_hybrid = {
			rs_layout::Entry{(int)RTHybridE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)RTHybridE::OUTPUT, 3, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)RTHybridE::ACCELERATION_STRUCTURE, 1, rs_layout::Type::SRV},
			rs_layout::Entry{(int)RTHybridE::INDICES, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)RTHybridE::LIGHTS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)RTHybridE::VERTICES, 1, rs_layout::Type::SRV},
			rs_layout::Entry{(int)RTHybridE::MATERIALS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)RTHybridE::OFFSETS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)RTHybridE::SKYBOX, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)RTHybridE::PREF_ENV_MAP, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)RTHybridE::BRDF_LUT, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)RTHybridE::IRRADIANCE_MAP, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)RTHybridE::TEXTURES, d3d12::settings::num_max_rt_textures, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)RTHybridE::GBUFFERS, 4, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)RTHybridE::FALLBACK_PTRS, 9, rs_layout::Type::SRV_RANGE},
		};

		enum class SpatialReconstructionE
		{
			CAMERA_PROPERTIES,
			OUTPUT,
			REFLECTION_BUFFER,
			GBUFFERS
		};

		constexpr std::array<rs_layout::Entry, 4> spatial_reconstruction = {
			rs_layout::Entry{(int)SpatialReconstructionE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)SpatialReconstructionE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)SpatialReconstructionE::REFLECTION_BUFFER, 2, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)SpatialReconstructionE::GBUFFERS, 3, rs_layout::Type::SRV_RANGE}
		};

    enum class ReflectionDenoiserE
    {
		INPUT,
		RAY_RAW,
		RAY_DIR,
		ALBEDO_ROUGHNESS,
		NORMAL_METALLIC,
		MOTION,
		LINEAR_DEPTH,
		WORLD_POS,

		IN_HISTORY,

		ACCUM,
		PREV_NORMAL,
		PREV_DEPTH,
		IN_MOMENTS,

		OUTPUT,
		OUT_HISTORY,
		OUT_MOMENTS,

		CAMERA_PROPERTIES,
		DENOISER_SETTINGS,
		WAVELET_ITERATION,
    };

    constexpr std::array<rs_layout::Entry, 19> reflection_denoiser = {
		rs_layout::Entry{(int)ReflectionDenoiserE::INPUT, 1, rs_layout::Type::SRV_RANGE},
		rs_layout::Entry{(int)ReflectionDenoiserE::RAY_RAW, 1, rs_layout::Type::SRV_RANGE},
		rs_layout::Entry{(int)ReflectionDenoiserE::RAY_DIR, 1, rs_layout::Type::SRV_RANGE},
		rs_layout::Entry{(int)ReflectionDenoiserE::ALBEDO_ROUGHNESS, 1, rs_layout::Type::SRV_RANGE},
		rs_layout::Entry{(int)ReflectionDenoiserE::NORMAL_METALLIC, 1, rs_layout::Type::SRV_RANGE},
		rs_layout::Entry{(int)ReflectionDenoiserE::MOTION, 1, rs_layout::Type::SRV_RANGE},
		rs_layout::Entry{(int)ReflectionDenoiserE::LINEAR_DEPTH, 1, rs_layout::Type::SRV_RANGE},
		rs_layout::Entry{(int)ReflectionDenoiserE::WORLD_POS, 1, rs_layout::Type::SRV_RANGE},

		rs_layout::Entry{(int)ReflectionDenoiserE::IN_HISTORY, 1, rs_layout::Type::SRV_RANGE},

		rs_layout::Entry{(int)ReflectionDenoiserE::ACCUM, 1, rs_layout::Type::SRV_RANGE},
		rs_layout::Entry{(int)ReflectionDenoiserE::PREV_NORMAL, 1, rs_layout::Type::SRV_RANGE},
		rs_layout::Entry{(int)ReflectionDenoiserE::PREV_DEPTH, 1, rs_layout::Type::SRV_RANGE},
		rs_layout::Entry{(int)ReflectionDenoiserE::IN_MOMENTS, 1, rs_layout::Type::SRV_RANGE},

		rs_layout::Entry{(int)ReflectionDenoiserE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},
		rs_layout::Entry{(int)ReflectionDenoiserE::OUT_HISTORY, 1, rs_layout::Type::UAV_RANGE},
		rs_layout::Entry{(int)ReflectionDenoiserE::OUT_MOMENTS, 1, rs_layout::Type::UAV_RANGE},

		rs_layout::Entry{(int)ReflectionDenoiserE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
		rs_layout::Entry{(int)ReflectionDenoiserE::DENOISER_SETTINGS, 1, rs_layout::Type::CBV_OR_CONST},
		rs_layout::Entry{(int)ReflectionDenoiserE::WAVELET_ITERATION, 1, rs_layout::Type::CBV_OR_CONST},
    };

		enum class RTAOE
		{
			CAMERA_PROPERTIES,
			ACCELERATION_STRUCTURE,
			OUTPUT,
			GBUFFERS,
			FALLBACK_PTRS
		};

		constexpr std::array<rs_layout::Entry, 5> rt_ao = {
			rs_layout::Entry{(int)RTAOE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)RTAOE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)RTAOE::ACCELERATION_STRUCTURE, 1, rs_layout::Type::SRV},
			rs_layout::Entry{(int)RTAOE::GBUFFERS, 2, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)RTAOE::FALLBACK_PTRS, 9, rs_layout::Type::SRV_RANGE},
		};

		enum class PathTracingE
		{
			CAMERA_PROPERTIES,
			ACCELERATION_STRUCTURE,
			OUTPUT,
			INDICES,
			VERTICES,
			LIGHTS,
			MATERIALS,
			OFFSETS,
			SKYBOX,
			PREF_ENV_MAP,
			BRDF_LUT,
			IRRADIANCE_MAP,
			TEXTURES,
			GBUFFERS,
			FALLBACK_PTRS
		};

		constexpr std::array<rs_layout::Entry, 20> path_tracing = {
			rs_layout::Entry{(int)PathTracingE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
			rs_layout::Entry{(int)PathTracingE::OUTPUT, 1, rs_layout::Type::UAV_RANGE}, // TEMPORARY: This should be 1. its 2 so the path tracer doesn't overwrite it.
			rs_layout::Entry{(int)PathTracingE::ACCELERATION_STRUCTURE, 1, rs_layout::Type::SRV},
			rs_layout::Entry{(int)PathTracingE::INDICES, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)PathTracingE::LIGHTS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)PathTracingE::VERTICES, 1, rs_layout::Type::SRV},
			rs_layout::Entry{(int)PathTracingE::MATERIALS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)PathTracingE::OFFSETS, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)PathTracingE::SKYBOX, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)PathTracingE::PREF_ENV_MAP, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)PathTracingE::BRDF_LUT, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)PathTracingE::IRRADIANCE_MAP, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)PathTracingE::TEXTURES, d3d12::settings::num_max_rt_textures, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)PathTracingE::GBUFFERS, 4, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)PathTracingE::FALLBACK_PTRS, 9, rs_layout::Type::SRV_RANGE},
		};

		enum class DoFCoCE
		{
			CAMERA_PROPERTIES,
			GDEPTH,
			OUTPUT
		};

		constexpr std::array<rs_layout::Entry, 3> dof_coc = {
			rs_layout::Entry{(int)DoFCoCE::GDEPTH, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DoFCoCE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)DoFCoCE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST}
		};

		enum class DoFNearMaskE
		{
			INPUT,
			OUTPUT
		};

		constexpr std::array<rs_layout::Entry, 2> dof_near_mask = {
			rs_layout::Entry{(int)DoFNearMaskE::INPUT, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DoFNearMaskE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},

		};

		enum class DownScaleE
		{
			SOURCE,
			OUTPUT_NEAR,
			OUTPUT_FAR,
			COC,
		};

		constexpr std::array<rs_layout::Entry,4> down_scale = {
			rs_layout::Entry{(int)DownScaleE::SOURCE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DownScaleE::OUTPUT_NEAR, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)DownScaleE::OUTPUT_FAR, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)DownScaleE::COC, 1, rs_layout::Type::SRV_RANGE},
		};



		enum class DoFDilateE
		{
			SOURCE,
			OUTPUT
		};

		constexpr std::array<rs_layout::Entry, 2> dof_dilate = {
			rs_layout::Entry{(int)DoFDilateE::SOURCE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DoFDilateE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},
		};

		enum class DoFBokehE
		{
			CAMERA_PROPERTIES,
			SOURCE_NEAR,
			SOURCE_FAR,
			OUTPUT_NEAR,
			OUTPUT_FAR,
			COC
		};

		constexpr std::array<rs_layout::Entry, 6> dof_bokeh = {
			rs_layout::Entry{(int)DoFBokehE::SOURCE_NEAR, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DoFBokehE::SOURCE_FAR, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DoFBokehE::OUTPUT_NEAR, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)DoFBokehE::OUTPUT_FAR, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)DoFBokehE::COC, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DoFBokehE::CAMERA_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
		};

		enum class DoFBokehPostFilterE
		{
			SOURCE_NEAR,
			SOURCE_FAR,
			OUTPUT_NEAR,
			OUTPUT_FAR
		};

		constexpr std::array<rs_layout::Entry, 4> dof_bokeh_post_filter = {
			rs_layout::Entry{(int)DoFBokehPostFilterE::SOURCE_NEAR, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DoFBokehPostFilterE::SOURCE_FAR, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DoFBokehPostFilterE::OUTPUT_NEAR, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)DoFBokehPostFilterE::OUTPUT_FAR, 1, rs_layout::Type::UAV_RANGE}
		};

		enum class DoFCompositionE
		{
			SOURCE,
			OUTPUT,
			BOKEH_NEAR,
			BOKEH_FAR,
			COC
		};

		constexpr std::array<rs_layout::Entry, 5> dof_composition = {
			rs_layout::Entry{(int)DoFCompositionE::SOURCE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DoFCompositionE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)DoFCompositionE::BOKEH_NEAR, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DoFCompositionE::BOKEH_FAR, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)DoFCompositionE::COC, 1, rs_layout::Type::SRV_RANGE},
		};

		enum class BloomExtractBrightE
		{
			SOURCE,
			G_EMISSIVE,
			G_DEPTH,
			OUTPUT_BRIGHT
		};

		constexpr std::array<rs_layout::Entry, 4> bloom_extract_bright = {
			rs_layout::Entry{(int)BloomExtractBrightE::SOURCE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BloomExtractBrightE::G_EMISSIVE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BloomExtractBrightE::G_DEPTH, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BloomExtractBrightE::OUTPUT_BRIGHT, 1, rs_layout::Type::UAV_RANGE},
		};
		enum class BloomBlurE
		{
			SOURCE,
			OUTPUT,
			BLUR_DIRECTION
		};

		constexpr std::array<rs_layout::Entry, 3> bloom_blur = {
			rs_layout::Entry{(int)BloomBlurE::SOURCE, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BloomBlurE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)BloomBlurE::BLUR_DIRECTION, 1, rs_layout::Type::CBV_OR_CONST},
		};

		enum class BloomBlurHorizontalE
		{
			BLOOM_PROPERTIES,
			SOURCE_MAIN,
			OUTPUT,
			OUTPUT_QES
		};

		constexpr std::array<rs_layout::Entry, 4> bloom_blur_horizontal = {
			rs_layout::Entry{(int)BloomBlurHorizontalE::SOURCE_MAIN, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BloomBlurHorizontalE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)BloomBlurHorizontalE::OUTPUT_QES, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)BloomBlurHorizontalE::BLOOM_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
		};

		enum class BloomBlurVerticalE
		{
			BLOOM_PROPERTIES,
			SOURCE_MAIN,
			SOURCE_QES,
			OUTPUT,
			OUTPUT_QES
		};

		constexpr std::array<rs_layout::Entry, 5> bloom_blur_vertical = {
			rs_layout::Entry{(int)BloomBlurVerticalE::SOURCE_MAIN, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BloomBlurVerticalE::SOURCE_QES, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BloomBlurVerticalE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)BloomBlurVerticalE::OUTPUT_QES, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)BloomBlurVerticalE::BLOOM_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
		};

		enum class BloomCompositionE
		{
			BLOOM_PROPERTIES,
			SOURCE_MAIN,
			SOURCE_BLOOM_HALF,
			SOURCE_BLOOM_QES,
			OUTPUT
		};

		constexpr std::array<rs_layout::Entry, 5> bloom_composition = {
			rs_layout::Entry{(int)BloomCompositionE::SOURCE_MAIN, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BloomCompositionE::SOURCE_BLOOM_HALF, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BloomCompositionE::SOURCE_BLOOM_QES, 1, rs_layout::Type::SRV_RANGE},
			rs_layout::Entry{(int)BloomCompositionE::OUTPUT, 1, rs_layout::Type::UAV_RANGE},
			rs_layout::Entry{(int)BloomCompositionE::BLOOM_PROPERTIES, 1, rs_layout::Type::CBV_OR_CONST},
		};

	} /* srv */

	struct root_signatures
	{
		WISPRENDERER_EXPORT static RegistryHandle brdf_lut;
		WISPRENDERER_EXPORT static RegistryHandle basic;
		WISPRENDERER_EXPORT static RegistryHandle svgf_denoiser;
		WISPRENDERER_EXPORT static RegistryHandle deferred_composition;
		WISPRENDERER_EXPORT static RegistryHandle rt_test_global;
		WISPRENDERER_EXPORT static RegistryHandle mip_mapping;
		WISPRENDERER_EXPORT static RegistryHandle rt_hybrid_global;
		WISPRENDERER_EXPORT static RegistryHandle rt_ao_global;
		WISPRENDERER_EXPORT static RegistryHandle path_tracing_global;
		WISPRENDERER_EXPORT static RegistryHandle cubemap_conversion;
		WISPRENDERER_EXPORT static RegistryHandle cubemap_convolution;
		WISPRENDERER_EXPORT static RegistryHandle cubemap_prefiltering;
		WISPRENDERER_EXPORT static RegistryHandle post_processing;
		WISPRENDERER_EXPORT static RegistryHandle accumulation;
		WISPRENDERER_EXPORT static RegistryHandle dof_coc;
		WISPRENDERER_EXPORT static RegistryHandle dof_near_mask;
		WISPRENDERER_EXPORT static RegistryHandle down_scale;
		WISPRENDERER_EXPORT static RegistryHandle dof_dilate;
		WISPRENDERER_EXPORT static RegistryHandle dof_bokeh;
		WISPRENDERER_EXPORT static RegistryHandle dof_bokeh_post_filter;
		WISPRENDERER_EXPORT static RegistryHandle dof_composition;
		WISPRENDERER_EXPORT static RegistryHandle bloom_extract_bright;
		WISPRENDERER_EXPORT static RegistryHandle bloom_blur;
		WISPRENDERER_EXPORT static RegistryHandle bloom_blur_horizontal;
		WISPRENDERER_EXPORT static RegistryHandle bloom_blur_vertical;
		WISPRENDERER_EXPORT static RegistryHandle bloom_composition;
		WISPRENDERER_EXPORT static RegistryHandle spatial_reconstruction;
    WISPRENDERER_EXPORT static RegistryHandle reflection_denoiser;
	};

	struct shaders
	{
		WISPRENDERER_EXPORT static RegistryHandle brdf_lut_cs;
		WISPRENDERER_EXPORT static RegistryHandle basic_deferred_vs;
		WISPRENDERER_EXPORT static RegistryHandle basic_deferred_ps;
		WISPRENDERER_EXPORT static RegistryHandle basic_hybrid_vs;
		WISPRENDERER_EXPORT static RegistryHandle basic_hybrid_ps;
		WISPRENDERER_EXPORT static RegistryHandle fullscreen_quad_vs;
		WISPRENDERER_EXPORT static RegistryHandle svgf_denoiser_reprojection_cs;
		WISPRENDERER_EXPORT static RegistryHandle svgf_denoiser_filter_moments_cs;
		WISPRENDERER_EXPORT static RegistryHandle svgf_denoiser_wavelet_filter_cs;
		WISPRENDERER_EXPORT static RegistryHandle deferred_composition_cs;
		WISPRENDERER_EXPORT static RegistryHandle rt_lib;
		WISPRENDERER_EXPORT static RegistryHandle rt_ao_lib;
		WISPRENDERER_EXPORT static RegistryHandle path_tracer_lib;
		WISPRENDERER_EXPORT static RegistryHandle rt_shadow_lib;
		WISPRENDERER_EXPORT static RegistryHandle rt_reflection_lib;
		WISPRENDERER_EXPORT static RegistryHandle mip_mapping_cs;
		WISPRENDERER_EXPORT static RegistryHandle equirect_to_cubemap_vs;
		WISPRENDERER_EXPORT static RegistryHandle equirect_to_cubemap_ps;
		WISPRENDERER_EXPORT static RegistryHandle cubemap_convolution_ps;
		WISPRENDERER_EXPORT static RegistryHandle cubemap_prefiltering_cs;
		WISPRENDERER_EXPORT static RegistryHandle post_processing;
		WISPRENDERER_EXPORT static RegistryHandle accumulation;
		WISPRENDERER_EXPORT static RegistryHandle dof_coc;
		WISPRENDERER_EXPORT static RegistryHandle dof_near_mask;
		WISPRENDERER_EXPORT static RegistryHandle down_scale;
		WISPRENDERER_EXPORT static RegistryHandle dof_dilate;
		WISPRENDERER_EXPORT static RegistryHandle dof_bokeh;
		WISPRENDERER_EXPORT static RegistryHandle dof_bokeh_post_filter;
		WISPRENDERER_EXPORT static RegistryHandle dof_composition;
		WISPRENDERER_EXPORT static RegistryHandle bloom_extract_bright;
		WISPRENDERER_EXPORT static RegistryHandle bloom_blur;
		WISPRENDERER_EXPORT static RegistryHandle bloom_blur_horizontal;
		WISPRENDERER_EXPORT static RegistryHandle bloom_blur_vertical;
		WISPRENDERER_EXPORT static RegistryHandle bloom_composition;
		WISPRENDERER_EXPORT static RegistryHandle spatial_reconstruction;
		WISPRENDERER_EXPORT static RegistryHandle reflection_temporal_denoiser;
		WISPRENDERER_EXPORT static RegistryHandle reflection_variance_estimator;
		WISPRENDERER_EXPORT static RegistryHandle reflection_spatial_denoiser;
	};

	struct pipelines
	{
		WISPRENDERER_EXPORT static RegistryHandle brdf_lut_precalculation;
		WISPRENDERER_EXPORT static RegistryHandle basic_deferred;
		WISPRENDERER_EXPORT static RegistryHandle basic_hybrid;
		WISPRENDERER_EXPORT static RegistryHandle svgf_denoiser_reprojection;
		WISPRENDERER_EXPORT static RegistryHandle svgf_denoiser_filter_moments;
		WISPRENDERER_EXPORT static RegistryHandle svgf_denoiser_wavelet_filter;
		WISPRENDERER_EXPORT static RegistryHandle deferred_composition;
		WISPRENDERER_EXPORT static RegistryHandle mip_mapping;
		WISPRENDERER_EXPORT static RegistryHandle equirect_to_cubemap;
		WISPRENDERER_EXPORT static RegistryHandle cubemap_convolution;
		WISPRENDERER_EXPORT static RegistryHandle cubemap_prefiltering;
		WISPRENDERER_EXPORT static RegistryHandle post_processing;
		WISPRENDERER_EXPORT static RegistryHandle accumulation;
		WISPRENDERER_EXPORT static RegistryHandle dof_coc;
		WISPRENDERER_EXPORT static RegistryHandle dof_near_mask;
		WISPRENDERER_EXPORT static RegistryHandle down_scale;
		WISPRENDERER_EXPORT static RegistryHandle dof_dilate;
		WISPRENDERER_EXPORT static RegistryHandle dof_bokeh;
		WISPRENDERER_EXPORT static RegistryHandle dof_bokeh_post_filter;
		WISPRENDERER_EXPORT static RegistryHandle dof_composition;
		WISPRENDERER_EXPORT static RegistryHandle bloom_extract_bright;
		WISPRENDERER_EXPORT static RegistryHandle bloom_blur;
		WISPRENDERER_EXPORT static RegistryHandle bloom_blur_horizontal;
		WISPRENDERER_EXPORT static RegistryHandle bloom_blur_vertical;
		WISPRENDERER_EXPORT static RegistryHandle bloom_composition;
		WISPRENDERER_EXPORT static RegistryHandle spatial_reconstruction;
		WISPRENDERER_EXPORT static RegistryHandle reflection_temporal_denoiser;
		WISPRENDERER_EXPORT static RegistryHandle reflection_variance_estimator;
		WISPRENDERER_EXPORT static RegistryHandle reflection_spatial_denoiser;
	};

	struct state_objects
	{
		WISPRENDERER_EXPORT static RegistryHandle state_object;
		WISPRENDERER_EXPORT static RegistryHandle rt_ao_state_opbject;
		WISPRENDERER_EXPORT static RegistryHandle path_tracing_state_object;
		WISPRENDERER_EXPORT static RegistryHandle path_tracer_state_object;
		WISPRENDERER_EXPORT static RegistryHandle rt_shadow_state_object;
		WISPRENDERER_EXPORT static RegistryHandle rt_reflection_state_object;
	};

} /* wr */

