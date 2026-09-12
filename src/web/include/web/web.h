// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/steady_timer.hpp"
#include "odb/db.h"
#include "spdlog/common.h"
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
struct WebGif;  // defined in web.cpp; holds a GifEncoder + frame dimensions

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
    WebViewerHook* viewer_hook,
    int max_in_flight);

// A layout web server.  serve() starts the server in background I/O
// threads and returns immediately.  After serve(), the calling Tcl
// script (or main-thread REPL) can keep executing — each top-level
// command on the main thread is broadcast to connected browsers as a
// `console_input` echo via a Tcl_CreateObjTrace at level 1.  When the
// script settles, Main.cc parks the main thread in Tcl_DoOneEvent
// servicing browser-driven Tcl evaluations until exitRequested()
// or stopRequested() returns true; then the main thread calls
// stop() to tear down.

class WebServer
{
 public:
  WebServer(odb::dbDatabase* db,
            sta::dbSta* sta,
            utl::Logger* logger,
            Tcl_Interp* interp);
  ~WebServer();

  // Register the WebLogSink with the Logger so startup output is captured
  // (and buffered) before any client connects, without opening the network
  // or installing the headless viewer.  Idempotent and cheap; serve() calls
  // it too.  Splitting this out lets Main.cc capture read_db/script logs
  // while deferring serve() until the database is fully loaded, which avoids
  // the network threads racing the main thread's db construction.
  void initLogger();

  // Sets the number of thread workers for the server's I/O context and tile
  // generator.
  void setThreadCount(int num_threads);

  // Start the web server on the given port.  Launches background
  // I/O threads and returns immediately.  A second call is a no-op if
  // the server is already running.
  void serve(int port);

  // True after serve() returns and before stop()/destructor.
  bool isRunning() const { return ioc_ != nullptr; }

  // Tears down the I/O threads and cleans up hooks.  Safe to call
  // multiple times and from any thread (including a worker thread —
  // stopAndJoinIoThreads detaches the calling thread if it is itself
  // a worker, to avoid the EDEADLK self-join).  After it returns,
  // isRunning() is false and serve() may be called again to restart
  // the server.
  void stop();

  // Park the calling (main) thread in Tcl_DoOneEvent until exitRequested
  // or stopRequested is set.  Returns true if exit was requested
  // (caller is expected to stop() then std::exit()), false if stop
  // was requested (caller stop()s and continues).  No-op (returns false
  // immediately) if the server isn't running.
  bool runEventLoopUntilStop();

  // Worker-thread signals to wake the main thread out of Tcl_DoOneEvent.
  // requestExit() sets the exit flag; the main thread will std::exit(0)
  // after stop().  requestStop() sets the stop flag; main thread will
  // tear the server down but not exit the process.  Both queue a no-op
  // event on the main thread's Tcl event queue so a parked
  // Tcl_DoOneEvent returns and the wait loop re-checks the flags.
  void requestExit();
  void requestStop();

  bool exitRequested() const { return exit_requested_; }
  bool stopRequested() const { return stop_requested_; }

  // The Tcl thread id captured in serve().  Used by the level-1
  // command trace to filter out worker-thread Tcl_Eval invocations
  // (browser-typed tcl_eval requests) — the browser locally echoes
  // those, so we avoid a duplicate console_input broadcast.
  Tcl_ThreadId mainThreadId() const { return main_thread_id_; }

  // Lock used to serialize Tcl_Eval / STA access among worker threads.
  // TclEvaluator::eval() and the STA-touching request handlers in
  // request_handler.cpp lock this mutex.  Exposed publicly so callers
  // outside web/ that may directly evaluate Tcl on the same interp
  // can opt into the same serialization.
  std::mutex& tclMutex() { return tcl_mutex_; }

  // Broadcasts the given top-level command text to connected browsers
  // as a `{"type":"console_input","text":...}` push frame.  Called from
  // the level-1 command trace installed in serve().
  void broadcastConsoleInput(const std::string& cmd);

  // Broadcasts a chunk of bytes written to Tcl's stdout/stderr channels
  // as a `{"type":"log","text":...}` push frame.  Called from the
  // Tcl_StackChannel transform's outputProc installed in serve().
  void broadcastChannelChunk(std::string_view chunk);

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

  // User text labels (2.12), Tcl-driven (add_label/delete_label/clear_labels).
  // Delegate to the shared TileGenerator store; returns the label name.
  std::string addLabel(int x,
                       int y,
                       const std::string& text,
                       const std::string& anchor,
                       const std::string& color,
                       int size,
                       const std::string& name);
  void deleteLabel(const std::string& name);
  void clearLabels();

  // Persist the connected client's current display-controls state (as
  // synced via the "set_display_state" request) to a JSON file.  The cache
  // holds a single snapshot: with several clients connected, the state of
  // whichever client synced last is the one saved.
  void saveDisplayControls(const std::string& filename);

  // Read a display-controls JSON file and broadcast it to every connected
  // client so they re-apply the saved state.
  void restoreDisplayControls(const std::string& filename);

  // Cache a display-controls snapshot (forwarded to the viewer hook).
  // No-op if the server was never initialized.  The live
  // "set_display_state" request writes to the hook directly; this entry
  // point exists for tests and embedders.
  void setDisplayState(std::string json);

