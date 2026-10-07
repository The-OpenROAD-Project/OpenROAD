// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// WebServer::serve() and stop() in their own translation unit so that
// test executables linking libweb.a don't pull in web::Gui::get()
// references (which would require the full gui library including Qt
// SWIG wrappers and ord::OpenRoad symbols).

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "boost/asio/error.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/address.hpp"
#include "boost/asio/ip/address_v4.hpp"
#include "boost/asio/ip/address_v6.hpp"
#include "boost/asio/ip/host_name.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/post.hpp"
#include "boost/asio/steady_timer.hpp"
#include "boost/asio/strand.hpp"
#include "boost/system/error_code.hpp"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "boost/beast/core.hpp"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "boost/beast/websocket.hpp"
#include "boost/json/object.hpp"
#include "boost/json/serialize.hpp"
#include "clock_tree_report.h"
#include "odb/geom.h"
#include "request_handler.h"
#include "spdlog/sinks/base_sink.h"
#include "tcl.h"
#include "tile_generator.h"
#include "timing_report.h"
#include "utl/Logger.h"
#include "utl/env.h"
#include "web/core.h"
#include "web/web.h"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "web_chart.h"
#include "web_painter.h"
#include "web_viewer_hook.h"

namespace web {

namespace net = boost::asio;
using Tcp = net::ip::tcp;

// Tcl command name used to stash the original `exit` while our override
// is installed.  Mirrors web::TclCmdInputWidget's kCommandRenamePrefix.
static constexpr const char* kRenamedExitCmd = "::tcl::openroad::web_orig_exit";

namespace {

// getenv() as a string, empty when unset.
std::string envOrEmptyString(const char* name)
{
  const char* value = std::getenv(name);
  return value != nullptr ? std::string(value) : std::string();
}

#if !defined(__APPLE__) && !defined(_WIN32)
// This machine's interface addresses; empty when they cannot be listed.
std::vector<net::ip::address> localAddresses()
{
  std::vector<net::ip::address> addresses;
  ifaddrs* list = nullptr;
  if (getifaddrs(&list) != 0) {
    return addresses;
  }
  for (const ifaddrs* it = list; it != nullptr; it = it->ifa_next) {
    if (it->ifa_addr == nullptr) {
      continue;
    }
    if (it->ifa_addr->sa_family == AF_INET) {
      sockaddr_in in{};
      std::memcpy(&in, it->ifa_addr, sizeof(in));
      addresses.emplace_back(net::ip::address_v4(ntohl(in.sin_addr.s_addr)));
    } else if (it->ifa_addr->sa_family == AF_INET6) {
      sockaddr_in6 in6{};
      std::memcpy(&in6, it->ifa_addr, sizeof(in6));
      net::ip::address_v6::bytes_type bytes{};
      std::memcpy(bytes.data(), &in6.sin6_addr, bytes.size());
      addresses.emplace_back(net::ip::address_v6(bytes));
    }
  }
  freeifaddrs(list);
  return addresses;
}
#endif

// The environment the launch policy judges, read here so the policy itself
// stays free of globals.
BrowserEnv currentBrowserEnv(utl::Logger* logger,
                             std::string host_name,
                             const BrowserLaunch mode)
{
  BrowserEnv env;
  env.ssh_connection = envOrEmptyString("SSH_CONNECTION");
  env.ssh_client = envOrEmptyString("SSH_CLIENT");
  env.host_name = std::move(host_name);
  env.vscode_ipc_hook = envOrEmptyString("VSCODE_IPC_HOOK_CLI");
  env.browser = envOrEmptyString("BROWSER");
  // LSF, Slurm, SGE and PBS in that order; any of them means a scheduler
  // chose the host this runs on.
  for (const char* var : {"LSB_JOBID", "SLURM_JOB_ID", "JOB_ID", "PBS_JOBID"}) {
    env.batch_job = envOrEmptyString(var);
    if (!env.batch_job.empty()) {
      break;
    }
  }
  // An explicit -browser or -no_browser has already decided.
  if (mode == BrowserLaunch::kAuto) {
    try {
      env.no_browser = utl::readEnvarBool("OPENROAD_NO_BROWSER", false);
    } catch (const std::exception& e) {
      // A convenience variable must not keep the server from starting.
      logger->warn(utl::WEB, 123, "Ignoring OPENROAD_NO_BROWSER: {}", e.what());
    }
  }
#if !defined(__APPLE__) && !defined(_WIN32)
  env.display = envOrEmptyString("DISPLAY");
  env.has_display
      = !env.display.empty() || !envOrEmptyString("WAYLAND_DISPLAY").empty();
  const std::string socket = localDisplaySocket(env.display);
  struct stat st
  {
  };
  env.own_local_display = !socket.empty() && ::stat(socket.c_str(), &st) == 0
                          && S_ISSOCK(st.st_mode) && st.st_uid == getuid();
  env.local_addresses = localAddresses();
  env.wsl = !envOrEmptyString("WSL_DISTRO_NAME").empty();
#endif
  return env;
}

// This machine's name, for the ssh line the user is told to run.  Falls back
// to a placeholder rather than an empty word in the middle of a command.
std::string thisHostName()
{
  boost::system::error_code ec;
  std::string name = net::ip::host_name(ec);
  return ec || name.empty() ? "<this-host>" : name;
}

// Hand `launch_url` to a browser; false when the launcher reports failure.
bool launchBrowser(utl::Logger* logger,
                   const std::string& launch_url,
                   const std::string& errfile,
                   const bool through_browser_env)
{
  std::string open_cmd;
  if (through_browser_env) {
    // The shell expands $BROWSER; its value never enters this string.
    open_cmd = "\"$BROWSER\" '" + launch_url + "' < /dev/null > /dev/null 2> "
               + errfile;
  } else {
#if defined(__APPLE__)
    open_cmd = "open '" + launch_url + "' > /dev/null 2> " + errfile;
#elif defined(_WIN32)
    open_cmd = "start " + launch_url + " > nul 2> " + errfile;
#else
    // `setsid -f` forks the launcher into a new session, severing the
    // SIGHUP cascade from openroad's controlling pty.  Without this,
    // running openroad from inside an emacs shell-mode buffer kills
    // the browser tab as soon as openroad exits, because emacs holds
    // the pty master and SIGHUPs every process in the session.  Also
    // redirect stdin from /dev/null so xdg-open never blocks on input
    // inherited from the pty.
    // `setsid -w` waits for xdg-open to finish end so the return code
    // can be forwarded to setsid
    open_cmd = "setsid -f -w xdg-open '" + launch_url
               + "' < /dev/null > /dev/null 2> " + errfile;
#endif
  }
  const int ret = std::system(open_cmd.c_str());
  if (ret == 0) {
    return true;
  }
  std::string errout;
  std::ifstream err(errfile);
  if (err) {
    std::ostringstream ss;
    ss << err.rdbuf();
    errout = "\n" + ss.str();
    while (!errout.empty() && errout.back() == '\n') {
      errout.pop_back();
    }
  }
  logger->warn(utl::WEB,
               3,
               "Could not launch default browser (shell error {}){}\n"
               "Open the url above.",
               ret,
               errout);
  return false;
}

}  // namespace

// Logger sink that accumulates lines and sends them as a batch to
// connected browser clients.  Flushing is explicit (via drainToClients)
// rather than on every spdlog flush — sending per-line would overwhelm
// the WebSocket and kill the session.
class WebLogSink : public spdlog::sinks::base_sink<std::mutex>
{
 public:
  explicit WebLogSink(WebViewerHook* hook) : hook_(hook) {}

