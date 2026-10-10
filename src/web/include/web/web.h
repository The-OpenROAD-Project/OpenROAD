// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "boost/asio/ip/address.hpp"
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

// How a `web_server -bind` / `-web_bind` address must be treated.  The viewer
// runs Tcl commands, so binding anywhere but loopback hands a shell to whoever
// can reach the port (issue #11167).
enum class BindAddressKind
{
  kInvalid,
  kLoopback,
  kExposed
};

// Classify a bind address.  IP literals only: there is no name resolution, so
// "localhost" is kInvalid — the contract the commands document.  Callers that
// reach serve() from C++ (Main.cc) must check this first: serve() reports a
// bad address with utl::error, which throws.
BindAddressKind classifyBindAddress(std::string_view address);

// Host to put in the URL the browser is pointed at, for a server listening on
// `address`.  "localhost" only where it actually resolves to the listener —
// naming it for the whole 127.0.0.0/8 range sends the browser to a port
// nobody is bound to.  IPv6 literals come back bracketed, ready for a URL.
std::string browserHostForBind(const boost::asio::ip::address& address);

// Whether serve() launches a browser on the machine it runs on.  kAuto skips it
// where that browser would not be the user's (issue #11389).
enum class BrowserLaunch
{
  kAuto,
  kAlways,
  kNever
};

// What the launch policy looks at, read by serve() and passed in so the policy
// stays testable.  Owned strings: a later getenv() may invalidate earlier ones.
struct BrowserEnv
{
  std::string ssh_connection;   // $SSH_CONNECTION
  std::string ssh_client;       // $SSH_CLIENT
  std::string display;          // $DISPLAY, X11 platforms only
  std::string host_name;        // this machine, to read $DISPLAY against
  std::string batch_job;        // first of $LSB_JOBID, $SLURM_JOB_ID, ...
  std::string vscode_ipc_hook;  // $VSCODE_IPC_HOOK_CLI
  std::string browser;          // $BROWSER
  bool no_browser = false;      // $OPENROAD_NO_BROWSER
  // $DISPLAY is a bare ":N" whose X socket this user owns (VNC, x2go).
  bool own_local_display = false;
  // $DISPLAY or $WAYLAND_DISPLAY set; always true off X11 platforms.
  bool has_display = true;
  // This machine's interface addresses, to read an IP in $DISPLAY against.
  std::vector<boost::asio::ip::address> local_addresses;
  bool wsl = false;  // $WSL_DISTRO_NAME: the X server is the Windows desktop
};

// True when $DISPLAY names an X server on another machine.  Socket paths,
// loopback, this machine's name or addresses, and a container's host are local.
bool displayIsRemote(
    std::string_view display,
    std::string_view host_name,
    const std::vector<boost::asio::ip::address>& local_addresses = {});

// The X socket of a bare ":N" or "unix:N" display, or "" for any other form.
std::string localDisplaySocket(std::string_view display);

// True when $BROWSER is VS Code's helpers/browser.sh, which opens a URL where
// the editor runs.
bool launchesThroughBrowserEnv(const BrowserEnv& env);

// Why serve() is not launching a browser, so the message can say it without
// restating the policy.
enum class LaunchSkip
{
  kNone,
  kFlag,           // -no_browser / -web_no_browser
  kOptOut,         // OPENROAD_NO_BROWSER
  kSshSession,     // driven from another machine over ssh
  kBatchJob,       // a scheduler put this process on a farm host
  kRemoteDisplay,  // $DISPLAY belongs to another machine (issue #11389)
  kNoDisplay       // nowhere to draw at all
};

// Apply the kAuto policy, and say which clause decided.  kNone means launch.
LaunchSkip browserLaunchSkipReason(BrowserLaunch mode, const BrowserEnv& env);

// The reason, as a clause that reads inside "Not launching a browser here
// (...)".  Empty for kNone.
std::string_view browserSkipReasonText(LaunchSkip reason);

// Parse the mode name web_server_cmd passes.  Anything unrecognized is kAuto:
// the mode never keeps the server from starting.
BrowserLaunch browserLaunchFromString(std::string_view mode);

// How to reach the viewer from another machine, printed with WEB-0001: a tunnel
// for a loopback bind, this host's url (`target` appended) for a wildcard one.
std::string reachabilityHint(BindAddressKind kind,
                             const boost::asio::ip::address& address,
                             std::string_view host,
                             uint16_t port,
                             std::string_view target);

// A fresh 128-bit hex token, gating every request that can reach the Tcl
// interpreter (issue #11389).  Empty when the entropy source cannot be read.
std::string generateAuthToken();

// True if the `token` of a parsed query matches `expected`, compared in
// constant time.  An empty `expected` matches nothing.
bool tokenAllowed(const std::map<std::string, std::string>& params,
                  std::string_view expected);

