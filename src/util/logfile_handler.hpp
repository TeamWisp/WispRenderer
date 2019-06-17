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
