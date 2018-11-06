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

	private:
		d3d12::Device* m_device;
		std::optional<d3d12::RenderWindow*> m_render_window;
		d3d12::CommandQueue* m_default_queue;

	};

} /* wr */