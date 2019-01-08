#pragma once

#include "scene_graph.hpp"

namespace wr
{

	struct MeshNode : Node
	{
		Model* m_model;
		std::vector<MaterialHandle*> m_materials;

		DirectX::XMVECTOR m_aabb[2];

		MeshNode(Model* model) : m_model(model) 
		{ 
			m_materials.resize(m_model->m_meshes.size());
			for (int i = 0; i < m_materials.size(); ++i)
			{
				m_materials[i] = m_model->m_meshes[i].second;
			}
		}

		void SetMeshMaterial(int mesh_index, MaterialHandle* material)
		{
			if (mesh_index < m_materials.size())
			{
				m_materials[mesh_index] = material;
			}
		}

		void SetMeshMaterials(Mesh* mesh, MaterialHandle* material)
		{
			for (int i = 0; i < m_model->m_meshes.size(); ++i)
			{
				if (m_model->m_meshes[i].first->id == mesh->id)
				{
					m_materials[i] = material;
				}
			}
		}

		MaterialHandle* GetMeshMaterial(int mesh_index)
		{
			if (mesh_index < m_materials.size())
			{
				return m_materials[mesh_index];
			}
		}

		std::vector<MaterialHandle*> GetMeshMaterials(Mesh* mesh)
		{
			std::vector<MaterialHandle*> materials;
			for (int i = 0; i < m_model->m_meshes.size(); ++i)
			{
				if (m_model->m_meshes[i].first->id == mesh->id)
				{
					materials.push_back(m_materials[i]);
				}
			}
		}

		void RestoreMeshMaterial(int mesh_index)
		{
			if (mesh_index < m_materials.size())
			{
				m_materials[mesh_index] = m_model->m_meshes[mesh_index].second;
			}
		}

		void RestoreMeshMaterials(Mesh* mesh)
		{
			for (int i = 0; i < m_model->m_meshes.size(); ++i)
			{
				if (m_model->m_meshes[i].first->id == mesh->id)
				{
					m_materials[i] = m_model->m_meshes[i].second;
				}
			}
		}

		MaterialHandle* GetDefaultMeshMaterial(int mesh_index)
		{
			if (mesh_index < m_materials.size())
			{
				return m_model->m_meshes[mesh_index].second;
			}
		}

		std::vector<MaterialHandle*> GetDefaultMeshMaterials(Mesh* mesh)
		{
			std::vector<MaterialHandle*> materials;
			for (int i = 0; i < m_model->m_meshes.size(); ++i)
			{
				if (m_model->m_meshes[i].first->id == mesh->id)
				{
					materials.push_back(m_model->m_meshes[i].second);
				}
			}
		}

		void RestoreModelMaterials()
		{
			for (int i = 0; i < m_model->m_meshes.size(); ++i)
			{
				m_materials[i] = m_model->m_meshes[i].second;
			}
		}

		std::vector<MaterialHandle*> GetModelMaterials()
		{
			return m_materials;
		}

		void Update(uint32_t frame_idx)
		{
			//Reset AABB

			m_aabb[0] = {
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max()
			};

			m_aabb[1] = {
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max()
			};

			//Transform all coords from model to world space
			//Pick the min/max bounds

			for (DirectX::XMVECTOR& vec : m_model->m_box)
			{
				DirectX::XMVECTOR tvec = DirectX::XMVector4Transform(vec, m_transform);

				m_aabb[0] = {
					std::min(tvec.m128_f32[0], *m_aabb[0].m128_f32),
					std::min(tvec.m128_f32[1], m_aabb[0].m128_f32[1]),
					std::min(tvec.m128_f32[2], m_aabb[0].m128_f32[2]),
					1
				};

				m_aabb[1] = {
					std::max(tvec.m128_f32[0], *m_aabb[1].m128_f32),
					std::max(tvec.m128_f32[1], m_aabb[1].m128_f32[1]),
					std::max(tvec.m128_f32[2], m_aabb[1].m128_f32[2]),
					1
				};

			}

			SignalUpdate(frame_idx);
		}
	};

} /* wr */