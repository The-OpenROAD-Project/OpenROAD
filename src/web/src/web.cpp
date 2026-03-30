// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "web/web.h"

#include <netinet/in.h>

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/signal_set.hpp"
#include "boost/asio/strand.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/http.hpp"
#include "boost/beast/websocket.hpp"
#include "clock_tree_report.h"
#include "gui/heatMap.h"
#include "odb/db.h"
#include "request_handler.h"
#include "tcl.h"
#include "timing_report.h"
#include "utl/Logger.h"

namespace web {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static WebSocketRequest parse_web_socket_request(const std::string& msg)
{
  WebSocketRequest req;
  req.id = static_cast<uint32_t>(extract_int(msg, "id"));
  req.raw_json = msg;

  std::string type_str = extract_string(msg, "type");
  if (type_str == "tile") {
    req.type = WebSocketRequest::TILE;
    req.layer = extract_string(msg, "layer");
    req.z = extract_int(msg, "z");
    req.x = extract_int(msg, "x");
    req.y = extract_int(msg, "y");
    req.vis.parseFromJson(msg);
  } else if (type_str == "bounds") {
    req.type = WebSocketRequest::BOUNDS;
  } else if (type_str == "tech") {
    req.type = WebSocketRequest::TECH;
  } else if (type_str == "inspect") {
    req.type = WebSocketRequest::INSPECT;
    req.select_id = extract_int(msg, "select_id");
  } else if (type_str == "inspect_back") {
    req.type = WebSocketRequest::INSPECT_BACK;
  } else if (type_str == "hover") {
    req.type = WebSocketRequest::HOVER;
    req.select_id = extract_int(msg, "select_id");
  } else if (type_str == "tcl_eval") {
    req.type = WebSocketRequest::TCL_EVAL;
    req.tcl_cmd = extract_string(msg, "cmd");
  } else if (type_str == "tcl_complete") {
    req.type = WebSocketRequest::TCL_COMPLETE;
    req.tcl_complete_line = extract_string(msg, "line");
    req.tcl_complete_cursor_pos = extract_int_or(msg, "cursor_pos", -1);
  } else if (type_str == "timing_report") {
    req.type = WebSocketRequest::TIMING_REPORT;
    req.timing_is_setup = extract_int_or(msg, "is_setup", 1);
    req.timing_max_paths = extract_int_or(msg, "max_paths", 100);
    req.timing_slack_min = extract_float_or(
        msg, "slack_min", -std::numeric_limits<float>::max());
    req.timing_slack_max
        = extract_float_or(msg, "slack_max", std::numeric_limits<float>::max());
  } else if (type_str == "timing_highlight") {
    req.type = WebSocketRequest::TIMING_HIGHLIGHT;
    req.timing_path_index = extract_int_or(msg, "path_index", -1);
    req.timing_highlight_setup = extract_int_or(msg, "is_setup", 1);
    req.timing_pin_name = extract_string(msg, "pin_name");
  } else if (type_str == "clock_tree") {
    req.type = WebSocketRequest::CLOCK_TREE;
  } else if (type_str == "clock_tree_highlight") {
    req.type = WebSocketRequest::CLOCK_TREE_HIGHLIGHT;
    req.clock_tree_inst_name = extract_string(msg, "inst_name");
  } else if (type_str == "slack_histogram") {
    req.type = WebSocketRequest::SLACK_HISTOGRAM;
    req.histogram_is_setup = extract_int_or(msg, "is_setup", 1);
    req.histogram_path_group = extract_string(msg, "path_group");
    req.histogram_clock = extract_string(msg, "clock_name");
  } else if (type_str == "chart_filters") {
    req.type = WebSocketRequest::CHART_FILTERS;
  } else if (type_str == "module_hierarchy") {
    req.type = WebSocketRequest::MODULE_HIERARCHY;
  } else if (type_str == "set_module_colors") {
    req.type = WebSocketRequest::SET_MODULE_COLORS;
    req.vis.parseFromJson(msg);
  } else if (type_str == "set_focus_nets") {
    req.type = WebSocketRequest::SET_FOCUS_NETS;
    req.focus_action = extract_string(msg, "action");
    req.focus_net_name = extract_string(msg, "net_name");
  } else if (type_str == "set_route_guides") {
    req.type = WebSocketRequest::SET_ROUTE_GUIDES;
    req.route_guide_action = extract_string(msg, "action");
    req.route_guide_net_name = extract_string(msg, "net_name");
  } else if (type_str == "schematic_cone") {
    req.type = WebSocketRequest::SCHEMATIC_CONE;
    req.schematic_inst_name = extract_string(msg, "inst_name");
    req.schematic_fanin_depth = extract_int_or(msg, "fanin_depth", 1);
    req.schematic_fanout_depth = extract_int_or(msg, "fanout_depth", 1);
  } else if (type_str == "schematic_full") {
    req.type = WebSocketRequest::SCHEMATIC_FULL;
  } else if (type_str == "schematic_inspect") {
    req.type = WebSocketRequest::SCHEMATIC_INSPECT;
    req.schematic_inst_name = extract_string(msg, "inst_name");
  } else if (type_str == "select") {
    req.type = WebSocketRequest::SELECT;
    req.select_x = extract_int(msg, "dbu_x");
    req.select_y = extract_int(msg, "dbu_y");
    req.select_zoom = extract_int_or(msg, "zoom", 0);
    req.visible_layers = extract_string_array(msg, "visible_layers");
    req.vis.parseFromJson(msg);
  } else if (type_str == "snap") {
    req.type = WebSocketRequest::SNAP;
    req.snap_x = extract_int(msg, "dbu_x");
    req.snap_y = extract_int(msg, "dbu_y");
    req.snap_radius = extract_int(msg, "radius");
    req.snap_point_threshold = extract_int_or(msg, "point_threshold", 10);
    req.snap_horizontal = extract_int_or(msg, "horizontal", 1) != 0;
    req.snap_vertical = extract_int_or(msg, "vertical", 1) != 0;
    req.visible_layers = extract_string_array(msg, "visible_layers");
    req.vis.parseFromJson(msg);
  } else if (type_str == "heatmaps") {
    req.type = WebSocketRequest::HEATMAPS;
  } else if (type_str == "set_active_heatmap") {
    req.type = WebSocketRequest::SET_ACTIVE_HEATMAP;
    req.heatmap_name = extract_string(msg, "name");
  } else if (type_str == "set_heatmap") {
    req.type = WebSocketRequest::SET_HEATMAP;
    req.heatmap_name = extract_string(msg, "name");
    req.heatmap_option = extract_string(msg, "option");
    req.heatmap_string_value = extract_string(msg, "value");
  } else if (type_str == "heatmap_tile") {
    req.type = WebSocketRequest::HEATMAP_TILE;
    req.heatmap_name = extract_string(msg, "name");
    req.z = extract_int(msg, "z");
    req.x = extract_int(msg, "x");
    req.y = extract_int(msg, "y");
  } else if (type_str == "list_dir") {
    req.type = WebSocketRequest::LIST_DIR;
    req.dir_path = extract_string(msg, "path");
  } else {
    req.type = WebSocketRequest::UNKNOWN;
  }
  return req;
}

// Serialize a WebSocketResponse into the binary wire format:
//   [0..3] uint32_t id (big-endian)
//   [4]    uint8_t  type
//   [5..7] reserved
//   [8..]  payload
static std::vector<unsigned char> serialize_response(
    const WebSocketResponse& resp)
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

static http::response<http::string_body> handle_request(
    http::request<http::string_body>&& req,
    const TileGenerator& generator,
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
    WebSocketRequest websocket_req;
    websocket_req.type = WebSocketRequest::BOUNDS;
    WebSocketResponse websocket_resp
        = dispatch_request(websocket_req, generator);
    res.set(http::field::content_type, "application/json");
    res.body() = std::string(websocket_resp.payload.begin(),
                             websocket_resp.payload.end());
  } else if (req.method() == http::verb::get && req.target() == "/tech") {
    WebSocketRequest websocket_req;
    websocket_req.type = WebSocketRequest::TECH;
    WebSocketResponse websocket_resp
        = dispatch_request(websocket_req, generator);
    res.set(http::field::content_type, "application/json");
    res.body() = std::string(websocket_resp.payload.begin(),
                             websocket_resp.payload.end());
  } else if (req.method() == http::verb::get
             && std::regex_match(target_path, match_pieces, tile_regex)) {
    WebSocketRequest websocket_req;
    websocket_req.type = WebSocketRequest::TILE;
    websocket_req.layer = match_pieces[1].str();
    websocket_req.z = std::stoi(match_pieces[2].str());
    websocket_req.x = std::stoi(match_pieces[3].str());
    websocket_req.y = std::stoi(match_pieces[4].str());
    WebSocketResponse websocket_resp
        = dispatch_request(websocket_req, generator);

    res.set(http::field::content_type, "image/png");
    res.body() = std::string(websocket_resp.payload.begin(),
                             websocket_resp.payload.end());
    res.set(http::field::cache_control, "public, max-age=604800");
  } else if (req.method() == http::verb::get && !doc_root.empty()) {
    // Serve static files from doc_root
    std::string file_path = std::move(target_path);
    if (file_path == "/") {
      file_path = "/index.html";
    }
    // Reject paths with ".." to preventd irectory traversal
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
  websocket::stream<beast::tcp_stream> websocket_;
  beast::flat_buffer buffer_;
  utl::Logger* logger_;

  // Handler objects (transport-independent, testable)
  SessionState state_;
  SelectHandler select_handler_;
  TclHandler tcl_handler_;
  TimingHandler timing_handler_;
  ClockTreeHandler clock_tree_handler_;
  TileHandler tile_handler_;

  // Write serialization: strand + queue ensures one async_write at a time
  net::strand<net::any_io_executor> strand_;
  std::deque<std::vector<unsigned char>> write_queue_;
  bool writing_ = false;

  // Background search index initialization
  std::shared_ptr<TileGenerator> generator_;
  std::thread init_thread_;

 public:
  WebSocketSession(tcp::socket&& socket,
                   std::shared_ptr<TileGenerator> generator,
                   std::shared_ptr<TclEvaluator> tcl_eval,
                   std::shared_ptr<TimingReport> timing_report,
                   std::shared_ptr<ClockTreeReport> clock_report,
                   utl::Logger* logger);
  ~WebSocketSession();

  void run(http::request<http::string_body>&& req);

 private:
  void on_accept(beast::error_code ec);
  void do_read();
  void on_read(beast::error_code ec);
  void queue_response(const WebSocketResponse& resp);
  void do_write();
};

WebSocketSession::WebSocketSession(
    tcp::socket&& socket,
    // NOLINTBEGIN(performance-unnecessary-value-param)
    std::shared_ptr<TileGenerator> generator,
    std::shared_ptr<TclEvaluator> tcl_eval,
    // NOLINTEND(performance-unnecessary-value-param)
    std::shared_ptr<TimingReport> timing_report,
    std::shared_ptr<ClockTreeReport> clock_report,
    utl::Logger* logger)
    : websocket_(std::move(socket)),
      logger_(logger),
      select_handler_(generator, tcl_eval),
      tcl_handler_(tcl_eval),
      timing_handler_(generator, std::move(timing_report), tcl_eval),
      clock_tree_handler_(generator, std::move(clock_report), tcl_eval),
      tile_handler_(generator),
      strand_(net::make_strand(websocket_.get_executor())),
      generator_(std::move(generator))
{
  if (generator_->getBlock()) {
    tile_handler_.initializeHeatMaps(state_);
  }
}

WebSocketSession::~WebSocketSession()
{
  if (init_thread_.joinable()) {
    init_thread_.join();
  }
  std::lock_guard<std::mutex> lock(state_.heatmap_mutex);
  if (!state_.active_heatmap.empty()) {
    auto active = state_.heatmaps.find(state_.active_heatmap);
    if (active != state_.heatmaps.end()) {
      active->second->onHide();
    }
  }
}

void WebSocketSession::run(http::request<http::string_body>&& req)
{
  websocket_.set_option(
      websocket::stream_base::timeout::suggested(beast::role_type::server));
  websocket_.set_option(
      websocket::stream_base::decorator([](websocket::response_type& res) {
        res.set(http::field::server, "OpenROAD WebSocket Server");
      }));

  // Build search indices in the background; tiles render without shapes
  // until ready, then a "refresh" push notification triggers a redraw.
  init_thread_ = std::thread([self = shared_from_this()]() {
    self->generator_->eagerInit();
    // Only send refresh if there's actually a design to render.
    // Without this guard, eagerInit returns instantly when no block is
    // loaded and the push races with async_accept (Beast soft_mutex crash).
    if (!self->generator_->getBlock()) {
      return;
    }
    // Send server-push refresh notification (id=0)
    WebSocketResponse resp;
    resp.id = 0;
    resp.type = 0;  // JSON
    const std::string json = R"({"type":"refresh"})";
    resp.payload.assign(json.begin(), json.end());
    self->queue_response(resp);
  });

  websocket_.async_accept(req,
                          [self = shared_from_this()](beast::error_code ec) {
                            self->on_accept(ec);
                          });
}

void WebSocketSession::on_accept(beast::error_code ec)
{
  if (ec) {
    debugPrint(logger_,
               utl::WEB,
               "websocket",
               1,
               "websocket accept error: {}",
               ec.message());
    return;
  }

  do_read();
}

void WebSocketSession::do_read()
{
  websocket_.async_read(
      buffer_, [self = shared_from_this()](beast::error_code ec, std::size_t) {
        self->on_read(ec);
      });
}

void WebSocketSession::on_read(beast::error_code ec)
{
  if (ec) {
    if (ec != websocket::error::closed) {
      debugPrint(logger_,
                 utl::WEB,
                 "websocket",
                 1,
                 "websocket read error: {}",
                 ec.message());
    }
    return;
  }

  const std::string msg = beast::buffers_to_string(buffer_.data());
  buffer_.consume(buffer_.size());

  WebSocketRequest req = parse_web_socket_request(msg);
  auto self = shared_from_this();

  switch (req.type) {
    case WebSocketRequest::SELECT:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(
                      self->select_handler_.handleSelect(req, self->state_));
                });
      break;
    case WebSocketRequest::SNAP:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(self->select_handler_.handleSnap(req));
                });
      break;
    case WebSocketRequest::INSPECT:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(
                      self->select_handler_.handleInspect(req, self->state_));
                });
      break;
    case WebSocketRequest::INSPECT_BACK:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(self->select_handler_.handleInspectBack(
                      req, self->state_));
                });
      break;
    case WebSocketRequest::HOVER:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(
                      self->select_handler_.handleHover(req, self->state_));
                });
      break;
    case WebSocketRequest::SCHEMATIC_CONE:
      net::post(websocket_.get_executor(), [self, req]() {
        self->queue_response(self->select_handler_.handleSchematicCone(req));
      });
      break;
    case WebSocketRequest::SCHEMATIC_FULL:
      net::post(websocket_.get_executor(), [self, req]() {
        self->queue_response(self->select_handler_.handleSchematicFull(req));
      });
      break;
    case WebSocketRequest::SCHEMATIC_INSPECT:
      net::post(websocket_.get_executor(), [self, req]() {
        self->queue_response(
            self->select_handler_.handleSchematicInspect(req, self->state_));
      });
      break;
    case WebSocketRequest::TCL_EVAL:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(self->tcl_handler_.handleTclEval(req));
                });
      break;
    case WebSocketRequest::TCL_COMPLETE:
      net::post(
          websocket_.get_executor(),
          [self = std::move(self), req = std::move(req)]() {
            self->queue_response(self->tcl_handler_.handleTclComplete(req));
          });
      break;
    case WebSocketRequest::TIMING_REPORT:
      net::post(
          websocket_.get_executor(),
          [self = std::move(self), req = std::move(req)]() {
            self->queue_response(self->timing_handler_.handleTimingReport(req));
          });
      break;
    case WebSocketRequest::TIMING_HIGHLIGHT:
      net::post(
          websocket_.get_executor(),
          [self = std::move(self), req = std::move(req)]() {
            self->queue_response(
                self->timing_handler_.handleTimingHighlight(req, self->state_));
          });
      break;
    case WebSocketRequest::CLOCK_TREE:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(
                      self->clock_tree_handler_.handleClockTree(req));
                });
      break;
    case WebSocketRequest::CLOCK_TREE_HIGHLIGHT:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(
                      self->clock_tree_handler_.handleClockTreeHighlight(
                          req, self->state_));
                });
      break;
    case WebSocketRequest::SLACK_HISTOGRAM:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(
                      self->timing_handler_.handleSlackHistogram(req));
                });
      break;
    case WebSocketRequest::CHART_FILTERS:
      net::post(
          websocket_.get_executor(),
          [self = std::move(self), req = std::move(req)]() {
            self->queue_response(self->timing_handler_.handleChartFilters(req));
          });
      break;
    case WebSocketRequest::MODULE_HIERARCHY:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(
                      self->tile_handler_.handleModuleHierarchy(req));
                });
      break;
    case WebSocketRequest::SET_MODULE_COLORS:
      net::post(
          websocket_.get_executor(),
          [self = std::move(self), req = std::move(req)]() {
            self->queue_response(
                self->tile_handler_.handleSetModuleColors(req, self->state_));
          });
      break;
    case WebSocketRequest::SET_FOCUS_NETS:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(self->select_handler_.handleSetFocusNets(
                      req, self->state_));
                });
      break;
    case WebSocketRequest::SET_ROUTE_GUIDES:
      net::post(
          websocket_.get_executor(),
          [self = std::move(self), req = std::move(req)]() {
            self->queue_response(
                self->select_handler_.handleSetRouteGuides(req, self->state_));
          });
      break;
    case WebSocketRequest::HEATMAPS:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(
                      self->tile_handler_.handleHeatMaps(req, self->state_));
                });
      break;
    case WebSocketRequest::SET_ACTIVE_HEATMAP:
      net::post(
          websocket_.get_executor(),
          [self = std::move(self), req = std::move(req)]() {
            self->queue_response(
                self->tile_handler_.handleSetActiveHeatMap(req, self->state_));
          });
      break;
    case WebSocketRequest::SET_HEATMAP:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(
                      self->tile_handler_.handleSetHeatMap(req, self->state_));
                });
      break;
    case WebSocketRequest::HEATMAP_TILE:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(
                      self->tile_handler_.handleHeatMapTile(req, self->state_));
                });
      break;
    case WebSocketRequest::LIST_DIR:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(handleListDir(req));
                });
      break;
    default:
      net::post(websocket_.get_executor(),
                [self = std::move(self), req = std::move(req)]() {
                  self->queue_response(
                      self->tile_handler_.handleTile(req, self->state_));
                });
      break;
  }

  do_read();
}

