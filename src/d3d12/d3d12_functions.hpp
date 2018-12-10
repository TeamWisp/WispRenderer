#pragma once

#include <memory>

#include "d3d12_structs.hpp"

#include "d3d12_constant_buffer_pool.hpp"

namespace wr::d3d12
{

	// Device
	[[nodiscard]] Device* CreateDevice();
	RaytracingType GetRaytracingType(Device* device);
	void Destroy(Device* device);

	// CommandQueue
	[[nodiscard]] CommandQueue* CreateCommandQueue(Device* device, CmdListType type);
	void Execute(CommandQueue* cmd_queue, std::vector<CommandList*> const & cmd_lists, Fence* fence);
	void Destroy(CommandQueue* cmd_queue);

	// CommandList
	[[nodiscard]] CommandList* CreateCommandList(Device* device, unsigned int num_allocators, CmdListType type);
	void SetName(CommandList* cmd_list, std::string const & name);
	void SetName(CommandList* cmd_list, std::wstring const & name);
	void Begin(CommandList* cmd_list, unsigned int frame_idx);
	void End(CommandList* cmd_list);
	void ExecuteBundle(CommandList* cmd_list, CommandList* bundle);
	void ExecuteIndirect(CommandList* cmd_list, CommandSignature* cmd_signature, IndirectCommandBuffer* buffer);
	void BindRenderTarget(CommandList* cmd_list, RenderTarget* render_target, bool clear = true, bool clear_depth = true);
	void BindRenderTargetVersioned(CommandList* cmd_list, RenderTarget* render_target, unsigned int frame_idx, bool clear = true, bool clear_depth = true);
	void BindRenderTargetOnlyDepth(CommandList* cmd_list, RenderTarget* render_target, unsigned int frame_idx, bool clear = true);
	void BindViewport(CommandList* cmd_list, Viewport const & viewport);
	void BindPipeline(CommandList* cmd_list, PipelineState* pipeline_state);
	void BindComputePipeline(CommandList* cmd_list, PipelineState* pipeline_state);
	void BindDescriptorHeaps(CommandList* cmd_list, std::vector<DescriptorHeap*> heaps, unsigned int frame_idx, bool fallback = false);
	void SetPrimitiveTopology(CommandList* cmd_list, D3D12_PRIMITIVE_TOPOLOGY topology);
	void BindConstantBuffer(CommandList* cmd_list, HeapResource* buffer, unsigned int root_parameter_idx, unsigned int frame_idx);
	void BindComputeConstantBuffer(CommandList* cmd_list, HeapResource* buffer, unsigned int root_parameter_idx, unsigned int frame_idx);
	//void Bind(CommandList& cmd_list, TextureArray& ta, unsigned int root_param_index);
	void BindDescriptorTable(CommandList* cmd_list, DescHeapGPUHandle& handle, unsigned int root_param_index);
	void BindComputeDescriptorTable(CommandList* cmd_list, DescHeapGPUHandle& handle, unsigned int root_param_index);
	//void Bind(CommandList& cmd_list, std::vector<DescriptorHeap*> const & heaps);
	void BindVertexBuffer(CommandList* cmd_list, StagingBuffer* buffer, std::size_t offset, std::size_t size, std::size_t m_stride);
	void BindIndexBuffer(CommandList* cmd_list, StagingBuffer* buffer, unsigned int offset, unsigned int size);
	void Draw(CommandList* cmd_list, unsigned int vertex_count, unsigned int inst_count, unsigned int vertex_start);
	void DrawIndexed(CommandList* cmd_list, unsigned int idx_count, unsigned int inst_count, unsigned int idx_start, unsigned int vertex_start);
	void Dispatch(CommandList* cmd_list, unsigned int thread_group_count_x, unsigned int thread_group_count_y, unsigned int thread_group_count_z);
	void Transition(CommandList* cmd_list, RenderTarget* render_target, unsigned int frame_index, ResourceState from, ResourceState to);
	void Transition(CommandList* cmd_list, RenderTarget* render_target, ResourceState from, ResourceState to);
	void Transition(CommandList* cmd_list, std::vector<TextureResource*> const& textures, ResourceState from, ResourceState to);
	void Transition(CommandList* cmd_list, IndirectCommandBuffer* buffer, ResourceState from, ResourceState to);
	void TransitionDepth(CommandList* cmd_list, RenderTarget* render_target, ResourceState from, ResourceState to);
	void DispatchRays(CommandList* cmd_list);
	// void Transition(CommandList* cmd_list, Texture* texture, ResourceState from, ResourceState to);
	void Destroy(CommandList* cmd_list);

