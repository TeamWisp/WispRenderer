#include "d3d12_renderer.hpp"

#include "../util/defines.hpp"
#include "../util/log.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../frame_graph/frame_graph.hpp"
#include "../window.hpp"

#include "d3d12_resource_pool.hpp"
#include "d3d12_functions.hpp"

LINK_NODE_FUNCTION(wr::D3D12RenderSystem, wr::MeshNode, Init_MeshNode, Render_MeshNode)
LINK_NODE_FUNCTION(wr::D3D12RenderSystem, wr::AnimNode, Init_AnimNode, Render_AnimNode)

namespace wr
{
	D3D12RenderSystem::~D3D12RenderSystem()
	{
		delete m_device;
		delete m_default_queue;
		if (m_render_window.has_value()) delete m_render_window.value();
	}

	void D3D12RenderSystem::Init(std::optional<Window*> window)
	{
		m_device = d3d12::CreateDevice();
		m_default_queue = d3d12::CreateCommandQueue(m_device, d3d12::CmdListType::CMD_LIST_DIRECT);

		if (window.has_value())
		{
			m_render_window = d3d12::CreateRenderWindow(m_device, window.value()->GetWindowHandle(), m_default_queue, d3d12::settings::num_back_buffers);
		}
	}

	std::unique_ptr<Texture> D3D12RenderSystem::Render(std::shared_ptr<SceneGraph> const & scene_graph, fg::FrameGraph& frame_graph)
	{
		using recursive_func_t = std::function<void(std::shared_ptr<Node>)>;
		recursive_func_t recursive_render = [this, &recursive_render](std::shared_ptr<Node> const & parent)
		{
			for (auto & node : parent->m_children)
			{
				node->Render(this, node.get());
				recursive_render(node);
			}
		};

		frame_graph.Execute(*this, *scene_graph.get());

		recursive_render(scene_graph->GetRootNode());

		if (m_render_window.has_value())
		{
			d3d12::Present(m_render_window.value(), m_default_queue, m_device);
		}

		return std::unique_ptr<Texture>();
	}

	void D3D12RenderSystem::Resize(std::int32_t width, std::int32_t height)
	{
	}

	std::shared_ptr<MaterialPool> D3D12RenderSystem::CreateMaterialPool(std::size_t size_in_mb)
	{
		return std::make_shared<D3D12MaterialPool>(size_in_mb);
	}

	std::shared_ptr<ModelPool> D3D12RenderSystem::CreateModelPool(std::size_t size_in_mb)
	{
		return std::make_shared<D3D12ModelPool>(size_in_mb);
	}

	void D3D12RenderSystem::Init_MeshNode(MeshNode* node)
	{
		LOG("init mesh");
	}

	void D3D12RenderSystem::Init_AnimNode(AnimNode* node)
	{
		LOG("init anim");
	}

	void D3D12RenderSystem::Render_MeshNode(MeshNode* node)
	{
		LOG("render mesh");
	}

	void D3D12RenderSystem::Render_AnimNode(AnimNode* node)
	{
		LOG("render anim");
	}

} /*  */