// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "web/web.h"

#include <netinet/in.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <ios>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/strand.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/http.hpp"
#include "boost/beast/websocket.hpp"
#include "boost/json/array.hpp"
#include "boost/json/object.hpp"
#include "boost/json/parse.hpp"
#include "boost/json/serialize.hpp"
#include "boost/json/value.hpp"
#include "clock_tree_report.h"
#include "color.h"
#include "gui/heatMap.h"
#include "hierarchy_report.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbChipCallBackObj.h"
#include "request_dispatcher.h"
#include "request_handler.h"
#include "tcl.h"
#include "tile_generator.h"
#include "timing_report.h"
#include "utl/Logger.h"
#include "web_assets.h"
#include "web_chart.h"
#include "web_gif.h"
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
// HTTP request handler (serves embedded static assets + image downloads)
//------------------------------------------------------------------------------

namespace {

// Sanitize a client-supplied download filename before it goes into the
// Content-Disposition header: drop directory components, keep only
// [A-Za-z0-9._-] (replacing the rest with '_'), cap the length, and fall back
// to a default when empty.  Prevents header injection via quotes/CRLF.
std::string sanitizeFilename(std::string name)
{
  const auto slash = name.find_last_of("/\\");
  if (slash != std::string::npos) {
    name.erase(0, slash + 1);
  }
  constexpr std::size_t kMaxLen = 128;
  if (name.size() > kMaxLen) {
    name.resize(kMaxLen);
  }
  for (char& c : name) {
    const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                    || (c >= '0' && c <= '9') || c == '.' || c == '_'
                    || c == '-';
    if (!ok) {
      c = '_';
    }
  }
  if (name.empty()) {
    name = "layout.png";
  }
  return name;
}

// Parse an integer that must span the whole string_view (no trailing garbage),
// exception-free.  Returns false on any malformed/partial input.
template <typename T>
bool parseIntExact(std::string_view s, T& out, int base = 10)
{
  const char* const first = s.data();
  const char* const last = first + s.size();
  const auto res = std::from_chars(first, last, out, base);
  return res.ec == std::errc{} && res.ptr == last;
}

// Percent-decode a URL query value (e.g. the JSON `vis` payload).
std::string urlDecode(std::string_view s)
{
  std::string out;
  out.reserve(s.size());
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size()) {
      int value = 0;
      if (parseIntExact(s.substr(i + 1, 2), value, 16)) {
        out.push_back(static_cast<char>(value));
        i += 2;
        continue;
      }
      // Not valid hex: keep the literal '%'.
    }
    out.push_back(s[i] == '+' ? ' ' : s[i]);
  }
  return out;
}

// Parse the "k=v&k2=v2" query of a request target into a map (values decoded).
std::map<std::string, std::string> parseQuery(std::string_view target)
{
  std::map<std::string, std::string> params;
  const auto qpos = target.find('?');
  if (qpos == std::string_view::npos) {
    return params;
  }
  const std::string_view qs = target.substr(qpos + 1);
  std::size_t start = 0;
  while (start < qs.size()) {
    std::size_t amp = qs.find('&', start);
    if (amp == std::string_view::npos) {
      amp = qs.size();
    }
    const std::string_view kv = qs.substr(start, amp - start);
    const std::size_t eq = kv.find('=');
    if (eq != std::string_view::npos) {
      params.emplace(std::string(kv.substr(0, eq)),
                     urlDecode(kv.substr(eq + 1)));
    }
    start = amp + 1;
  }
  return params;
}

// Parse "x0,y0,x1,y1" (DBU) into a Rect.  Returns false on malformed input.
bool parseBbox(const std::string& s, odb::Rect& out)
{
  const std::string_view sv(s);
  int v[4];
  int idx = 0;
  std::size_t start = 0;
  while (idx < 4) {
    const std::size_t comma = sv.find(',', start);
    const std::string_view tok
        = sv.substr(start,
                    comma == std::string_view::npos ? std::string_view::npos
                                                    : comma - start);
    if (!parseIntExact(tok, v[idx++])) {
      return false;
    }
    if (comma == std::string_view::npos) {
      break;
    }
    start = comma + 1;
  }
  if (idx != 4) {
    return false;
  }
  out = odb::Rect(std::min(v[0], v[2]),
                  std::min(v[1], v[3]),
                  std::max(v[0], v[2]),
                  std::max(v[1], v[3]));
  return true;
}

// Render the layout image requested by /download/image into `res`.
void handleImageDownload(const std::shared_ptr<TileGenerator>& generator,
                         std::string_view target,
                         http::response<http::string_body>& res)
{
  if (!generator) {
    res.result(http::status::not_found);
    res.body() = "No design loaded.";
    return;
  }
  const auto params = parseQuery(target);
  const auto type_it = params.find("type");
  const std::string type = type_it == params.end() ? "entire" : type_it->second;

  odb::Rect region;  // zero-area => entire die (renderImagePng default)
  if (type == "visible") {
    const auto bbox_it = params.find("bbox");
    if (bbox_it == params.end() || !parseBbox(bbox_it->second, region)) {
      res.result(http::status::bad_request);
      res.body() = "Missing or malformed bbox.";
      return;
    }
  }

  // Respect the session's layer visibility: the frontend sends the same
  // visibility payload it uses for tiles as a JSON `vis` query param.
  TileVisibility vis;
  const auto vis_it = params.find("vis");
  if (vis_it != params.end()) {
    // Non-throwing parse: a malformed vis falls back to defaults (all visible).
    std::error_code ec;
    const boost::json::value parsed = boost::json::parse(vis_it->second, ec);
    if (!ec && parsed.is_object()) {
      vis.parseFromJson(parsed.as_object());
    }
  }

  // Background color (RRGGBB hex) so the saved image matches the viewer's
  // background; absent => transparent.
  Color bg{};  // {0,0,0,0}
  const auto bg_it = params.find("bg");
  unsigned rgb = 0;
  if (bg_it != params.end() && bg_it->second.size() == 6
      && parseIntExact(bg_it->second, rgb, 16)) {
    bg = Color{.r = static_cast<unsigned char>((rgb >> 16) & 0xFF),
               .g = static_cast<unsigned char>((rgb >> 8) & 0xFF),
               .b = static_cast<unsigned char>(rgb & 0xFF),
               .a = 255};
  }
  // Malformed/absent bg: keep transparent.

  const std::vector<unsigned char> png = generator->renderImagePng(
      region, /*width_px=*/0, /*dbu_per_pixel=*/0, vis, bg);
  if (png.empty()) {
    res.result(http::status::internal_server_error);
    res.body() = "Image render failed.";
    return;
  }

  const auto fname_it = params.find("filename");
  const std::string filename = sanitizeFilename(
      fname_it == params.end() ? "layout.png" : fname_it->second);
  res.set(http::field::content_type, "image/png");
  res.set(http::field::content_disposition,
          "attachment; filename=\"" + filename + "\"");
  res.body().assign(reinterpret_cast<const char*>(png.data()), png.size());
}

}  // namespace

