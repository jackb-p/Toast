#ifndef BOT_APIHELPER
#define BOT_APIHELPER

#include <string>

#include "json/json.hpp"

using json = nlohmann::json;

class HTTPHelper;

namespace DiscordAPI {
	json get_gateway();
	void send_message(std::string channel_id, std::string message);
}

#endif