// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "web/web.h"

#include <algorithm>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "color.h"
#include "db_sta/dbSta.hh"
#include "gui/descriptor_registry.h"
#include "gui/gui.h"
#include "lodepng.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "search.h"
#include "tile_generator.h"
#include "timing_report.h"
#include "tcl.h"
#include "utl/Logger.h"
#include "utl/timer.h"

namespace web {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// ShapeCollector — a gui::Painter that collects rectangles from
// descriptor->highlight() calls for use in tile rendering.
class ShapeCollector : public gui::Painter
{
 public:
  ShapeCollector() : Painter(nullptr, odb::Rect(), 1.0) {}

  std::vector<odb::Rect> rects;

  void drawRect(const odb::Rect& rect, int, int) override
  {
    rects.push_back(rect);
  }
  void drawPolygon(const odb::Polygon& polygon) override
  {
    rects.push_back(polygon.getEnclosingRect());
  }
  void drawOctagon(const odb::Oct& oct) override
  {
    rects.push_back(oct.getEnclosingRect());
  }

  // No-ops
  Color getPenColor() override { return {}; }
  void setPen(odb::dbTechLayer*, bool) override {}
  void setPen(const Color&, bool, int) override {}
  void setPenWidth(int) override {}
  void setBrush(odb::dbTechLayer*, int) override {}
  void setBrush(const Color&, const Brush&) override {}
  void setFont(const Font&) override {}
  void saveState() override {}
  void restoreState() override {}
  void drawLine(const odb::Point&, const odb::Point&) override {}
  void drawCircle(int, int, int) override {}
  void drawX(int, int, int) override {}
  void drawPolygon(const std::vector<odb::Point>&) override {}
  void drawString(int, int, Anchor, const std::string&, bool) override {}
  odb::Rect stringBoundaries(int, int, Anchor, const std::string&) override
  {
    return {};
  }
  void drawRuler(int, int, int, int, bool, const std::string&) override {}
};

// TileGenerator methods and collectTimingPathShapes are in tile_generator.cpp
//------------------------------------------------------------------------------
// Transport-agnostic request/response types and dispatch
//------------------------------------------------------------------------------

// Escape a string for safe inclusion in a JSON string value.
// Handles backslash and double-quote (the common offenders in Verilog names).
static std::string json_escape(const std::string& s)
{
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          // Escape other control characters as \u00XX
          char buf[8];
          snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
          out += buf;
        } else {
          out += c;
        }
        break;
    }
  }
  return out;
}

// Store a Selected in the clickables vector and return its index.
static int storeSelectable(std::vector<gui::Selected>& selectables,
                           const gui::Selected& sel)
{
  int id = static_cast<int>(selectables.size());
  selectables.push_back(sel);
  return id;
}

// Serialize a std::any value that might be a Selected.  If it is,
// write "value":"<name>", "select_id":<N> so the client can navigate.
// Otherwise just write "value":"<string>".
static void serializeAnyValue(std::stringstream& ss,
                              const char* field,
                              const std::any& value,
                              std::vector<gui::Selected>& selectables,
                              bool short_name = false)
{
  if (auto* sel = std::any_cast<gui::Selected>(&value)) {
    if (*sel) {
      std::string name = short_name ? sel->getShortName() : sel->getName();
      int id = storeSelectable(selectables, *sel);
      ss << "\"" << field << "\": \"" << json_escape(name) << "\", \""
         << field << "_select_id\": " << id;
      return;
    }
  }
  std::string str = gui::Descriptor::Property::toString(value);
  ss << "\"" << field << "\": \"" << json_escape(str) << "\"";
}

// Serialize a Descriptor::Property value to a JSON fragment.
// Leaf values: {"name":"...", "value":"..."}
// PropertyList / SelectionSet: {"name":"...", "children":[...]}
// Selected values get an additional select_id for click navigation.
static void serializeProperty(std::stringstream& ss,
                              const gui::Descriptor::Property& prop,
                              std::vector<gui::Selected>& selectables)
{
  ss << "{\"name\": \"" << json_escape(prop.name) << "\"";

  // Check for compound types that should be rendered as expandable groups.
  if (auto* plist
      = std::any_cast<gui::Descriptor::PropertyList>(&prop.value)) {
    ss << ", \"children\": [";
    bool first = true;
    for (const auto& [key, val] : *plist) {
      if (!first) {
        ss << ", ";
      }
      ss << "{";
      serializeAnyValue(ss, "name", key, selectables, /*short_name=*/true);
      ss << ", ";
      serializeAnyValue(ss, "value", val, selectables);
      ss << "}";
      first = false;
    }
    ss << "]";
  } else if (auto* sel_set
             = std::any_cast<gui::SelectionSet>(&prop.value)) {
    ss << ", \"children\": [";
    bool first = true;
    for (const auto& sel : *sel_set) {
      if (!first) {
        ss << ", ";
      }
      int id = storeSelectable(selectables, sel);
      ss << "{\"name\": \"" << json_escape(sel.getName())
         << "\", \"name_select_id\": " << id << "}";
      first = false;
    }
    ss << "]";
  } else if (auto* sel = std::any_cast<gui::Selected>(&prop.value)) {
    // Single Selected leaf value (e.g., "Master" → dbMaster*)
    if (*sel) {
      int id = storeSelectable(selectables, *sel);
      ss << ", \"value\": \"" << json_escape(sel->getName())
         << "\", \"value_select_id\": " << id;
    }
  } else {
    // Plain leaf value — convert to string.
    std::string val_str = prop.toString();
    ss << ", \"value\": \"" << json_escape(val_str) << "\"";
  }

  ss << "}";
}

//------------------------------------------------------------------------------
// TclEvaluator — thread-safe Tcl command evaluation with output capture.
// Uses Logger::redirectStringBegin/End to capture all output (puts, logger
// messages) without echoing to the server console.
//------------------------------------------------------------------------------

struct TclEvaluator
{
  Tcl_Interp* interp;
  utl::Logger* logger;
  std::mutex mutex;

