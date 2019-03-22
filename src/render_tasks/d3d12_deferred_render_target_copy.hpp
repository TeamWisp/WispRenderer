#pragma once

#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../frame_graph/frame_graph.hpp"
#include <locale>
#include <codecvt>
#include <string>

namespace wr
{
	struct RenderTargetCopyTaskData
	{
		d3d12::RenderTarget* out_rt;
	};

	namespace internal
	{
		template<typename T>
		inline void SetupCopyTask(RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle)
		{
			auto& data = fg.GetData<RenderTargetCopyTaskData>(handle);
			data.out_rt = static_cast<d3d12::RenderTarget*>(fg.GetPredecessorRenderTarget<T>());
		}

		inline void ExecuteCopyTask(RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto& n_render_system = static_cast<D3D12RenderSystem&>(rs);
			auto& data = fg.GetData<RenderTargetCopyTaskData>(handle);
			auto cmd_list = fg.GetCommandList<d3d12::CommandList>(handle);
			auto render_target = fg.GetRenderTarget<d3d12::RenderTarget>(handle);

			const auto frame_idx = n_render_system.GetFrameIdx();

			D3D12_TEXTURE_COPY_LOCATION dst = {};
			dst.pResource = render_target->m_render_targets[frame_idx % render_target->m_render_targets.size()];
			dst.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.pResource = data.out_rt->m_render_targets[0];
			src.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			src.SubresourceIndex = 0;

			cmd_list->m_native->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		}

	} /* internal */

	template<typename T>
	inline void AddRenderTargetCopyTask(FrameGraph& frame_graph)
	{
		std::string name_temp = std::string("Render Target (") + std::string(typeid(T).name()) + std::string(") Copy task");

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wide = converter.from_bytes(name_temp.c_str());

		RenderTargetProperties rt_properties{
			RenderTargetProperties::IsRenderWindow(true),
			RenderTargetProperties::Width(std::nullopt),
			RenderTargetProperties::Height(std::nullopt),
			RenderTargetProperties::ExecuteResourceState(ResourceState::COPY_DEST),
			RenderTargetProperties::FinishedResourceState(ResourceState::PRESENT),
			RenderTargetProperties::CreateDSVBuffer(false),
			RenderTargetProperties::DSVFormat(Format::UNKNOWN),
			RenderTargetProperties::RTVFormats({ Format::R8G8B8A8_UNORM }),
			RenderTargetProperties::NumRTVFormats(1),
			RenderTargetProperties::Clear(true),
			RenderTargetProperties::ClearDepth(true),
			RenderTargetProperties::ResourceName(wide)
		};

		RenderTaskDesc desc;
		desc.m_setup_func = [](RenderSystem& rs, FrameGraph& fg, RenderTaskHandle handle, bool) {
			internal::SetupCopyTask<T>(rs, fg, handle);
		};
		desc.m_execute_func = [](RenderSystem& rs, FrameGraph& fg, SceneGraph& sg, RenderTaskHandle handle) {
			internal::ExecuteCopyTask(rs, fg, sg, handle);
		};
		desc.m_destroy_func = [](FrameGraph&, RenderTaskHandle, bool) {
			// Nothing to destroy
		};
		
		desc.m_properties = rt_properties;
		desc.m_type = RenderTaskType::COPY;
		desc.m_allow_multithreading = true;

		frame_graph.AddTask<RenderTargetCopyTaskData>(desc);
	}

} /* wr */
