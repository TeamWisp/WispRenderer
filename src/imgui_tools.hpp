#pragma once

namespace wr
{
	class D3D12RenderSystem;
}

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
		void D3D12HardwareInfo(D3D12RenderSystem& render_system);
		void D3D12Settings();

		static bool open_hardware_info = true;
		static bool open_d3d12_settings = true;
		static bool open_shader_registry = true;
		static bool open_pipeline_registry = true;
		static bool open_root_signature_registry = true;
	}

} /* wr::imgui */
