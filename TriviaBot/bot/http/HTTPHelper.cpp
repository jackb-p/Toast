#include "HTTPHelper.hpp"

/*
*  Warning: (Awful) C Code
*/

std::string HTTPHelper::post_request(std::string url, std::string content_type, std::string data) {
	CURL *curl;
	CURLcode res;
	std::string read_buffer;
	struct curl_slist *headers = nullptr;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		// Now with real HTTPS!
		curl_easy_setopt(curl, CURLOPT_CAINFO, "bot/http/DiscordCA.crt");

		std::string content_header = "Content-Type: " + content_type;
		headers = curl_slist_append(headers, content_header.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			return "ERROR";
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

	return read_buffer;
}

size_t HTTPHelper::write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
	((std::string *) userp)->append((char *)contents, size * nmemb);
	return size * nmemb;
}