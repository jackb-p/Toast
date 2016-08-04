#include "APIHelper.hpp"

#include <cstdio>
#include <thread>
#include <chrono>

#include "http/HTTPHelper.hpp"
#include "Logger.hpp"

using namespace std::chrono_literals;

APIHelper::APIHelper() : BASE_URL("https://discordapp.com/api"), CHANNELS_URL(BASE_URL + "/channels"),
	JSON_CTYPE("application/json") {
	http = new HTTPHelper();
}

void APIHelper::send_message(std::string channel_id, std::string message) {
	const std::string url = CHANNELS_URL + "/" + channel_id + "/messages";
	json data = {
		{ "content", message }
	};

	std::string response;
	long response_code = -1;
	response = http->post_request(url, JSON_CTYPE, data.dump(), &response_code);

	int retries = 0;
	while (response_code != 200 && retries < 2) {
		Logger::write("[send_message] Got non-200 response code, retrying", Logger::LogLevel::Warning);
		std::this_thread::sleep_for(100ms);
		// try 3 times. usually enough to prevent 502 bad gateway issues
		response = http->post_request(url, JSON_CTYPE, data.dump(), &response_code);
		retries++;
	}

	if (response_code != 200) {
		Logger::write("[send_message] Giving up on sending message", Logger::LogLevel::Warning);
	}
}