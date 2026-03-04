// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "web/web.h"

#include <algorithm>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "color.h"
#include "db_sta/dbSta.hh"
#include "gui/descriptor_registry.h"
#include "gui/gui.h"
#include "lodepng.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "search.h"
#include "tile_generator.h"
#include "utl/Logger.h"
#include "utl/timer.h"

namespace web {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

TileGenerator::TileGenerator(odb::dbDatabase* db,
                             sta::dbSta* sta,
                             utl::Logger* logger)
    : db_(db), sta_(sta), logger_(logger), search_(std::make_unique<Search>())
{
  odb::dbChip* chip = db_->getChip();
  if (!chip) {
    logger_->error(utl::WEB, 99, "No chip to serve");
  }
  search_->setTopChip(chip);
}

TileGenerator::~TileGenerator() = default;

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

std::vector<std::string> TileGenerator::getRoutingLayers()
{
  std::vector<std::string> layers;
  odb::dbTech* tech = db_->getTech();
  for (odb::dbTechLayer* layer : tech->getLayers()) {
    if (layer->getRoutingLevel() > 0) {
      layers.push_back(layer->getName());
    }
  }
  return layers;
}

bool TileGenerator::isInstVisible(odb::dbInst* inst,
                                  const TileVisibility& vis) const
{
  odb::dbMaster* master = inst->getMaster();
  odb::dbMasterType mtype = master->getType();

  if (sta_) {
    using IT = sta::dbSta::InstType;
    switch (sta_->getInstanceType(inst)) {
      case IT::BLOCK:
        return vis.macros;
      case IT::PAD_INPUT:
        return vis.pad_input;
      case IT::PAD_OUTPUT:
        return vis.pad_output;
      case IT::PAD_INOUT:
        return vis.pad_inout;
      case IT::PAD_POWER:
        return vis.pad_power;
      case IT::PAD_SPACER:
        return vis.pad_spacer;
      case IT::PAD_AREAIO:
        return vis.pad_areaio;
      case IT::PAD:
        return vis.pad_other;
      case IT::ENDCAP:
        return vis.phys_endcap;
      case IT::FILL:
        return vis.phys_fill;
      case IT::TAPCELL:
        return vis.phys_welltap;
      case IT::TIE:
        return vis.phys_tie;
      case IT::ANTENNA:
        return vis.phys_antenna;
      case IT::COVER:
        return vis.phys_cover;
      case IT::BUMP:
        return vis.phys_bump;
      case IT::LEF_OTHER:
        return vis.phys_other;
      case IT::STD_BUF:
      case IT::STD_INV:
        return vis.std_bufinv;
      case IT::STD_BUF_TIMING_REPAIR:
      case IT::STD_INV_TIMING_REPAIR:
        return vis.std_bufinv_timing;
      case IT::STD_BUF_CLK_TREE:
      case IT::STD_INV_CLK_TREE:
        return vis.std_clock_bufinv;
      case IT::STD_CLOCK_GATE:
        return vis.std_clock_gate;
      case IT::STD_LEVEL_SHIFT:
        return vis.std_level_shift;
      case IT::STD_SEQUENTIAL:
        return vis.std_sequential;
      case IT::STD_COMBINATIONAL:
        return vis.std_combinational;
      case IT::STD_CELL:
      case IT::STD_PHYSICAL:
      case IT::STD_OTHER:
      default:
        return vis.stdcells;
    }
  }

  // Fallback: dbMasterType-only classification (no Liberty)
  if (mtype.isBlock()) {
    return vis.macros;
  }
  if (mtype.isPad()) {
    if (mtype == odb::dbMasterType::PAD_INPUT) {
      return vis.pad_input;
    }
    if (mtype == odb::dbMasterType::PAD_OUTPUT) {
      return vis.pad_output;
    }
    if (mtype == odb::dbMasterType::PAD_INOUT) {
      return vis.pad_inout;
    }
    if (mtype == odb::dbMasterType::PAD_POWER) {
      return vis.pad_power;
    }
    if (mtype == odb::dbMasterType::PAD_SPACER) {
      return vis.pad_spacer;
    }
    if (mtype == odb::dbMasterType::PAD_AREAIO) {
      return vis.pad_areaio;
    }
    return vis.pad_other;
  }
  if (mtype.isEndCap()) {
    return vis.phys_endcap;
  }
  if (master->isFiller()) {
    return vis.phys_fill;
  }
  if (mtype == odb::dbMasterType::CORE_WELLTAP) {
    return vis.phys_welltap;
  }
  if (mtype == odb::dbMasterType::CORE_TIEHIGH
      || mtype == odb::dbMasterType::CORE_TIELOW) {
    return vis.phys_tie;
  }
  if (mtype == odb::dbMasterType::CORE_ANTENNACELL) {
    return vis.phys_antenna;
  }
  if (mtype.isCover()) {
    if (mtype == odb::dbMasterType::COVER_BUMP) {
      return vis.phys_bump;
    }
    return vis.phys_cover;
  }
  if (mtype == odb::dbMasterType::CORE_SPACER
      || inst->getSourceType() == odb::dbSourceType::DIST) {
    return vis.phys_other;
  }
  return vis.stdcells;
}

std::vector<SelectionResult> TileGenerator::selectAt(const int dbu_x,
                                                     const int dbu_y,
                                                     const int zoom,
                                                     const TileVisibility& vis)
{
  std::vector<SelectionResult> results;
  odb::dbBlock* block = db_->getChip()->getBlock();
  // Compute a search margin of 2 pixels at the current zoom level.
  // This accounts for coordinate conversion rounding between the client's
  // Leaflet CRS.Simple coordinates and the server's DBU space.
  const int num_tiles = 1 << std::max(0, zoom);
  const int margin
      = std::max(1, static_cast<int>(getBounds().maxDXDY()
                                     / (kTileSizeInPixel * num_tiles) * 2));
  debugPrint(logger_,
             utl::WEB,
             "select",
             1,
             "selectAt dbu=({},{}) zoom={} margin={}",
             dbu_x,
             dbu_y,
             zoom,
             margin);
  int rtree_count = 0;
  for (odb::dbInst* inst : search_->searchInsts(block,
                                                 dbu_x - margin,
                                                 dbu_y - margin,
                                                 dbu_x + margin,
                                                 dbu_y + margin)) {
    ++rtree_count;
    const odb::Rect bbox = inst->getBBox()->getBox();
    const bool hits = bbox.intersects(odb::Point(dbu_x, dbu_y));
    const bool visible = isInstVisible(inst, vis);
    debugPrint(logger_,
               utl::WEB,
               "select",
               2,
               "  candidate {} master={} bbox=({},{},{},{}) "
               "intersects={} visible={}",
               inst->getName(),
               inst->getMaster()->getName(),
               bbox.xMin(),
               bbox.yMin(),
               bbox.xMax(),
               bbox.yMax(),
               hits,
               visible);
    if (hits && visible) {
      results.push_back(
          {inst, inst->getName(), inst->getMaster()->getName(), bbox});
    }
  }
  debugPrint(logger_,
             utl::WEB,
             "select",
             1,
             "  rtree_results={} selected={}",
             rtree_count,
             results.size());
  // Sort by area descending so larger instances (macros) come first
  std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
    return a.bbox.area() > b.bbox.area();
  });
  return results;
}

