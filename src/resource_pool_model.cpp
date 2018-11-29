#include "resource_pool_model.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "util/log.hpp"

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
			aiProcess_ImproveCacheLocality);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOGW(std::string("Loading model ") +
				path.data() + 
				std::string(" failed with error ") + 
				importer.GetErrorString());
			return nullptr;
		}

	}

	//! Loads a model with materials
	std::pair<Model*, std::vector<MaterialHandle>> ModelPool::LoadWithMaterials(MaterialPool* material_pool, std::string_view path, ModelType type)
	{
		auto model = new Model();
		return std::pair<Model*, std::vector<MaterialHandle>>();
	}

	void ModelPool::Destroy(Model * model)
	{
		DestroyModel(model);
	}

	void ModelPool::Destroy(Mesh * mesh)
	{
		DestroyMesh(mesh);
	}

} /* wr */