#pragma once
#include <filesystem>

namespace wr
{
	class LogfileHandler {
	public:
		LogfileHandler();
		LogfileHandler(std::filesystem::path& dir_path, std::string& file_name);
		~LogfileHandler();

		std::FILE* GetFilePtr();
		const std::filesystem::path& GetDirPath();

	private:
		std::FILE* m_file = nullptr;
		std::filesystem::path dir_path;
	};
}
