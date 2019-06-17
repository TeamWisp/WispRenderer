// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "d3d12_functions.hpp"

#include "../util/log.hpp"
#include "d3d12_defines.hpp"

namespace wr::d3d12
{

	namespace internal
	{
		inline DXGI_SWAP_CHAIN_DESC1 GetSwapChainDesc(unsigned int width, unsigned int height, unsigned int num_back_buffers)
		{
			DXGI_SAMPLE_DESC sample_desc = {};
			sample_desc.Count = 1;
			sample_desc.Quality = 0;

			DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
			swap_chain_desc.Width = width;
			swap_chain_desc.Height = height;
			swap_chain_desc.Format = (DXGI_FORMAT)settings::back_buffer_format;
			swap_chain_desc.SampleDesc = sample_desc;
			swap_chain_desc.BufferCount = num_back_buffers;
			swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swap_chain_desc.SwapEffect = settings::flip_mode;
			swap_chain_desc.AlphaMode = settings::swapchain_alpha_mode;
			swap_chain_desc.Flags = settings::swapchain_flags;
			swap_chain_desc.Scaling = settings::swapchain_scaling;

			return swap_chain_desc;
		}

		inline void EnsureSwapchainColorSpace(RenderWindow* render_window, std::uint32_t swapchain_bit_depth, bool enable_st2084)
		{
			DXGI_COLOR_SPACE_TYPE color_space = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

			switch (swapchain_bit_depth)
			{
			default:
				break;

			case 10:
				color_space = enable_st2084 ? DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
				break;

			case 16:
				color_space = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
				break;
			}

			UINT support = 0;
			if (SUCCEEDED(render_window->m_swap_chain->CheckColorSpaceSupport(color_space, &support)) &&
				((support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) == DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
			{
				render_window->m_swap_chain->SetColorSpace1(color_space);
			}
		}

		inline void SetHDRMetaData(RenderWindow* render_window, std::uint32_t swapchain_bit_depth,
			float max_output_nits,
			float min_output_nits,
			float max_cll,
			float max_fall)
		{
			struct DisplayChromacities
			{
				float m_red_x;
				float m_red_y;
				float m_green_x;
				float m_green_y;
				float m_blue_x;
				float m_blue_y;
				float m_white_x;
				float m_white_y;
			};

			static const DisplayChromacities chromaticity[] =
			{
				{ 0.64000f, 0.33000f, 0.30000f, 0.60000f, 0.15000f, 0.06000f, 0.31270f, 0.32900f }, // Display Gamut Rec709 
				{ 0.70800f, 0.29200f, 0.17000f, 0.79700f, 0.13100f, 0.04600f, 0.31270f, 0.32900f }, // Display Gamut Rec2020
			};

			// Select the chromaticity based on HDR format of the DWM.
			int selected_chroma = 0;
			if (swapchain_bit_depth == 16)
			{
				selected_chroma = 0;
			}
			else if (swapchain_bit_depth == 10)
			{
				selected_chroma = 1;
			}
			else
			{
				// Reset the metadata since this is not a supported HDR format.
				TRY_M(render_window->m_swap_chain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_NONE, 0, nullptr), "Failed to reset hdr meta data");
				return;
			}

			// Set HDR meta data
			const DisplayChromacities& chroma = chromaticity[selected_chroma];
			DXGI_HDR_METADATA_HDR10 hdr10_metadata = {};
			hdr10_metadata.RedPrimary[0] = static_cast<std::uint16_t>(chroma.m_red_x * 50000.0f);
			hdr10_metadata.RedPrimary[1] = static_cast<std::uint16_t>(chroma.m_red_y * 50000.0f);
			hdr10_metadata.GreenPrimary[0] = static_cast<std::uint16_t>(chroma.m_green_x * 50000.0f);
			hdr10_metadata.GreenPrimary[1] = static_cast<std::uint16_t>(chroma.m_green_y * 50000.0f);
			hdr10_metadata.BluePrimary[0] = static_cast<std::uint16_t>(chroma.m_blue_x * 50000.0f);
			hdr10_metadata.BluePrimary[1] = static_cast<std::uint16_t>(chroma.m_blue_y * 50000.0f);
			hdr10_metadata.WhitePoint[0] = static_cast<std::uint16_t>(chroma.m_white_x * 50000.0f);
			hdr10_metadata.WhitePoint[1] = static_cast<std::uint16_t>(chroma.m_white_y * 50000.0f);
			hdr10_metadata.MaxMasteringLuminance = static_cast<std::uint32_t>(max_output_nits * 10000.0f);
			hdr10_metadata.MinMasteringLuminance = static_cast<std::uint32_t>(min_output_nits * 10000.0f);
			hdr10_metadata.MaxContentLightLevel = static_cast<std::uint16_t>(max_cll);
			hdr10_metadata.MaxFrameAverageLightLevel = static_cast<std::uint16_t>(max_fall);
			TRY_M(render_window->m_swap_chain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(DXGI_HDR_METADATA_HDR10), &hdr10_metadata), "Failed to set hdr meta data");
		}
	}

