// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "gui/gui.h"

namespace web {

class WebChart;

// Tracks connected WebSocket sessions so code outside of WebSocketSession
// (like WebViewerHook) can broadcast server-push messages.  Each session
// registers a send callback on construction and removes it on destruction.
class SessionRegistry
{
 public:
  using SendFn = std::function<void(const std::string& json)>;
  using WaitInterruptFn = std::function<bool()>;

  // Register a send callback.  Returns a token the caller must pass to
  // remove() during teardown.
  std::size_t add(SendFn send);
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

 private:
  mutable std::mutex mutex_;
  mutable std::condition_variable client_cv_;
  std::size_t next_token_ = 1;
  std::unordered_map<std::size_t, SendFn> senders_;
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

 private:
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
};

}  // namespace web