static http::response<http::string_body> handle_request(
    http::request<http::string_body>&& req,
    const std::shared_ptr<TileGenerator>& generator)
{
  http::response<http::string_body> res{http::status::ok, req.version()};
  res.set(http::field::server, "Boost.Beast Server (C++17)");
  res.set(http::field::content_type, "text/plain");
  res.keep_alive(req.keep_alive());
  res.set(http::field::access_control_allow_origin, "*");

  if (req.method() == http::verb::get) {
    // The route match uses the path alone; the download handler needs the
    // raw target because its parameters live in the query string.
    const std::string target(req.target());
    const std::string file_path = assetPathFromTarget(target);

    if (file_path == "/download/image") {
      handleImageDownload(generator, target, res);
    } else {
      const auto* asset = findEmbeddedAsset(file_path);
      if (asset) {
        res.set(http::field::content_type, asset->content_type);
        // The assets are compiled into the binary, so their URLs carry no
        // version and never change.  Without this a browser is free to
        // heuristically cache them, and a tab reloaded against a rebuilt
        // OpenROAD keeps running the old JavaScript -- the change appears
        // to have done nothing until someone thinks to hard-reload.  They
        // are served from memory, so re-fetching them costs nothing.
        res.set(http::field::cache_control, "no-store");
        res.body() = std::string(asset->content());
      } else {
        res.result(http::status::not_found);
        res.body() = "Resource not found.";
      }
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

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>,
                         public odb::dbChipCallBackObj,
                         public odb::dbBlockCallBackObj
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
  EditHandler edit_handler_;

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

  // In-flight request window announced to the client on connect.
  int max_in_flight_ = 16;

 public:
  WebSocketSession(Tcp::socket&& socket,
                   std::shared_ptr<TileGenerator> generator,
                   std::shared_ptr<TclEvaluator> tcl_eval,
                   std::shared_ptr<TimingReport> timing_report,
                   std::shared_ptr<ClockTreeReport> clock_report,
                   utl::Logger* logger,
                   WebViewerHook* viewer_hook,
                   int max_in_flight);
  ~WebSocketSession();

  void run(http::request<http::string_body>&& req);

 private:
  void on_accept(beast::error_code ec);
  void do_read();
  void on_read(beast::error_code ec);
  void queue_response(const WebSocketResponse& resp,
                      std::function<void()> on_complete = {});
  void do_write();

  void inDbMarkerCategoryCreate(odb::dbMarkerCategory*) override
  {
    WebSocketResponse resp;
    resp.type = WebSocketResponse::kJson;
    const std::string json = R"({"type":"drcUpdated"})";
    resp.payload.assign(json.begin(), json.end());
    queue_response(resp);
  }

  void inDbMarkerCategoryDestroy(odb::dbMarkerCategory*) override
  {
    WebSocketResponse resp;
    resp.type = WebSocketResponse::kJson;
    const std::string json = R"({"type":"drcUpdated"})";
    resp.payload.assign(json.begin(), json.end());
    queue_response(resp);
  }

  void inDbMarkerCreate(odb::dbMarker*) override
  {
    WebSocketResponse resp;
    resp.type = WebSocketResponse::kJson;
    const std::string json = R"({"type":"drcUpdated"})";
    resp.payload.assign(json.begin(), json.end());
    queue_response(resp);
  }

  void inDbMarkerDestroy(odb::dbMarker*) override
  {
    WebSocketResponse resp;
    resp.type = WebSocketResponse::kJson;
    const std::string json = R"({"type":"drcUpdated"})";
    resp.payload.assign(json.begin(), json.end());
    queue_response(resp);
  }

  // Destroying any selectable object (via trigger_action or a Tcl
  // command) leaves the session's stored gui::Selected wrappers holding
  // dangling odb pointers.  Raise the staleness flag — handlers drop the
  // whole selection state via consumeStaleSelection() before the next
  // dereference — and tell the client once so it clears its inspector.
  // Deliberately coarse: destroys are rare interactive events and
  // membership testing against indirect references (e.g. an ITerm of a
  // destroyed inst) is error-prone.
  void invalidateSelection()
  {
    if (state_.selection_stale.exchange(true)) {
      return;  // already pending (debounces mass deletes)
    }
    WebSocketResponse resp;
    resp.type = WebSocketResponse::kJson;
    const std::string json = R"({"type":"selection_invalidated"})";
    resp.payload.assign(json.begin(), json.end());
    queue_response(resp);
  }

  // Moving an object does not dangle any pointer, so the selection stands --
  // but every shape derived from the old placement is now wrong.  This fires
  // in all sessions, which is what makes another client's set_property edit
  // reach this one's cached highlight-group rectangles.
  void inDbPostMoveInst(odb::dbInst*) override
  {
    state_.highlight_geometry_stale = true;
  }
  void inDbInstSwapMasterAfter(odb::dbInst*) override
  {
    state_.highlight_geometry_stale = true;
  }

  void inDbInstDestroy(odb::dbInst*) override { invalidateSelection(); }
  void inDbNetDestroy(odb::dbNet*) override { invalidateSelection(); }
  void inDbITermDestroy(odb::dbITerm*) override { invalidateSelection(); }
  void inDbBTermDestroy(odb::dbBTerm*) override { invalidateSelection(); }
  void inDbBlockageDestroy(odb::dbBlockage*) override { invalidateSelection(); }
  void inDbObstructionDestroy(odb::dbObstruction*) override
  {
    invalidateSelection();
  }
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
    WebViewerHook* viewer_hook,
    int max_in_flight)
    : websocket_(std::move(socket)),
      logger_(logger),
      select_handler_(generator, tcl_eval),
      tcl_handler_(tcl_eval),
      timing_handler_(generator, std::move(timing_report), tcl_eval),
      clock_tree_handler_(generator, std::move(clock_report), tcl_eval),
      tile_handler_(generator),
      drc_handler_(generator),
      edit_handler_(generator, tcl_eval),
      strand_(net::make_strand(websocket_.get_executor())),
      generator_(std::move(generator)),
      viewer_hook_(viewer_hook),
      max_in_flight_(max_in_flight)
{
  if (generator_) {
    odb::dbChip* chip = generator_->getChip();
    if (chip) {
      odb::dbChipCallBackObj::addOwner(chip);
      odb::dbBlock* block = chip->getBlock();
      if (block) {
        odb::dbBlockCallBackObj::addOwner(block);
      }
    }
  }

  if (generator_->getBlock()) {
    tile_handler_.initializeHeatMaps(state_);
  }

  // DB-mutating requests (set_property) notify every connected client so
  // all views re-render.  Fire-and-forget; safe from any thread.
  select_handler_.setBroadcastFn([hook = viewer_hook_](const std::string& j) {
    if (hook != nullptr) {
      hook->sessions().broadcast(j);
    }
  });

  // Labels are global, not per-session, so one client's edit changes what
  // every client should be drawing.
  tile_handler_.setBroadcastFn([hook = viewer_hook_](const std::string& j) {
    if (hook != nullptr) {
      hook->sessions().broadcast(j);
    }
  });

  // Register all handler request types with the dispatcher.
  select_handler_.registerRequests(dispatcher_);
  tcl_handler_.registerRequests(dispatcher_);
  timing_handler_.registerRequests(dispatcher_);
  clock_tree_handler_.registerRequests(dispatcher_);
  tile_handler_.registerRequests(dispatcher_);
  drc_handler_.registerRequests(dispatcher_);
  edit_handler_.registerRequests(dispatcher_);

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
        boost::json::object root;
        boost::json::array charts;
        if (viewer_hook_ != nullptr) {
          const auto& hook_charts = viewer_hook_->charts();
          charts.reserve(hook_charts.size());
          for (WebChart* chart : hook_charts) {
            boost::json::object c;
            c["name"] = chart->name();
            c["x_label"] = chart->xLabel();
            boost::json::array y_labels;
            for (const auto& lbl : chart->yLabels()) {
              y_labels.emplace_back(lbl);
            }
            c["y_labels"] = std::move(y_labels);
            c["x_format"] = chart->xAxisFormat();
            boost::json::array y_formats;
            for (const auto& f : chart->yAxisFormats()) {
              y_formats.emplace_back(f);
            }
            c["y_formats"] = std::move(y_formats);
            boost::json::array points;
            const auto& chart_points = chart->points();
            points.reserve(chart_points.size());
            for (const auto& pt : chart_points) {
              boost::json::object p;
              p["x"] = pt.x;
              boost::json::array ys;
              ys.reserve(pt.ys.size());
              for (double v : pt.ys) {
                ys.emplace_back(v);
              }
              p["ys"] = std::move(ys);
              points.emplace_back(std::move(p));
            }
            c["points"] = std::move(points);
            charts.emplace_back(std::move(c));
          }
        }
        root["charts"] = std::move(charts);
        std::string s = boost::json::serialize(root);
        resp.payload.assign(s.begin(), s.end());
        return resp;
      },
      /*run_inline=*/true);

  // Client continuously syncs its full display-controls state here so the
  // Tcl save_display_controls command can persist it to a file.  Runs
  // inline (no net::post) — it only touches the mutex-guarded cache.
  dispatcher_.add(
      "set_display_state",
      WebSocketRequest::kSetDisplayState,
      [this](const WebSocketRequest& req, SessionState&) -> WebSocketResponse {
        if (viewer_hook_ != nullptr) {
          if (auto* state = req.json.if_contains("state")) {
            viewer_hook_->setDisplayState(boost::json::serialize(*state));
          }
        }
        WebSocketResponse resp;
        resp.id = req.id;
        resp.type = WebSocketResponse::kJson;
        const std::string json = R"({"ok":1})";
        resp.payload.assign(json.begin(), json.end());
        return resp;
      },
      /*run_inline=*/true);

  // Serves the current custom-UI registry (Tcl-registered menu items /
  // toolbar buttons) to a client on connect.  Live updates arrive later as
  // {"type":"custom_ui",...} server-push broadcasts from WebViewerHook.
  dispatcher_.add(
      "custom_ui",
      WebSocketRequest::kCustomUi,
      [this](const WebSocketRequest& req, SessionState&) -> WebSocketResponse {
        WebSocketResponse resp;
        resp.id = req.id;
        resp.type = WebSocketResponse::kJson;
        const std::string json = viewer_hook_ != nullptr
                                     ? viewer_hook_->customUiJson()
                                     : R"({"type":"custom_ui","menu":[],)"
                                       R"("toolbar":[]})";
        resp.payload.assign(json.begin(), json.end());
        return resp;
      },
      /*run_inline=*/true);
}

