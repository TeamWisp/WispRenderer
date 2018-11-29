#include "../d3d12/d3d12_structs.hpp"
#include "../d3d12/d3d12_functions.hpp"
#include "../renderer.hpp"
#include "../d3d12/d3d12_renderer.hpp"
#include "../d3d12/d3d12_resource_pool_model.hpp"
#include "../window.hpp"
/*
TEST(ModelResourcePoolTest, ResourcePoolCreation) 
{
	auto render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

	window->SetKeyCallback([](int key, int action, int mods)
	{

	});

	render_system->Init(window.get());

	std::shared_ptr<wr::ModelPool> model_pool = render_system->CreateModelPool(2, 1);

	EXPECT_EQ(static_cast<wr::D3D12ModelPool*>(model_pool.get())->GetVertexStagingBuffer()->m_size, 2 * 1024 * 1024) <<
		"model pool vertex buffer size is " <<
		static_cast<wr::D3D12ModelPool*>(model_pool.get())->GetVertexStagingBuffer()->m_size <<
		" bytes while expected size is 2*1024*1024";

	EXPECT_EQ(static_cast<wr::D3D12ModelPool*>(model_pool.get())->GetIndexStagingBuffer()->m_size, 1 * 1024 * 1024) <<
		"model pool index buffer size is " <<
		static_cast<wr::D3D12ModelPool*>(model_pool.get())->GetIndexStagingBuffer()->m_size <<
		" bytes while expected size is 1*1024*1024";

	model_pool.reset();
	render_system.reset();
	window.reset();
}
 */
/*
TEST(ModelResourcePoolTest, ResourcePoolAllocDealloc)
{
	auto render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

	window->SetKeyCallback([](int key, int action, int mods)
	{

	});

	render_system->Init(window.get());

	std::shared_ptr<wr::ModelPool> model_pool = render_system->CreateModelPool(2, 1);

	wr::MeshData<wr::Vertex, std::uint32_t> data = {};

	for (int i = 0; i < 64; ++i)
	{
		wr::Vertex vertex = {};
		data.m_vertices.push_back(vertex);
	}

	for (int i = 0; i < 1024; ++i) 
	{
		data.m_indices->push_back(0);
	}

	std::vector<wr::MeshData<wr::Vertex, std::uint32_t>> meshes = { data };
	wr::Model* model = model_pool->LoadCustom(meshes);
	wr::Model* model_2 = model_pool->LoadCustom(meshes);

	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_staging_buffer_offset, 0) <<
		"Model vertex offset is " <<
		static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_staging_buffer_offset <<
		" instead of 0";

	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_index_staging_buffer_offset, 0) <<
		"Model index offset is " <<
		static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_index_staging_buffer_offset <<
		" instead of 0";

	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_count, 64) <<
		"Model vertex count is " <<
		static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_count <<
		" instead of 64";
	
	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_index_count, 1024) <<
		"Model index count is " <<
		static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_index_count <<
		" instead of 1024";

	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_staging_buffer_stride, sizeof(wr::Vertex)) <<
		"Model vertex stride is " <<
		static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_staging_buffer_stride <<
		" instead of " <<
		sizeof(wr::Vertex);
	
	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model_2->m_meshes[0])->m_vertex_staging_buffer_offset, 65536) <<
		"Model vertex offset is " <<
		static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_staging_buffer_offset <<
		" instead of 65536";

	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model_2->m_meshes[0])->m_index_staging_buffer_offset, 65536) <<
		"Model index offset is " <<
		static_cast<wr::D3D12Mesh*>(model_2->m_meshes[0])->m_index_staging_buffer_offset <<
		" instead of 65536";

	model_pool->Destroy(model);

	meshes[0].m_indices.reset();

	model = model_pool->LoadCustom(meshes);

	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_staging_buffer_offset, 0) <<
		"Model vertex offset is " <<
		static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_staging_buffer_offset <<
		" instead of 0";

	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_index_staging_buffer_offset, 0) <<
		"Model index offset is " <<
		static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_index_staging_buffer_offset <<
		" instead of 0";

	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_index_staging_buffer_size, 0) <<
		"Model index buffer size is " <<
		static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_index_staging_buffer_offset <<
		" instead of 0";

	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_count, 64) <<
		"Model vertex count is " <<
		static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_count <<
		" instead of 64";

	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_index_count, 0) <<
		"Model index count is " <<
		static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_index_count <<
		" instead of 0";

	EXPECT_EQ(static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_staging_buffer_stride, sizeof(wr::Vertex)) <<
		"Model vertex stride is " <<
		static_cast<wr::D3D12Mesh*>(model->m_meshes[0])->m_vertex_staging_buffer_stride <<
		" instead of " <<
		sizeof(wr::Vertex);

	model_pool.reset();
	render_system.reset();
	window.reset();
}
*/
