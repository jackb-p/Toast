#include "HTTPHelper.hpp"

#include "../Logger.hpp"

extern std::string bot_token;

/*
*  Warning: (Awful) C Code
*/

std::string HTTPHelper::post_request(std::string url, std::string content_type, std::string data, long *response_code) {
	CURL *curl;
	CURLcode res;
	std::string read_buffer;
	struct curl_slist *headers = nullptr;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		// Now with real HTTPS!
		curl_easy_setopt(curl, CURLOPT_CAINFO, "bot/http/DiscordCA.crt");

		std::string header_arr[3];
		header_arr[0] = "Content-Type: " + content_type;
		header_arr[1] = "Authorization: Bot " + bot_token;
		header_arr[2] = "User-Agent: DiscordBot(http://github.com/jackb-p/triviadiscord, 1.0)";

		for (std::string h : header_arr) {
			headers = curl_slist_append(headers, h.c_str());
		}

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

		res = curl_easy_perform(curl);

		if (res == CURLE_OK) {
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, response_code);
		} else {
			Logger::write("curl error", Logger::LogLevel::Warning);
			return "";
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

	return read_buffer;
}

size_t HTTPHelper::write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
	((std::string *) userp)->append((char *) contents, size * nmemb);
	return size * nmemb;
}