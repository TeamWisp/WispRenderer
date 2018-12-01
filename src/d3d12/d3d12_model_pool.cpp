#include "d3d12_model_pool.hpp"

#include "../util/bitmap_allocator.hpp"

#include "d3d12_functions.hpp"
#include "d3d12_defines.hpp"
#include "d3d12_renderer.hpp"

namespace wr
{
	D3D12ModelPool::D3D12ModelPool(D3D12RenderSystem& render_system,
		std::size_t vertex_buffer_size_in_mb,
		std::size_t index_buffer_size_in_mb) :
		ModelPool(vertex_buffer_size_in_mb, index_buffer_size_in_mb),
		m_render_system(render_system)
	{
		m_vertex_buffer = d3d12::CreateStagingBuffer(render_system.m_device,
			nullptr,
			vertex_buffer_size_in_mb * 1024 * 1024,
			sizeof(Vertex),
			ResourceState::VERTEX_AND_CONSTANT_BUFFER);

		m_vertex_buffer_size = vertex_buffer_size_in_mb * 1024 * 1024;

		m_index_buffer = d3d12::CreateStagingBuffer(render_system.m_device,
			nullptr,
			index_buffer_size_in_mb * 1024 * 1024,
			sizeof(std::uint32_t),
			ResourceState::INDEX_BUFFER);

		m_index_buffer_size = index_buffer_size_in_mb * 1024 * 1024;

		auto page_frame_count = SizeAlign(m_vertex_buffer_size / 65536, 64) / 64;
		auto frame_count = m_vertex_buffer_size / 65536;

		m_vertex_buffer_bitmap.resize(page_frame_count);

		for (auto& value : m_vertex_buffer_bitmap)
		{
			value = 0xffffffffffffffff;
		}
		for (int i = 0; i < 64; ++i)
		{
			if ((m_vertex_buffer_bitmap.size() - 1) * 64 + i < frame_count)
			{
				m_vertex_buffer_bitmap[m_vertex_buffer_bitmap.size() - 1] |= 1Ui64 << i;
			}
			else
			{
				break;
			}
		}

		page_frame_count = SizeAlign(m_index_buffer_size / 65536, 64) / 64;
		frame_count = m_index_buffer_size / 65536;

		m_index_buffer_bitmap.resize(page_frame_count);

		for (auto& value : m_index_buffer_bitmap)
		{
			value = 0xffffffffffffffff;
		}
		for (int i = 0; i < 64; ++i)
		{
			if ((m_index_buffer_bitmap.size() - 1) * 64 + i < frame_count)
			{
				m_index_buffer_bitmap[m_index_buffer_bitmap.size() - 1] |= 1Ui64 << i;
			}
			else
			{
				break;
			}
		}
	}

	D3D12ModelPool::~D3D12ModelPool()
	{
		d3d12::Destroy(m_vertex_buffer);
		d3d12::Destroy(m_index_buffer);

		for (auto& handle : m_mesh_handles)
		{
			delete handle;
		}
	}

	void D3D12ModelPool::Evict()
	{
		d3d12::Evict(m_vertex_buffer);
		d3d12::Evict(m_index_buffer);
	}

	void D3D12ModelPool::MakeResident()
	{
		d3d12::MakeResident(m_vertex_buffer);
		d3d12::MakeResident(m_index_buffer);
	}

	void D3D12ModelPool::StageMeshes(d3d12::CommandList * cmd_list)
	{
		while (!m_mesh_stage_queue.empty())
		{
			D3D12Mesh* d3d12_mesh = static_cast<D3D12Mesh*>(m_mesh_stage_queue.front());
			d3d12::StageBufferRegion(m_vertex_buffer,
				d3d12_mesh->m_vertex_staging_buffer_size,
				d3d12_mesh->m_vertex_staging_buffer_offset,
				cmd_list);
			if (d3d12_mesh->m_index_staging_buffer_size != 0)
			{
				d3d12::StageBufferRegion(m_index_buffer,
					d3d12_mesh->m_index_staging_buffer_size,
					d3d12_mesh->m_index_staging_buffer_offset,
					cmd_list);
			}
			m_mesh_stage_queue.pop();
		}
	}

