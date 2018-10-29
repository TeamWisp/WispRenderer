#pragma once

#include <optional>
#include <memory>

#define LINK_NODE_FUNCTION(renderer_type, node_type, init_function, render_function) \
decltype(node_type::init_func_impl) node_type::init_func_impl = [](wr::RenderSystem* render_system, Node* node) \
{ \
	static_cast<renderer_type*>(render_system)->init_function(static_cast<node_type*>(node)); \
}; \
decltype(node_type::render_func_impl) node_type::render_func_impl = [](wr::RenderSystem* render_system, Node* node) \
{ \
	static_cast<renderer_type*>(render_system)->render_function(static_cast<node_type*>(node)); \
};

namespace wr::fg
{
	class FrameGraph;
}

namespace wr
{

	class Texture {};
	class Window;
	class SceneGraph;
	struct MeshNode;
	struct AnimNode;
	class MaterialPool;
	class ModelPool;

	class RenderSystem
	{
	public:
		RenderSystem();
		virtual ~RenderSystem();

		RenderSystem(RenderSystem const &) = delete;
		//RenderSystem& operator=(RenderSystem const &) = delete;
		RenderSystem(RenderSystem&&) = delete;
		//RenderSystem& operator=(RenderSystem&&) = delete;

		virtual std::shared_ptr<MaterialPool> CreateMaterialPool(std::size_t size_in_mb) = 0;
		virtual std::shared_ptr<ModelPool> CreateModelPool(std::size_t size_in_mb) = 0;

		virtual void Init(std::optional<std::shared_ptr<Window>> const & window) = 0;
		virtual std::unique_ptr<Texture> Render(std::shared_ptr<SceneGraph> const & scene_graph, fg::FrameGraph & frame_graph) = 0;
		virtual void Resize(std::int32_t width, std::int32_t height) = 0;

	private:

	};

} /* wr */