std::vector<unsigned char> TileGenerator::generateTile(
    const std::string& layer,
    const int z,
    const int x,
    int y,
    const TileVisibility& vis,
    const std::set<odb::dbInst*>& selected)
{
  static_assert(sizeof(Color) == 4);
  std::vector<unsigned char> image_buffer(
      kTileSizeInPixel * kTileSizeInPixel * 4, 0);

  // Per-layer colors: routing level 1=blue, 2=red, then distinct hues
  static const Color palette[] = {
      {70, 130, 210, 180},  // moderate blue
      {200, 50, 50, 180},   // red
      {50, 180, 80, 180},   // green
      {200, 160, 40, 180},  // amber
      {160, 60, 200, 180},  // purple
      {40, 190, 190, 180},  // teal
      {220, 120, 50, 180},  // orange
      {180, 70, 150, 180},  // magenta
  };
  static constexpr int palette_size = sizeof(palette) / sizeof(palette[0]);

  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* tech_layer = tech->findLayer(layer.c_str());

  int layer_index = 0;
  if (tech_layer) {
    layer_index = std::max(0, tech_layer->getRoutingLevel() - 1);
  }
  const Color color = palette[layer_index % palette_size];
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

    // Special "_instances" layer: only draw instance borders, no routing
    const bool instances_only = (layer == "_instances");

    // Draw instances
    for (odb::dbInst* inst : search_->searchInsts(
             block, dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max)) {
      odb::Rect inst_bbox = inst->getBBox()->getBox();
      if (!dbu_tile.overlaps(inst_bbox)) {
        continue;
      }
      odb::dbMaster* master = inst->getMaster();

      if (!isInstVisible(inst, vis)) {
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

      if (instances_only) {
        // Draw the rectangle border (instances-only layer)
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
      } else {
        // Layer-specific: obstructions and pins
        if (vis.blockages) {
          for (odb::dbBox* obs : master->getObstructions()) {
            if (tech_layer && obs->getTechLayer() != tech_layer) {
              continue;
            }
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
        }

        if (vis.pins) {
          for (odb::dbMTerm* mterm : master->getMTerms()) {
            for (odb::dbMPin* mpin : mterm->getMPins()) {
              for (odb::dbBox* geom : mpin->getGeometry()) {
                if (tech_layer && geom->getTechLayer() != tech_layer) {
                  continue;
                }
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
    }

    // Draw routing shapes (wires, vias, bterms) on top of instances
    if (!instances_only && tech_layer && vis.routing) {
      for (const auto& shape : search_->searchBoxShapes(
               block, tech_layer, dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max)) {
        if (!vis.isNetVisible(std::get<2>(shape))) {
          continue;
        }
        const odb::Rect& box = std::get<0>(shape);
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

    // Draw special net shapes (power/ground straps) on top of instances
    if (!instances_only && tech_layer && vis.special_nets) {
      for (const auto& shape : search_->searchSNetShapes(
               block, tech_layer, dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max)) {
        if (!vis.isNetVisible(std::get<2>(shape))) {
          continue;
        }
        const odb::Rect box = std::get<0>(shape)->getBox();
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

    if (!selected.empty()) {
      drawSelection(image_buffer, selected, dbu_tile, scale);
    }
  }

  if (vis.debug) {
    drawDebugOverlay(image_buffer, z, x, y);
  }

  std::vector<unsigned char> png_data;
  unsigned error = lodepng::encode(
      png_data, image_buffer, kTileSizeInPixel, kTileSizeInPixel);
  if (error) {
    logger_->report("PNG encoder error: {}", lodepng_error_text(error));
  }

  // Debug: save tile to file for inspection
  {
    std::string filename = "/tmp/tile_" + layer + "_" + std::to_string(z) + "_"
                           + std::to_string(x) + "_" + std::to_string(y)
                           + ".png";
    lodepng::save_file(png_data, filename);
  }

  return png_data;
}

void TileGenerator::drawDebugOverlay(std::vector<unsigned char>& image,
                                     const int z,
                                     const int x,
                                     const int y)
{
  // Minimal 5x7 bitmap font for digits 0-9 and punctuation.
  // Each glyph is 5 columns wide, 7 rows tall.  Stored as 7 bytes per glyph
  // where each byte is one row (MSB = leftmost pixel, only 5 bits used).
  // clang-format off
  static const unsigned char font[][7] = {
    // '0'
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
    // '1'
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    // '2'
    {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F},
    // '3'
    {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E},
    // '4'
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    // '5'
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    // '6'
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    // '7'
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    // '8'
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    // '9'
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},
    // '/'  (index 10)
    {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10},
    // '='  (index 11)
    {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00},
    // 'x'  (index 12)
    {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11},
    // 'y'  (index 13)
    {0x00, 0x00, 0x11, 0x0A, 0x04, 0x04, 0x04},
    // 'z'  (index 14)
    {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F},
  };
  // clang-format on

  const Color yellow{255, 255, 0, 255};
  const int last = kTileSizeInPixel - 1;

  // Draw 1-pixel yellow border
  for (int i = 0; i < kTileSizeInPixel; ++i) {
    setPixel(image, i, 0, yellow);
    setPixel(image, i, last, yellow);
    setPixel(image, 0, i, yellow);
    setPixel(image, last, i, yellow);
  }

  // Build the label string "z=<zoom> <x>/<y>"
  std::string label = "z=" + std::to_string(z) + " " + std::to_string(x) + "/"
                      + std::to_string(y);

  // Draw each character at 3x scale (5x7 glyphs become 15x21 pixels).
  constexpr int glyph_w = 5;
  constexpr int glyph_h = 7;
  constexpr int scale = 3;
  constexpr int spacing = scale;  // 3px gap between characters
  int cx = 4;                     // starting x pixel
  const int cy = 4;               // starting y pixel

  for (const char ch : label) {
    int glyph_idx = -1;
    if (ch >= '0' && ch <= '9') {
      glyph_idx = ch - '0';
    } else if (ch == '/') {
      glyph_idx = 10;
    } else if (ch == '=') {
      glyph_idx = 11;
    } else if (ch == 'x') {
      glyph_idx = 12;
    } else if (ch == 'y') {
      glyph_idx = 13;
    } else if (ch == 'z') {
      glyph_idx = 14;
    } else if (ch == ' ') {
      cx += glyph_w * scale + spacing;
      continue;
    }
    if (glyph_idx < 0) {
      cx += glyph_w * scale + spacing;
      continue;
    }

    for (int row = 0; row < glyph_h; ++row) {
      const unsigned char bits = font[glyph_idx][row];
      for (int col = 0; col < glyph_w; ++col) {
        if (bits & (0x10 >> col)) {
          for (int sy = 0; sy < scale; ++sy) {
            for (int sx = 0; sx < scale; ++sx) {
              setPixel(
                  image, cx + col * scale + sx, cy + row * scale + sy, yellow);
            }
          }
        }
      }
    }
    cx += glyph_w * scale + spacing;
  }
}

/* static */
void TileGenerator::blendPixel(std::vector<unsigned char>& image,
                               const int x,
                               const int y,
                               const Color& c)
{
  if (x < 0 || x >= kTileSizeInPixel || y < 0 || y >= kTileSizeInPixel) {
    return;
  }
  const int i = (y * kTileSizeInPixel + x) * 4;
  const float a = c.a / 255.0f;
  const float inv_a = 1.0f - a;
  image[i + 0] = static_cast<unsigned char>(c.r * a + image[i + 0] * inv_a);
  image[i + 1] = static_cast<unsigned char>(c.g * a + image[i + 1] * inv_a);
  image[i + 2] = static_cast<unsigned char>(c.b * a + image[i + 2] * inv_a);
  image[i + 3] = std::max(image[i + 3], c.a);
}

void TileGenerator::drawSelection(std::vector<unsigned char>& image,
                                  const std::set<odb::dbInst*>& selected,
                                  const odb::Rect& dbu_tile,
                                  const double scale)
{
  const Color fill{255, 255, 0, 80};
  const Color border{255, 255, 0, 255};

  for (odb::dbInst* inst : selected) {
    const odb::Rect inst_bbox = inst->getBBox()->getBox();
    if (!dbu_tile.overlaps(inst_bbox)) {
      continue;
    }
    const odb::Rect overlap = inst_bbox.intersect(dbu_tile);
    const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

    // Semi-transparent yellow fill
    for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        blendPixel(image, ix, 255 - iy, fill);
      }
    }

    // Solid yellow border (only where instance edge is within the tile)
    const odb::Rect full_draw = toPixels(scale, inst_bbox, dbu_tile);
    // Left edge
    if (full_draw.xMin() >= 0 && full_draw.xMin() < kTileSizeInPixel) {
      for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
        setPixel(image, full_draw.xMin(), 255 - iy, border);
      }
    }
    // Right edge
    const int rx = std::min(full_draw.xMax() - 1, kTileSizeInPixel - 1);
    if (rx >= 0 && full_draw.xMax() > 0) {
      for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
        setPixel(image, rx, 255 - iy, border);
      }
    }
    // Bottom edge
    if (full_draw.yMin() >= 0 && full_draw.yMin() < kTileSizeInPixel) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        setPixel(image, ix, 255 - full_draw.yMin(), border);
      }
    }
    // Top edge
    const int ty = std::min(full_draw.yMax() - 1, kTileSizeInPixel - 1);
    if (ty >= 0 && full_draw.yMax() > 0) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        setPixel(image, ix, 255 - ty, border);
      }
    }
  }
}

//------------------------------------------------------------------------------
// Transport-agnostic request/response types and dispatch
//------------------------------------------------------------------------------

// Escape a string for safe inclusion in a JSON string value.
// Handles backslash and double-quote (the common offenders in Verilog names).
static std::string json_escape(const std::string& s)
{
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    if (c == '\\' || c == '"') {
      out += '\\';
    }
    out += c;
  }
  return out;
}

