#pragma once

#include <optional>
#include <memory>

#include "engine_registry.hpp"

namespace wr
{
	class FrameGraph;
} /* wr */

namespace wr
{

	class Texture {};
	class Window;
	class SceneGraph;
	class MaterialPool;
	class ModelPool;

	class RenderSystem
	{
	public:
		RenderSystem() = default;
		virtual ~RenderSystem() = default;

		RenderSystem(RenderSystem const &) = delete;
		RenderSystem& operator=(RenderSystem const &) = delete;
		RenderSystem(RenderSystem&&) = delete;
		RenderSystem& operator=(RenderSystem&&) = delete;

		virtual std::shared_ptr<MaterialPool> CreateMaterialPool(std::size_t size_in_mb) = 0;
		virtual std::shared_ptr<ModelPool> CreateModelPool(std::size_t size_in_mb) = 0;

		virtual void PrepareRootSignatureRegistry() = 0;
		virtual void PrepareShaderRegistry() = 0;
		virtual void PreparePipelineRegistry() = 0;

		virtual void Init(std::optional<Window*> window) = 0;
		virtual std::unique_ptr<Texture> Render(std::shared_ptr<SceneGraph> const & scene_graph, FrameGraph & frame_graph) = 0;
		virtual void Resize(std::int32_t width, std::int32_t height) = 0;
		
		std::optional<Window*> m_window;
	};

} /* wr */