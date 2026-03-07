// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "web/web.h"

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
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "request_handler.h"
#include "timing_report.h"

namespace web {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

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

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{
  websocket::stream<beast::tcp_stream> ws_;
  beast::flat_buffer buffer_;
  utl::Logger* logger_;

  // Handler objects (transport-independent, testable)
  SessionState state_;
  SelectHandler select_handler_;
  TclHandler tcl_handler_;
  TimingHandler timing_handler_;
  TileHandler tile_handler_;

  // Write serialization: strand + queue ensures one async_write at a time
  net::strand<net::any_io_executor> strand_;
  std::deque<std::vector<unsigned char>> write_queue_;
  bool writing_ = false;

 public:
  WebSocketSession(tcp::socket&& socket,
                   std::shared_ptr<TileGenerator> generator,
                   std::shared_ptr<TclEvaluator> tcl_eval,
                   std::shared_ptr<TimingReport> timing_report,
                   utl::Logger* logger);

  void run(http::request<http::string_body>&& req);

 private:
  void on_accept(beast::error_code ec);
  void do_read();
  void on_read(beast::error_code ec);
  void queue_response(const WsResponse& resp);
  void do_write();
};

WebSocketSession::WebSocketSession(
    tcp::socket&& socket,
    std::shared_ptr<TileGenerator> generator,
    std::shared_ptr<TclEvaluator> tcl_eval,
    std::shared_ptr<TimingReport> timing_report,
    utl::Logger* logger)
    : ws_(std::move(socket)),
      logger_(logger),
      select_handler_(generator),
      tcl_handler_(tcl_eval),
      timing_handler_(generator, timing_report, tcl_eval),
      tile_handler_(generator),
      strand_(net::make_strand(ws_.get_executor()))
{
}

void WebSocketSession::run(http::request<http::string_body>&& req)
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

void WebSocketSession::on_accept(beast::error_code ec)
{
  if (ec) {
    debugPrint(
        logger_, utl::WEB, "ws", 1, "ws accept error: {}", ec.message());
    return;
  }
  do_read();
}

void WebSocketSession::do_read()
{
  ws_.async_read(
      buffer_,
      [self = shared_from_this()](beast::error_code ec, std::size_t) {
        self->on_read(ec);
      });
}

void WebSocketSession::on_read(beast::error_code ec)
{
  if (ec) {
    if (ec != websocket::error::closed) {
      debugPrint(
          logger_, utl::WEB, "ws", 1, "ws read error: {}", ec.message());
    }
    return;
  }

  const std::string msg = beast::buffers_to_string(buffer_.data());
  buffer_.consume(buffer_.size());

  const WsRequest req = parse_ws_request(msg);
  auto self = shared_from_this();

  switch (req.type) {
    case WsRequest::SELECT:
      net::post(ws_.get_executor(), [self, req]() {
        self->queue_response(
            self->select_handler_.handleSelect(req, self->state_));
      });
      break;
    case WsRequest::INSPECT:
      net::post(ws_.get_executor(), [self, req]() {
        self->queue_response(
            self->select_handler_.handleInspect(req, self->state_));
      });
      break;
    case WsRequest::HOVER:
      net::post(ws_.get_executor(), [self, req]() {
        self->queue_response(
            self->select_handler_.handleHover(req, self->state_));
      });
      break;
    case WsRequest::TCL_EVAL:
      net::post(ws_.get_executor(), [self, req]() {
        self->queue_response(self->tcl_handler_.handleTclEval(req));
      });
      break;
    case WsRequest::TIMING_REPORT:
      net::post(ws_.get_executor(), [self, req]() {
        self->queue_response(self->timing_handler_.handleTimingReport(req));
      });
      break;
    case WsRequest::TIMING_HIGHLIGHT:
      net::post(ws_.get_executor(), [self, req]() {
        self->queue_response(
            self->timing_handler_.handleTimingHighlight(req, self->state_));
      });
      break;
    default:
      net::post(ws_.get_executor(), [self, req]() {
        self->queue_response(
            self->tile_handler_.handleTile(req, self->state_));
      });
      break;
  }

  do_read();
}

void WebSocketSession::queue_response(const WsResponse& resp)
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

void WebSocketSession::do_write()
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
        net::post(self->strand_, [self, ec]() {
          if (ec) {
            debugPrint(self->logger_,
                       utl::WEB,
                       "ws",
                       1,
                       "ws write error: {}",
                       ec.message());
            return;
          }
          self->write_queue_.pop_front();
          self->do_write();
        });
      });
}

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
  utl::Logger* logger_;

 public:
  HttpSession(tcp::socket&& socket,
              std::shared_ptr<TileGenerator> generator,
              std::string doc_root,
              utl::Logger* logger);

  void run() { do_read(); }

  void run_with_request(http::request<http::string_body> req,
                        beast::flat_buffer buffer);

 private:
  void do_read();
  void on_read(beast::error_code ec);
  void do_write();
  void on_write(beast::error_code ec);
  void do_close();
};

HttpSession::HttpSession(tcp::socket&& socket,
                         std::shared_ptr<TileGenerator> generator,
                         std::string doc_root,
                         utl::Logger* logger)
    : stream_(std::move(socket)),
      generator_(generator),
      doc_root_(std::move(doc_root)),
      logger_(logger)
{
}

