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
	const std::string BASE_URL;
	const std::string CHANNELS_URL;
	const std::string JSON_CTYPE;

	HTTPHelper *http;
};

#endif