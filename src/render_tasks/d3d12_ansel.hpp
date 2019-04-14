#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../d3d12/d3d12_constant_buffer_pool.hpp"
#include "d3d12_raytracing_task.hpp"
#include "d3d12_deferred_main.hpp"

#ifdef NVIDIA_GAMEWORKS_ANSEL
#include <AnselSDK.h>
#endif

namespace wr
{
	struct AnselSettings
	{
		struct Runtime
		{
		};

		Runtime m_runtime;
	};

	struct AnselData
	{
#ifdef NVIDIA_GAMEWORKS_ANSEL
		std::optional<ansel::Camera> out_original_camera;
#endif
	};

	static bool ansel_session_running = false;

	namespace internal
	{
#ifdef NVIDIA_GAMEWORKS_ANSEL
		ansel::StartSessionStatus startAnselSessionCallback(ansel::SessionConfiguration& conf, void* userPointer)
		{
			ansel_session_running = true;

			conf.isTranslationAllowed = true;
			conf.isRotationAllowed = true;
			conf.isFovChangeAllowed = true;
			conf.is360MonoAllowed = true;
			conf.is360StereoAllowed = true;


			return ansel::kAllowed;
		}

		void stopAnselSessionCallback(void* userPointer)
		{
			ansel_session_running = false;
		}

		void startAnselCaptureCallback(ansel::CaptureConfiguration const & conf, void* userPointer)
		{
		}

		void stopAnselCaptureCallback(void* userPointer)
		{
		}


		ansel::Camera NativeToAnselCamera(std::shared_ptr<CameraNode> const camera)
		{
			auto proj_offset = camera->GetProjectionOffset();

			ansel::Camera ansel_camera;
			ansel_camera.aspectRatio = camera->m_aspect_ratio;
			ansel_camera.fov = DirectX::XMConvertToDegrees(camera->m_fov.m_fov);
			ansel_camera.position = { camera->m_position.m128_f32[0], camera->m_position.m128_f32[1], camera->m_position.m128_f32[2] };
			ansel_camera.rotation = { camera->m_rotation.m128_f32[0], camera->m_rotation.m128_f32[1], camera->m_rotation.m128_f32[2], camera->m_rotation.m128_f32[3] };
			ansel_camera.nearPlane = camera->m_frustum_near;
			ansel_camera.farPlane = camera->m_frustum_far;

			return ansel_camera;
		}

		void AnselToNativeCamera(ansel::Camera const & ansel_camera, std::shared_ptr<CameraNode> camera)
		{
			camera->SetProjectionOffset(ansel_camera.projectionOffsetX, ansel_camera.projectionOffsetY);
			camera->SetPosition({ ansel_camera.position.x, ansel_camera.position.y, ansel_camera.position.z });
			camera->SetRotationQuaternion({ ansel_camera.rotation.x, ansel_camera.rotation.y , ansel_camera.rotation.z , ansel_camera.rotation.w });
			camera->m_fov = wr::CameraNode::FoV(wr::CameraNode::FovDefault(ansel_camera.fov));
		}
#endif

		inline void SetupAnselTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
#ifdef NVIDIA_GAMEWORKS_ANSEL
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto n_device = n_render_system.m_device;
			auto& data = fg.GetData<AnselData>(handle);
			auto n_render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			if (!n_render_system.m_window.has_value())
			{
				LOGW("Tried initializing ansel without a render window! Ansel is not supported for headless rendering.");
				return;
			}

			if (!ansel::isAnselAvailable())
			{
				LOGW("Your machine doesn't seem to support NVIDIA Ansel");
			}

			auto window = n_render_system.m_window.value();

			ansel::Configuration config;
			config.translationalSpeedInWorldUnitsPerSecond = 2.f;
			config.metersInWorldUnit = 0.5;
			config.right = { 1, 0, 0 };
			config.up = { 0, 1, 0 };
			config.forward = { 0, 0, -1 };
			config.fovType = ansel::kVerticalFov;
			config.isCameraOffcenteredProjectionSupported = true;
			config.isCameraRotationSupported = true;
			config.isCameraTranslationSupported = true;
			config.isCameraFovSupported = true;
			config.gameWindowHandle = window->GetWindowHandle();
			config.titleNameUtf8 = window->GetTitle().c_str();

			config.startSessionCallback = startAnselSessionCallback;
			config.stopSessionCallback = stopAnselSessionCallback;
			config.startCaptureCallback = startAnselCaptureCallback;
			config.stopCaptureCallback = stopAnselCaptureCallback;

			auto status = ansel::setConfiguration(config);
			if (status != ansel::kSetConfigurationSuccess)
			{
				LOGW("Failed to initialize NVIDIA Ansel.");
			}

			LOG("NVIDIA Ansel is initialized");
#endif
		}

		inline void ExecuteAnselTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
#ifdef NVIDIA_GAMEWORKS_ANSEL
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<AnselData>(handle);
			auto settings = fg.GetSettings<AnselSettings>(handle);

			if (!n_render_system.m_render_window.has_value())
			{
				LOGW("Tried initializing ansel without a render window! Ansel is not supported for headless rendering.");
				return;
			}

			if (ansel_session_running)
			{
				auto camera = sg.GetActiveCamera();
				auto ansel_camera = NativeToAnselCamera(camera);

				if (!data.out_original_camera.has_value())
				{
					data.out_original_camera = ansel_camera;
				}

				ansel::updateCamera(ansel_camera);

				AnselToNativeCamera(ansel_camera, camera);
			} 
			else if (data.out_original_camera.has_value())
			{
				auto camera = sg.GetActiveCamera();
				auto ansel_camera = data.out_original_camera.value();

				AnselToNativeCamera(ansel_camera, camera);

				data.out_original_camera = std::nullopt;
			}
#endif
		}

		inline void DestroyAnselTask(FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
#ifdef NVIDIA_GAMEWORKS_ANSEL
#endif
		}

	} /* internal */

	inline void AddAnselTask(FrameGraph& frame_graph)
	{
#ifndef NVIDIA_GAMEWORKS_ANSEL
		LOGW("Ansel task has been added to the frame graph. But `NVIDIA_GAMEWORKS_ANSEL` is not defined.");
#endif

		std::wstring name(L"NVIDIA Ansel");

		RenderTargetProperties rt_properties
		{
			RenderTargetProperties::IsRenderWindow(false),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::RENDER_TARGET),
			RenderTargetProperties::FinishedResourceState(ResourceState::COPY_SOURCE),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ d3d12::settings::back_buffer_format }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
			RenderTargetProperties::ResourceName(name)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::SetupAnselTask(rs, fg, handle, resize);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteAnselTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph& fg, RenderTaskHandle handle, bool resize) {
			internal::DestroyAnselTask(fg, handle, resize);
		};

		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COMPUTE;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<AnselData>(desc, FG_DEPS(1, DeferredMainTaskData));
		frame_graph.UpdateSettings<AnselData>(AnselSettings());
	}

} /* wr */