  void drainToClients()
  {
    std::lock_guard<std::mutex> lock(this->mutex_);
    sendPending();
  }

 protected:
  // NOLINTNEXTLINE(misc-include-cleaner)
  void sink_it_(const spdlog::details::log_msg& msg) override
  {
    spdlog::memory_buf_t formatted;  // NOLINT(misc-include-cleaner)
    spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
    std::string text(formatted.data(), formatted.size());
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
      text.pop_back();
    }
    if (text.empty()) {
      return;
    }
    constexpr size_t kMaxPendingBytes = 1 << 20;
    if (pending_.size() < kMaxPendingBytes) {
      pending_ += text;
      pending_ += '\n';
    }
  }
  void flush_() override {}

 private:
  void sendPending()
  {
    if (pending_.empty()) {
      return;
    }
    // Keep accumulating when nobody is listening so the first
    // client that connects receives the full startup output.
    if (!hook_->sessions().hasClients()) {
      return;
    }
    while (!pending_.empty()
           && (pending_.back() == '\n' || pending_.back() == '\r')) {
      pending_.pop_back();
    }
    if (pending_.empty()) {
      return;
    }
    boost::json::object msg;
    msg["type"] = "log";
    msg["text"] = pending_;
    hook_->sessions().broadcast(boost::json::serialize(msg));
    pending_.clear();
  }