  struct Result
  {
    std::string output;
    std::string result;
    bool is_error;
  };

  TclEvaluator(Tcl_Interp* interp, utl::Logger* logger)
      : interp(interp), logger(logger)
  {
  }

  Result eval(const std::string& cmd)
  {
    std::lock_guard<std::mutex> lock(mutex);
    logger->redirectStringBegin();
    int rc = Tcl_Eval(interp, cmd.c_str());
    Result r;
    r.output = logger->redirectStringEnd();
    r.result = Tcl_GetStringResult(interp);
    r.is_error = (rc != TCL_OK);
    return r;
  }
};

//------------------------------------------------------------------------------

struct WsRequest
{
  uint32_t id = 0;
  enum Type
  {
    TILE,
    BOUNDS,
    LAYERS,
    INFO,
    SELECT,
    INSPECT,
    HOVER,
    TCL_EVAL,
    TIMING_REPORT,
    TIMING_HIGHLIGHT,
    UNKNOWN
  } type
      = UNKNOWN;
  std::string layer;
  int z = 0;
  int x = 0;
  int y = 0;

  // SELECT fields
  int select_x = 0;
  int select_y = 0;
  int select_zoom = 0;

  // INSPECT / HOVER fields
  int select_id = -1;

  // TCL_EVAL fields
  std::string tcl_cmd;

  // TIMING_REPORT fields
  bool timing_is_setup = true;
  int timing_max_paths = 100;

  // TIMING_HIGHLIGHT fields
  int timing_path_index = -1;  // -1 = clear
  bool timing_highlight_setup = true;
  std::string timing_pin_name;  // optional: highlight this pin's net in yellow

  // Visibility flags (default: all visible)
  TileVisibility vis;
};

struct WsResponse
{
  uint32_t id = 0;
  // 0 = JSON payload, 1 = PNG payload, 2 = error
  uint8_t type = 0;
  std::vector<unsigned char> payload;
};

// Minimal JSON field extraction (no JSON library dependency)
static std::string extract_string(const std::string& json,
                                  const std::string& key)
{
  const std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos) {
    return {};
  }
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos) {
    return {};
  }
  auto quote_start = json.find('"', pos + 1);
  if (quote_start == std::string::npos) {
    return {};
  }
  // Find closing quote, skipping escaped quotes
  std::string result;
  for (size_t i = quote_start + 1; i < json.size(); i++) {
    if (json[i] == '\\' && i + 1 < json.size()) {
      switch (json[i + 1]) {
        case '"': result += '"'; break;
        case '\\': result += '\\'; break;
        case 'n': result += '\n'; break;
        case 'r': result += '\r'; break;
        case 't': result += '\t'; break;
        default: result += json[i + 1]; break;
      }
      i++;  // skip the escaped char
    } else if (json[i] == '"') {
      break;  // closing quote
    } else {
      result += json[i];
    }
  }
  return result;
}

static int extract_int(const std::string& json, const std::string& key)
{
  const std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos) {
    return 0;
  }
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos) {
    return 0;
  }
  // Skip whitespace after colon
  pos++;
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) {
    pos++;
  }
  try {
    return std::stoi(json.substr(pos));
  } catch (...) {
    return 0;
  }
}

// Like extract_int but returns default_val when key is absent
static int extract_int_or(const std::string& json,
                          const std::string& key,
                          int default_val)
{
  const std::string needle = "\"" + key + "\"";
  if (json.find(needle) == std::string::npos) {
    return default_val;
  }
  return extract_int(json, key);
}

