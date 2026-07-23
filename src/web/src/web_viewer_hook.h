// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "gui/gui.h"

namespace utl {
class Logger;
}

namespace web {

class WebChart;

// Tracks connected WebSocket sessions so code outside of WebSocketSession
// (like WebViewerHook) can broadcast server-push messages.  Each session
// registers a send callback on construction and removes it on destruction.
class SessionRegistry
{
 public:
  using SendFn = std::function<void(const std::string& json)>;
  // Sends JSON and invokes the callback after the session write completes.
  using SendAndWaitFn
      = std::function<void(const std::string& json, std::function<void()>)>;
  using WaitInterruptFn = std::function<bool()>;

  // Register a send callback.  Returns a token the caller must pass to
  // remove() during teardown.  The optional SendAndWaitFn lets
  // broadcastAndWait() wait until a write has completed.
  std::size_t add(SendFn send, SendAndWaitFn send_and_wait = {});
  void remove(std::size_t token);

  // True if at least one session is registered.
  bool hasClients() const;

  // Block until at least one client connects (or timeout expires).
  // Returns true if a client is connected, false on timeout.
  bool waitForClient(int timeout_seconds, const WaitInterruptFn& interrupted);

  // Wake threads blocked in waitForClient() so they can observe an
  // interrupt predicate change.
  void notifyClientWaiters();

  // Deliver the JSON string to every currently-registered session.
  void broadcast(const std::string& json);

  // Like broadcast(), but waits until every fenceable session has completed
  // the queued write (or timeout expires).
  bool broadcastAndWait(const std::string& json,
                        std::chrono::milliseconds timeout);

 private:
  struct SessionCallbacks
  {
    SendFn send;
    SendAndWaitFn send_and_wait;  // may be empty for legacy callers
  };

  mutable std::mutex mutex_;
  mutable std::condition_variable client_cv_;
  std::size_t next_token_ = 1;
  std::unordered_map<std::size_t, SessionCallbacks> senders_;
};

// The web viewer's bridge to gui::Gui.  Installed as the Gui's
// HeadlessViewer so renderers (gpl::GraphicsImpl, etc.) can drive pause
// and redraw even when the Qt GUI is not present.  Also owns the
// gui::Chart factory so addChart() returns WebChart instances.
//
// Lifetime: constructed by WebServer before sessions start, destroyed
// after all sessions are torn down.  The destructor signals any thread
// blocked in pause() so the placer doesn't hang on shutdown.
class WebViewerHook : public gui::HeadlessViewer
{
 public:
  WebViewerHook();
  ~WebViewerHook() override;

  SessionRegistry& sessions() { return sessions_; }

  // Flush accumulated log output to all connected clients.
  void drainLogs();

  // --- gui::HeadlessViewer ---
  void redraw() override;
  void pause(int timeout_ms) override;
  bool isPaused() const override;

  // Release any blocked placer thread.  Called by the DEBUG_CONTINUE
  // request handler.
  void continueExecution();

  // Optional callback invoked at the start of pause() to flush
  // accumulated log output to clients before blocking.
  using DrainLogsFn = std::function<void()>;
  void setDrainLogsFn(DrainLogsFn fn);

  // gui::Chart factory.  The WebServer installs this on gui::Gui via
  // setChartFactory.  The hook retains ownership of every chart it
  // creates so clients can query them later.
  gui::Chart* createChart(const std::string& name,
                          const std::string& x_label,
                          const std::vector<std::string>& y_labels);

  // Snapshot of all charts (pointers are owned by this hook — do not
  // delete).  Holds the chart-list mutex while copying.
  std::vector<WebChart*> charts() const;

  // --- Custom UI registered from Tcl (create_menu_item /
  // create_toolbar_button) --------------------------------------------------
  //
  // Mirrors gui::MainWindow's buttons_/menu_actions_ maps: the hook is the
  // server-side source of truth so the definitions survive page reloads and
  // are served to clients that connect after the commands ran.  Each add/
  // remove broadcasts the full registry to connected clients so live edits
  // (e.g. typed in the browser Tcl console) update every open browser at once
  // — something the single-window Qt GUI cannot do.

