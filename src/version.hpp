/*!
A constexpr way to obtain program version information.
Copyright © 2018, Viktor Zoutman

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#define VERSION_FILE "../wisp.version"

#include <cctype>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

namespace wr
{

	struct Version
	{
		std::uint32_t m_major = 0, m_minor = 0, m_patch = 0;
	};

	namespace std_cexpr
	{
		constexpr size_t strlen(const char* str)
		{
			return (*str == 0) ? 0 : strlen(str + 1) + 1;
		}

		constexpr bool isdigit(char c)
		{
			return c <= '9' && c >= '0';
		}

		constexpr std::uint32_t stoi_impl(const char* str, std::uint32_t value = 0)
		{
			return *str ?
				std_cexpr::isdigit(*str) ?
				stoi_impl(str + 1, (*str - '0') + value * 10)
				: throw std::logic_error("compile-time-error: Not a digit")
				: value;
		}

		constexpr std::uint32_t stoi(const char* str)
		{
			return stoi_impl(str);
		}
	}

	constexpr Version GetVersion() {
		const char* str = static_cast<const char*>(
			#include VERSION_FILE
		);

		char major_buf[128] = "";
		char minor_buf[128] = "";
		char patch_buf[128] = "";

		std::uint32_t num = 0;
		std::uint32_t itt = 0;
		for (std::uint32_t i = 0; i < std_cexpr::strlen(str); i++) {
			if (str[i] == '.') {
				num++;
				itt = 0;
				continue;
			}
			else if (num == 0) {
				major_buf[itt] = str[i];
			}
			else if (num == 1) {
				minor_buf[itt] = str[i];
			}
			else if (num == 2) {
				patch_buf[itt] = str[i];
			}

			itt++;
		}

		if (num != 2) {
			throw std::logic_error("compile-time-error: Incorrect length");
		}

		return { std_cexpr::stoi(major_buf), std_cexpr::stoi(minor_buf), std_cexpr::stoi(patch_buf) };
	}

} /* wr */
