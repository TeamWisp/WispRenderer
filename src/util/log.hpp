#pragma once

#define LOG_PRINT_TIME
//#define LOG_PRINT_THREAD
#define LOG_PRINT_LOC
#define LOG_PRINT_COLORS

#if defined(LOG_PRINT_COLORS) && defined(_WIN32)
#include <Windows.h>
#endif
#ifdef LOG_PRINT_THREAD
#include <thread>
#include <sstream>
#endif

#include "fmt/format.h"
#include "fmt/time.h"

#define LOG_BREAK DebugBreak();

namespace util::internal
{
	enum class MSGB_ICON
	{
		CRITICAL_ERROR = MB_OK | MB_ICONERROR
	};

	template <typename S, typename... Args>
	inline void log_impl(int color, char type, std::string file, std::string func, int line, S const & format, Args const &... args)
	{
		std::string str = "";

#ifdef LOG_PRINT_TIME
		std::tm s;
		std::time_t t = std::time(nullptr);
		localtime_s(&s, &t);

		str += fmt::format("[{:%H:%M}]", s) + " [" + type + "] ";
#endif
#ifdef LOG_PRINT_THREAD
		auto thread_id = std::this_thread::get_id();
		std::stringstream ss;
		ss << thread_id;
		std::string thread_id_str = ss.str();

		if (thread_id_str != "1")
		{
			str += "[thread:" + thread_id_str + "] ";
		}
#endif
#ifdef LOG_PRINT_LOC
		str += "[" + file + ":" + func + ":" + std::to_string(line) + "] ";
#endif
		str += format;
		str += "\n";

#if defined(LOG_PRINT_COLORS) && defined(_WIN32)
		if (color != 0)
		{
			auto console = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(console, color);
		}
#endif

		fmt::print(stdout, str, args...);

#if defined(LOG_PRINT_COLORS) && defined(_WIN32)
		if (color != 0)
		{
			auto console = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(console, 7);
		}
#endif
	}

	template <typename S, typename... Args>
	inline void log_msgb_impl(MSGB_ICON icon, std::string file, std::string func, int line, S const & format, Args const &... args)
	{
		std::string str = "";

#ifdef LOG_PRINT_TIME
		std::tm s;
		std::time_t t = std::time(nullptr);
		localtime_s(&s, &t);

		str += fmt::format("[{:%H:%M}]\n", s);
#endif
#ifdef LOG_PRINT_THREAD
		auto thread_id = std::this_thread::get_id();
		std::stringstream ss;
		ss << thread_id;
		std::string thread_id_str = ss.str();

		if (thread_id_str != "1")
		{
			str += "[thread:" + thread_id_str + "]\n";
		}
#endif
#ifdef LOG_PRINT_LOC
		str += "[" + file + ":" + func + ":" + std::to_string(line) + "] ";
#endif
		str += format;
		str += "\n";

		switch(icon)
		{
		case MSGB_ICON::CRITICAL_ERROR:
			MessageBox(0, str.c_str(), "Critical Error", (int)icon);
			break;
		default:
			MessageBox(0, str.c_str(), "Unkown Error", MB_OK | MB_ICONQUESTION);
			break;
		}
	}

} /* internal */

#define LOG(csr, ...)  { util::internal::log_impl(7, 'I', __FILE__, __func__, __LINE__, csr, ##__VA_ARGS__); }
#define LOGW(csr, ...) { util::internal::log_impl(6, 'W', __FILE__, __func__, __LINE__, csr, ##__VA_ARGS__); }
#define LOGE(csr, ...) { util::internal::log_impl(4, 'E', __FILE__, __func__, __LINE__, csr, ##__VA_ARGS__); LOG_BREAK }
#define LOGC(csr, ...) { util::internal::log_impl(71, 'C', __FILE__, __func__, __LINE__, csr, ##__VA_ARGS__); LOG_BREAK }
//#define LOGC(csr, ...) { util::internal::log_msgb_impl(util::internal::MSGB_ICON::CRITICAL_ERROR, __FILE__, __func__, __LINE__, csr, ##__VA_ARGS__); }