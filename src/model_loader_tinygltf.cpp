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
#include "model_loader_tinygltf.hpp"

#include "util/log.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

namespace wr
{

	TinyGLTFModelLoader::TinyGLTFModelLoader()
	{
		m_supported_model_formats = { "gltf" };
	}

	TinyGLTFModelLoader::~TinyGLTFModelLoader()
	{
	}

	std::pair<std::vector<DirectX::XMFLOAT3>, std::vector<DirectX::XMFLOAT3>> ComputeTangents(ModelMeshData* mesh_data)
	{
		size_t vtxCount = mesh_data->m_positions.size();
		std::vector<DirectX::XMFLOAT3> tanA(vtxCount, { 0, 0, 0 });
		std::vector<DirectX::XMFLOAT3> tanB(vtxCount, { 0, 0, 0 });

		if (mesh_data->m_uvw.empty())
		{
			return { tanA, tanB };
		}

		// (1)
		size_t indexCount = mesh_data->m_indices.size();
		for (size_t i = 0; i < mesh_data->m_indices.size(); i += 3) {
			size_t i0 = mesh_data->m_indices[i];
			size_t i1 = mesh_data->m_indices[i + 1];
			size_t i2 = mesh_data->m_indices[i + 2];

			auto pos0 = DirectX::XMLoadFloat3(&mesh_data->m_positions[i0]);
			auto pos1 = DirectX::XMLoadFloat3(&mesh_data->m_positions[i1]);
			auto pos2 = DirectX::XMLoadFloat3(&mesh_data->m_positions[i2]);

			auto tex0 = DirectX::XMLoadFloat2(&DirectX::XMFLOAT2{ mesh_data->m_uvw[i0].x, mesh_data->m_uvw[i0].y });
			auto tex1 = DirectX::XMLoadFloat2(&DirectX::XMFLOAT2{ mesh_data->m_uvw[i1].x, mesh_data->m_uvw[i1].y });
			auto tex2 = DirectX::XMLoadFloat2(&DirectX::XMFLOAT2{ mesh_data->m_uvw[i2].x, mesh_data->m_uvw[i2].y });

			DirectX::XMFLOAT3 edge1, edge2;
			DirectX::XMStoreFloat3(&edge1, DirectX::XMVectorSubtract(pos1, pos0));
			DirectX::XMStoreFloat3(&edge2, DirectX::XMVectorSubtract(pos2, pos0));

			DirectX::XMFLOAT2 uv1, uv2;
			DirectX::XMStoreFloat2(&uv1, DirectX::XMVectorSubtract(tex1, tex0));
			DirectX::XMStoreFloat2(&uv2, DirectX::XMVectorSubtract(tex2, tex0));

			float r = 1.0f / (uv1.x * uv2.y - uv1.y * uv2.x);

			DirectX::XMFLOAT3 tangent(
				((edge1.x * uv2.y) - (edge2.x * uv1.y)) * r,
				((edge1.y * uv2.y) - (edge2.y * uv1.y)) * r,
				((edge1.z * uv2.y) - (edge2.z * uv1.y)) * r
			);

			DirectX::XMFLOAT3 bitangent(
				((edge1.x * uv2.x) - (edge2.x * uv1.x)) * r,
				((edge1.y * uv2.x) - (edge2.y * uv1.x)) * r,
				((edge1.z * uv2.x) - (edge2.z * uv1.x)) * r
			);

			tanA[i0] = tangent;
			tanA[i1] = tangent;
			tanA[i2] = tangent;

			tanB[i0] = bitangent;
			tanB[i1] = bitangent;
			tanB[i2] = bitangent;

		}

		return { tanA, tanB };
	}

