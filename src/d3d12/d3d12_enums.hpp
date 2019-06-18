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

#include <dxgi1_5.h>
#include <d3d12.h>

namespace wr
{

	enum class RaytracingType
	{
		FALLBACK,
		NATIVE,
		NONE,
	};

	enum class PipelineType
	{
		GRAPHICS_PIPELINE,
		COMPUTE_PIPELINE,
	};

	enum class ShaderType
	{
		VERTEX_SHADER,
		PIXEL_SHADER,
		DOMAIN_SHADER,
		GEOMETRY_SHADER,
		HULL_SHADER,
		DIRECT_COMPUTE_SHADER,
		LIBRARY_SHADER,
	};

	enum class CmdListType
	{
		CMD_LIST_DIRECT = (int)D3D12_COMMAND_LIST_TYPE_DIRECT,
		CMD_LIST_COMPUTE = (int)D3D12_COMMAND_LIST_TYPE_COMPUTE,
		CMD_LIST_COPY = (int)D3D12_COMMAND_LIST_TYPE_COPY,
		CMD_LIST_BUNDLE = (int)D3D12_COMMAND_LIST_TYPE_BUNDLE,
	};

	enum class StateObjType
	{
		RAYTRACING_PIPELINE = (int)D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		RAYTRACING, D3D12_STATE_OBJECT_TYPE_RAYTRACING,
		COLLECTION, D3D12_STATE_OBJECT_TYPE_COLLECTION,
	};

	enum HeapOptimization
	{
		SMALL_BUFFERS = 0,
		SMALL_STATIC_BUFFERS = 1,
		BIG_BUFFERS = 2,
		BIG_STATIC_BUFFERS = 3,
	};

	enum class HeapType
	{
		HEAP_DEFAULT = (int)D3D12_HEAP_TYPE_DEFAULT,
		HEAP_READBACK = (int)D3D12_HEAP_TYPE_READBACK,
		HEAP_UPLOAD = (int)D3D12_HEAP_TYPE_UPLOAD,
		HEAP_CUSTOM = (int)D3D12_HEAP_TYPE_CUSTOM,
	};

	enum class ResourceType
	{
		BUFFER,
		TEXTURE,
		RT_DS,
	};

	enum class TopologyType
	{
		TRIANGLE = (int)D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		PATCH = (int)D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH,
		POINT = (int)D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
		LINE = (int)D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
	};

	enum class CullMode
	{
		CULL_FRONT = (int)D3D12_CULL_MODE_FRONT,
		CULL_BACK = (int)D3D12_CULL_MODE_BACK,
		CULL_NONE = (int)D3D12_CULL_MODE_NONE,
	};

	enum class TextureFilter
	{
		FILTER_LINEAR = (int)D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		FILTER_POINT = (int)D3D12_FILTER_MIN_MAG_MIP_POINT,
		FILTER_LINEAR_POINT = (int)D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
		FILTER_ANISOTROPIC = (INT)D3D12_FILTER_ANISOTROPIC,
	};

	enum class SRVDimension
	{
		DIMENSION_BUFFER = (int)D3D12_SRV_DIMENSION_BUFFER,
		DIMENSION_TEXTURE1D = (int)D3D12_SRV_DIMENSION_TEXTURE1D,
		DIMENSION_TEXTURE1DARRAY = (int)D3D12_SRV_DIMENSION_TEXTURE1DARRAY,
		DIMENSION_TEXTURE2D = (int)D3D12_SRV_DIMENSION_TEXTURE2D,
		DIMENSION_TEXTURE2DARRAY = (int)D3D12_SRV_DIMENSION_TEXTURE2DARRAY,
		DIMENSION_TEXTURE2DMS = (int)D3D12_SRV_DIMENSION_TEXTURE2DMS,
		DIMENSION_TEXTURE2DMSARRAY = (int)D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY,
		DIMENSION_TEXTURE3D = (int)D3D12_SRV_DIMENSION_TEXTURE3D,
		DIMENSION_TEXTURECUBE = (int)D3D12_SRV_DIMENSION_TEXTURECUBE,
		DIMENSION_TEXTURECUBEARRAY = (int)D3D12_SRV_DIMENSION_TEXTURECUBEARRAY,
	};

