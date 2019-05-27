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
		struct ProjectionView_CBData
		{
			DirectX::XMMATRIX m_view = DirectX::XMMatrixIdentity();
			DirectX::XMMATRIX m_projection = DirectX::XMMatrixIdentity();
			DirectX::XMMATRIX m_inverse_projection = DirectX::XMMatrixIdentity();
			DirectX::XMMATRIX m_inverse_view = DirectX::XMMatrixIdentity();
			DirectX::XMMATRIX m_prev_projection = DirectX::XMMatrixIdentity();
			DirectX::XMMATRIX m_prev_view = DirectX::XMMatrixIdentity();
			unsigned int m_is_hybrid = 0u;
			unsigned int m_is_path_tracer = 0u;
			unsigned int m_is_ao = 0u;
			unsigned int m_has_shadows = 0u;
			unsigned int m_has_reflections = 0u;
			int padding[3];
		};

		struct ShadowDenoiserSettings_CBData
		{
			float m_alpha;
			float m_moments_alpha;
			float m_l_phi;
			float m_n_phi;
			float m_z_phi;
			float m_step_distance;
			float m_padding[2];
		};

		struct DenoiserCamera_CBData
		{
			DirectX::XMMATRIX m_view;
			DirectX::XMMATRIX m_prev_view;
			DirectX::XMMATRIX m_inverse_view;
			DirectX::XMMATRIX m_projection;
			DirectX::XMMATRIX m_prev_projection;
			DirectX::XMMATRIX m_inverse_projection;
			float m_padding[2];
			float m_near_plane;
			float m_far_plane;
		};

		struct RTHybridCamera_CBData
		{
			DirectX::XMMATRIX m_inverse_view = DirectX::XMMatrixIdentity();
			DirectX::XMMATRIX m_inverse_projection = DirectX::XMMatrixIdentity();
			DirectX::XMMATRIX m_inv_vp = DirectX::XMMatrixIdentity();

			uint32_t m_padding[2] = { 0u };
			float m_frame_idx = 0.0f;
			float m_intensity = 0.0f;
		};

		struct RTAO_CBData
		{
			DirectX::XMMATRIX m_inv_vp = DirectX::XMMatrixIdentity();

			float bias = 0.0f;
			float radius = 0.0f;
			float power = 0.0f;
			std::uint32_t sample_count = 0u;
		};

		struct RayTracingCamera_CBData
		{
			DirectX::XMMATRIX m_view = DirectX::XMMatrixIdentity();
			DirectX::XMMATRIX m_inverse_view_projection = DirectX::XMMatrixIdentity();
			DirectX::XMVECTOR m_camera_position = DirectX::XMVectorZero();

			float light_radius = 0.0f;
			float metal = 0.0f;
			float roughness = 0.0f;
			float intensity = 0.0f;
		};

		struct RayTracingMaterial_CBData
		{
			std::uint32_t albedo_id = 0u;
			std::uint32_t normal_id = 0u;
			std::uint32_t roughness_id = 0u;
			std::uint32_t metallicness_id = 0u;
			std::uint32_t emissive_id = 0u;
			std::uint32_t ao_id = 0u;

			std::uint32_t padding[2] = { 0u };
			Material::MaterialData material_data;
		};

		struct RayTracingOffset_CBData
		{
			std::uint32_t material_idx = 0u;
			std::uint32_t idx_offset = 0u;
			std::uint32_t vertex_offset = 0u;
		};



		static const constexpr float size = 1.0f;
		static const constexpr Vertex2D quad_vertices[] = {
			{ -size, -size },
			{ size, -size },
			{ -size, size },
			{ size, size },
		};

	} /* temp */

	class D3D12RenderSystem final : public RenderSystem
	{
	public:
		~D3D12RenderSystem();

		void Init(std::optional<Window*> window);
		CPUTextures Render(SceneGraph& scene_graph, FrameGraph& frame_graph);
		void Resize(std::uint32_t width, std::uint32_t height);

		std::shared_ptr<TexturePool> CreateTexturePool();
		std::shared_ptr<MaterialPool> CreateMaterialPool(std::size_t size_in_bytes);
		std::shared_ptr<ModelPool> CreateModelPool(std::size_t vertex_buffer_pool_size_in_bytes, std::size_t index_buffer_pool_size_in_bytes);
		std::shared_ptr<ConstantBufferPool> CreateConstantBufferPool(std::size_t size_in_bytes);
		std::shared_ptr<StructuredBufferPool> CreateStructuredBufferPool(std::size_t size_in_bytes);

		std::shared_ptr<TexturePool> GetDefaultTexturePool();

		void PrepareRootSignatureRegistry();
		void PrepareShaderRegistry();
		void PreparePipelineRegistry();
		void PrepareRTPipelineRegistry();
		void ReloadPipelineRegistryEntry(RegistryHandle handle);
		void ReloadRTPipelineRegistryEntry(RegistryHandle handle);
		void ReloadShaderRegistryEntry(RegistryHandle handle);
		void ReloadRootSignatureRegistryEntry(RegistryHandle handle);
		void DestroyRootSignatureRegistry();
		void DestroyShaderRegistry();
		void DestroyPipelineRegistry();
		void DestroyRTPipelineRegistry();

		void WaitForAllPreviousWork();

		wr::CommandList* GetDirectCommandList(unsigned int num_allocators);
		wr::CommandList* GetBundleCommandList(unsigned int num_allocators);
		wr::CommandList* GetComputeCommandList(unsigned int num_allocators);
		wr::CommandList* GetCopyCommandList(unsigned int num_allocators);
		void DestroyCommandList(CommandList* cmd_list);
		RenderTarget* GetRenderTarget(RenderTargetProperties properties);
		void ResizeRenderTarget(RenderTarget** render_target, std::uint32_t width, std::uint32_t height);
		void RequestFullscreenChange(bool fullscreen_state);

		void ResetCommandList(CommandList* cmd_list);
		void CloseCommandList(CommandList* cmd_list);
		void StartRenderTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target);
		void StopRenderTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target);
		void StartComputeTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target);
		void StopComputeTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target);
		void StartCopyTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target);
		void StopCopyTask(CommandList* cmd_list, std::pair<RenderTarget*, RenderTargetProperties> render_target);

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
		void BindMaterial(MaterialHandle material_handle, CommandList* cmd_list);

		unsigned int GetFrameIdx();
		d3d12::RenderWindow* GetRenderWindow();

		//SimpleShapes don't have a material attached to them. The user is expected to provide one.
		wr::Model* GetSimpleShape(SimpleShapes type);

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

		std::vector <std::shared_ptr<TexturePool>> m_texture_pools;

		d3d12::HeapResource* m_light_buffer;
		std::shared_ptr<ConstantBufferPool> m_camera_pool; // TODO: Should be part of the scene graph

		std::shared_ptr<ConstantBufferPool> m_raytracing_cb_pool;
		std::shared_ptr<StructuredBufferPool> m_raytracing_material_sb_pool;
		std::shared_ptr<StructuredBufferPool> m_raytracing_offset_sb_pool;

		std::vector<std::shared_ptr<D3D12StructuredBufferPool>> m_structured_buffer_pools;
		std::vector<std::shared_ptr<D3D12ModelPool>> m_model_pools;
		D3D12ModelPool* m_bound_model_pool;
		std::size_t m_bound_model_pool_stride;

		std::optional<wr::TextureHandle> m_brdf_lut = std::nullopt;
		bool m_brdf_lut_generated = false;
    
		float temp_metal = 1.0f;
		float temp_rough = -3;
		bool clear_path = false;
		float light_radius = 50;
		float temp_intensity = 1;

	protected:
		void SaveRenderTargetToDisc(std::string const& path, RenderTarget* render_target, unsigned int index);

	private:
		void ResetBatches(SceneGraph& sg);
		void LoadPrimitiveShapes();

		d3d12::CommandSignature* m_cmd_signature;
		d3d12::CommandSignature* m_cmd_signature_indexed;

		std::optional<bool> m_requested_fullscreen_state;

		MaterialHandle m_last_material = { nullptr, 0 };

	};

} /* wr */
