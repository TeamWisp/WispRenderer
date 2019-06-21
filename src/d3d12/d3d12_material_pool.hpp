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
#pragma once

#include "../material_pool.hpp"

#include "d3d12_renderer.hpp"

namespace wr
{

	class D3D12MaterialPool : public MaterialPool
	{
	public:
		explicit D3D12MaterialPool(D3D12RenderSystem& render_system, size_t material_count);
		~D3D12MaterialPool() final;

		void Evict() final;
		void MakeResident() final;

	protected:
		D3D12RenderSystem& m_render_system;
	};

} /* wr */
