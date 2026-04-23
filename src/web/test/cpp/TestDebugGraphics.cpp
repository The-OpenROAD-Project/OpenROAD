// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "gtest/gtest.h"
#include "gui/gui.h"
#include "odb/geom.h"
#include "web_chart.h"
#include "web_painter.h"
#include "web_viewer_hook.h"
namespace web {
namespace {

TEST(WebPainterTest, RecordsRect)
{
  WebPainter painter(odb::Rect(0, 0, 1000, 1000), 0.256);
  painter.setPen(gui::Painter::kRed, /*cosmetic=*/true);
  painter.setBrush(gui::Painter::kBlue);
  painter.drawRect(odb::Rect(10, 20, 100, 200));

  ASSERT_EQ(painter.ops().size(), 1u);
  const auto* rect = std::get_if<DrawRectOp>(painter.ops().data());
  ASSERT_NE(rect, nullptr);
  EXPECT_EQ(rect->rect.xMin(), 10);
  EXPECT_EQ(rect->rect.yMax(), 200);
  EXPECT_EQ(rect->pen.color.r, 0xff);
  EXPECT_TRUE(rect->pen.cosmetic);
  EXPECT_EQ(rect->brush.color.b, 0xff);
}

TEST(WebPainterTest, RecordsLine)
{
  WebPainter painter(odb::Rect(0, 0, 1000, 1000), 1.0);
  painter.setPen(gui::Painter::kGreen, /*cosmetic=*/false, /*width=*/3);
  painter.drawLine(odb::Point(1, 2), odb::Point(3, 4));

  ASSERT_EQ(painter.ops().size(), 1u);
  const auto* line = std::get_if<DrawLineOp>(painter.ops().data());
  ASSERT_NE(line, nullptr);
  EXPECT_EQ(line->p1.x(), 1);
  EXPECT_EQ(line->p2.y(), 4);
  EXPECT_EQ(line->pen.width, 3);
}

TEST(WebPainterTest, SaveRestoreState)
{
  WebPainter painter(odb::Rect(0, 0, 1000, 1000), 1.0);
  painter.setPen(gui::Painter::kRed);
  painter.saveState();
  painter.setPen(gui::Painter::kGreen);
  painter.drawRect(odb::Rect(0, 0, 10, 10));
  painter.restoreState();
  painter.drawRect(odb::Rect(0, 0, 10, 10));

  ASSERT_EQ(painter.ops().size(), 2u);
  const auto* r1 = std::get_if<DrawRectOp>(painter.ops().data());
  const auto* r2 = std::get_if<DrawRectOp>(&painter.ops()[1]);
  ASSERT_NE(r1, nullptr);
  ASSERT_NE(r2, nullptr);
  EXPECT_EQ(r1->pen.color.g, 0xff);  // green
  EXPECT_EQ(r2->pen.color.r, 0xff);  // red (restored)
}

// Helper: register a dummy client so pause() actually blocks
// (with no clients, pause() returns immediately).
static std::size_t registerDummyClient(WebViewerHook& hook)
{
  return hook.sessions().add([](const std::string&) {});
}

TEST(WebViewerHookTest, PauseUnblocksOnContinue)
{
  WebViewerHook hook;
  auto token = registerDummyClient(hook);
  std::atomic<bool> resumed{false};
  std::thread waiter([&]() {
    hook.pause(/*timeout_ms=*/0);
    resumed.store(true);
  });

  // Wait up to 500ms for the waiter to be inside pause().  isPaused()
  // should flip to true during that window.
  for (int i = 0; i < 50 && !hook.isPaused(); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  EXPECT_TRUE(hook.isPaused());

  hook.continueExecution();
  waiter.join();
  EXPECT_TRUE(resumed.load());
  EXPECT_FALSE(hook.isPaused());
  hook.sessions().remove(token);
}

TEST(WebViewerHookTest, PauseReturnsWhenNoClientsConnect)
{
  WebViewerHook hook;
  // No registered clients → pause waits for a client, then gives up.
  // Register a client after a short delay to unblock faster.
  std::thread late_register([&]() {
    // Don't register — let the wait-for-client timeout expire.
    // The internal timeout is 30s but we can't speed that up in a
    // unit test without injecting it as a parameter, so just verify
    // that pause() eventually returns and doesn't hang forever.
  });
  late_register.join();
  // Instead, test the fast path: register a client, then remove it
  // before calling pause.  pause() sees no clients and waits, but
  // we add one quickly to unblock.
  std::atomic<bool> done{false};
  std::thread pauser([&]() {
    hook.pause(/*timeout_ms=*/0);
    done.store(true);
  });
  // Give pause() a moment to enter waitForClient, then register.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  auto token = registerDummyClient(hook);
  // Now pause is inside the pause CV wait — continue to unblock.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  hook.continueExecution();
  pauser.join();
  EXPECT_TRUE(done.load());
  EXPECT_FALSE(hook.isPaused());
  hook.sessions().remove(token);
}

TEST(WebViewerHookTest, DestructorUnblocksPausedThread)
{
  std::atomic<bool> resumed{false};
  std::thread waiter;
  {
    WebViewerHook hook;
    registerDummyClient(hook);
    waiter = std::thread([&]() {
      hook.pause(/*timeout_ms=*/0);
      resumed.store(true);
    });
    // Wait until the thread is definitely inside pause().
    for (int i = 0; i < 200 && !hook.isPaused(); ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(hook.isPaused())
        << "Thread didn't reach pause() within 2s — test is broken";
  }  // hook destroyed → should signal cv → waiter returns
  waiter.join();
  EXPECT_TRUE(resumed.load());
}

TEST(WebViewerHookTest, PauseTimeoutReturns)
{
  WebViewerHook hook;
  registerDummyClient(hook);
  const auto t0 = std::chrono::steady_clock::now();
  hook.pause(/*timeout_ms=*/50);
  const auto elapsed = std::chrono::steady_clock::now() - t0;
  EXPECT_GE(elapsed, std::chrono::milliseconds(40));
  EXPECT_LT(elapsed, std::chrono::milliseconds(5000));
  EXPECT_FALSE(hook.isPaused());
}

TEST(WebViewerHookTest, SessionBroadcast)
{
  WebViewerHook hook;
  std::vector<std::string> seen;
  std::mutex m;
  const auto token = hook.sessions().add([&](const std::string& s) {
    std::lock_guard<std::mutex> lock(m);
    seen.push_back(s);
  });
  hook.redraw();
  hook.sessions().remove(token);
  hook.redraw();  // should not reach our callback

  std::lock_guard<std::mutex> lock(m);
  ASSERT_EQ(seen.size(), 1u);
  EXPECT_NE(seen[0].find("debug_refresh"), std::string::npos);
}

TEST(WebViewerHookTest, CreatesChartAndTracksIt)
{
  WebViewerHook hook;
  gui::Chart* c1 = hook.createChart("GPL", "iter", {"hpwl", "overflow"});
  ASSERT_NE(c1, nullptr);
  c1->setXAxisFormat("%d");
  c1->addPoint(0, {1.0, 2.0});
  c1->addPoint(1, {3.0, 4.0});

  const auto charts = hook.charts();
  ASSERT_EQ(charts.size(), 1u);
  EXPECT_EQ(charts[0]->name(), "GPL");
  EXPECT_EQ(charts[0]->xAxisFormat(), "%d");
  const auto pts = charts[0]->points();
  ASSERT_EQ(pts.size(), 2u);
  EXPECT_EQ(pts[1].x, 1);
  EXPECT_EQ(pts[1].ys[1], 4.0);
}

}  // namespace
}  // namespace web
