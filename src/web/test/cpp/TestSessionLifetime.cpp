// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// The web server against a real listener; the other web tests drive the
// handlers directly.

#include <poll.h>

#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <thread>

#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/address.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/write.hpp"
#include "clock_tree_report.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "request_handler.h"
#include "tile_generator.h"
#include "timing_report.h"
#include "utl/Logger.h"
#include "web/web.h"
#include "web_viewer_hook.h"

namespace web {
namespace {

namespace net = boost::asio;
using Tcp = net::ip::tcp;

// The status code of a response head; 0 when there is no status line.
int statusOf(const std::string& head)
{
  int code = 0;
  if (!head.starts_with("HTTP/1.1 ") || head.size() < 12) {
    return 0;
  }
  const char* const first = head.data() + 9;
  const auto res = std::from_chars(first, first + 3, code);
  return res.ec == std::errc{} && res.ptr == first + 3 ? code : 0;
}

// A loopback listener with no design, so eagerInit() returns at once and the
// init thread is often the one to drop a session's last reference.
class ListenerFixture : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    logger_ = std::make_unique<utl::Logger>();
    hook_ = std::make_unique<WebViewerHook>();
    ioc_ = std::make_unique<net::io_context>(1);
    db_.reset(odb::dbDatabase::create());
    buildDesign(db_.get());
    generator_ = std::make_shared<TileGenerator>(
        db_.get(), /*sta=*/nullptr, logger_.get());
    tcl_eval_
        = std::make_shared<TclEvaluator>(/*interp=*/nullptr, logger_.get());

    handle_ = startListener();
    generator_baseline_ = generator_.use_count();
    io_thread_ = std::thread([this] { ioc_->run(); });
  }

  void TearDown() override
  {
    stopServing();
    generator_.reset();
    hook_.reset();
    // A detached init thread can outlive the test, as it outlives stop() in
    // the server; leave it what it may still touch.
    (void) ioc_.release();     // NOLINT(bugprone-unused-return-value)
    (void) db_.release();      // NOLINT(bugprone-unused-return-value)
    (void) logger_.release();  // NOLINT(bugprone-unused-return-value)
  }

  ListenerHandle startListener()
  {
    return createAndRunListener(
        *ioc_,
        Tcp::endpoint{net::ip::address_v4::loopback(), 0},
        generator_,
        tcl_eval_,
        std::make_shared<TimingReport>(/*sta=*/nullptr),
        std::make_shared<ClockTreeReport>(/*sta=*/nullptr),
        logger_.get(),
        hook_.get(),
        /*max_in_flight=*/16);
  }

  // Runs before the generator exists; the default leaves the database empty.
  virtual void buildDesign(odb::dbDatabase* /*db*/) {}

  // What WebServer::stop() does to the io side, dropping the listener with its
  // handle as stop() does.  Safe to call twice.
  void stopServing()
  {
    if (!io_thread_.joinable()) {
      return;
    }
    if (handle_.shutdown) {
      handle_.shutdown();
      handle_.shutdown = nullptr;
    }
    ioc_->stop();
    io_thread_.join();
  }

  // False when nothing arrives in five seconds, so a server that stops
  // answering fails the test instead of hanging it.
  static bool waitReadable(Tcp::socket& socket)
  {
    pollfd pfd{};
    pfd.fd = socket.native_handle();
    pfd.events = POLLIN;
    return ::poll(&pfd, 1, /*timeout=*/5000) > 0;
  }

  Tcp::socket connectAndSend(const std::string& request)
  {
    Tcp::socket socket(*ioc_);
    socket.connect(Tcp::endpoint{net::ip::address_v4::loopback(), port()});
    net::write(socket, net::buffer(request));
    return socket;
  }

  // The status line and headers.  Read in chunks: a slow reader lets the init
  // thread finish first and hides the race ClientThatDropsTheSocket catches.
  static std::string readHead(Tcp::socket& socket)
  {
    std::string received;
    std::size_t end = std::string::npos;
    while ((end = received.find("\r\n\r\n")) == std::string::npos
           && received.size() < 4096) {
      if (!waitReadable(socket)) {
        ADD_FAILURE() << "no response within 5 s; got: " << received;
        return received;
      }
      char chunk[1024];
      boost::system::error_code ec;
      const std::size_t n = socket.read_some(net::buffer(chunk), ec);
      if (ec) {
        return received;
      }
      received.append(chunk, n);
    }
    return end == std::string::npos ? received : received.substr(0, end + 4);
  }

  // Send a raw request, read the response head, and close at once.
  std::string requestHead(const std::string& request)
  {
    Tcp::socket socket = connectAndSend(request);
    return readHead(socket);
  }

