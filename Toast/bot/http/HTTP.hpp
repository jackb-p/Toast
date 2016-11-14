#ifndef BOT_HTTP_HTTP
#define BOT_HTTP_HTTP

#include <iostream>

#include <curl/curl.h>

class BotConfig;

namespace HTTP {
	std::string post_request(std::string url, std::string content_type, std::string data, long *response_code, std::string token, std::string ca_location);
	std::string get_request(std::string url, long *response_code, std::string token, std::string ca_location);
}

#endif