static WsRequest parse_ws_request(const std::string& msg)
{
  WsRequest req;
  req.id = static_cast<uint32_t>(extract_int(msg, "id"));

  std::string type_str = extract_string(msg, "type");
  if (type_str == "tile") {
    req.type = WsRequest::TILE;
    req.layer = extract_string(msg, "layer");
    req.z = extract_int(msg, "z");
    req.x = extract_int(msg, "x");
    req.y = extract_int(msg, "y");
    req.vis.stdcells = extract_int_or(msg, "stdcells", 1);
    req.vis.macros = extract_int_or(msg, "macros", 1);
    // Pad sub-types
    req.vis.pad_input = extract_int_or(msg, "pad_input", 1);
    req.vis.pad_output = extract_int_or(msg, "pad_output", 1);
    req.vis.pad_inout = extract_int_or(msg, "pad_inout", 1);
    req.vis.pad_power = extract_int_or(msg, "pad_power", 1);
    req.vis.pad_spacer = extract_int_or(msg, "pad_spacer", 1);
    req.vis.pad_areaio = extract_int_or(msg, "pad_areaio", 1);
    req.vis.pad_other = extract_int_or(msg, "pad_other", 1);
    // Physical sub-types
    req.vis.phys_fill = extract_int_or(msg, "phys_fill", 1);
    req.vis.phys_endcap = extract_int_or(msg, "phys_endcap", 1);
    req.vis.phys_welltap = extract_int_or(msg, "phys_welltap", 1);
    req.vis.phys_tie = extract_int_or(msg, "phys_tie", 1);
    req.vis.phys_antenna = extract_int_or(msg, "phys_antenna", 1);
    req.vis.phys_cover = extract_int_or(msg, "phys_cover", 1);
    req.vis.phys_bump = extract_int_or(msg, "phys_bump", 1);
    req.vis.phys_other = extract_int_or(msg, "phys_other", 1);
    // Std cell sub-types
    req.vis.std_bufinv = extract_int_or(msg, "std_bufinv", 1);
    req.vis.std_bufinv_timing = extract_int_or(msg, "std_bufinv_timing", 1);
    req.vis.std_clock_bufinv = extract_int_or(msg, "std_clock_bufinv", 1);
    req.vis.std_clock_gate = extract_int_or(msg, "std_clock_gate", 1);
    req.vis.std_level_shift = extract_int_or(msg, "std_level_shift", 1);
    req.vis.std_sequential = extract_int_or(msg, "std_sequential", 1);
    req.vis.std_combinational = extract_int_or(msg, "std_combinational", 1);
    // Net sub-types
    req.vis.net_signal = extract_int_or(msg, "net_signal", 1);
    req.vis.net_power = extract_int_or(msg, "net_power", 1);
    req.vis.net_ground = extract_int_or(msg, "net_ground", 1);
    req.vis.net_clock = extract_int_or(msg, "net_clock", 1);
    req.vis.net_reset = extract_int_or(msg, "net_reset", 1);
    req.vis.net_tieoff = extract_int_or(msg, "net_tieoff", 1);
    req.vis.net_scan = extract_int_or(msg, "net_scan", 1);
    req.vis.net_analog = extract_int_or(msg, "net_analog", 1);
    // Shapes
    req.vis.routing = extract_int_or(msg, "routing", 1);
    req.vis.special_nets = extract_int_or(msg, "special_nets", 1);
    req.vis.pins = extract_int_or(msg, "pins", 1);
    req.vis.blockages = extract_int_or(msg, "blockages", 1);
    // Debug
    req.vis.debug = extract_int_or(msg, "debug", 0);
  } else if (type_str == "bounds") {
    req.type = WsRequest::BOUNDS;
  } else if (type_str == "layers") {
    req.type = WsRequest::LAYERS;
  } else if (type_str == "info") {
    req.type = WsRequest::INFO;
  } else if (type_str == "inspect") {
    req.type = WsRequest::INSPECT;
    req.select_id = extract_int(msg, "select_id");
  } else if (type_str == "hover") {
    req.type = WsRequest::HOVER;
    req.select_id = extract_int(msg, "select_id");
  } else if (type_str == "tcl_eval") {
    req.type = WsRequest::TCL_EVAL;
    req.tcl_cmd = extract_string(msg, "cmd");
  } else if (type_str == "timing_report") {
    req.type = WsRequest::TIMING_REPORT;
    req.timing_is_setup = extract_int_or(msg, "is_setup", 1);
    req.timing_max_paths = extract_int_or(msg, "max_paths", 100);
  } else if (type_str == "timing_highlight") {
    req.type = WsRequest::TIMING_HIGHLIGHT;
    req.timing_path_index = extract_int_or(msg, "path_index", -1);
    req.timing_highlight_setup = extract_int_or(msg, "is_setup", 1);
    req.timing_pin_name = extract_string(msg, "pin_name");
  } else if (type_str == "select") {
    req.type = WsRequest::SELECT;
    req.select_x = extract_int(msg, "dbu_x");
    req.select_y = extract_int(msg, "dbu_y");
    req.select_zoom = extract_int_or(msg, "zoom", 0);
    // Visibility flags for filtering selectable instances
    req.vis.stdcells = extract_int_or(msg, "stdcells", 1);
    req.vis.macros = extract_int_or(msg, "macros", 1);
    req.vis.pad_input = extract_int_or(msg, "pad_input", 1);
    req.vis.pad_output = extract_int_or(msg, "pad_output", 1);
    req.vis.pad_inout = extract_int_or(msg, "pad_inout", 1);
    req.vis.pad_power = extract_int_or(msg, "pad_power", 1);
    req.vis.pad_spacer = extract_int_or(msg, "pad_spacer", 1);
    req.vis.pad_areaio = extract_int_or(msg, "pad_areaio", 1);
    req.vis.pad_other = extract_int_or(msg, "pad_other", 1);
    req.vis.phys_fill = extract_int_or(msg, "phys_fill", 1);
    req.vis.phys_endcap = extract_int_or(msg, "phys_endcap", 1);
    req.vis.phys_welltap = extract_int_or(msg, "phys_welltap", 1);
    req.vis.phys_tie = extract_int_or(msg, "phys_tie", 1);
    req.vis.phys_antenna = extract_int_or(msg, "phys_antenna", 1);
    req.vis.phys_cover = extract_int_or(msg, "phys_cover", 1);
    req.vis.phys_bump = extract_int_or(msg, "phys_bump", 1);
    req.vis.phys_other = extract_int_or(msg, "phys_other", 1);
    req.vis.std_bufinv = extract_int_or(msg, "std_bufinv", 1);
    req.vis.std_bufinv_timing = extract_int_or(msg, "std_bufinv_timing", 1);
    req.vis.std_clock_bufinv = extract_int_or(msg, "std_clock_bufinv", 1);
    req.vis.std_clock_gate = extract_int_or(msg, "std_clock_gate", 1);
    req.vis.std_level_shift = extract_int_or(msg, "std_level_shift", 1);
    req.vis.std_sequential = extract_int_or(msg, "std_sequential", 1);
    req.vis.std_combinational = extract_int_or(msg, "std_combinational", 1);
  } else {
    req.type = WsRequest::UNKNOWN;
  }
  return req;
}

static WsResponse dispatch_request(
    const WsRequest& req,
    const std::shared_ptr<TileGenerator>& gen,
    const std::vector<odb::Rect>& highlight_rects = {},
    const std::vector<ColoredRect>& colored_rects = {},
    const std::vector<FlightLine>& flight_lines = {})
{
  WsResponse resp;
  resp.id = req.id;

  switch (req.type) {
    case WsRequest::BOUNDS: {
      resp.type = 0;  // JSON
      const odb::Rect bounds = gen->getBounds();
      std::stringstream ss;
      ss << "{\"bounds\": [[" << bounds.yMin() << ", " << bounds.xMin()
         << "], [" << bounds.yMax() << ", " << bounds.xMax() << "]]}";
      const std::string json = ss.str();
      resp.payload.assign(json.begin(), json.end());
      break;
    }
    case WsRequest::LAYERS: {
      resp.type = 0;  // JSON
      std::stringstream ss;
      ss << "{\"layers\": [";
      bool first = true;
      for (const auto& name : gen->getLayers()) {
        if (!first) {
          ss << ", ";
        }
        ss << "\"" << name << "\"";
        first = false;
      }
      ss << "]}";
      const std::string json = ss.str();
      resp.payload.assign(json.begin(), json.end());
      break;
    }
    case WsRequest::TILE: {
      resp.type = 1;  // PNG
      resp.payload = gen->generateTile(
          req.layer, req.z, req.x, req.y, req.vis, highlight_rects,
          colored_rects, flight_lines);
      break;
    }
    case WsRequest::INFO: {
      resp.type = 0;  // JSON
      const std::string json = gen->hasSta() ? "{\"has_liberty\": true}"
                                             : "{\"has_liberty\": false}";
      resp.payload.assign(json.begin(), json.end());
      break;
    }
    default: {
      resp.type = 2;  // error
      const std::string err = "Unknown request type";
      resp.payload.assign(err.begin(), err.end());
      break;
    }
  }

  return resp;
}

