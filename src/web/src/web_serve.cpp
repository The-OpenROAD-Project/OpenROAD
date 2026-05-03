// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// WebServer::serve() and stop() in their own translation unit so that
// test executables linking libweb.a don't pull in gui::Gui::get()
// references (which would require the full gui library including Qt
// SWIG wrappers and ord::OpenRoad symbols).

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "boost/beast/core.hpp"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "boost/beast/websocket.hpp"
#include "clock_tree_report.h"
#include "gui/gui.h"
#include "json_builder.h"
#include "odb/geom.h"
#include "request_handler.h"
#include "spdlog/sinks/base_sink.h"
#include "tcl.h"
#include "tile_generator.h"
#include "timing_report.h"
#include "utl/Logger.h"
#include "web/web.h"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "web_chart.h"
#include "web_painter.h"
#include "web_viewer_hook.h"

namespace web {

namespace net = boost::asio;
using Tcp = net::ip::tcp;

// Tcl command name used to stash the original `exit` while our override
// is installed.  Mirrors gui::TclCmdInputWidget's kCommandRenamePrefix.
static constexpr const char* kRenamedExitCmd = "::tcl::openroad::web_orig_exit";

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
    hook_->sessions().broadcast(R"({"type":"log","text":")"
                                + json_escape(pending_) + R"("})");
    pending_.clear();
  }

  WebViewerHook* hook_;
  std::string pending_;
};

void WebServer::serve(int port)
{
  if (ioc_) {
    logger_->warn(utl::WEB, 6, "Web server is already running.");
    return;
  }

  // Clear any stale stop request left over from a previous session.
  // Without this, a requestStop() that arrives during teardown (after
  // waitForStop() cleared the flag but before stop() finishes) would
  // cause the next waitForStop() to return immediately.
  {
    std::lock_guard<std::mutex> lock(stop_mutex_);
    stop_requested_ = false;
  }

  try {
    generator_ = std::make_shared<TileGenerator>(db_, sta_, logger_);
    auto timing_report = std::make_shared<TimingReport>(sta_);
    auto clock_report = std::make_shared<ClockTreeReport>(sta_);

    auto tcl_eval = std::make_shared<TclEvaluator>(interp_, logger_);

    // Override Tcl's `exit` so a user typing `exit` in the browser tcl
    // widget doesn't run Tcl_Exit on the worker thread (which triggers
    // ~WebServer's self-join → std::terminate).  Same pattern as
    // gui::TclCmdInputWidget.  The handler signals waitForStop() and
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

    viewer_hook_ = std::make_unique<WebViewerHook>();
    gui::Gui::get()->setHeadlessViewer(viewer_hook_.get());
    gui::Gui::get()->setChartFactory(
        [hook = viewer_hook_.get()](const std::string& name,
                                    const std::string& x_label,
                                    const std::vector<std::string>& y_labels) {
          return hook->createChart(name, x_label, y_labels);
        });

    auto log_sink = std::make_shared<WebLogSink>(viewer_hook_.get());
    log_sink_ = log_sink;
    logger_->addSink(log_sink_);
    viewer_hook_->setDrainLogsFn(
        [log_sink = std::move(log_sink)]() { log_sink->drainToClients(); });

    TileGenerator::setDebugOverlayCallback(
        [weak_gen = std::weak_ptr<TileGenerator>(generator_),
         hook = viewer_hook_.get()](std::vector<unsigned char>& image,
                                    const odb::Rect& dbu_tile,
                                    double scale,
                                    bool debug_live) {
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
          for (gui::Renderer* renderer : gui::Gui::get()->renderers()) {
            WebPainter painter(dbu_tile, scale);
            renderer->drawObjects(painter);
            gen->rasterizeWebPainterOps(image, painter.ops(), dbu_tile, scale);
          }
        });

    auto const address = net::ip::make_address("0.0.0.0");
    uint16_t const u_port = port;
    int const num_threads = num_threads_;

    ioc_ = std::make_unique<net::io_context>(num_threads);

    auto handle = createAndRunListener(*ioc_,
                                       Tcp::endpoint{address, u_port},
                                       generator_,
                                       tcl_eval,
                                       timing_report,
                                       clock_report,
                                       logger_,
                                       viewer_hook_.get());
    shutdown_listener_ = std::move(handle.shutdown);

    const std::string url = "http://localhost:" + std::to_string(handle.port);

    threads_.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
      threads_.emplace_back([this] { ioc_->run(); });
    }

#if defined(__APPLE__)
    std::string open_cmd = "open " + url + " > /dev/null 2>&1";
#elif defined(_WIN32)
    std::string open_cmd = "start " + url + " > nul 2>&1";
#else
    // `setsid -f` forks the launcher into a new session, severing the
    // SIGHUP cascade from openroad's controlling pty.  Without this,
    // running openroad from inside an emacs shell-mode buffer kills
    // the browser tab as soon as openroad exits, because emacs holds
    // the pty master and SIGHUPs every process in the session.  Also
    // redirect stdin from /dev/null so xdg-open never blocks on input
    // inherited from the pty.
    std::string open_cmd
        = "setsid -f xdg-open " + url + " < /dev/null > /dev/null 2>&1";
#endif
    int ret = std::system(open_cmd.c_str());
    (void) ret;

    logger_->info(utl::WEB, 1, "Server started on {}.", url);

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
    TileGenerator::setDebugOverlayCallback({});
    if (gui::Gui::get()->getHeadlessViewer() == viewer_hook_.get()) {
      gui::Gui::get()->setHeadlessViewer(nullptr);
    }
    gui::Gui::get()->setChartFactory({});
  }
  if (log_sink_) {
    logger_->removeSink(log_sink_);
    log_sink_.reset();
  }

  if (shutdown_listener_) {
    shutdown_listener_();
    shutdown_listener_ = {};
  }
  stopAndJoinIoThreads();
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
  logger_->info(utl::WEB, 41, "Web session closed.");
}

}  // namespace web
