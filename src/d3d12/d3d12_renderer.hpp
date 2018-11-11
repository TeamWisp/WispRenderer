#pragma once

#include "../renderer.hpp"

#include "d3d12_structs.hpp"

namespace wr
{

	class D3D12RenderSystem : public RenderSystem
	{
	public:
		virtual ~D3D12RenderSystem();

		virtual void Init(std::optional<Window*> window) final;
		virtual std::unique_ptr<Texture> Render(std::shared_ptr<SceneGraph> const & scene_graph, fg::FrameGraph & frame_graph) final;
		virtual void Resize(std::int32_t width, std::int32_t height) final;

		virtual std::shared_ptr<MaterialPool> CreateMaterialPool(std::size_t size_in_mb) final;
		virtual std::shared_ptr<ModelPool> CreateModelPool(std::size_t size_in_mb) final;

		void Init_MeshNode(wr::MeshNode* node);
		void Init_AnimNode(wr::AnimNode* node);

		void Render_MeshNode(wr::MeshNode* node);
		void Render_AnimNode(wr::AnimNode* node);

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
		d3d12::Viewport m_viewport;
		d3d12::CommandList* m_direct_cmd_list;
		d3d12::PipelineState* m_pipeline_state;
		d3d12::RootSignature* m_root_signature;
		d3d12::Shader* m_vertex_shader;
		d3d12::Shader* m_pixel_shader;
		d3d12::StagingBuffer* m_vertex_buffer;
	};

} /* wr */