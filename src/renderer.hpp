#pragma once

#include <optional>
#include <memory>

#include "engine_registry.hpp"
#include "platform_independend_structs.hpp"

namespace wr
{

	struct TextureHandle;

	class Window;
	class SceneGraph;
	class TexturePool;
	class MaterialPool;
	class ModelPool;
	class ConstantBufferPool;
	class StructuredBufferPool;
	class FrameGraph;

	class RenderSystem
	{
	public:
		RenderSystem() = default;
		virtual ~RenderSystem() = default;

		RenderSystem(RenderSystem const &) = delete;
		RenderSystem& operator=(RenderSystem const &) = delete;
		RenderSystem(RenderSystem&&) = delete;
		RenderSystem& operator=(RenderSystem&&) = delete;

		virtual std::shared_ptr<TexturePool> CreateTexturePool(std::size_t size_in_mb, std::size_t num_of_textures) = 0;
		virtual std::shared_ptr<MaterialPool> CreateMaterialPool(std::size_t size_in_mb) = 0;
		virtual std::shared_ptr<ModelPool> CreateModelPool(std::size_t vertex_buffer_pool_size_in_mb, std::size_t index_buffer_pool_size_in_mb) = 0;
		virtual std::shared_ptr<ConstantBufferPool> CreateConstantBufferPool(std::size_t size_in_mb) = 0;
		virtual std::shared_ptr<StructuredBufferPool> CreateStructuredBufferPool(std::size_t size_in_mb) = 0;

		virtual void PrepareRootSignatureRegistry() = 0;
		virtual void PrepareShaderRegistry() = 0;
		virtual void PreparePipelineRegistry() = 0;
		virtual void PrepareRTPipelineRegistry() = 0;

		virtual void WaitForAllPreviousWork() = 0;

		virtual CommandList* GetDirectCommandList(unsigned int num_allocators) = 0;
		virtual CommandList* GetBundleCommandList(unsigned int num_allocators) = 0;
		virtual CommandList* GetComputeCommandList(unsigned int num_allocators) = 0;
		virtual CommandList* GetCopyCommandList(unsigned int num_allocators) = 0;
		virtual RenderTarget* GetRenderTarget(RenderTargetProperties properties) = 0;
		virtual void ResizeRenderTarget(RenderTarget** render_target, std::uint32_t width, std::uint32_t height) = 0;

		virtual void StartRenderTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target) = 0;
		virtual void StopRenderTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target) = 0;
		virtual void StartComputeTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target) = 0;
		virtual void StopComputeTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target) = 0;
		virtual void StartCopyTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target) = 0;
		virtual void StopCopyTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target) = 0;

		virtual void Init(std::optional<Window*> window) = 0;
		virtual std::unique_ptr<TextureHandle> Render(std::shared_ptr<SceneGraph> const & scene_graph, FrameGraph & frame_graph) = 0;
		virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
		
		std::optional<Window*> m_window;
	};

} /* wr */