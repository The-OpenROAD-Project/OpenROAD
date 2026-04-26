// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <functional>
#include <memory>
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

namespace web {

struct Color;
class Search;

class ClockTreeReport;
class TileGenerator;
struct TclEvaluator;
class TimingReport;
class WebViewerHook;

// Factory that creates, starts, and returns a shutdown callback for a
// Listener.  Defined in web.cpp (where Listener is local); called from
// web_serve.cpp.
std::function<void()> createAndRunListener(
    boost::asio::io_context& ioc,
    const boost::asio::ip::tcp::endpoint& endpoint,
    std::shared_ptr<TileGenerator> generator,
    std::shared_ptr<TclEvaluator> tcl_eval,
    std::shared_ptr<TimingReport> timing_report,
    std::shared_ptr<ClockTreeReport> clock_report,
    const std::string& doc_root,
    utl::Logger* logger,
    WebViewerHook* viewer_hook);

// A layout web server.  serve() starts the server in background threads
// and returns immediately so the Tcl interpreter remains interactive
// (analogous to -gui for the Qt frontend).

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
  void serve(int port, const std::string& doc_root);

  // True after serve() returns and before stop/destructor.
  bool isRunning() const { return ioc_ != nullptr; }

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

 private:
  // Tears down the I/O threads and cleans up hooks.  Called by the
  // destructor; safe to call multiple times.
  void stop();

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

  // Type-erased callback that closes the Listener's acceptor before the
  // io_context is destroyed — prevents a crash where the acceptor's
  // destructor references the io_context that's mid-destruction.
  std::function<void()> shutdown_listener_;
};

}  // namespace web