WebSocketSession::~WebSocketSession()
{
  if (generator_) {
    odb::dbChip* chip = generator_->getChip();
    if (chip) {
      odb::dbChipCallBackObj::removeOwner();
      odb::dbBlock* block = chip->getBlock();
      if (block) {
        odb::dbBlockCallBackObj::removeOwner();
      }
    }
  }
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

  // Tell the client how many requests to keep in flight at once. This bounds
  // the client's send rate so a burst of tile requests (rapid pan/zoom) can't
  // flood the socket send buffer and wedge the connection. The limit is scaled
  // to the server's I/O worker count (see WebServer::serve). Sent first so the
  // client has it before requesting any tiles.
  {
    WebSocketResponse cfg;
    cfg.id = 0;
    cfg.type = WebSocketResponse::kJson;
    const std::string cfg_json = R"({"type":"config","max_in_flight":)"
                                 + std::to_string(max_in_flight_) + "}";
    cfg.payload.assign(cfg_json.begin(), cfg_json.end());
    queue_response(cfg);
  }

  // Build search indices in the background; tiles render without shapes
  // until ready, then a "refresh" push notification triggers a redraw.
  init_thread_ = std::thread([self = shared_from_this()]() {
    self->generator_->eagerInit();
    // Only send refresh if there's actually a design to render.
    // Without this guard, eagerInit returns instantly when no block is
    // loaded and the push races with async_accept (Beast soft_mutex crash).
    // We gate on the dbChip (not dbBlock) so 3DBlox multi-tech designs
    // — whose top chip is HIER and has no dbBlock — still register the
    // chip observer and send the refresh notification.
    if (!self->generator_->getChip()) {
      return;
    }

    // Re-register chip/block observer if the chip was created after session
    // construction (e.g. read_def ran after browser connected).
    if (!self->odb::dbChipCallBackObj::hasOwner()) {
      odb::dbChip* chip = self->generator_->getChip();
      if (chip) {
        self->odb::dbChipCallBackObj::addOwner(chip);
        odb::dbBlock* block = chip->getBlock();
        if (block && !self->odb::dbBlockCallBackObj::hasOwner()) {
          self->odb::dbBlockCallBackObj::addOwner(block);
        }
      }
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

namespace {

// Run a request handler, converting any exception into an error response.
//
// Handlers run either on the read thread or on a bare io_context thread, and
// neither has a try/catch above it: the threads run io_context::run() directly
// (see createAndRunListener), so an exception escaping a handler unwinds out of
// the thread function and calls std::terminate -- the whole openroad process
// dies, taking the design with it.  A malformed request must cost the client
// one error response, never the session.
//
// The obvious offender is a field whose JSON type is wrong -- boost::json's
// as_int64()/as_string() throw rather than return an error.  See
// parseTileCoords() in request_handler.h for why a careful client cannot avoid
// this on its own.
WebSocketResponse invoke_handler(const RequestDispatcher::HandleFn& handle,
                                 const WebSocketRequest& req,
                                 SessionState& state)
{
  WebSocketResponse resp;
  // `handle` returns by value, so on a throw `resp` is still the untouched
  // default -- no partial payload to clear.
  try {
    resp = handle(req, state);
  } catch (const std::exception& e) {
    resp = errorResponse(req.id, std::string("server error: ") + e.what());
  }
  resp.request_type = req.raw_type;
  return resp;
}

}  // namespace

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
      queue_response(invoke_handler(entry->handle, req, state_));
    } else {
      auto handle = entry->handle;
      net::post(
          websocket_.get_executor(),
          [self = std::move(self),
           req = std::move(req),
           handle = std::move(handle)]() {
            self->queue_response(invoke_handler(handle, req, self->state_));
          });
    }
  } else {
    // Unknown type -- return an error so the client knows the request
    // was not understood (e.g. typo or client/server version mismatch).
    WebSocketResponse resp;
    resp.id = req.id;
    resp.type = WebSocketResponse::kError;
    resp.request_type = req.raw_type;
    std::string err;
    if (!req.raw_type.empty()) {
      err = "Unknown request type: " + req.raw_type;
    } else if (!req.parse_error.empty()) {
      err = "Malformed request: " + req.parse_error;
    } else {
      err = "Malformed request (missing or invalid id/type)";
    }
    resp.payload.assign(err.begin(), err.end());
    queue_response(resp);
  }

  do_read();
}

void WebSocketSession::queue_response(const WebSocketResponse& resp,
                                      std::function<void()> on_complete)
{
  // Surface every error response in the server log so contract violations
  // (malformed payloads, missing fields, wrong field types) are visible to
  // the operator/developer and not silently swallowed by the client.
  if (resp.type == WebSocketResponse::kError) {
    const std::string_view err(
        reinterpret_cast<const char*>(resp.payload.data()),
        resp.payload.size());
    const std::string type_label
        = resp.request_type.empty() ? "unknown" : resp.request_type;
    logger_->warn(utl::WEB,
                  43,
                  "request id={} type={} failed: {}",
                  resp.id,
                  type_label,
                  err);
  }

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
  std::shared_ptr<TileGenerator> generator_;
  utl::Logger* logger_;

 public:
  HttpSession(Tcp::socket&& socket,
              std::shared_ptr<TileGenerator> generator,
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

HttpSession::HttpSession(Tcp::socket&& socket,
                         std::shared_ptr<TileGenerator> generator,
                         utl::Logger* logger)
    : stream_(std::move(socket)),
      generator_(std::move(generator)),
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
      handle_request(std::move(req_), generator_));
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
  int max_in_flight_ = 16;

 public:
  DetectSession(Tcp::socket&& socket,
                std::shared_ptr<TileGenerator> generator,
                std::shared_ptr<TclEvaluator> tcl_eval,
                std::shared_ptr<TimingReport> timing_report,
                std::shared_ptr<ClockTreeReport> clock_report,
                utl::Logger* logger,
                WebViewerHook* viewer_hook,
                int max_in_flight);

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
                             WebViewerHook* viewer_hook,
                             int max_in_flight)
    : stream_(std::move(socket)),
      generator_(std::move(generator)),
      tcl_eval_(std::move(tcl_eval)),
      timing_report_(std::move(timing_report)),
      clock_report_(std::move(clock_report)),
      logger_(logger),
      viewer_hook_(viewer_hook),
      max_in_flight_(max_in_flight)
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
                                             viewer_hook_,
                                             max_in_flight_);
    websocket_session->run(std::move(req_));
  } else {
    // Regular HTTP - hand off to session with already-read request
    auto s = std::make_shared<HttpSession>(
        stream_.release_socket(), generator_, logger_);
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
  int max_in_flight_ = 16;

 public:
  Listener(net::io_context& ioc,
           const Tcp::endpoint& endpoint,
           std::shared_ptr<TileGenerator> generator,
           std::shared_ptr<TclEvaluator> tcl_eval,
           std::shared_ptr<TimingReport> timing_report,
           std::shared_ptr<ClockTreeReport> clock_report,
           utl::Logger* logger,
           WebViewerHook* viewer_hook,
           int max_in_flight);

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
                   WebViewerHook* viewer_hook,
                   int max_in_flight)
    : ioc_(ioc),
      acceptor_(ioc),
      generator_(std::move(generator)),
      tcl_eval_(std::move(tcl_eval)),
      timing_report_(std::move(timing_report)),
      clock_report_(std::move(clock_report)),
      logger_(logger),
      viewer_hook_(viewer_hook),
      max_in_flight_(max_in_flight)
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
                                    viewer_hook_,
                                    max_in_flight_)
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
    : db_(db), sta_(sta), logger_(logger), interp_(interp), num_threads_(1)
{
}

