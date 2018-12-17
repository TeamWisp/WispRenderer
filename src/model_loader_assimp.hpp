#pragma once

#include "model_loader.hpp"

#undef min
#undef max
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace wr
{
	class AssimpModelLoader : public ModelLoader
	{
	public:
		AssimpModelLoader();

		~AssimpModelLoader() final;

	protected:
		ModelData* LoadModel(std::string_view model_path) final;
		ModelData* LoadModel(void* data, std::size_t length, std::string format) final;

	private:
		void LoadMeshes(ModelData* model, const aiScene* scene, aiNode* node);
		void LoadMaterials(ModelData* model, const aiScene* scene);
		void LoadEmbeddedTextures(ModelData* model, const aiScene* scene);
	};
}