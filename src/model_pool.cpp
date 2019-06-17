// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "model_pool.hpp"
#include <utility>

namespace wr
{

	void Model::Expand(float(&pos)[3])
	{
		m_box.Expand(pos);
	}

	ModelPool::ModelPool(std::size_t vertex_buffer_pool_size_in_bytes,
		std::size_t index_buffer_pool_size_in_bytes) : 
		m_vertex_buffer_pool_size_in_bytes(vertex_buffer_pool_size_in_bytes),
		m_index_buffer_pool_size_in_bytes(index_buffer_pool_size_in_bytes),
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

	template<>
	int ModelPool::LoadNodeMeshes<Vertex, std::uint32_t>(ModelData* data, Model* model, MaterialHandle default_material)
	{
		model->m_meshes.reserve(data->m_meshes.size());

		for (unsigned int i = 0; i < data->m_meshes.size(); ++i)
		{
			ModelMeshData* mesh = data->m_meshes[i];

			std::vector<Vertex> vertices(mesh->m_positions.size());
			std::vector<std::uint32_t> indices(mesh->m_indices.size());

			Mesh* mesh_handle = new Mesh();

			for (unsigned int j = 0; j < mesh->m_positions.size(); ++j)
			{
				Vertex &vertex = vertices[j];

				memcpy(vertex.m_pos, &mesh->m_positions[j], sizeof(vertex.m_pos));
				memcpy(vertex.m_normal, &mesh->m_normals[j], sizeof(vertex.m_normal));
				memcpy(vertex.m_tangent, &mesh->m_tangents[j], sizeof(vertex.m_tangent));
				memcpy(vertex.m_bitangent, &mesh->m_bitangents[j], sizeof(vertex.m_bitangent));
				memcpy(vertex.m_uv, &mesh->m_uvw[j], sizeof(vertex.m_uv));

				model->Expand(vertex.m_pos);

			}

			memcpy(indices.data(), mesh->m_indices.data(), mesh->m_indices.size() * 4);

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(Vertex),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (mesh_data == nullptr)
			{
				for (auto &elem : model->m_meshes)
					delete elem.first;

				delete model;

				return 1;
			}

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle material_handle = default_material;

			std::pair<Mesh*, MaterialHandle> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);


			model->m_meshes.push_back(n_mesh);
		}

