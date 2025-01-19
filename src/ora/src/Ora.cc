// Copyright (c) 2025, The Regents of the University of California
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "ora/Ora.hh"

#include <curl/curl.h>

#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "sta/StaMain.hh"
#include "utl/Logger.h"

namespace sta {
// Tcl files encoded into strings.
extern const char* ora_tcl_inits[];
}  // namespace sta

namespace ora {

using utl::Logger;

extern "C" {
extern int Ora_Init(Tcl_Interp* interp);
}

void setLogger(Logger* logger);

Logger* logger_ = nullptr;

Ora::Ora() : sourceFlag_(true)
{
}

Ora::~Ora()
{
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
  ((std::string*) userp)->append((char*) contents, size * nmemb);
  return size * nmemb;
}

std::string getLocalDirPath()
{
  return std::string(getenv("HOME")) + "/.local/share/openroad";
};

bool checkConsent()
{
  std::string folderPath = getLocalDirPath();
  std::ifstream folderFile(folderPath);
  if (!folderFile) {
    return false;
  }

  std::string consentFilePath = folderPath + "/orassistant_consent.txt";
  std::ifstream consentFile(consentFilePath);

  if (consentFile.is_open()) {
    std::string consent;
    std::getline(consentFile, consent);
    consentFile.close();
    return true;
  }

  return false;
}

std::string sendPostRequest(const std::string& url, const std::string& jsonData)
{
  CURL* curl;
  CURLcode res;
  std::string response;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();

  if (curl) {
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      response = "Error: " + std::string(curl_easy_strerror(res));
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  } else {
    response = "Error: curl_easy_init() failed";
  }

  curl_global_cleanup();
  return response;
}

void Ora::init(Tcl_Interp* tcl_interp, odb::dbDatabase* db, utl::Logger* logger)
{
  db_ = db;
  logger_ = logger;

  Ora_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::ora_tcl_inits);

  localDirPath = getLocalDirPath();
  cloudHostUrl
      = "https://bursting-stallion-friendly.ngrok-free.app/graphs/"
        "agent-retriever";
  consentMessage
      = "The ORAssistant app collects and processes your data in accordance "
        "with the General Data Protection Regulation (GDPR). By using the "
        "cloud-hosted version of this app, you consent to the collection and "
        "processing of your queries and the subsequently generated answers. "
        "Give consent to use the application by using the `ora_init consent "
        "hostURL`  command.";

  try {
    std::ifstream localHostUrlFile(getLocalDirPath() + "/orassistant_host.txt");
    if (localHostUrlFile.is_open()) {
      std::getline(localHostUrlFile, localHostUrl);
      localHostUrlFile.close();
    }
  } catch (const std::exception& e) {
    logger_->warn(utl::ORA,
                  101,
                  "Failed to read ORAssistant local host. Exception: {}",
                  e.what());
  }
}

void Ora::checkLocalDir()
{
  std::ifstream localDir(localDirPath);

  if (!localDir) {
    logger_->info(utl::ORA, 102, "Creating ~/.local/share/openroad directory.");
    try {
      std::filesystem::create_directories(localDirPath);
    } catch (const std::exception& e) {
      logger_->warn(
          utl::ORA, 103, "Failed to create ~/.local/share/openroad directory.");
    }
  }
}

