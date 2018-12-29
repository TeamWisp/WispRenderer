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
	}

#ifndef USE_UWP
	RenderWindow* CreateRenderWindow(Device* device, HWND window, CommandQueue* cmd_queue, unsigned int num_back_buffers)
	{
		auto render_window = new RenderWindow();
		render_window->m_num_render_targets = num_back_buffers;

		// Get client area for the swap chain size
		RECT r;
		GetClientRect(window, &r);
		unsigned int width = static_cast<decltype(width)>(r.right - r.left);
		unsigned int height = static_cast<decltype(height)>(r.bottom - r.top);

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

		render_window->m_render_targets.resize(num_back_buffers);
		for (decltype(num_back_buffers) i = 0; i < num_back_buffers; i++)
		{
			TRY_M(render_window->m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_window->m_render_targets[i])),
				"Failed to get swap chain buffer.");
		}

		CreateRenderTargetViews(render_window, device, width, height);
		CreateDepthStencilBuffer(render_window, device, width, height);

		return render_window;
	}

#else

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

		CreateRenderTargetViews(render_window, device, width, height);
		CreateDepthStencilBuffer(render_window, device, width, height);

		return render_window;
	}
#endif

	void Resize(RenderWindow* render_window, Device* device, unsigned int width, unsigned int height, bool fullscreen)
	{
		DestroyDepthStencilBuffer(render_window);
		DestroyRenderTargetViews(render_window);

		render_window->m_swap_chain->ResizeBuffers(render_window->m_num_render_targets, width, height, DXGI_FORMAT_UNKNOWN, 0);

		render_window->m_render_targets.resize(render_window->m_num_render_targets);
		for (auto i = 0; i < render_window->m_num_render_targets; i++)
		{
			TRY_M(render_window->m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_window->m_render_targets[i])),
				"Failed to get swap chain buffer.");
		}

		CreateRenderTargetViews(render_window, device, width, height);
		CreateDepthStencilBuffer(render_window, device, width, height);
	}

	void Present(RenderWindow* render_window, Device* device)
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