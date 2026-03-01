// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "web/web.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstring>
#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "color.h"
#include "lodepng.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "search.h"
#include "utl/Logger.h"
#include "utl/timer.h"

namespace web {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class TileGenerator
{
 public:
  TileGenerator(odb::dbDatabase* db, utl::Logger* logger)
      : db_(db), logger_(logger), search_(std::make_unique<Search>())

  {
    odb::dbChip* chip = db_->getChip();
    if (!chip) {
      logger_->error(utl::WEB, 99, "No chip to serve");
    }
    search_->setTopChip(chip);
  }

  odb::Rect getBounds();

  std::vector<unsigned char> generateTile(const std::string& layer,
                                          int z,
                                          int x,
                                          int y);

 private:
  void setPixel(std::vector<unsigned char>& image,
                int x,
                int y,
                const Color& c);

  static odb::Rect toPixels(double scale,
                            const odb::Rect& rect,
                            const odb::Rect& dbu_tile);

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  std::unique_ptr<Search> search_;
  static constexpr int kTileSizeInPixel = 256;
};

/* static */
odb::Rect TileGenerator::toPixels(const double scale,
                                  const odb::Rect& rect,
                                  const odb::Rect& dbu_tile)
{
  return odb::Rect((rect.xMin() - dbu_tile.xMin()) * scale,
                   (rect.yMin() - dbu_tile.yMin()) * scale,
                   std::ceil((rect.xMax() - dbu_tile.xMin()) * scale),
                   std::ceil((rect.yMax() - dbu_tile.yMin()) * scale));
}

void TileGenerator::setPixel(std::vector<unsigned char>& image,
                             const int x,
                             const int y,
                             const Color& c)
{
  if (x < 0 || x >= kTileSizeInPixel || y < 0 || y >= kTileSizeInPixel) {
    return;
  }
  const int index = (y * kTileSizeInPixel + x) * 4;
  image[index + 0] = c.r;
  image[index + 1] = c.g;
  image[index + 2] = c.b;
  image[index + 3] = c.a;
}

odb::Rect TileGenerator::getBounds()
{
  odb::Rect bounds;
  if (odb::dbBlock* block = db_->getChip()->getBlock()) {
    bounds = block->getBBox()->getBox();
  }
  return bounds;
}

std::vector<unsigned char> TileGenerator::generateTile(const std::string& layer,
                                                       const int z,
                                                       const int x,
                                                       int y)
{
  static_assert(sizeof(Color) == 4);
  std::vector<unsigned char> image_buffer(
      kTileSizeInPixel * kTileSizeInPixel * 4, 0);

  Color color{0, 0, 255, 180};
  Color obs_color = color.lighter();

  // Determine our tile's bounding box in dbu coordinates.
  const double num_tiles_at_zoom = pow(2, z);
  if (x >= 0 && y >= 0 && x < num_tiles_at_zoom && y < num_tiles_at_zoom) {
    y = num_tiles_at_zoom - 1 - y;  // flip
    const double tile_dbu_size = getBounds().maxDXDY() / num_tiles_at_zoom;
    const int dbu_x_min = x * tile_dbu_size;
    const int dbu_y_min = y * tile_dbu_size;
    const int dbu_x_max = std::ceil((x + 1) * tile_dbu_size);
    const int dbu_y_max = std::ceil((y + 1) * tile_dbu_size);
    const odb::Rect dbu_tile(dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max);
    const double scale = kTileSizeInPixel / tile_dbu_size;

    odb::dbBlock* block = db_->getChip()->getBlock();
    for (odb::dbInst* inst : search_->searchInsts(
             block, dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max)) {
      odb::Rect inst_bbox = inst->getBBox()->getBox();
      if (!dbu_tile.overlaps(inst_bbox)) {
        continue;
      }
      odb::dbMaster* master = inst->getMaster();
      if (master->getType() == odb::dbMasterType::CORE_SPACER) {
        continue;
      }
      const int xl = inst_bbox.xMin();
      const int yl = inst_bbox.yMin();
      const int xh = inst_bbox.xMax();
      const int yh = inst_bbox.yMax();

      const int pixel_xl = (int) ((xl - dbu_x_min) * scale);
      const int pixel_yl = (int) ((yl - dbu_y_min) * scale);
      const int pixel_xh = (int) std::ceil((xh - dbu_x_min) * scale);
      const int pixel_yh = (int) std::ceil((yh - dbu_y_min) * scale);

      // Draw the rectangle border
      Color gray{128, 128, 128, 255};
      if (dbu_x_min <= xl && xl <= dbu_x_max) {
        for (int iy = pixel_yl; iy < pixel_yh; ++iy) {
          const int draw_y = (255 - iy);
          setPixel(image_buffer, pixel_xl, draw_y, gray);
        }
      }
      if (dbu_x_min <= xh && xh <= dbu_x_max) {
        for (int iy = pixel_yl; iy < pixel_yh; ++iy) {
          const int draw_y = (255 - iy);
          setPixel(image_buffer, pixel_xh, draw_y, gray);
        }
      }
      if (dbu_y_min <= yl && yl <= dbu_y_max) {
        for (int ix = pixel_xl; ix < pixel_xh; ++ix) {
          const int draw_y = (255 - pixel_yl);
          setPixel(image_buffer, ix, draw_y, gray);
        }
      }
      if (dbu_y_min <= yh && yh <= dbu_y_max) {
        for (int ix = pixel_xl; ix < pixel_xh; ++ix) {
          const int draw_y = (255 - pixel_yh);
          setPixel(image_buffer, ix, draw_y, gray);
        }
      }

      for (odb::dbBox* obs : master->getObstructions()) {
        odb::Rect box = obs->getBox();
        inst->getTransform().apply(box);
        if (!box.overlaps(dbu_tile)) {
          continue;
        }
        const odb::Rect overlap = box.intersect(dbu_tile);
        const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

        for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
          for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
            const int draw_y = (255 - iy);
            setPixel(image_buffer, ix, draw_y, obs_color);
          }
        }
      }

      for (odb::dbMTerm* mterm : master->getMTerms()) {
        for (odb::dbMPin* mpin : mterm->getMPins()) {
          for (odb::dbBox* geom : mpin->getGeometry()) {
            odb::Rect box = geom->getBox();
            inst->getTransform().apply(box);
            if (!box.overlaps(dbu_tile)) {
              continue;
            }
            const odb::Rect overlap = box.intersect(dbu_tile);
            const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

            for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
              for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
                const int draw_y = (255 - iy);
                setPixel(image_buffer, ix, draw_y, color);
              }
            }
          }
        }
      }
    }
  }

  std::vector<unsigned char> png_data;
  unsigned error = lodepng::encode(
      png_data, image_buffer, kTileSizeInPixel, kTileSizeInPixel);
  if (error) {
    logger_->report("PNG encoder error: {}", lodepng_error_text(error));
  }

  return png_data;
}