// Serialize a WsResponse into the binary wire format:
//   [0..3] uint32_t id (big-endian)
//   [4]    uint8_t  type
//   [5..7] reserved
//   [8..]  payload
static std::vector<unsigned char> serialize_response(const WsResponse& resp)
{
  std::vector<unsigned char> frame(8 + resp.payload.size());
  const uint32_t id_be = htonl(resp.id);
  std::memcpy(frame.data(), &id_be, 4);
  frame[4] = resp.type;
  frame[5] = frame[6] = frame[7] = 0;
  if (!resp.payload.empty()) {
    std::memcpy(frame.data() + 8, resp.payload.data(), resp.payload.size());
  }
  return frame;
}

//------------------------------------------------------------------------------
// HTTP request handler (wraps dispatch_request for HTTP transport)
//------------------------------------------------------------------------------

static std::string content_type_for(const std::string& path)
{
  auto ext = std::filesystem::path(path).extension().string();
  if (ext == ".html") {
    return "text/html";
  }
  if (ext == ".js") {
    return "application/javascript";
  }
  if (ext == ".css") {
    return "text/css";
  }
  if (ext == ".png") {
    return "image/png";
  }
  if (ext == ".json") {
    return "application/json";
  }
  return "application/octet-stream";
}

http::response<http::string_body> handle_request(
    http::request<http::string_body>&& req,
    std::shared_ptr<TileGenerator> generator,
    const std::string& doc_root)
{
  http::response<http::string_body> res{http::status::ok, req.version()};
  res.set(http::field::server, "Boost.Beast Server (C++17)");
  res.set(http::field::content_type, "text/plain");
  res.keep_alive(req.keep_alive());
  res.set(http::field::access_control_allow_origin, "*");

  std::regex tile_regex(R"(/tile/(\w+)/(\d+)/(-?\d+)/(-?\d+)\.png)");
  std::smatch match_pieces;
  std::string target_path(req.target());

  if (req.method() == http::verb::get && req.target() == "/bounds") {
    WsRequest ws_req;
    ws_req.type = WsRequest::BOUNDS;
    WsResponse ws_resp = dispatch_request(ws_req, generator);
    res.set(http::field::content_type, "application/json");
    res.body() = std::string(ws_resp.payload.begin(), ws_resp.payload.end());
  } else if (req.method() == http::verb::get && req.target() == "/layers") {
    WsRequest ws_req;
    ws_req.type = WsRequest::LAYERS;
    WsResponse ws_resp = dispatch_request(ws_req, generator);
    res.set(http::field::content_type, "application/json");
    res.body() = std::string(ws_resp.payload.begin(), ws_resp.payload.end());
  } else if (req.method() == http::verb::get
             && std::regex_match(target_path, match_pieces, tile_regex)) {
    WsRequest ws_req;
    ws_req.type = WsRequest::TILE;
    ws_req.layer = match_pieces[1].str();
    ws_req.z = std::stoi(match_pieces[2].str());
    ws_req.x = std::stoi(match_pieces[3].str());
    ws_req.y = std::stoi(match_pieces[4].str());
    WsResponse ws_resp = dispatch_request(ws_req, generator);

    res.set(http::field::content_type, "image/png");
    res.body() = std::string(ws_resp.payload.begin(), ws_resp.payload.end());
    res.set(http::field::cache_control, "public, max-age=604800");
  } else if (req.method() == http::verb::get && !doc_root.empty()) {
    // Serve static files from doc_root
    std::string file_path = target_path;
    if (file_path == "/") {
      file_path = "/index.html";
    }
    // Reject paths with ".." to prevent directory traversal
    if (file_path.find("..") == std::string::npos) {
      auto full_path = std::filesystem::path(doc_root) / file_path.substr(1);
      std::ifstream file(full_path, std::ios::binary);
      if (file) {
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        res.set(http::field::content_type, content_type_for(file_path));
        res.body() = std::move(content);
      } else {
        res.result(http::status::not_found);
        res.body() = "File not found.";
      }
    } else {
      res.result(http::status::bad_request);
      res.body() = "Invalid path.";
    }
  } else {
    res.result(http::status::not_found);
    res.body() = "Resource not found.";
  }

  res.prepare_payload();
  return res;
}

//------------------------------------------------------------------------------
// WebSocket session - multiplexes many requests over a single connection
//------------------------------------------------------------------------------

class WsSession : public std::enable_shared_from_this<WsSession>
{
  websocket::stream<beast::tcp_stream> ws_;
  beast::flat_buffer buffer_;
  std::shared_ptr<TileGenerator> generator_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
  std::shared_ptr<TimingReport> timing_report_;

  // Per-session highlight shapes (collected via descriptor->highlight())
  std::mutex selection_mutex_;
  std::vector<odb::Rect> highlight_rects_;
  std::vector<ColoredRect> timing_rects_;
  std::vector<FlightLine> timing_lines_;

  // Clickable objects from the last property response.
  // Index in this vector is the select_id sent to the client.
  std::mutex selectables_mutex_;
  std::vector<gui::Selected> selectables_;

  // Write serialization: strand + queue ensures one async_write at a time
  net::strand<net::any_io_executor> strand_;
  std::deque<std::vector<unsigned char>> write_queue_;
  bool writing_ = false;

