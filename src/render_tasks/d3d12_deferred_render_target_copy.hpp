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
		std::wstring w_name(name_temp.begin(), name_temp.end());

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
			RenderTargetProperties::Clear(false),
			RenderTargetProperties::ClearDepth(false),
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

		//Temporarry stuff for FG editor 
		w_name = L"Copy render target"
		frame_graph.AddTask<RenderTargetCopyTaskData>(desc, w_name, FG_DEPS<T>());
	}

} /* wr */