//------------------------------------------------------------------------------
// Transport-agnostic request/response types and dispatch
//------------------------------------------------------------------------------

struct WsRequest
{
  uint32_t id = 0;
  enum Type
  {
    TILE,
    BOUNDS,
    LAYERS,
    UNKNOWN
  } type
      = UNKNOWN;
  std::string layer;
  int z = 0;
  int x = 0;
  int y = 0;
};

struct WsResponse
{
  uint32_t id = 0;
  // 0 = JSON payload, 1 = PNG payload, 2 = error
  uint8_t type = 0;
  std::vector<unsigned char> payload;
};

// Minimal JSON field extraction (no JSON library dependency)
static std::string extract_string(const std::string& json,
                                  const std::string& key)
{
  const std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos) {
    return {};
  }
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos) {
    return {};
  }
  auto quote_start = json.find('"', pos + 1);
  if (quote_start == std::string::npos) {
    return {};
  }
  auto quote_end = json.find('"', quote_start + 1);
  if (quote_end == std::string::npos) {
    return {};
  }
  return json.substr(quote_start + 1, quote_end - quote_start - 1);
}

static int extract_int(const std::string& json, const std::string& key)
{
  const std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos) {
    return 0;
  }
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos) {
    return 0;
  }
  // Skip whitespace after colon
  pos++;
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) {
    pos++;
  }
  try {
    return std::stoi(json.substr(pos));
  } catch (...) {
    return 0;
  }
}

static WsRequest parse_ws_request(const std::string& msg)
{
  WsRequest req;
  req.id = static_cast<uint32_t>(extract_int(msg, "id"));

  std::string type_str = extract_string(msg, "type");
  if (type_str == "tile") {
    req.type = WsRequest::TILE;
    req.layer = extract_string(msg, "layer");
    req.z = extract_int(msg, "z");
    req.x = extract_int(msg, "x");
    req.y = extract_int(msg, "y");
  } else if (type_str == "bounds") {
    req.type = WsRequest::BOUNDS;
  } else if (type_str == "layers") {
    req.type = WsRequest::LAYERS;
  } else {
    req.type = WsRequest::UNKNOWN;
  }
  return req;
}

