#include "d3d12_functions.hpp"

#include <DXGIDebug.h>

#include "../util/log.hpp"
#include "d3d12_defines.hpp"

namespace d3d12
{

	namespace internal
	{
		void EnableDebugLayer(Device* device)
		{
			if (!(settings::enable_debug_layer & settings::DebugLayer::DISABLE)) // If the debug layer isn't disabled
			{
				// Setup debug layers
				if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&device->m_debug_controller))))
				{
					if (settings::enable_debug_layer & settings::DebugLayer::ENABLE_WITH_GPU_VALIDATION) // If GPU validation is requested.
					{
						device->m_debug_controller->SetEnableSynchronizedCommandQueueValidation(true);
						device->m_debug_controller->SetEnableGPUBasedValidation(true);
					}
					device->m_debug_controller->EnableDebugLayer();
				}
			}
		}

		void EnableGpuErrorBreaking(Device* device)
		{
#ifdef _DEBUG
			// Set error behaviour
			if (SUCCEEDED(device->m_native->QueryInterface(IID_PPV_ARGS(&device->m_info_queue))))
			{
				device->m_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
				device->m_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			}
#endif
		}

		void GetSysInfo(Device* device)
		{
			DXGI_ADAPTER_DESC3 desc;
			GetNativeSystemInfo(&device->m_sys_info);
			device->m_adapter->GetDesc3(&desc);
		}

		void CreateFactory(Device* device)
		{
			TRY_M(CreateDXGIFactory2(settings::enable_debug_factory ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&device->m_dxgi_factory)),
				"Failed to create DXGIFactory.");
		}

		void FindAdapter(Device* device)
		{
			IDXGIAdapter4* adapter = nullptr;
			int adapter_idx = 0;

			// Find a compatible adapter.
			while (device->m_dxgi_factory->EnumAdapterByGpuPreference(adapter_idx, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				// Skip software adapters.
				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					adapter_idx++;
					continue;
				}

				// Create a device to test if the adapter supports the specified feature level.
				HRESULT hr = D3D12CreateDevice(adapter, settings::feature_level, _uuidof(ID3D12Device), nullptr);
				if (SUCCEEDED(hr))
				{
					break;
				}

				adapter_idx++;
			}

			if (adapter == nullptr)
			{
				device->m_dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));

				TRY_M(D3D12CreateDevice(adapter, settings::feature_level, _uuidof(ID3D12Device), nullptr),
					"Failed to create warp adapter.");
			}

			if (adapter == nullptr)
			{
				LOGC("Failed to find hardware adapter or create warp adapter.");
			}

			device->m_adapter = adapter;
		}
	}

	Device* CreateDevice()
	{
		auto device = new Device();

		internal::EnableDebugLayer(device);
		internal::CreateFactory(device);
		internal::FindAdapter(device);

		TRY_M(D3D12CreateDevice(device->m_adapter, settings::feature_level, IID_PPV_ARGS(&device->m_native)),
			"Failed to create D3D12Device.");

		internal::EnableGpuErrorBreaking(device);
		internal::GetSysInfo(device);

		return device;
	}

	void Destroy(Device* device)
	{
		SAFE_RELEASE(device->m_adapter);
		SAFE_RELEASE(device->m_native);
		SAFE_RELEASE(device->m_dxgi_factory);
		SAFE_RELEASE(device->m_debug_controller);
		SAFE_RELEASE(device->m_info_queue);
		delete device;
	}

} /* d3d12 */