#include "DiscordAPI.hpp"

#include <cstdio>
#include <thread>
#include <chrono>

#include "http/HTTPHelper.hpp"
#include "Logger.hpp"

using namespace std::chrono_literals;

namespace DiscordAPI {
	const std::string base_url = "https://discordapp.com/api";
	const std::string channels_url = base_url + "/channels";
	const std::string gateway_url = base_url + "/gateway";

	const std::string json_mime_type = "application/json";

	void send_message(std::string channel_id, std::string message) {
		if (message == "") {
			Logger::write("[API] [send_message] Tried to send empty message", Logger::LogLevel::Warning);
			return;
		}

		if (message.length() > 4000) {
			Logger::write("[API] [send_message] Tried to send a message over 4000 characters", Logger::LogLevel::Warning);
			return;
		}
		else if (message.length() > 2000) {
			std::cout << message.length() << std::endl;

			std::string first = message.substr(0, 2000);
			std::string second = message.substr(2000);
			send_message(channel_id, first);
			std::this_thread::sleep_for(50ms);
			send_message(channel_id, second);
			return;
		}

		const std::string url = channels_url + "/" + channel_id + "/messages";
		json data = {
			{ "content", message }
		};

		std::string response;
		long response_code = 0;
		response = HTTP::post_request(url, json_mime_type, data.dump(), &response_code);

		int retries = 0;
		while (response_code != 200 && retries < 2) {
			Logger::write("[API] [send_message] Got non-200 response code, retrying", Logger::LogLevel::Warning);
			std::this_thread::sleep_for(100ms);
			// try 3 times. usually enough to prevent 502 bad gateway issues
			response = HTTP::post_request(url, json_mime_type, data.dump(), &response_code);
			retries++;
		}

		if (response_code != 200) {
			Logger::write("[API] [send_message] Giving up on sending message", Logger::LogLevel::Warning);
		}
	}

	json get_gateway() {
		std::string response;
		long response_code;
		response = HTTP::get_request(gateway_url, &response_code);

		int retries = 0;
		while (response_code != 200 && retries < 4) {
			Logger::write("[API] [get_gateway] Got non-200 response code, retrying", Logger::LogLevel::Warning);
			std::this_thread::sleep_for(100ms);
			// try 3 times. usually enough to prevent 502 bad gateway issues
			response = HTTP::get_request(gateway_url, &response_code);
			retries++;
		}

		if (response_code != 200) {
			Logger::write("[API] [get_gateway] Giving up on getting gateway url", Logger::LogLevel::Warning);
			return json {};
		}

		return json::parse(response);
	}
}