  // A toolbar button.  `icon`/`tooltip` are web-only niceties (Qt buttons are
  // text-only).  When `toggle` is true the button is checkable: clicking runs
  // `script` when turning on and `script_off` when turning off.
  struct CustomButton
  {
    std::string key;
    std::string text;
    std::string script;
    std::string icon;
    std::string tooltip;
    std::string script_off;
    bool toggle = false;
    bool echo = false;
  };

  // A menu item.  `path` is a '/'-separated menu hierarchy (default
  // "Custom Scripts"), matching gui::MainWindow::findMenu.
  struct CustomMenuItem
  {
    std::string key;
    std::string path;
    std::string text;
    std::string script;
    std::string shortcut;
    bool echo = false;
  };

  // Register a toolbar button.  Returns the (possibly auto-generated) key.
  // Auto-generates "button{N}" when `name` is empty; errors (via `logger`)
  // when a supplied `name` is already registered.  Broadcasts on success.
  std::string addToolbarButton(utl::Logger* logger,
                               const std::string& name,
                               const std::string& text,
                               const std::string& script,
                               const std::string& icon,
                               const std::string& tooltip,
                               bool toggle,
                               const std::string& script_off,
                               bool echo);
  // Idempotent: no-op when `name` is not registered.  Broadcasts on removal.
  void removeToolbarButton(const std::string& name);

  // Register a menu item.  Auto-generates "action{N}" when `name` is empty;
  // errors (via `logger`) on duplicate `name`.  Broadcasts on success.
  std::string addMenuItem(utl::Logger* logger,
                          const std::string& name,
                          const std::string& path,
                          const std::string& text,
                          const std::string& script,
                          const std::string& shortcut,
                          bool echo);
  void removeMenuItem(const std::string& name);

  // Serialized registry: {"type":"custom_ui","menu":[...],"toolbar":[...]}.
  // Used both by broadcast and by the custom_ui request handler.
  std::string customUiJson() const;

 private:
  // Builds the serialized registry assuming custom_ui_mutex_ is already held.
  // The single source of truth for both customUiJson() and the add/remove
  // broadcast path.
  std::string customUiJsonLocked() const;

  // Shared add/remove for the two custom-UI vectors (toolbar buttons and menu
  // items), which differ only in element type, id counter and key prefix.
  // registerCustom appends `item` under the lock (auto-generating "prefix{N}"
  // when `name` is empty), then broadcasts the new registry outside the lock;
  // it sets *is_duplicate (and does NOT add) when a non-empty `name` already
  // exists, so the caller can log the tool-specific error with a literal id.
  template <class T>
  std::string registerCustom(std::vector<T>& vec,
                             int& next_id,
                             const char* prefix,
                             const std::string& name,
                             T item,
                             bool* is_duplicate);
  template <class T>
  void removeCustom(std::vector<T>& vec, const std::string& name);

  SessionRegistry sessions_;

  DrainLogsFn drain_logs_;

  mutable std::mutex pause_mutex_;
  std::condition_variable pause_cv_;
  // Atomic so isPaused() (called per-tile on 32 I/O threads) avoids
  // acquiring pause_mutex_ on the hot path.
  std::atomic<bool> paused_{false};
  std::atomic<bool> client_wait_interrupted_{false};
  bool released_ = false;  // true once continueExecution() or dtor fires
  // Single-caller invariant: Gui::pause() drives one debug thread at a time.
  bool in_pause_ = false;  // true while pause() is executing (any path)
  std::condition_variable done_cv_;  // signaled when in_pause_ goes false

  mutable std::mutex charts_mutex_;
  std::vector<std::unique_ptr<WebChart>> charts_;

  // Custom-UI registry.  Order is preserved (insertion order = display order,
  // like Qt's view_tool_bar_->actions()).
  mutable std::mutex custom_ui_mutex_;
  std::vector<CustomButton> custom_buttons_;
  std::vector<CustomMenuItem> custom_menu_items_;
  int next_button_id_ = 0;
  int next_menu_id_ = 0;
};

}  // namespace web
