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
	}
  
	struct MeshNode;
	struct CameraNode;
	struct D3D12ConstantBufferHandle;

	class D3D12StructuredBufferPool;
	class D3D12ModelPool;

	namespace temp
	{
		struct ProjectionView_CBData
		{
			DirectX::XMMATRIX m_view;
			DirectX::XMMATRIX m_projection;
			DirectX::XMMATRIX m_inverse_projection;
		};

		static const constexpr float size = 1.0f;
		static const constexpr Vertex2D quad_vertices[] = {
			{ -size, -size },
			{ size, -size },
			{ -size, size },
			{ size, size },
		};

	} /* temp */

	//! D3D12 platform independend Command List implementation
	struct D3D12CommandList : CommandList, d3d12::CommandList {};

	//! D3D12 platform independend Render Target implementation
	struct D3D12RenderTarget : RenderTarget, d3d12::RenderTarget {};

	class D3D12RenderSystem : public RenderSystem
	{
	public:
		~D3D12RenderSystem() final;

		void Init(std::optional<Window*> window) final;
		std::unique_ptr<Texture> Render(std::shared_ptr<SceneGraph> const & scene_graph, FrameGraph & frame_graph) final;
		void Resize(std::uint32_t width, std::uint32_t height) final;

		std::shared_ptr<MaterialPool> CreateMaterialPool(std::size_t size_in_mb) final;
		std::shared_ptr<ModelPool> CreateModelPool(std::size_t vertex_buffer_pool_size_in_mb, std::size_t index_buffer_pool_size_in_mb) final;
		std::shared_ptr<ConstantBufferPool> CreateConstantBufferPool(std::size_t size_in_mb) final;
		std::shared_ptr<StructuredBufferPool> CreateStructuredBufferPool(std::size_t size_in_mb) final;

		void PrepareRootSignatureRegistry() final;
		void PrepareShaderRegistry() final;
		void PreparePipelineRegistry() final;

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

		void InitSceneGraph(SceneGraph& scene_graph);

		void Init_MeshNodes(std::vector<std::shared_ptr<MeshNode>>& nodes);
		void Init_CameraNodes(std::vector<std::shared_ptr<CameraNode>>& nodes);
		void Init_LightNodes(std::vector<std::shared_ptr<LightNode>>& nodes, std::vector<Light>& lights);

		void Update_MeshNodes(std::vector<std::shared_ptr<MeshNode>>& nodes);
		void Update_CameraNodes(std::vector<std::shared_ptr<CameraNode>>& nodes);
		void Update_LightNodes(std::vector<std::shared_ptr<LightNode>>& nodes, std::vector<Light>& lights, StructuredBufferHandle* structured_buffer, CommandList* cmd_list);

		void Render_MeshNodes(temp::MeshBatches& batches, CommandList* cmd_list);

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

		std::shared_ptr<ConstantBufferPool> m_camera_pool;

		std::vector<std::shared_ptr<D3D12StructuredBufferPool>> m_structured_buffer_pools;
		std::vector<std::shared_ptr<D3D12ModelPool>> m_model_pools;
		D3D12ModelPool* m_bound_model_pool;
		std::size_t m_bound_model_pool_stride;
    
	private:
		std::optional<bool> m_requested_fullscreen_state;
	};

} /* wr */