	// Command List Signature
	CommandSignature* CreateCommandSignature(Device* device, RootSignature* root_signature, std::vector<D3D12_INDIRECT_ARGUMENT_DESC> arg_descs, size_t byte_stride);
	void Destroy(CommandSignature* cmd_signature);

	// RenderTarget
	[[nodiscard]] RenderTarget* CreateRenderTarget(Device* device, unsigned int width, unsigned int height, desc::RenderTargetDesc descriptor);
	void SetName(RenderTarget* render_target, std::wstring name);
	void SetName(RenderTarget* render_target, std::string name);
	void CreateRenderTargetViews(RenderTarget* render_target, Device* device, unsigned int width, unsigned int height);
	void CreateDepthStencilBuffer(RenderTarget* render_target, Device* device, unsigned int width, unsigned int height);
	void CreateSRVFromDSV(RenderTarget* render_target, DescHeapCPUHandle& handle);
	void CreateSRVFromRTV(RenderTarget* render_target, DescHeapCPUHandle& handle, unsigned int num, Format formats[8]);
	void CreateUAVFromRTV(RenderTarget* render_target, DescHeapCPUHandle& handle, unsigned int num, Format formats[8]);
	void CreateSRVFromSpecificRTV(RenderTarget* render_target, DescHeapCPUHandle& handle, unsigned int id, Format format);
	void CreateSRVFromStructuredBuffer(HeapResource* structured_buffer, DescHeapCPUHandle& handle, unsigned int id);
	// void CreateUAVFromTexture(Texture* tex, DescHeapCPUHandle& handle, unsigned int mip_slice = 0, unsigned int array_slice = 0);
	// void CreateSRVFromTexture(Texture* tex, DescHeapCPUHandle& handle);
	void Resize(RenderTarget** render_target, Device* device, unsigned int width, unsigned int height);
	void IncrementFrameIdx(RenderTarget* render_target);
	void DestroyDepthStencilBuffer(RenderTarget* render_target);
	void DestroyRenderTargetViews(RenderTarget* render_target);
	void Destroy(RenderTarget* render_target);

	// Texture
	[[nodiscard]] TextureResource* CreateTexture(Device* device, desc::TextureDesc* description, bool allow_uav);
	void CreateSRVFromTexture(TextureResource* tex, DescHeapCPUHandle& handle, Format format);
	//void CreateUAVFromTexture(TextureResource* tex, DescHeapCPUHandle& handle, unsigned int mip_slice = 0, unsigned int array_slice = 0);
	void Destroy(TextureResource* tex);

	// RenderWindow
	[[nodiscard]] RenderWindow* CreateRenderWindow(Device* device, HWND window, CommandQueue* cmd_queue, unsigned int num_back_buffers);
	[[nodiscard]] RenderWindow* CreateRenderWindow(Device* device, IUnknown* window, CommandQueue* cmd_queue, unsigned int num_back_buffers);
	void Resize(RenderWindow* render_window, Device* device, unsigned int width, unsigned int height, bool fullscreen);
	void Present(RenderWindow* render_window, Device* device);
	void Destroy(RenderWindow* render_window);

	// Descriptor Heap
	[[nodiscard]] DescriptorHeap* CreateDescriptorHeap(Device* device, desc::DescriptorHeapDesc const & descriptor);
	[[nodiscard]] DescHeapGPUHandle GetGPUHandle(DescriptorHeap* desc_heap, unsigned int frame_idx, unsigned int index = 0);
	[[nodiscard]] DescHeapCPUHandle GetCPUHandle(DescriptorHeap* desc_heap, unsigned int frame_idx, unsigned int index = 0);
	void Offset(DescHeapGPUHandle& handle, unsigned int index, unsigned int increment_size);
	void Offset(DescHeapCPUHandle& handle, unsigned int index, unsigned int increment_size);
	void Destroy(DescriptorHeap* desc_heap);

