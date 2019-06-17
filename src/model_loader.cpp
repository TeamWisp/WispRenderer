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

#include "model_loader.hpp"

namespace wr
{
	WISPRENDERER_EXPORT std::vector<ModelLoader*> ModelLoader::m_registered_model_loaders = std::vector<ModelLoader*>();

	ModelLoader::ModelLoader()
	{
		m_registered_model_loaders.push_back(this);
	}

	ModelLoader::~ModelLoader()
	{
		std::vector<ModelData*> temp = m_loaded_models;
		for (ModelData* model : temp)
		{
			DeleteModel(model);
		}

		for (std::vector<ModelLoader*>::iterator it = m_registered_model_loaders.begin();
			it != m_registered_model_loaders.end(); ++it)
		{
			if ((*it) == this)
			{
				m_registered_model_loaders.erase(it);
				break;
			}
		}
	}

	ModelData * ModelLoader::Load(std::string_view model_path)
	{
		ModelData* model = LoadModel(model_path);
		m_loaded_models.push_back(model);
		return model;
	}

	ModelData * ModelLoader::Load(void * data, std::size_t length, std::string format)
	{
		ModelData* model = LoadModel(data, length, format);
		m_loaded_models.push_back(model);
		return model;
	}

	void ModelLoader::DeleteModel(ModelData * model)
	{
		std::vector<ModelData*>::iterator it = std::find(m_loaded_models.begin(), m_loaded_models.end(), model);

		if (it == m_loaded_models.end())
		{
			return;
		}

		m_loaded_models.erase(it);

		for (int i = 0; i < model->m_meshes.size(); ++i) 
		{
			delete model->m_meshes[i];
		}

		for (int i = 0; i < model->m_materials.size(); ++i) 
		{
			delete model->m_materials[i];
		}

		for (int i = 0; i < model->m_embedded_textures.size(); ++i)
		{
			delete model->m_embedded_textures[i];
		}

		for (int i = 0; i < model->m_skeleton_data->m_animations.size(); ++i)
		{
			delete model->m_skeleton_data->m_animations[i];
		}

		for (int i = 0; i < model->m_skeleton_data->m_bones.size(); ++i)
		{
			delete model->m_skeleton_data->m_bones[i];
		}

		delete model->m_skeleton_data;
		delete model;
	}

	std::vector<std::string> ModelLoader::SupportedModelFormats()
	{
		return m_supported_model_formats;
	}

	bool ModelLoader::SupportsModelFormat(const std::string_view& model_format)
	{
		for (int i = 0; i < m_supported_model_formats.size(); ++i)
		{
			if (m_supported_model_formats[i].compare(model_format) == 0)
			{
				return true;
			}
		}
		return false;
	}

	ModelLoader * ModelLoader::FindFittingModelLoader(const std::string_view & model_format)
	{
		for (int i = 0; i < m_registered_model_loaders.size(); ++i)
		{
			if (m_registered_model_loaders[i]->SupportsModelFormat(model_format))
			{
				return m_registered_model_loaders[i];
			}
		}
		return nullptr;
	}

}