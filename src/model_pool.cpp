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
	int ModelPool::LoadNodeMeshes<Vertex, std::uint32_t>(const aiScene * scene, aiNode * node, Model* model, MaterialHandle* default_material)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			std::vector<Vertex> vertices;
			std::vector<std::uint32_t> indices;

			for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
			{
				Vertex vertex = {};

				vertex.m_pos[0] = mesh->mVertices[j].x;
				vertex.m_pos[1] = mesh->mVertices[j].y;
				vertex.m_pos[2] = mesh->mVertices[j].z;

				model->CalculateAABB(vertex.m_pos);

				if (mesh->mNormals)
				{
					vertex.m_normal[0] = mesh->mNormals[j].x;
					vertex.m_normal[1] = mesh->mNormals[j].y;
					vertex.m_normal[2] = mesh->mNormals[j].z;
				}

				if (mesh->mTextureCoords[0] > 0)
				{
					vertex.m_uv[0] = mesh->mTextureCoords[0][j].x;
					vertex.m_uv[1] = mesh->mTextureCoords[0][j].y;
				}

				if (mesh->mTangents)
				{
					vertex.m_tangent[0] = mesh->mTangents[j].x;
					vertex.m_tangent[1] = mesh->mTangents[j].y;
					vertex.m_tangent[2] = mesh->mTangents[j].z;

					vertex.m_bitangent[0] = mesh->mBitangents[j].x;
					vertex.m_bitangent[1] = mesh->mBitangents[j].y;
					vertex.m_bitangent[2] = mesh->mBitangents[j].z;
				}

				vertices.push_back(vertex);
			}

			for (size_t j = 0; j < mesh->mNumFaces; ++j)
			{
				aiFace *face = &mesh->mFaces[j];

				// retrieve all indices of the face and store them in the indices vector
				for (size_t k = 0; k < face->mNumIndices; k++)
				{
					indices.push_back(static_cast<unsigned>(face->mIndices[k]));
				}
			}

			Mesh* mesh_handle = new Mesh();

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(Vertex),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle* material_handle = default_material;

			std::pair<Mesh*, MaterialHandle*> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);

			if (n_mesh.first == nullptr)
			{
				return 1;
			}

			model->m_meshes.push_back(n_mesh);
		}
		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			int ret = LoadNodeMeshes<Vertex, std::uint32_t>(scene, node->mChildren[i], model, default_material);
			if (ret == 1)
				return 1;
		}
		return 0;
	}

	template<>
	int ModelPool::LoadNodeMeshes<VertexNoTangent, std::uint32_t>(const aiScene * scene, aiNode * node, Model* model, MaterialHandle* default_material)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			std::vector<VertexNoTangent> vertices;
			std::vector<std::uint32_t> indices;

			for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
			{
				VertexNoTangent vertex = {};

				vertex.m_pos[0] = mesh->mVertices[j].x;
				vertex.m_pos[1] = mesh->mVertices[j].y;
				vertex.m_pos[2] = mesh->mVertices[j].z;

				model->CalculateAABB(vertex.m_pos);

				if (mesh->mNormals)
				{
					vertex.m_normal[0] = mesh->mNormals[j].x;
					vertex.m_normal[1] = mesh->mNormals[j].y;
					vertex.m_normal[2] = mesh->mNormals[j].z;
				}

				if (mesh->mTextureCoords[0] > 0)
				{
					vertex.m_uv[0] = mesh->mTextureCoords[0][j].x;
					vertex.m_uv[1] = mesh->mTextureCoords[0][j].y;
				}

				vertices.push_back(vertex);
			}

			for (size_t j = 0; j < mesh->mNumFaces; ++j)
			{
				aiFace *face = &mesh->mFaces[j];

				// retrieve all indices of the face and store them in the indices vector
				for (size_t k = 0; k < face->mNumIndices; k++)
				{
					indices.push_back(static_cast<unsigned>(face->mIndices[k]));
				}
			}



			Mesh* mesh_handle = new Mesh();

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(VertexNoTangent),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			std::pair<Mesh*, MaterialHandle*> n_mesh = std::make_pair(
				mesh_handle,
				default_material);

			if (n_mesh.first == nullptr)
			{
				return 1;
			}

			model->m_meshes.push_back(n_mesh);
		}
		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			int ret = LoadNodeMeshes<VertexNoTangent, std::uint32_t>(scene, node->mChildren[i], model, default_material);
			if (ret == 1)
				return 1;
		}
		return 0;
	}

	template<>
	int ModelPool::LoadNodeMeshesWithMaterials<Vertex, std::uint32_t>(const aiScene * scene, aiNode * node, Model * model, std::vector<MaterialHandle*> materials)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			std::vector<Vertex> vertices;
			std::vector<std::uint32_t> indices;

			for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
			{
				Vertex vertex = {};

				vertex.m_pos[0] = mesh->mVertices[j].x;
				vertex.m_pos[1] = mesh->mVertices[j].y;
				vertex.m_pos[2] = mesh->mVertices[j].z;

				model->CalculateAABB(vertex.m_pos);

				if (mesh->mNormals)
				{
					vertex.m_normal[0] = mesh->mNormals[j].x;
					vertex.m_normal[1] = mesh->mNormals[j].y;
					vertex.m_normal[2] = mesh->mNormals[j].z;
				}

				if (mesh->mTextureCoords[0] > 0)
				{
					vertex.m_uv[0] = mesh->mTextureCoords[0][j].x;
					vertex.m_uv[1] = mesh->mTextureCoords[0][j].y;
				}

				if (mesh->mTangents)
				{
					vertex.m_tangent[0] = mesh->mTangents[j].x;
					vertex.m_tangent[1] = mesh->mTangents[j].y;
					vertex.m_tangent[2] = mesh->mTangents[j].z;

					vertex.m_bitangent[0] = mesh->mBitangents[j].x;
					vertex.m_bitangent[1] = mesh->mBitangents[j].y;
					vertex.m_bitangent[2] = mesh->mBitangents[j].z;
				}

				vertices.push_back(vertex);
			}

			for (size_t j = 0; j < mesh->mNumFaces; ++j)
			{
				aiFace *face = &mesh->mFaces[j];

				// retrieve all indices of the face and store them in the indices vector
				for (size_t k = 0; k < face->mNumIndices; k++)
				{
					indices.push_back(static_cast<unsigned>(face->mIndices[k]));
				}
			}



			Mesh* mesh_handle = new Mesh();

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(Vertex),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			std::pair<Mesh*, MaterialHandle*> n_mesh = std::make_pair(
				mesh_handle,
				nullptr);

			if (n_mesh.first == nullptr)
			{
				return 1;
			}

			if (scene->HasMaterials())
			{
				n_mesh.second = materials[mesh->mMaterialIndex];
			}

			model->m_meshes.push_back(n_mesh);
		}
		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			int ret = LoadNodeMeshesWithMaterials<Vertex, std::uint32_t>(scene, node->mChildren[i], model, materials);
			if (ret == 1)
				return 1;
		}
		return 0;
	}

	template<>
	int ModelPool::LoadNodeMeshesWithMaterials<VertexNoTangent, std::uint32_t>(const aiScene * scene, aiNode * node, Model * model, std::vector<MaterialHandle*> materials)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			std::vector<VertexNoTangent> vertices;
			std::vector<std::uint32_t> indices;

			for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
			{
				VertexNoTangent vertex = {};

				vertex.m_pos[0] = mesh->mVertices[j].x;
				vertex.m_pos[1] = mesh->mVertices[j].y;
				vertex.m_pos[2] = mesh->mVertices[j].z;

				model->CalculateAABB(vertex.m_pos);

				if (mesh->mNormals)
				{
					vertex.m_normal[0] = mesh->mNormals[j].x;
					vertex.m_normal[1] = mesh->mNormals[j].y;
					vertex.m_normal[2] = mesh->mNormals[j].z;
				}

				if (mesh->mTextureCoords[0] > 0)
				{
					vertex.m_uv[0] = mesh->mTextureCoords[0][j].x;
					vertex.m_uv[1] = mesh->mTextureCoords[0][j].y;
				}

				vertices.push_back(vertex);
			}

			for (size_t j = 0; j < mesh->mNumFaces; ++j)
			{
				aiFace *face = &mesh->mFaces[j];

				// retrieve all indices of the face and store them in the indices vector
				for (size_t k = 0; k < face->mNumIndices; k++)
				{
					indices.push_back(static_cast<unsigned>(face->mIndices[k]));
				}
			}



			Mesh* mesh_handle = new Mesh();

			internal::MeshInternal* mesh_data = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(VertexNoTangent),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			std::uint64_t id = GetNewID();
			m_loaded_meshes[id] = mesh_data;
			mesh_handle->id = id;

			MaterialHandle* material_handle = nullptr;

			std::pair<Mesh*, MaterialHandle*> n_mesh = std::make_pair(
				mesh_handle,
				material_handle);

			if (n_mesh.first == nullptr)
			{
				return 1;
			}

			if (scene->HasMaterials())
			{
				n_mesh.second = materials[mesh->mMaterialIndex];
			}

			model->m_meshes.push_back(n_mesh);
		}
		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			int ret = LoadNodeMeshesWithMaterials<VertexNoTangent, std::uint32_t>(scene, node->mChildren[i], model, materials);
			if (ret == 1)
				return 1;
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