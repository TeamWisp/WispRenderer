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

	std::variant<Shader*, std::string> LoadShader(ShaderType type, std::string const & path, std::string const & entry)
	{
		auto shader = new Shader();

		shader->m_entry = entry;
		shader->m_path = path;
		shader->m_type = type;

		std::wstring wpath(path.begin(), path.end());
		std::wstring wentry(entry.begin(), entry.end());
		std::string temp_shader_type(internal::ShaderTypeToString(type));
		std::wstring wshader_type(temp_shader_type.begin(), temp_shader_type.end());

		IDxcLibrary* library = nullptr;
		DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void **)&library);

		IDxcBlobEncoding* source;
		std::uint32_t code_page = CP_ACP;
		library->CreateBlobFromFile(wpath.c_str(), &code_page, &source);

		IDxcIncludeHandler* include_handler;
		TRY_M(library->CreateIncludeHandler(&include_handler), "Failed to create default include handler.");

		IDxcOperationResult* result;
		HRESULT hr = Device::m_compiler->Compile(
			source,          // program text
			wpath.c_str(),   // file name, mostly for error messages
			wentry.c_str(),         // entry point function
			wshader_type.c_str(),   // target profile
#ifdef _DEBUG
			d3d12::settings::debug_shader_args.data(), d3d12::settings::debug_shader_args.size(),
#else
			d3d12::settings::release_shader_args.data(), d3d12::settings::release_shader_args.size(),
#endif
			nullptr, 0,       // name/value defines and their count
			include_handler,          // handler for #include directives
			&result);

		if (FAILED(hr))
		{
			LOGC("Failed to compile {} (entry: {}), Incorrect entry or path?", path, entry);
		}

		// compiler output
		result->GetStatus(&hr);
		if (FAILED(hr))
		{
			delete shader;

			IDxcBlobEncoding* error;
			result->GetErrorBuffer(&error);
			return std::string((char*)error->GetBufferPointer());
		}

		result->GetResult(&shader->m_native);

		return shader;
	}

	void Destroy(Shader* shader)
	{
		SAFE_RELEASE(shader->m_native);
		delete shader;
	}

} /* wr::d3d12 */
