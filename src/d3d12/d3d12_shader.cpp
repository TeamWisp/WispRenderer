#include "d3d12_functions.hpp"

#include <D3Dcompiler.h>
#include <dxcapi.h>

#include "../util/log.hpp"
#include "d3d12_defines.hpp"
#include <sstream>

namespace wr::d3d12
{

	namespace internal
	{

		const char* ShaderTypeToString(ShaderType type)
		{
			switch (type)
			{
			case ShaderType::VERTEX_SHADER:
				return "vs_6_3";
				break;
			case ShaderType::PIXEL_SHADER:
				return "ps_6_3";
				break;
			case ShaderType::DOMAIN_SHADER:
				return "ds_6_3";
				break;
			case ShaderType::GEOMETRY_SHADER:
				return "gs_6_3";
				break;
			case ShaderType::HULL_SHADER:
				return "hs_6_3";
				break;
			case ShaderType::DIRECT_COMPUTE_SHADER:
				return "cs_6_3";
				break;
			case ShaderType::LIBRARY_SHADER:
				return "lib_6_3";
			}

			return "UNKNOWN";
		}

	} /* internal */

	Shader* LoadShader(ShaderType type, std::string const & path, std::string const & entry)
	{
		auto shader = new Shader();

		/*shader->m_path = path;
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
		}*/

		return shader;
	}
#define DXC_COMPILE_STANDARD_FILE_INCLUDE ((IDxcIncludeHandler*)(UINT_PTR)1)

	Shader* LoadDXCShader(ShaderType type, std::string const & path, std::string const & entry)
	{
		Shader* shader = new Shader();

		std::wstring wpath(path.begin(), path.end());
		std::wstring wentry(entry.begin(), entry.end());
		std::string temp_shader_type(internal::ShaderTypeToString(type));
		std::wstring wshader_type(temp_shader_type.begin(), temp_shader_type.end());

		static IDxcCompiler* compiler = nullptr;
		if (!compiler)
		{
			DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void **)&compiler);
		}

		IDxcLibrary* library = nullptr;
		DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void **)&library);

		IDxcBlobEncoding* source;
		std::uint32_t code_page = CP_ACP;
		library->CreateBlobFromFile(wpath.c_str(), &code_page, &source);

		IDxcIncludeHandler* include_handler;
		TRY_M(library->CreateIncludeHandler(&include_handler), "Failed to create default include handler.");

		IDxcOperationResult* result;
#ifdef _DEBUG
		LPCWSTR args[] = { L"/Zi /Od" };
#else
		LPCWSTR args[] = { L"/O3" };
#endif
		HRESULT hr = compiler->Compile(
			source,          // program text
			wpath.c_str(),   // file name, mostly for error messages
			wentry.c_str(),         // entry point function
			wshader_type.c_str(),   // target profile
			args, _countof(args),   // compilation argument and count
			nullptr, 0,       // name/value defines and their count
			include_handler,          // handler for #include directives
			&result);


		//if (FAILED(hr))
		{
			IDxcBlobEncoding* error;
			result->GetErrorBuffer(&error);
			LOG((char*)error->GetBufferPointer());
		}

		result->GetResult(&shader->m_native);

		return shader;
	}

	bool ReloadShader(Shader* shader)
	{
		/*ID3DBlob* error;
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
		}*/
		return false;
	}

	void Destroy(Shader* shader)
	{
		SAFE_RELEASE(shader->m_native);
		delete shader;
	}

} /* wr::d3d12 */