  // Custom UI registered from Tcl (create_menu_item / create_toolbar_button).
  // These are thin facades over WebViewerHook (which owns the registry and
  // broadcasts to clients), mirroring how gui::Gui delegates to MainWindow.
  // initLogger() is called first so the hook exists even when the command
  // runs from a startup script before web_server.  Returns the item key.
  std::string addToolbarButton(const std::string& name,
                               const std::string& text,
                               const std::string& script,
                               const std::string& icon,
                               const std::string& tooltip,
                               bool toggle,
                               const std::string& script_off,
                               bool echo);
  void removeToolbarButton(const std::string& name);
  std::string addMenuItem(const std::string& name,
                          const std::string& path,
                          const std::string& text,
                          const std::string& script,
                          const std::string& shortcut,
                          bool echo);
  void removeMenuItem(const std::string& name);

  // Animated-GIF export (mirrors gui's save_animated_gif).  A 3-call state
  // machine: gifStart opens a stream and returns its key; gifAddFrame captures
  // the current layout (via TileGenerator, same compositing as saveImage) as
  // one frame; gifEnd finalizes the file.  Multiple concurrent streams are
  // keyed by the returned index.  delay is in hundredths of a second.
  int gifStart(const std::string& filename);
  void gifAddFrame(std::optional<int> key,
                   const odb::Rect& region,
                   int width_px,
                   double dbu_per_pixel,
                   std::optional<int> delay,
                   const std::string& vis_json);
  void gifEnd(std::optional<int> key);

 private:
  // Wakes the main thread's Tcl event loop by queuing a no-op Tcl
  // event so a parked Tcl_DoOneEvent returns and the wait loop in
  // runEventLoopUntilStop re-checks the exit/stop flags.  Safe to
  // call from any thread; no-op if main_thread_id_ is unset.
  void wakeMainEventLoop();

  // Stops ioc_, joins every worker thread except the current one, and
  // clears threads_. Detaches the current thread if it happens to be a
  // worker (would otherwise raise EDEADLK on self-join).
  void stopAndJoinIoThreads();

  // Push the current label set to every connected client.  Labels are global
  // and live outside ODB, so nothing else notifies the other sessions that a
  // Tcl-driven add/delete/clear changed what they should draw.
  void broadcastLabels();

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  Tcl_Interp* interp_ = nullptr;
  int num_threads_ = 0;
  // Creates the tile generator on first use (the server need not be running)
  // and hands it the configured thread count.
  TileGenerator& ensureGenerator();

  std::shared_ptr<TileGenerator> generator_;
  std::unique_ptr<WebViewerHook> viewer_hook_;

  // Open animated-GIF streams, indexed by the key returned from gifStart.
  // Kept as a vector (like gui's gifs_) so multiple GIFs can record at once;
  // a finished slot is reset to nullptr rather than erased so keys stay stable.
  std::vector<std::unique_ptr<WebGif>> gifs_;
  static constexpr int kDefaultGifDelay = 250;  // hundredths of a second

  // Background I/O context and worker threads (non-null while running).
  std::unique_ptr<boost::asio::io_context> ioc_;
  std::vector<std::thread> threads_;

  // Periodic timer that drains WebLogSink so log output produced by
  // long-running Tcl commands streams to clients without waiting for a
  // debug pause/redraw or for the command to return.  Reschedules
  // itself; cancelled in stop() before ioc_ is shut down.
  std::unique_ptr<boost::asio::steady_timer> log_drain_timer_;
  void scheduleLogDrain();

  // Closes the Listener's acceptor before the io_context is destroyed,
  // avoiding a crash where the acceptor references a half-destroyed
  // io_context.
  std::function<void()> shutdown_listener_;

  // Held so stop() can remove it from the Logger before viewer_hook_ is
  // destroyed — the sink stores a raw pointer into the hook.
  spdlog::sink_ptr log_sink_;

  // Shared STA / Tcl_Eval serialization mutex (see tclMutex()).
  // TclEvaluator::eval() and the STA-touching request handlers in
  // request_handler.cpp lock this mutex.
  std::mutex tcl_mutex_;

  // Set by tclExitHandler / requestExit() when an exit is being
  // requested from a non-main thread (browser tcl widget).
  std::atomic<bool> exit_requested_{false};
  // Set by requestStop() / web_server_stop_cmd from a worker.
  std::atomic<bool> stop_requested_{false};

  // Tcl thread id of the thread that called serve().  Used as the
  // target for Tcl_ThreadQueueEvent / Tcl_ThreadAlert wakeups.
  Tcl_ThreadId main_thread_id_ = nullptr;

  // Level-1 command trace token; installed in serve(), removed in
  // teardown().
  Tcl_Trace trace_token_ = nullptr;

  // Tcl_StackChannel handles + per-channel state for the stdout / stderr
  // mirror.  The Tcl_Channel pointers are returned by Tcl_StackChannel
  // and unstacked in teardown(); the void* fields point at heap-
  // allocated WebChannelMirror structs that the channel type's
  // outputProc reads as instance data.
  Tcl_Channel mirror_stdout_chan_ = nullptr;
  Tcl_Channel mirror_stderr_chan_ = nullptr;
  void* mirror_stdout_data_ = nullptr;
  void* mirror_stderr_data_ = nullptr;

  // True once initLogger() registered the WebLogSink.  Lets serve() and
  // initLogger() be idempotent and lets stop() know the sink needs removing.
  // Reset in stop() so a subsequent serve() re-registers the sink.
  bool logger_initialized_ = false;

  // Tcl command override: replaces `exit` while the server is running
  // so a worker-thread `exit` doesn't run Tcl_Exit (which would self-join
  // the worker thread inside ~WebServer).
  static int tclExitHandler(ClientData clientData,
                            Tcl_Interp* interp,
                            int argc,
                            const char* argv[]);
};

}  // namespace web
