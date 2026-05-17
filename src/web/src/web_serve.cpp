// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// WebServer::serve() and stop() in their own translation unit so that
// test executables linking libweb.a don't pull in gui::Gui::get()
// references (which would require the full gui library including Qt
// SWIG wrappers and ord::OpenRoad symbols).

#include <cerrno>
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

#include "boost/asio/error.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/post.hpp"
#include "boost/asio/steady_timer.hpp"
#include "boost/asio/strand.hpp"
#include "boost/system/error_code.hpp"
#include "tclDecls.h"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "boost/beast/core.hpp"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "boost/beast/websocket.hpp"
#include "boost/json/object.hpp"
#include "boost/json/serialize.hpp"
#include "clock_tree_report.h"
#include "gui/gui.h"
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
    boost::json::object msg;
    msg["type"] = "log";
    msg["text"] = pending_;
    hook_->sessions().broadcast(boost::json::serialize(msg));
    pending_.clear();
  }

  WebViewerHook* hook_;
  std::string pending_;
};

// Tcl stdout / stderr capture --------------------------------------------
//
// Tcl `puts` and other commands write to Tcl's stdout/stderr channels,
// not through utl::Logger, so WebLogSink never sees them.  We push a
// transform onto each channel via Tcl_StackChannel: the transform's
// outputProc forwards bytes to the underlying channel (so the launching
// terminal still sees them) AND broadcasts them to connected browsers
// as a `{"type":"log","text":...}` push frame so the browser Tcl console
// shows the same output stream.  Pattern mirrors src/sta/util/ReportTcl.cc.
//
// Per-channel state pinned for the lifetime of the stack — the bottom-
// of-stack channel pointer is needed by the outputProc to forward bytes
// downward (Tcl_GetStackedChannel exists in newer Tcls but we save the
// channel explicitly to match the STA pattern and avoid version churn).

struct WebChannelMirror
{
  WebServer* server;
  Tcl_Channel parent;
};

extern "C" {

static int webChannelOutputProc(ClientData instanceData,
                                const char* buf,
                                int toWrite,
                                int* errorCodePtr)
{
  auto* m = static_cast<WebChannelMirror*>(instanceData);

  // Forward to the next channel in the stack so the terminal still
  // sees the output.
  const Tcl_ChannelType* parent_type = Tcl_GetChannelType(m->parent);
  Tcl_DriverOutputProc* parent_output = Tcl_ChannelOutputProc(parent_type);
  ClientData parent_data = Tcl_GetChannelInstanceData(m->parent);
  const int written = parent_output(
      parent_data, const_cast<char*>(buf), toWrite, errorCodePtr);

  // Only broadcast bytes that actually reached the underlying channel —
  // a short write means the rest didn't make it to the terminal, so we
  // shouldn't pretend the browser saw them either.
  if (m->server != nullptr && written > 0) {
    m->server->broadcastChannelChunk(std::string_view(buf, written));
  }
  return written;
}

static int webChannelInputProc(ClientData /*instanceData*/,
                               char* /*buf*/,
                               int /*bufSize*/,
                               int* errorCodePtr)
{
  *errorCodePtr = EINVAL;
  return -1;
}

static int webChannelSetOptionProc(ClientData /*instanceData*/,
                                   Tcl_Interp* /*interp*/,
                                   const char* /*optionName*/,
                                   const char* /*value*/)
{
  return TCL_OK;
}

static int webChannelGetOptionProc(ClientData /*instanceData*/,
                                   Tcl_Interp* /*interp*/,
                                   const char* /*optionName*/,
                                   Tcl_DString* /*dsPtr*/)
{
  return TCL_OK;
}

static void webChannelWatchProc(ClientData /*instanceData*/, int /*mask*/)
{
}

static int webChannelGetHandleProc(ClientData /*instanceData*/,
                                   int /*direction*/,
                                   ClientData* /*handlePtr*/)
{
  return TCL_ERROR;
}

static int webChannelBlockModeProc(ClientData /*instanceData*/, int /*mode*/)
{
  return 0;
}

static int webChannelClose2Proc(ClientData /*instanceData*/,
                                Tcl_Interp* /*interp*/,
                                int /*flags*/)
{
  return 0;
}

}  // extern "C"

