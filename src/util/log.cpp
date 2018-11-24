#include "log.hpp"

namespace util
{
	std::function<void(std::string const &)> log_callback::impl = nullptr;
}
