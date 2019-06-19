/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
#include <functional>
#include <chrono>
#include <thread>

#include "delegate.hpp"

namespace util
{

	//! FileWatcher
	/*!
		This is a simple file watcher that checks every X milliseconds whether a file has been created, modified or erased
		and gives you a callback if a change is detected.
		This file watcher can run both asyncronously and syncrounously.
	*/
	class FileWatcher
	{
	public:
		enum class FileStatus
		{
			MODIFIED,
			CREATED,
			ERASED
		};

		//! FileWatcher constructor
		/*!
			\param path The path to watch for changes.
			\param delay The interval used to check changes.
			\param regular_files_only Setting this to true will ignore all non-regular files.
		*/
		FileWatcher(std::string const & path, const std::chrono::milliseconds delay, bool regular_files_only = false);
		~FileWatcher();

		//! Launches the file watcher. This will stall the current thread.
		void Start(util::Delegate<void(std::string const &, FileStatus)> const & callback);
		//! Launches the file watcher asynchrounously. When the file watcher goes out of scope the thread gets killed.
		void StartAsync(util::Delegate<void(std::string const &, FileStatus)> const & callback);

	private:
		bool m_running;
		bool m_regular_files_only;
		const std::chrono::milliseconds m_delay;
		std::string m_watch_path;
		std::unordered_map<std::string, std::filesystem::file_time_type> m_paths;
		std::thread m_thread;
	};

} /* util */
