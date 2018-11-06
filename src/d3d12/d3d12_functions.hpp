#pragma once

#include <memory>

#include "d3d12_structs.hpp"

namespace d3d12
{

	// Device
	[[nodiscard]] Device* CreateDevice();
	void Destroy(Device* device);

	// CommandQueue
	[[nodiscard]] CommandQueue* CreateCommandQueue(Device* device, CmdListType type);
	void Execute(CommandQueue* cmd_queue, std::vector<CommandList> const & cmd_lists, VFence* fence);
	void Destroy(CommandQueue* cmd_queue);

	// CommandList
	[[nodiscard]] CommandList* CreateCommandList(Device* device, unsigned int num_allocators, CmdListType type);
	void SetName(CommandList* cmd_list, std::string const& name);
	void SetName(CommandList* cmd_list, std::wstring const& name);
	void Begin(CommandList& cmd_list, unsigned int frame_idx);
	void End(CommandList& cmd_list);
	void Bind(CommandList& cmd_list, RenderTarget& render_target, unsigned int frame_idx, bool clear = true, bool clear_depth = true);
	void BindVersioned(CommandList& cmd_list, RenderTarget& render_target, unsigned int frame_idx, bool clear = true, bool clear_depth = true);
	void BindOnlyDepth(CommandList& cmd_list, RenderTarget& render_target, unsigned int frame_idx, bool clear = true);
	void Bind(CommandList& cmd_list, Viewport& viewport);
	void Bind(CommandList& cmd_list, PipelineState* pipeline_state);
	void BindCompute(CommandList& cmd_list, PipelineState* pipeline_state);
	void SetPrimitiveTopology(CommandList& cmd_list, D3D12_PRIMITIVE_TOPOLOGY topology);
	// void Bind(CommandList* cmd_list, ConstantBuffer* buffer, unsigned int root_parameter_idx, unsigned int frame_idx);
	// void BindCompute(CommandList* cmd_list, ConstantBuffer* buffer, unsigned int root_parameter_idx, unsigned int frame_idx);
	// void Bind(CommandList& cmd_list, TextureArray& ta, unsigned int root_param_index);
	void Bind(CommandList& cmd_list, DescHeapGPUHandle& handle, unsigned int root_param_index);
	void BindCompute(CommandList& cmd_list, DescHeapGPUHandle& handle, unsigned int root_param_index);
	void Bind(CommandList& cmd_list, std::vector<DescriptorHeap*> heaps);
	// void BindVertexBuffer(CommandList* cmd_list, StagingBuffer* buffer);
	// void BindIndexBuffer(CommandList* cmd_list, StagingBuffer* buffer, unsigned int offset = 0);
	void Draw(CommandList* cmd_list, unsigned int vertex_count, unsigned int inst_count);
	void DrawIndexed(CommandList* cmd_list, unsigned int idx_count, unsigned int inst_count);
	void Transition(CommandList& cmd_list, RenderTarget& render_target, unsigned int frame_index, ResourceState from, ResourceState to);
	void Transition(CommandList& cmd_list, RenderTarget& render_target, ResourceState from, ResourceState to);
	// void Transition(CommandList* cmd_list, Texture* texture, ResourceState from, ResourceState to);
	void Destroy(CommandList* cmd_list);

	// RenderTarget
	[[nodiscard]] RenderTarget* CreateRenderTarget(Device* device, CommandQueue* cmd_queue, unsigned int width, unsigned int height, desc::RenderTargetDesc descriptor);
	void SetName(RenderTarget* render_target, std::wstring name);
	void SetName(RenderTarget* render_target, std::string name);
	void CreateRenderTargetViews(RenderTarget* render_target, Device* device, CommandQueue* cmd_queue, unsigned int width, unsigned int height);
	void CreateDepthStencilBuffer(RenderTarget* render_target, Device* device, CommandQueue* cmd_queue, unsigned int width, unsigned int height);
	void CreateSRVFromDSV(RenderTarget* render_target, DescHeapCPUHandle& handle);
	void CreateSRVFromRTV(RenderTarget* render_target, DescHeapCPUHandle& handle, unsigned int num, Format formats[8]);
	void CreateSRVFromSpecificRTV(RenderTarget* render_target, DescHeapCPUHandle& handle, unsigned int id, Format format);
	// void CreateUAVFromTexture(Texture* tex, DescHeapCPUHandle& handle, unsigned int mip_slice = 0, unsigned int array_slice = 0);
	// void CreateSRVFromTexture(Texture* tex, DescHeapCPUHandle& handle);
	void Resize(RenderTarget** render_target, Device* device, CommandQueue* cmd_queue, unsigned int width, unsigned int height);
	void IncrementFrameIdx(RenderTarget* render_target);
	void DestroyDepthStencilBuffer(RenderTarget* render_target);
	void DestroyRenderTargetViews(RenderTarget* render_target);
	void Destroy(RenderTarget* render_target);

	// RenderWindow
	RenderWindow* CreateRenderWindow(Device* device, HWND window, CommandQueue* cmd_queue, unsigned int num_back_buffers);
	RenderWindow* CreateRenderWindow(Device* device, IUnknown* window, CommandQueue* cmd_queue, unsigned int num_back_buffers);
	void Resize(RenderWindow* render_window, Device* device, CommandQueue* cmd_queue, unsigned int width, unsigned int height);
	void Present(RenderWindow* render_window, CommandQueue* cmd_queue, Device* device);
	void Destroy(RenderWindow* render_window);

	// Descriptor Heap
	[[nodiscard]] DescriptorHeap* CreateDescriptorHeap(Device* device, desc::DescriptorHeapDesc const & descriptor);
	[[nodiscard]] DescHeapGPUHandle GetGPUHandle(DescriptorHeap* desc_heap, unsigned int index = 0);
	[[nodiscard]] DescHeapCPUHandle GetCPUHandle(DescriptorHeap* desc_heap, unsigned int index = 0);
	void Offset(DescHeapGPUHandle& handle, unsigned int index, unsigned int increment_size);
	void Offset(DescHeapCPUHandle& handle, unsigned int index, unsigned int increment_size);
	void Destroy(DescriptorHeap* desc_heap);

} /* d3d12 */