// The same check straight off a request target, for the callers that have no
// parsed query of their own (the WebSocket upgrade).
bool requestTokenAllowed(std::string_view target, std::string_view expected);

// The redirect for a redeemed ticket: the request's raw query with `ticket`
// swapped for `token`, so viewer options like ?mergetiles=0 survive.
std::string authRedirectTarget(std::string_view target, std::string_view token);

// One server session's credentials: the token every gated request carries, and
// a one-shot ticket that keeps the token off the browser's command line.
class SessionAuth
{
 public:
  SessionAuth(std::string token,
              std::string ticket,
              std::chrono::seconds ticket_ttl);

  const std::string& token() const { return token_; }
  const std::string& ticket() const { return ticket_; }

  // True for the first caller that presents the live ticket, and only then:
  // redeeming spends it, and an expired ticket is already spent.
  bool redeemTicket(std::string_view candidate);

  // Spend the ticket unredeemed: once its launch failed or was skipped, no
  // browser of ours will ever present it.
  void revokeTicket();

  // True for the first refused WebSocket upgrade of this session only.
  bool firstRejection() { return !rejected_.exchange(true); }

 private:
  const std::string token_;
  const std::string ticket_;
  std::atomic<bool> spent_{false};
  std::atomic<bool> rejected_{false};
  const std::chrono::steady_clock::time_point expiry_;
};

// How long a launch ticket stays redeemable.  Long enough for a cold browser
// start, short enough that what `ps` saw stops working soon after.
inline constexpr std::chrono::seconds kTicketTtl{300};

// What serve() binds to when the caller passes no address.  Owned here so the
// Tcl and command-line front ends cannot drift apart on the security default.
inline constexpr const char* kDefaultBindAddress = "127.0.0.1";

// Shared by every "that address is not usable" message, so they cannot drift.
inline constexpr const char* kBindAddressHint
    = "expected an IP literal such as 127.0.0.1 or ::1";

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
    int max_in_flight,
    std::shared_ptr<SessionAuth> auth);

// A layout web server.  serve() starts the server in background I/O
// threads; waitForStop() blocks the calling thread until requestStop()
// is called, mirroring web::show / web::hide.

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

  // Start the web server on the given port, listening on `bind_address` — an
  // IP literal, or empty for kDefaultBindAddress; see BindAddressKind.
  // Launches background I/O threads and returns immediately.  A second call is
  // a no-op if the server is already running.  Logs the reason and throws if
  // the server cannot start.
  void serve(int port,
             const std::string& bind_address,
             BrowserLaunch launch = BrowserLaunch::kAuto);

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

  // Load a heat map from a CSV file and attach it to one chiplet of the
  // design; returns the short name clients use to reference it.
  // CSV row 0 = (chiplet_name, heatmap_name); rows 1+ = x0,y0,x1,y1,value
  // with the coordinates in the chiplet's local frame, in microns.  The
  // chiplet is resolved against TileGenerator::chiplets(), so its world
  // transform is applied and the data lands in the right place in a
  // multi-die view.
  std::string loadChipletHeatMap(const std::string& file_path);

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
  // broadcasts to clients), mirroring how web::Gui delegates to MainWindow.
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

  // Tears down the I/O threads and cleans up hooks.  Safe to call multiple
  // times and from any thread; after it returns, isRunning() is false and
  // serve() may be called again to restart the server.
  void stop();

 private:
  // Stops ioc_, joins every worker thread except the current one, and
  // clears threads_. Detaches the current thread if it happens to be a
  // worker (would otherwise raise EDEADLK on self-join).
  void stopAndJoinIoThreads();

  // Push the current label set to every connected client.  Labels are global
  // and live outside ODB, so nothing else notifies the other sessions that a
  // Tcl-driven add/delete/clear changed what they should draw.
  void broadcastLabels();

  // Tell every connected client the registered heat-map set changed, so it
  // re-requests "heatmaps" and redraws its control panel.  Sessions build
  // their heat-map instances from the registry, so a source registered after
  // a client connected is invisible to it until this push arrives.
  void broadcastHeatMapsChanged();

  // Serial for the short names handed out by loadChipletHeatMap.  The
  // registry keys on short name and keeps the first registration, so these
  // must not collide across calls.
  int chiplet_heat_map_count_ = 0;

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
  // Blocking support: waitForStop() sleeps on stop_cv_ until
  // requestStop() sets stop_requested_.
  std::mutex stop_mutex_;
  std::condition_variable stop_cv_;
  bool stop_requested_ = false;

  // Set by tclExitHandler when `exit` is run on a worker thread.
  bool exit_requested_ = false;

  // Minted per serve() and cleared in stop(), so a restarted server does not
  // honour the previous session's links.
  std::shared_ptr<SessionAuth> auth_;

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
