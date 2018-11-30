#include "d3d12_resource_pool_model.hpp"

#include "d3d12_functions.hpp"
#include "d3d12_defines.hpp"
#include "d3d12_renderer.hpp"

namespace wr
{
	namespace internal
	{
		inline std::uint64_t IndexFromBit(std::uint64_t frame) 
		{
			return frame / (8 * 8);
		}

		inline std::uint64_t OffsetFromBit(std::uint64_t frame)
		{
			return frame % (8 * 8);
		}

		void SetPage(std::vector<uint64_t>* bitmap, std::uint64_t frame) 
		{
			std::uint64_t idx = IndexFromBit(frame);
			std::uint64_t off = OffsetFromBit(frame);
			bitmap->operator[](idx) |= (1Ui64 << off);
		}

		void ClearPage(std::vector<uint64_t>* bitmap, std::uint64_t frame)
		{
			std::uint64_t idx = IndexFromBit(frame);
			std::uint64_t off = OffsetFromBit(frame);
			bitmap->operator[](idx) &= ~(1Ui64 << off);
		}

		bool TestPage(std::vector<uint64_t>* bitmap, std::uint64_t frame)
		{
			std::uint64_t idx = IndexFromBit(frame);
			std::uint64_t off = OffsetFromBit(frame);
			return (bitmap->operator[](idx) & (1Ui64 << off));
		}
	}

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

