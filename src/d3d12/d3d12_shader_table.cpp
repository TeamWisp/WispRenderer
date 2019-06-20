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
#include "d3d12_functions.hpp"

#include <variant>

#include "d3d12_defines.hpp"

namespace wr::d3d12
{
	
	ShaderRecord CreateShaderRecord(void* identifier, std::uint64_t identifier_size, void* local_root_args, std::uint64_t local_root_args_size)
	{
		ShaderRecord record;

		record.m_shader_identifier = { identifier, identifier_size };
		record.m_local_root_args = { local_root_args, local_root_args_size };

		return record;
	}

	// TODO: This might be problematic
	void CopyShaderRecord(ShaderRecord src, void* dest)
	{
		uint8_t* byte_dest = static_cast<uint8_t*>(dest);
		memcpy(byte_dest, src.m_shader_identifier.first, src.m_shader_identifier.second);
		if (src.m_local_root_args.first)
		{
			// byte_dest = static_cast<uint8_t*>(dest.m_local_root_args.first);
			memcpy(byte_dest + src.m_shader_identifier.second, src.m_local_root_args.first, src.m_local_root_args.second);
		}
	}

	ShaderTable* CreateShaderTable(Device* device,
			std::uint64_t num_shader_records,
			std::uint64_t shader_record_size)
	{
		auto table = new ShaderTable();

		table->m_shader_record_size = SizeAlignTwoPower(shader_record_size, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		table->m_shader_records.reserve(num_shader_records);
		std::uint64_t buffer_size = num_shader_records * table->m_shader_record_size;
		
		auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);
		TRY(device->m_native->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&table->m_resource)));
		table->m_resource->SetName(L"Shader table resource");


		uint8_t* mappedData;
		// We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		TRY_M(table->m_resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)), "Failed to map shader table resource");

		table->m_mapped_shader_records = mappedData;

		return table;
	}

	void AddShaderRecord(ShaderTable* table, ShaderRecord record)
	{		
		assert(table->m_shader_records.size() < table->m_shader_records.capacity());
		table->m_shader_records.push_back(record);
		CopyShaderRecord(record, table->m_mapped_shader_records);
		table->m_mapped_shader_records += table->m_shader_record_size;
	}

	void Destroy(ShaderTable* table)
	{
		SAFE_RELEASE(table->m_resource);
		delete table;
	}

} /* wr::d3d12 */