void WebServer::setThreadCount(int num_threads)
{
  num_threads_ = num_threads;
  if (generator_) {
    generator_->setThreadCount(num_threads);
  }
}

TileGenerator& WebServer::ensureGenerator()
{
  if (!generator_) {
    generator_ = std::make_shared<TileGenerator>(db_, sta_, logger_);
    generator_->setThreadCount(num_threads_);
  }
  return *generator_;
}

// Defined here (not in web_serve.cpp) so the destructor's TU does not
// pull in web_serve.cpp's gui::Gui::get() references — keeps WebServer
// usable from tests that don't link the full gui library.
void WebServer::stopAndJoinIoThreads()
{
  if (ioc_) {
    ioc_->stop();
  }
  const auto self_id = std::this_thread::get_id();
  for (auto& t : threads_) {
    if (!t.joinable()) {
      continue;
    }
    if (t.get_id() == self_id) {
      // Self-join would raise EDEADLK. ioc_->stop() above unblocks the
      // worker so detaching is safe — the thread runs to completion on
      // its own.
      t.detach();
    } else {
      t.join();
    }
  }
  threads_.clear();
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
  // objects may be destroyed.  We avoid the full stop() path (which
  // tears down boost::asio's reactor and would crash on residual async
  // handlers referencing destroyed statics) — the OS reclaims memory
  // and sockets at process exit, so we only need to release the worker
  // threads so the process can actually exit.
  stopAndJoinIoThreads();
  if (shutdown_listener_) {
    shutdown_listener_();
    shutdown_listener_ = {};
  }
  // Release without destroying — ~io_context() crashes because
  // reactor::shutdown() destroys pending async operations whose
  // handlers reference the dying reactor.  The OS reclaims at exit.
  (void) ioc_.release();  // NOLINT(bugprone-unused-return-value)
  // Remove the WebLogSink from the Logger before leaking viewer_hook_: the
  // sink holds a raw pointer into the hook and the CLI thread may still emit
  // log lines.  In the normal path stop() already did this (log_sink_ is
  // null here); this covers paths where serve()/stop() never ran — e.g.
  // initLogger() registered the sink but read_db failed and exited.  logger_
  // outlives us: ~OpenRoad deletes web_server_ before logger_.
  if (log_sink_) {
    logger_->removeSink(log_sink_);
    log_sink_.reset();
  }
  // Also leak viewer_hook_ — it may be referenced by Gui's
  // headless_viewer_ pointer which outlives us (static singleton).
  (void) viewer_hook_.release();  // NOLINT(bugprone-unused-return-value)
}