static const Tcl_ChannelType web_channel_type = {
    .typeName = "web_mirror",
    .version = TCL_CHANNEL_VERSION_5,
    .closeProc = nullptr,  // closeProc unused with close2Proc
    .inputProc = webChannelInputProc,
    .outputProc = webChannelOutputProc,
    .seekProc = nullptr,
    .setOptionProc = webChannelSetOptionProc,
    .getOptionProc = webChannelGetOptionProc,
    .watchProc = webChannelWatchProc,
    .getHandleProc = webChannelGetHandleProc,
    .close2Proc = webChannelClose2Proc,
    .blockModeProc = webChannelBlockModeProc,
    .flushProc = nullptr,
    .handlerProc = nullptr,
    .wideSeekProc = nullptr,
    .threadActionProc = nullptr,
    .truncateProc = nullptr,
};

void WebServer::broadcastChannelChunk(std::string_view chunk)
{
  if (!viewer_hook_ || chunk.empty()) {
    return;
  }
  // Reuse the existing `log` frame type so the browser Tcl console
  // appends with the same styling as logger output.  The browser strips
  // a single trailing newline before re-appending one.
  boost::json::object msg;
  msg["type"] = "log";
  msg["text"] = std::string(chunk);
  viewer_hook_->sessions().broadcast(boost::json::serialize(msg));
}

// Tcl_CreateObjTrace callback (level=1) that broadcasts each top-level
// command on the main thread to connected browsers as a console_input
// echo.  This is what makes the rest of an `openroad foo.tcl` script
// after `web_server` show up in the browser Tcl console "as if user
// input".  We deliberately skip worker-thread Tcl_Evals (browser-typed
// tcl_eval requests run via TclEvaluator on a worker, also at depth 1)
// because main.js already locally echoes those — broadcasting from the
// trace would render `>>> cmd` twice in the browser that typed it.
static int objTraceProc(ClientData clientData,
                        Tcl_Interp* /*interp*/,
                        int /*level*/,
                        const char* command,
                        Tcl_Command /*cmd*/,
                        int /*objc*/,
                        Tcl_Obj* const /*objv*/[])
{
  if (command == nullptr) {
    return TCL_OK;
  }
  auto* server = static_cast<WebServer*>(clientData);
  if (Tcl_GetCurrentThread() != server->mainThreadId()) {
    return TCL_OK;
  }
  server->broadcastConsoleInput(command);
  return TCL_OK;
}

void WebServer::broadcastConsoleInput(const std::string& cmd)
{
  if (!viewer_hook_) {
    return;
  }
  std::string trimmed = cmd;
  // Drop trailing newline / whitespace that comes from script lines.
  while (!trimmed.empty()
         && (trimmed.back() == '\n' || trimmed.back() == '\r'
             || trimmed.back() == ' ' || trimmed.back() == '\t')) {
    trimmed.pop_back();
  }
  if (trimmed.empty()) {
    return;
  }
  boost::json::object msg;
  msg["type"] = "console_input";
  msg["text"] = trimmed;
  viewer_hook_->sessions().broadcast(boost::json::serialize(msg));
}

// No-op event used to wake a parked Tcl_DoOneEvent on the main thread
// after a worker sets exit_requested_ / stop_requested_.  The wait loop
// re-checks the flags after each event.
static int wakeEventProc(Tcl_Event* /*ev*/, int /*flags*/)
{
  return 1;
}

void WebServer::wakeMainEventLoop()
{
  if (main_thread_id_ == nullptr) {
    return;
  }
  auto* ev = static_cast<Tcl_Event*>(ckalloc(sizeof(Tcl_Event)));
  ev->proc = &wakeEventProc;
  ev->nextPtr = nullptr;
  Tcl_ThreadQueueEvent(main_thread_id_, ev, TCL_QUEUE_TAIL);
  Tcl_ThreadAlert(main_thread_id_);
}