		for (int i = 0; i < m_vertex_buffer_bitmap.size() - 1; ++i)
		{
			m_vertex_buffer_bitmap[i] = 0xffffffffffffffff;
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

		for (int i = 0; i < m_index_buffer_bitmap.size() - 1; ++i) 
		{
			m_index_buffer_bitmap[i] = 0xffffffffffffffff;
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

		for (int i = 0; i < m_mesh_handles.size(); ++i) {
			delete m_mesh_handles[i];
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

	void D3D12ModelPool::StageMesh(Mesh * mesh, d3d12::CommandList * cmd_list)
	{
		D3D12Mesh* d3d12_mesh = static_cast<D3D12Mesh*>(mesh);
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

		std::uint64_t vertex_start_frame = 0;
		bool counting = false;
		bool found = false;
		int free_frames = 0;

		//Go through all all pages to find free ones.
		//Because we're storing the data in a single bit we can compare an entire int64 at once to speed things up
		for (std::uint64_t i = 0; i < m_vertex_buffer_bitmap.size(); ++i) 
		{
			//Check 64 pages at once
			if (m_vertex_buffer_bitmap[i] != 0Ui64)
			{
				//At least a single page is free, so check all 64 pages
				for (std::uint64_t j = 0; j < 64; ++j) 
				{
					//If the current page being checked is larger than the amount of available pages, break
					if (i * 64 + j >= vertex_frame_count)
					{
						break;
					}

					//Set bit corresponding to page being checked in temporary variable
					std::uint64_t to_test = 1Ui64 << j;
					
					//Run an 'and' against the temporary variable to see if the page is available
					if ((m_vertex_buffer_bitmap[i] & to_test)) 
					{
						//Is this the first free page found?
						if (counting) 
						{
							//Not the first free page, so increment the counter of consecutive free pages
							free_frames++;
							//Have we found the nessecary amount of free pages? If so, indicate that and break
							if (free_frames == vertex_needed_frames)
							{
								found = true;
								break;
							}
						}
						else 
						{
							//This is the first free page
							if (vertex_needed_frames == 1)
							{
								//We only need a single page, so store the number of the page and break
								found = true;
								vertex_start_frame = i * 64 + j;
								break;
							}
							else 
							{
								//We need multiple pages, so store the number of this page as the start page and continue
								vertex_start_frame = i * 64 + j;
								counting = true;
								free_frames = 1;
							}
						}
					}
					else
					{
						//The page wasn't free, so stop counting.
						counting = 0;
						free_frames = 0;
					}
				}
			}
			else
			{
				//All 64 pages are occupied, so stop counting
				counting = false;
				free_frames = 0;
			}

			//Have we found enough free pages? If so, break
			if (found == true)
			{
				break;
			}
		}

		//The loop has exited, see if we've found enough free pages
		if (found == false) 
		{
			//We haven't found enough pages, so delete the mesh and return a nullptr
			delete mesh;
			return nullptr;
		}
		

		//Repeat the same procedure as before, but now for the index buffer

		auto index_frame_count = m_index_buffer_size / 65536;
		auto index_needed_frames = SizeAlign(num_indices*index_size, 65536) / 65536;

		std::uint64_t index_start_frame = 0;
		counting = false;
		found = false;
		free_frames = 0;

		for (std::uint64_t i = 0; i < m_index_buffer_bitmap.size(); ++i)
		{
			if (m_index_buffer_bitmap[i] != 0Ui64) 
			{
				for (std::uint64_t j = 0; j < 64; ++j) 
				{
					std::uint64_t to_test = 1Ui64 << j;
					if (i * 64 + j >= index_frame_count)
						break;

					if ((m_index_buffer_bitmap[i] & to_test)) 
					{
						if (counting)
						{
							free_frames++;
							if (free_frames == index_needed_frames) 
							{
								found = true;
								break;
							}
						}
						else
						{
							if (index_needed_frames == 1)
							{
								found = true;
								index_start_frame = i * 64 + j;
								break;
							}
							else 
							{
								index_start_frame = i * 64 + j;
								counting = true;
								free_frames = 1;
							}
						}
					}
					else 
					{
						counting = 0;
						free_frames = 0;
					}
				}
			}
			else 
			{
				counting = false;
				free_frames = 0;
			}
			if (found == true) 
			{
				break;
			}
		}

		if (found == false) 
		{
			delete mesh;
			return nullptr;
		}

		//Memory for both the vertex and index buffer were found

		//Mark all pages occupied by the new vertex buffer
		for (std::uint64_t i = 0; i < vertex_needed_frames; ++i)
		{
			internal::ClearPage(&(m_vertex_buffer_bitmap), vertex_start_frame + i);
		}

		//Mark all pages occupied by the new index buffer
		for (std::uint64_t i = 0; i < vertex_needed_frames; ++i)
		{
			internal::ClearPage(&(m_index_buffer_bitmap), index_start_frame + i);
		}

		//Store the offset of the allocated memory from the start of the staging buffer
		mesh->m_vertex_staging_buffer_offset = vertex_start_frame * 65536;
		mesh->m_vertex_staging_buffer_size = num_vertices * vertex_size;
		mesh->m_vertex_staging_buffer_stride = vertex_size;
		mesh->m_vertex_count = num_vertices;

		mesh->m_vertex_buffer_base_address = m_vertex_buffer->m_gpu_address;

		//Send the vertex data to the vertex staging buffer
		d3d12::UpdateStagingBuffer(m_vertex_buffer, vertices_data, num_vertices*vertex_size, mesh->m_vertex_staging_buffer_offset);

		//Store the offset of the allocated memory from the start of the staging buffer
		mesh->m_index_staging_buffer_offset = index_start_frame * 65536;
		mesh->m_index_staging_buffer_size = num_indices * index_size;
		mesh->m_index_count = num_indices;

		mesh->m_index_buffer_base_address = m_index_buffer->m_gpu_address;

		//Send the index data to the index staging buffer
		d3d12::UpdateStagingBuffer(m_index_buffer, indices_data, num_indices*index_size, mesh->m_index_staging_buffer_offset);

		//Store the handle to the mesh
		m_mesh_handles.push_back(mesh);

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

		std::uint64_t vertex_start_frame = 0;
		bool counting = false;
		bool found = false;
		int free_frames = 0;

		//Go through all all pages to find free ones.
		//Because we're storing the data in a single bit we can compare an entire int64 at once to speed things up
		for (std::uint64_t i = 0; i < m_vertex_buffer_bitmap.size(); ++i)
		{
			//Check 64 pages at once
			if (m_vertex_buffer_bitmap[i] != 0Ui64)
			{
				//At least a single page is free, so check all 64 pages
				for (std::uint64_t j = 0; j < 64; ++j)
				{
					//If the current page being checked is larger than the amount of available pages, break
					if (i * 64 + j >= vertex_frame_count)
					{
						break;
					}

					//Set bit corresponding to page being checked in temporary variable
					std::uint64_t to_test = 1Ui64 << j;

					//Run an 'and' against the temporary variable to see if the page is available
					if ((m_vertex_buffer_bitmap[i] & to_test))
					{
						//Is this the first free page found?
						if (counting)
						{
							//Not the first free page, so increment the counter of consecutive free pages
							free_frames++;
							//Have we found the nessecary amount of free pages? If so, indicate that and break
							if (free_frames == vertex_needed_frames)
							{
								found = true;
								break;
							}
						}
						else
						{
							//This is the first free page
							if (vertex_needed_frames == 1)
							{
								//We only need a single page, so store the number of the page and break
								found = true;
								vertex_start_frame = i * 64 + j;
								break;
							}
							else
							{
								//We need multiple pages, so store the number of this page as the start page and continue
								vertex_start_frame = i * 64 + j;
								counting = true;
								free_frames = 1;
							}
						}
					}
					else
					{
						//The page wasn't free, so stop counting.
						counting = 0;
						free_frames = 0;
					}
				}
			}
			else
			{
				//All 64 pages are occupied, so stop counting
				counting = false;
				free_frames = 0;
			}

			//Have we found enough free pages? If so, break
			if (found == true)
			{
				break;
			}
		}

		//The loop has exited, see if we've found enough free pages
		if (found == false)
		{
			//We haven't found enough pages, so delete the mesh and return a nullptr
			delete mesh;
			return nullptr;
		}

		//Mark the allocated pages as occupied
		for (std::uint64_t i = 0; i < vertex_needed_frames; ++i) 
		{
			internal::ClearPage(&(m_vertex_buffer_bitmap), vertex_start_frame + i);
		}

		//Store the offset of the allocated memory from the start of the staging buffer
		mesh->m_vertex_staging_buffer_offset = vertex_start_frame * 65536;
		mesh->m_vertex_staging_buffer_size = num_vertices * vertex_size;
		mesh->m_vertex_staging_buffer_stride = vertex_size;
		mesh->m_vertex_count = num_vertices;

		mesh->m_vertex_buffer_base_address = m_vertex_buffer->m_gpu_address;

		//Send the vertex data to the vertex staging buffer
		d3d12::UpdateStagingBuffer(m_vertex_buffer, vertices_data, num_vertices*vertex_size, mesh->m_vertex_staging_buffer_offset);

		//Store the mesh handle
		m_mesh_handles.push_back(mesh);

		return mesh;
	}

	void D3D12ModelPool::DestroyModel(Model * model)
	{
		for (int i = 0; i < model->m_meshes.size(); ++i) 
		{
			DestroyMesh(model->m_meshes[i]);
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

		//Find where in the vector storing meshes the mesh is located
		std::vector<Mesh*>::iterator it;
		for (it = m_mesh_handles.begin(); it != m_mesh_handles.end(); ++it) 
		{
			if ((*it) == mesh) 
			{
				break;
			}
		}

		//Check if the mesh was actually located
		if (it == m_mesh_handles.end())
		{
			return;
		}

		//Remove the mesh from the vector
		m_mesh_handles.erase(it);

		D3D12Mesh* n_mesh = static_cast<D3D12Mesh*>(mesh);

		//Find at which page in memory the mesh vertex data starts.
		std::uint64_t frame = n_mesh->m_vertex_staging_buffer_offset / 65536;
		//Find how much pages are occupied by the mesh vertex data
		std::uint64_t frame_count = SizeAlign(n_mesh->m_vertex_staging_buffer_size, 65536) / 65536;

		//Mark all occupied pages as free
		for (int i = 0; i < frame_count; ++i)
		{
			internal::SetPage(&(m_vertex_buffer_bitmap), frame + i);
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
				internal::SetPage(&(m_index_buffer_bitmap), frame + i);
			}

		}

		//Delete the mesh
		delete mesh;
	}

} /* wr */
