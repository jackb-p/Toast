#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <iostream>

// callback function writes data to a std::ostream
class HTTPHelper {
public:
	static std::string read_url(std::string url);

private:
	static size_t data_write(void* buf, size_t size, size_t nmemb, void* userp);
	static CURLcode curl_read(const std::string& url, std::ostream& os, long timeout);
};