	enum class UAVDimension
	{
		DIMENSION_BUFFER = (int)D3D12_UAV_DIMENSION_BUFFER,
		DIMENSION_TEXTURE1D = (int)D3D12_UAV_DIMENSION_TEXTURE1D,
		DIMENSION_TEXTURE1DARRAY = (int)D3D12_UAV_DIMENSION_TEXTURE1DARRAY,
		DIMENSION_TEXTURE2D = (int)D3D12_UAV_DIMENSION_TEXTURE2D,
		DIMENSION_TEXTURE2DARRAY = (int)D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
		DIMENSION_TEXTURE3D = (int)D3D12_UAV_DIMENSION_TEXTURE3D,
	};

	enum class TextureAddressMode
	{
		TAM_MIRROR_ONCE = (int)D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE,
		TAM_MIRROR = (int)D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
		TAM_CLAMP = (int)D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		TAM_BORDER = (int)D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		TAM_WRAP = (int)D3D12_TEXTURE_ADDRESS_MODE_WRAP,
	};

	enum class BorderColor
	{
		BORDER_TRANSPARENT = (int)D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
		BORDER_BLACK = (int)D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
		BORDER_WHITE = (int)D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
	};

	enum class DescriptorHeapType
	{
		DESC_HEAP_TYPE_CBV_SRV_UAV = (int)D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		DESC_HEAP_TYPE_SAMPLER = (int)D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		DESC_HEAP_TYPE_RTV = (int)D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		DESC_HEAP_TYPE_DSV = (int)D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		DESC_HEAP_TYPE_NUM_TYPES = (int)D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES,
	};

	enum class ResourceState
	{
		VERTEX_AND_CONSTANT_BUFFER = (int)D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		INDEX_BUFFER = (int)D3D12_RESOURCE_STATE_INDEX_BUFFER,
		COMMON = (int)D3D12_RESOURCE_STATE_COMMON,
		PRESENT = (int)D3D12_RESOURCE_STATE_PRESENT,
		RENDER_TARGET = (int)D3D12_RESOURCE_STATE_RENDER_TARGET,
		PIXEL_SHADER_RESOURCE = (int)D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		NON_PIXEL_SHADER_RESOURCE = (int)D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		UNORDERED_ACCESS = (int)D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		COPY_SOURCE = (int)D3D12_RESOURCE_STATE_COPY_SOURCE,
		COPY_DEST = (int)D3D12_RESOURCE_STATE_COPY_DEST,
		DEPTH_WRITE = (int)D3D12_RESOURCE_STATE_DEPTH_WRITE,
		DEPTH_READ = (int)D3D12_RESOURCE_STATE_DEPTH_READ,
		INDIRECT_ARGUMENT = (int)D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
	};

	enum class BufferUsageFlag
	{
		INDEX_BUFFER = (int)D3D12_RESOURCE_STATE_INDEX_BUFFER,
		VERTEX_BUFFER = (int)D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
	};

