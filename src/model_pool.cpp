#define NOMINMAX 1
#include "model_pool.hpp"
#include <utility>

namespace wr
{
	ModelPool::ModelPool(std::size_t vertex_buffer_pool_size_in_mb,
		std::size_t index_buffer_pool_size_in_mb) : 
		m_vertex_buffer_pool_size_in_mb(vertex_buffer_pool_size_in_mb),
		m_index_buffer_pool_size_in_mb(index_buffer_pool_size_in_mb),
		m_current_id(0)
	{
	}

	void ModelPool::Destroy(Model * model)
	{
		DestroyModel(model);
	}

	void ModelPool::Destroy(internal::MeshInternal * mesh)
	{
		DestroyMesh(mesh);
	}

	void Model::CalculateAABB(float(&pos)[3])
	{
		if (pos[0] < m_box[0].m128_f32[0])
		{
			m_box[0] = { pos[0], pos[1], pos[2], 1 };
		}

		if (pos[0] > m_box[1].m128_f32[0])
		{
			m_box[1] = { pos[0], pos[1], pos[2], 1 };
		}

		if (pos[1] < m_box[2].m128_f32[1])
		{
			m_box[2] = { pos[0], pos[1], pos[2], 1 };
		}

		if (pos[1] > m_box[3].m128_f32[1])
		{
			m_box[3] = { pos[0], pos[1], pos[2], 1 };
		}

		if (pos[2] < m_box[4].m128_f32[2])
		{
			m_box[4] = { pos[0], pos[1], pos[2], 1 };
		}

		if (pos[2] > m_box[5].m128_f32[2])
		{
			m_box[5] = { pos[0], pos[1], pos[2], 1 };
		}
	}