void WebServer::serve(int port)
{
  if (ioc_) {
    logger_->warn(utl::WEB, 6, "Web server is already running.");
    return;
  }

  exit_requested_ = false;
  stop_requested_ = false;
  main_thread_id_ = Tcl_GetCurrentThread();

  try {
    generator_ = std::make_shared<TileGenerator>(db_, sta_, logger_);
    auto timing_report = std::make_shared<TimingReport>(sta_);
    auto clock_report = std::make_shared<ClockTreeReport>(sta_);

    auto tcl_eval = std::make_shared<TclEvaluator>(
        interp_, logger_, tcl_mutex_, main_thread_id_);

    // Override Tcl's `exit` so a user typing `exit` in the browser tcl
    // widget doesn't run Tcl_Exit on the worker thread (which triggers
    // ~WebServer's self-join → std::terminate).  The handler signals
    // the main thread via requestExit(); runEventLoopUntilStop wakes
    // and the caller (web_server_wait_cmd or Main.cc) calls stop()
    // and then std::exit on the main thread.  TclHandler::handleTclEval
    // detects kExitResultMsg in the Tcl result and sends
    // `action: "shutdown"` to the browser.
    {
      const std::string rename_orig
          = std::string("rename exit ") + kRenamedExitCmd;
      Tcl_Eval(interp_, rename_orig.c_str());
      Tcl_CreateCommand(
          interp_, "exit", &WebServer::tclExitHandler, this, nullptr);
    }

    // Level-1 command trace: broadcast each top-level command on the
    // main thread to browsers as a console_input echo.  Installed only
    // while the server is running.
    trace_token_ = Tcl_CreateObjTrace(interp_,
                                      /*level=*/1,
                                      /*flags=*/0,
                                      &objTraceProc,
                                      this,
                                      /*delProc=*/nullptr);

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
    // Flush WebLogSink at the end of every Tcl eval so log output
    // emitted during a command reaches clients before the response
    // carrying the Tcl result.  viewer_hook_ outlives every io thread
    // that can run a request handler (stop() joins io threads before
    // resetting viewer_hook_), so the raw pointer capture is safe.
    tcl_eval->drain_output
        = [hook = viewer_hook_.get()]() { hook->drainLogs(); };

    // Install Tcl_StackChannel mirrors on stdout / stderr so that
    // `puts` and similar Tcl-level output reaches the browser console
    // in addition to the launching terminal.  The transform's instance
    // data points at the channel below (the original stdout / STA
    // ReportTcl encap), so writes pass straight through after we
    // broadcast.
    Tcl_Channel stdout_chan = Tcl_GetStdChannel(TCL_STDOUT);
    if (stdout_chan != nullptr) {
      auto* mirror
          = new WebChannelMirror{.server = this, .parent = stdout_chan};
      mirror_stdout_data_ = mirror;
      mirror_stdout_chan_ = Tcl_StackChannel(
          interp_, &web_channel_type, mirror, TCL_WRITABLE, stdout_chan);
    }
    Tcl_Channel stderr_chan = Tcl_GetStdChannel(TCL_STDERR);
    if (stderr_chan != nullptr) {
      auto* mirror
          = new WebChannelMirror{.server = this, .parent = stderr_chan};
      mirror_stderr_data_ = mirror;
      mirror_stderr_chan_ = Tcl_StackChannel(
          interp_, &web_channel_type, mirror, TCL_WRITABLE, stderr_chan);
    }

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

int WebServer::tclExitHandler(ClientData clientData,
                              Tcl_Interp* interp,
                              int /*argc*/,
                              const char* /*argv*/[])
{
  auto* self = static_cast<WebServer*>(clientData);
  self->requestExit();
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

void WebServer::requestExit()
{
  if (!isRunning()) {
    return;
  }
  exit_requested_ = true;
  wakeMainEventLoop();
}

void WebServer::requestStop()
{
  // No warning if the server already stopped — common in races where
  // browser-typed `exit` (or another path) tore the server down before
  // a `web_server -stop` arrived.
  if (!isRunning()) {
    return;
  }
  stop_requested_ = true;
  wakeMainEventLoop();
}

bool WebServer::runEventLoopUntilStop()
{
  if (!isRunning()) {
    return false;
  }
  // Service timers, idle handlers, and cross-thread queued events
  // (the wakeup events posted by requestExit/requestStop), but
  // explicitly NOT TCL_FILE_EVENTS — otherwise tclreadline's stdin
  // file handler would still read lines from the launching terminal,
  // defeating the "browser is the only Tcl input surface" model when
  // web_server is invoked interactively.
  constexpr int kEventMask
      = TCL_TIMER_EVENTS | TCL_IDLE_EVENTS | TCL_WINDOW_EVENTS;
  while (!exitRequested() && !stopRequested() && isRunning()) {
    Tcl_DoOneEvent(kEventMask);
  }
  return exitRequested();
}

void WebServer::stop()
{
  // Notify browsers first so they show "Server stopped" instead of
  // attempting to reconnect.
  if (viewer_hook_) {
    constexpr auto kShutdownFlushTimeout = std::chrono::seconds(2);
    viewer_hook_->sessions().broadcastAndWait(R"({"type":"shutdown"})",
                                              kShutdownFlushTimeout);
  }

  // Stop accepting new connections.
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

  // Stop ioc_ and join workers BEFORE touching the Tcl interpreter —
  // otherwise a worker mid-Tcl_Eval would race with the trace removal
  // / exit-command restoration below.  stopAndJoinIoThreads handles
  // the self-join case (current thread is one of the workers — e.g.
  // browser-typed shutdown delivered via a worker that ends up calling
  // stop() through the atexit chain) by detaching that thread.
  stopAndJoinIoThreads();

  // Reset only after threads are joined so no handler can dereference
  // the timer mid-shutdown.
  log_drain_timer_.reset();

  // Unstack the stdout / stderr mirror channels.  Tcl_UnstackChannel
  // calls our channel type's close2Proc, which is a no-op; we then
  // free the heap-allocated instance data ourselves.
  if (mirror_stdout_chan_ != nullptr) {
    Tcl_UnstackChannel(interp_, mirror_stdout_chan_);
    mirror_stdout_chan_ = nullptr;
  }
  if (mirror_stderr_chan_ != nullptr) {
    Tcl_UnstackChannel(interp_, mirror_stderr_chan_);
    mirror_stderr_chan_ = nullptr;
  }
  delete static_cast<WebChannelMirror*>(mirror_stdout_data_);
  mirror_stdout_data_ = nullptr;
  delete static_cast<WebChannelMirror*>(mirror_stderr_data_);
  mirror_stderr_data_ = nullptr;

  // Remove the level-1 command trace.
  if (trace_token_ != nullptr) {
    Tcl_DeleteTrace(interp_, trace_token_);
    trace_token_ = nullptr;
  }

  // Restore the original Tcl `exit` command — pairs with the rename
  // in serve().
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

  // Release without destroying — destroying io_context can crash on
  // residual async handlers.  Leak is bounded (at most one io_context
  // per serve/stop cycle).
  (void) ioc_.release();  // NOLINT(bugprone-unused-return-value)
  generator_.reset();

  // Remove the log sink before destroying viewer_hook_ — the sink
  // stores a raw pointer into the hook and the CLI thread may emit a
  // log line at any moment.
  if (log_sink_) {
    logger_->removeSink(log_sink_);
    log_sink_.reset();
  }
  viewer_hook_.reset();

  main_thread_id_ = nullptr;
  exit_requested_ = false;
  stop_requested_ = false;

  logger_->info(utl::WEB, 41, "Web session closed.");
}

}  // namespace web