// Serialize a Descriptor::Property value to a JSON fragment.
// Leaf values: {"name":"...", "value":"..."}
// PropertyList / SelectionSet: {"name":"...", "children":[...]}
static void serializeProperty(std::stringstream& ss,
                              const gui::Descriptor::Property& prop)
{
  ss << "{\"name\": \"" << json_escape(prop.name) << "\"";

  // Check for compound types that should be rendered as expandable groups.
  if (auto* plist
      = std::any_cast<gui::Descriptor::PropertyList>(&prop.value)) {
    ss << ", \"children\": [";
    bool first = true;
    for (const auto& [key, val] : *plist) {
      if (!first) {
        ss << ", ";
      }
      std::string key_str = gui::Descriptor::Property::toString(key);
      std::string val_str = gui::Descriptor::Property::toString(val);
      ss << "{\"name\": \"" << json_escape(key_str) << "\", \"value\": \""
         << json_escape(val_str) << "\"}";
      first = false;
    }
    ss << "]";
  } else if (auto* sel_set
             = std::any_cast<gui::SelectionSet>(&prop.value)) {
    ss << ", \"children\": [";
    bool first = true;
    for (const auto& sel : *sel_set) {
      if (!first) {
        ss << ", ";
      }
      ss << "{\"name\": \"" << json_escape(sel.getName())
         << "\", \"value\": \"" << json_escape(sel.getTypeName()) << "\"}";
      first = false;
    }
    ss << "]";
  } else {
    // Leaf value — convert to string.
    std::string val_str = prop.toString();
    ss << ", \"value\": \"" << json_escape(val_str) << "\"";
  }

  ss << "}";
}