	enum class Format
	{
		UNKNOWN = (int)DXGI_FORMAT_UNKNOWN,
		R10G10B10A2_UNORM = (int)DXGI_FORMAT_R10G10B10A2_UNORM,
		R10G10B10A2_UINT = (int)DXGI_FORMAT_R10G10B10A2_UINT,
		R32G32B32A32_FLOAT = (int)DXGI_FORMAT_R32G32B32A32_FLOAT,
		R32G32B32A32_UINT = (int)DXGI_FORMAT_R32G32B32A32_UINT,
		R32G32B32A32_SINT = (int)DXGI_FORMAT_R32G32B32A32_SINT,
		R32G32B32_FLOAT = (int)DXGI_FORMAT_R32G32B32_FLOAT,
		R32G32B32_UINT = (int)DXGI_FORMAT_R32G32B32_UINT,
		R32G32B32_SINT = (int)DXGI_FORMAT_R32G32B32_SINT,
		R16G16B16A16_FLOAT = (int)DXGI_FORMAT_R16G16B16A16_FLOAT,
		R16G16B16A16_UINT = (int)DXGI_FORMAT_R16G16B16A16_UINT,
		R16G16B16A16_SINT = (int)DXGI_FORMAT_R16G16B16A16_SINT,
		R16G16B16A16_UNORM = (int)DXGI_FORMAT_R16G16B16A16_UNORM,
		R16G16B16A16_SNORM = (int)DXGI_FORMAT_R16G16B16A16_SNORM,
		R32G32_FLOAT = (int)DXGI_FORMAT_R32G32_FLOAT,
		R32G32_UINT = (int)DXGI_FORMAT_R32G32_UINT,
		R32G32_SINT = (int)DXGI_FORMAT_R32G32_SINT,
		//R10G10B10_UNORM = (int)DXGI_FORMAT_R10G10B10_UNORM,
		//R10G10B10_UINT = (int)vk::Format::eA2R10G10B10UintPack32, //FIXME: Their are more vulcan variants?
		R11G11B10_FLOAT = (int)DXGI_FORMAT_R11G11B10_FLOAT,
		R8G8B8A8_UNORM = (int)DXGI_FORMAT_R8G8B8A8_UNORM,
		R8G8B8A8_UNORM_SRGB = (int)DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		R8G8B8A8_SNORM = (int)DXGI_FORMAT_R8G8B8A8_SNORM,
		R8G8B8A8_UINT = (int)DXGI_FORMAT_R8G8B8A8_UINT,
		R8G8B8A8_SINT = (int)DXGI_FORMAT_R8G8B8A8_SINT,
		R16G16_FLOAT = (int)DXGI_FORMAT_R16G16_FLOAT,
		R16G16_UINT = (int)DXGI_FORMAT_R16G16_UINT,
		R16G16_UNORM = (int)DXGI_FORMAT_R16G16_UNORM,
		R16G16_SNORM = (int)DXGI_FORMAT_R16G16_SNORM,
		R16G16_SINT = (int)DXGI_FORMAT_R16G16_SINT,
		D32_FLOAT = (int)DXGI_FORMAT_D32_FLOAT,
		R32_UINT = (int)DXGI_FORMAT_R32_UINT,
		R32_TYPELESS = (int)DXGI_FORMAT_R32_TYPELESS,
		R32_SINT = (int)DXGI_FORMAT_R32_SINT,
		R32_FLOAT = (int)DXGI_FORMAT_R32_FLOAT,
		R16_UNORM = (int)DXGI_FORMAT_R16_UNORM,
		D24_UNFORM_S8_UINT = (int)DXGI_FORMAT_D24_UNORM_S8_UINT,
		R8G8_UNORM = (int)DXGI_FORMAT_R8G8_UNORM,
		R8G8_UINT = (int)DXGI_FORMAT_R8G8_UINT,
		R8G8_SNORM = (int)DXGI_FORMAT_R8G8_SNORM,
		R8G8_SINT = (int)DXGI_FORMAT_R8G8_SINT,
		R16_FLOAT = (int)DXGI_FORMAT_R16_FLOAT,
		//D16_UNORM = (int)vk::Format::eD16Unorm,
		//R16_UNORM = (int)vk::Format::eR16Unorm,
		R16_UINT = (int)DXGI_FORMAT_R16_UINT,
		R16_SNORM = (int)DXGI_FORMAT_R16_SNORM,
		R16_SINT = (int)DXGI_FORMAT_R16_SINT,
		R8_UNORM = (int)DXGI_FORMAT_R8_UNORM,
		R8_UINT = (int)DXGI_FORMAT_R8_UINT,
		R8_SNORM = (int)DXGI_FORMAT_R8_SNORM,
		R8_SINT = (int)DXGI_FORMAT_R8_SINT,
		A8_UNORM = (int)DXGI_FORMAT_A8_UNORM,
		//BC1_UNORM = (int)vk::Format::eBc1RgbUnormBlock, //FIXME: is this correct?
		//BC1_UNORM_SRGB = (int)vk::Format::eBc1RgbSrgbBlock, //FIXME: is this correct?
		//BC2_UNORM = (int)vk::Format::eBc2UnormBlock,
		//BC2_UNORM_SRGB = (int)vk::Format::eBc2SrgbBlock,
		//BC3_UNORM = (int)vk::Format::eBc3UnormBlock,
		//BC3_UNORM_SRGB = (int)vk::Format::eBc3SrgbBlock,
		//BC4_UNORM = (int)vk::Format::eBc4UnormBlock,
		//BC4_SNORM = (int)vk::Format::eBc4SnormBlock,
		//BC5_UNORM = (int)vk::Format::eBc5UnormBlock,
		//BC5_SNORM = (int)vk::Format::eBc5SnormBlock,
		B5G6R5_UNORM = (int)DXGI_FORMAT_B5G6R5_UNORM,
		B5G5R5A1_UNORM = (int)DXGI_FORMAT_B5G5R5A1_UNORM,
		B8G8R8A8_UNORM = (int)DXGI_FORMAT_B8G8R8A8_UNORM,
		B8G8R8A8_UNORM_SRGB = (int)DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		//B8G8R8A8_SNORM = (int)vk::Format::eB8G8R8A8Snorm,
		//B8G8R8A8_UINT = (int)vk::Format::eB8G8R8A8Uint,
		//B8G8R8A8_SINT = (int)vk::Format::eB8G8R8A8Sint,
		B8G8R8X8_UNORM = (int)DXGI_FORMAT_B8G8R8X8_UNORM,
		B8G8R8X8_UNORM_SRGB = (int)DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
		//BC6H_UF16 = (int)vk::Format::eBc6HUfloatBlock,
		//BC6H_SF16 = (int)vk::Format::eBc6HSfloatBlock,
		//BC7_UNORM = (int)vk::Format::eBc7UnormBlock,
		//BC7_UNORM_SRGB = (int)vk::Format::eBc7SrgbBlock,
		B4G4R4A4_UNORM = (int)DXGI_FORMAT_B4G4R4A4_UNORM,
		D32_FLOAT_S8X24_UINT = (int)DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
	};

