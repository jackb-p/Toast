#ifndef BOT_APIHELPER
#define BOT_APIHELPER

#include <string>

#include "json/json.hpp"

using json = nlohmann::json;

class BotConfig;

namespace DiscordAPI {
	json get_gateway(std::string ca_location);
	void send_message(std::string channel_id, std::string message, std::string token, std::string ca_location);
}

#endif