	inline void LoadMaterial(ModelData* model, tinygltf::Model tg_model, tinygltf::Material mat)
	{
		ModelMaterialData* mat_data = new ModelMaterialData();
				
		mat_data->m_albedo_texture_location = TextureLocation::NON_EXISTENT;
		mat_data->m_normal_map_texture_location = TextureLocation::NON_EXISTENT;
		mat_data->m_roughness_texture_location = TextureLocation::NON_EXISTENT;
		mat_data->m_metallic_texture_location = TextureLocation::NON_EXISTENT;
		mat_data->m_emissive_texture_location = TextureLocation::NON_EXISTENT;
		mat_data->m_ambient_occlusion_texture_location = TextureLocation::NON_EXISTENT;

		for (auto value : mat.values)
		{
			if (value.first == "baseColorTexture")
			{
				auto img = tg_model.images[value.second.json_double_value.begin()->second];						

				mat_data->m_albedo_texture_location = TextureLocation::EXTERNAL;
				mat_data->m_albedo_texture = img.uri;
			}
			else if (value.first == "metallicRoughnessTexture")
			{
				auto img = tg_model.images[value.second.json_double_value.begin()->second];

				mat_data->m_metallic_texture_location = TextureLocation::EXTERNAL;
				mat_data->m_metallic_texture = img.uri;
				mat_data->m_roughness_texture_location = TextureLocation::EXTERNAL;
				mat_data->m_roughness_texture = img.uri;
			}
			else if (value.first == "roughnessFactor")
			{
				auto factor = value.second.Factor();
				mat_data->m_base_roughness = factor;
			}
			else if (value.first == "metallicFactor")
			{
				auto factor = value.second.Factor();
				mat_data->m_base_metallic = factor;
			}
		}

		for (auto value : mat.additionalValues)
		{
			if (value.first == "normalTexture")
			{
				auto img = tg_model.images[value.second.json_double_value.begin()->second];

				mat_data->m_normal_map_texture_location = TextureLocation::EXTERNAL;
				mat_data->m_normal_map_texture = img.uri;
			}
			else if (value.first == "occlusionTexture")
			{
				auto img = tg_model.images[value.second.json_double_value.begin()->second];

				mat_data->m_ambient_occlusion_texture_location = TextureLocation::EXTERNAL;
				mat_data->m_ambient_occlusion_texture = img.uri;
			}
			else if (value.first == "emissiveTexture")
			{
				auto img = tg_model.images[value.second.json_double_value.begin()->second];

				mat_data->m_emissive_texture_location = TextureLocation::EXTERNAL;
				mat_data->m_emissive_texture = img.uri;
			}
			else if (value.first == "emissiveFactor")
			{
				auto factor = value.second.ColorFactor();
				mat_data->m_base_emissive = factor[0] + factor[1] + factor[2];
			}
		}

		model->m_materials.push_back(mat_data);
	}

	inline void LoadMesh(ModelData* model, tinygltf::Model tg_model, tinygltf::Node node, DirectX::XMMATRIX parent_transform)
	{
		auto mesh = tg_model.meshes[node.mesh];

		auto translation = node.translation;
		auto rotation = node.rotation;
		auto scale = node.scale;
		auto matrix = node.matrix;

		for (auto primitive : mesh.primitives)
		{
			std::size_t idx_buffer_offset = 0;
			std::size_t uv_buffer_offset = 0;
			std::size_t position_buffer_offset = 0;
			std::size_t normal_buffer_offset = 0;
			ModelMeshData* mesh_data = new ModelMeshData();

			{ // GET INDICES
				auto idx_accessor = tg_model.accessors[primitive.indices];

				const auto& buffer_view = tg_model.bufferViews[idx_accessor.bufferView];
				const auto& buffer = tg_model.buffers[buffer_view.buffer];
				const auto data_address = buffer.data.data() + buffer_view.byteOffset + idx_accessor.byteOffset;
				const auto byte_stride = idx_accessor.ByteStride(buffer_view);

				mesh_data->m_indices.resize(idx_buffer_offset + idx_accessor.count);

				memcpy(mesh_data->m_indices.data() + idx_buffer_offset, data_address, idx_accessor.count * byte_stride);
				idx_buffer_offset += idx_accessor.count * byte_stride;
			}

			// Get other attributes
			for (const auto& attrib : primitive.attributes)
			{
				const auto attrib_accessor = tg_model.accessors[attrib.second];
				const auto& buffer_view = tg_model.bufferViews[attrib_accessor.bufferView];
				const auto& buffer = tg_model.buffers[buffer_view.buffer];
				const auto data_address = buffer.data.data() + buffer_view.byteOffset + attrib_accessor.byteOffset;
				const auto byte_stride = attrib_accessor.ByteStride(buffer_view);
				const auto count = attrib_accessor.count;
				if (attrib.first == "POSITION")
				{
					mesh_data->m_positions.resize(position_buffer_offset + count);
					mesh_data->m_colors.resize(position_buffer_offset + count);

					memcpy(mesh_data->m_positions.data() + position_buffer_offset, data_address, count * byte_stride);
					position_buffer_offset += count * byte_stride;
				}
				else if (attrib.first == "NORMAL")
				{
					mesh_data->m_normals.resize(normal_buffer_offset + count);

					memcpy(mesh_data->m_normals.data() + normal_buffer_offset, data_address, count * byte_stride);
					normal_buffer_offset += count * byte_stride;
				}
				else if (attrib.first == "TEXCOORD_0")
				{
					std::vector<DirectX::XMFLOAT2> f2_data;
					f2_data.resize(count);

					memcpy(f2_data.data(), data_address, count * byte_stride);

					for (auto& f2 : f2_data)
					{
						mesh_data->m_uvw.push_back({ f2.x, f2.y*-1, 0 });
					}

					uv_buffer_offset += count * sizeof(DirectX::XMFLOAT3);
				}
			}

			// Apply Transformation
			for (auto& position : mesh_data->m_positions)
			{
				auto transformed_pos = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&position), parent_transform);
				DirectX::XMStoreFloat3(&position, transformed_pos);
			}

