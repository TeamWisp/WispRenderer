#include "d3d12_resource_pool_model.hpp"

#include "d3d12_functions.hpp"
#include "d3d12_defines.hpp"
#include "d3d12_renderer.hpp"

namespace wr
{
	static inline std::uint64_t IndexFromBit(std::uint64_t frame) {
		return frame / (8 * 8);
	}
	static inline std::uint64_t OffsetFromBit(std::uint64_t frame) {
		return frame % (8 * 8);
	}

	static void SetFrame(std::vector<uint64_t>* bitmap, std::uint64_t frame) {
		std::uint64_t idx = IndexFromBit(frame);
		std::uint64_t off = OffsetFromBit(frame);
		bitmap->operator[](idx) |= (1Ui64 << off);
	}

	static void ClearFrame(std::vector<uint64_t>* bitmap, std::uint64_t frame) {
		std::uint64_t idx = IndexFromBit(frame);
		std::uint64_t off = OffsetFromBit(frame);
		bitmap->operator[](idx) &= ~(1Ui64 << off);
	}

	static bool TestFrame(std::vector<uint64_t>* bitmap, std::uint64_t frame) {
		std::uint64_t idx = IndexFromBit(frame);
		std::uint64_t off = OffsetFromBit(frame);
		return (bitmap->operator[](idx) & (1Ui64 << off));
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

		for (int i = 0; i < m_vertex_buffer_bitmap.size() - 1; ++i) {
			m_vertex_buffer_bitmap[i] = 0xffffffffffffffff;
		}
		for (int i = 0; i < 64; ++i) {
			if ((m_vertex_buffer_bitmap.size() - 1) * 64 + i < frame_count) {
				m_vertex_buffer_bitmap[m_vertex_buffer_bitmap.size() - 1] |= 1Ui64 << i;
			}
			else {
				break;
			}
		}
		
		page_frame_count = SizeAlign(m_index_buffer_size / 65536, 64) / 64;
		frame_count = m_index_buffer_size / 65536;

		m_index_buffer_bitmap.resize(page_frame_count);

		for (int i = 0; i < m_index_buffer_bitmap.size() - 1; ++i) {
			m_index_buffer_bitmap[i] = 0xffffffffffffffff;
		}
		for (int i = 0; i < 64; ++i) {
			if ((m_index_buffer_bitmap.size() - 1) * 64 + i < frame_count) {
				m_index_buffer_bitmap[m_index_buffer_bitmap.size() - 1] |= 1Ui64 << i;
			}
			else {
				break;
			}
		}
	}

