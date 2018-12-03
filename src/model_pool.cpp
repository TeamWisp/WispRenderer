#define NOMINMAX 1
#include "model_pool.hpp"

namespace wr
{
	ModelPool::ModelPool(std::size_t vertex_buffer_pool_size_in_mb,
		std::size_t index_buffer_pool_size_in_mb) : 
		m_vertex_buffer_pool_size_in_mb(vertex_buffer_pool_size_in_mb),
		m_index_buffer_pool_size_in_mb(index_buffer_pool_size_in_mb)
	{
	}

	void ModelPool::Destroy(Model * model)
	{
		DestroyModel(model);
	}

	void ModelPool::Destroy(Mesh * mesh)
	{
		DestroyMesh(mesh);
	}

	template<>
	int ModelPool::LoadNodeMeshes<Vertex, std::uint32_t>(const aiScene * scene, aiNode * node, Model* model)
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

			Mesh* n_mesh = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(Vertex),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (n_mesh == nullptr)
			{
				return 1;
			}

			model->m_meshes.push_back(n_mesh);
		}
		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			int ret = LoadNodeMeshes<Vertex, std::uint32_t>(scene, node->mChildren[i], model);
			if (ret == 1)
				return 1;
		}
		return 0;
	}

	template<>
	int ModelPool::LoadNodeMeshes<VertexNoTangent, std::uint32_t>(const aiScene * scene, aiNode * node, Model* model)
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

			Mesh* n_mesh = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(VertexNoTangent),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (n_mesh == nullptr)
			{
				return 1;
			}

			model->m_meshes.push_back(n_mesh);
		}
		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			int ret = LoadNodeMeshes<VertexNoTangent, std::uint32_t>(scene, node->mChildren[i], model);
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

			Mesh* n_mesh = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(Vertex),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (n_mesh == nullptr)
			{
				return 1;
			}

			if (scene->HasMaterials())
			{
				n_mesh->m_material = materials[mesh->mMaterialIndex];
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

			Mesh* n_mesh = LoadCustom_VerticesAndIndices(
				vertices.data(),
				vertices.size(),
				sizeof(VertexNoTangent),
				indices.data(),
				indices.size(),
				sizeof(std::uint32_t));

			if (n_mesh == nullptr)
			{
				return 1;
			}

			if (scene->HasMaterials())
			{
				n_mesh->m_material = materials[mesh->mMaterialIndex];
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

} /* wr */