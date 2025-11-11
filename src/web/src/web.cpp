// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "web/web.h"

#include <memory>
#include <utility>

#include "color.h"
#include "cpp-httplib/httplib.h"
#include "lodepng.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"

namespace web {

WebServer::WebServer(odb::dbDatabase* db, utl::Logger* logger)
    : db_(db), logger_(logger)
{
}

void WebServer::setPixel(std::vector<unsigned char>& image,
                         const int x,
                         const int y,
                         const Color& c)
{
  if (x < 0 || x >= tile_size_in_pixel || y < 0 || y >= tile_size_in_pixel) {
    return;
  }
  const int index = (y * tile_size_in_pixel + x) * 4;
  image[index + 0] = c.r;
  image[index + 1] = c.g;
  image[index + 2] = c.b;
  image[index + 3] = c.a;
}

odb::Rect toPixels(const double scale,
                   const odb::Rect& rect,
                   const odb::Rect& dbu_tile)
{
  return odb::Rect((rect.xMin() - dbu_tile.xMin()) * scale,
                   (rect.yMin() - dbu_tile.yMin()) * scale,
                   std::ceil((rect.xMax() - dbu_tile.xMin()) * scale),
                   std::ceil((rect.yMax() - dbu_tile.yMin()) * scale));
}

std::vector<unsigned char> WebServer::generateTile(const std::string& layer,
                                                   const int z,
                                                   const int x,
                                                   int y)
{
  static_assert(sizeof(Color) == 4);
  std::vector<unsigned char> image_buffer(
      tile_size_in_pixel * tile_size_in_pixel * 4, 0);

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
    const double scale = tile_size_in_pixel / tile_dbu_size;

    odb::dbBlock* block = db_->getChip()->getBlock();
    for (odb::dbInst* inst : block->getInsts()) {
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
    for (int ix = 0; ix < 255; ++ix) {
      setPixel(image_buffer, ix, 0, {255, 0, 0, 255});
      setPixel(image_buffer, ix, 255, {255, 0, 0, 255});
    }
    for (int iy = 0; iy < 255; ++iy) {
      setPixel(image_buffer, 0, iy, {255, 0, 0, 255});
      setPixel(image_buffer, 255, iy, {255, 0, 0, 255});
    }
  }

  std::vector<unsigned char> png_data;
  unsigned error = lodepng::encode(
      png_data, image_buffer, tile_size_in_pixel, tile_size_in_pixel);
  if (error) {
    logger_->report("PNG encoder error: {}", lodepng_error_text(error));
  }

  return png_data;
}

odb::Rect WebServer::getBounds() const
{
  odb::Rect bounds;
  if (odb::dbChip* chip = db_->getChip()) {
    if (odb::dbBlock* block = chip->getBlock()) {
      bounds = block->getBBox()->getBox();
    }
  }
  return bounds;
}

void WebServer::serve()
{
  logger_->info(utl::WEB, 1, "Start web server");

  httplib::Server svr;

  svr.Get(R"(/tile/(\w+)/(\d+)/(-?\d+)/(-?\d+)\.png)",
          [&](const httplib::Request& req, httplib::Response& res) {
            std::string layer = req.matches[1].str();
            int z = std::stoi(req.matches[2].str());
            int x = std::stoi(req.matches[3].str());
            int y = std::stoi(req.matches[4].str());
            logger_->report("request png {} {} {} {}", layer, z, x, y);

            std::vector<unsigned char> png_data = generateTile(layer, z, x, y);

            res.set_header("Access-Control-Allow-Origin", "*");

            res.set_content(reinterpret_cast<const char*>(png_data.data()),
                            png_data.size(),
                            "image/png");
          });

  svr.Get("/bounds", [&](const httplib::Request& req, httplib::Response& res) {
    logger_->report("request bounds");
    const odb::Rect bounds = getBounds();
    std::stringstream ss;
    // Returns bounds in Leaflet's [y, x] format: [[yMin, xMin], [yMax, xMax]]
    ss << "{\"bounds\": [[" << bounds.yMin() << ", " << bounds.xMin() << "], ["
       << bounds.yMax() << ", " << bounds.xMax() << "]]}";

    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_content(ss.str(), "application/json");
  });

  svr.Get("/layers", [&](const httplib::Request& req, httplib::Response& res) {
    std::stringstream ss;
    ss << "{";
    ss << "\"layers\": [";

    odb::dbTech* tech = db_->getTech();
    if (tech) {
      bool first = true;
      for (odb::dbTechLayer* layer : tech->getLayers()) {
        const odb::dbTechLayerType type = layer->getType();
        if (type != odb::dbTechLayerType::ROUTING
            && type != odb::dbTechLayerType::CUT) {
          continue;
        }
        if (!first) {
          ss << ", ";
        }
        ss << "\"" << layer->getName() << "\"";
        first = false;
        break;  // FIXME
      }
    }

    ss << "]";
    ss << "}";

    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_content(ss.str(), "application/json");
  });
  svr.listen("0.0.0.0", 8080);
}

}  // namespace web