	RenderWindow* CreateRenderWindow(Device* device, HWND window, CommandQueue* cmd_queue, unsigned int num_back_buffers)
	{
		auto render_window = new RenderWindow();
		render_window->m_num_render_targets = num_back_buffers;

		// Get client area for the swap chain size
		RECT r;
		GetClientRect(window, &r);
		unsigned int width = static_cast<decltype(width)>(r.right - r.left);
		unsigned int height = static_cast<decltype(height)>(r.bottom - r.top);

		render_window->m_width = width;
		render_window->m_height = height;

		const auto swap_chain_desc = internal::GetSwapChainDesc(width, height, num_back_buffers);

		IDXGISwapChain1* temp_swap_chain;
		TRY_M(device->m_dxgi_factory->CreateSwapChainForHwnd(
			cmd_queue->m_native,
			window,
			&swap_chain_desc,
			NULL,
			NULL,
			&temp_swap_chain
		), "Failed to create swap chain for HWND.");

		render_window->m_swap_chain = static_cast<IDXGISwapChain4*>(temp_swap_chain);
		render_window->m_frame_idx = (render_window->m_swap_chain)->GetCurrentBackBufferIndex();

		const float meta_data_args_pool[4][4] =
		{
			// MaxOutputNits, MinOutputNits, MaxCLL, MaxFALL
			// These values are made up for testing. You need to figure out those numbers for your app.
			{ 1000.0f, 0.001f, 2000.0f, 500.0f },
			{ 500.0f, 0.001f, 2000.0f, 500.0f },
			{ 500.0f, 0.100f, 500.0f, 100.0f },
			{ 2000.0f, 1.000f, 2000.0f, 1000.0f }
		};
		int meta_data_args_idx = 0;

		if (d3d12::settings::output_hdr)
		{
			internal::EnsureSwapchainColorSpace(render_window, 16, true);
			internal::SetHDRMetaData(render_window, 16,
				meta_data_args_pool[meta_data_args_idx][0],
				meta_data_args_pool[meta_data_args_idx][1],
				meta_data_args_pool[meta_data_args_idx][2],
				meta_data_args_pool[meta_data_args_idx][3]);
		}

		render_window->m_render_targets.resize(num_back_buffers);
		for (decltype(num_back_buffers) i = 0; i < num_back_buffers; i++)
		{
			TRY_M(render_window->m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_window->m_render_targets[i])),
				"Failed to get swap chain buffer.");
		}

		CreateRenderTargetViews(render_window, device);
		CreateDepthStencilBuffer(render_window, device, width, height);

		return render_window;
	}

	RenderWindow* CreateRenderWindow(Device* device, IUnknown* window, CommandQueue* cmd_queue, unsigned int num_back_buffers)
	{
		auto render_window = new RenderWindow();
		render_window->m_num_render_targets = num_back_buffers;

		unsigned int width = 100;
		unsigned int height = 100; // TODO: Get the actual size.
		auto swap_chain_desc = internal::GetSwapChainDesc(width, height, num_back_buffers);

		IDXGISwapChain1* temp_swap_chain;
		TRY_M(device->m_dxgi_factory->CreateSwapChainForCoreWindow(
			cmd_queue->m_native,
			window,
			&swap_chain_desc,
			NULL,
			&temp_swap_chain
		), "Failed to create swap chain for Core Window (UWP)");

		render_window->m_swap_chain = static_cast<IDXGISwapChain4*>(temp_swap_chain);
		render_window->m_frame_idx = (render_window->m_swap_chain)->GetCurrentBackBufferIndex();

		render_window->m_swap_chain->SetMaximumFrameLatency(num_back_buffers);

		render_window->m_render_targets.resize(num_back_buffers);
		for (decltype(num_back_buffers) i = 0; i < num_back_buffers; i++)
		{
			TRY_M(render_window->m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_window->m_render_targets[i])),
				"Failed to get swap chain buffer.");
		}

		CreateRenderTargetViews(render_window, device);
		CreateDepthStencilBuffer(render_window, device, width, height);

		return render_window;
	}

	void Resize(RenderWindow* render_window, Device* device, unsigned int width, unsigned int height)
	{
		DestroyDepthStencilBuffer(render_window);
		DestroyRenderTargetViews(render_window);

		render_window->m_width = width;
		render_window->m_height = height;
		render_window->m_swap_chain->ResizeBuffers(render_window->m_num_render_targets, width, height, DXGI_FORMAT_UNKNOWN, 0);

		render_window->m_render_targets.resize(render_window->m_num_render_targets);
		for (auto i = 0u; i < render_window->m_num_render_targets; i++)
		{
			TRY_M(render_window->m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_window->m_render_targets[i])),
				"Failed to get swap chain buffer.");
		}

		CreateRenderTargetViews(render_window, device);
		CreateDepthStencilBuffer(render_window, device, width, height);
	}

	void Present(RenderWindow* render_window)
	{
		render_window->m_swap_chain->Present(0, 0);
		render_window->m_frame_idx = render_window->m_swap_chain->GetCurrentBackBufferIndex();
	}

	void Destroy(RenderWindow* render_window)
	{
		DestroyDepthStencilBuffer(render_window);
		render_window->m_rtv_descriptor_heap->Release();
		render_window->m_frame_idx = 0;

		render_window->m_swap_chain->Release();
	}

} /* wr::d3d12 */