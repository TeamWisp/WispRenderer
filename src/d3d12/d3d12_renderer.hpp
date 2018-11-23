#pragma once

#include "../renderer.hpp"

#include <DirectXMath.h>

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

	namespace temp
	{
		struct ProjectionView_CBData
		{
			DirectX::XMMATRIX m_view;
			DirectX::XMMATRIX m_projection;
		};

		static const constexpr float size = 0.5f;
		static const constexpr Vertex quad_vertices[] = {
			{ -size, -size, 0.f },
			{ size, -size, 0.f },
			{ -size, size, 0.f },
			{ size, size, 0.f },
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
		void Resize(std::int32_t width, std::int32_t height) final;

		std::shared_ptr<MaterialPool> CreateMaterialPool(std::size_t size_in_mb) final;
		std::shared_ptr<ModelPool> CreateModelPool(std::size_t size_in_mb) final;

		void PrepareRootSignatureRegistry() final;
		void PrepareShaderRegistry() final;
		void PreparePipelineRegistry() final;

		wr::CommandList* GetDirectCommandList(unsigned int num_allocators) final;
		wr::CommandList* GetComputeCommandList(unsigned int num_allocators) final;
		wr::CommandList* GetCopyCommandList(unsigned int num_allocators) final;
		RenderTarget* GetRenderTarget(std::optional<std::pair<unsigned int, unsigned int>> size, bool render_window) final;

		void StartRenderTask(CommandList* cmd_list, RenderTarget* render_target) final;
		void StopRenderTask(CommandList* cmd_list, RenderTarget* render_target) final;

		void InitSceneGraph(SceneGraph& scene_graph);
		void RenderSceneGraph(SceneGraph& scene_graph);

		void Init_MeshNode(MeshNode* node);
		void Init_CameraNode(CameraNode* node);

		void Update_MeshNode(MeshNode* node);
		void Update_CameraNode(CameraNode* node);

		void Render_MeshNode(MeshNode* node);
		void Render_CameraNode(CameraNode* node);

		void RenderMeshBatches(SceneGraph& scene_graph);

		unsigned int GetFrameIdx();
		d3d12::RenderWindow* GetRenderWindow();

	public:
		d3d12::Device* m_device;
		std::optional<d3d12::RenderWindow*> m_render_window;
		d3d12::CommandQueue* m_direct_queue;
		d3d12::CommandQueue* m_compute_queue;
		d3d12::CommandQueue* m_copy_queue;
		std::array<d3d12::Fence*, d3d12::settings::num_back_buffers> m_fences;

		// temporary
		d3d12::Heap<HeapOptimization::SMALL_BUFFERS>* m_cb_heap;

		d3d12::Viewport m_viewport;
		d3d12::CommandList* m_direct_cmd_list;
		d3d12::PipelineState* m_pipeline_state;
		d3d12::RootSignature* m_root_signature;
		d3d12::Shader* m_vertex_shader;
		d3d12::Shader* m_pixel_shader;
		d3d12::StagingBuffer* m_vertex_buffer;

	};

} /* wr */