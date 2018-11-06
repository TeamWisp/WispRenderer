#include "d3d12_functions.hpp"

#include "../util/log.hpp"
#include "d3d12_defines.hpp"

namespace d3d12
{
	CommandList* CreateCommandList(Device* device, unsigned int num_allocators, CmdListType type)
	{
		auto* cmd_list = new CommandList();
		const auto n_device = device->m_native;
		const auto n_cmd_list = cmd_list->m_native;

		cmd_list->m_allocators.resize(num_allocators);

		// Create the allocators
		for (auto& allocator : cmd_list->m_allocators)
		{
			TRY_M(n_device->CreateCommandAllocator((D3D12_COMMAND_LIST_TYPE)type, IID_PPV_ARGS(&allocator)),
				"Failed to create command allocator");
			NAME_D3D12RESOURCE(allocator);
		}

		// Create the command lists
		TRY_M(device->m_native->CreateCommandList(
			0,
			(D3D12_COMMAND_LIST_TYPE)type,
			cmd_list->m_allocators[0],
			NULL,
			IID_PPV_ARGS(&cmd_list->m_native)
		), "Failed to create command list");
		NAME_D3D12RESOURCE(cmd_list->m_native);
		cmd_list->m_native->Close(); // TODO: Can be optimized away.

		return cmd_list;
	}

	void SetName(CommandList* cmd_list, std::string const& name)
	{
		SetName(cmd_list, std::wstring(name.begin(), name.end()));
	}

	void SetName(CommandList* cmd_list, std::wstring const& name)
	{
		cmd_list->m_native->SetName(name.c_str());

		for (auto& allocator : cmd_list->m_allocators)
		{
			allocator->SetName((name + L" Allocator").c_str());
		}
	}

	void Begin(CommandList& cmd_list, unsigned int frame_idx)
	{
		// TODO: move resetting to when the command list is executed. This is how vulkan does it.
		TRY_M(cmd_list.m_allocators[frame_idx]->Reset(), 
			"Failed to reset cmd allocators");

		// Only reset with pipeline state if using bundles since only then this will impact fps.
		// Otherwise its just easier to pass NULL and suffer the insignificant performance loss.
		TRY_M(cmd_list.m_native->Reset(cmd_list.m_allocators[frame_idx], NULL),
			"Failed to reset command list.");
	}

	void End(CommandList& cmd_list)
	{
		cmd_list.m_native->Close();
	}

	void Draw(CommandList* cmd_list, unsigned int vertex_count, unsigned int inst_count)
	{
		cmd_list->m_native->DrawInstanced(vertex_count, inst_count, 0, 0);
	}

	void DrawIndexed(CommandList* cmd_list, unsigned int idx_count, unsigned int inst_count)
	{
		cmd_list->m_native->DrawIndexedInstanced(idx_count, inst_count, 0, 0, 0);
	}

	void Transition(CommandList& cmd_list, RenderTarget& render_target, unsigned int frame_index, ResourceState from, ResourceState to)
	{
		CD3DX12_RESOURCE_BARRIER end_transition = CD3DX12_RESOURCE_BARRIER::Transition(
			render_target.m_render_targets[frame_index],
			(D3D12_RESOURCE_STATES)from,
			(D3D12_RESOURCE_STATES)to
		);

		cmd_list.m_native->ResourceBarrier(1, &end_transition);
	}

	void Transition(CommandList& cmd_list, RenderTarget& render_target, ResourceState from, ResourceState to)
	{
		std::vector<CD3DX12_RESOURCE_BARRIER> barriers;
		barriers.resize(render_target.m_num_render_targets);
		for (auto i = 0; i < render_target.m_num_render_targets; i++)
		{
			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				render_target.m_render_targets[i],
				(D3D12_RESOURCE_STATES)from,
				(D3D12_RESOURCE_STATES)to
			);

			barriers[i] = barrier;
		}
		cmd_list.m_native->ResourceBarrier(barriers.size(), barriers.data());
	}

	void Destroy(CommandList* cmd_list)
	{
		SAFE_RELEASE(cmd_list->m_native);
		for (auto& allocator : cmd_list->m_allocators)
		{
			SAFE_RELEASE(allocator);
		}
		delete cmd_list;
	}	

} /* d3d12 */