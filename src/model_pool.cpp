#define NOMINMAX 1
#include "model_pool.hpp"

 #include <assimp/Importer.hpp>
 #include <assimp/scene.h>
 #include <assimp/postprocess.h>
#include "util/log.hpp"
#include "vertex.hpp"

namespace wr
{
	ModelPool::ModelPool(std::size_t vertex_buffer_pool_size_in_mb,
		std::size_t index_buffer_pool_size_in_mb) : 
		m_vertex_buffer_pool_size_in_mb(vertex_buffer_pool_size_in_mb),
		m_index_buffer_pool_size_in_mb(index_buffer_pool_size_in_mb)
	{
	}

	//! Loads a model without materials
	Model* ModelPool::Load(std::string_view path, ModelType type)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path.data(),
			aiProcess_Triangulate |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_OptimizeMeshes |
			aiProcess_ImproveCacheLocality |
			aiProcess_MakeLeftHanded);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOGW(std::string("Loading model ") +
				path.data() + 
				std::string(" failed with error ") + 
				importer.GetErrorString());
			return nullptr;
		}

		Model* model = new Model;

		std::vector<aiNode*> meshes;
		FindMeshes(scene, scene->mRootNode, meshes);

		if (!LoadMeshes(scene, meshes, model))
		{
			DestroyModel(model);
			return nullptr;
		}

		return model;
	}

	//! Loads a model with materials
	std::pair<Model*, std::vector<MaterialHandle*>> ModelPool::LoadWithMaterials(MaterialPool* material_pool, std::string_view path, ModelType type)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path.data(),
			aiProcess_Triangulate |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_OptimizeMeshes |
			aiProcess_ImproveCacheLocality |
			aiProcess_MakeLeftHanded);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOGW(std::string("Loading model ") +
				path.data() +
				std::string(" failed with error ") +
				importer.GetErrorString());
			return std::pair<Model*, std::vector<MaterialHandle*>>();
		}

		Model* model = new Model;
		std::vector<MaterialHandle*> material_handles;

		for (int i = 0; i < scene->mNumMaterials; ++i) 
		{
			material_handles.push_back(material_pool->Load(scene->mMaterials[i]));
		}

		std::vector<aiNode*> meshes;
		FindMeshes(scene, scene->mRootNode, meshes);

		if (!LoadMeshes(scene, meshes, model, material_handles))
		{
			DestroyModel(model);
			return { nullptr, {} };
		}

		return { model, material_handles };
	}

	void ModelPool::Destroy(Model * model)
	{
		DestroyModel(model);
	}

	void ModelPool::Destroy(Mesh * mesh)
	{
		DestroyMesh(mesh);
	}

	void ModelPool::FindMeshes(const aiScene* scene, aiNode* node, std::vector<aiNode*>& meshes)
	{
		if (node->mNumMeshes != 0)
		{
			meshes.push_back(node);
		}

		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			FindMeshes(scene, node->mChildren[i], meshes);
		}
	}

	bool ModelPool::LoadMeshes(const aiScene* scene, std::vector<aiNode*> nodes, Model* model, std::vector<MaterialHandle*> materials)
	{

		//Generate indices to find the mesh node from mesh id

		uint32_t mesh_count = 0;

		for (aiNode* node : nodes)
		{
			mesh_count += node->mNumMeshes;
		}

		//Get offsets and number of indices and vertices

		std::vector<uint32_t> index_offsets(mesh_count);
		std::vector<uint32_t> vertex_offsets(mesh_count);

		uint32_t index_offset = 0, vertex_offset = 0;

		for (uint32_t i = 0, j = (uint32_t) nodes.size(), l = 0; i < j; ++i)
		{
			aiNode* node = nodes[i];

			for (uint32_t k = 0; k < node->mNumMeshes; ++k)
			{
				aiMesh* mesh = scene->mMeshes[node->mMeshes[k]];

				index_offsets[l] = index_offset;
				vertex_offsets[l] = vertex_offset;

				uint32_t indices = 0;

				for (uint32_t m = 0; m < mesh->mNumFaces; ++m)
				{
					aiFace *face = mesh->mFaces + m;
					indices += face->mNumIndices;
				}

				index_offset += indices;
				vertex_offset += mesh->mNumVertices;
				++l;
			}
		}

		//Setup compression details as max bounds (so first vertex becomes min & max)

		CompressedVertex::Details& compression_details = model->m_compression_details;

		float* pos_start = compression_details.m_pos_start;
		float* uv_start = compression_details.m_uv_start;
		float pos_end[3], uv_end[2];

		if (!compression_details.m_is_defined)
		{
			float f32_max = std::numeric_limits<float>::max();
			pos_start[0] = pos_start[1] = pos_start[2] = uv_start[0] = uv_start[1] = f32_max;

			float f32_min = std::numeric_limits<float>::min();
			pos_end[0] = pos_end[1] = pos_end[2] = uv_end[0] = uv_end[1] = f32_min;
		}

		//Compress directions and detect bounds

		std::vector<CompressedVertex> vertices(vertex_offset);
		std::vector<uint32_t> indices(index_offset);

		for (uint32_t i = 0, j = (uint32_t)nodes.size(), l = 0; i < j; ++i)
		{
			aiNode* node = nodes[i];

			for (uint32_t k = 0; k < node->mNumMeshes; ++k)
			{
				aiMesh* mesh = scene->mMeshes[node->mMeshes[k]];

				CompressedVertex* vert_start = vertices.data() + vertex_offsets[l];
				uint32_t* ind_start = indices.data() + index_offsets[l];

				uint32_t index = 0;

				for (uint32_t m = 0; m < mesh->mNumFaces; ++m)
				{
					aiFace* face = mesh->mFaces + m;
					memcpy(ind_start + index, face->mIndices, face->mNumIndices * 4);
					index += face->mNumIndices;
				}

				for (uint32_t m = 0; m < mesh->mNumVertices; ++m)
				{
					CompressedVertex::CompressDirection((float*)(mesh->mNormals + m), (vert_start + m)->m_normal);
					CompressedVertex::CompressDirection((float*)(mesh->mTangents + m), (vert_start + m)->m_tangent);
					CompressedVertex::CompressDirection((float*)(mesh->mBitangents + m), (vert_start + m)->m_bitangent);

					CompressedVertex::DetectBoundsPosition((float*)(mesh->mVertices + m), pos_start, pos_end);
					CompressedVertex::DetectBoundsUv((float*)(mesh->mTextureCoords[0] + m), uv_start, uv_end);
				}

				++l;
			}

		}

		//Finalize compression details

		if (!compression_details.m_is_defined)
		{
			compression_details.m_is_defined = 1;
			compression_details.m_pos_length[0] = pos_end[0] - pos_start[0];
			compression_details.m_pos_length[1] = pos_end[1] - pos_start[1];
			compression_details.m_pos_length[2] = pos_end[2] - pos_start[2];
			compression_details.m_uv_length[0] = uv_end[0] - uv_start[0];
			compression_details.m_uv_length[1] = uv_end[1] - uv_start[1];
		}

		//Compress position & uv using bounds

		for (uint32_t i = 0, j = (uint32_t) nodes.size(), l = 0; i < j; ++i)
		{
			aiNode* node = nodes[i];

			for (uint32_t k = 0; k < node->mNumMeshes; ++k)
			{
				aiMesh* mesh = scene->mMeshes[node->mMeshes[k]];
				CompressedVertex* vert_start = vertices.data() + vertex_offsets[l];
				uint32_t* ind_start = indices.data() + index_offsets[l];

				for (uint32_t m = 0; m < mesh->mNumVertices; ++m)
				{
					CompressedVertex::CompressPosition((float*)(mesh->mVertices + m), (vert_start + m)->m_pos, compression_details);
					CompressedVertex::CompressUv((float*)(mesh->mTextureCoords[0] + m), (vert_start + m)->m_uv_x, (vert_start + m)->m_uv_y, compression_details);
				}

				uint32_t curr_index = k == node->mNumMeshes - 1 && i == j - 1 ? index_offset : index_offsets[l + 1];
				uint32_t index_count = curr_index - index_offsets[l];

				Mesh* n_mesh = LoadCustom_VerticesAndIndices(
					vert_start,
					mesh->mNumVertices,
					sizeof(wr::CompressedVertex),
					ind_start,
					index_count,
					sizeof(std::uint32_t));

				if (n_mesh == nullptr)
				{
					return false;
				}

				if (scene->HasMaterials() && materials.size() != 0)
				{
					n_mesh->material = materials[mesh->mMaterialIndex];
				}

				model->m_meshes.push_back(n_mesh);

				++l;
			}
		}

		return true;
	}

} /* wr */