 public:
  WsSession(tcp::socket&& socket,
             std::shared_ptr<TileGenerator> generator,
             std::shared_ptr<TclEvaluator> tcl_eval,
             std::shared_ptr<TimingReport> timing_report)
      : ws_(std::move(socket)),
        generator_(std::move(generator)),
        tcl_eval_(std::move(tcl_eval)),
        timing_report_(std::move(timing_report)),
        strand_(net::make_strand(ws_.get_executor()))
  {
  }

  // Accept the WebSocket upgrade using the already-read HTTP request
  void run(http::request<http::string_body>&& req)
  {
    ws_.set_option(
        websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws_.set_option(
        websocket::stream_base::decorator([](websocket::response_type& res) {
          res.set(http::field::server, "OpenROAD WebSocket Server");
        }));

    ws_.async_accept(req, [self = shared_from_this()](beast::error_code ec) {
      self->on_accept(ec);
    });
  }

 private:
  void on_accept(beast::error_code ec)
  {
    if (ec) {
      std::cerr << "ws accept error: " << ec.message() << "\n";
      return;
    }
    do_read();
  }

  void do_read()
  {
    ws_.async_read(
        buffer_,
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
          self->on_read(ec);
        });
  }

  void on_read(beast::error_code ec)
  {
    if (ec) {
      if (ec != websocket::error::closed) {
        std::cerr << "ws read error: " << ec.message() << "\n";
      }
      return;
    }

    // Parse the incoming text message as a request
    const std::string msg = beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());

    const WsRequest req = parse_ws_request(msg);

    auto self = shared_from_this();
    auto gen = generator_;

