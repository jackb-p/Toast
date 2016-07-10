#ifndef BOT_APIHELPER
#define BOT_APIHELPER

#include <string>

#include "json/json.hpp"

using json = nlohmann::json;

class HTTPHelper;

class APIHelper {
public:
	APIHelper();

	void send_message(std::string channel_id, std::string message);

private:
	const std::string BASE_URL = "https://discordapp.com/api";
	const std::string CHANNELS_URL = BASE_URL + "/channels";
	const std::string TOKEN = "MTk5NjU3MDk1MjU4MTc3NTM5.ClyBNQ.15qTa-XBKRtGNMMYeXCrU50GhWE";
	const std::string TOKEN_PARAM = "token=" + TOKEN;
	const std::string JSON_CTYPE = "application/json";

	HTTPHelper *http;
};

#endif