// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// WebServer and listener setup.  Separating this from web.cpp keeps
// the test executable (which links web_test_support) free of the
// network listener, including its transitive dependency on
// gui::Gui::get() from the browser-open code path.

#include "web/web.h"

#include <netinet/in.h>

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
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
#include "boost/json/array.hpp"
#include "boost/json/object.hpp"
#include "boost/json/serialize.hpp"
#include "boost/json/value.hpp"
#include "clock_tree_report.h"
#include "gui/heatMap.h"
#include "hierarchy_report.h"
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

// =============================================================================
// Listen — create, bind, and start an acceptor on the given endpoint.
// =============================================================================
//
//  1.  Create a tcp::acceptor.
//  2.  Open+set_option (reuse_address).
//  3.  Bind to the configured endpoint and start listening.
//  4.  Use the listener's strand so accept, read, and write callbacks
//      all run on the same io thread — the producer side of the
//      shared-state handoff is already serialized by the strand.
//  5.  Return an opaque handle containing the listener and a port number
//      so the caller (WebServer::serve) can own the listener and stop it
//      later.
//  6.  Start the first async_accept immediately.
//
//  The single-threaded io_context can handle thousands of mostly-idle
//  WebSocket clients — each client has one outstanding read, plus the
//  occasional write.  One thread is enough.
//
//  To use more threads, pass num_threads=1 to io_context() — Qt's
//  main-thread-only pixel-buffer access means every tile render and
//  screen-snapshot must be posted to the main thread.
//

std::pair<net::io_context::work, net::steady_timer::duration>
checkTimerArgs(Tcl_Interp* interp);

struct Listener : public std::enable_shared_from_this<Listener>
{
  Listener(net::io_context& ioc,
           const net::ip::tcp::endpoint& endpoint,
           RequestHandlerProvider& provider)
      : acceptor_(net::make_strand(ioc)), provider_(provider)
  {
    beast::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      logger_->error(
          utl::WEB, 15, "Failed to open acceptor: {}", ec.message());
    }

    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
      logger_->error(
          utl::WEB, 16, "Failed to set reuse_address: {}", ec.message());
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
      logger_->error(
          utl::WEB, 17, "Failed to bind to endpoint: {}", ec.message());
    }

    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
      logger_->error(
          utl::WEB, 18, "Failed to listen on endpoint: {}", ec.message());
    }
  }

  void startAccept() { doAccept(); }

 private:
  net::ip::tcp::acceptor acceptor_;
  std::reference_wrapper<RequestHandlerProvider> provider_;
  utl::Logger* logger_ = nullptr;

  void doAccept();
};

// =============================================================================
// Browser-open helper
// =============================================================================

static void openBrowser(std::string url)
{
  if (url.empty()) {
    return;
  }

#if defined(__APPLE__)
  std::string cmd = "open " + url + " 2>/dev/null";
#else
  std::string cmd = "xdg-open " + url + " 2>/dev/null &";
#endif
  // Redirect stdin from /dev/null so xdg-open never blocks on input
  // inherited from the pty.
  std::string open_cmd
      = cmd + " < /dev/null";
  std::call_once(browser_launch_flag_, [&open_cmd]() {
    // Best-effort: failures are non-fatal (e.g. no display).
    std::thread([open_cmd]() {
      FILE* f = popen(open_cmd.c_str(), "r");
      if (f) {
        pclose(f);
      }
    }).detach();
  });
}

// =============================================================================
// WebServer::serve
// =============================================================================

WebServer::Handle WebServer::serve(const int port)
{
  stop_requested_ = false;

  // Build handler objects once and share them across all sessions.
  auto generator = std::make_shared<TileGenerator>(block_);
  auto timing_report = std::make_shared<TimingReport>(sta_);
  auto clock_report = std::make_shared<ClockTreeReport>(block_, sta_);
  auto tcl_eval = std::make_shared<TclEvaluator>(interp_, logger_);

  // Override Tcl's `exit` so a user typing `exit` in the browser tcl
  // widget doesn't run Tcl_Exit on the worker thread (which triggers
  // join and would deadlock).  Instead WebServer::tclExitHandler
  // sets exit_requested_; the main thread does the real exit.
  // TclHandler::handleTclEval detects kExitResultMsg in the Tcl
  // result and sends `action: "shutdown"` to the browser.
  exit_requested_ = false;
  {
    const std::string rename_orig
        = std::string("rename exit ") + kRenamedExitCmd;
    Tcl_Eval(interp_, rename_orig.c_str());
    Tcl_CreateCommand(
        interp_, "exit", &WebServer::tclExitHandler, this, nullptr);
  }

  auto log_sink = std::make_shared<WebLogSink>(viewer_hook_.get());
  logger_->addSink(log_sink);

  // Flush WebLogSink at the end of every Tcl eval so log output
  // emitted during a command reaches clients before the response
  // carrying the Tcl result.  viewer_hook_ outlives every io thread
  // that can run a request handler (stop() joins io threads before
  // resetting viewer_hook_), so the raw pointer capture is safe.
  tcl_eval->drain_output
      = [hook = viewer_hook_.get()]() { hook->drainLogs(); };

  TileGenerator::setDebugOverlayCallback(
      [hook = viewer_hook_.get()](std::vector<unsigned char>& image,
                                  int width,
                                  int height) {
        if (hook->debugOverlay()) {
          return;
        }
        hook->remoteDebug()->setDebugImage(image, width, height);
      });

  // Restrict to localhost for security — the Tcl interpreter has no
  // authentication and accepts arbitrary commands.
  auto const address = net::ip::make_address("127.0.0.1");
  uint16_t const u_port = port;
  int const num_threads = num_threads_;

  ioc_ = std::make_unique<net::io_context>(num_threads);
  auto listener = std::make_shared<Listener>(
      *ioc_,
      net::ip::tcp::endpoint{address, u_port},
      *this);

  // Build the request handler provider.
  setupHandlers(generator,
                timing_report,
                clock_report,
                tcl_eval,
                logger_,
                viewer_hook_);

  listener->startAccept();

  // 1 work guard + N-1 threads (the caller is the Nth thread).
  work_guard_.emplace(net::make_work_guard(*ioc_));
  for (int i = 1; i < num_threads; ++i) {
    thread_group_.emplace_back([this] { ioc_->run(); });
  }

  // Browser launch and log drain timer.
  auto const port_str = std::to_string(listener->port());
  const std::string url = "http://127.0.0.1:" + port_str;
  openBrowser(url);

  // Drain the WebLogSink periodically so clients watching logs on a
  // design that never runs Tcl still see output.
  log_drain_timer_ = std::make_unique<net::steady_timer>(
      net::make_strand(ioc_->get_executor()));
  scheduleLogDrain();

  return {.shutdown = [listener]() { listener->close(); },
          .port = listener->port()};
}