	D3D12ModelPool::~D3D12ModelPool()
	{
		d3d12::Destroy(m_vertex_buffer);
		d3d12::Destroy(m_index_buffer);

		//Do model handles created from this pool be destroyed by the pool?
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
		if (d3d12_mesh->m_index_staging_buffer_size != 0) {
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

		auto frame_count = m_vertex_buffer_size / 65536;
		auto needed_frames = SizeAlign(num_vertices*vertex_size, 65536) / 65536;

		std::uint64_t start_frame = 0;
		bool counting = false;
		bool found = false;
		int free_frames = 0;

		for (std::uint64_t i = 0; i <= IndexFromBit(frame_count); ++i) {
			if (m_vertex_buffer_bitmap[i] != 0Ui64) {
				for (std::uint64_t j = 0; j < 64; ++j) {
					std::uint64_t to_test = 1Ui64 << j;
					if (i * 64 + j >= frame_count)
						break;

					if ((m_vertex_buffer_bitmap[i] & to_test)) {
						if (counting) {
							free_frames++;
							if (free_frames == needed_frames) {
								found = true;
								break;
							}
						}
						else {
							if (needed_frames == 1) {
								found = true;
								start_frame = i * 64 + j;
								break;
							}
							else {
								start_frame = i * 64 + j;
								counting = true;
								free_frames = 1;
							}
						}
					}
					else {
						counting = 0;
						free_frames = 0;
					}
				}
			}
			else {
				counting = false;
				free_frames = 0;
			}
			if (found == true) {
				break;
			}
		}

		if (found == false) {
			return nullptr;
		}

		for (std::uint64_t i = 0; i < needed_frames; ++i) {
			ClearFrame(&(m_vertex_buffer_bitmap), start_frame + i);
		}

		mesh->m_vertex_staging_buffer_offset = start_frame * 65536;
		mesh->m_vertex_staging_buffer_size = num_vertices * vertex_size;
		mesh->m_vertex_staging_buffer_stride = vertex_size;
		mesh->m_vertex_count = num_vertices;

		mesh->m_vertex_buffer_base_address = m_vertex_buffer->m_gpu_address;

		d3d12::SetStagingBufferData(m_vertex_buffer, vertices_data, num_vertices*vertex_size, mesh->m_vertex_staging_buffer_offset);
		
		frame_count = m_index_buffer_size / 65536;
		needed_frames = SizeAlign(num_indices*index_size, 65536) / 65536;

		start_frame = 0;
		counting = false;
		found = false;
		free_frames = 0;

		for (std::uint64_t i = 0; i <= IndexFromBit(frame_count); ++i) {
			if (m_index_buffer_bitmap[i] != 0Ui64) {
				for (std::uint64_t j = 0; j < 64; ++j) {
					std::uint64_t to_test = 1Ui64 << j;
					if (i * 64 + j >= frame_count)
						break;

					if ((m_index_buffer_bitmap[i] & to_test)) {
						if (counting) {
							free_frames++;
							if (free_frames == needed_frames) {
								found = true;
								break;
							}
						}
						else {
							if (needed_frames == 1) {
								found = true;
								start_frame = i * 64 + j;
								break;
							}
							else {
								start_frame = i * 64 + j;
								counting = true;
								free_frames = 1;
							}
						}
					}
					else {
						counting = 0;
						free_frames = 0;
					}
				}
			}
			else {
				counting = false;
				free_frames = 0;
			}
			if (found == true) {
				break;
			}
		}

		if (found == false) {
			return nullptr;
		}

		for (std::uint64_t i = 0; i < needed_frames; ++i) {
			ClearFrame(&(m_index_buffer_bitmap), start_frame + i);
		}

		mesh->m_index_staging_buffer_offset = start_frame * 65536;
		mesh->m_index_staging_buffer_size = num_indices * index_size;
		mesh->m_index_count = num_indices;

		mesh->m_index_buffer_base_address = m_index_buffer->m_gpu_address;

		d3d12::SetStagingBufferData(m_index_buffer, indices_data, num_indices*index_size, mesh->m_index_staging_buffer_offset);

		return mesh;
	}

	Mesh* D3D12ModelPool::LoadCustom_VerticesOnly(void* vertices_data, std::size_t num_vertices, std::size_t vertex_size)
	{
		D3D12Mesh* mesh = new D3D12Mesh();
		memset(mesh, 0, sizeof(D3D12Mesh));
		mesh->m_model_pool = this;

		auto frame_count = m_vertex_buffer_size / 65536;
		auto needed_frames = SizeAlign(num_vertices*vertex_size, 65536) / 65536;

		std::uint64_t start_frame = 0;
		bool counting = false;
		bool found = false;
		int free_frames = 0;

		for (std::uint64_t i = 0; i <= IndexFromBit(frame_count); ++i) {
			if (m_vertex_buffer_bitmap[i] != 0Ui64) {
				for (std::uint64_t j = 0; j < 64; ++j) {
					std::uint64_t to_test = 1Ui64 << j;
					if (i * 64 + j >= frame_count)
						break;

					if ((m_vertex_buffer_bitmap[i] & to_test)) {
						if (counting) {
							free_frames++;
							if (free_frames == needed_frames) {
								found = true;
								break;
							}
						}
						else {
							if (needed_frames == 1) {
								found = true;
								start_frame = i * 64 + j;
								break;
							}
							else {
								start_frame = i * 64 + j;
								counting = true;
								free_frames = 1;
							}
						}
					}
					else {
						counting = 0;
						free_frames = 0;
					}
				}
			}
			else {
				counting = false;
				free_frames = 0;
			}
			if (found == true) {
				break;
			}
		}

		if (found == false) {
			return nullptr;
		}

		for (std::uint64_t i = 0; i < needed_frames; ++i) {
			ClearFrame(&(m_vertex_buffer_bitmap), start_frame + i);
		}

		mesh->m_vertex_staging_buffer_offset = start_frame * 65536;
		mesh->m_vertex_staging_buffer_size = num_vertices * vertex_size;
		mesh->m_vertex_staging_buffer_stride = vertex_size;
		mesh->m_vertex_count = num_vertices;

		mesh->m_vertex_buffer_base_address = m_vertex_buffer->m_gpu_address;

		d3d12::SetStagingBufferData(m_vertex_buffer, vertices_data, num_vertices*vertex_size, mesh->m_vertex_staging_buffer_offset);

		return mesh;
	}

	void D3D12ModelPool::DestroyModel(Model * model)
	{
		for (int i = 0; i < model->m_meshes.size(); ++i) {
			DestroyMesh(model->m_meshes[i]);
		}

		//TODO: Destroy possible materials owned by model. Material might be used by multiple models, use ref counts?

		delete model;
	}

	void D3D12ModelPool::DestroyMesh(Mesh * mesh)
	{
		if (mesh == nullptr)
			return;

		if (mesh->m_model_pool != this)
			return;

		D3D12Mesh* n_mesh = static_cast<D3D12Mesh*>(mesh);

		std::uint64_t frame = n_mesh->m_vertex_staging_buffer_offset / 65536;
		std::uint64_t frame_count = SizeAlign(n_mesh->m_vertex_staging_buffer_size, 65536) / 65536;

		for (int i = 0; i < frame_count; ++i) {
			SetFrame(&(m_vertex_buffer_bitmap), frame + i);
		}

		if (n_mesh->m_index_buffer_base_address != 0) {
			std::uint64_t frame = n_mesh->m_index_staging_buffer_offset / 65536;
			std::uint64_t frame_count = SizeAlign(n_mesh->m_index_staging_buffer_size, 65536) / 65536;

			for (int i = 0; i < frame_count; ++i) {
				SetFrame(&(m_index_buffer_bitmap), frame + i);
			}

		}

		delete mesh;
	}

} /* wr */