    if (req.type == WsRequest::SELECT) {
      // Handle selection on the thread pool — single selectAt call
      net::post(ws_.get_executor(), [self, gen, req]() {
        WsResponse resp;
        resp.id = req.id;
        try {
          auto results = gen->selectAt(
              req.select_x, req.select_y, req.select_zoom, req.vis);

          // Collect highlight shapes from the first selected instance
          {
            std::lock_guard<std::mutex> lock(self->selection_mutex_);
            self->highlight_rects_.clear();
            self->timing_rects_.clear();
            self->timing_lines_.clear();
            if (!results.empty()) {
              auto* registry = gui::DescriptorRegistry::instance();
              gui::Selected sel
                  = registry->makeSelected(std::any(results[0].inst));
              if (sel) {
                ShapeCollector collector;
                sel.highlight(collector);
                self->highlight_rects_ = std::move(collector.rects);
              }
            }
          }

          // Build JSON response with selection and properties
          resp.type = 0;  // JSON
          std::stringstream ss;
          ss << "{\"selected\": [";
          bool first = true;
          for (const auto& r : results) {
            if (!first) {
              ss << ", ";
            }
            ss << "{\"name\": \"" << json_escape(r.name)
               << "\", \"master\": \"" << json_escape(r.master)
               << "\", \"bbox\": [" << r.bbox.xMin() << ", "
               << r.bbox.yMin() << ", " << r.bbox.xMax() << ", "
               << r.bbox.yMax() << "]}";
            first = false;
          }
          ss << "]";

          // Add properties for the first selected instance
          std::vector<gui::Selected> new_selectables;
          if (!results.empty()) {
            // Top-level bbox for the toolbar zoom button
            const auto& r0 = results[0];
            ss << ", \"bbox\": [" << r0.bbox.xMin() << ", "
               << r0.bbox.yMin() << ", " << r0.bbox.xMax() << ", "
               << r0.bbox.yMax() << "]";

            auto* registry = gui::DescriptorRegistry::instance();
            gui::Selected sel
                = registry->makeSelected(std::any(r0.inst));
            if (sel) {
              auto props = sel.getProperties();
              ss << ", \"properties\": [";
              bool pfirst = true;
              for (const auto& prop : props) {
                if (!pfirst) {
                  ss << ", ";
                }
                serializeProperty(ss, prop, new_selectables);
                pfirst = false;
              }
              ss << "]";
            }
          }
          {
            std::lock_guard<std::mutex> lock(self->selectables_mutex_);
            self->selectables_ = std::move(new_selectables);
          }

          ss << "}";
          const std::string json = ss.str();
          resp.payload.assign(json.begin(), json.end());
        } catch (const std::exception& e) {
          resp.type = 2;
          std::string err = std::string("server error: ") + e.what();
          resp.payload.assign(err.begin(), err.end());
        }
        self->queue_response(resp);
      });
    } else if (req.type == WsRequest::INSPECT) {
      // Navigate to a previously-serialized Selected object by its ID
      net::post(ws_.get_executor(), [self, req]() {
        WsResponse resp;
        resp.id = req.id;
        try {
          gui::Selected sel;
          {
            std::lock_guard<std::mutex> lock(self->selectables_mutex_);
            if (req.select_id >= 0
                && req.select_id
                       < static_cast<int>(self->selectables_.size())) {
              sel = self->selectables_[req.select_id];
            }
          }

          // Collect highlight shapes from the inspected object
          {
            std::lock_guard<std::mutex> lock(self->selection_mutex_);
            self->highlight_rects_.clear();
            self->timing_rects_.clear();
            self->timing_lines_.clear();
            if (sel) {
              ShapeCollector collector;
              sel.highlight(collector);
              self->highlight_rects_ = std::move(collector.rects);
            }
          }

          resp.type = 0;
          std::stringstream ss;
          if (sel) {
            auto props = sel.getProperties();
            std::vector<gui::Selected> new_selectables;
            ss << "{\"name\": \"" << json_escape(sel.getName())
               << "\", \"type\": \"" << json_escape(sel.getTypeName())
               << "\", \"properties\": [";
            bool first = true;
            for (const auto& prop : props) {
              if (!first) {
                ss << ", ";
              }
              serializeProperty(ss, prop, new_selectables);
              first = false;
            }
            ss << "]";
            // Include bbox for highlight
            odb::Rect bbox;
            if (sel.getBBox(bbox)) {
              ss << ", \"bbox\": [" << bbox.xMin() << ", " << bbox.yMin()
                 << ", " << bbox.xMax() << ", " << bbox.yMax() << "]";
            }
            ss << "}";
            {
              std::lock_guard<std::mutex> lock(self->selectables_mutex_);
              self->selectables_ = std::move(new_selectables);
            }
          } else {
            ss << "{\"error\": \"invalid select_id\"}";
          }
          const std::string json = ss.str();
          resp.payload.assign(json.begin(), json.end());
        } catch (const std::exception& e) {
          resp.type = 2;
          std::string err = std::string("server error: ") + e.what();
          resp.payload.assign(err.begin(), err.end());
        }
        self->queue_response(resp);
      });
    } else if (req.type == WsRequest::HOVER) {
      // Return just the bbox for a selectable (no state changes)
      net::post(ws_.get_executor(), [self, req]() {
        WsResponse resp;
        resp.id = req.id;
        try {
          gui::Selected sel;
          {
            std::lock_guard<std::mutex> lock(self->selectables_mutex_);
            if (req.select_id >= 0
                && req.select_id
                       < static_cast<int>(self->selectables_.size())) {
              sel = self->selectables_[req.select_id];
            }
          }

          resp.type = 0;
          std::stringstream ss;
          if (sel) {
            ShapeCollector collector;
            sel.highlight(collector);
            if (!collector.rects.empty()) {
              ss << "{\"rects\": [";
              bool first = true;
              for (const auto& r : collector.rects) {
                if (!first) {
                  ss << ", ";
                }
                ss << "[" << r.xMin() << ", " << r.yMin() << ", "
                   << r.xMax() << ", " << r.yMax() << "]";
                first = false;
              }
              ss << "]}";
            } else {
              // Fall back to bbox if no shapes
              odb::Rect bbox;
              if (sel.getBBox(bbox)) {
                ss << "{\"rects\": [[" << bbox.xMin() << ", " << bbox.yMin()
                   << ", " << bbox.xMax() << ", " << bbox.yMax() << "]]}";
              } else {
                ss << "{\"error\": \"no bbox\"}";
              }
            }
          } else {
            ss << "{\"error\": \"invalid select_id\"}";
          }
          const std::string json = ss.str();
          resp.payload.assign(json.begin(), json.end());
        } catch (const std::exception& e) {
          resp.type = 2;
          std::string err = std::string("server error: ") + e.what();
          resp.payload.assign(err.begin(), err.end());
        }
        self->queue_response(resp);
      });
    } else if (req.type == WsRequest::TCL_EVAL) {
      // Evaluate a Tcl command and return the result + logger output
      net::post(ws_.get_executor(), [self, req]() {
        WsResponse resp;
        resp.id = req.id;
        resp.type = 0;
        try {
          auto result = self->tcl_eval_->eval(req.tcl_cmd);
          std::stringstream ss;
          ss << "{\"output\": \"" << json_escape(result.output)
             << "\", \"result\": \"" << json_escape(result.result)
             << "\", \"is_error\": "
             << (result.is_error ? "true" : "false") << "}";
          const std::string json = ss.str();
          resp.payload.assign(json.begin(), json.end());
        } catch (const std::exception& e) {
          resp.type = 2;
          std::string err = std::string("server error: ") + e.what();
          resp.payload.assign(err.begin(), err.end());
        }
        self->queue_response(resp);
      });
    } else if (req.type == WsRequest::TIMING_REPORT) {
      auto tr = timing_report_;
      auto tcl = tcl_eval_;
      net::post(ws_.get_executor(), [self, tr, tcl, req]() {
        WsResponse resp;
        resp.id = req.id;
        resp.type = 0;
        try {
          // STA is not thread-safe; serialize with the Tcl evaluator mutex
          std::lock_guard<std::mutex> lock(tcl->mutex);
          auto paths = tr->getReport(req.timing_is_setup, req.timing_max_paths);
          std::stringstream ss;
          ss << "{\"paths\": [";
          bool first_path = true;
          for (const auto& p : paths) {
            if (!first_path) {
              ss << ", ";
            }
            first_path = false;
            ss << "{\"start_clk\": \"" << json_escape(p.start_clk)
               << "\", \"end_clk\": \"" << json_escape(p.end_clk)
               << "\", \"required\": " << p.required
               << ", \"arrival\": " << p.arrival << ", \"slack\": " << p.slack
               << ", \"skew\": " << p.skew
               << ", \"path_delay\": " << p.path_delay
               << ", \"logic_depth\": " << p.logic_depth
               << ", \"fanout\": " << p.fanout << ", \"start_pin\": \""
               << json_escape(p.start_pin) << "\", \"end_pin\": \""
               << json_escape(p.end_pin) << "\", \"data_nodes\": [";
            bool first_node = true;
            for (const auto& n : p.data_nodes) {
              if (!first_node) {
                ss << ", ";
              }
              first_node = false;
              ss << "{\"pin\": \"" << json_escape(n.pin_name) << "\""
                 << ", \"fanout\": " << n.fanout
                 << ", \"rise\": " << (n.is_rising ? "true" : "false")
                 << ", \"clk\": " << (n.is_clock ? "true" : "false")
                 << ", \"time\": " << n.time << ", \"delay\": " << n.delay
                 << ", \"slew\": " << n.slew << ", \"load\": " << n.load
                 << "}";
            }
            ss << "], \"capture_nodes\": [";
            first_node = true;
            for (const auto& n : p.capture_nodes) {
              if (!first_node) {
                ss << ", ";
              }
              first_node = false;
              ss << "{\"pin\": \"" << json_escape(n.pin_name) << "\""
                 << ", \"fanout\": " << n.fanout
                 << ", \"rise\": " << (n.is_rising ? "true" : "false")
                 << ", \"clk\": " << (n.is_clock ? "true" : "false")
                 << ", \"time\": " << n.time << ", \"delay\": " << n.delay
                 << ", \"slew\": " << n.slew << ", \"load\": " << n.load
                 << "}";
            }
            ss << "]}";
          }
          ss << "]}";
          const std::string json = ss.str();
          resp.payload.assign(json.begin(), json.end());
        } catch (const std::exception& e) {
          resp.type = 2;
          std::string err = std::string("server error: ") + e.what();
          resp.payload.assign(err.begin(), err.end());
        }
        self->queue_response(resp);
      });
    } else if (req.type == WsRequest::TIMING_HIGHLIGHT) {
      auto gen = generator_;
      auto tr = timing_report_;
      auto tcl = tcl_eval_;
      net::post(ws_.get_executor(), [self, gen, tr, tcl, req]() {
        WsResponse resp;
        resp.id = req.id;
        resp.type = 0;
        try {
          std::vector<ColoredRect> new_rects;
          std::vector<FlightLine> new_lines;

          if (req.timing_path_index >= 0) {
            // Re-fetch timing paths to get the selected path's data
            std::lock_guard<std::mutex> sta_lock(tcl->mutex);
            auto paths = tr->getReport(req.timing_highlight_setup);
            if (req.timing_path_index < static_cast<int>(paths.size())) {
              odb::dbBlock* block = gen->getBlock();
              collectTimingPathShapes(
                  block, paths[req.timing_path_index], new_rects, new_lines);

              // If a specific pin is selected, highlight its net in yellow
              if (!req.timing_pin_name.empty()) {
                static const Color kStageColor{255, 255, 0, 180};
                auto [iterm, bterm]
                    = resolvePin(block, req.timing_pin_name);
                odb::dbNet* net = iterm   ? iterm->getNet()
                                  : bterm ? bterm->getNet()
                                          : nullptr;
                if (net) {
                  collectNetShapes(net, iterm, bterm, nullptr, nullptr,
                                   kStageColor, new_rects, new_lines);
                }
              }
            }
          }

          std::cerr << "TIMING_HIGHLIGHT: " << new_rects.size()
                    << " rects, " << new_lines.size() << " lines"
                    << " pin_name='" << req.timing_pin_name << "'\n";

          {
            std::lock_guard<std::mutex> lock(self->selection_mutex_);
            self->timing_rects_ = std::move(new_rects);
            self->timing_lines_ = std::move(new_lines);
            self->highlight_rects_.clear();
          }

          const std::string json = "{\"ok\": true}";
          resp.payload.assign(json.begin(), json.end());
        } catch (const std::exception& e) {
          resp.type = 2;
          std::string err = std::string("server error: ") + e.what();
          resp.payload.assign(err.begin(), err.end());
        }
        self->queue_response(resp);
      });
    } else {
      // Capture current highlight rects for tile rendering
      std::vector<odb::Rect> rects;
      std::vector<ColoredRect> colored;
      std::vector<FlightLine> lines;
      {
        std::lock_guard<std::mutex> lock(selection_mutex_);
        rects = highlight_rects_;
        colored = timing_rects_;
        lines = timing_lines_;
      }

      // Dispatch tile generation to the thread pool (not to the strand).
      // This lets multiple tiles render concurrently across all 32 threads.
      net::post(
          ws_.get_executor(),
          [self, gen, req, rects = std::move(rects),
           colored = std::move(colored), lines = std::move(lines)]() {
            WsResponse resp;
            try {
              resp = dispatch_request(req, gen, rects, colored, lines);
            } catch (const std::exception& e) {
              resp.id = req.id;
              resp.type = 2;  // error
              std::string err = std::string("server error: ") + e.what();
              resp.payload.assign(err.begin(), err.end());
              std::cerr << "dispatch error for request " << req.id << ": "
                        << e.what() << "\n";
            }
            self->queue_response(resp);
          });
    }

