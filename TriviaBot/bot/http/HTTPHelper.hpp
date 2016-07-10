#ifndef BOT_HTTP_HTTPHELPER
#define BOT_HTTP_HTTPHELPER

#include <iostream>

#include <curl/curl.h>

class HTTPHelper {
public:
	std::string post_request(std::string url, std::string content_type, std::string data);

private:
	static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
};

#endif