// Embedded JS/CSS for standalone timing report (generated at build time
// by embed_report_assets.py → report_assets.cpp).
extern const std::string_view kReportCSS;
extern const std::string_view kReportJS;

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
  ensureGenerator().eagerInit();

  odb::dbBlock* block = generator_->getBlock();
  if (!block && generator_->chiplets().size() <= 1) {
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
    setup_json = boost::json::serialize(serializeTimingPaths(setup_paths));
    hold_json = boost::json::serialize(serializeTimingPaths(hold_paths));
    hist_setup = boost::json::serialize(
        serializeSlackHistogram(report.getSlackHistogram(true)));
    hist_hold = boost::json::serialize(
        serializeSlackHistogram(report.getSlackHistogram(false)));
    filters = boost::json::serialize(
        serializeChartFilters(report.getChartFilters()));
  } else {
    logger_->warn(utl::WEB, 30, "No STA data — timing sections will be empty.");
    setup_json = boost::json::serialize(serializeTimingPaths({}));
    hold_json = setup_json;
    hist_setup = boost::json::serialize(serializeSlackHistogram({}));
    hist_hold = hist_setup;
    filters = boost::json::serialize(serializeChartFilters({}));
  }
  // Net fanout histogram depends only on odb, so it's always populated.
  const std::string hist_fanout = boost::json::serialize(
      serializeFanoutHistogram(computeFanoutHistogram(block)));
  const std::string tech_json
      = boost::json::serialize(serializeTechResponse(*generator_));
  const std::string bounds_json
      = boost::json::serialize(serializeBoundsResponse(*generator_, true));
  const auto tech_layers = generator_->getLayers();

  // ── Serialize module hierarchy ──

  HierarchyReport hier_report(block, sta_);
  auto hier_result = hier_report.getReport();

  const std::string hierarchy_json
      = boost::json::serialize(serializeHierarchyResult(hier_result));

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
      if (block) {
        collectTimingPathShapes(block, path, rects, lines);
      } else {
        collectTimingPathShapes(generator_->chiplets(), path, rects, lines);
      }
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
    "fanout_histogram": )"
      << hist_fanout << R"(,
    "chart_filters": )"
      << filters << R"(,
    "module_hierarchy": )"
      << hierarchy_json << R"(
  },
  tiles: {)";

  // Emit tile entries.  Use boost::json::serialize for the key to escape
  // special characters consistently (the value side is already base64).
  for (size_t i = 0; i < tile_entries.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << "\n    "
        << boost::json::serialize(boost::json::value(tile_entries[i].first))
        << ":\"" << tile_entries[i].second << "\"";
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
import * as THREE from 'https://esm.sh/three@0.160.0';
)" << kReportJS
      << R"(
