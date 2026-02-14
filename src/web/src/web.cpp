// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "web/web.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <memory>  // For shared_ptr and enable_shared_from_this
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "color.h"
#include "cpp-httplib/httplib.h"
#include "lodepng.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "search.h"
#include "utl/Logger.h"
#include "utl/timer.h"

namespace web {

namespace beast = boost::beast;
namespace http = beast::http;
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
  // utl::DebugScopedTimer timer(logger_, utl::WEB, "tile", 1, "generateTile
  // {}");
  static_assert(sizeof(Color) == 4);
  std::vector<unsigned char> image_buffer(
      kTileSizeInPixel * kTileSizeInPixel * 4, 0);

  Color color{0, 0, 255, 180};
  Color obs_color = color.lighter();

  // 2. Determine our tile's bounding box in dbu coordinates.
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
      // const odb::Rect pixel_bbox = toPixels(scale, inst_bbox, dbu_tile);

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
    // Draw tile border
    if (false) {
      for (int ix = 0; ix < 255; ++ix) {
        setPixel(image_buffer, ix, 0, {255, 0, 0, 255});
        setPixel(image_buffer, ix, 255, {255, 0, 0, 255});
      }
      for (int iy = 0; iy < 255; ++iy) {
        setPixel(image_buffer, 0, iy, {255, 0, 0, 255});
        setPixel(image_buffer, 255, iy, {255, 0, 0, 255});
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
/**
 * @brief This is your "Router". (Unchanged from C++20)
 *
 * This function is called by the session to handle a complete
 * request. It's where you implement your endpoint logic.
 */
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

  // --- Endpoint Routing Logic ---
  if (req.method() == http::verb::get && req.target() == "/bounds") {
    // const odb::Rect bounds = getBounds();
    const odb::Rect bounds{0, 0, 20000, 20000};
    std::stringstream ss;
    // Returns bounds in Leaflet's [y, x] format: [[yMin, xMin], [yMax, xMax]]
    ss << "{\"bounds\": [[" << bounds.yMin() << ", " << bounds.xMin() << "], ["
       << bounds.yMax() << ", " << bounds.xMax() << "]]}";
    res.set(http::field::content_type, "application/json");
    res.body() = ss.str();
  } else if (req.method() == http::verb::get && req.target() == "/layers") {
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"layers\": [\"m1\"]}";
  } else if (req.method() == http::verb::get
             && std::regex_match(target_path, match_pieces, tile_regex)) {
    std::string layer = match_pieces[1].str();
    int z = std::stoi(match_pieces[2].str());
    int x = std::stoi(match_pieces[3].str());
    int y = std::stoi(match_pieces[4].str());

    std::vector<unsigned char> png_data
        = generator->generateTile(layer, z, x, y);

    res.set(http::field::content_type, "image/png");
    res.body() = std::string(png_data.begin(), png_data.end());
    res.set(http::field::cache_control, "public, max-age=604800");
  } else {
    res.result(http::status::not_found);
    res.body() = "Resource not found.";
  }
  // --- End Routing Logic ---

  res.prepare_payload();
  return res;
}

//------------------------------------------------------------------------------

/**
 * @brief Handles a single HTTP server connection.
 *
 * Inherits from enable_shared_from_this to manage its own lifetime.
 */
class session : public std::enable_shared_from_this<session>
{
  beast::tcp_stream stream_;
  beast::flat_buffer buffer_;
  std::shared_ptr<TileGenerator> generator_;

  // We use a shared_ptr for the response because http::async_write
  // requires a non-const reference, and we need to keep it
  // alive during the async operation.
  std::shared_ptr<http::response<http::string_body>> res_;

  // The request parser.
  // Note: We'd use a parser if we weren't using the simple http::string_body
  http::request<http::string_body> req_;

 public:
  // Take ownership of the socket
  session(tcp::socket&& socket, std::shared_ptr<TileGenerator> generator)
      : stream_(std::move(socket)), generator_(generator)
  {
  }

  // Start the asynchronous operations
  void run()
  {
    // We need to be executing within an asio strand to
    // safely access the member variables.
    // For this simple example, the callback chain itself
    // provides implicit stranding, so we start with a read.
    do_read();
  }

 private:
  void do_read()
  {
    // Clear the request
    req_ = {};

    // Read a request
    http::async_read(
        stream_,
        buffer_,
        req_,
        // Capture 'self' to keep this object alive
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
          self->on_read(ec);
        });
  }

  void on_read(beast::error_code ec)
  {
    // This means the client closed the connection
    if (ec == http::error::end_of_stream) {
      return do_close();
    }

    if (ec) {
      std::cerr << "Session read error: " << ec.message() << "\n";
      return;
    }

    // --- We have a request, now send a response ---

    // Create the response object. We use a shared_ptr
    // so it lives long enough for the async_write.
    res_ = std::make_shared<http::response<http::string_body>>(
        handle_request(std::move(req_), generator_));

    do_write();
  }

  void do_write()
  {
    // Write the response
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

    // Check if we should close the connection
    bool keep_alive = res_->keep_alive();

    // Clear the response to free memory
    res_ = nullptr;

    if (keep_alive) {
      // Go back to reading the next request
      do_read();
    } else {
      // This means "Connection: close" was set
      do_close();
    }
  }

  void do_close()
  {
    // Send a TCP shutdown
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    // At this point, the connection is closed.
    // The session object will be destroyed when the last shared_ptr goes away.
  }
};

//------------------------------------------------------------------------------

/**
 * @brief Accepts incoming connections and launches sessions
 */
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

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) { /* handle error */
      return;
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) { /* handle error */
      return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) { /* handle error */
      return;
    }

    // Start listening for connections
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) { /* handle error */
      return;
    }
  }

  // Start accepting connections
  void run() { do_accept(); }

 private:
  void do_accept()
  {
    // Asynchronously wait for a new connection
    acceptor_.async_accept(
        ioc_,  // The executor for the socket
        [self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
          self->on_accept(ec, std::move(socket));
        });
  }

  void on_accept(beast::error_code ec, tcp::socket socket)
  {
    if (ec) {
      std::cerr << "Listener accept error: " << ec.message() << "\n";
      // Don't stop, just log and keep accepting
    } else {
      // We have a new connection.
      // Create a 'session' and run it.
      std::make_shared<session>(std::move(socket), generator_)->run();
    }

    // Immediately go back to accepting the *next* connection
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
                  //"Server starting on http://{}:{} with {} threads...",
                  // address,
                  "Server starting on http://:{} with {} threads...",
                  port,
                  num_threads);

    // The I/O context is the "engine" for all async I/O
    net::io_context ioc{num_threads};

    // Create and launch the main 'listener'
    // It will start accepting connections
    std::make_shared<listener>(ioc, tcp::endpoint{address, port}, generator_)
        ->run();

    // Set up a signal handler to gracefully shut down
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) {
      logger_->info(utl::WEB, 3, "Shutting down...");
      ioc.stop();
    });

    // Create a pool of threads to run the io_context
    std::vector<std::thread> threads;
    threads.reserve(num_threads - 1);
    for (int i = 0; i < num_threads - 1; ++i) {
      threads.emplace_back([&ioc] { ioc.run(); });
    }

    // The main thread also joins the pool
    ioc.run();

    // Wait for all threads to finish
    for (auto& t : threads) {
      t.join();
    }

    std::cout << "Server stopped.\n";
  } catch (std::exception const& e) {
    logger_->error(utl::WEB, 2, "Server error : {}", e.what());
  }
}

}  // namespace web
