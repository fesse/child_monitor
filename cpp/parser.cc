// Simple Hello World

#include <iostream>
#include <list>
#include <stdio.h>
#include <curl/curl.h>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

void parseMacs(std::string readBuffer, std::string key1, std::string key2, std::list<std::string> *macs) {
	const std::size_t keyIndex1 = readBuffer.find(key1);
	const std::size_t keyIndex2 = readBuffer.find(key2);

	const std::string startKey = ("[\"");
	const std::string endKey = ("\",");

	std::size_t startIndex = keyIndex1;
	std::size_t endIndex;

	while (true) {
		startIndex = readBuffer.find(startKey, startIndex);
		endIndex = readBuffer.find(endKey, startIndex);
		if (startIndex < keyIndex2 && endIndex !=  std::string::npos) {
			macs->push_back(readBuffer.substr(startIndex+2, endIndex-startIndex-2));
			startIndex = endIndex;
		} else {
			break;
		}
	}
}

void populateBuffer(std::string ipaddress, std::string *readBuffer) {
	CURL *curl;
	CURLcode res;
	std::string url = ipaddress + "/update_clients.asp";

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, readBuffer);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
}

int main(){
	std::string readBuffer;
	std::string key1 ("wlList_2g: [");
	std::string key2 ("wlList_5g: [");
	std::string key3 ("wlList_5g_2: [");

	std::list<std::string> macs;

	populateBuffer("192.168.1.253", &readBuffer);
	parseMacs(readBuffer, key1, key2, &macs);
	parseMacs(readBuffer, key2, key3, &macs);

	readBuffer = "";
	populateBuffer("192.168.1.254", &readBuffer);
	parseMacs(readBuffer, key1, key2, &macs);
	parseMacs(readBuffer, key2, key3, &macs);

//	std::cout << readBuffer << std::endl;
//	std::cout << "----------------" << std::endl;

	for (std::list<std::string>::iterator it=macs.begin(); it != macs.end(); ++it) {
		std::cout << *it << '\n';
	}
	return 0;
}
