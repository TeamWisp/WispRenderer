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