	// Fence
	[[nodiscard]] Fence* CreateFence(Device* device);
	void Signal(Fence* fence, CommandQueue* cmd_queue);
	void WaitFor(Fence* fence);
	void Destroy(Fence* fence);

	// Viewport
	[[nodiscard]] Viewport CreateViewport(int width, int height);
	void ResizeViewport(Viewport& viewport, int width, int height);

	// Shader
	[[nodiscard]] Shader* LoadShader(ShaderType type, std::string const & path, std::string const & entry = "main");
	[[nodiscard]] Shader* LoadDXCShader(ShaderType type, std::string const & path, std::string const & entry = "main");
	bool ReloadShader(Shader* shader);
	void Destroy(Shader* shader);

	// Root Signature
	[[nodiscard]] RootSignature* CreateRootSignature(desc::RootSignatureDesc create_info);
	void FinalizeRootSignature(RootSignature* root_signature, Device* device);
	void Destroy(RootSignature* root_signature);

	// Pipeline State
	[[nodiscard]] PipelineState* CreatePipelineState(); // TODO: Creation of root signature and pipeline are not the same related to the descriptor.
	void SetVertexShader(PipelineState* pipeline_state, Shader* shader);
	void SetFragmentShader(PipelineState* pipeline_state, Shader* shader);
	void SetComputeShader(PipelineState* pipeline_state, Shader* shader);
	void SetRootSignature(PipelineState* pipeline_state, RootSignature* root_signature);
	void FinalizePipeline(PipelineState* pipeline_state, Device* device, desc::PipelineStateDesc desc);
	void RefinalizePipeline(PipelineState* pipeline_state, Device* device, desc::PipelineStateDesc desc);
	void Destroy(PipelineState* pipeline_state);

	// Staging Buffer
	[[nodiscard]] StagingBuffer* CreateStagingBuffer(Device* device, void* data, std::uint64_t size, std::uint64_t m_stride, ResourceState resource_state);
	void UpdateStagingBuffer(StagingBuffer* buffer, void* data, std::uint64_t size, std::uint64_t offset);
	void StageBuffer(StagingBuffer* buffer, CommandList* cmd_list);
	void StageBufferRegion(StagingBuffer* buffer, std::uint64_t size, std::uint64_t offset, CommandList* cmd_list);
	void FreeStagingBuffer(StagingBuffer* buffer);
	void Evict(StagingBuffer* buffer);
	void MakeResident(StagingBuffer* buffer);
	void Destroy(StagingBuffer* buffer);