  WebViewerHook* hook_;
  std::string pending_;
};

void WebServer::initLogger()
{
  // Idempotent: skip if already registered, or if serve() is already
  // running (it registers the sink itself).
  if (logger_initialized_ || ioc_) {
    return;
  }

  // Create the hook object now because WebLogSink holds a raw pointer to it
  // (for sessions()).  Do NOT install it as the Gui headless viewer here:
  // that would make web::Gui::enabled() true during startup scripts, and
  // web::pause() would then block kClientConnectTimeoutSeconds (~30s)
  // waiting for a web client that cannot connect until serve() opens the
  // network.  The headless viewer and chart factory are installed in
  // serve() instead.
  if (!viewer_hook_) {
    viewer_hook_ = std::make_unique<WebViewerHook>();
  }

  auto log_sink = std::make_shared<WebLogSink>(viewer_hook_.get());
  log_sink_ = log_sink;
  logger_->addSink(log_sink_);
  viewer_hook_->setDrainLogsFn(
      [log_sink = std::move(log_sink)]() { log_sink->drainToClients(); });

  logger_initialized_ = true;
}

void WebServer::serve(int port,
                      const std::string& bind_address,
                      const BrowserLaunch launch)
{
  if (ioc_) {
    logger_->warn(utl::WEB, 6, "Web server is already running.");
    return;
  }

  // Register the WebLogSink if Main.cc / the interactive `web_server`
  // command did not already call initLogger().  Idempotent.
  initLogger();

  // Clear any stale stop request left over from a previous session.
  // Without this, a requestStop() that arrives during teardown (after
  // waitForStop() cleared the flag but before stop() finishes) would
  // cause the next waitForStop() to return immediately.
  {
    std::lock_guard<std::mutex> lock(stop_mutex_);
    stop_requested_ = false;
  }

  try {
    ensureGenerator();
    auto timing_report = std::make_shared<TimingReport>(sta_);
    auto clock_report = std::make_shared<ClockTreeReport>(sta_);

    auto tcl_eval = std::make_shared<TclEvaluator>(interp_, logger_);

    // Override Tcl's `exit` so a user typing `exit` in the browser tcl
    // widget doesn't run Tcl_Exit on the worker thread (which triggers
    // ~WebServer's self-join → std::terminate).  Same pattern as
    // web::TclCmdInputWidget.  The handler signals waitForStop() and
    // sets exit_requested_; the main thread does the real exit.
    // TclHandler::handleTclEval detects kExitResultMsg in the Tcl
    // result and sends `action: "shutdown"` to the browser.
    exit_requested_ = false;
    {
      const std::string rename_orig
          = std::string("rename exit ") + kRenamedExitCmd;
      // NOLINTNEXTLINE(misc-include-cleaner)
      Tcl_Eval(interp_, rename_orig.c_str());
      // NOLINTNEXTLINE(misc-include-cleaner)
      Tcl_CreateCommand(
          interp_, "exit", &WebServer::tclExitHandler, this, nullptr);
    }

    // viewer_hook_ and the WebLogSink were created by initLogger() above.
    // Install the hook as the Gui headless viewer and chart factory now
    // that the network is about to open — deferred from initLogger() so
    // startup scripts run with web::Gui::enabled() == false (see
    // initLogger()).
    web::Gui::get()->setHeadlessViewer(viewer_hook_.get());
    web::Gui::get()->setChartFactory(
        [hook = viewer_hook_.get()](const std::string& name,
                                    const std::string& x_label,
                                    const std::vector<std::string>& y_labels) {
          return hook->createChart(name, x_label, y_labels);
        });

    // Flush WebLogSink at the end of every Tcl eval so log output
    // emitted during a command reaches clients before the response
    // carrying the Tcl result.  viewer_hook_ outlives every io thread
    // that can run a request handler (stop() joins io threads before
    // resetting viewer_hook_), so the raw pointer capture is safe.
    tcl_eval->drain_output
        = [hook = viewer_hook_.get()]() { hook->drainLogs(); };

    // The renderer bridge: one struct so both halves are installed and, in
    // stop(), cleared together.
    TileGenerator::RendererHooks hooks;

    hooks.draw = [weak_gen = std::weak_ptr<TileGenerator>(generator_),
                  hook = viewer_hook_.get()](std::vector<unsigned char>& image,
                                             const TileFrame& frame,
                                             bool debug_live,
                                             odb::dbTechLayer* layer) {
      if (hook == nullptr) {
        return;
      }
      auto gen = weak_gen.lock();
      if (!gen) {
        return;
      }
      if (!debug_live && !hook->isPaused()) {
        return;
      }
      seedRendererControls(hook);
      for (web::Renderer* renderer : web::Gui::get()->renderers()) {
        // A Renderer sees the tile as a whole-DBU window (Painter's API is
        // integer DBU); only the rasterization below needs the exact
        // origin, which it takes from the frame.
        WebPainter painter(frame.cull, frame.scale);
        // The Qt GUI's two passes: drawLayer once per tech layer
        // (RenderThread::drawLayer) and drawObjects once, after the
        // layers.  A renderer may implement either or both -- four of the
        // six that draw per layer implement no drawObjects at all, so
        // skipping the layer pass made them invisible here.  saveState /
        // restoreState around it mirrors Qt, so a renderer that leaves a
        // pen set cannot bleed into the next one.
        painter.saveState();
        if (layer != nullptr) {
          renderer->drawLayer(layer, painter);
        } else {
          renderer->drawObjects(painter);
        }
        painter.restoreState();
        gen->rasterizeWebPainterOps(image, painter.ops(), frame);
      }
    };

    // Answered only while the run is paused — a stricter gate than the
    // drawing above, which also honours "Live".  These implementations read
    // live algorithm state AND write their own (GraphicsImpl::select walks
    // nbc_->getGCells(), indexes it, and sets selected_; DebugGui::select
    // queries the solver's rtrees and fills selected_shapes_), and the Qt GUI
    // only ever reaches them from inside web::pause(), which spins the event
    // loop while the algorithm is blocked.  Reading a torn frame is a garbled
    // overlay; indexing a vector mid-reallocation is a crash, and clicking a
    // gcell that is still moving buys nothing — so Live does not extend here.
    hooks.select = [hook = viewer_hook_.get()](
                       odb::dbTechLayer* layer,
                       const odb::Rect& region,
                       std::vector<SelectionResult>& out) {
      if (hook == nullptr || !hook->isPaused()) {
        return;
      }
      for (web::Renderer* renderer : web::Gui::get()->renderers()) {
        for (const web::Selected& selected : renderer->select(layer, region)) {
          odb::Rect bbox;
          if (!selected.getBBox(bbox)) {
            // Nothing to zoom to or highlight; the client keys its
            // selection off the bbox, so skip rather than send a degenerate
            // rectangle.
            continue;
          }
          out.push_back({selected.getObject(),
                         selected.getName(),
                         selected.getTypeName(),
                         bbox,
                         odb::dbTransform(),
                         /*is_inst=*/false});
        }
      }
    };

    TileGenerator::setRendererHooks(std::move(hooks));

    // After a design edit invalidates the tile cache, push a refresh so every
    // connected client re-requests its tiles (mirrors the Qt GUI's repaint on
    // Search::modified).  Fired on the design-mutation thread; broadcast() is
    // safe from any thread (it posts writes onto each session's strand).
    generator_->setDesignChangedCallback([hook = viewer_hook_.get()]() {
      if (hook == nullptr) {
        return;
      }
      hook->sessions().broadcast(R"({"type":"refresh"})");
    });

    // Who may reach the port decides who may run Tcl here; see
    // BindAddressKind (issue #11167).
    const std::string bind_to
        = bind_address.empty() ? kDefaultBindAddress : bind_address;
    const BindAddressKind bind_kind = classifyBindAddress(bind_to);
    if (bind_kind == BindAddressKind::kInvalid) {
      // noreturn: the catch below tears the half-built server down.
      logger_->error(utl::WEB,
                     79,
                     "Invalid bind address \"{}\"; {}.",
                     bind_to,
                     kBindAddressHint);
    }
    if (bind_kind == BindAddressKind::kExposed) {
      logger_->warn(utl::WEB,
                    80,
                    "Web server bound to {}, reachable beyond this machine. "
                    "The viewer runs Tcl commands as this user; the access "
                    "token in the URL is all that stands in the way, and it "
                    "travels in clear text over HTTP. Prefer the default "
                    "loopback bind with an SSH tunnel.",
                    bind_to);
    }
    auto const address = net::ip::make_address(bind_to);  // validated above
    uint16_t const u_port = port;
    int const num_threads = num_threads_;

    // Bound how many requests the client keeps in flight at once. Scale with
    // the server's I/O worker count (the threads that actually service
    // requests) so the window tracks the configured thread budget, with an
    // absolute cap so a many-core box doesn't hand out an unbounded window.
    // Announced to the client on connect; see WebSocketSession::on_accept.
    int const max_in_flight = std::clamp(num_threads * 4, 16, 256);

    ioc_ = std::make_unique<net::io_context>(num_threads);

    // No token, no server: without the gate, whoever reaches the port gets the
    // interpreter.
    std::string token = generateAuthToken();
    std::string ticket = generateAuthToken();
    if (token.empty() || ticket.empty()) {
      // noreturn: the catch below tears the half-built server down.
      logger_->error(utl::WEB,
                     124,
                     "Could not read /dev/urandom to mint an access token.");
    }
    auth_ = std::make_shared<SessionAuth>(
        std::move(token), std::move(ticket), kTicketTtl);

    auto handle = createAndRunListener(*ioc_,
                                       Tcp::endpoint{address, u_port},
                                       generator_,
                                       tcl_eval,
                                       timing_report,
                                       clock_report,
                                       logger_,
                                       viewer_hook_.get(),
                                       max_in_flight,
                                       auth_);
    shutdown_listener_ = std::move(handle.shutdown);

    // Point the browser at something it can actually reach.  The launch uses
    // the ticket url, so the token never reaches a command line.
    const std::string origin = "http://" + browserHostForBind(address) + ":"
                               + std::to_string(handle.port);
    const std::string target = "/?token=" + auth_->token();
    const std::string url = origin + target;
    const std::string launch_url = origin + "/?ticket=" + auth_->ticket();

    // Bind the timer to a strand so all timer operations (expires_after,
    // async_wait, cancel) run serialized on a single io thread.  Without
    // this, stop()'s cancel from the caller thread would race with the
    // async_wait handler rescheduling on a worker thread — the same
    // steady_timer cannot be safely mutated from multiple threads.  The
    // initial scheduleLogDrain() below runs before io threads start, so
    // it is single-threaded by construction.
    log_drain_timer_ = std::make_unique<net::steady_timer>(
        net::make_strand(ioc_->get_executor()));
    scheduleLogDrain();

    // Error file for the browser launcher below.  Created here, before the
    // io threads start, because umask() is process-wide; mkstemp already
    // creates the file 0600 and the clamp only pins it for static analysis.
    char tmp_filename[] = "/tmp/openroad-XXXXXX";
    const mode_t old_umask = umask(S_IRWXG | S_IRWXO);
    const int fd = mkstemp(tmp_filename);
    umask(old_umask);
    std::string errfile = "/dev/null";
    if (fd != -1) {
      errfile = tmp_filename;
      close(fd);
    }

    threads_.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
      threads_.emplace_back([this] { runIoContext(*ioc_, logger_); });
    }

    const std::string host_name = thisHostName();
    const std::string reach_hint
        = reachabilityHint(bind_kind, address, host_name, handle.port, target);
    logger_->info(utl::WEB, 1, "Server started on {}\n{}", url, reach_hint);

    const BrowserEnv env = currentBrowserEnv(logger_, host_name, launch);
    const LaunchSkip skip = browserLaunchSkipReason(launch, env);
    // A ticket stays live only while a browser we launched may redeem it.
    if (skip != LaunchSkip::kNone) {
      const bool overridable
          = skip != LaunchSkip::kFlag && skip != LaunchSkip::kOptOut;
      logger_->info(utl::WEB,
                    120,
                    "Not launching a browser here ({}); open the url above.{}",
                    browserSkipReasonText(skip),
                    overridable
                        ? "\nPass -browser (-web_browser) to launch one anyway."
                        : "");
      auth_->revokeTicket();
    } else if (!launchBrowser(logger_,
                              launch_url,
                              errfile,
                              launchesThroughBrowserEnv(env))) {
      auth_->revokeTicket();
    }
    if (fd != -1) {
      std::error_code err_ignored;
      std::filesystem::remove(errfile, err_ignored);
    }
  } catch (std::exception const& e) {
    stop();
    logger_->error(utl::WEB, 2, "Server error : {}", e.what());
  }
}