static WsResponse dispatch_request(const WsRequest& req,
                                   const std::shared_ptr<TileGenerator>& gen)
{
  WsResponse resp;
  resp.id = req.id;

  switch (req.type) {
    case WsRequest::BOUNDS: {
      resp.type = 0;  // JSON
      const odb::Rect bounds = gen->getBounds();
      std::stringstream ss;
      ss << "{\"bounds\": [[" << bounds.yMin() << ", " << bounds.xMin()
         << "], [" << bounds.yMax() << ", " << bounds.xMax() << "]]}";
      const std::string json = ss.str();
      resp.payload.assign(json.begin(), json.end());
      break;
    }
    case WsRequest::LAYERS: {
      resp.type = 0;  // JSON
      const std::string json = "{\"layers\": [\"m1\"]}";
      resp.payload.assign(json.begin(), json.end());
      break;
    }
    case WsRequest::TILE: {
      resp.type = 1;  // PNG
      resp.payload = gen->generateTile(req.layer, req.z, req.x, req.y);
      break;
    }
    default: {
      resp.type = 2;  // error
      const std::string err = "Unknown request type";
      resp.payload.assign(err.begin(), err.end());
      break;
    }
  }

  return resp;
}

// Serialize a WsResponse into the binary wire format:
//   [0..3] uint32_t id (big-endian)
//   [4]    uint8_t  type
//   [5..7] reserved
//   [8..]  payload
static std::vector<unsigned char> serialize_response(const WsResponse& resp)
{
  std::vector<unsigned char> frame(8 + resp.payload.size());
  const uint32_t id_be = htonl(resp.id);
  std::memcpy(frame.data(), &id_be, 4);
  frame[4] = resp.type;
  frame[5] = frame[6] = frame[7] = 0;
  if (!resp.payload.empty()) {
    std::memcpy(frame.data() + 8, resp.payload.data(), resp.payload.size());
  }
  return frame;
}

//------------------------------------------------------------------------------
// HTTP request handler (wraps dispatch_request for HTTP transport)
//------------------------------------------------------------------------------

http::response<http::string_body> handle_request(
    http::request<http::string_body>&& req,
    std::shared_ptr<TileGenerator> generator)
{
  http::response<http::string_body> res{http::status::ok, req.version()};
  res.set(http::field::server, "Boost.Beast Server (C++17)");
  res.set(http::field::content_type, "text/plain");
  res.keep_alive(req.keep_alive());
  res.set(http::field::access_control_allow_origin, "*");

  std::regex tile_regex(R"(/tile/(\w+)/(\d+)/(-?\d+)/(-?\d+)\.png)");
  std::smatch match_pieces;
  std::string target_path(req.target());

  if (req.method() == http::verb::get && req.target() == "/bounds") {
    WsRequest ws_req;
    ws_req.type = WsRequest::BOUNDS;
    WsResponse ws_resp = dispatch_request(ws_req, generator);
    res.set(http::field::content_type, "application/json");
    res.body() = std::string(ws_resp.payload.begin(), ws_resp.payload.end());
  } else if (req.method() == http::verb::get && req.target() == "/layers") {
    WsRequest ws_req;
    ws_req.type = WsRequest::LAYERS;
    WsResponse ws_resp = dispatch_request(ws_req, generator);
    res.set(http::field::content_type, "application/json");
    res.body() = std::string(ws_resp.payload.begin(), ws_resp.payload.end());
  } else if (req.method() == http::verb::get
             && std::regex_match(target_path, match_pieces, tile_regex)) {
    WsRequest ws_req;
    ws_req.type = WsRequest::TILE;
    ws_req.layer = match_pieces[1].str();
    ws_req.z = std::stoi(match_pieces[2].str());
    ws_req.x = std::stoi(match_pieces[3].str());
    ws_req.y = std::stoi(match_pieces[4].str());
    WsResponse ws_resp = dispatch_request(ws_req, generator);

    res.set(http::field::content_type, "image/png");
    res.body()
        = std::string(ws_resp.payload.begin(), ws_resp.payload.end());
    res.set(http::field::cache_control, "public, max-age=604800");
  } else {
    res.result(http::status::not_found);
    res.body() = "Resource not found.";
  }

  res.prepare_payload();
  return res;
}