	// Heap
	[[nodiscard]] Heap<HeapOptimization::SMALL_BUFFERS>* CreateHeap_SBO(Device* device, std::uint64_t size_in_bytes, ResourceType resource_type, unsigned int versioning_count);
	[[nodiscard]] Heap<HeapOptimization::BIG_BUFFERS>* CreateHeap_BBO(Device* device, std::uint64_t size_in_bytes, ResourceType resource_type, unsigned int versioning_count);
	[[nodiscard]] Heap<HeapOptimization::SMALL_STATIC_BUFFERS>* CreateHeap_SSBO(Device* device, std::uint64_t size_in_bytes, ResourceType resource_type, unsigned int versioning_count);
	[[nodiscard]] Heap<HeapOptimization::BIG_STATIC_BUFFERS>* CreateHeap_BSBO(Device* device, std::uint64_t size_in_bytes, ResourceType resource_type, unsigned int versioning_count);
	[[nodiscard]] HeapResource* AllocConstantBuffer(Heap<HeapOptimization::SMALL_BUFFERS>* heap, std::uint64_t size_in_bytes);
	[[nodiscard]] HeapResource* AllocConstantBuffer(Heap<HeapOptimization::BIG_BUFFERS>* heap, std::uint64_t size_in_bytes);
	[[nodiscard]] HeapResource* AllocStructuredBuffer(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap, std::uint64_t size_in_bytes, std::uint64_t stride, bool used_as_uav);
	[[nodiscard]] HeapResource* AllocGenericBuffer(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap, std::uint64_t size_in_bytes);
	void DeallocConstantBuffer(Heap<HeapOptimization::SMALL_BUFFERS>* heap, HeapResource* heapResource);
	void DeallocConstantBuffer(Heap<HeapOptimization::BIG_BUFFERS>* heap, HeapResource* heapResource);
	void DeallocBuffer(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap, HeapResource* heapResource);
	void MapHeap(Heap<HeapOptimization::SMALL_BUFFERS>* heap);
	void MapHeap(Heap<HeapOptimization::BIG_BUFFERS>* heap);
	void UnmapHeap(Heap<HeapOptimization::SMALL_BUFFERS>* heap);
	void UnmapHeap(Heap<HeapOptimization::BIG_BUFFERS>* heap);
	void MakeResident(Heap<HeapOptimization::SMALL_BUFFERS>* heap);
	void MakeResident(Heap<HeapOptimization::BIG_BUFFERS>* heap);
	void MakeResident(Heap<HeapOptimization::SMALL_STATIC_BUFFERS>* heap);
	void MakeResident(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap);
	void EnqueueMakeResident(Heap<HeapOptimization::SMALL_BUFFERS>* heap, Fence* fence); // Untested
	void EnqueueMakeResident(Heap<HeapOptimization::BIG_BUFFERS>* heap, Fence* fence);  // Untested
	void EnqueueMakeResident(Heap<HeapOptimization::SMALL_STATIC_BUFFERS>* heap, Fence* fence); // Untested
	void EnqueueMakeResident(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap, Fence* fence);  // Untested
	void Evict(Heap<HeapOptimization::SMALL_BUFFERS>* heap);
	void Evict(Heap<HeapOptimization::BIG_BUFFERS>* heap);
	void Evict(Heap<HeapOptimization::SMALL_STATIC_BUFFERS>* heap);
	void Evict(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap);
	void Destroy(Heap<HeapOptimization::SMALL_BUFFERS>* heap);
	void Destroy(Heap<HeapOptimization::BIG_BUFFERS>* heap);
	void Destroy(Heap<HeapOptimization::SMALL_STATIC_BUFFERS>* heap);
	void Destroy(Heap<HeapOptimization::BIG_STATIC_BUFFERS>* heap);

	// Resources
	void UpdateConstantBuffer(HeapResource* buffer, unsigned int frame_idx, void* data, std::uint64_t size_in_bytes);
	void UpdateStructuredBuffer(HeapResource* buffer,
		unsigned int frame_idx,
		void* data,
		std::uint64_t size_in_bytes,
		std::uint64_t offset,
		std::uint64_t stride,
		CommandList* cmd_list);

	// Indirect Command Buffer
	[[nodiscard]] IndirectCommandBuffer* CreateIndirectCommandBuffer(Device* device, std::size_t max_num_buffers, std::size_t command_size);
	void StageBuffer(CommandList* cmd_list, IndirectCommandBuffer* buffer, void* data, std::size_t num_commands);

	// State Object
	[[nodiscard]] StateObject* CreateStateObject(Device* device, CD3DX12_STATE_OBJECT_DESC desc);
	[[nodiscard]] std::uint64_t GetShaderIdentifierSize(Device* device, StateObject* obj);
	[[nodiscard]] void* GetShaderIdentifier(Device* device, StateObject* obj, std::string const & name);
	void Destroy(StateObject* obj);

	// Acceelration Structure
	[[nodiscard]] std::pair<AccelerationStructure, AccelerationStructure> CreateAccelerationStructures(Device* device,
		CommandList* cmd_list,
		DescriptorHeap* desc_heap,
		std::vector<StagingBuffer*> vertex_buffers);

	// Shader Record
	[[nodiscard]] ShaderRecord CreateShaderRecord(void* identifier, std::uint64_t identifier_size, void* local_root_args = nullptr, std::uint64_t local_root_args_size = 0);
	void CopyShaderRecord(ShaderRecord src, void* dest);

	// Shader Table
	[[nodiscard]] ShaderTable* CreateShaderTable(Device* device,
			std::uint64_t num_shader_records,
			std::uint64_t shader_record_size);
	void AddShaderRecord(ShaderTable* table, ShaderRecord record);

} /* wr::d3d12 */
