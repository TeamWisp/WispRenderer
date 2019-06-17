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
#include <string_view>

namespace util
{
	inline std::optional<std::string_view> GetFileExtension(std::string_view path)
	{
		std::size_t pos = path.find_last_of(".");

		if (pos != std::string_view::npos)
		{
			std::size_t length = path.length();
			std::string_view extension = path.substr(pos, length);

			return extension;
		}
		else
		{
			return {};
		}
	}

	inline bool MatchFileExtension(std::string_view path, std::string_view extension)
	{
		std::size_t dot_position = path.find_last_of(".");

		if (dot_position == std::string_view::npos)
			return false;

		std::size_t last_occurrence_pos = path.rfind(extension);
		
		if (last_occurrence_pos == std::string_view::npos)
			return false;

		return last_occurrence_pos >= dot_position;
	}
}