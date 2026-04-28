// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "web/web.h"

#include <netinet/in.h>

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <ios>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/strand.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/http.hpp"
#include "boost/beast/websocket.hpp"
#include "clock_tree_report.h"
#include "gui/heatMap.h"
#include "hierarchy_report.h"
#include "json_builder.h"
#include "odb/db.h"
#include "request_dispatcher.h"
#include "request_handler.h"
#include "tcl.h"
#include "tile_generator.h"
#include "timing_report.h"
#include "utl/Logger.h"
#include "web_assets.h"
#include "web_chart.h"
#include "web_viewer_hook.h"

namespace web {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using Tcp = net::ip::tcp;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

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
// HTTP request handler (serves embedded static assets)
//------------------------------------------------------------------------------

static http::response<http::string_body> handle_request(
    http::request<http::string_body>&& req)
{
  http::response<http::string_body> res{http::status::ok, req.version()};
  res.set(http::field::server, "Boost.Beast Server (C++17)");
  res.set(http::field::content_type, "text/plain");
  res.keep_alive(req.keep_alive());
  res.set(http::field::access_control_allow_origin, "*");

  if (req.method() == http::verb::get) {
    std::string file_path(req.target());
    if (file_path == "/") {
      file_path = "/index.html";
    }
    const auto* asset = findEmbeddedAsset(file_path);
    if (asset) {
      res.set(http::field::content_type, asset->content_type);
      res.body() = std::string(asset->content());
    } else {
      res.result(http::status::not_found);
      res.body() = "Resource not found.";
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
  DRCHandler drc_handler_;

  // Registration-based request dispatcher (replaces parse/dispatch switches)
  RequestDispatcher dispatcher_;

  // Write serialization: strand + queue ensures one async_write at a time
  struct PendingWrite
  {
    std::vector<unsigned char> frame;
    std::function<void()> on_complete;
  };
  net::strand<net::any_io_executor> strand_;
  std::deque<PendingWrite> write_queue_;
  bool writing_ = false;

  // Background search index initialization
  std::shared_ptr<TileGenerator> generator_;
  std::thread init_thread_;

  // Debug-graphics hook (nullable).  When set, this session registers a
  // send callback for server-push broadcasts (pause/continue notifications).
  WebViewerHook* viewer_hook_ = nullptr;
  std::size_t viewer_token_ = 0;

 public:
  WebSocketSession(Tcp::socket&& socket,
                   std::shared_ptr<TileGenerator> generator,
                   std::shared_ptr<TclEvaluator> tcl_eval,
                   std::shared_ptr<TimingReport> timing_report,
                   std::shared_ptr<ClockTreeReport> clock_report,
                   utl::Logger* logger,
                   WebViewerHook* viewer_hook);
  ~WebSocketSession();

  void run(http::request<http::string_body>&& req);

 private:
  void on_accept(beast::error_code ec);
  void do_read();
  void on_read(beast::error_code ec);
  void queue_response(const WebSocketResponse& resp,
                      std::function<void()> on_complete = {});
  void do_write();
};

WebSocketSession::WebSocketSession(
    Tcp::socket&& socket,
    // NOLINTBEGIN(performance-unnecessary-value-param)
    std::shared_ptr<TileGenerator> generator,
    std::shared_ptr<TclEvaluator> tcl_eval,
    // NOLINTEND(performance-unnecessary-value-param)
    std::shared_ptr<TimingReport> timing_report,
    std::shared_ptr<ClockTreeReport> clock_report,
    utl::Logger* logger,
    WebViewerHook* viewer_hook)
    : websocket_(std::move(socket)),
      logger_(logger),
      select_handler_(generator, tcl_eval),
      tcl_handler_(tcl_eval),
      timing_handler_(generator, std::move(timing_report), tcl_eval),
      clock_tree_handler_(generator, std::move(clock_report), tcl_eval),
      tile_handler_(generator),
      drc_handler_(generator),
      strand_(net::make_strand(websocket_.get_executor())),
      generator_(std::move(generator)),
      viewer_hook_(viewer_hook)
{
  if (generator_->getBlock()) {
    tile_handler_.initializeHeatMaps(state_);
  }

  // Register all handler request types with the dispatcher.
  select_handler_.registerRequests(dispatcher_);
  tcl_handler_.registerRequests(dispatcher_);
  timing_handler_.registerRequests(dispatcher_);
  clock_tree_handler_.registerRequests(dispatcher_);
  tile_handler_.registerRequests(dispatcher_);
  drc_handler_.registerRequests(dispatcher_);

  // Free function handler
  dispatcher_.add("list_dir",
                  WebSocketRequest::kListDir,
                  [](const WebSocketRequest& req, SessionState&) {
                    return handleListDir(req);
                  });

  // Session-specific debug handlers (need viewer_hook_, run inline)
  dispatcher_.add(
      "debug_continue",
      WebSocketRequest::kDebugContinue,
      [this](const WebSocketRequest& req, SessionState&) -> WebSocketResponse {
        if (viewer_hook_ != nullptr) {
          viewer_hook_->continueExecution();
        }
        WebSocketResponse resp;
        resp.id = req.id;
        resp.type = WebSocketResponse::kJson;
        const std::string json = R"({"ok":1})";
        resp.payload.assign(json.begin(), json.end());
        return resp;
      },
      /*run_inline=*/true);

  dispatcher_.add(
      "debug_charts",
      WebSocketRequest::kDebugCharts,
      [this](const WebSocketRequest& req, SessionState&) -> WebSocketResponse {
        WebSocketResponse resp;
        resp.id = req.id;
        resp.type = WebSocketResponse::kJson;
        JsonBuilder builder;
        builder.beginObject();
        builder.beginArray("charts");
        if (viewer_hook_ != nullptr) {
          for (WebChart* chart : viewer_hook_->charts()) {
            builder.beginObject();
            builder.field("name", chart->name());
            builder.field("x_label", chart->xLabel());
            builder.beginArray("y_labels");
            for (const auto& lbl : chart->yLabels()) {
              builder.value(lbl);
            }
            builder.endArray();
            builder.field("x_format", chart->xAxisFormat());
            builder.beginArray("y_formats");
            for (const auto& f : chart->yAxisFormats()) {
              builder.value(f);
            }
            builder.endArray();
            builder.beginArray("points");
            for (const auto& pt : chart->points()) {
              builder.beginObject();
              builder.field("x", pt.x);
              builder.beginArray("ys");
              for (double v : pt.ys) {
                builder.value(v);
              }
              builder.endArray();
              builder.endObject();
            }
            builder.endArray();
            builder.endObject();
          }
        }
        builder.endArray();
        builder.endObject();
        const std::string& json = builder.str();
        resp.payload.assign(json.begin(), json.end());
        return resp;
      },
      /*run_inline=*/true);
}

WebSocketSession::~WebSocketSession()
{
  if (viewer_hook_ != nullptr && viewer_token_ != 0) {
    viewer_hook_->sessions().remove(viewer_token_);
  }
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

  // Register this session with the viewer hook so debug_paused /
  // debug_refresh / debug_resumed push messages reach the client.  The
  // lambda captures a weak_ptr so we never keep the session alive on the
  // registry's behalf.  This must happen AFTER accept completes — writing
  // before the handshake finishes sends masked (client-role) frames that
  // browsers reject with "A server must not mask any frames".
  if (viewer_hook_ != nullptr) {
    auto weak_self = std::weak_ptr<WebSocketSession>(shared_from_this());
    viewer_token_ = viewer_hook_->sessions().add(
        // SendFn — queue a JSON push message on this session's write queue.
        [weak_self](const std::string& json) {
          auto self = weak_self.lock();
          if (!self) {
            return;
          }
          WebSocketResponse resp;
          resp.id = 0;
          resp.type = WebSocketResponse::kJson;
          resp.payload.assign(json.begin(), json.end());
          self->queue_response(resp);
        },
        // SendAndWaitFn — queue a JSON push message and invoke the callback
        // after async_write completes.
        [weak_self](const std::string& json, std::function<void()> fn) {
          auto self = weak_self.lock();
          if (!self) {
            fn();  // session gone — signal fence immediately
            return;
          }
          WebSocketResponse resp;
          resp.id = 0;
          resp.type = WebSocketResponse::kJson;
          resp.payload.assign(json.begin(), json.end());
          self->queue_response(resp, std::move(fn));
        });

    // Flush any log output that accumulated before this client
    // connected (splash screen, script output, etc.).
    viewer_hook_->drainLogs();
  }

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
    resp.type = WebSocketResponse::kJson;
    const std::string json = R"({"type":"refresh"})";
    resp.payload.assign(json.begin(), json.end());
    self->queue_response(resp);
  });

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

  WebSocketRequest req = dispatcher_.parse(msg);
  auto self = shared_from_this();

  const auto* entry = dispatcher_.find(req.type);
  if (entry != nullptr) {
    if (entry->run_inline) {
      queue_response(entry->handle(req, state_));
    } else {
      auto handle = entry->handle;
      net::post(websocket_.get_executor(),
                [self = std::move(self),
                 req = std::move(req),
                 handle = std::move(handle)]() {
                  self->queue_response(handle(req, self->state_));
                });
    }
  } else {
    // Unknown type -- return an error so the client knows the request
    // was not understood (e.g. typo or client/server version mismatch).
    WebSocketResponse resp;
    resp.id = req.id;
    resp.type = WebSocketResponse::kError;
    const std::string err = "Unknown request type";
    resp.payload.assign(err.begin(), err.end());
    queue_response(resp);
  }

  do_read();
}

void WebSocketSession::queue_response(const WebSocketResponse& resp,
                                      std::function<void()> on_complete)
{
  std::vector<unsigned char> frame = serialize_response(resp);

  // Post to the strand to serialize write queue access
  net::post(
      strand_,
      [self = shared_from_this(),
       frame = std::move(frame),
       on_complete = std::move(on_complete)]() mutable {
        self->write_queue_.push_back(PendingWrite{
            .frame = std::move(frame), .on_complete = std::move(on_complete)});
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
      net::buffer(write_queue_.front().frame),
      [self = shared_from_this()](beast::error_code ec, std::size_t) {
        net::post(self->strand_, [self, ec]() {
          auto on_complete = std::move(self->write_queue_.front().on_complete);
          self->write_queue_.pop_front();
          if (on_complete) {
            on_complete();
          }
          if (ec) {
            debugPrint(self->logger_,
                       utl::WEB,
                       "websocket",
                       1,
                       "websocket write error: {}",
                       ec.message());
            self->writing_ = false;
            return;
          }
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
  std::shared_ptr<http::response<http::string_body>> res_;
  http::request<http::string_body> req_;
  utl::Logger* logger_;

 public:
  HttpSession(Tcp::socket&& socket, utl::Logger* logger);

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

HttpSession::HttpSession(Tcp::socket&& socket, utl::Logger* logger)
    : stream_(std::move(socket)), logger_(logger)
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
      handle_request(std::move(req_)));
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
  stream_.socket().shutdown(Tcp::socket::shutdown_send, ec);
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
  utl::Logger* logger_;
  WebViewerHook* viewer_hook_ = nullptr;

 public:
  DetectSession(Tcp::socket&& socket,
                std::shared_ptr<TileGenerator> generator,
                std::shared_ptr<TclEvaluator> tcl_eval,
                std::shared_ptr<TimingReport> timing_report,
                std::shared_ptr<ClockTreeReport> clock_report,
                utl::Logger* logger,
                WebViewerHook* viewer_hook);

  void run();

 private:
  void on_read(beast::error_code ec);
};

DetectSession::DetectSession(Tcp::socket&& socket,
                             std::shared_ptr<TileGenerator> generator,
                             std::shared_ptr<TclEvaluator> tcl_eval,
                             std::shared_ptr<TimingReport> timing_report,
                             std::shared_ptr<ClockTreeReport> clock_report,
                             utl::Logger* logger,
                             WebViewerHook* viewer_hook)
    : stream_(std::move(socket)),
      generator_(std::move(generator)),
      tcl_eval_(std::move(tcl_eval)),
      timing_report_(std::move(timing_report)),
      clock_report_(std::move(clock_report)),
      logger_(logger),
      viewer_hook_(viewer_hook)
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
                                             logger_,
                                             viewer_hook_);
    websocket_session->run(std::move(req_));
  } else {
    // Regular HTTP - hand off to session with already-read request
    auto s = std::make_shared<HttpSession>(stream_.release_socket(), logger_);
    s->run_with_request(std::move(req_), std::move(buffer_));
  }
}

//------------------------------------------------------------------------------
// Listener - accepts incoming connections
//------------------------------------------------------------------------------

class Listener : public std::enable_shared_from_this<Listener>
{
  net::io_context& ioc_;
  Tcp::acceptor acceptor_;
  std::shared_ptr<TileGenerator> generator_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
  std::shared_ptr<TimingReport> timing_report_;
  std::shared_ptr<ClockTreeReport> clock_report_;
  utl::Logger* logger_;
  WebViewerHook* viewer_hook_ = nullptr;

 public:
  Listener(net::io_context& ioc,
           const Tcp::endpoint& endpoint,
           std::shared_ptr<TileGenerator> generator,
           std::shared_ptr<TclEvaluator> tcl_eval,
           std::shared_ptr<TimingReport> timing_report,
           std::shared_ptr<ClockTreeReport> clock_report,
           utl::Logger* logger,
           WebViewerHook* viewer_hook);

  void run() { do_accept(); }

  // The actual port the acceptor bound to (useful when port 0 was
  // requested and the OS assigned a free port).
  uint16_t port() const { return acceptor_.local_endpoint().port(); }

  // Close the acceptor so its destructor doesn't touch a dying
  // io_context.  Called from WebServer::stop() before ioc_.reset().
  void close()
  {
    beast::error_code ec;
    acceptor_.close(ec);
  }

 private:
  void do_accept();
  void on_accept(beast::error_code ec, Tcp::socket socket);
};

Listener::Listener(net::io_context& ioc,
                   const Tcp::endpoint& endpoint,
                   std::shared_ptr<TileGenerator> generator,
                   std::shared_ptr<TclEvaluator> tcl_eval,
                   std::shared_ptr<TimingReport> timing_report,
                   std::shared_ptr<ClockTreeReport> clock_report,
                   utl::Logger* logger,
                   WebViewerHook* viewer_hook)
    : ioc_(ioc),
      acceptor_(ioc),
      generator_(std::move(generator)),
      tcl_eval_(std::move(tcl_eval)),
      timing_report_(std::move(timing_report)),
      clock_report_(std::move(clock_report)),
      logger_(logger),
      viewer_hook_(viewer_hook)
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
      [self = shared_from_this()](beast::error_code ec, Tcp::socket socket) {
        self->on_accept(ec, std::move(socket));
      });
}

