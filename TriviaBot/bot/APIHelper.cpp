#include "http/HTTPHelper.hpp"

#include <cstdio>

#include "APIHelper.hpp"

APIHelper::APIHelper() {
	http = new HTTPHelper();
}

void APIHelper::send_message(std::string channel_id, std::string message) {
	const std::string url = CHANNELS_URL + "/" + channel_id + "/messages?" + TOKEN_PARAM;
	json data = {
		{ "content", message }
	};

	const std::string response = http->post_request(url, JSON_CTYPE, data.dump());
	std::cout << response << std::endl;
}