// Simple MAC finder

#include <curl/curl.h>
#include <iostream>
#include <list>
#include <stdio.h>

static size_t writeCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

/**
 * Session cookie extractor
 */
static size_t headerCallback(void *contents, size_t size, size_t nmemb,
                             void *userp) {
  const std::string key = ("Set-Cookie: ");
  size_t numbytes = size * nmemb;

  std::string headerBuffer;
  headerBuffer.assign((char *)contents, numbytes);

  size_t keyIndex = headerBuffer.find(key);
  if (keyIndex != -1) {
    headerBuffer.assign(headerBuffer, keyIndex + key.size(),
                        headerBuffer.size());
    keyIndex = headerBuffer.find(";");
    if (keyIndex != -1) {
      headerBuffer.assign(headerBuffer, 0, keyIndex);
    }
    ((std::string *)userp)->append(headerBuffer.c_str(), headerBuffer.size());
  }
  return numbytes;
}

void parseMacs(std::string readBuffer, std::string key1, std::string key2,
               std::list<std::string> *macs) {
  const std::size_t keyIndex1 = readBuffer.find(key1);
  const std::size_t keyIndex2 = readBuffer.find(key2);

  const std::string startKey = ("[\"");
  const std::string endKey = ("\",");

  std::size_t startIndex = keyIndex1;
  std::size_t endIndex;

  while (true) {
    startIndex = readBuffer.find(startKey, startIndex);
    endIndex = readBuffer.find(endKey, startIndex);
    if (startIndex < keyIndex2 && endIndex != std::string::npos) {
      macs->push_back(
          readBuffer.substr(startIndex + 2, endIndex - startIndex - 2));
      startIndex = endIndex;
    } else {
      break;
    }
  }
}

void populateBuffer(std::string ipaddress, std::string *readBuffer) {
  CURL *curl;
  CURLcode res;

  std::string headerBuffer;
  std::string originHeader = ("Origin: http://");
  originHeader.append(ipaddress);
  std::string refererHeader = ("Referer: http://");
  refererHeader.append(ipaddress).append("/Main_Login.asp");

  //	std::cout << originHeader.c_str();
  struct curl_slist *headerList = NULL;
  headerList = curl_slist_append(headerList, originHeader.c_str());
  headerList = curl_slist_append(headerList, refererHeader.c_str());

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, (ipaddress + "/login.cgi").c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, readBuffer);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerBuffer);

  // Login
  const char *data = "login_authorization=YWRtaW46cGFzc3dvcmQK";
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

  res = curl_easy_perform(curl);
  //	std::cout << "Session Cookie: " << headerBuffer.c_str() << '\n';
  // get data
  //	std::cout << readBuffer->c_str();
  readBuffer->clear();
  curl_easy_setopt(curl, CURLOPT_COOKIE, headerBuffer.c_str());
  curl_easy_setopt(curl, CURLOPT_URL,
                   (ipaddress + "/update_clients.asp").c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 0L);
  res = curl_easy_perform(curl);

  //	std::cout << readBuffer->c_str();

  curl_slist_free_all(headerList);
  curl_easy_cleanup(curl);
}

void addMacs(std::string ipaddress, std::list<std::string> *macs) {
  std::string key1("wlList_2g: [");
  std::string key2("wlList_5g: [");
  std::string key3("wlList_5g_2: [");

  std::string readBuffer;
  populateBuffer(ipaddress, &readBuffer);
  //	std::cout <<
  //"****************************************************************************\n";
  //	std::cout <<
  //"****************************************************************************\n";
  //	std::cout <<
  //"****************************************************************************\n";
  //	std::cout <<
  //"****************************************************************************\n";
  //	std::cout <<
  //"****************************************************************************\n";
  //	std::cout << readBuffer.c_str();
  parseMacs(readBuffer, key1, key2, macs);
  parseMacs(readBuffer, key2, key3, macs);
}

bool containsMac(std::string mac, std::list<std::string> *macs) {
  for (std::list<std::string>::iterator it = macs->begin(); it != macs->end();
       ++it) {
    if (it->compare(mac) == 0) {
      return true;
    }
  }
  return false;
}

int main() {

  std::list<std::string> macs;
  addMacs("192.168.1.253", &macs);

  for (std::list<std::string>::iterator it = macs.begin(); it != macs.end(); ++it) {
    std::cout << *it << '\n';
  }

  std::cout << "-----\n";

  addMacs("192.168.1.254", &macs);

  for (std::list<std::string>::iterator it = macs.begin(); it != macs.end(); ++it) {
    std::cout << *it << '\n';
  }
  std::cout << "-----\n";
  
  bool kevin = containsMac("04:4B:ED:7D:7F:72", &macs);
  bool elin = containsMac("24:F0:94:71:1B:03", &macs);

  std::cout << "Kevin " << kevin << '\n';
  std::cout << "Elin " << elin << '\n';
}
