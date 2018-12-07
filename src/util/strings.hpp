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

		return (last_occurrence_pos >= dot_position) ? true : false;
	}
}