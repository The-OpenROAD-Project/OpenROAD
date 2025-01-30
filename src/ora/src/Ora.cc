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
        "If you wish to remove your consent, please run `ora_init n`.";
  consentVersion = "1.0";

  try {
    std::ifstream consentFile(getLocalDirPath() + "/orassistant_consent.txt");
    if (consentFile.is_open()) {
      std::string line;
      while (std::getline(consentFile, line)) {
        if (line.find("consent: ") == 0) {
          givenConsent = line.substr(9);
        } else if (line.find("consent_version: ") == 0) {
          givenConsentVersion = line.substr(17);
        }
      }
      consentFile.close();
    }
  } catch (const std::exception& e) {
    logger_->warn(utl::ORA,
                  121,
                  "Failed to read ORAssistant consent file. Exception: {}",
                  e.what());
  }

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

  try {
    std::ifstream modeFile(getLocalDirPath() + "/orassistant_mode.txt");
    if (modeFile.is_open()) {
      std::getline(modeFile, mode_);
      modeFile.close();
    }
  } catch (const std::exception& e) {
    logger_->warn(utl::ORA,
                  125,
                  "Failed to read ORAssistant mode. Exception: {}",
                  e.what());
  }
}

void Ora::checkLocalDir()
{
  std::ifstream localDir(localDirPath);

  if (!localDir) {
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
  if (mode_.empty()) {
    logger_->warn(
        utl::ORA,
        104,
        "To use ORAssistant, please configure the tool by running "
        "`ora_init "
        "mode consent hostURL` command. Only the query you type in will be "
        "sent "
        "outside the application—no other user or design data will be shared. "
        "For instructions on setting up a locally hosted copy, refer to the "
        "documentation in https://github.com/The-OpenROAD-Project/ORAssistant. "
        "Note: Consent must be set to y only for the web-hosted ORAssistant. "
        "Please set it to n and provide a hostURL if you choose to use a "
        "locally hosted version.");
    return;
  }
  if (givenConsent == "y") {
    hostUrl = cloudHostUrl;
    if (givenConsentVersion != consentVersion) {
      logger_->warn(
          utl::ORA,
          120,
          "The consent messaage has been updated since your last use. Please "
          "run `ora_init y` to provide consent again.");
      return;
    }
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
    consentFile << "consent: " << consent << "\n";
    consentFile << "consent_version: " << consentVersion << "\n";
    consentFile.close();
    logger_->info(
        utl::ORA,
        116,
        "Consent saved to ~/.local/share/openroad/orassistant_consent.txt");
  } else {
    logger_->warn(utl::ORA, 117, "Failed to write consent to file.");
  }

  if (std::string(consent) == "y") {
    logger_->info(utl::ORA, 118, consentMessage);
  } else if (std::string(consent) == "n") {
    logger_->info(
        utl::ORA, 119, "Consent removed for using a cloud hosted ORAssistant.");
  }

  givenConsent = std::string(consent);
  givenConsentVersion = consentVersion;
}

void Ora::setMode(const char* mode)
{
  checkLocalDir();
  std::string modeFilePath = localDirPath + "/orassistant_mode.txt";
  std::ofstream modeFile(modeFilePath);

  if (modeFile.is_open()) {
    modeFile << mode;
    modeFile.close();
    logger_->info(utl::ORA,
                  123,
                  "Mode saved to ~/.local/share/openroad/orassistant_mode.txt");
  } else {
    logger_->warn(utl::ORA, 124, "Failed to write mode to file.");
  }
}

}  // namespace ora