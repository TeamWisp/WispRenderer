#include "model_loader_assimp.hpp"

#include "util/log.hpp"

namespace wr
{

	AssimpModelLoader::AssimpModelLoader()
	{
		m_supported_model_formats = { "obj","fbx","raw","sib","smd","gltf" };
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
		for (int i = 0; i < node->mNumMeshes; ++i)
		{
			ModelMeshData* mesh_data = new ModelMeshData();
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			mesh_data->m_positions.resize(mesh->mNumVertices);
			mesh_data->m_normals.resize(mesh->mNumVertices);
			mesh_data->m_uvw.resize(mesh->mNumVertices);
			mesh_data->m_colors.resize(mesh->mNumVertices);
			mesh_data->m_tangents.resize(mesh->mNumVertices);
			mesh_data->m_bitangents.resize(mesh->mNumVertices);
			mesh_data->m_bone_weights.resize(mesh->mNumVertices);
			mesh_data->m_bone_ids.resize(mesh->mNumVertices);

			for (int j = 0; j < mesh->mNumVertices; ++j)
			{
				mesh_data->m_positions[j].x = mesh->mVertices[j].x;
				mesh_data->m_positions[j].y = mesh->mVertices[j].y;
				mesh_data->m_positions[j].z = mesh->mVertices[j].z;

				if (mesh->HasNormals())
				{
					mesh_data->m_normals[j].x = mesh->mNormals[j].x;
					mesh_data->m_normals[j].y = mesh->mNormals[j].y;
					mesh_data->m_normals[j].z = mesh->mNormals[j].z;
				}

				if (mesh->mTextureCoords[0] > 0)
				{
					mesh_data->m_uvw[j][0] = mesh->mTextureCoords[0][j].x;
					mesh_data->m_uvw[j][1] = 1.f - mesh->mTextureCoords[0][j].y;
					mesh_data->m_uvw[j][2] = mesh->mTextureCoords[0][j].z;
				}

				if (mesh->HasVertexColors(0))
				{
					mesh_data->m_colors[j].x = mesh->mColors[0][j].r;
					mesh_data->m_colors[j].y = mesh->mColors[0][j].g;
					mesh_data->m_colors[j].z = mesh->mColors[0][j].b;
				}

				if (mesh->HasTangentsAndBitangents())
				{
					mesh_data->m_tangents[j].x = mesh->mTangents[j].x;
					mesh_data->m_tangents[j].y = mesh->mTangents[j].y;
					mesh_data->m_tangents[j].z = mesh->mTangents[j].z;

					mesh_data->m_bitangents[j].x = mesh->mBitangents[j].x;
					mesh_data->m_bitangents[j].y = mesh->mBitangents[j].y;
					mesh_data->m_bitangents[j].z = mesh->mBitangents[j].z;
				}
			}

			for (size_t j = 0; j < mesh->mNumFaces; ++j)
			{
				aiFace *face = &mesh->mFaces[j];

				// retrieve all indices of the face and store them in the indices vector
				for (size_t k = 0; k < face->mNumIndices; k++)
				{
					mesh_data->m_indices.push_back(static_cast<unsigned>(face->mIndices[k]));
				}
			}

			mesh_data->m_material_id = mesh->mMaterialIndex;

			model->m_meshes.push_back(mesh_data);
		}

		for (int i = 0; i < node->mNumChildren; ++i)
		{
			LoadMeshes(model, scene, node->mChildren[i]);
		}
	}

	void AssimpModelLoader::LoadMaterials(ModelData * model, const aiScene * scene)
	{
		model->m_materials.resize(scene->mNumMaterials);
		for (int i = 0; i < scene->mNumMaterials; ++i)
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

			aiColor3D color;
			material->Get(AI_MATKEY_COLOR_DIFFUSE, color);

			material_data->m_base_color.x = color.r;
			material_data->m_base_color.y = color.g;
			material_data->m_base_color.z = color.b;

			material->Get(AI_MATKEY_COLOR_SPECULAR, color);

			material_data->m_base_metallic.x = color.r;
			material_data->m_base_metallic.y = color.g;
			material_data->m_base_metallic.z = color.b;

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
		for (int i = 0; i < scene->mNumTextures; ++i) 
		{
			EmbeddedTexture* texture = new EmbeddedTexture;

			texture->m_compressed = scene->mTextures[i]->mHeight == 0 ? true : false;
			texture->format = std::string(scene->mTextures[i]->achFormatHint);

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
				for (int j = 0; j < texture->m_width*texture->m_height; ++j)
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