struct WsRequest
{
  uint32_t id = 0;
  enum Type
  {
    TILE,
    BOUNDS,
    LAYERS,
    INFO,
    SELECT,
    UNKNOWN
  } type
      = UNKNOWN;
  std::string layer;
  int z = 0;
  int x = 0;
  int y = 0;

  // SELECT fields
  int select_x = 0;
  int select_y = 0;
  int select_zoom = 0;

  // Visibility flags (default: all visible)
  TileVisibility vis;
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

// Like extract_int but returns default_val when key is absent
static int extract_int_or(const std::string& json,
                          const std::string& key,
                          int default_val)
{
  const std::string needle = "\"" + key + "\"";
  if (json.find(needle) == std::string::npos) {
    return default_val;
  }
  return extract_int(json, key);
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
    req.vis.stdcells = extract_int_or(msg, "stdcells", 1);
    req.vis.macros = extract_int_or(msg, "macros", 1);
    // Pad sub-types
    req.vis.pad_input = extract_int_or(msg, "pad_input", 1);
    req.vis.pad_output = extract_int_or(msg, "pad_output", 1);
    req.vis.pad_inout = extract_int_or(msg, "pad_inout", 1);
    req.vis.pad_power = extract_int_or(msg, "pad_power", 1);
    req.vis.pad_spacer = extract_int_or(msg, "pad_spacer", 1);
    req.vis.pad_areaio = extract_int_or(msg, "pad_areaio", 1);
    req.vis.pad_other = extract_int_or(msg, "pad_other", 1);
    // Physical sub-types
    req.vis.phys_fill = extract_int_or(msg, "phys_fill", 1);
    req.vis.phys_endcap = extract_int_or(msg, "phys_endcap", 1);
    req.vis.phys_welltap = extract_int_or(msg, "phys_welltap", 1);
    req.vis.phys_tie = extract_int_or(msg, "phys_tie", 1);
    req.vis.phys_antenna = extract_int_or(msg, "phys_antenna", 1);
    req.vis.phys_cover = extract_int_or(msg, "phys_cover", 1);
    req.vis.phys_bump = extract_int_or(msg, "phys_bump", 1);
    req.vis.phys_other = extract_int_or(msg, "phys_other", 1);
    // Std cell sub-types
    req.vis.std_bufinv = extract_int_or(msg, "std_bufinv", 1);
    req.vis.std_bufinv_timing = extract_int_or(msg, "std_bufinv_timing", 1);
    req.vis.std_clock_bufinv = extract_int_or(msg, "std_clock_bufinv", 1);
    req.vis.std_clock_gate = extract_int_or(msg, "std_clock_gate", 1);
    req.vis.std_level_shift = extract_int_or(msg, "std_level_shift", 1);
    req.vis.std_sequential = extract_int_or(msg, "std_sequential", 1);
    req.vis.std_combinational = extract_int_or(msg, "std_combinational", 1);
    // Net sub-types
    req.vis.net_signal = extract_int_or(msg, "net_signal", 1);
    req.vis.net_power = extract_int_or(msg, "net_power", 1);
    req.vis.net_ground = extract_int_or(msg, "net_ground", 1);
    req.vis.net_clock = extract_int_or(msg, "net_clock", 1);
    req.vis.net_reset = extract_int_or(msg, "net_reset", 1);
    req.vis.net_tieoff = extract_int_or(msg, "net_tieoff", 1);
    req.vis.net_scan = extract_int_or(msg, "net_scan", 1);
    req.vis.net_analog = extract_int_or(msg, "net_analog", 1);
    // Shapes
    req.vis.routing = extract_int_or(msg, "routing", 1);
    req.vis.special_nets = extract_int_or(msg, "special_nets", 1);
    req.vis.pins = extract_int_or(msg, "pins", 1);
    req.vis.blockages = extract_int_or(msg, "blockages", 1);
    // Debug
    req.vis.debug = extract_int_or(msg, "debug", 0);
  } else if (type_str == "bounds") {
    req.type = WsRequest::BOUNDS;
  } else if (type_str == "layers") {
    req.type = WsRequest::LAYERS;
  } else if (type_str == "info") {
    req.type = WsRequest::INFO;
  } else if (type_str == "select") {
    req.type = WsRequest::SELECT;
    req.select_x = extract_int(msg, "dbu_x");
    req.select_y = extract_int(msg, "dbu_y");
    req.select_zoom = extract_int_or(msg, "zoom", 0);
    // Visibility flags for filtering selectable instances
    req.vis.stdcells = extract_int_or(msg, "stdcells", 1);
    req.vis.macros = extract_int_or(msg, "macros", 1);
    req.vis.pad_input = extract_int_or(msg, "pad_input", 1);
    req.vis.pad_output = extract_int_or(msg, "pad_output", 1);
    req.vis.pad_inout = extract_int_or(msg, "pad_inout", 1);
    req.vis.pad_power = extract_int_or(msg, "pad_power", 1);
    req.vis.pad_spacer = extract_int_or(msg, "pad_spacer", 1);
    req.vis.pad_areaio = extract_int_or(msg, "pad_areaio", 1);
    req.vis.pad_other = extract_int_or(msg, "pad_other", 1);
    req.vis.phys_fill = extract_int_or(msg, "phys_fill", 1);
    req.vis.phys_endcap = extract_int_or(msg, "phys_endcap", 1);
    req.vis.phys_welltap = extract_int_or(msg, "phys_welltap", 1);
    req.vis.phys_tie = extract_int_or(msg, "phys_tie", 1);
    req.vis.phys_antenna = extract_int_or(msg, "phys_antenna", 1);
    req.vis.phys_cover = extract_int_or(msg, "phys_cover", 1);
    req.vis.phys_bump = extract_int_or(msg, "phys_bump", 1);
    req.vis.phys_other = extract_int_or(msg, "phys_other", 1);
    req.vis.std_bufinv = extract_int_or(msg, "std_bufinv", 1);
    req.vis.std_bufinv_timing = extract_int_or(msg, "std_bufinv_timing", 1);
    req.vis.std_clock_bufinv = extract_int_or(msg, "std_clock_bufinv", 1);
    req.vis.std_clock_gate = extract_int_or(msg, "std_clock_gate", 1);
    req.vis.std_level_shift = extract_int_or(msg, "std_level_shift", 1);
    req.vis.std_sequential = extract_int_or(msg, "std_sequential", 1);
    req.vis.std_combinational = extract_int_or(msg, "std_combinational", 1);
  } else {
    req.type = WsRequest::UNKNOWN;
  }
  return req;
}