    // Immediately start reading the next request
    do_read();
  }

  void queue_response(const WsResponse& resp)
  {
    std::vector<unsigned char> frame = serialize_response(resp);

    // Post to the strand to serialize write queue access
    net::post(strand_,
              [self = shared_from_this(), frame = std::move(frame)]() mutable {
                self->write_queue_.push_back(std::move(frame));
                if (!self->writing_) {
                  self->do_write();
                }
              });
  }

  void do_write()
  {
    if (write_queue_.empty()) {
      writing_ = false;
      return;
    }
    writing_ = true;
    ws_.binary(true);
    ws_.async_write(
        net::buffer(write_queue_.front()),
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
          // Post back to the strand to serialize queue access
          net::post(self->strand_, [self, ec]() {
            if (ec) {
              std::cerr << "ws write error: " << ec.message() << "\n";
              return;
            }
            self->write_queue_.pop_front();
            self->do_write();
          });
        });
  }
};

//------------------------------------------------------------------------------
// HTTP session - handles traditional HTTP connections
//------------------------------------------------------------------------------

class HttpSession : public std::enable_shared_from_this<HttpSession>
{
  beast::tcp_stream stream_;
  beast::flat_buffer buffer_;
  std::shared_ptr<TileGenerator> generator_;
  std::shared_ptr<http::response<http::string_body>> res_;
  http::request<http::string_body> req_;
  std::string doc_root_;

 public:
  HttpSession(tcp::socket&& socket,
          std::shared_ptr<TileGenerator> generator,
          std::string doc_root)
      : stream_(std::move(socket)),
        generator_(generator),
        doc_root_(std::move(doc_root))
  {
  }

  void run() { do_read(); }

  // Entry point when the first request was already read by DetectSession
  void run_with_request(http::request<http::string_body> req,
                        beast::flat_buffer buffer)
  {
    req_ = std::move(req);
    buffer_ = std::move(buffer);
    on_read({});
  }

