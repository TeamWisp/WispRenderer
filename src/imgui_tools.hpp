#pragma once

namespace wr::imgui
{

	namespace menu
	{
		void Registries();
	}

	namespace window
	{
		void ShaderRegistry();
		void PipelineRegistry();
		void RootSignatureRegistry();

		static bool open_shader_registry = true;
		static bool open_pipeline_registry = true;
		static bool open_root_signature_registry = true;
	}

} /* wr::imgui */
