/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "model_loader_assimp.hpp"

#include "util/log.hpp"

namespace wr
{

	AssimpModelLoader::AssimpModelLoader()
	{
		m_supported_model_formats = { "obj","fbx","raw","sib","smd" };
	}

	AssimpModelLoader::~AssimpModelLoader()
	{
	}

	ModelData * AssimpModelLoader::LoadModel(std::string_view model_path)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(model_path.data(),
			aiProcess_Triangulate |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_OptimizeMeshes |
			aiProcess_ImproveCacheLocality | 
			aiProcess_MakeLeftHanded);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOGW(std::string("Loading model ") +
				model_path.data() +
				std::string(" failed with error ") +
				importer.GetErrorString());
			return nullptr;
		}

		ModelData* model = new ModelData;

		if (scene->HasTextures())
		{
			LoadEmbeddedTextures(model, scene);
		}

		LoadMaterials(model, scene);
		LoadMeshes(model, scene, scene->mRootNode);

		model->m_skeleton_data = new ModelSkeletonData();

		return model;
	}

	ModelData * AssimpModelLoader::LoadModel(void * data, std::size_t length, std::string format)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFileFromMemory(data,
			length,
			aiProcess_Triangulate |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_OptimizeMeshes |
			aiProcess_ImproveCacheLocality |
			aiProcess_MakeLeftHanded,
			format.c_str());

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOGW(std::string("Loading model ") +
				std::string("failed with error ") +
				importer.GetErrorString());
			return nullptr;
		}

		ModelData* model = new ModelData;

		if (scene->HasTextures())
		{
			LoadEmbeddedTextures(model, scene);
		}

		LoadMaterials(model, scene);
		LoadMeshes(model, scene, scene->mRootNode);

		model->m_skeleton_data = new ModelSkeletonData();

		return model;
	}

	void AssimpModelLoader::LoadMeshes(ModelData * model, const aiScene * scene, aiNode * node)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; ++i)
		{
			ModelMeshData* mesh_data = new ModelMeshData();
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			//Copy data

			mesh_data->m_positions.resize(mesh->mNumVertices);
			mesh_data->m_normals.resize(mesh->mNumVertices);
			mesh_data->m_uvw.resize(mesh->mNumVertices);
			mesh_data->m_colors.resize(mesh->mNumVertices);
			mesh_data->m_tangents.resize(mesh->mNumVertices);
			mesh_data->m_bitangents.resize(mesh->mNumVertices);
			mesh_data->m_bone_weights.resize(mesh->mNumVertices);
			mesh_data->m_bone_ids.resize(mesh->mNumVertices);

			#ifdef ASSIMP_DOUBLE_PRECISION
			static_assert(false, "Assimp should use single precision, since model_loader_assimp.cpp requires floats");
			#endif

			const size_t float3 = sizeof(float) * 3, float3s = mesh->mNumVertices * float3;

			memcpy(mesh_data->m_positions.data(), mesh->mVertices, float3s);

			if (mesh->HasNormals())
			{
				memcpy(mesh_data->m_normals.data(), mesh->mNormals, float3s);
			}

			if (mesh->HasTangentsAndBitangents())
			{
				memcpy(mesh_data->m_tangents.data(), mesh->mTangents, float3s);
				memcpy(mesh_data->m_bitangents.data(), mesh->mBitangents, float3s);
			}

			if (mesh->HasTextureCoords(0))
			{
				memcpy(mesh_data->m_uvw.data(), mesh->mTextureCoords[0], float3s);
			}

			if (mesh->HasVertexColors(0))
			{
				for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
				{
					memcpy(&mesh_data->m_colors[j], &mesh->mColors[0][j], float3);
				}
			}

			//Copy indices

			size_t count = 0;

			for (size_t j = 0; j < mesh->mNumFaces; ++j)
			{
				aiFace *face = &mesh->mFaces[j];
				count += face->mNumIndices;
			}

			mesh_data->m_indices.resize(count);
			count = 0;

			for (size_t j = 0; j < mesh->mNumFaces; ++j)
			{
				aiFace *face = &mesh->mFaces[j];
				memcpy(mesh_data->m_indices.data() + count, face->mIndices, face->mNumIndices * 4);
				count += face->mNumIndices;
			}

			mesh_data->m_material_id = mesh->mMaterialIndex;

			model->m_meshes.push_back(mesh_data);
		}

		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			LoadMeshes(model, scene, node->mChildren[i]);
		}
	}

	void AssimpModelLoader::LoadMaterials(ModelData * model, const aiScene * scene)
	{
		model->m_materials.resize(scene->mNumMaterials);
		for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
		{
			aiMaterial* material = scene->mMaterials[i];

			ModelMaterialData* material_data = new ModelMaterialData;

			if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
			{
				aiString path;
				material->GetTexture(aiTextureType_DIFFUSE, 0, &path);

				if (path.data[0] == '*')
				{
					uint32_t index = atoi(path.C_Str() + 1);
					material_data->m_albedo_embedded_texture = index;
					material_data->m_albedo_texture_location = TextureLocation::EMBEDDED;
				}
				else
				{
					material_data->m_albedo_texture = std::string(path.C_Str());
					material_data->m_albedo_texture_location = TextureLocation::EXTERNAL;
				}
			}
			else
			{
				material_data->m_albedo_texture_location = TextureLocation::NON_EXISTENT;
			}

			if (material->GetTextureCount(aiTextureType_SPECULAR) > 0)
			{
				aiString path;
				material->GetTexture(aiTextureType_SPECULAR, 0, &path);

				if (path.data[0] == '*')
				{
					uint32_t index = atoi(path.C_Str() + 1);
					material_data->m_metallic_embedded_texture = index;
					material_data->m_metallic_texture_location = TextureLocation::EMBEDDED;
				}
				else
				{
					material_data->m_metallic_texture = std::string(path.C_Str());
					material_data->m_metallic_texture_location = TextureLocation::EXTERNAL;
				}
			}
			else
			{
				material_data->m_metallic_texture_location = TextureLocation::NON_EXISTENT;
			}

			if (material->GetTextureCount(aiTextureType_SHININESS) > 0)
			{
				aiString path;
				material->GetTexture(aiTextureType_SHININESS, 0, &path);

				if (path.data[0] == '*')
				{
					uint32_t index = atoi(path.C_Str() + 1);
					material_data->m_roughness_embedded_texture = index;
					material_data->m_roughness_texture_location = TextureLocation::EMBEDDED;
				}
				else
				{
					material_data->m_roughness_texture = std::string(path.C_Str());
					material_data->m_roughness_texture_location = TextureLocation::EXTERNAL;
				}
			}
			else
			{
				material_data->m_roughness_texture_location = TextureLocation::NON_EXISTENT;
			}

			if (material->GetTextureCount(aiTextureType_AMBIENT) > 0)
			{
				aiString path;
				material->GetTexture(aiTextureType_AMBIENT, 0, &path);

				if (path.data[0] == '*')
				{
					uint32_t index = atoi(path.C_Str() + 1);
					material_data->m_ambient_occlusion_embedded_texture = index;
					material_data->m_ambient_occlusion_texture_location = TextureLocation::EMBEDDED;
				}
				else
				{
					material_data->m_ambient_occlusion_texture = std::string(path.C_Str());
					material_data->m_ambient_occlusion_texture_location = TextureLocation::EXTERNAL;
				}
			}
			else
			{
				material_data->m_ambient_occlusion_texture_location = TextureLocation::NON_EXISTENT;
			}

			if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
			{
				aiString path;
				material->GetTexture(aiTextureType_NORMALS, 0, &path);

				if (path.data[0] == '*')
				{
					uint32_t index = atoi(path.C_Str() + 1);
					material_data->m_normal_map_embedded_texture = index;
					material_data->m_normal_map_texture_location = TextureLocation::EMBEDDED;
				}
				else
				{
					material_data->m_normal_map_texture = std::string(path.C_Str());
					material_data->m_normal_map_texture_location = TextureLocation::EXTERNAL;
				}
			}
			else
			{
				material_data->m_normal_map_texture_location = TextureLocation::NON_EXISTENT;
			}

			if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0)
			{
				aiString path;
				material->GetTexture(aiTextureType_EMISSIVE, 0, &path);

				if (path.data[0] == '*')
				{
					uint32_t index = atoi(path.C_Str() + 1);
					material_data->m_emissive_embedded_texture = index;
					material_data->m_emissive_texture_location = TextureLocation::EMBEDDED;
				}
				else
				{
					material_data->m_emissive_texture = std::string(path.C_Str());
					material_data->m_emissive_texture_location = TextureLocation::EXTERNAL;
				}
			}
			else
			{
				material_data->m_emissive_texture_location = TextureLocation::NON_EXISTENT;
			}

			aiColor3D color;
			material->Get(AI_MATKEY_COLOR_DIFFUSE, color);

			memcpy(&material_data->m_base_color, &color, sizeof(color));

			material->Get(AI_MATKEY_COLOR_SPECULAR, color);

			memcpy(&material_data->m_base_metallic, &color, sizeof(color));

			float roughness;
			material->Get(AI_MATKEY_SHININESS, roughness);
			material_data->m_base_roughness = roughness;

			float opacity;
			material->Get(AI_MATKEY_OPACITY, opacity);
			material_data->m_base_transparency = opacity;

			int two_sided;
			material->Get(AI_MATKEY_TWOSIDED, two_sided);
			material_data->m_two_sided = two_sided > 0;

			model->m_materials[i] = material_data;
		}
	}

	void AssimpModelLoader::LoadEmbeddedTextures(ModelData * model, const aiScene * scene)
	{
		model->m_embedded_textures.resize(scene->mNumTextures);
		for (unsigned int i = 0; i < scene->mNumTextures; ++i) 
		{
			EmbeddedTexture* texture = new EmbeddedTexture;

			texture->m_compressed = scene->mTextures[i]->mHeight == 0 ? true : false;
			texture->m_format = std::string(scene->mTextures[i]->achFormatHint);

			texture->m_width = scene->mTextures[i]->mWidth;
			texture->m_height = scene->mTextures[i]->mHeight;

			if (texture->m_compressed)
			{
				texture->m_data.resize(texture->m_width);
				memcpy(texture->m_data.data(), scene->mTextures[i]->pcData, texture->m_width);
			}
			else
			{
				texture->m_data.resize(texture->m_width*texture->m_height * 4);
				for (unsigned int j = 0; j < texture->m_width*texture->m_height; ++j)
				{
					texture->m_data[j * 4 + 0] = scene->mTextures[i]->pcData[j].r;
					texture->m_data[j * 4 + 1] = scene->mTextures[i]->pcData[j].g;
					texture->m_data[j * 4 + 2] = scene->mTextures[i]->pcData[j].b;
					texture->m_data[j * 4 + 3] = scene->mTextures[i]->pcData[j].a;
				}
			}

			model->m_embedded_textures[i] = texture;
		}
	}

}