//------------------------------------------------------------------------------
// WebSocket session - multiplexes many requests over a single connection
//------------------------------------------------------------------------------

class ws_session : public std::enable_shared_from_this<ws_session>
{
  websocket::stream<beast::tcp_stream> ws_;
  beast::flat_buffer buffer_;
  std::shared_ptr<TileGenerator> generator_;

  // Write serialization: strand + queue ensures one async_write at a time
  net::strand<net::any_io_executor> strand_;
  std::deque<std::vector<unsigned char>> write_queue_;
  bool writing_ = false;

 public:
  ws_session(tcp::socket&& socket, std::shared_ptr<TileGenerator> generator)
      : ws_(std::move(socket)),
        generator_(std::move(generator)),
        strand_(net::make_strand(ws_.get_executor()))
  {
  }

  // Accept the WebSocket upgrade using the already-read HTTP request
  void run(http::request<http::string_body>&& req)
  {
    ws_.set_option(
        websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws_.set_option(
        websocket::stream_base::decorator([](websocket::response_type& res) {
          res.set(http::field::server, "OpenROAD WebSocket Server");
        }));

    ws_.async_accept(
        req,
        [self = shared_from_this()](beast::error_code ec) {
          self->on_accept(ec);
        });
  }

 private:
  void on_accept(beast::error_code ec)
  {
    if (ec) {
      std::cerr << "ws accept error: " << ec.message() << "\n";
      return;
    }
    do_read();
  }

  void do_read()
  {
    ws_.async_read(
        buffer_,
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
          self->on_read(ec);
        });
  }

  void on_read(beast::error_code ec)
  {
    if (ec) {
      if (ec != websocket::error::closed) {
        std::cerr << "ws read error: " << ec.message() << "\n";
      }
      return;
    }

    // Parse the incoming text message as a request
    const std::string msg = beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());

    const WsRequest req = parse_ws_request(msg);

    // Dispatch tile generation to the thread pool (not to the strand).
    // This lets multiple tiles render concurrently across all 32 threads.
    auto self = shared_from_this();
    auto gen = generator_;
    net::post(ws_.get_executor(), [self, gen, req]() {
      WsResponse resp = dispatch_request(req, gen);
      self->queue_response(resp);
    });

    // Immediately start reading the next request
    do_read();
  }

  void queue_response(const WsResponse& resp)
  {
    std::vector<unsigned char> frame = serialize_response(resp);

    // Post to the strand to serialize write queue access
    net::post(strand_, [self = shared_from_this(),
                        frame = std::move(frame)]() mutable {
      self->write_queue_.push_back(std::move(frame));
      if (!self->writing_) {
        self->do_write();
      }
    });
  }

  void do_write()
  {
    if (write_queue_.empty()) {
      writing_ = false;
      return;
    }
    writing_ = true;
    ws_.binary(true);
    ws_.async_write(
        net::buffer(write_queue_.front()),
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
          // Post back to the strand to serialize queue access
          net::post(self->strand_, [self, ec]() {
            if (ec) {
              std::cerr << "ws write error: " << ec.message() << "\n";
              return;
            }
            self->write_queue_.pop_front();
            self->do_write();
          });
        });
  }
};

//------------------------------------------------------------------------------
// HTTP session - handles traditional HTTP connections
//------------------------------------------------------------------------------

class session : public std::enable_shared_from_this<session>
{
  beast::tcp_stream stream_;
  beast::flat_buffer buffer_;
  std::shared_ptr<TileGenerator> generator_;
  std::shared_ptr<http::response<http::string_body>> res_;
  http::request<http::string_body> req_;

 public:
  session(tcp::socket&& socket, std::shared_ptr<TileGenerator> generator)
      : stream_(std::move(socket)), generator_(generator)
  {
  }

  void run() { do_read(); }

  // Entry point when the first request was already read by detect_session
  void run_with_request(http::request<http::string_body> req,
                        beast::flat_buffer buffer)
  {
    req_ = std::move(req);
    buffer_ = std::move(buffer);
    on_read({});
  }

