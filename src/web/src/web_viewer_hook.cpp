// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "web_viewer_hook.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "gui/gui.h"
#include "web_chart.h"

namespace web {

// How long pause() waits for a browser to connect when no clients are
// registered at the time of the first debug pause.
constexpr int kClientConnectTimeoutSeconds = 30;

// Safety cap on indefinite pauses (timeout_ms == 0) so a disconnected
// client can't hang the process forever.
constexpr auto kMaxPauseTimeout = std::chrono::minutes(10);

//------------------------------------------------------------------------------
// SessionRegistry
//------------------------------------------------------------------------------

std::size_t SessionRegistry::add(SendFn send)
{
  std::size_t token;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    token = next_token_++;
    senders_.emplace(token, std::move(send));
  }
  client_cv_.notify_all();
  return token;
}

void SessionRegistry::remove(std::size_t token)
{
  std::lock_guard<std::mutex> lock(mutex_);
  senders_.erase(token);
}

bool SessionRegistry::hasClients() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return !senders_.empty();
}

bool SessionRegistry::waitForClient(int timeout_seconds,
                                    const WaitInterruptFn& interrupted)
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (!senders_.empty()) {
    return true;
  }
  const auto ready = [this, &interrupted]() {
    return !senders_.empty() || (interrupted && interrupted());
  };
  client_cv_.wait_for(lock, std::chrono::seconds(timeout_seconds), ready);
  return !senders_.empty();
}

void SessionRegistry::notifyClientWaiters()
{
  client_cv_.notify_all();
}

void SessionRegistry::broadcast(const std::string& json)
{
  // Copy the senders out under the lock so we don't hold it while
  // invoking callbacks (which may take session-level locks of their own).
  std::vector<SendFn> to_send;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    to_send.reserve(senders_.size());
    for (const auto& [_, fn] : senders_) {
      to_send.push_back(fn);
    }
  }
  for (const auto& fn : to_send) {
    fn(json);
  }
}

//------------------------------------------------------------------------------
// WebViewerHook
//------------------------------------------------------------------------------

WebViewerHook::WebViewerHook() = default;

WebViewerHook::~WebViewerHook()
{
  // If the placer is currently paused in a call into this hook, make sure
  // it unblocks and finishes all member access before we're destroyed.
  std::unique_lock<std::mutex> lock(pause_mutex_);
  released_ = true;
  client_wait_interrupted_.store(true);
  paused_ = false;
  sessions_.notifyClientWaiters();
  pause_cv_.notify_all();
  // Wait for pause() to fully exit (including any unlocked broadcasts).
  // done_cv_.wait releases the lock, allowing the pause thread to proceed.
  done_cv_.wait(lock, [this]() { return !in_pause_; });
}

void WebViewerHook::redraw()
{
  // Flush accumulated log output so the browser console stays
  // up-to-date even on non-pause iterations.
  //
  // Lock ordering: drain_logs_ acquires the spdlog base_sink mutex,
  // then sessions_.broadcast() acquires SessionRegistry::mutex_.
  // No code path acquires these in the reverse order, so no deadlock.
  if (drain_logs_) {
    drain_logs_();
  }
  sessions_.broadcast(R"({"type":"debug_refresh"})");
}

void WebViewerHook::pause(int timeout_ms)
{
  std::unique_lock<std::mutex> lock(pause_mutex_);
  in_pause_ = true;

  if (released_) {
    released_ = false;  // reset for next call
    client_wait_interrupted_.store(false);
    in_pause_ = false;
    done_cv_.notify_all();
    return;
  }
  client_wait_interrupted_.store(false);

  // If no web clients are connected, wait briefly for one to appear.
  // This handles the common case where the placer starts before the
  // browser has finished connecting.  If nobody connects in time,
  // skip the pause — there's nobody to click Continue.
  if (!sessions_.hasClients()) {
    lock.unlock();
    if (!sessions_.waitForClient(kClientConnectTimeoutSeconds, [this]() {
          return client_wait_interrupted_.load();
        })) {
      lock.lock();
      released_ = false;
      client_wait_interrupted_.store(false);
      in_pause_ = false;
      done_cv_.notify_all();
      return;
    }
    lock.lock();
    // Re-check released_ in case continueExecution() was called
    // while we were waiting.
    if (released_) {
      released_ = false;
      client_wait_interrupted_.store(false);
      in_pause_ = false;
      done_cv_.notify_all();
      return;
    }
  }

  paused_ = true;
  released_ = false;

  // Flush accumulated log output to clients before blocking, so the
  // browser console shows the iteration output leading up to this pause.
  lock.unlock();
  if (drain_logs_) {
    drain_logs_();
  }
  sessions_.broadcast(R"({"type":"debug_paused"})");
  lock.lock();

  if (timeout_ms > 0) {
    pause_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() {
      return released_;
    });
  } else {
    // Cap indefinite pauses so a disconnected client can't hang the
    // process forever.
    pause_cv_.wait_for(lock, kMaxPauseTimeout, [this]() { return released_; });
  }

  paused_ = false;
  released_ = false;
  lock.unlock();

  sessions_.broadcast(R"({"type":"debug_resumed"})");

  lock.lock();
  in_pause_ = false;
  done_cv_.notify_all();
}

bool WebViewerHook::isPaused() const
{
  return paused_.load(std::memory_order_acquire);
}

void WebViewerHook::setDrainLogsFn(DrainLogsFn fn)
{
  drain_logs_ = std::move(fn);
}

void WebViewerHook::continueExecution()
{
  {
    std::lock_guard<std::mutex> lock(pause_mutex_);
    released_ = true;
    client_wait_interrupted_.store(true);
  }
  sessions_.notifyClientWaiters();
  pause_cv_.notify_all();
}

gui::Chart* WebViewerHook::createChart(const std::string& name,
                                       const std::string& x_label,
                                       const std::vector<std::string>& y_labels)
{
  auto chart = std::make_unique<WebChart>(name, x_label, y_labels);
  WebChart* ptr = chart.get();
  std::lock_guard<std::mutex> lock(charts_mutex_);
  charts_.push_back(std::move(chart));
  return ptr;
}

std::vector<WebChart*> WebViewerHook::charts() const
{
  std::lock_guard<std::mutex> lock(charts_mutex_);
  std::vector<WebChart*> out;
  out.reserve(charts_.size());
  for (const auto& c : charts_) {
    out.push_back(c.get());
  }
  return out;
}

}  // namespace web
