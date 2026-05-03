// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <spdlog/common.h>

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "boost/asio/ip/tcp.hpp"
#include "odb/db.h"
#include "tcl.h"
#include "utl/Logger.h"

namespace boost::asio {
class io_context;
}

namespace sta {
class dbSta;
}

namespace spdlog::sinks {
class sink;
}

namespace web {

struct Color;
class Search;

class ClockTreeReport;
class TileGenerator;
struct TclEvaluator;
class TimingReport;
class WebViewerHook;

// Returned by createAndRunListener: a shutdown callback and the actual
// port the listener bound to (useful when the caller passes port 0).
struct ListenerHandle
{
  std::function<void()> shutdown;
  uint16_t port;
};

// Factory that creates, starts, and returns a handle for a Listener.
// Defined in web.cpp (where Listener is local); called from web_serve.cpp.
ListenerHandle createAndRunListener(
    boost::asio::io_context& ioc,
    const boost::asio::ip::tcp::endpoint& endpoint,
    std::shared_ptr<TileGenerator> generator,
    std::shared_ptr<TclEvaluator> tcl_eval,
    std::shared_ptr<TimingReport> timing_report,
    std::shared_ptr<ClockTreeReport> clock_report,
    utl::Logger* logger,
    WebViewerHook* viewer_hook);

// A layout web server.  serve() starts the server in background I/O
// threads; waitForStop() blocks the calling thread until requestStop()
// is called, mirroring gui::show / gui::hide.

class WebServer
{
 public:
  WebServer(odb::dbDatabase* db,
            sta::dbSta* sta,
            utl::Logger* logger,
            Tcl_Interp* interp,
            int num_threads);
  ~WebServer();

  // Start the web server on the given port.  Launches background
  // I/O threads and returns immediately.  A second call is a no-op if
  // the server is already running.
  void serve(int port);

  // True after serve() returns and before stop/destructor.
  bool isRunning() const { return ioc_ != nullptr; }

  // Block the calling thread until requestStop() is called, then
  // tear down the server.  Typically called on the main/Tcl thread.
  void waitForStop();

  // Signal waitForStop() to return.  Safe to call from any thread
  // (e.g. an ASIO worker thread executing a Tcl command).
  void requestStop();

  // True if `exit` was invoked from a Tcl command running on a worker
  // thread.  Main.cc / web.i checks this after waitForStop() returns
  // and exits the process cleanly from the main thread.
  bool exitRequested() const { return exit_requested_; }

  void saveReport(const std::string& filename,
                  int max_setup_paths,
                  int max_hold_paths);

  void saveImage(const std::string& filename,
                 int x0,
                 int y0,
                 int x1,
                 int y1,
                 int width_px,
                 double dbu_per_pixel,
                 const std::string& vis_json);

  // Tears down the I/O threads and cleans up hooks.  Safe to call multiple
  // times and from any thread; after it returns, isRunning() is false and
  // serve() may be called again to restart the server.
  void stop();

 private:
  // Stops ioc_, joins every worker thread except the current one, and
  // clears threads_. Detaches the current thread if it happens to be a
  // worker (would otherwise raise EDEADLK on self-join).
  void stopAndJoinIoThreads();

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  Tcl_Interp* interp_ = nullptr;
  int num_threads_ = 0;
  std::shared_ptr<TileGenerator> generator_;
  std::unique_ptr<WebViewerHook> viewer_hook_;

  // Background I/O context and worker threads (non-null while running).
  std::unique_ptr<boost::asio::io_context> ioc_;
  std::vector<std::thread> threads_;

  // Closes the Listener's acceptor before the io_context is destroyed,
  // avoiding a crash where the acceptor references a half-destroyed
  // io_context.
  std::function<void()> shutdown_listener_;

  // Held so stop() can remove it from the Logger before viewer_hook_ is
  // destroyed — the sink stores a raw pointer into the hook.
  spdlog::sink_ptr log_sink_;
  // Blocking support: waitForStop() sleeps on stop_cv_ until
  // requestStop() sets stop_requested_.
  std::mutex stop_mutex_;
  std::condition_variable stop_cv_;
  bool stop_requested_ = false;

  // Set by tclExitHandler when `exit` is run on a worker thread.
  bool exit_requested_ = false;

  // Tcl command override: replaces `exit` while the server is running
  // so a worker-thread `exit` doesn't run Tcl_Exit (which would self-join
  // the worker thread inside ~WebServer).
  static int tclExitHandler(ClientData clientData,
                            Tcl_Interp* interp,
                            int argc,
                            const char* argv[]);
};

}  // namespace web
