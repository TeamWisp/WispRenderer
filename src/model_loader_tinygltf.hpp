#pragma once

#include "model_loader.hpp"

namespace wr
{
	class TinyGLTFModelLoader : public ModelLoader
	{
	public:
		TinyGLTFModelLoader();

		~TinyGLTFModelLoader() final;

	protected:
		ModelData* LoadModel(std::string_view model_path) final;
		ModelData* LoadModel(void* data, std::size_t length, std::string format) final;

	private:
		//void LoadMeshes(ModelData* model, tinygltf::Model tg_model, tinygltf::Node node);
		//void LoadMaterials(ModelData* model, const aiScene* scene);
		//void LoadEmbeddedTextures(ModelData* model, const aiScene* scene);
	};
}