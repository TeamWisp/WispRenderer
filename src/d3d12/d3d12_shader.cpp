#include "d3d12_functions.hpp"

#include <D3Dcompiler.h>

#include "../util/log.hpp"
#include "d3d12_defines.hpp"

namespace wr::d3d12
{

	namespace internal
	{

		const char* ShaderTypeToString(ShaderType type)
		{
			switch (type)
			{
			case ShaderType::VERTEX_SHADER:
				return "vs_5_1";
				break;
			case ShaderType::PIXEL_SHADER:
				return "ps_5_1";
				break;
			case ShaderType::DOMAIN_SHADER:
				return "ds_5_1";
				break;
			case ShaderType::GEOMETRY_SHADER:
				return "gs_5_1";
				break;
			case ShaderType::HULL_SHADER:
				return "hs_5_1";
				break;
			case ShaderType::DIRECT_COMPUTE_SHADER:
				return "cs_5_1";
				break;
			}

			return "UNKNOWN";
		}

	} /* internal */

	Shader* LoadShader(ShaderType type, std::string const & path, std::string const & entry)
	{
		auto shader = new Shader();

		shader->m_path = path;
		shader->m_type = type;
		shader->m_entry = entry;

		ID3DBlob* error;
		std::wstring wpath(path.begin(), path.end());
		HRESULT hr = D3DCompileFromFile(wpath.c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entry.c_str(),
			internal::ShaderTypeToString(type),
#ifdef _DEBUG
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION |
#endif
			D3DCOMPILE_ALL_RESOURCES_BOUND,
			0,
			&shader->m_native,
			&error);
		if (FAILED(hr))
		{
			LOGC((char*)error->GetBufferPointer());
		}

		return shader;
	}

	bool ReloadShader(Shader* shader)
	{
		ID3DBlob* error;
		ID3DBlob* temp_shader;

		std::wstring wpath(shader->m_path.begin(), shader->m_path.end());
		HRESULT hr = D3DCompileFromFile(wpath.c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			shader->m_entry.c_str(),
			internal::ShaderTypeToString(shader->m_type),
#ifdef _DEBUG
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION |
#endif
			D3DCOMPILE_ALL_RESOURCES_BOUND,
			0,
			&temp_shader,
			&error);
		if (FAILED(hr))
		{
			LOGC((char*)error->GetBufferPointer());
			return false;
		}
		else
		{
			SAFE_RELEASE(shader->m_native);
			shader->m_native = temp_shader;
			return true;
		}
	}

	void Destroy(Shader* shader)
	{
		SAFE_RELEASE(shader->m_native);
		delete shader;
	}

} /* wr::d3d12 */