void Listener::on_accept(beast::error_code ec, Tcp::socket socket)
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
                                    logger_,
                                    viewer_hook_)
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
                     Tcl_Interp* interp,
                     int num_threads)
    : db_(db),
      sta_(sta),
      logger_(logger),
      interp_(interp),
      num_threads_(num_threads)
{
}

WebServer::~WebServer()
{
  // Wake any thread blocked in waitForStop() so it can return before
  // we tear down the io_context.
  {
    std::lock_guard<std::mutex> lock(stop_mutex_);
    stop_requested_ = true;
  }
  stop_cv_.notify_one();

  // The destructor fires during Tcl_Exit → atexit → ~OpenRoad chain.
  // By this point the Tcl interpreter is partially torn down and static
  // objects may be destroyed.  Calling stop() (which joins 32 threads
  // and tears down boost::asio's reactor) triggers SIGSEGV because the
  // reactor's internal state references destroyed statics.
  //
  // Since the destructor only runs at process exit, the OS reclaims all
  // memory and closes all sockets.  We just need to stop the threads so
  // the process can actually exit:
  if (ioc_) {
    ioc_->stop();
  }
  for (auto& t : threads_) {
    if (t.joinable()) {
      t.join();
    }
  }
  threads_.clear();
  // Close the Listener's acceptor and release the shared_ptr to it
  // before the io_context goes away.
  if (shutdown_listener_) {
    shutdown_listener_();
    shutdown_listener_ = {};
  }
  // Release without destroying — ~io_context() crashes because
  // reactor::shutdown() destroys pending async operations whose
  // handlers reference the dying reactor.  The OS reclaims at exit.
  (void) ioc_.release();  // NOLINT(bugprone-unused-return-value)
  // Also leak viewer_hook_ — it may be referenced by Gui's
  // headless_viewer_ pointer which outlives us (static singleton).
  (void) viewer_hook_.release();  // NOLINT(bugprone-unused-return-value)
}

