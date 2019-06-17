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

#include "file_watcher.hpp"

#include <thread>
#include "log.hpp"

namespace util
{

	FileWatcher::FileWatcher(std::string const & path,
		const std::chrono::milliseconds delay,
		bool regular_files_only)
		: m_watch_path(path),
		m_delay(delay),
		m_regular_files_only(regular_files_only),
		m_running(false)
	{
		// Record all files
		for (auto &file : std::filesystem::recursive_directory_iterator(path)) {
			m_paths[file.path().u8string()] = std::filesystem::last_write_time(file);
		}
	}

	FileWatcher::~FileWatcher()
	{
		if (m_thread.joinable())
		{
			m_thread.join();
		}
	}

	void FileWatcher::Start(util::Delegate<void(std::string const &, FileWatcher::FileStatus)> const & callback)
	{
		if (m_running)
		{
			LOGW("Tried to start the file watcher while it is already running");
			return;
		}
		else
		{
			m_running = true;
		}

		// Lambda that checks whether the callback should be called or not based on settings.
		auto run_callback = [this, callback](std::string path, FileStatus status) {
			bool is_special = (!std::filesystem::is_regular_file(std::filesystem::path(path)) && status != FileStatus::ERASED);
			if (m_regular_files_only && is_special)
			{
				return;
			}

			callback(path, status);
		};

		while (m_running)
		{
			std::this_thread::sleep_for(m_delay);

			// Check for file ereasure.
			for (auto& it : m_paths)
			{
				if (!std::filesystem::exists(it.first))
				{
					run_callback(it.first, FileStatus::ERASED);
					m_paths.erase(it.first);
				}
			}

			// Check if file was created or modified.
			for (auto& file : std::filesystem::recursive_directory_iterator(m_watch_path))
			{
				auto last_write_time = std::filesystem::last_write_time(file);
				auto file_path = file.path().u8string();

				// File Created
				if (!m_paths.contains(file_path))
				{
					m_paths[file_path] = last_write_time;
					run_callback(file_path, FileStatus::CREATED);
				}
				// File Modified
				else
				{
					if (m_paths[file_path] != last_write_time)
					{
						m_paths[file_path] = last_write_time;
						run_callback(file_path, FileStatus::MODIFIED);
					}
				}
			}
		}
	}

	void FileWatcher::StartAsync(util::Delegate<void(std::string const & path, FileStatus)> const & callback)
	{
		if (m_running)
		{
			LOGW("Tried to start the file watcher while it is already running");
			return;
		}

		m_thread = std::thread([this, callback]() {
			Start(callback);
		});
	}

} /* util */
