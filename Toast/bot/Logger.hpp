#ifndef BOT_LOGGER
#define BOT_LOGGER

#include <string>

namespace Logger {
	enum class LogLevel {
		Debug, Info, Warning, Severe
	};

	void write(std::string text, LogLevel log_level);
}

#endif
