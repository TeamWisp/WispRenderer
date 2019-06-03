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
		struct Setup
		{
			float m_meters_in_world_unit = 0.5f; // Game scale, the size of a world unit measured in meters. Viktor: Honestly I don't get this value. All speeds are set in unit space not meters...
		};

		struct Runtime
		{
			bool m_allow_translation = true; // User can move the camera during session
			bool m_allow_rotation = true; // Camera can be rotated during session
			bool m_allow_fov = true; // FoV can be modified during session
			bool m_allow_mono_360 = true; // Game allows 360 capture during session
			bool m_allow_stero_360 = true; // Game allows 360 stereo capture during session
			bool m_allow_raw = false; // Game allows capturing pre-tonemapping raw HDR buffer
			bool m_allow_pause = true; // Game is paused during capture
			bool m_allow_highres = true; // Game allows highres capture during session
			float m_translation_speed_in_world_units_per_sec = 2.f; // The speed at which camera moves in the world, initialized with a value given in Configuration
			float m_rotation_speed_in_deg_per_second = 90.f; // The speed at which camera rotates, initialized with a value given in Configuration
			float m_maximum_fov_in_deg = 179; // The maximum FoV value in degrees displayed in the Ansel UI. Any value in the range [140, 179] can be specified and values outside will be clamped to this range.
		};

		Setup m_setup;
		Runtime m_runtime;
	};

	struct AnselData
	{
		bool out_ansel_support = false;
#ifdef NVIDIA_GAMEWORKS_ANSEL
		std::optional<ansel::Camera> out_original_camera; // Used to restore the camera to the original settings when exiting Ansel.
#endif
	};

	// Static variables so we can access them from the callbacks.
	static AnselSettings ansel_settings = AnselSettings();
	static bool ansel_session_running = false;

	namespace internal
	{
#ifdef NVIDIA_GAMEWORKS_ANSEL
		auto fill_ansel_config_obj = [](auto & conf)
		{
			conf.isTranslationAllowed = ansel_settings.m_runtime.m_allow_translation;
			conf.isRotationAllowed = ansel_settings.m_runtime.m_allow_rotation;
			conf.isFovChangeAllowed = ansel_settings.m_runtime.m_allow_fov;
			conf.is360MonoAllowed = ansel_settings.m_runtime.m_allow_mono_360;
			conf.is360StereoAllowed = ansel_settings.m_runtime.m_allow_stero_360;
			conf.isHighresAllowed = ansel_settings.m_runtime.m_allow_highres;
			conf.isRawAllowed = ansel_settings.m_runtime.m_allow_raw;
			conf.isPauseAllowed = ansel_settings.m_runtime.m_allow_pause;
			conf.translationalSpeedInWorldUnitsPerSecond = ansel_settings.m_runtime.m_translation_speed_in_world_units_per_sec;
			conf.rotationalSpeedInDegreesPerSecond = ansel_settings.m_runtime.m_rotation_speed_in_deg_per_second;
			conf.maximumFovInDegrees = ansel_settings.m_runtime.m_maximum_fov_in_deg;
		};

		ansel::StartSessionStatus startAnselSessionCallback(ansel::SessionConfiguration& conf, void* userPointer)
		{
			ansel_session_running = true;

			fill_ansel_config_obj(conf);


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


		ansel::Camera NativeToAnselCamera(std::shared_ptr<CameraNode> const & camera)
		{
			auto proj_offset = camera->GetProjectionOffset();

			ansel::Camera ansel_camera{};
			ansel_camera.aspectRatio = camera->m_aspect_ratio;
			ansel_camera.fov = DirectX::XMConvertToDegrees(camera->m_fov.m_fov);
			ansel_camera.position = { camera->m_position.m128_f32[0], camera->m_position.m128_f32[1], camera->m_position.m128_f32[2] };
			ansel_camera.rotation = { camera->m_rotation.m128_f32[0], camera->m_rotation.m128_f32[1], camera->m_rotation.m128_f32[2], camera->m_rotation.m128_f32[3] };
			ansel_camera.nearPlane = camera->m_frustum_near;
			ansel_camera.farPlane = camera->m_frustum_far;
			ansel_camera.projectionOffsetX = proj_offset.first;
			ansel_camera.projectionOffsetY = proj_offset.second;

			return ansel_camera;
		}

		void AnselToNativeCamera(ansel::Camera const & ansel_camera, std::shared_ptr<CameraNode> & camera)
		{
			camera->SetProjectionOffset(ansel_camera.projectionOffsetX, ansel_camera.projectionOffsetY);
			camera->SetPosition({ ansel_camera.position.x, ansel_camera.position.y, ansel_camera.position.z });
			camera->SetRotationQuaternion({ ansel_camera.rotation.x, ansel_camera.rotation.y , ansel_camera.rotation.z , ansel_camera.rotation.w });
			camera->m_fov = wr::CameraNode::FoV(wr::CameraNode::FovDefault(ansel_camera.fov));
			camera->m_frustum_near = ansel_camera.nearPlane;
			camera->m_frustum_far = ansel_camera.farPlane;
		}
#endif

		inline void SetupAnselTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool resize)
		{
#ifdef NVIDIA_GAMEWORKS_ANSEL
			if (resize)
			{
				return;
			}

			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<AnselData>(handle);
			ansel_settings = fg.GetSettings<AnselSettings>(handle);

			if (!n_render_system.m_window.has_value())
			{
				LOGW("Tried initializing ansel without a render window! Ansel is not supported for headless rendering.");
				return;
			}

			data.out_ansel_support = ansel::isAnselAvailable();
			if (!data.out_ansel_support)
			{
				LOGW("Your machine doesn't seem to support NVIDIA Ansel");
				return;
			}

			auto window = n_render_system.m_window.value();

			ansel::Configuration config;
			config.translationalSpeedInWorldUnitsPerSecond = ansel_settings.m_runtime.m_translation_speed_in_world_units_per_sec;
			config.metersInWorldUnit = ansel_settings.m_setup.m_meters_in_world_unit;
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
			ansel_settings = fg.GetSettings<AnselSettings>(handle);

			if (!data.out_ansel_support)
			{
				return;
			}

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

		frame_graph.AddTask<AnselData>(desc, L"NVIDIA Ansel");
		frame_graph.UpdateSettings<AnselData>(AnselSettings());
	}

} /* wr */