  std::string upgradeRequest(const std::string& origin = "") const
  {
    std::string request = "GET /ws HTTP/1.1\r\n"
                          "Host: localhost:"
                          + std::to_string(port())
                          + "\r\n"
                            "Upgrade: websocket\r\n"
                            "Connection: Upgrade\r\n"
                            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                            "Sec-WebSocket-Version: 13\r\n";
    if (!origin.empty()) {
      request += "Origin: " + origin + "\r\n";
    }
    return request + "\r\n";
  }

  std::string upgradeAndDrop(const std::string& origin = "")
  {
    return requestHead(upgradeRequest(origin));
  }

  std::string httpGet(const std::string& target)
  {
    return requestHead("GET " + target + " HTTP/1.1\r\nHost: localhost:"
                       + std::to_string(port())
                       + "\r\nConnection: close\r\n\r\n");
  }

  uint16_t port() const { return handle_.port; }

  // Every server-side object of a connection holds a copy of generator_ and
  // drops its last one in the destructor, after the init thread is handled.
  bool waitForSessionsGone(std::chrono::milliseconds timeout)
  {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (generator_.use_count() > generator_baseline_) {
      if (std::chrono::steady_clock::now() >= deadline) {
        return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return true;
  }

  std::unique_ptr<utl::Logger> logger_;
  std::unique_ptr<WebViewerHook> hook_;
  std::unique_ptr<net::io_context> ioc_;
  std::unique_ptr<odb::dbDatabase, void (*)(odb::dbDatabase*)> db_{
      nullptr,
      &odb::dbDatabase::destroy};
  std::shared_ptr<TileGenerator> generator_;
  std::int64_t generator_baseline_ = 0;
  std::shared_ptr<TclEvaluator> tcl_eval_;
  ListenerHandle handle_;
  std::thread io_thread_;
};

// A block with no die area and no shapes: nothing to frame, so the render
// raises.
class DegenerateDesignFixture : public ListenerFixture
{
 protected:
  void buildDesign(odb::dbDatabase* db) override
  {
    odb::dbTech* tech = odb::dbTech::create(db, "tech");
    odb::dbBlock::create(odb::dbChip::create(db, tech), "top");
  }
};

// The init thread can drop the last reference and so run ~WebSocketSession on
// itself.  Which thread does is a race; the loop makes it near certain.
TEST_F(ListenerFixture, ClientThatDropsTheSocketDoesNotAbortTheServer)
{
  constexpr int kConnections = 25;
  for (int i = 0; i < kConnections; ++i) {
    EXPECT_EQ(statusOf(upgradeAndDrop()), 101) << "connection " << i;
    ASSERT_TRUE(waitForSessionsGone(std::chrono::seconds(5)))
        << "connection " << i;
  }
}

// A refused handshake must not take the listener with it.
TEST_F(ListenerFixture, RefusedUpgradeLeavesTheListenerServing)
{
  EXPECT_EQ(statusOf(upgradeAndDrop("http://evil.example")), 403);
  EXPECT_EQ(statusOf(upgradeAndDrop()), 101);
  EXPECT_TRUE(waitForSessionsGone(std::chrono::seconds(5)));
}

// stop() deletes the hook while a session can still be alive, on its init
// thread or with a read pending; when it goes, it must not reach the hook.
TEST_F(ListenerFixture, SessionThatOutlivesItsHookLeavesItAlone)
{
  Tcp::socket client = connectAndSend(upgradeRequest());
  ASSERT_EQ(statusOf(readHead(client)), 101);
  ASSERT_TRUE(hook_->sessions().waitForClient(5, [] { return false; }));
  stopServing();
  hook_.reset();

  // Let the pending read fail, so the session goes now that its hook is gone.
  client.close();
  ioc_->restart();
  io_thread_ = std::thread([this] { ioc_->run(); });
  EXPECT_TRUE(waitForSessionsGone(std::chrono::seconds(5)));
}

TEST_F(ListenerFixture, ImageDownloadWithoutADesignAnswers404)
{
  EXPECT_EQ(statusOf(httpGet("/download/image?type=entire")), 404);
  EXPECT_EQ(statusOf(httpGet("/")), 200);
}

// An escaping utl::Logger::error would drop the connection unanswered.  The
// 500 keeps the CORS header, or a cross-origin client could not read it.
TEST_F(DegenerateDesignFixture, FailedRenderAnswers500AndKeepsServing)
{
  const std::string head = httpGet("/download/image?type=entire");
  EXPECT_EQ(statusOf(head), 500);
  EXPECT_NE(head.find("Access-Control-Allow-Origin: *"), std::string::npos)
      << head;
  EXPECT_EQ(statusOf(httpGet("/")), 200);
}

}  // namespace
}  // namespace web