// Embedded JS/CSS for standalone timing report (generated at build time
// by embed_report_assets.py → report_assets.cpp).
extern const std::string_view kReportCSS;
extern const std::string_view kReportJS;

static std::string serializeToJson(auto serialize_fn)
{
  JsonBuilder b;
  serialize_fn(b);
  return b.str();
}

static std::string base64Encode(const std::vector<unsigned char>& data)
{
  static const char kChars[]
      = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string result;
  result.reserve((data.size() + 2) / 3 * 4);
  for (size_t i = 0; i < data.size(); i += 3) {
    const unsigned b0 = data[i];
    const unsigned b1 = (i + 1 < data.size()) ? data[i + 1] : 0;
    const unsigned b2 = (i + 2 < data.size()) ? data[i + 2] : 0;
    result += kChars[b0 >> 2];
    result += kChars[((b0 & 3) << 4) | (b1 >> 4)];
    result
        += (i + 1 < data.size()) ? kChars[((b1 & 0xF) << 2) | (b2 >> 6)] : '=';
    result += (i + 2 < data.size()) ? kChars[b2 & 0x3F] : '=';
  }
  return result;
}

void WebServer::saveReport(const std::string& filename,
                           const int max_setup,
                           const int max_hold)
{
  // Create/init the tile generator.
  if (!generator_) {
    generator_ = std::make_shared<TileGenerator>(db_, sta_, logger_);
  }
  generator_->eagerInit();

  odb::dbBlock* block = generator_->getBlock();
  if (!block) {
    logger_->error(utl::WEB, 35, "No design loaded.");
    return;
  }

  std::ofstream out(filename);
  if (!out) {
    logger_->error(utl::WEB, 31, "Cannot open file: {}", filename);
    return;
  }

  // ── Serialize JSON cache responses ──

  std::string setup_json, hold_json, hist_setup, hist_hold, filters;
  std::vector<TimingPathSummary> setup_paths, hold_paths;
  if (sta_) {
    TimingReport report(sta_);
    setup_paths = report.getReport(true, max_setup);
    hold_paths = report.getReport(false, max_hold);
    setup_json = serializeToJson(
        [&](JsonBuilder& b) { serializeTimingPaths(b, setup_paths); });
    hold_json = serializeToJson(
        [&](JsonBuilder& b) { serializeTimingPaths(b, hold_paths); });
    hist_setup = serializeToJson([&](JsonBuilder& b) {
      serializeSlackHistogram(b, report.getSlackHistogram(true));
    });
    hist_hold = serializeToJson([&](JsonBuilder& b) {
      serializeSlackHistogram(b, report.getSlackHistogram(false));
    });
    filters = serializeToJson([&](JsonBuilder& b) {
      serializeChartFilters(b, report.getChartFilters());
    });
  } else {
    logger_->warn(utl::WEB, 30, "No STA data — timing sections will be empty.");
    setup_json
        = serializeToJson([](JsonBuilder& b) { serializeTimingPaths(b, {}); });
    hold_json = setup_json;
    hist_setup = serializeToJson(
        [](JsonBuilder& b) { serializeSlackHistogram(b, {}); });
    hist_hold = hist_setup;
    filters
        = serializeToJson([](JsonBuilder& b) { serializeChartFilters(b, {}); });
  }
  const std::string tech_json = serializeToJson(
      [&](JsonBuilder& b) { serializeTechResponse(b, *generator_); });
  const std::string bounds_json = serializeToJson(
      [&](JsonBuilder& b) { serializeBoundsResponse(b, *generator_, true); });
  const auto tech_layers = generator_->getLayers();

  // ── Serialize module hierarchy ──

  HierarchyReport hier_report(block, sta_);
  auto hier_result = hier_report.getReport();

  const std::string hierarchy_json = serializeToJson(
      [&](JsonBuilder& b) { serializeHierarchyResult(b, hier_result); });

  auto module_colors = computeDefaultModuleColors(hier_result);
  const std::map<uint32_t, Color>* mod_colors_ptr
      = module_colors.empty() ? nullptr : &module_colors;

  // ── Render tiles at a fixed zoom level ──

  // Pick z so the design fits in a typical panel (~500px).
  // In Leaflet CRS.Simple, the design spans 256 units = 256*2^z pixels.
  // z=1 → 512px, a good fit for most panel sizes.
  constexpr int kZ = 1;
  const int num_tiles = 1 << kZ;

  TileVisibility vis;
  // A 256x256 fully-transparent RGBA PNG is exactly 102 bytes with lodepng.
  // Any tile with visible content will be larger.
  constexpr size_t kEmptyPngSize = 102;

  // All layers to cache tiles for.
  std::vector<std::string> all_layers;
  all_layers.emplace_back("_instances");
  for (const auto& name : tech_layers) {
    all_layers.push_back(name);
  }
  all_layers.emplace_back("_modules");
  all_layers.emplace_back("_pins");

  // Collect non-empty tiles as "layer/z/x/y" -> base64.
  std::vector<std::pair<std::string, std::string>> tile_entries;
  for (const auto& layer : all_layers) {
    for (int ty = 0; ty < num_tiles; ++ty) {
      for (int tx = 0; tx < num_tiles; ++tx) {
        auto png = generator_->generateTile(
            layer, kZ, tx, ty, vis, {}, {}, {}, {}, mod_colors_ptr);
        if (png.size() > kEmptyPngSize) {
          std::string key = layer + "/" + std::to_string(kZ) + "/"
                            + std::to_string(tx) + "/" + std::to_string(ty);
          tile_entries.emplace_back(std::move(key), base64Encode(png));
        }
      }
    }
  }

  logger_->info(
      utl::WEB, 33, "Cached {} tiles at zoom {}.", tile_entries.size(), kZ);

  // ── Render per-path overlay images ──

  auto render_path_overlays = [&](const std::vector<TimingPathSummary>& paths) {
    std::vector<std::string> overlays;
    for (const auto& path : paths) {
      std::vector<ColoredRect> rects;
      std::vector<FlightLine> lines;
      collectTimingPathShapes(block, path, rects, lines);
      const int overlay_px = 256 * (1 << kZ);
      auto png = generator_->renderOverlayPng(overlay_px, rects, lines);
      if (png.size() > kEmptyPngSize) {
        overlays.push_back(base64Encode(png));
      } else {
        overlays.emplace_back();
      }
    }
    return overlays;
  };
  const auto setup_overlays = render_path_overlays(setup_paths);
  const auto hold_overlays = render_path_overlays(hold_paths);

  logger_->info(utl::WEB,
                34,
                "Rendered {} setup + {} hold path overlays.",
                setup_overlays.size(),
                hold_overlays.size());

  // ── Write the HTML ──

  // HTML head — same CDN deps as index.html.
  out << R"(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>OpenROAD Timing Report</title>
<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"/>
<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/golden-layout@2.6.0/dist/css/goldenlayout-base.css"/>
<link rel="stylesheet" id="gl-theme-dark" href="https://cdn.jsdelivr.net/npm/golden-layout@2.6.0/dist/css/themes/goldenlayout-dark-theme.css"/>
<link rel="stylesheet" id="gl-theme-light" href="https://cdn.jsdelivr.net/npm/golden-layout@2.6.0/dist/css/themes/goldenlayout-light-theme.css" disabled/>
<style>
)" << kReportCSS
      << R"(
