#pragma once

#include "../renderer.hpp"

#include <DirectXMath.h>

#include "../scene_graph/scene_graph.hpp"
#include "../scene_graph/light_node.hpp"
#include "../vertex.hpp"
#include "d3d12_structs.hpp"


namespace wr
{
	namespace d3d12
	{
		struct CommandList;
		struct RenderTarget;
	}
  
	struct MeshNode;
	struct CameraNode;
	struct D3D12ConstantBufferHandle;

	class D3D12StructuredBufferPool;
	class D3D12ModelPool;
	class D3D12TexturePool;
	class DynamicDescriptorHeap;

	namespace temp
	{
		struct IndirectCommand
		{
			D3D12_GPU_VIRTUAL_ADDRESS cbv_camera;
			D3D12_GPU_VIRTUAL_ADDRESS cbv_object;
			D3D12_VERTEX_BUFFER_VIEW vb_view;
			D3D12_DRAW_ARGUMENTS draw_arguments;
		};

		struct IndirectCommandIndexed
		{
			D3D12_GPU_VIRTUAL_ADDRESS cbv_camera;
			D3D12_GPU_VIRTUAL_ADDRESS cbv_object;
			D3D12_VERTEX_BUFFER_VIEW vb_view;
			D3D12_DRAW_INDEXED_ARGUMENTS draw_arguments;
		};

		struct ProjectionView_CBData
		{
			DirectX::XMMATRIX m_view;
			DirectX::XMMATRIX m_projection;
			DirectX::XMMATRIX m_inverse_projection;
		};

		struct RayTracingCamera_CBData
		{
			DirectX::XMMATRIX m_view;
			DirectX::XMMATRIX m_inverse_view_projection;
			DirectX::XMVECTOR m_camera_position;

			float light_radius;
			float metal;
			float roughness;
			float intensity;
		};

		struct RayTracingMaterial_CBData
		{
			DirectX::XMMATRIX m_model;
			float idx_offset;
			float vertex_offset;
			float albedo_id;
			float normal_id;
			float roughness_id;
			float metallicness_id;
			float padding0;
			float padding1;
		};

		static const constexpr float size = 1.0f;
		static const constexpr Vertex2D quad_vertices[] = {
			{ -size, -size },
			{ size, -size },
			{ -size, size },
			{ size, size },
		};

	} /* temp */

	class D3D12RenderSystem : public RenderSystem
	{
	public:
		~D3D12RenderSystem() final;

		void Init(std::optional<Window*> window) final;
		std::unique_ptr<TextureHandle> Render(std::shared_ptr<SceneGraph> const & scene_graph, FrameGraph & frame_graph) final;
		void Resize(std::uint32_t width, std::uint32_t height) final;

		std::shared_ptr<TexturePool> CreateTexturePool(std::size_t size_in_mb, std::size_t num_of_textures) final;
		std::shared_ptr<MaterialPool> CreateMaterialPool(std::size_t size_in_mb) final;
		std::shared_ptr<ModelPool> CreateModelPool(std::size_t vertex_buffer_pool_size_in_mb, std::size_t index_buffer_pool_size_in_mb) final;
		std::shared_ptr<ConstantBufferPool> CreateConstantBufferPool(std::size_t size_in_mb) final;
		std::shared_ptr<StructuredBufferPool> CreateStructuredBufferPool(std::size_t size_in_mb) final;

		void PrepareRootSignatureRegistry() final;
		void PrepareShaderRegistry() final;
		void PreparePipelineRegistry() final;
		void ReloadPipelineRegistryEntry(RegistryHandle handle);
		void PrepareRTPipelineRegistry() final;

		void WaitForAllPreviousWork() final;

		wr::CommandList* GetDirectCommandList(unsigned int num_allocators) final;
		wr::CommandList* GetBundleCommandList(unsigned int num_allocators) final;
		wr::CommandList* GetComputeCommandList(unsigned int num_allocators) final;
		wr::CommandList* GetCopyCommandList(unsigned int num_allocators) final;
		RenderTarget* GetRenderTarget(RenderTargetProperties properties) final;
		void ResizeRenderTarget(RenderTarget** render_target, std::uint32_t width, std::uint32_t height) final;
		void RequestFullscreenChange(bool fullscreen_state);

		void StartRenderTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target) final;
		void StopRenderTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target) final;
		void StartComputeTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target) final;
		void StopComputeTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target) final;
		void StartCopyTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target) final;
		void StopCopyTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target) final;

		void InitSceneGraph(SceneGraph& scene_graph);

		void Init_MeshNodes(std::vector<std::shared_ptr<MeshNode>>& nodes);
		void Init_CameraNodes(std::vector<std::shared_ptr<CameraNode>>& nodes);
		void Init_LightNodes(std::vector<std::shared_ptr<LightNode>>& nodes, std::vector<Light>& lights);

		void Update_MeshNodes(std::vector<std::shared_ptr<MeshNode>>& nodes);
		void Update_CameraNodes(std::vector<std::shared_ptr<CameraNode>>& nodes);
		void Update_LightNodes(SceneGraph& scene_graph);
		void Update_Transforms(SceneGraph& scene_graph, std::shared_ptr<Node>& node);

		void PreparePreRenderCommands(bool clear_frame_buffer, int frame_idx);

		void Render_MeshNodes(temp::MeshBatches& batches, CameraNode* camera, CommandList* cmd_list);
		void BindMaterial(MaterialHandle* material_handle, CommandList* cmd_list);

		unsigned int GetFrameIdx();
		d3d12::RenderWindow* GetRenderWindow();

	public:
		d3d12::Device* m_device;
		std::optional<d3d12::RenderWindow*> m_render_window;
		d3d12::CommandQueue* m_direct_queue;
		d3d12::CommandQueue* m_compute_queue;
		d3d12::CommandQueue* m_copy_queue;
		std::array<d3d12::Fence*, d3d12::settings::num_back_buffers> m_fences;

		d3d12::Viewport m_viewport;
		d3d12::CommandList* m_direct_cmd_list;
		d3d12::StagingBuffer* m_fullscreen_quad_vb;

		std::vector<std::uint64_t> m_buffer_frame_graph_uids;

		std::shared_ptr<TexturePool> m_texture_pool;

		d3d12::HeapResource* m_light_buffer;
		std::shared_ptr<ConstantBufferPool> m_camera_pool; // TODO: Should be part of the scene graph

		std::shared_ptr<ConstantBufferPool> m_raytracing_cb_pool;
		std::shared_ptr<StructuredBufferPool> m_raytracing_material_sb_pool;

		std::vector<std::shared_ptr<D3D12StructuredBufferPool>> m_structured_buffer_pools;
		std::vector<std::shared_ptr<D3D12ModelPool>> m_model_pools;
		D3D12ModelPool* m_bound_model_pool;
		std::size_t m_bound_model_pool_stride;
    
		float temp_metal = 1.0f;
		float temp_rough = 0.45f;
		bool clear_path = false;
		float light_radius = 50;
		float temp_intensity = 1;
	private:

		d3d12::IndirectCommandBuffer* m_indirect_cmd_buffer;
		d3d12::IndirectCommandBuffer* m_indirect_cmd_buffer_indexed;
		d3d12::CommandSignature* m_cmd_signature;
		d3d12::CommandSignature* m_cmd_signature_indexed;

		std::optional<bool> m_requested_fullscreen_state;

		MaterialHandle* m_last_material = nullptr;
	};

} /* wr */
