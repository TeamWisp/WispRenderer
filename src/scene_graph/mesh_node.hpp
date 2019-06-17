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