</script>
</body>
</html>
)";

  out.close();
  logger_->info(utl::WEB, 32, "Saved timing report to {}", filename);
}

namespace {
// Nearest-neighbor resample of a top-down RGBA8 buffer into a dw x dh canvas,
// preserving aspect ratio (fit + center, black letterbox) — matches the Qt
// GUI's QImage::scaled(..., KeepAspectRatio).  Used to fit later GIF frames
// into the dimensions locked by the first frame without distortion.
std::vector<unsigned char> resampleRgba(const std::vector<unsigned char>& src,
                                        int sw,
                                        int sh,
                                        int dw,
                                        int dh)
{
  // Opaque-black background (GIF ignores alpha, so use RGB black + full alpha).
  std::vector<unsigned char> dst(4UL * dw * dh, 0);
  for (size_t i = 3; i < dst.size(); i += 4) {
    dst[i] = 255;
  }
  if (sw <= 0 || sh <= 0) {
    return dst;
  }
  // Largest integer size that fits in dw x dh while preserving aspect.
  const double scale
      = std::min(static_cast<double>(dw) / sw, static_cast<double>(dh) / sh);
  const int fw = std::clamp(static_cast<int>(std::lround(sw * scale)), 1, dw);
  const int fh = std::clamp(static_cast<int>(std::lround(sh * scale)), 1, dh);
  const int off_x = (dw - fw) / 2;
  const int off_y = (dh - fh) / 2;
  for (int y = 0; y < fh; ++y) {
    const int sy = std::min(sh - 1, y * sh / fh);
    for (int x = 0; x < fw; ++x) {
      const int sx = std::min(sw - 1, x * sw / fw);
      const size_t s = (static_cast<size_t>(sy) * sw + sx) * 4;
      const size_t d = (static_cast<size_t>(off_y + y) * dw + (off_x + x)) * 4;
      for (int c = 0; c < 4; ++c) {
        dst[d + c] = src[s + c];
      }
    }
  }
  return dst;
}

// Parse the visibility JSON produced by the Tcl layer (shared by saveImage
// and gifAddFrame).
TileVisibility parseVis(const std::string& vis_json, utl::Logger* logger)
{
  TileVisibility vis;
  if (!vis_json.empty()) {
    try {
      boost::json::value v = boost::json::parse(vis_json);
      if (auto* obj = v.if_object()) {
        vis.parseFromJson(*obj);
      }
    } catch (const std::exception& e) {
      logger->warn(
          utl::WEB, 42, "Ignoring malformed visibility JSON: {}", e.what());
    }
  }
  return vis;
}
}  // namespace

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
  ensureGenerator().eagerInit();

  const odb::Rect region(x0, y0, x1, y1);
  const TileVisibility vis = parseVis(vis_json, logger_);
  generator_->saveImage(filename, region, width_px, dbu_per_pixel, vis);
}

namespace {

// Parse a color given as "#rgb", "#rrggbb", or a small set of names.
// Defaults to opaque white on empty/unknown input.
Color parseColorString(const std::string& s)
{
  Color c{.r = 255, .g = 255, .b = 255, .a = 255};
  if (s.empty()) {
    return c;
  }
  std::string lower;
  lower.reserve(s.size());
  for (char ch : s) {
    lower.push_back(static_cast<char>(std::tolower(ch)));
  }
  static const std::map<std::string, Color> kNamed = {
      {"white", {255, 255, 255, 255}},
      {"black", {0, 0, 0, 255}},
      {"red", {255, 0, 0, 255}},
      {"green", {0, 255, 0, 255}},
      {"blue", {0, 0, 255, 255}},
      {"yellow", {255, 255, 0, 255}},
      {"cyan", {0, 255, 255, 255}},
      {"magenta", {255, 0, 255, 255}},
  };
  const auto it = kNamed.find(lower);
  if (it != kNamed.end()) {
    return it->second;
  }
  if (lower[0] == '#') {
    lower.erase(0, 1);
  }
  auto hex = [](const std::string& h) {
    return static_cast<unsigned char>(std::strtol(h.c_str(), nullptr, 16));
  };
  if (lower.size() == 6) {
    c.r = hex(lower.substr(0, 2));
    c.g = hex(lower.substr(2, 2));
    c.b = hex(lower.substr(4, 2));
  } else if (lower.size() == 3) {
    c.r = hex(std::string(2, lower[0]));
    c.g = hex(std::string(2, lower[1]));
    c.b = hex(std::string(2, lower[2]));
  }
  return c;
}

}  // namespace