	static inline std::string FormatToStr(Format format)
	{
		switch (format)
		{
		case Format::UNKNOWN: return "UNKNOWN";
		case Format::R32G32B32A32_FLOAT: return "R32G32B32A32_FLOAT";
		case Format::R32G32B32A32_UINT: return "R32G32B32A32_UINT";
		case Format::R32G32B32A32_SINT: return "R32G32B32A32_SINT";
		case Format::R32G32B32_FLOAT: return "R32G32B32_FLOAT";
		case Format::R32G32B32_UINT: return "R32G32B32_UINT";
		case Format::R32G32B32_SINT: return "R32G32B32_SINT";
		case Format::R16G16B16A16_FLOAT: return "R16G16B16A16_FLOAT";
		case Format::R16G16B16A16_UINT: return "R16G16B16A16_UINT";
		case Format::R16G16B16A16_SINT: return "R16G16B16A16_SINT";
		case Format::R16G16B16A16_UNORM: return "R16G16B16A16_UNORM";
		case Format::R16G16B16A16_SNORM: return "R16G16B16A16_SNORM";
		case Format::R32G32_FLOAT: return "R32G32_FLOAT";
		case Format::R32G32_UINT: return "R32G32_UINT";
		case Format::R32G32_SINT: return "R32G32_SINT";
		case Format::R8G8B8A8_UNORM: return "R8G8B8A8_UNORM";
		case Format::R8G8B8A8_UNORM_SRGB: return "R8G8B8A8_UNORM_SRGB";
		case Format::R8G8B8A8_SNORM: return "R8G8B8A8_SNORM";
		case Format::R8G8B8A8_UINT: return "R8G8B8A8_UINT";
		case Format::R8G8B8A8_SINT: return "R8G8B8A8_SINT";
		case Format::D32_FLOAT: return "D32_FLOAT";
		case Format::R32_UINT: return "R32_UINT";
		case Format::R32_SINT: return "R32_SINT";
		case Format::R32_FLOAT: return "R32_FLOAT";
		case Format::D24_UNFORM_S8_UINT: return "D24_UNFORM_S8_UINT";
		case Format::R8_UNORM: return "R8_UNORM";
		case Format::R10G10B10A2_UNORM: return "R10G10B10A2_UNORM";
		case Format::R10G10B10A2_UINT: return "R10G10B10A2_UINT";
		case Format::R11G11B10_FLOAT: return "R11G11B10_FLOAT";
		case Format::R16G16_FLOAT: return "R16G16_FLOAT";
		case Format::R16G16_UNORM: return "R16G16_UNORM";
		case Format::R16G16_UINT: return "R16G16_UINT";
		case Format::R16G16_SNORM: return "R16G16_SNORM";
		case Format::R16G16_SINT: return "R16G16_SINT";
		case Format::R8G8_UNORM: return "R8G8_UNORM";
		case Format::R8G8_UINT: return "R8G8_UINT";
		case Format::R8G8_SNORM: return "R8G8_SNORM";
		case Format::R8G8_SINT: return "R8G8_SINT";
		case Format::R16_UNORM: return "R16_UNORM";
		case Format::R16_SNORM: return "R16_SNORM";
		case Format::R8_SNORM: return "R8_SNORM";
		case Format::A8_UNORM: return "A8_UNORM";
		case Format::B5G6R5_UNORM: return "B5G6R5_UNORM";
		case Format::B5G5R5A1_UNORM: return "B5G5R5A1_UNORM";
		case Format::B4G4R4A4_UNORM: return "B4G4R4A4_UNORM";

		default: return "UNKNOWN (DEFAULT SWITCH STATEMENT)";
		}
	}

