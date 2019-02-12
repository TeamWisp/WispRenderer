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
		void Start(util::Delegate<void(std::string path, FileStatus)> const & callback);
		//! Launches the file watcher asynchrounously. When the file watcher goes out of scope the thread gets killed.
		void StartAsync(util::Delegate<void(std::string path, FileStatus)> const & callback);

	private:
		//! C++20's path::contains function.
		bool PathsContains(std::string const & key);

		bool m_running;
		bool m_regular_files_only;
		const std::chrono::milliseconds m_delay;
		std::string m_watch_path;
		std::unordered_map<std::string, std::filesystem::file_time_type> m_paths;
		std::thread m_thread;
	};

} /* util */