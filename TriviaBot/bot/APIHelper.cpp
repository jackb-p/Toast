#include "http/HTTPHelper.hpp"

#include <cstdio>

#include "APIHelper.hpp"

extern std::string bot_token;

APIHelper::APIHelper() : BASE_URL("https://discordapp.com/api"), CHANNELS_URL(BASE_URL + "/channels"),
	TOKEN_PARAM("token=" + bot_token), JSON_CTYPE("application/json") {
	http = new HTTPHelper();
}

void APIHelper::send_message(std::string channel_id, std::string message) {
	const std::string url = CHANNELS_URL + "/" + channel_id + "/messages?" + TOKEN_PARAM;
	json data = {
		{ "content", message }
	};

	const std::string response = http->post_request(url, JSON_CTYPE, data.dump());
	// TODO: verify success
}