		return 0;
	}
	
	template<>
	int ModelPool::LoadNodeMeshes<VertexColor, std::uint32_t>(ModelData* data, Model* model, MaterialHandle default_material)
	{
		model->m_meshes.reserve(data->m_meshes.size());

		for (unsigned int i = 0; i < data->m_meshes.size(); ++i)
		{
			ModelMeshData* mesh = data->m_meshes[i];

			std::vector<VertexColor> vertices(mesh->m_positions.size());
			std::vector<std::uint32_t> indices(mesh->m_indices.size());

			Mesh* mesh_handle = new Mesh();

			for (unsigned int j = 0; j < mesh->m_positions.size(); ++j)
			{
				VertexColor &vertex = vertices[j];

				memcpy(vertex.m_pos, &mesh->m_positions[j], sizeof(vertex.m_pos));
				memcpy(vertex.m_normal, &mesh->m_normals[j], sizeof(vertex.m_normal));
				memcpy(vertex.m_tangent, &mesh->m_tangents[j], sizeof(vertex.m_tangent));
				memcpy(vertex.m_bitangent, &mesh->m_bitangents[j], sizeof(vertex.m_bitangent));
				memcpy(vertex.m_uv, &mesh->m_uvw[j], sizeof(vertex.m_uv));
				memcpy(vertex.m_color, &mesh->m_colors[j], sizeof(vertex.m_color));

				model->Expand(vertex.m_pos);

			}

			memcpy(indices.data(), mesh->m_indices.data(), sizeof(std::uint32_t)*mesh->m_indices.size());

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(VertexColor),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (mesh_data == nullptr)
			{
				for (auto &elem : model->m_meshes)
					delete elem.first;

				delete model;

				return 1;
			}

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle material_handle = default_material;

			std::pair<Mesh*, MaterialHandle> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);


			model->m_meshes.push_back(n_mesh);
		}

		return 0;
	}

	template<>
	int ModelPool::LoadNodeMeshes<VertexNoTangent, std::uint32_t>(ModelData* data, Model* model, MaterialHandle default_material)
	{
		model->m_meshes.reserve(data->m_meshes.size());

		for (unsigned int i = 0; i < data->m_meshes.size(); ++i)
		{
			ModelMeshData* mesh = data->m_meshes[i];

			std::vector<VertexNoTangent> vertices(mesh->m_positions.size());
			std::vector<std::uint32_t> indices(mesh->m_indices.size());

			Mesh* mesh_handle = new Mesh();

			for (unsigned int j = 0; j < mesh->m_positions.size(); ++j)
			{
				VertexNoTangent &vertex = vertices[j];

				memcpy(vertex.m_pos, &mesh->m_positions[j], sizeof(vertex.m_pos));
				memcpy(vertex.m_normal, &mesh->m_normals[j], sizeof(vertex.m_normal));
				memcpy(vertex.m_uv, &mesh->m_uvw[j], sizeof(vertex.m_uv));

				model->Expand(vertex.m_pos);

			}

			memcpy(indices.data(), mesh->m_indices.data(), sizeof(std::uint32_t)*mesh->m_indices.size());

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(VertexNoTangent),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (mesh_data == nullptr)
			{
				for (auto &elem : model->m_meshes)
					delete elem.first;

				delete model;

				return 1;
			}

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle material_handle = default_material;

			std::pair<Mesh*, MaterialHandle> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);

			model->m_meshes.push_back(n_mesh);
		}

		return 0;
	}

	template<>
	int ModelPool::LoadNodeMeshesWithMaterials<Vertex, std::uint32_t>(ModelData* data, Model * model, std::vector<MaterialHandle> materials)
	{
		model->m_meshes.reserve(data->m_meshes.size());

		for (unsigned int i = 0; i < data->m_meshes.size(); ++i)
		{
			ModelMeshData* mesh = data->m_meshes[i];

			std::vector<Vertex> vertices(mesh->m_positions.size());
			std::vector<std::uint32_t> indices(mesh->m_indices.size());

			Mesh* mesh_handle = new Mesh();

			for (unsigned int j = 0; j < mesh->m_positions.size(); ++j)
			{
				Vertex &vertex = vertices[j];

				memcpy(vertex.m_pos, &mesh->m_positions[j], sizeof(vertex.m_pos));
				memcpy(vertex.m_normal, &mesh->m_normals[j], sizeof(vertex.m_normal));
				memcpy(vertex.m_tangent, &mesh->m_tangents[j], sizeof(vertex.m_tangent));
				memcpy(vertex.m_bitangent, &mesh->m_bitangents[j], sizeof(vertex.m_bitangent));
				memcpy(vertex.m_uv, &mesh->m_uvw[j], sizeof(vertex.m_uv));

				model->Expand(vertex.m_pos);

			}

			memcpy(indices.data(), mesh->m_indices.data(), sizeof(std::uint32_t)*mesh->m_indices.size());

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(Vertex),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (mesh_data == nullptr)
			{
				for (auto &elem : model->m_meshes)
					delete elem.first;

				delete model;

				return 1;
			}

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle material_handle = materials[mesh->m_material_id];

			std::pair<Mesh*, MaterialHandle> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);
			
			model->m_meshes.push_back(n_mesh);
		}

		return 0;
	}

	template<>
	void ModelPool::UpdateModelBoundingBoxes<Vertex>(Model * model, std::vector<Vertex> vertices_data)
	{
		for (int i = 0; i < vertices_data.size(); ++i)
		{
			model->Expand(vertices_data[i].m_pos);
		}
	}

	template<>
	void ModelPool::UpdateModelBoundingBoxes<VertexNoTangent>(Model * model, std::vector<VertexNoTangent> vertices_data)
	{
		for (int i = 0; i < vertices_data.size(); ++i)
		{
			model->Expand(vertices_data[i].m_pos);
		}
	}

	template<>
	void ModelPool::UpdateModelBoundingBoxes<VertexColor>(Model * model, std::vector<VertexColor> vertices_data)
	{
		for (int i = 0; i < vertices_data.size(); ++i)
		{
			model->Expand(vertices_data[i].m_pos);
		}
	}
	
	template<>
	int ModelPool::LoadNodeMeshesWithMaterials<VertexColor, std::uint32_t>(ModelData* data, Model * model, std::vector<MaterialHandle> materials)
	{
		model->m_meshes.reserve(data->m_meshes.size());

		for (unsigned int i = 0; i < data->m_meshes.size(); ++i)
		{
			ModelMeshData* mesh = data->m_meshes[i];

			std::vector<VertexColor> vertices(mesh->m_positions.size());
			std::vector<std::uint32_t> indices(mesh->m_indices.size());

			Mesh* mesh_handle = new Mesh();

			for (unsigned int j = 0; j < mesh->m_positions.size(); ++j)
			{
				VertexColor &vertex = vertices[j];

				memcpy(vertex.m_pos, &mesh->m_positions[j], sizeof(vertex.m_pos));
				memcpy(vertex.m_normal, &mesh->m_normals[j], sizeof(vertex.m_normal));
				memcpy(vertex.m_tangent, &mesh->m_tangents[j], sizeof(vertex.m_tangent));
				memcpy(vertex.m_bitangent, &mesh->m_bitangents[j], sizeof(vertex.m_bitangent));
				memcpy(vertex.m_uv, &mesh->m_uvw[j], sizeof(vertex.m_uv));
				memcpy(vertex.m_color, &mesh->m_colors[j], sizeof(vertex.m_color));

				model->Expand(vertex.m_pos);

			}

			memcpy(indices.data(), mesh->m_indices.data(), sizeof(std::uint32_t)*mesh->m_indices.size());

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(VertexColor),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (mesh_data == nullptr)
			{
				for (auto &elem : model->m_meshes)
					delete elem.first;

				delete model;

				return 1;
			}

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle material_handle = materials[mesh->m_material_id];

			std::pair<Mesh*, MaterialHandle> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);
			
			model->m_meshes.push_back(n_mesh);
		}

		return 0;
	}

	template<>
	int ModelPool::LoadNodeMeshesWithMaterials<VertexNoTangent, std::uint32_t>(ModelData* data, Model * model, std::vector<MaterialHandle> materials)
	{
		model->m_meshes.reserve(data->m_meshes.size());

		for (unsigned int i = 0; i < data->m_meshes.size(); ++i)
		{
			ModelMeshData* mesh = data->m_meshes[i];

			std::vector<VertexNoTangent> vertices(mesh->m_positions.size());
			std::vector<std::uint32_t> indices(mesh->m_indices.size());

			Mesh* mesh_handle = new Mesh();

			for (unsigned int j = 0; j < mesh->m_positions.size(); ++j)
			{
				VertexNoTangent &vertex = vertices[j];

				memcpy(vertex.m_pos, &mesh->m_positions[j], sizeof(vertex.m_pos));
				memcpy(vertex.m_normal, &mesh->m_normals[j], sizeof(vertex.m_normal));
				memcpy(vertex.m_uv, &mesh->m_uvw[j], sizeof(vertex.m_uv));

				model->Expand(vertex.m_pos);

			}

			memcpy(indices.data(), mesh->m_indices.data(), sizeof(std::uint32_t)*mesh->m_indices.size());

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(VertexNoTangent),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (mesh_data == nullptr)
			{
				for (auto &elem : model->m_meshes)
					delete elem.first;

				delete model;

				return 1;
			}

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle material_handle = materials[mesh->m_material_id];

			std::pair<Mesh*, MaterialHandle> n_mesh = std::make_pair(
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