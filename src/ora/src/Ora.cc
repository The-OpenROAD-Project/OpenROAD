// Copyright (c) 2024, The Regents of the University of California
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
#include "sta/StaMain.hh"
#include "utl/Logger.h"
#include <string>
#include <sstream>
#include <curl/curl.h>
#include <boost/json.hpp>

namespace sta {
// Tcl files encoded into strings.
extern const char *ora_tcl_inits[];
}

namespace ora {

using utl::Logger;

extern "C" {
extern int Ora_Init(Tcl_Interp *interp);
}

void setLogger(Logger* logger);

Logger* logger_ = nullptr;

Ora::Ora() :
  sourceFlag_(true)
{
}

Ora::~Ora()
{
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string sendPostRequest(const std::string& url, const std::string& jsonData) {
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

void Ora::init(Tcl_Interp *tcl_interp,
               odb::dbDatabase *db, 
               utl::Logger* logger)
{
    db_ = db;
    logger_ = logger;

    Ora_Init(tcl_interp);
    sta::evalTclInit(tcl_interp, sta::ora_tcl_inits);
    
    hostUrl = "https://bursting-stallion-friendly.ngrok-free.app/graphs/agent-retriever";
    sourceFlag_ = true;
}

void Ora::askbot(const char* query)
{
    logger_->info(utl::ORA, 101, "Sending POST request to {}", hostUrl);

    std::stringstream jsonDataStream;
    jsonDataStream << R"({
        "query": ")" << query << R"(",
        "chat_history": [],
        "list_sources": )" << (sourceFlag_ ? "true" : "false") << R"(
    })";

    std::string jsonData = jsonDataStream.str();
    std::string postResponse = sendPostRequest(hostUrl, jsonData);

    if (postResponse.empty()) {
        logger_->warn(utl::ORA, 102, "No response received from API.");
        return;
    }

    try {
        boost::json::value jsonObject = boost::json::parse(postResponse);
        
        if (jsonObject.as_object().contains("response") && jsonObject.at("response").is_string()) {
            std::string response = jsonObject.at("response").as_string().c_str();
            
            if (sourceFlag_ && jsonObject.as_object().contains("sources") && jsonObject.at("sources").is_array()) {
                std::string sources;
                for (const auto& src : jsonObject.at("sources").as_array()) {
                    sources += src.as_string().c_str();
                    sources += "\n";
                }
                logger_->info(utl::ORA, 103, "ORAssistant Response: \n\n{}\nSources:\n{}", response, sources);
            } else {
                logger_->info(utl::ORA, 104, "ORAssistant Response: \n\n{}\n", response);
            }
        } else {
            logger_->warn(utl::ORA, 106, "API response does not contain 'response' field or it is not a string.");
            logger_->warn(utl::ORA, 107, "API response: {}", postResponse);
        }
    } catch (const boost::json::system_error& e) {
        logger_->error(utl::ORA, 105, "JSON Parsing Error: {}", e.what());
    }
}

void Ora::setSourceFlag(bool sourceFlag)
{
    sourceFlag_ = sourceFlag;
}

void Ora::set_bothost(const char* host)
{
    hostUrl = host;
    logger_->info(utl::ORA, 100, "Setting ORAssistant host to {}", hostUrl);
}


} // namespace ora