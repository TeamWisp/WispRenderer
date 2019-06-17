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