void Ora::askbot(const char* query)
{
  if (!checkConsent()) {
    logger_->warn(
        utl::ORA,
        104,
        "To use the ORAssistant, please provide consent by running `ora_init "
        "consent hostURL` command. Only the query you type in will be sent "
        "outside the application—no other user or design data will be shared. "
        "For instructions on setting up a locally hosted copy, refer to the "
        "documentation in https://github.com/The-OpenROAD-Project/ORAssistant. "
        "Note: Consent must be set to y only for the web-hosted ORAssistant. "
        "Please set it to n and provide a hostURL if you choose to use a "
        "locally hosted version.");

    return;
  }

  if (cloudConsent_) {
    hostUrl = cloudHostUrl;
  } else {
    hostUrl = localHostUrl;
  }

  logger_->info(utl::ORA, 105, "Sending POST request to {}", hostUrl);

  std::stringstream jsonDataStream;
  jsonDataStream << R"({
        "query": ")"
                 << query << R"(",
        "chat_history": [],
        "list_sources": )"
                 << (sourceFlag_ ? "true" : "false") << R"(
    })";

  std::string jsonData = jsonDataStream.str();
  std::string postResponse = sendPostRequest(hostUrl, jsonData);

  if (postResponse.empty()) {
    logger_->warn(utl::ORA, 106, "No response received from API.");
    return;
  }

  try {
    boost::json::value jsonObject = boost::json::parse(postResponse);

    if (jsonObject.as_object().contains("response")
        && jsonObject.at("response").is_string()) {
      std::string response = jsonObject.at("response").as_string().c_str();

      if (sourceFlag_ && jsonObject.as_object().contains("sources")
          && jsonObject.at("sources").is_array()) {
        std::string sources;
        for (const auto& src : jsonObject.at("sources").as_array()) {
          sources += src.as_string().c_str();
          sources += "\n";
        }
        logger_->info(utl::ORA,
                      107,
                      "ORAssistant Response: \n\n{}\nSources:\n{}",
                      response,
                      sources);
      } else {
        logger_->info(
            utl::ORA, 108, "ORAssistant Response: \n\n{}\n", response);
      }
    } else {
      logger_->warn(utl::ORA,
                    109,
                    "API response does not contain 'response' field or it is "
                    "not a string.");
      logger_->warn(utl::ORA, 110, "API response: {}", postResponse);
    }
  } catch (const boost::json::system_error& e) {
    logger_->warn(utl::ORA,
                  111,
                  "JSON Parsing Error: {}\nPlease check if you have access to "
                  "ORAssistant's API.",
                  e.what());
  }
}

void Ora::setSourceFlag(bool sourceFlag)
{
  sourceFlag_ = sourceFlag;
}

void Ora::setBotHost(const char* host)
{
  hostUrl = host;
  logger_->info(utl::ORA, 112, "Setting ORAssistant host to {}", hostUrl);

  checkLocalDir();
  std::string hostFilePath = localDirPath + "/orassistant_host.txt";
  std::ofstream hostUrlFile(hostFilePath);

  if (hostUrlFile.is_open()) {
    hostUrlFile << hostUrl;
    hostUrlFile.close();
    logger_->info(utl::ORA,
                  113,
                  "ORAssistant host saved to "
                  "~/.local/share/openroad/orassistant_host.txt");
  } else {
    logger_->warn(utl::ORA, 114, "Failed to write ORAssistant host to file.");
  }
}

void Ora::setConsent(const char* consent)
{
  checkLocalDir();
  std::string consentFilePath = localDirPath + "/orassistant_consent.txt";
  std::ofstream consentFile(consentFilePath);

  // check if consent is y or n
  if (std::string(consent) != "y" && std::string(consent) != "n") {
    logger_->error(
        utl::ORA, 115, "{} : Invalid consent value. Use 'y' or 'n'.", consent);
    return;
  }

  if (consentFile.is_open()) {
    consentFile << consent;
    consentFile.close();
    logger_->info(
        utl::ORA,
        116,
        "Consent saved to ~/.local/share/openroad/orassistant_consent.txt");
  } else {
    logger_->warn(utl::ORA, 117, "Failed to write consent to file.");
  }

  if (std::string(consent) == "y") {
    cloudConsent_ = true;
    logger_->info(utl::ORA,
                  118,
                  "{}\nConsent granted for using a cloud hosted ORAssistant. "
                  "Please run `ora_init n` to remove your consent.",
                  consentMessage);
  } else if (std::string(consent) == "n") {
    cloudConsent_ = false;
    logger_->info(
        utl::ORA, 119, "Consent removed for using a cloud hosted ORAssistant.");
  }
}

}  // namespace ora