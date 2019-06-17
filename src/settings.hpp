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

namespace wr::settings
{

	static const constexpr bool use_multithreading = true;
	static const constexpr unsigned int num_frame_graph_threads = 4;

	static const constexpr std::uint8_t default_textures_count = 5;
	static const constexpr std::uint32_t default_textures_size_in_bytes = 4ul * 1024ul * 1024ul;
	static constexpr const char* default_albedo_path = "resources/materials/metalgrid2_basecolor.png";
	static constexpr const char* default_normal_path = "resources/materials/flat_normal.png";
	static constexpr const char* default_white_texture = "resources/materials/white.png";
	static constexpr const char* default_black_texture = "resources/materials/black.png";

} /* wr::settings */