static WsResponse dispatch_request(
    const WsRequest& req,
    const std::shared_ptr<TileGenerator>& gen,
    const std::set<odb::dbInst*>& selected = {})
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
      std::stringstream ss;
      ss << "{\"layers\": [";
      bool first = true;
      for (const auto& name : gen->getRoutingLayers()) {
        if (!first) {
          ss << ", ";
        }
        ss << "\"" << name << "\"";
        first = false;
      }
      ss << "]}";
      const std::string json = ss.str();
      resp.payload.assign(json.begin(), json.end());
      break;
    }
    case WsRequest::TILE: {
      resp.type = 1;  // PNG
      resp.payload = gen->generateTile(
          req.layer, req.z, req.x, req.y, req.vis, selected);
      break;
    }
    case WsRequest::INFO: {
      resp.type = 0;  // JSON
      const std::string json = gen->hasSta() ? "{\"has_liberty\": true}"
                                             : "{\"has_liberty\": false}";
      resp.payload.assign(json.begin(), json.end());
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

static std::string content_type_for(const std::string& path)
{
  auto ext = std::filesystem::path(path).extension().string();
  if (ext == ".html") {
    return "text/html";
  }
  if (ext == ".js") {
    return "application/javascript";
  }
  if (ext == ".css") {
    return "text/css";
  }
  if (ext == ".png") {
    return "image/png";
  }
  if (ext == ".json") {
    return "application/json";
  }
  return "application/octet-stream";
}

http::response<http::string_body> handle_request(
    http::request<http::string_body>&& req,
    std::shared_ptr<TileGenerator> generator,
    const std::string& doc_root)
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
    res.body() = std::string(ws_resp.payload.begin(), ws_resp.payload.end());
    res.set(http::field::cache_control, "public, max-age=604800");
  } else if (req.method() == http::verb::get && !doc_root.empty()) {
    // Serve static files from doc_root
    std::string file_path = target_path;
    if (file_path == "/") {
      file_path = "/index.html";
    }
    // Reject paths with ".." to prevent directory traversal
    if (file_path.find("..") == std::string::npos) {
      auto full_path = std::filesystem::path(doc_root) / file_path.substr(1);
      std::ifstream file(full_path, std::ios::binary);
      if (file) {
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        res.set(http::field::content_type, content_type_for(file_path));
        res.body() = std::move(content);
      } else {
        res.result(http::status::not_found);
        res.body() = "File not found.";
      }
    } else {
      res.result(http::status::bad_request);
      res.body() = "Invalid path.";
    }
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

  // Per-session selection state
  std::mutex selection_mutex_;
  std::set<odb::dbInst*> selected_insts_;

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

    ws_.async_accept(req, [self = shared_from_this()](beast::error_code ec) {
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

    auto self = shared_from_this();
    auto gen = generator_;

    if (req.type == WsRequest::SELECT) {
      // Handle selection on the thread pool — single selectAt call
      net::post(ws_.get_executor(), [self, gen, req]() {
        WsResponse resp;
        resp.id = req.id;
        try {
          auto results = gen->selectAt(
              req.select_x, req.select_y, req.select_zoom, req.vis);

          // Update session selection state
          std::set<odb::dbInst*> new_selection;
          for (const auto& r : results) {
            new_selection.insert(r.inst);
          }
          {
            std::lock_guard<std::mutex> lock(self->selection_mutex_);
            self->selected_insts_ = std::move(new_selection);
          }

          // Build JSON response with selection and properties
          resp.type = 0;  // JSON
          std::stringstream ss;
          ss << "{\"selected\": [";
          bool first = true;
          for (const auto& r : results) {
            if (!first) {
              ss << ", ";
            }
            ss << "{\"name\": \"" << json_escape(r.name)
               << "\", \"master\": \"" << json_escape(r.master)
               << "\", \"bbox\": [" << r.bbox.xMin() << ", "
               << r.bbox.yMin() << ", " << r.bbox.xMax() << ", "
               << r.bbox.yMax() << "]}";
            first = false;
          }
          ss << "]";

          // Add properties for the first selected instance
          if (!results.empty()) {
            auto* registry = gui::DescriptorRegistry::instance();
            gui::Selected sel
                = registry->makeSelected(std::any(results[0].inst));
            if (sel) {
              auto props = sel.getProperties();
              ss << ", \"properties\": [";
              bool pfirst = true;
              for (const auto& prop : props) {
                if (!pfirst) {
                  ss << ", ";
                }
                serializeProperty(ss, prop);
                pfirst = false;
              }
              ss << "]";
            }
          }

          ss << "}";
          const std::string json = ss.str();
          resp.payload.assign(json.begin(), json.end());
        } catch (const std::exception& e) {
          resp.type = 2;
          std::string err = std::string("server error: ") + e.what();
          resp.payload.assign(err.begin(), err.end());
        }
        self->queue_response(resp);
      });
    } else {
      // Capture current selection for tile rendering
      std::set<odb::dbInst*> selected;
      {
        std::lock_guard<std::mutex> lock(selection_mutex_);
        selected = selected_insts_;
      }

      // Dispatch tile generation to the thread pool (not to the strand).
      // This lets multiple tiles render concurrently across all 32 threads.
      net::post(
          ws_.get_executor(), [self, gen, req, selected = std::move(selected)]() {
            WsResponse resp;
            try {
              resp = dispatch_request(req, gen, selected);
            } catch (const std::exception& e) {
              resp.id = req.id;
              resp.type = 2;  // error
              std::string err = std::string("server error: ") + e.what();
              resp.payload.assign(err.begin(), err.end());
              std::cerr << "dispatch error for request " << req.id << ": "
                        << e.what() << "\n";
            }
            self->queue_response(resp);
          });
    }

    // Immediately start reading the next request
    do_read();
  }

  void queue_response(const WsResponse& resp)
  {
    std::vector<unsigned char> frame = serialize_response(resp);

    // Post to the strand to serialize write queue access
    net::post(strand_,
              [self = shared_from_this(), frame = std::move(frame)]() mutable {
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
  std::string doc_root_;

 public:
  session(tcp::socket&& socket,
          std::shared_ptr<TileGenerator> generator,
          std::string doc_root)
      : stream_(std::move(socket)),
        generator_(generator),
        doc_root_(std::move(doc_root))
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
        handle_request(std::move(req_), generator_, doc_root_));
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
  std::string doc_root_;

 public:
  detect_session(tcp::socket&& socket,
                 std::shared_ptr<TileGenerator> generator,
                 std::string doc_root)
      : stream_(std::move(socket)),
        generator_(std::move(generator)),
        doc_root_(std::move(doc_root))
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
      auto ws
          = std::make_shared<ws_session>(stream_.release_socket(), generator_);
      ws->run(std::move(req_));
    } else {
      // Regular HTTP - hand off to session with already-read request
      auto s = std::make_shared<session>(
          stream_.release_socket(), generator_, doc_root_);
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
  std::string doc_root_;

 public:
  listener(net::io_context& ioc,
           tcp::endpoint endpoint,
           std::shared_ptr<TileGenerator> generator,
           std::string doc_root)
      : ioc_(ioc),
        acceptor_(ioc),
        generator_(generator),
        doc_root_(std::move(doc_root))
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
      std::make_shared<detect_session>(std::move(socket), generator_, doc_root_)
          ->run();
    }
    do_accept();
  }
};

WebServer::WebServer(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger)
    : db_(db), sta_(sta), logger_(logger)
{
}

WebServer::~WebServer() = default;

void WebServer::serve(const std::string& doc_root)
{
  try {
    generator_ = std::make_shared<TileGenerator>(db_, sta_, logger_);

    auto const address = net::ip::make_address("127.0.0.1");
    unsigned short const port = 8080;
    int const num_threads = 32;

    if (!doc_root.empty()) {
      logger_->info(utl::WEB, 4, "Serving static files from {}", doc_root);
    }
    logger_->info(utl::WEB,
                  1,
                  "Server starting on http://:{} with {} threads...",
                  port,
                  num_threads);

    net::io_context ioc{num_threads};

    std::make_shared<listener>(
        ioc, tcp::endpoint{address, port}, generator_, doc_root)
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