	template<>
	int ModelPool::LoadNodeMeshes<Vertex, std::uint32_t>(ModelData* data, Model* model, MaterialHandle* default_material)
	{
		for (unsigned int i = 0; i < data->m_meshes.size(); ++i)
		{
			ModelMeshData* mesh = data->m_meshes[i];

			std::vector<Vertex> vertices;

			vertices.resize(mesh->m_positions.size());

			std::vector<std::uint32_t> indices;

			indices.resize(mesh->m_indices.size());

			for (unsigned int j = 0; j < mesh->m_positions.size(); ++j)
			{
				Vertex vertex = {};

				vertex.m_pos[0] = mesh->m_positions[j].x;
				vertex.m_pos[1] = mesh->m_positions[j].y;
				vertex.m_pos[2] = mesh->m_positions[j].z;

				model->CalculateAABB(vertex.m_pos);

				vertex.m_normal[0] = mesh->m_normals[j].x;
				vertex.m_normal[1] = mesh->m_normals[j].y;
				vertex.m_normal[2] = mesh->m_normals[j].z;

				vertex.m_uv[0] = mesh->m_uvw[j].x;
				vertex.m_uv[1] = mesh->m_uvw[j].y;

				vertex.m_tangent[0] = mesh->m_tangents[j].x;
				vertex.m_tangent[1] = mesh->m_tangents[j].y;
				vertex.m_tangent[2] = mesh->m_tangents[j].z;

				vertex.m_bitangent[0] = mesh->m_bitangents[j].x;
				vertex.m_bitangent[1] = mesh->m_bitangents[j].y;
				vertex.m_bitangent[2] = mesh->m_bitangents[j].z;
				
				vertices[j] = vertex;
			}

			memcpy(indices.data(), mesh->m_indices.data(), sizeof(std::uint32_t)*mesh->m_indices.size());

			Mesh* mesh_handle = new Mesh();

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(Vertex),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (mesh_data == nullptr)
			{
				return 1;
			}

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle* material_handle = default_material;

			std::pair<Mesh*, MaterialHandle*> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);


			model->m_meshes.push_back(n_mesh);
		}
		return 0;
	}
	
	template<>
	int ModelPool::LoadNodeMeshes<VertexColor, std::uint32_t>(ModelData* data, Model* model, MaterialHandle* default_material)
	{
		for (unsigned int i = 0; i < data->m_meshes.size(); ++i)
		{
			ModelMeshData* mesh = data->m_meshes[i];

			std::vector<VertexColor> vertices;

			vertices.resize(mesh->m_positions.size());

			std::vector<std::uint32_t> indices;

			indices.resize(mesh->m_indices.size());

			for (unsigned int j = 0; j < mesh->m_positions.size(); ++j)
			{
				VertexColor vertex = {};

				vertex.m_pos[0] = mesh->m_positions[j].x;
				vertex.m_pos[1] = mesh->m_positions[j].y;
				vertex.m_pos[2] = mesh->m_positions[j].z;

				model->CalculateAABB(vertex.m_pos);

				vertex.m_normal[0] = mesh->m_normals[j].x;
				vertex.m_normal[1] = mesh->m_normals[j].y;
				vertex.m_normal[2] = mesh->m_normals[j].z;

				vertex.m_uv[0] = mesh->m_uvw[j].x;
				vertex.m_uv[1] = mesh->m_uvw[j].y;

				vertex.m_tangent[0] = mesh->m_tangents[j].x;
				vertex.m_tangent[1] = mesh->m_tangents[j].y;
				vertex.m_tangent[2] = mesh->m_tangents[j].z;

				vertex.m_bitangent[0] = mesh->m_bitangents[j].x;
				vertex.m_bitangent[1] = mesh->m_bitangents[j].y;
				vertex.m_bitangent[2] = mesh->m_bitangents[j].z;

				vertex.m_color[0] = mesh->m_colors[j].x;
				vertex.m_color[1] = mesh->m_colors[j].y;
				vertex.m_color[2] = mesh->m_colors[j].z;
				
				vertices[j] = vertex;
			}

			memcpy(indices.data(), mesh->m_indices.data(), sizeof(std::uint32_t)*mesh->m_indices.size());

			Mesh* mesh_handle = new Mesh();

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(VertexColor),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (mesh_data == nullptr)
			{
				return 1;
			}

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle* material_handle = default_material;

			std::pair<Mesh*, MaterialHandle*> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);


			model->m_meshes.push_back(n_mesh);
		}
		return 0;
	}

	template<>
	int ModelPool::LoadNodeMeshes<VertexNoTangent, std::uint32_t>(ModelData* data, Model* model, MaterialHandle* default_material)
	{
		for (unsigned int i = 0; i < data->m_meshes.size(); ++i)
		{
			ModelMeshData* mesh = data->m_meshes[i];

			std::vector<VertexNoTangent> vertices;

			vertices.resize(mesh->m_positions.size());

			std::vector<std::uint32_t> indices;

			indices.resize(mesh->m_indices.size());

			for (unsigned int j = 0; j < mesh->m_positions.size(); ++j)
			{
				VertexNoTangent vertex = {};

				vertex.m_pos[0] = mesh->m_positions[j].x;
				vertex.m_pos[1] = mesh->m_positions[j].y;
				vertex.m_pos[2] = mesh->m_positions[j].z;

				model->CalculateAABB(vertex.m_pos);

				vertex.m_normal[0] = mesh->m_normals[j].x;
				vertex.m_normal[1] = mesh->m_normals[j].y;
				vertex.m_normal[2] = mesh->m_normals[j].z;

				vertex.m_uv[0] = mesh->m_uvw[j].x;
				vertex.m_uv[1] = mesh->m_uvw[j].y;

				vertices[j] = vertex;
			}

			memcpy(indices.data(), mesh->m_indices.data(), sizeof(std::uint32_t)*mesh->m_indices.size());

			Mesh* mesh_handle = new Mesh();

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(VertexNoTangent),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (mesh_data == nullptr)
			{
				return 1;
			}

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle* material_handle = default_material;

			std::pair<Mesh*, MaterialHandle*> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);

			model->m_meshes.push_back(n_mesh);
		}
		return 0;
	}

	template<>
	int ModelPool::LoadNodeMeshesWithMaterials<Vertex, std::uint32_t>(ModelData* data, Model * model, std::vector<MaterialHandle*> materials)
	{
		for (unsigned int i = 0; i < data->m_meshes.size(); ++i)
		{
			ModelMeshData* mesh = data->m_meshes[i];

			std::vector<Vertex> vertices;

			vertices.resize(mesh->m_positions.size());

			std::vector<std::uint32_t> indices;

			indices.resize(mesh->m_indices.size());

			for (unsigned int j = 0; j < mesh->m_positions.size(); ++j)
			{
				Vertex vertex = {};

				vertex.m_pos[0] = mesh->m_positions[j].x;
				vertex.m_pos[1] = mesh->m_positions[j].y;
				vertex.m_pos[2] = mesh->m_positions[j].z;

				model->CalculateAABB(vertex.m_pos);

				vertex.m_normal[0] = mesh->m_normals[j].x;
				vertex.m_normal[1] = mesh->m_normals[j].y;
				vertex.m_normal[2] = mesh->m_normals[j].z;

				vertex.m_uv[0] = mesh->m_uvw[j].x;
				vertex.m_uv[1] = mesh->m_uvw[j].y;

				vertex.m_tangent[0] = mesh->m_tangents[j].x;
				vertex.m_tangent[1] = mesh->m_tangents[j].y;
				vertex.m_tangent[2] = mesh->m_tangents[j].z;

				vertex.m_bitangent[0] = mesh->m_bitangents[j].x;
				vertex.m_bitangent[1] = mesh->m_bitangents[j].y;
				vertex.m_bitangent[2] = mesh->m_bitangents[j].z;

				vertices[j] = vertex;
			}

			memcpy(indices.data(), mesh->m_indices.data(), sizeof(std::uint32_t)*mesh->m_indices.size());

			Mesh* mesh_handle = new Mesh();

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(Vertex),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (mesh_data == nullptr)
			{
				return 1;
			}

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle* material_handle = materials[mesh->m_material_id];

			std::pair<Mesh*, MaterialHandle*> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);
			
			model->m_meshes.push_back(n_mesh);
		}
		return 0;
	}
	
	template<>
	int ModelPool::LoadNodeMeshesWithMaterials<VertexColor, std::uint32_t>(ModelData* data, Model * model, std::vector<MaterialHandle*> materials)
	{
		for (unsigned int i = 0; i < data->m_meshes.size(); ++i)
		{
			ModelMeshData* mesh = data->m_meshes[i];

			std::vector<VertexColor> vertices;

			vertices.resize(mesh->m_positions.size());

			std::vector<std::uint32_t> indices;

			indices.resize(mesh->m_indices.size());

			for (unsigned int j = 0; j < mesh->m_positions.size(); ++j)
			{
				VertexColor vertex = {};

				vertex.m_pos[0] = mesh->m_positions[j].x;
				vertex.m_pos[1] = mesh->m_positions[j].y;
				vertex.m_pos[2] = mesh->m_positions[j].z;

				model->CalculateAABB(vertex.m_pos);

				vertex.m_normal[0] = mesh->m_normals[j].x;
				vertex.m_normal[1] = mesh->m_normals[j].y;
				vertex.m_normal[2] = mesh->m_normals[j].z;

				vertex.m_uv[0] = mesh->m_uvw[j].x;
				vertex.m_uv[1] = mesh->m_uvw[j].y;

				vertex.m_tangent[0] = mesh->m_tangents[j].x;
				vertex.m_tangent[1] = mesh->m_tangents[j].y;
				vertex.m_tangent[2] = mesh->m_tangents[j].z;

				vertex.m_bitangent[0] = mesh->m_bitangents[j].x;
				vertex.m_bitangent[1] = mesh->m_bitangents[j].y;
				vertex.m_bitangent[2] = mesh->m_bitangents[j].z;

				vertex.m_color[0] = mesh->m_colors[j].x;
				vertex.m_color[1] = mesh->m_colors[j].y;
				vertex.m_color[2] = mesh->m_colors[j].z;

				vertices[j] = vertex;
			}

			memcpy(indices.data(), mesh->m_indices.data(), sizeof(std::uint32_t)*mesh->m_indices.size());

			Mesh* mesh_handle = new Mesh();

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(VertexColor),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (mesh_data == nullptr)
			{
				return 1;
			}

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle* material_handle = materials[mesh->m_material_id];

			std::pair<Mesh*, MaterialHandle*> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);
			
			model->m_meshes.push_back(n_mesh);
		}
		return 0;
	}

	template<>
	int ModelPool::LoadNodeMeshesWithMaterials<VertexNoTangent, std::uint32_t>(ModelData* data, Model * model, std::vector<MaterialHandle*> materials)
	{
		for (unsigned int i = 0; i < data->m_meshes.size(); ++i)
		{
			ModelMeshData* mesh = data->m_meshes[i];

			std::vector<VertexNoTangent> vertices;

			vertices.resize(mesh->m_positions.size());

			std::vector<std::uint32_t> indices;

			indices.resize(mesh->m_indices.size());

			for (unsigned int j = 0; j < mesh->m_positions.size(); ++j)
			{
				VertexNoTangent vertex = {};

				vertex.m_pos[0] = mesh->m_positions[j].x;
				vertex.m_pos[1] = mesh->m_positions[j].y;
				vertex.m_pos[2] = mesh->m_positions[j].z;

				model->CalculateAABB(vertex.m_pos);

				vertex.m_normal[0] = mesh->m_normals[j].x;
				vertex.m_normal[1] = mesh->m_normals[j].y;
				vertex.m_normal[2] = mesh->m_normals[j].z;

				vertex.m_uv[0] = mesh->m_uvw[j].x;
				vertex.m_uv[1] = mesh->m_uvw[j].y;

				vertices[j] = vertex;
			}

			memcpy(indices.data(), mesh->m_indices.data(), sizeof(std::uint32_t)*mesh->m_indices.size());

			Mesh* mesh_handle = new Mesh();

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(VertexNoTangent),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));
			
			if (mesh_data == nullptr)
			{
				return 1;
			}

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle* material_handle = materials[mesh->m_material_id];

			std::pair<Mesh*, MaterialHandle*> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);

			model->m_meshes.push_back(n_mesh);
		}
		return 0;
	}

	std::uint64_t ModelPool::GetNewID()
	{
		std::uint64_t id;
		if (m_freed_ids.size() > 0)
		{
			id = m_freed_ids.top();
			m_freed_ids.pop();
		}
		else
		{
			id = m_current_id;
			m_current_id++;
		}
		return id;
	}

	void ModelPool::FreeID(std::uint64_t id)
	{
		m_freed_ids.push(id);
	}

} /* wr */