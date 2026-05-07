// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>
#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

#include "request_handler.h"

namespace web {

// Registration-based dispatcher that maps JSON "type" strings to handler
// functions.  Each handler class registers its own request types via
// registerRequests(), keeping dispatch logic co-located with the handler.
//
// The dispatcher parses the incoming message exactly once and stores the
// resulting object on req.json; handlers read fields directly via the
// getJson* helpers in request_handler.h.
class RequestDispatcher
{
 public:
  using HandleFn = std::function<WebSocketResponse(const WebSocketRequest& req,
                                                   SessionState& state)>;

  struct Entry
  {
    WebSocketRequest::Type type;
    HandleFn handle;
    bool run_inline = false;  // true = skip net::post (debug handlers)
  };

  void add(const std::string& type_name,
           WebSocketRequest::Type type,
           HandleFn handle,
           bool run_inline = false)
  {
    Entry entry{
        .type = type, .handle = std::move(handle), .run_inline = run_inline};
    by_type_[static_cast<int>(type)]
        = &by_name_.emplace(type_name, std::move(entry)).first->second;
  }

  // Parse a raw JSON message into a WebSocketRequest.  On any failure
  // (malformed JSON, non-object root, missing/wrongly-typed `id` or `type`)
  // the request type is kUnknown and req.json is left empty.
  WebSocketRequest parse(const std::string& msg) const
  {
    WebSocketRequest req;
    try {
      boost::json::value parsed = boost::json::parse(msg);
      req.json = std::move(parsed.as_object());
      req.id = static_cast<uint32_t>(req.json.at("id").as_int64());
      req.raw_type = std::string(req.json.at("type").as_string());
      if (auto it = by_name_.find(req.raw_type); it != by_name_.end()) {
        req.type = it->second.type;
        return req;
      }
    } catch (const std::exception& e) {
      req.parse_error = e.what();
    }
    req.type = WebSocketRequest::kUnknown;
    return req;
  }

  // Look up the registration for a parsed request.  Returns nullptr
  // for unregistered types (kUnknown).
  const Entry* find(WebSocketRequest::Type type) const
  {
    auto it = by_type_.find(static_cast<int>(type));
    return it != by_type_.end() ? it->second : nullptr;
  }

 private:
  std::unordered_map<std::string, Entry> by_name_;
  // Keyed by int to avoid requiring a hash specialization for the enum.
  std::unordered_map<int, const Entry*> by_type_;
};

}  // namespace web
