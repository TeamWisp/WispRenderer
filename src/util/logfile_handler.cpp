#include "logfile_handler.hpp"

#include <sstream>

wr::LogfileHandler::LogfileHandler()
{
	std::filesystem::path path("./logs/");
	if (!std::filesystem::exists(path))
	{
		std::filesystem::create_directory(path);
	}
	std::stringstream ss;
	ss << "log-default";
	path /= ss.str();
	std::filesystem::create_directory(path);
	path /= "default-wisp.log";
	m_file = fopen(path.string().c_str(), "w");
}

wr::LogfileHandler::LogfileHandler(std::filesystem::path& dir_path, std::string& file_name)
{
	std::filesystem::path path("./logs/");
	if (!std::filesystem::exists(path))
	{
		std::filesystem::create_directory(path);
	}
	path /= dir_path;
	std::filesystem::create_directory(path);
	path /= file_name;
	m_file = fopen(path.string().c_str(), "w");
}

std::FILE* wr::LogfileHandler::GetFilePtr()
{
	return m_file;
}

wr::LogfileHandler::~LogfileHandler()
{
	fflush(m_file);
	fclose(m_file);
}

const std::filesystem::path& wr::LogfileHandler::GetDirPath()
{
	return dir_path;
}