	// From: https://github.com/Microsoft/DirectXTK/blob/master/Src/LoaderHelpers.h
	static inline unsigned int BitsPerPixel(Format format)
	{
		switch (format)
		{
		//case Format::R32G32B32A32_TYPELESS:
		case Format::R32G32B32A32_FLOAT:
		case Format::R32G32B32A32_UINT:
		case Format::R32G32B32A32_SINT:
			return 128;

		//case Format::R32G32B32_TYPELESS:
		case Format::R32G32B32_FLOAT:
		case Format::R32G32B32_UINT:
		case Format::R32G32B32_SINT:
			return 96;

		//case Format::R16G16B16A16_TYPELESS:
		case Format::R16G16B16A16_FLOAT:
		case Format::R16G16B16A16_UNORM:
		case Format::R16G16B16A16_UINT:
		case Format::R16G16B16A16_SNORM:
		case Format::R16G16B16A16_SINT:
		//case Format::R32G32_TYPELESS:
		case Format::R32G32_FLOAT:
		case Format::R32G32_UINT:
		case Format::R32G32_SINT:
		//case Format::R32G8X24_TYPELESS:
		case Format::D32_FLOAT_S8X24_UINT:
		//case Format::R32_FLOAT_X8X24_TYPELESS:
		//case Format::X32_TYPELESS_G8X24_UINT:
		//case Format::Y416:
		//case Format::Y210:
		//case Format::Y216:
			return 64;

		//case Format::R10G10B10A2_TYPELESS:
		case Format::R10G10B10A2_UNORM:
		case Format::R10G10B10A2_UINT:
		case Format::R11G11B10_FLOAT:
		//case Format::R8G8B8A8_TYPELESS:
		case Format::R8G8B8A8_UNORM:
		case Format::R8G8B8A8_UNORM_SRGB:
		case Format::R8G8B8A8_UINT:
		case Format::R8G8B8A8_SNORM:
		case Format::R8G8B8A8_SINT:
		//case Format::R16G16_TYPELESS:
		case Format::R16G16_FLOAT:
		case Format::R16G16_UNORM:
		case Format::R16G16_UINT:
		case Format::R16G16_SNORM:
		case Format::R16G16_SINT:
		case Format::R32_TYPELESS:
		case Format::D32_FLOAT:
		case Format::R32_FLOAT:
		case Format::R32_UINT:
		case Format::R32_SINT:
		//case Format::R24G8_TYPELESS:
		//case Format::D24_UNORM_S8_UINT:
		//case Format::R24_UNORM_X8_TYPELESS:
		//case Format::X24_TYPELESS_G8_UINT:
		//case Format::R9G9B9E5_SHAREDEXP:
		//case Format::R8G8_B8G8_UNORM:
		//case Format::G8R8_G8B8_UNORM:
		case Format::B8G8R8A8_UNORM:
		case Format::B8G8R8X8_UNORM:
		//case Format::R10G10B10_XR_BIAS_A2_UNORM:
		//case Format::B8G8R8A8_TYPELESS:
		case Format::B8G8R8A8_UNORM_SRGB:
		//case Format::B8G8R8X8_TYPELESS:
		case Format::B8G8R8X8_UNORM_SRGB:
		//case Format::AYUV:
		//case Format::Y410:
		//case Format::YUY2:
			return 32;

		//case Format::P010:
		//case Format::P016:
		//	return 24;

		//case Format::R8G8_TYPELESS:
		case Format::R8G8_UNORM:
		case Format::R8G8_UINT:
		case Format::R8G8_SNORM:
		case Format::R8G8_SINT:
		//case Format::R16_TYPELESS:
		case Format::R16_FLOAT:
		//case Format::D16_UNORM:
		case Format::R16_UNORM:
		case Format::R16_UINT:
		case Format::R16_SNORM:
		case Format::R16_SINT:
		case Format::B5G6R5_UNORM:
		case Format::B5G5R5A1_UNORM:
		//case Format::A8P8:
		case Format::B4G4R4A4_UNORM:
			return 16;

		//case Format::NV12:
		//case Format::420_OPAQUE:
		//case Format::NV11:
		//	return 12;

		//case Format::R8_TYPELESS:
		case Format::R8_UNORM:
		case Format::R8_UINT:
		case Format::R8_SNORM:
		case Format::R8_SINT:
		case Format::A8_UNORM:
		//case Format::AI44:
		//case Format::IA44:
		//case Format::P8:
			return 8;

		//case Format::R1_UNORM:
		//	return 1;

		//case Format::BC1_TYPELESS:
		//case Format::BC1_UNORM:
		//case Format::BC1_UNORM_SRGB:
		//case Format::BC4_TYPELESS:
		//case Format::BC4_UNORM:
		//case Format::BC4_SNORM:
		//	return 4;

		//case Format::BC2_TYPELESS:
		//case Format::BC2_UNORM:
		//case Format::BC2_UNORM_SRGB:
		//case Format::BC3_TYPELESS:
		//case Format::BC3_UNORM:
		//case Format::BC3_UNORM_SRGB:
		//case Format::BC5_TYPELESS:
		//case Format::BC5_UNORM:
		//case Format::BC5_SNORM:
		//case Format::BC6H_TYPELESS:
		//case Format::BC6H_UF16:
		//case Format::BC6H_SF16:
		//case Format::BC7_TYPELESS:
		//case Format::BC7_UNORM:
		//case Format::BC7_UNORM_SRGB:
		//	return 8;

//#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
//
//		case Format::V408:
//			return 24;
//
//		case Format::P208:
//		case Format::V208:
//			return 16;
//
//#endif // (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
//
//#if defined(_XBOX_ONE) && defined(_TITLE)
//
//		case Format::R10G10B10_7E3_A2_FLOAT:
//		case Format::R10G10B10_6E4_A2_FLOAT:
//		case Format::R10G10B10_SNORM_A2_UNORM:
//			return 32;
//
//		case Format::D16_UNORM_S8_UINT:
//		case Format::R16_UNORM_X8_TYPELESS:
//		case Format::X16_TYPELESS_G8_UINT:
//			return 24;
//
//		case Format::R4G4_UNORM:
//			return 8;
//
//#endif // _XBOX_ONE && _TITLE

		default:
			return 0;
		}
	}

	static inline unsigned int BytesPerPixel(Format format)
	{
		return BitsPerPixel(format) / 8;
	}

} /* wr */