void WebServer::waitForStop()
{
  std::unique_lock<std::mutex> lock(stop_mutex_);
  stop_cv_.wait(lock, [this] { return stop_requested_; });
  stop_requested_ = false;
  lock.unlock();

  // Notify connected browsers so they can show "Server stopped" and
  // disable auto-reconnect.  broadcastAndWait() waits for the write to
  // complete before stop() tears down the io_context.
  if (viewer_hook_) {
    constexpr auto kShutdownFlushTimeout = std::chrono::seconds(2);
    viewer_hook_->sessions().broadcastAndWait(R"({"type":"shutdown"})",
                                              kShutdownFlushTimeout);
  }

  stop();
}

int WebServer::tclExitHandler(ClientData clientData,
                              Tcl_Interp* interp,
                              int /*argc*/,
                              const char* /*argv*/[])
{
  auto* self = static_cast<WebServer*>(clientData);
  self->exit_requested_ = true;
  // Wake waitForStop() on the main thread so it can join the worker
  // threads (including the one currently executing this handler) and
  // exit cleanly via std::exit() from the main thread.
  self->requestStop();
  Tcl_SetResult(interp, const_cast<char*>(kExitResultMsg), TCL_STATIC);
  return TCL_ERROR;
}

// Drain interval chosen to keep the browser console feeling live without
// burning CPU when nothing is logged.  An idle tick costs one mutex
// acquire + empty-buffer check inside WebLogSink::drainToClients.
static constexpr auto kLogDrainInterval = std::chrono::milliseconds(250);