	d3d12::StagingBuffer * D3D12ModelPool::GetVertexStagingBuffer()
	{
		return m_vertex_buffer;
	}

	d3d12::StagingBuffer * D3D12ModelPool::GetIndexStagingBuffer()
	{
		return m_index_buffer;
	}

	Model* D3D12ModelPool::LoadFBX(std::string_view path, ModelType type)
	{
		return new Model();
	}

	Mesh* D3D12ModelPool::LoadCustom_VerticesAndIndices(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size, void* indices_data, std::size_t num_indices, std::size_t index_size)
	{
		D3D12Mesh* mesh = new D3D12Mesh();
		memset(mesh, 0, sizeof(D3D12Mesh));
		mesh->m_model_pool = this;

		//Allocate vertex buffer memory

		//Get maximum amount of memory pages
		auto vertex_frame_count = m_vertex_buffer_size / 65536;
		//Get amount of needed memory pages
		auto vertex_needed_frames = SizeAlign(num_vertices*vertex_size, 65536) / 65536;

		// Find Free Page
		auto vertex_start_frame = util::FindFreePage(m_vertex_buffer_bitmap, vertex_frame_count, vertex_needed_frames);

		// Check if we found a page.
		if (!vertex_start_frame.has_value())
		{
			//We haven't found enough pages, so delete the mesh and return a nullptr
			delete mesh;
			return nullptr;
		}


		//Repeat the same procedure as before, but now for the index buffer

		auto index_frame_count = m_index_buffer_size / 65536;
		auto index_needed_frames = SizeAlign(num_indices*index_size, 65536) / 65536;

		auto index_start_frame = util::FindFreePage(m_index_buffer_bitmap, index_frame_count, index_needed_frames);

		if (!index_start_frame.has_value())
		{
			delete mesh;
			return nullptr;
		}

		//Memory for both the vertex and index buffer were found

		//Mark all pages occupied by the new vertex buffer
		for (std::uint64_t i = 0; i < vertex_needed_frames; ++i)
		{
			util::ClearPage(m_vertex_buffer_bitmap, vertex_start_frame.value() + i);
		}

		//Mark all pages occupied by the new index buffer
		for (std::uint64_t i = 0; i < vertex_needed_frames; ++i)
		{
			util::ClearPage(m_index_buffer_bitmap, index_start_frame.value() + i);
		}

		//Store the offset of the allocated memory from the start of the staging buffer
		mesh->m_vertex_staging_buffer_offset = vertex_start_frame.value() * 65536;
		mesh->m_vertex_staging_buffer_size = num_vertices * vertex_size;
		mesh->m_vertex_staging_buffer_stride = vertex_size;
		mesh->m_vertex_count = num_vertices;

		mesh->m_vertex_buffer_base_address = m_vertex_buffer->m_gpu_address;

		//Send the vertex data to the vertex staging buffer
		d3d12::UpdateStagingBuffer(m_vertex_buffer, vertices_data, num_vertices*vertex_size, mesh->m_vertex_staging_buffer_offset);

		//Store the offset of the allocated memory from the start of the staging buffer
		mesh->m_index_staging_buffer_offset = index_start_frame.value() * 65536;
		mesh->m_index_staging_buffer_size = num_indices * index_size;
		mesh->m_index_count = num_indices;

		mesh->m_index_buffer_base_address = m_index_buffer->m_gpu_address;

		//Send the index data to the index staging buffer
		d3d12::UpdateStagingBuffer(m_index_buffer, indices_data, num_indices*index_size, mesh->m_index_staging_buffer_offset);

		//Store the handle to the mesh
		m_mesh_handles.push_back(mesh);

		m_mesh_stage_queue.push(mesh);

		return mesh;
	}

