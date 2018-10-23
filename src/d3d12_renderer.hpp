#pragma once

#include "renderer.hpp"

namespace wr
{

	class D3D12RenderSystem : public RenderSystem
	{
	public:
		D3D12RenderSystem();
		virtual ~D3D12RenderSystem();

		virtual void Init(std::optional<std::shared_ptr<Window>> const & window) final;
		virtual std::unique_ptr<Texture> Render(std::shared_ptr<SceneGraph> const & scene_graph, fg::FrameGraph & frame_graph) final;
		virtual void Resize(std::int32_t width, std::int32_t height) final;

		virtual std::shared_ptr<MaterialPool> CreateMaterialPool(std::size_t size_in_mb) final;
		virtual std::shared_ptr<ModelPool> CreateModelPool(std::size_t size_in_mb) final;

		void Init_MeshNode(wr::MeshNode* node);
		void Init_AnimNode(wr::AnimNode* node);

		void Render_MeshNode(wr::MeshNode* node);
		void Render_AnimNode(wr::AnimNode* node);

	private:

	};

} /* wr */