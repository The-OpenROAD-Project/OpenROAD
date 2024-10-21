// Copyright (c) 2021, The Regents of the University of California
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
  sourceFlag_(false),
  contextFlag_(false)
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

    // Define SWIG TCL commands.
    Ora_Init(tcl_interp);
    // Eval encoded sta TCL sources.
    sta::evalTclInit(tcl_interp, sta::ora_tcl_inits);
}

void Ora::askbot(const char* query)
{
    std::string apiUrl = "http://localhost:8000/graphs/agent-retriever";
    logger_->info(utl::ORD, 101, "Sending POST request to {}", apiUrl);
    logger_->info(utl::ORD, 102, "Data: {}", query);

    std::stringstream jsonDataStream;
    jsonDataStream << R"({
        "query": ")" << query << R"(",
        "chat_history": [],
        "list_sources": )" << (sourceFlag_ ? "true" : "false") << R"(,
        "list_context": )" << (contextFlag_ ? "true" : "false") << R"(
    })";

    std::string jsonData = jsonDataStream.str();
    std::string response = sendPostRequest(apiUrl, jsonData);

    if (response.empty()) {
        logger_->warn(utl::ORD, 103, "No response received from API.");
    } else {
        logger_->info(utl::ORD, 104, "API Response: {}", response);
    }
}

void Ora::setSourceFlag(bool sourceFlag)
{
    sourceFlag_ = sourceFlag;
}

void Ora::setContextFlag(bool contextFlag)
{
    contextFlag_ = contextFlag;
}

} // namespace ora