	Mesh* D3D12ModelPool::LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size)
	{
		D3D12Mesh* mesh = new D3D12Mesh();
		memset(mesh, 0, sizeof(D3D12Mesh));
		mesh->m_model_pool = this;

		//Allocate vertex buffer memory

		//Get maximum amount of memory pages
		auto vertex_frame_count = m_vertex_buffer_size / 65536;
		//Get amount of needed memory pages
		auto vertex_needed_frames = SizeAlign(num_vertices*vertex_size, 65536) / 65536;

		// Find Free Page
		auto vertex_start_frame = util::FindFreePage(m_vertex_buffer_bitmap, vertex_frame_count, vertex_needed_frames);

		//The loop has exited, see if we've found enough free pages
		if (!vertex_start_frame.has_value())
		{
			//We haven't found enough pages, so delete the mesh and return a nullptr
			delete mesh;
			return nullptr;
		}

		//Mark the allocated pages as occupied
		for (std::uint64_t i = 0; i < vertex_needed_frames; ++i)
		{
			util::ClearPage(m_vertex_buffer_bitmap, vertex_start_frame.value() + i);
		}

		//Store the offset of the allocated memory from the start of the staging buffer
		mesh->m_vertex_staging_buffer_offset = vertex_start_frame.value() * 65536;
		mesh->m_vertex_staging_buffer_size = num_vertices * vertex_size;
		mesh->m_vertex_staging_buffer_stride = vertex_size;
		mesh->m_vertex_count = num_vertices;

		mesh->m_vertex_buffer_base_address = m_vertex_buffer->m_gpu_address;

		//Send the vertex data to the vertex staging buffer
		d3d12::UpdateStagingBuffer(m_vertex_buffer, vertices_data, num_vertices*vertex_size, mesh->m_vertex_staging_buffer_offset);

		//Store the mesh handle
		m_mesh_handles.push_back(mesh);

		m_mesh_stage_queue.push(mesh);

		return mesh;
	}

	void D3D12ModelPool::DestroyModel(Model * model)
	{
		for (auto& mesh : model->m_meshes)
		{
			DestroyMesh(mesh);
		}

		//TODO: Destroy possible materials owned by model. Material might be used by multiple models, use ref counts?

		delete model;
	}

	void D3D12ModelPool::DestroyMesh(Mesh * mesh)
	{
		//Check for null pointers
		if (mesh == nullptr)
			return;

		//Check if the mesh was allocated from this pool
		if (mesh->m_model_pool != this)
			return;

		m_mesh_handles.erase(std::remove(m_mesh_handles.begin(), m_mesh_handles.end(), mesh), m_mesh_handles.end());

		D3D12Mesh* n_mesh = static_cast<D3D12Mesh*>(mesh);

		//Find at which page in memory the mesh vertex data starts.
		std::uint64_t frame = n_mesh->m_vertex_staging_buffer_offset / 65536;
		//Find how much pages are occupied by the mesh vertex data
		std::uint64_t frame_count = SizeAlign(n_mesh->m_vertex_staging_buffer_size, 65536) / 65536;

		//Mark all occupied pages as free
		for (int i = 0; i < frame_count; ++i)
		{
			util::SetPage(m_vertex_buffer_bitmap, frame + i);
		}

		//Does the mesh also have an index buffer?
		if (n_mesh->m_index_buffer_base_address != 0)
		{
			//Find at which page in memory the mesh index data starts
			std::uint64_t frame = n_mesh->m_index_staging_buffer_offset / 65536;
			//Find how much pages are occupied by the mesh index data
			std::uint64_t frame_count = SizeAlign(n_mesh->m_index_staging_buffer_size, 65536) / 65536;

			//Mark all occupied pages as free
			for (int i = 0; i < frame_count; ++i)
			{
				util::SetPage(m_index_buffer_bitmap, frame + i);
			}

		}

		//Delete the mesh
		delete mesh;
	}

} /* wr */