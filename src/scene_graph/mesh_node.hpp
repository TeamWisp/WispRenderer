#pragma once

#include "node.hpp"
#include "../model_pool.hpp"
#include "../util/aabb.hpp"

namespace wr
{

	struct MeshNode : Node
	{
		explicit MeshNode(Model* model);

		void Update(uint32_t frame_idx);
		/*! Add a material */
		/*!
			You can add a material for every single sub-mesh.
		*/
		void AddMaterial(MaterialHandle handle);
		/*! Get all materials */
		std::vector<MaterialHandle>& GetMaterials();
		/*! Set the materials */
		void SetMaterials(std::vector<MaterialHandle> const & materials);
		/*! Remove materials */
		void ClearMaterials();

		Model* m_model;
		AABB m_aabb;
		std::vector<MaterialHandle> m_materials;
		bool m_visible;

	private:
		/*! Check whether their are more materials than meshes */
		/*!
			If there are more materials than meshes.
			This function will throw a warning.
		*/
		void CheckMaterialCount() const;
	};

} /* wr */