#pragma once

namespace wr::settings
{

	static const constexpr bool use_multithreading = true;
	static const constexpr unsigned int num_frame_graph_threads = 8;

	static const constexpr std::uint8_t default_textures_count = 5;
	static const constexpr std::uint8_t default_textures_size_in_bytes = 4*1024*1024;
	static constexpr const char* default_albedo_path = "resources/materials/metalgrid2_basecolor.png";
	static constexpr const char* default_normal_path = "resources/materials/flat_normal.png";
	static constexpr const char* default_roughness_path = "resources/materials/white.png";
	static constexpr const char* default_metalic_path = "resources/materials/black.png";
	static constexpr const char* default_ao_path = "resources/materials/black.png";

} /* wr::settings */