// Push the current label set to every connected client.  Labels sit outside
// ODB, so no design-change callback fires for them and a Tcl-driven edit
// would otherwise leave every open browser showing the old set.  A no-op
// before serve(), where there is nobody to tell.
void WebServer::broadcastLabels()
{
  if (!viewer_hook_ || !generator_) {
    return;
  }
  boost::json::object msg;
  msg["type"] = "labels_changed";
  msg["labels"] = generator_->labelsJson();
  viewer_hook_->sessions().broadcast(boost::json::serialize(msg));
}

std::string WebServer::addLabel(const int x,
                                const int y,
                                const std::string& text,
                                const std::string& anchor,
                                const std::string& color,
                                const int size,
                                const std::string& name)
{
  ensureGenerator();
  // Reject an unknown anchor rather than silently centring the label, which
  // reads as "the option did nothing".  Qt's add_label errors the same way
  // (GUI-45); listing the choices saves a trip to the manual over a typo.
  const std::string anchor_name = anchor.empty() ? "center" : anchor;
  if (!isValidAnchor(anchor_name)) {
    std::string choices;
    for (const std::string& n : anchorNames()) {
      if (!choices.empty()) {
        choices += ", ";
      }
      choices += n;
    }
    logger_->error(utl::WEB,
                   58,
                   "Anchor not recognized: {}. Expected one of: {}.",
                   anchor_name,
                   choices);
  }
  const std::string result = generator_->addLabel(
      {x, y}, text, parseColorString(color), size, anchor_name, name);
  if (result.empty()) {
    logger_->warn(utl::WEB, 57, "Label name '{}' already exists.", name);
  } else {
    broadcastLabels();
  }
  return result;
}

void WebServer::deleteLabel(const std::string& name)
{
  if (generator_ && generator_->deleteLabel(name)) {
    broadcastLabels();
  }
}

void WebServer::clearLabels()
{
  if (generator_) {
    generator_->clearLabels();
    broadcastLabels();
  }
}

void WebServer::saveDisplayControls(const std::string& filename)
{
  if (!viewer_hook_) {
    logger_->error(utl::WEB, 51, "Web server is not running.");
    return;
  }
  const std::string state = viewer_hook_->getDisplayState();
  if (state.empty()) {
    logger_->warn(utl::WEB,
                  44,
                  "No display state has been received from a client yet; "
                  "open the web viewer before saving.");
    return;
  }
  std::ofstream out(filename);
  if (!out) {
    logger_->error(utl::WEB, 45, "Cannot open {} for writing.", filename);
    return;
  }
  out << state;
  // Opening the file succeeding says nothing about the write: a full disk or a
  // quota surfaces only here, and reporting success would leave a truncated
  // file behind.  close() flushes, so check after it.
  out.close();
  if (!out) {
    logger_->error(
        utl::WEB, 53, "Failed writing display controls to {}.", filename);
    return;
  }
  logger_->info(utl::WEB, 46, "Saved display controls to {}.", filename);
}

void WebServer::restoreDisplayControls(const std::string& filename)
{
  if (!viewer_hook_) {
    logger_->error(utl::WEB, 47, "Web server is not running.");
    return;
  }
  std::ifstream in(filename);
  if (!in) {
    logger_->error(utl::WEB, 48, "Cannot open {}.", filename);
    return;
  }
  const std::string state((std::istreambuf_iterator<char>(in)),
                          std::istreambuf_iterator<char>());
  boost::json::value parsed;
  try {
    parsed = boost::json::parse(state);
  } catch (const std::exception& e) {
    logger_->error(utl::WEB,
                   49,
                   "Invalid display-controls JSON in {}: {}",
                   filename,
                   e.what());
    return;
  }
  // Validate the shape here, at the file boundary, so the client can trust
  // what it is handed: it writes these values straight into document.cookie,
  // where a ';' would inject cookie attributes, and a JSON number or boolean
  // would silently change a key's meaning.  Rejecting loudly beats a restore
  // that half-applies.
  const auto* root = parsed.if_object();
  const auto* entries_value
      = root != nullptr ? root->if_contains("entries") : nullptr;
  const boost::json::object* entries
      = entries_value != nullptr ? entries_value->if_object() : nullptr;
  if (entries == nullptr) {
    logger_->error(utl::WEB,
                   52,
                   "Invalid display-controls state in {}: expected an "
                   "\"entries\" object.",
                   filename);
    return;
  }
  // One error site for every way an entry can be unusable, so the reason is
  // carried as text rather than burning a message id per case.  error() does
  // not return, so reporting at the point of detection needs no loop-carried
  // state.
  for (const auto& [key, value] : *entries) {
    const char* reason = nullptr;
    if (!value.is_string()) {
      reason = "is not a string";
    } else if (const std::string_view text = value.get_string();
               text.find_first_of(";\r\n") != std::string_view::npos) {
      // Cookie delimiters would let the file forge attributes on the cookies
      // the client writes these into.  No legitimate value carries them: the
      // cookie-backed ones are URI-encoded by their writers, the rest is JSON.
      reason = "contains a ';', CR or LF";
    }
    if (reason != nullptr) {
      logger_->error(utl::WEB,
                     54,
                     "Invalid display-controls state in {}: entry \"{}\" {}.",
                     filename,
                     std::string(key),
                     reason);
    }
  }

  boost::json::object msg;
  msg["type"] = "restore_display_state";
  msg["state"] = std::move(parsed);
  viewer_hook_->sessions().broadcast(boost::json::serialize(msg));
  logger_->info(utl::WEB, 50, "Restored display controls from {}.", filename);
}

void WebServer::setDisplayState(std::string json)
{
  if (viewer_hook_) {
    viewer_hook_->setDisplayState(std::move(json));
  }
}

