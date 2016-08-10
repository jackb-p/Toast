#ifndef BOT_HTTP_HTTPHELPER
#define BOT_HTTP_HTTPHELPER

#include <iostream>

#include <curl/curl.h>

namespace HTTP {
	std::string post_request(std::string url, std::string content_type, std::string data, long *response_code);
	std::string get_request(std::string url, long *response_code);
}

#endif