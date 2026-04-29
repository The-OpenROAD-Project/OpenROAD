// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
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
// Handlers extract their own fields from req.raw_json using the extract_*
// utilities, so WebSocketRequest carries only {id, type, raw_json}.
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

  // Parse a raw JSON message into a WebSocketRequest.
  // Extracts {id, type, raw_json} only -- handlers parse their own fields.
  WebSocketRequest parse(const std::string& msg) const
  {
    WebSocketRequest req;
    req.id = static_cast<uint32_t>(extract_int(msg, "id"));
    req.raw_json = msg;

    const std::string type_str = extract_string(msg, "type");
    auto it = by_name_.find(type_str);
    if (it != by_name_.end()) {
      req.type = it->second.type;
    } else {
      req.type = WebSocketRequest::kUnknown;
    }
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