// One open animated-GIF stream.  Defined here (not in web.h) so the public
// header stays free of the GifEncoder type.
struct WebGif
{
  std::string filename;
  GifEncoder encoder;
  int width = -1;
  int height = -1;
  int frame_count = 0;
};

namespace {
// Resolve a GIF stream by key (default = most-recent).  Reports the resolved
// index via *out_idx and returns nullptr when the key is out of range or the
// slot was closed; the caller logs the error (with a literal message id).
WebGif* resolveGif(std::vector<std::unique_ptr<WebGif>>& gifs,
                   std::optional<int> key,
                   int* out_idx)
{
  const int idx = key.value_or(static_cast<int>(gifs.size()) - 1);
  *out_idx = idx;
  if (idx < 0 || idx >= static_cast<int>(gifs.size()) || !gifs[idx]) {
    return nullptr;
  }
  return gifs[idx].get();
}
}  // namespace

int WebServer::gifStart(const std::string& filename)
{
  if (filename.empty()) {
    logger_->error(utl::WEB, 69, "GIF filename is empty.");
    return -1;
  }
  auto gif = std::make_unique<WebGif>();
  gif->filename = filename;
  gifs_.push_back(std::move(gif));
  return static_cast<int>(gifs_.size()) - 1;
}

void WebServer::gifAddFrame(std::optional<int> key,
                            const odb::Rect& region,
                            const int width_px,
                            const double dbu_per_pixel,
                            std::optional<int> delay,
                            const std::string& vis_json)
{
  int idx = 0;
  WebGif* gif = resolveGif(gifs_, key, &idx);
  if (gif == nullptr) {
    logger_->error(utl::WEB, 70, "No active GIF for key {}.", idx);
    return;
  }

  // Create generator on demand (server may not be running).  eagerInit()
  // rebuilds the spatial index, so run it only once per stream (first frame);
  // the design is static across a GIF's frames.
  ensureGenerator();
  if (gif->frame_count == 0) {
    generator_->eagerInit();
  }

  const TileVisibility vis = parseVis(vis_json, logger_);
  int w = 0;
  int h = 0;
  std::vector<unsigned char> rgba = generator_->renderImageBuffer(
      region, width_px, dbu_per_pixel, vis, /*bg=*/{}, &w, &h);
  if (rgba.empty()) {
    return;  // renderImageBuffer already logged the error.
  }

  const int d = delay.value_or(kDefaultGifDelay);
  if (gif->frame_count == 0) {
    // First frame locks the GIF dimensions.
    gif->width = w;
    gif->height = h;
    if (!gif->encoder.begin(gif->filename, w, h, d)) {
      logger_->error(
          utl::WEB, 71, "Failed to open GIF file {}.", gif->filename);
      gifs_[idx].reset();
      return;
    }
  } else if (w != gif->width || h != gif->height) {
    // Later frames are scaled to match the first (like gui's QImage::scaled).
    rgba = resampleRgba(rgba, w, h, gif->width, gif->height);
  }

  if (!gif->encoder.addFrame(rgba, gif->width, gif->height, d)) {
    logger_->error(utl::WEB, 72, "Failed to write GIF frame.");
    return;
  }
  ++gif->frame_count;
}

void WebServer::gifEnd(std::optional<int> key)
{
  int idx = 0;
  WebGif* gif = resolveGif(gifs_, key, &idx);
  if (gif == nullptr) {
    logger_->error(utl::WEB, 73, "No active GIF for key {}.", idx);
    return;
  }
  if (gif->frame_count == 0) {
    logger_->warn(
        utl::WEB, 74, "GIF {} has no frames; nothing written.", gif->filename);
    gifs_[idx].reset();
    return;
  }
  gif->encoder.end();
  logger_->info(utl::WEB,
                75,
                "Saved animated GIF ({} frames) to {}.",
                gif->frame_count,
                gif->filename);
  gifs_[idx].reset();
}

std::string WebServer::addToolbarButton(const std::string& name,
                                        const std::string& text,
                                        const std::string& script,
                                        const std::string& icon,
                                        const std::string& tooltip,
                                        const bool toggle,
                                        const std::string& script_off,
                                        const bool echo)
{
  // Ensure the hook exists even when called from a startup script before
  // serve() (initLogger() is idempotent and creates viewer_hook_).
  initLogger();
  return viewer_hook_->addToolbarButton(
      logger_, name, text, script, icon, tooltip, toggle, script_off, echo);
}

void WebServer::removeToolbarButton(const std::string& name)
{
  initLogger();
  viewer_hook_->removeToolbarButton(name);
}

std::string WebServer::addMenuItem(const std::string& name,
                                   const std::string& path,
                                   const std::string& text,
                                   const std::string& script,
                                   const std::string& shortcut,
                                   const bool echo)
{
  initLogger();
  return viewer_hook_->addMenuItem(
      logger_, name, path, text, script, shortcut, echo);
}

void WebServer::removeMenuItem(const std::string& name)
{
  initLogger();
  viewer_hook_->removeMenuItem(name);
}

ListenerHandle createAndRunListener(
    net::io_context& ioc,
    const Tcp::endpoint& endpoint,
    std::shared_ptr<TileGenerator> generator,
    std::shared_ptr<TclEvaluator> tcl_eval,
    std::shared_ptr<TimingReport> timing_report,
    std::shared_ptr<ClockTreeReport> clock_report,
    utl::Logger* logger,
    WebViewerHook* viewer_hook,
    int max_in_flight)
{
  auto listener = std::make_shared<Listener>(ioc,
                                             endpoint,
                                             std::move(generator),
                                             std::move(tcl_eval),
                                             std::move(timing_report),
                                             std::move(clock_report),
                                             logger,
                                             viewer_hook,
                                             max_in_flight);
  listener->run();
  return {.shutdown = [listener]() { listener->close(); },
          .port = listener->port()};
}

}  // namespace web