 private:
  void do_read()
  {
    req_ = {};
    http::async_read(
        stream_,
        buffer_,
        req_,
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
          self->on_read(ec);
        });
  }

  void on_read(beast::error_code ec)
  {
    if (ec == http::error::end_of_stream) {
      return do_close();
    }
    if (ec) {
      std::cerr << "Session read error: " << ec.message() << "\n";
      return;
    }

    res_ = std::make_shared<http::response<http::string_body>>(
        handle_request(std::move(req_), generator_));
    do_write();
  }

  void do_write()
  {
    http::async_write(
        stream_,
        *res_,
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
          self->on_write(ec);
        });
  }

  void on_write(beast::error_code ec)
  {
    if (ec) {
      std::cerr << "Session write error: " << ec.message() << "\n";
      return;
    }

    bool keep_alive = res_->keep_alive();
    res_ = nullptr;

    if (keep_alive) {
      do_read();
    } else {
      do_close();
    }
  }

  void do_close()
  {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
  }
};

//------------------------------------------------------------------------------
// Detect session - reads first HTTP request, routes to WS or HTTP session
//------------------------------------------------------------------------------

class detect_session : public std::enable_shared_from_this<detect_session>
{
  beast::tcp_stream stream_;
  beast::flat_buffer buffer_;
  std::shared_ptr<TileGenerator> generator_;
  http::request<http::string_body> req_;

 public:
  detect_session(tcp::socket&& socket,
                 std::shared_ptr<TileGenerator> generator)
      : stream_(std::move(socket)), generator_(std::move(generator))
  {
  }

  void run()
  {
    http::async_read(
        stream_,
        buffer_,
        req_,
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
          self->on_read(ec);
        });
  }

 private:
  void on_read(beast::error_code ec)
  {
    if (ec) {
      std::cerr << "Detect read error: " << ec.message() << "\n";
      return;
    }

    if (websocket::is_upgrade(req_)) {
      // WebSocket upgrade - hand off to ws_session
      auto ws = std::make_shared<ws_session>(
          stream_.release_socket(), generator_);
      ws->run(std::move(req_));
    } else {
      // Regular HTTP - hand off to session with already-read request
      auto s = std::make_shared<session>(
          stream_.release_socket(), generator_);
      s->run_with_request(std::move(req_), std::move(buffer_));
    }
  }
};

//------------------------------------------------------------------------------
// Listener - accepts incoming connections
//------------------------------------------------------------------------------

class listener : public std::enable_shared_from_this<listener>
{
  net::io_context& ioc_;
  tcp::acceptor acceptor_;
  std::shared_ptr<TileGenerator> generator_;

 public:
  listener(net::io_context& ioc,
           tcp::endpoint endpoint,
           std::shared_ptr<TileGenerator> generator)
      : ioc_(ioc), acceptor_(ioc), generator_(generator)
  {
    beast::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      return;
    }

    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
      return;
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
      return;
    }

    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
      return;
    }
  }

  void run() { do_accept(); }

 private:
  void do_accept()
  {
    acceptor_.async_accept(
        ioc_,
        [self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
          self->on_accept(ec, std::move(socket));
        });
  }

  void on_accept(beast::error_code ec, tcp::socket socket)
  {
    if (ec) {
      std::cerr << "Listener accept error: " << ec.message() << "\n";
    } else {
      // Route through detect_session to handle both HTTP and WebSocket
      std::make_shared<detect_session>(std::move(socket), generator_)->run();
    }
    do_accept();
  }
};

WebServer::WebServer(odb::dbDatabase* db, utl::Logger* logger)
    : db_(db), logger_(logger)
{
}

WebServer::~WebServer() = default;

void WebServer::serve()
{
  try {
    generator_ = std::make_shared<TileGenerator>(db_, logger_);

    auto const address = net::ip::make_address("127.0.0.1");
    unsigned short const port = 8080;
    int const num_threads = 32;

    logger_->info(utl::WEB,
                  1,
                  "Server starting on http://:{} with {} threads...",
                  port,
                  num_threads);

    net::io_context ioc{num_threads};

    std::make_shared<listener>(ioc, tcp::endpoint{address, port}, generator_)
        ->run();

    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) {
      logger_->info(utl::WEB, 3, "Shutting down...");
      ioc.stop();
    });

    std::vector<std::thread> threads;
    threads.reserve(num_threads - 1);
    for (int i = 0; i < num_threads - 1; ++i) {
      threads.emplace_back([&ioc] { ioc.run(); });
    }

    ioc.run();

    for (auto& t : threads) {
      t.join();
    }

    std::cout << "Server stopped.\n";
  } catch (std::exception const& e) {
    logger_->error(utl::WEB, 2, "Server error : {}", e.what());
  }
}

}  // namespace web