			auto tangent_bitangent = ComputeTangents(mesh_data);
			mesh_data->m_tangents = tangent_bitangent.first;
			mesh_data->m_bitangents = tangent_bitangent.second;
			mesh_data->m_uvw.resize(mesh_data->m_positions.size());
			mesh_data->m_material_id = primitive.material;

			model->m_meshes.push_back(mesh_data);
		}
	}

	ModelData* TinyGLTFModelLoader::LoadModel(std::string_view model_path)
	{
		tinygltf::Model tg_model;
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;
		std::string path = std::string(model_path);

		if (!loader.LoadASCIIFromFile(&tg_model, &err, &warn, path))
		{
			LOGC("TinyGLTF Parsing Failed");
		}

		if (!warn.empty())
		{
			LOGW("TinyGLTF Warning: {}", warn);
		}

		if (!err.empty())
		{
			LOGE("TinyGLTF Error: {}", err);
		}

		auto model = new ModelData();
		model->m_skeleton_data = new wr::ModelSkeletonData();

		for (auto mat : tg_model.materials)
		{
			LoadMaterial(model, tg_model, mat);
		}

		std::function<void(int, DirectX::XMMATRIX)> recursive_func = [&](int node_id, DirectX::XMMATRIX parent_transform)
		{
			auto node = tg_model.nodes[node_id];

			auto translation = node.translation;
			auto scale = node.scale;
			auto rotation = node.rotation;
			auto matrix = node.matrix;

			DirectX::XMMATRIX transform;

			if (matrix.empty())
			{
				DirectX::XMMATRIX translation_mat = translation.empty() ? DirectX::XMMatrixIdentity() : DirectX::XMMatrixTranslationFromVector({ (float)translation[0], (float)translation[1], (float)translation[2] });
				DirectX::XMMATRIX rotation_mat = rotation.empty() ? DirectX::XMMatrixIdentity() : DirectX::XMMatrixRotationQuaternion({ (float)rotation[0], (float)rotation[1], (float)rotation[2], (float)rotation[3] });
				DirectX::XMMATRIX scale_mat = scale.empty() ? DirectX::XMMatrixIdentity() : DirectX::XMMatrixScalingFromVector({ (float)scale[0], (float)scale[1], (float)scale[2] });
				transform = scale_mat * rotation_mat * translation_mat;
			}
			else
			{
				transform.r[0].m128_f32[0] = matrix[0];
				transform.r[0].m128_f32[1] = matrix[1];
				transform.r[0].m128_f32[2] = matrix[2];
				transform.r[0].m128_f32[3] = matrix[3];

				transform.r[1].m128_f32[0] = matrix[4];
				transform.r[1].m128_f32[1] = matrix[5];
				transform.r[1].m128_f32[2] = matrix[6];
				transform.r[1].m128_f32[3] = matrix[7];

				transform.r[2].m128_f32[0] = matrix[8];
				transform.r[2].m128_f32[1] = matrix[9];
				transform.r[2].m128_f32[2] = matrix[10];
				transform.r[2].m128_f32[3] = matrix[11];

				transform.r[3].m128_f32[0] = matrix[12];
				transform.r[3].m128_f32[1] = matrix[13];
				transform.r[3].m128_f32[2] = matrix[14];
				transform.r[3].m128_f32[3] = matrix[15];

			}

			parent_transform = parent_transform * transform;

			if (node.mesh > -1)
			{
				LoadMesh(model, tg_model, node, parent_transform);
			}

			for (auto child_id : node.children)
			{
				recursive_func(child_id, parent_transform);
			}
		};

		DirectX::XMMATRIX parent_transform = DirectX::XMMatrixIdentity();
		for (auto node_id : tg_model.scenes[tg_model.defaultScene].nodes)
		{
			recursive_func(node_id, parent_transform);
		}

		return model;
	}

	ModelData* TinyGLTFModelLoader::LoadModel(void* data, std::size_t length, std::string format)
	{
		return new ModelData();
	}

}