</style>
</head>
<body>
<div id="menu-bar"></div>
<div id="gl-container"></div>
<div id="websocket-status"></div>
<div id="loading-overlay" style="display:none">
  <div class="loading-overlay-content">
    <div class="spinner"></div>
    <span>Loading shapes…</span>
  </div>
</div>
<script>
window.__STATIC_CACHE__ = {
  zoom: )"
      << kZ << R"(,
  json: {
    "tech": )"
      << tech_json << R"(,
    "bounds": )"
      << bounds_json << R"(,
    "heatmaps": {"active":"","heatmaps":[]},
    "timing_report:setup": )"
      << setup_json << R"(,
    "timing_report:hold": )"
      << hold_json << R"(,
    "slack_histogram:setup": )"
      << hist_setup << R"(,
    "slack_histogram:hold": )"
      << hist_hold << R"(,
    "chart_filters": )"
      << filters << R"(,
    "module_hierarchy": )"
      << hierarchy_json << R"(
  },
  tiles: {)";

  // Emit tile entries.
  for (size_t i = 0; i < tile_entries.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << "\n    \"" << json_escape(tile_entries[i].first) << "\":\""
        << tile_entries[i].second << "\"";
  }

  out << R"(
  },
  overlays: {
    setup: [)";
  for (size_t i = 0; i < setup_overlays.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    if (setup_overlays[i].empty()) {
      out << "null";
    } else {
      out << "\"" << setup_overlays[i] << "\"";
    }
  }
  out << R"(],
    hold: [)";
  for (size_t i = 0; i < hold_overlays.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    if (hold_overlays[i].empty()) {
      out << "null";
    } else {
      out << "\"" << hold_overlays[i] << "\"";
    }
  }
  out << R"(]
  }
};
</script>
<script type="module">
import { GoldenLayout, LayoutConfig } from 'https://esm.sh/golden-layout@2.6.0';
)" << kReportJS
      << R"(
</script>
</body>
</html>
)";

  out.close();
  logger_->info(utl::WEB, 32, "Saved timing report to {}", filename);
}

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

ListenerHandle createAndRunListener(
    net::io_context& ioc,
    const Tcp::endpoint& endpoint,
    std::shared_ptr<TileGenerator> generator,
    std::shared_ptr<TclEvaluator> tcl_eval,
    std::shared_ptr<TimingReport> timing_report,
    std::shared_ptr<ClockTreeReport> clock_report,
    utl::Logger* logger,
    WebViewerHook* viewer_hook)
{
  auto listener = std::make_shared<Listener>(ioc,
                                             endpoint,
                                             std::move(generator),
                                             std::move(tcl_eval),
                                             std::move(timing_report),
                                             std::move(clock_report),
                                             logger,
                                             viewer_hook);
  listener->run();
  return {.shutdown = [listener]() { listener->close(); },
          .port = listener->port()};
}

}  // namespace web