void WebSocketSession::queue_response(const WebSocketResponse& resp)
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
  websocket_.binary(true);
  websocket_.async_write(
      net::buffer(write_queue_.front()),
      [self = shared_from_this()](beast::error_code ec, std::size_t) {
        net::post(self->strand_, [self, ec]() {
          if (ec) {
            debugPrint(self->logger_,
                       utl::WEB,
                       "websocket",
                       1,
                       "websocket write error: {}",
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
      generator_(std::move(generator)),
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
    do_close();
    return;
  }
  if (ec) {
    debugPrint(
        logger_, utl::WEB, "http", 1, "http read error: {}", ec.message());
    return;
  }

  res_ = std::make_shared<http::response<http::string_body>>(
      handle_request(std::move(req_), *generator_, doc_root_));
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
  std::shared_ptr<ClockTreeReport> clock_report_;
  http::request<http::string_body> req_;
  std::string doc_root_;
  utl::Logger* logger_;

 public:
  DetectSession(tcp::socket&& socket,
                std::shared_ptr<TileGenerator> generator,
                std::shared_ptr<TclEvaluator> tcl_eval,
                std::shared_ptr<TimingReport> timing_report,
                std::shared_ptr<ClockTreeReport> clock_report,
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
                             std::shared_ptr<ClockTreeReport> clock_report,
                             std::string doc_root,
                             utl::Logger* logger)
    : stream_(std::move(socket)),
      generator_(std::move(generator)),
      tcl_eval_(std::move(tcl_eval)),
      timing_report_(std::move(timing_report)),
      clock_report_(std::move(clock_report)),
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
    auto websocket_session
        = std::make_shared<WebSocketSession>(stream_.release_socket(),
                                             generator_,
                                             tcl_eval_,
                                             timing_report_,
                                             clock_report_,
                                             logger_);
    websocket_session->run(std::move(req_));
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
  std::shared_ptr<ClockTreeReport> clock_report_;
  std::string doc_root_;
  utl::Logger* logger_;

 public:
  Listener(net::io_context& ioc,
           const tcp::endpoint& endpoint,
           std::shared_ptr<TileGenerator> generator,
           std::shared_ptr<TclEvaluator> tcl_eval,
           std::shared_ptr<TimingReport> timing_report,
           std::shared_ptr<ClockTreeReport> clock_report,
           std::string doc_root,
           utl::Logger* logger);

  void run() { do_accept(); }

 private:
  void do_accept();
  void on_accept(beast::error_code ec, tcp::socket socket);
};

Listener::Listener(net::io_context& ioc,
                   const tcp::endpoint& endpoint,
                   std::shared_ptr<TileGenerator> generator,
                   std::shared_ptr<TclEvaluator> tcl_eval,
                   std::shared_ptr<TimingReport> timing_report,
                   std::shared_ptr<ClockTreeReport> clock_report,
                   std::string doc_root,
                   utl::Logger* logger)
    : ioc_(ioc),
      acceptor_(ioc),
      generator_(std::move(generator)),
      tcl_eval_(std::move(tcl_eval)),
      timing_report_(std::move(timing_report)),
      clock_report_(std::move(clock_report)),
      doc_root_(std::move(doc_root)),
      logger_(logger)
{
  beast::error_code ec;

  acceptor_.open(endpoint.protocol(), ec);
  if (ec) {
    logger_->error(utl::WEB, 10, "Failed to open acceptor: {}", ec.message());
  }

  acceptor_.set_option(net::socket_base::reuse_address(true), ec);
  if (ec) {
    logger_->error(
        utl::WEB, 11, "Failed to set reuse_address option: {}", ec.message());
  }

  acceptor_.bind(endpoint, ec);
  if (ec) {
    logger_->error(
        utl::WEB, 17, "Failed to bind to endpoint: {}", ec.message());
  }

  acceptor_.listen(net::socket_base::max_listen_connections, ec);
  if (ec) {
    logger_->error(utl::WEB, 18, "Failed to listen: {}", ec.message());
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
    debugPrint(logger_, utl::WEB, "http", 1, "accept error: {}", ec.message());
    if (ec == net::error::operation_aborted || !acceptor_.is_open()) {
      return;
    }
  } else {
    // Route through DetectSession to handle both HTTP and WebSocket
    std::make_shared<DetectSession>(std::move(socket),
                                    generator_,
                                    tcl_eval_,
                                    timing_report_,
                                    clock_report_,
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

void WebServer::saveImage(const std::string& filename,
                          const int x0,
                          const int y0,
                          const int x1,
                          const int y1,
                          const int width_px,
                          const double dbu_per_pixel,
                          const std::string& vis_json)
{
  // Create generator on demand (server may not be running).
  if (!generator_) {
    generator_ = std::make_shared<TileGenerator>(db_, sta_, logger_);
  }
  generator_->eagerInit();

  const odb::Rect region(x0, y0, x1, y1);
  TileVisibility vis;
  if (!vis_json.empty()) {
    vis.parseFromJson(vis_json);
  }
  generator_->saveImage(filename, region, width_px, dbu_per_pixel, vis);
}

void WebServer::serve(int port, const std::string& doc_root)
{
  try {
    generator_ = std::make_shared<TileGenerator>(db_, sta_, logger_);
    auto timing_report = std::make_shared<TimingReport>(sta_);
    auto clock_report = std::make_shared<ClockTreeReport>(sta_);

    // Create Tcl evaluator with logger sink for output capture
    auto tcl_eval = std::make_shared<TclEvaluator>(interp_, logger_);

    auto const address = net::ip::make_address("127.0.0.1");
    uint16_t const u_port = port;
    int const num_threads = 32;

    if (!doc_root.empty()) {
      logger_->info(utl::WEB, 4, "Serving static files from {}", doc_root);
    }

    const std::string url = "http://localhost:" + std::to_string(port);
    logger_->info(utl::WEB,
                  1,
                  "Server starting on {} with {} threads...",
                  url,
                  num_threads);

#if defined(__APPLE__)
    std::string cmd = "open " + url + " > /dev/null 2>&1";
#elif defined(_WIN32)
    std::string cmd = "start " + url + " > nul 2>&1";
#else
    std::string cmd = "xdg-open " + url + " > /dev/null 2>&1 &";
#endif
    int ret = std::system(cmd.c_str());
    (void) ret;

    net::io_context ioc{num_threads};

    std::make_shared<Listener>(ioc,
                               tcp::endpoint{address, u_port},
                               generator_,
                               tcl_eval,
                               timing_report,
                               clock_report,
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