void HttpSession::run_with_request(http::request<http::string_body> req,
                                   beast::flat_buffer buffer)
{
  req_ = std::move(req);
  buffer_ = std::move(buffer);
  on_read({});
}

void HttpSession::do_read()
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

void HttpSession::on_read(beast::error_code ec)
{
  if (ec == http::error::end_of_stream) {
    return do_close();
  }
  if (ec) {
    debugPrint(
        logger_, utl::WEB, "http", 1, "http read error: {}", ec.message());
    return;
  }

  res_ = std::make_shared<http::response<http::string_body>>(
      handle_request(std::move(req_), generator_, doc_root_));
  do_write();
}

void HttpSession::do_write()
{
  http::async_write(
      stream_,
      *res_,
      [self = shared_from_this()](beast::error_code ec, std::size_t) {
        self->on_write(ec);
      });
}

void HttpSession::on_write(beast::error_code ec)
{
  if (ec) {
    debugPrint(
        logger_, utl::WEB, "http", 1, "http write error: {}", ec.message());
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

void HttpSession::do_close()
{
  beast::error_code ec;
  stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

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
  utl::Logger* logger_;

 public:
  DetectSession(tcp::socket&& socket,
                std::shared_ptr<TileGenerator> generator,
                std::shared_ptr<TclEvaluator> tcl_eval,
                std::shared_ptr<TimingReport> timing_report,
                std::string doc_root,
                utl::Logger* logger);

  void run();

 private:
  void on_read(beast::error_code ec);
};

DetectSession::DetectSession(tcp::socket&& socket,
                             std::shared_ptr<TileGenerator> generator,
                             std::shared_ptr<TclEvaluator> tcl_eval,
                             std::shared_ptr<TimingReport> timing_report,
                             std::string doc_root,
                             utl::Logger* logger)
    : stream_(std::move(socket)),
      generator_(std::move(generator)),
      tcl_eval_(std::move(tcl_eval)),
      timing_report_(std::move(timing_report)),
      doc_root_(std::move(doc_root)),
      logger_(logger)
{
}

void DetectSession::run()
{
  http::async_read(
      stream_,
      buffer_,
      req_,
      [self = shared_from_this()](beast::error_code ec, std::size_t) {
        self->on_read(ec);
      });
}

void DetectSession::on_read(beast::error_code ec)
{
  if (ec) {
    debugPrint(
        logger_, utl::WEB, "http", 1, "detect read error: {}", ec.message());
    return;
  }

  if (websocket::is_upgrade(req_)) {
    // WebSocket upgrade - hand off to WebSocketSession
    auto ws = std::make_shared<WebSocketSession>(
        stream_.release_socket(),
        generator_,
        tcl_eval_,
        timing_report_,
        logger_);
    ws->run(std::move(req_));
  } else {
    // Regular HTTP - hand off to session with already-read request
    auto s = std::make_shared<HttpSession>(
        stream_.release_socket(), generator_, doc_root_, logger_);
    s->run_with_request(std::move(req_), std::move(buffer_));
  }
}

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
  utl::Logger* logger_;

 public:
  Listener(net::io_context& ioc,
           tcp::endpoint endpoint,
           std::shared_ptr<TileGenerator> generator,
           std::shared_ptr<TclEvaluator> tcl_eval,
           std::shared_ptr<TimingReport> timing_report,
           std::string doc_root,
           utl::Logger* logger);

  void run() { do_accept(); }

 private:
  void do_accept();
  void on_accept(beast::error_code ec, tcp::socket socket);
};

Listener::Listener(net::io_context& ioc,
                   tcp::endpoint endpoint,
                   std::shared_ptr<TileGenerator> generator,
                   std::shared_ptr<TclEvaluator> tcl_eval,
                   std::shared_ptr<TimingReport> timing_report,
                   std::string doc_root,
                   utl::Logger* logger)
    : ioc_(ioc),
      acceptor_(ioc),
      generator_(generator),
      tcl_eval_(std::move(tcl_eval)),
      timing_report_(std::move(timing_report)),
      doc_root_(std::move(doc_root)),
      logger_(logger)
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

void Listener::do_accept()
{
  acceptor_.async_accept(
      ioc_,
      [self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
        self->on_accept(ec, std::move(socket));
      });
}

void Listener::on_accept(beast::error_code ec, tcp::socket socket)
{
  if (ec) {
    debugPrint(
        logger_, utl::WEB, "http", 1, "accept error: {}", ec.message());
  } else {
    // Route through DetectSession to handle both HTTP and WebSocket
    std::make_shared<DetectSession>(std::move(socket),
                                    generator_,
                                    tcl_eval_,
                                    timing_report_,
                                    doc_root_,
                                    logger_)
        ->run();
  }
  do_accept();
}

//------------------------------------------------------------------------------
// WebServer
//------------------------------------------------------------------------------

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

    std::make_shared<Listener>(ioc,
                                tcp::endpoint{address, port},
                                generator_,
                                tcl_eval,
                                timing_report,
                                doc_root,
                                logger_)
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

    logger_->info(utl::WEB, 5, "Server stopped.");
  } catch (std::exception const& e) {
    logger_->error(utl::WEB, 2, "Server error : {}", e.what());
  }
}

}  // namespace web
