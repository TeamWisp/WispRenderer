#include "d3d12_functions.hpp"

#include <DXGIDebug.h>
#include <functional>

#include "../util/log.hpp"
#include "d3d12_defines.hpp"

namespace wr::d3d12
{

	namespace internal
	{
		void EnableDebugLayer(Device* device)
		{
			if (!(settings::enable_debug_layer == settings::DebugLayer::DISABLE)) // If the debug layer isn't disabled
			{
				// Setup debug layers
				ID3D12Debug* temp_debug_controller;
				if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&temp_debug_controller))) && SUCCEEDED(temp_debug_controller->QueryInterface(IID_PPV_ARGS(&device->m_debug_controller))))
				{
					if (settings::enable_debug_layer == settings::DebugLayer::ENABLE_WITH_GPU_VALIDATION) // If GPU validation is requested.
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
			GetNativeSystemInfo(&device->m_sys_info);
			device->m_adapter->GetDesc3(&device->m_adapter_info);
		}

		void CreateFactory(Device* device)
		{
			TRY_M(CreateDXGIFactory2(settings::enable_debug_factory ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&device->m_dxgi_factory)),
				"Failed to create DXGIFactory.");
		}

		void FindAdapter(Device* device)
		{
			IDXGIAdapter1* adapter = nullptr;
			int adapter_idx = 0;

			// Find a compatible adapter.
			//while (device->m_dxgi_factory->EnumAdapterByGpuPreference(adapter_idx, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND)
			while ((device->m_dxgi_factory)->EnumAdapters1(adapter_idx, &adapter) != DXGI_ERROR_NOT_FOUND)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				// Skip software adapters.
				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					adapter_idx++;
					continue;
				}

				std::function<bool(int)> recursive_dc = [&](int i) -> bool
				{
					// Create a device to test if the adapter supports the specified feature level.
					HRESULT hr = D3D12CreateDevice(adapter, settings::possible_feature_levels[i], _uuidof(ID3D12Device), nullptr);
					if (SUCCEEDED(hr))
					{
						device->m_feature_level = settings::possible_feature_levels[i];
						return true;
					}

					if (i + 1 >= settings::possible_feature_levels.size())
					{
						return false;
					}
					else
					{
						i++;
						recursive_dc(i);
					}

					return true;
				};

				if (recursive_dc(0))
				{
					break;
				}

				adapter_idx++;
			}

			if (adapter == nullptr)
			{
				device->m_dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));

				LOGW("Using Warp Adapter!");

				device->m_feature_level = settings::possible_feature_levels[0];
				TRY_M(D3D12CreateDevice(adapter, settings::possible_feature_levels[0], _uuidof(ID3D12Device), nullptr),
					"Failed to create warp adapter.");
			}

			if (adapter == nullptr)
			{
				LOGC("Failed to find hardware adapter or create warp adapter.");
			}

			device->m_adapter = (IDXGIAdapter4*)adapter;
		}

		// Returns bool whether the device supports DirectX Raytracing tier.
		inline bool IsDXRSupported(IDXGIAdapter1* adapter)
		{
			ID3D12Device* test_device;
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 feature_support_data = {};

			auto retval = SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&test_device)))
				&& SUCCEEDED(test_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_support_data, sizeof(feature_support_data)))
				&& feature_support_data.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;

			SAFE_RELEASE(test_device);

			return retval;
		}

		// Enable experimental features required for compute-based raytracing fallback.
		// This will set active D3D12 devices to DEVICE_REMOVED state.
		// Returns bool whether the call succeeded and the device supports the feature.
		inline bool IsDXRFallbackSupported(IDXGIAdapter1* adapter)
		{
			ID3D12Device* test_device;
			UUID experimentalFeatures[] = { D3D12ExperimentalShaderModels };

			auto retval = SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&test_device)));

			SAFE_RELEASE(test_device);

			return retval;
		}

		inline void EnableDXRFallback()
		{
			UUID experimental_features[] = { D3D12ExperimentalShaderModels };
			TRY_M(D3D12EnableExperimentalFeatures(1, experimental_features, nullptr, nullptr), "Failed to enable experimantal dxr fallback features.");
		}

		inline void SetRaytracingType(Device* device)
		{
			if (d3d12::settings::disable_rtx)
			{
				device->m_rt_type = RaytracingType::NONE;
			}
			else if (d3d12::settings::force_dxr_fallback && device->m_dxr_fallback_support)
			{
				device->m_rt_type = RaytracingType::FALLBACK;
			}
			else if (device->m_dxr_support)
			{
				device->m_rt_type = RaytracingType::NATIVE;
			}
			else
			{
				device->m_rt_type = RaytracingType::NONE;

			}
		}
	}

	Device* CreateDevice()
	{
		auto device = new Device();

		internal::EnableDebugLayer(device);
		internal::CreateFactory(device);
		internal::FindAdapter(device);

		device->m_dxr_support = internal::IsDXRSupported(device->m_adapter);
		device->m_dxr_fallback_support = internal::IsDXRFallbackSupported(device->m_adapter);
		internal::SetRaytracingType(device);

		if (!device->m_dxr_support)
		{
			LOGW(
				"No DXR support detected.\n"
				"Possible Reasons:\n"
				"\t 1) Wrong SDK version. (Required: `Windows 10 October 2018 Update SDK (17763)`)\n"
				"\t 2) Wrong OS version. (Required: 1809 (17763.107))\n"
				"\t 3) DX12 GPU with a incompatible DirectX Raytracing driver. (NVIDIA: driver version 415 or higher, AMD: Consult Vendor for availability)"
			);
		}
		if (!device->m_dxr_fallback_support)
		{
			LOGW(
				"No DXR Fallback support detected.\n"
				"Possible Reasons:\n"
				"GPU without feature level 11.1 or Resource Binding Tier 3."
			);
		}

		if (GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			LOGW("Enabling DXR Fallback.");
			//internal::EnableDXRFallback();
		}

		TRY_M(D3D12CreateDevice(device->m_adapter, device->m_feature_level, IID_PPV_ARGS(&device->m_native)),
			"Failed to create D3D12Device.");
		
		if (GetRaytracingType(device) == RaytracingType::FALLBACK)
		{
			auto fallback_device_flags = d3d12::settings::force_dxr_fallback ? CreateRaytracingFallbackDeviceFlags::ForceComputeFallback : CreateRaytracingFallbackDeviceFlags::None;
			TRY_M(D3D12CreateRaytracingFallbackDevice(device->m_native, fallback_device_flags, 0, IID_PPV_ARGS(&device->m_fallback_native)), "Failed to create fallback layer.");
		}

		internal::EnableGpuErrorBreaking(device);
		internal::GetSysInfo(device);

		std::wstring g = device->m_adapter_info.Description;
		LOG("{}", std::string(g.begin(), g.end()));

		return device;
	}

	RaytracingType GetRaytracingType(Device* device)
	{
		return device->m_rt_type;
	}

	void Destroy(Device* device)
	{
		SAFE_RELEASE(device->m_adapter);
		SAFE_RELEASE(device->m_native);
		SAFE_RELEASE(device->m_dxgi_factory);
		SAFE_RELEASE(device->m_debug_controller);
		SAFE_RELEASE(device->m_info_queue);
		SAFE_RELEASE(device->m_fallback_native);
		delete device;
	}

	void SetName(Device * device, std::wstring name)
	{
		device->m_native->SetName(name.c_str());
	}

} /* wr::d3d12 */