 private:
  void do_read()
  {
    req_ = {};
    http::async_read(
        stream_,
        buffer_,
        req_,
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
          self->on_read(ec);
        });
  }

  void on_read(beast::error_code ec)
  {
    if (ec == http::error::end_of_stream) {
      return do_close();
    }
    if (ec) {
      std::cerr << "Session read error: " << ec.message() << "\n";
      return;
    }

    res_ = std::make_shared<http::response<http::string_body>>(
        handle_request(std::move(req_), generator_, doc_root_));
    do_write();
  }

  void do_write()
  {
    http::async_write(
        stream_,
        *res_,
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
          self->on_write(ec);
        });
  }

  void on_write(beast::error_code ec)
  {
    if (ec) {
      std::cerr << "Session write error: " << ec.message() << "\n";
      return;
    }

    bool keep_alive = res_->keep_alive();
    res_ = nullptr;

    if (keep_alive) {
      do_read();
    } else {
      do_close();
    }
  }

  void do_close()
  {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
  }
};

//------------------------------------------------------------------------------
// Detect session - reads first HTTP request, routes to WS or HTTP session
//------------------------------------------------------------------------------

class DetectSession : public std::enable_shared_from_this<DetectSession>
{
  beast::tcp_stream stream_;
  beast::flat_buffer buffer_;
  std::shared_ptr<TileGenerator> generator_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
  std::shared_ptr<TimingReport> timing_report_;
  http::request<http::string_body> req_;
  std::string doc_root_;

 public:
  DetectSession(tcp::socket&& socket,
                 std::shared_ptr<TileGenerator> generator,
                 std::shared_ptr<TclEvaluator> tcl_eval,
                 std::shared_ptr<TimingReport> timing_report,
                 std::string doc_root)
      : stream_(std::move(socket)),
        generator_(std::move(generator)),
        tcl_eval_(std::move(tcl_eval)),
        timing_report_(std::move(timing_report)),
        doc_root_(std::move(doc_root))
  {
  }

  void run()
  {
    http::async_read(
        stream_,
        buffer_,
        req_,
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
          self->on_read(ec);
        });
  }

 private:
  void on_read(beast::error_code ec)
  {
    if (ec) {
      std::cerr << "Detect read error: " << ec.message() << "\n";
      return;
    }

    if (websocket::is_upgrade(req_)) {
      // WebSocket upgrade - hand off to WsSession
      auto ws = std::make_shared<WsSession>(
          stream_.release_socket(), generator_, tcl_eval_, timing_report_);
      ws->run(std::move(req_));
    } else {
      // Regular HTTP - hand off to session with already-read request
      auto s = std::make_shared<HttpSession>(
          stream_.release_socket(), generator_, doc_root_);
      s->run_with_request(std::move(req_), std::move(buffer_));
    }
  }
};

//------------------------------------------------------------------------------
// Listener - accepts incoming connections
//------------------------------------------------------------------------------

class Listener : public std::enable_shared_from_this<Listener>
{
  net::io_context& ioc_;
  tcp::acceptor acceptor_;
  std::shared_ptr<TileGenerator> generator_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
  std::shared_ptr<TimingReport> timing_report_;
  std::string doc_root_;

 public:
  Listener(net::io_context& ioc,
           tcp::endpoint endpoint,
           std::shared_ptr<TileGenerator> generator,
           std::shared_ptr<TclEvaluator> tcl_eval,
           std::shared_ptr<TimingReport> timing_report,
           std::string doc_root)
      : ioc_(ioc),
        acceptor_(ioc),
        generator_(generator),
        tcl_eval_(std::move(tcl_eval)),
        timing_report_(std::move(timing_report)),
        doc_root_(std::move(doc_root))
  {
    beast::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      return;
    }

    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
      return;
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
      return;
    }

    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
      return;
    }
  }

  void run() { do_accept(); }

 private:
  void do_accept()
  {
    acceptor_.async_accept(
        ioc_,
        [self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
          self->on_accept(ec, std::move(socket));
        });
  }

  void on_accept(beast::error_code ec, tcp::socket socket)
  {
    if (ec) {
      std::cerr << "Listener accept error: " << ec.message() << "\n";
    } else {
      // Route through DetectSession to handle both HTTP and WebSocket
      std::make_shared<DetectSession>(
          std::move(socket), generator_, tcl_eval_, timing_report_, doc_root_)
          ->run();
    }
    do_accept();
  }
};

WebServer::WebServer(odb::dbDatabase* db,
                     sta::dbSta* sta,
                     utl::Logger* logger,
                     Tcl_Interp* interp)
    : db_(db), sta_(sta), logger_(logger), interp_(interp)
{
}

WebServer::~WebServer() = default;

void WebServer::serve(const std::string& doc_root)
{
  try {
    generator_ = std::make_shared<TileGenerator>(db_, sta_, logger_);
    auto timing_report = std::make_shared<TimingReport>(sta_);

    // Create Tcl evaluator with logger sink for output capture
    auto tcl_eval = std::make_shared<TclEvaluator>(interp_, logger_);

    auto const address = net::ip::make_address("127.0.0.1");
    unsigned short const port = 8080;
    int const num_threads = 32;

    if (!doc_root.empty()) {
      logger_->info(utl::WEB, 4, "Serving static files from {}", doc_root);
    }
    logger_->info(utl::WEB,
                  1,
                  "Server starting on http://:{} with {} threads...",
                  port,
                  num_threads);

    net::io_context ioc{num_threads};

    std::make_shared<Listener>(
        ioc,
        tcp::endpoint{address, port},
        generator_,
        tcl_eval,
        timing_report,
        doc_root)
        ->run();

    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) {
      logger_->info(utl::WEB, 3, "Shutting down...");
      ioc.stop();
    });

    std::vector<std::thread> threads;
    threads.reserve(num_threads - 1);
    for (int i = 0; i < num_threads - 1; ++i) {
      threads.emplace_back([&ioc] { ioc.run(); });
    }

    ioc.run();

    for (auto& t : threads) {
      t.join();
    }

    std::cout << "Server stopped.\n";
  } catch (std::exception const& e) {
    logger_->error(utl::WEB, 2, "Server error : {}", e.what());
  }
}

}  // namespace web