void WebServer::scheduleLogDrain()
{
  if (!log_drain_timer_) {
    return;
  }
  log_drain_timer_->expires_after(kLogDrainInterval);
  log_drain_timer_->async_wait([this](const boost::system::error_code& ec) {
    // operation_aborted means cancel() was called from stop().  Any
    // other error (or none) means the timer fired normally — drain and
    // reschedule.  ioc_->stop() in stop() will discard a re-armed timer
    // before its next firing, so no UAF risk on shutdown.
    if (ec == net::error::operation_aborted) {
      return;
    }
    if (viewer_hook_) {
      viewer_hook_->drainLogs();
    }
    scheduleLogDrain();
  });
}

void WebServer::requestStop()
{
  if (!isRunning()) {
    logger_->warn(utl::WEB, 36, "Web server is not running.");
    return;
  }
  {
    std::lock_guard<std::mutex> lock(stop_mutex_);
    stop_requested_ = true;
  }
  stop_cv_.notify_one();
}

void WebServer::stop()
{
  // Restore the original Tcl `exit` command before tearing down — pairs
  // with the rename in serve().  Skip if not installed (stop() is safe
  // to call multiple times).
  if (Tcl_FindCommand(interp_, kRenamedExitCmd, nullptr, 0) != nullptr) {
    Tcl_DeleteCommand(interp_, "exit");
    const std::string restore
        = std::string("rename ") + kRenamedExitCmd + " exit";
    Tcl_Eval(interp_, restore.c_str());
  }

  if (viewer_hook_) {
    TileGenerator::setRendererHooks({});
    if (generator_) {
      generator_->setDesignChangedCallback({});
    }
    if (web::Gui::get()->getHeadlessViewer() == viewer_hook_.get()) {
      web::Gui::get()->setHeadlessViewer(nullptr);
    }
    web::Gui::get()->setChartFactory({});
  }
  if (log_sink_) {
    logger_->removeSink(log_sink_);
    log_sink_.reset();
  }

  if (shutdown_listener_) {
    shutdown_listener_();
    shutdown_listener_ = {};
  }
  // Cancel the periodic log drain so its handler stops re-arming.  The
  // cancel is posted onto the timer's strand so it runs serialized with
  // scheduleLogDrain — calling cancel() directly from the caller thread
  // would race with the async_wait handler mutating the timer on an io
  // thread.  Any in-flight handler completes normally; a re-armed timer
  // is discarded by ioc_->stop() below.
  if (log_drain_timer_) {
    auto* timer = log_drain_timer_.get();
    net::post(timer->get_executor(), [timer] { timer->cancel(); });
  }
  stopAndJoinIoThreads();
  // Reset only after threads are joined so no handler can dereference
  // the timer mid-shutdown.
  log_drain_timer_.reset();
  // Release without destroying — destroying io_context can crash on
  // residual async handlers. Leak is bounded (at most one io_context
  // per serve/stop cycle).
  (void) ioc_.release();  // NOLINT(bugprone-unused-return-value)
  generator_.reset();
  // Remove the log sink before destroying viewer_hook_ — the sink
  // stores a raw pointer into it and the CLI thread may emit a log
  // line at any moment.
  if (log_sink_) {
    logger_->removeSink(log_sink_);
    log_sink_.reset();
  }
  viewer_hook_.reset();
  // Reset so a subsequent serve()/initLogger() re-registers the sink.
  logger_initialized_ = false;
  // A restarted server mints new credentials, so links to the old ones stop
  // working rather than outliving the session they were issued for.
  auth_.reset();
  logger_->info(utl::WEB, 41, "Web session closed.");
}

}  // namespace web
