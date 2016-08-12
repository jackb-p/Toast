#include "Logger.hpp"

#include <iostream>
#include <ctime>

namespace Logger {
	std::ostream &operator<<(std::ostream &out, const LogLevel log_level) {
		switch (log_level) {
		case LogLevel::Debug:
			return out << "debug";
		case LogLevel::Info:
			return out << "info";
		case LogLevel::Warning:
			return out << "warning";
		case LogLevel::Severe:
			return out << "severe";
		}
		return out << "";
	}

	std::ostream &get_ostream(LogLevel log_level) {
		switch (log_level) {
		case LogLevel::Debug:
		case LogLevel::Info:
			return std::clog;
		case LogLevel::Severe:
		case LogLevel::Warning:
			return std::cerr;
		}

		return std::cerr;
	}

	void write(std::string text, LogLevel log_level) {
		time_t rawtime;
		struct tm *timeinfo;
	  char buffer[80];

	  time(&rawtime);
	  timeinfo = localtime(&rawtime);

	  strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
	  std::string time_str(buffer);

		get_ostream(log_level) << "[" << time_str << "] [" << log_level << "] " << text << std::endl;
	}
}
