// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// WebServer::serve() and stop() in their own translation unit so that
// test executables linking libweb.a don't pull in gui::Gui::get()
// references (which would require the full gui library including Qt
// SWIG wrappers and ord::OpenRoad symbols).

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
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

void WebServer::serve(int port, const std::string& doc_root)
{
  if (ioc_) {
    logger_->warn(utl::WEB, 6, "Web server is already running.");
    return;
  }

  try {
    generator_ = std::make_shared<TileGenerator>(db_, sta_, logger_);
    auto timing_report = std::make_shared<TimingReport>(sta_);
    auto clock_report = std::make_shared<ClockTreeReport>(sta_);

    auto tcl_eval = std::make_shared<TclEvaluator>(interp_, logger_);

    viewer_hook_ = std::make_unique<WebViewerHook>();
    gui::Gui::get()->setHeadlessViewer(viewer_hook_.get());
    gui::Gui::get()->setChartFactory(
        [hook = viewer_hook_.get()](const std::string& name,
                                    const std::string& x_label,
                                    const std::vector<std::string>& y_labels) {
          return hook->createChart(name, x_label, y_labels);
        });

    auto log_sink = std::make_shared<WebLogSink>(viewer_hook_.get());
    logger_->addSink(log_sink);
    viewer_hook_->setDrainLogsFn([log_sink]() { log_sink->drainToClients(); });

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

    if (!doc_root.empty()) {
      logger_->info(utl::WEB, 4, "Serving static files from {}", doc_root);
    }

    const std::string url = "http://localhost:" + std::to_string(port);

    ioc_ = std::make_unique<net::io_context>(num_threads);

    shutdown_listener_ = createAndRunListener(*ioc_,
                                              Tcp::endpoint{address, u_port},
                                              generator_,
                                              tcl_eval,
                                              timing_report,
                                              clock_report,
                                              doc_root,
                                              logger_,
                                              viewer_hook_.get());

    threads_.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
      threads_.emplace_back([this] { ioc_->run(); });
    }

#if defined(__APPLE__)
    std::string open_cmd = "open " + url + " > /dev/null 2>&1";
#elif defined(_WIN32)
    std::string open_cmd = "start " + url + " > nul 2>&1";
#else
    std::string open_cmd = "xdg-open " + url + " > /dev/null 2>&1 &";
#endif
    int ret = std::system(open_cmd.c_str());
    (void) ret;

    logger_->info(utl::WEB, 1, "Server started on {}.", url);

  } catch (std::exception const& e) {
    stop();
    logger_->error(utl::WEB, 2, "Server error : {}", e.what());
  }
}

void WebServer::stop()
{
  if (viewer_hook_) {
    TileGenerator::setDebugOverlayCallback({});
    if (gui::Gui::get()->getHeadlessViewer() == viewer_hook_.get()) {
      gui::Gui::get()->setHeadlessViewer(nullptr);
    }
    gui::Gui::get()->setChartFactory({});
  }

  if (shutdown_listener_) {
    shutdown_listener_();
    shutdown_listener_ = {};
  }
  if (ioc_) {
    ioc_->stop();
  }
  for (auto& t : threads_) {
    if (t.joinable()) {
      t.join();
    }
  }
  threads_.clear();
  (void) ioc_.release();  // NOLINT(bugprone-unused-return-value)
  generator_.reset();
  viewer_hook_.reset();
}

}  // namespace web
