#include "model_loader.hpp"

namespace wr
{
	ModelLoader::~ModelLoader()
	{
		for (ModelData* model : m_loaded_models)
		{
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
		std::vector<ModelData*>::iterator it;
		for (it = m_loaded_models.begin(); it != m_loaded_models.end(); ++it)
		{
			if ((*it) == model)
				break;
		}

		if (it == m_loaded_models.end())
			return;

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

	bool ModelLoader::SupportsModelFormat(std::string model_type)
	{
		for (int i = 0; i < m_supported_model_formats.size(); ++i)
		{
			if (m_supported_model_formats[i].compare(model_type) == 0)
				return true;
		}
		return false;
	}

}