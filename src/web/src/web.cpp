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
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "search.h"
#include "tile_generator.h"
#include "timing_report.h"
#include "tcl.h"
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

std::vector<std::string> TileGenerator::getLayers()
{
  std::vector<std::string> layers;
  odb::dbTech* tech = db_->getTech();
  for (odb::dbTechLayer* layer : tech->getLayers()) {
    if (layer->getRoutingLevel() > 0
        || layer->getType() == odb::dbTechLayerType::CUT) {
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

odb::dbBlock* TileGenerator::getBlock() const
{
  return db_->getChip()->getBlock();
}

std::vector<unsigned char> TileGenerator::generateTile(
    const std::string& layer,
    const int z,
    const int x,
    int y,
    const TileVisibility& vis,
    const std::vector<odb::Rect>& highlight_rects,
    const std::vector<ColoredRect>& colored_rects,
    const std::vector<FlightLine>& flight_lines)
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
    const auto all_layers = getLayers();
    auto it = std::find(all_layers.begin(), all_layers.end(), layer);
    if (it != all_layers.end()) {
      layer_index = std::distance(all_layers.begin(), it);
    }
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

    // Draw special net vias — decompose into individual cut boxes
    if (!instances_only && tech_layer && vis.special_nets) {
      for (const auto& shape : search_->searchSNetViaShapes(
               block, tech_layer, dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max)) {
        if (!vis.isNetVisible(std::get<1>(shape))) {
          continue;
        }
        odb::dbSBox* sbox = std::get<0>(shape);
        std::vector<odb::dbBox*> via_boxes;
        if (auto tech_via = sbox->getTechVia()) {
          via_boxes.assign(tech_via->getBoxes().begin(),
                           tech_via->getBoxes().end());
        } else if (auto block_via = sbox->getBlockVia()) {
          via_boxes.assign(block_via->getBoxes().begin(),
                           block_via->getBoxes().end());
        }
        const odb::Point origin(
            (sbox->xMin() + sbox->xMax()) / 2,
            (sbox->yMin() + sbox->yMax()) / 2);
        for (odb::dbBox* vbox : via_boxes) {
          if (vbox->getTechLayer() != tech_layer) {
            continue;
          }
          odb::Rect box = vbox->getBox();
          box.moveDelta(origin.x(), origin.y());
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

    if (!highlight_rects.empty()) {
      drawHighlight(image_buffer, highlight_rects, dbu_tile, scale);
    }
    if (!colored_rects.empty()) {
      drawColoredHighlight(image_buffer, colored_rects, layer, dbu_tile, scale);
    }
    if (!flight_lines.empty()) {
      drawFlightLines(image_buffer, flight_lines, dbu_tile, scale);
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

// ShapeCollector — a gui::Painter that collects rectangles from
// descriptor->highlight() calls for use in tile rendering.
class ShapeCollector : public gui::Painter
{
 public:
  ShapeCollector() : Painter(nullptr, odb::Rect(), 1.0) {}

  std::vector<odb::Rect> rects;

  void drawRect(const odb::Rect& rect, int, int) override
  {
    rects.push_back(rect);
  }
  void drawPolygon(const odb::Polygon& polygon) override
  {
    rects.push_back(polygon.getEnclosingRect());
  }
  void drawOctagon(const odb::Oct& oct) override
  {
    rects.push_back(oct.getEnclosingRect());
  }

  // No-ops
  Color getPenColor() override { return {}; }
  void setPen(odb::dbTechLayer*, bool) override {}
  void setPen(const Color&, bool, int) override {}
  void setPenWidth(int) override {}
  void setBrush(odb::dbTechLayer*, int) override {}
  void setBrush(const Color&, const Brush&) override {}
  void setFont(const Font&) override {}
  void saveState() override {}
  void restoreState() override {}
  void drawLine(const odb::Point&, const odb::Point&) override {}
  void drawCircle(int, int, int) override {}
  void drawX(int, int, int) override {}
  void drawPolygon(const std::vector<odb::Point>&) override {}
  void drawString(int, int, Anchor, const std::string&, bool) override {}
  odb::Rect stringBoundaries(int, int, Anchor, const std::string&) override
  {
    return {};
  }
  void drawRuler(int, int, int, int, bool, const std::string&) override {}
};

void TileGenerator::drawHighlight(std::vector<unsigned char>& image,
                                  const std::vector<odb::Rect>& rects,
                                  const odb::Rect& dbu_tile,
                                  const double scale)
{
  const Color fill{255, 255, 0, 30};
  const Color border{255, 255, 0, 255};

  for (const odb::Rect& rect : rects) {
    if (!dbu_tile.overlaps(rect)) {
      continue;
    }
    const odb::Rect overlap = rect.intersect(dbu_tile);
    const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

    // Semi-transparent yellow fill
    for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        blendPixel(image, ix, 255 - iy, fill);
      }
    }

    // Solid yellow border (only where edge is within the tile)
    const odb::Rect full_draw = toPixels(scale, rect, dbu_tile);
    if (full_draw.xMin() >= 0 && full_draw.xMin() < kTileSizeInPixel) {
      for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
        setPixel(image, full_draw.xMin(), 255 - iy, border);
      }
    }
    const int rx = std::min(full_draw.xMax() - 1, kTileSizeInPixel - 1);
    if (rx >= 0 && full_draw.xMax() > 0) {
      for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
        setPixel(image, rx, 255 - iy, border);
      }
    }
    if (full_draw.yMin() >= 0 && full_draw.yMin() < kTileSizeInPixel) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        setPixel(image, ix, 255 - full_draw.yMin(), border);
      }
    }
    const int ty = std::min(full_draw.yMax() - 1, kTileSizeInPixel - 1);
    if (ty >= 0 && full_draw.yMax() > 0) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        setPixel(image, ix, 255 - ty, border);
      }
    }
  }
}

void TileGenerator::drawColoredHighlight(
    std::vector<unsigned char>& image,
    const std::vector<ColoredRect>& rects,
    const std::string& current_layer,
    const odb::Rect& dbu_tile,
    const double scale)
{
  const bool is_instances_layer = (current_layer == "_instances");
  for (const auto& cr : rects) {
    // Layer filtering: draw on _instances (overview) or matching layer
    if (!is_instances_layer && !cr.layer.empty()
        && cr.layer != current_layer) {
      continue;
    }
    if (!dbu_tile.overlaps(cr.rect)) {
      continue;
    }
    const odb::Rect overlap = cr.rect.intersect(dbu_tile);
    const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

    // Draw a fixed-width centerline through the shape (cosmetic pen style,
    // matching the GUI's 2px cosmetic pen approach from dbDescriptors.cpp).
    // This ensures consistent visibility regardless of zoom level.
    const int cx = (draw.xMin() + draw.xMax()) / 2;
    const int cy = (draw.yMin() + draw.yMax()) / 2;

    Color line_color = cr.color;
    line_color.a = 255;

    if (draw.dx() >= draw.dy()) {
      // Horizontal shape: draw horizontal centerline
      drawLine(image, draw.xMin(), 255 - cy, draw.xMax(), 255 - cy, line_color);
    } else {
      // Vertical shape: draw vertical centerline
      drawLine(image, cx, 255 - draw.yMin(), cx, 255 - draw.yMax(), line_color);
    }
  }
}

/* static */
void TileGenerator::drawLine(std::vector<unsigned char>& image,
                              int x0,
                              int y0,
                              int x1,
                              int y1,
                              const Color& c)
{
  // Bresenham's line algorithm
  int dx = std::abs(x1 - x0);
  int dy = std::abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = dx - dy;

  while (true) {
    // Draw 3px wide
    for (int dy2 = -1; dy2 <= 1; dy2++) {
      for (int dx2 = -1; dx2 <= 1; dx2++) {
        blendPixel(image, x0 + dx2, y0 + dy2, c);
      }
    }

    if (x0 == x1 && y0 == y1) {
      break;
    }
    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void TileGenerator::drawFlightLines(std::vector<unsigned char>& image,
                                     const std::vector<FlightLine>& lines,
                                     const odb::Rect& dbu_tile,
                                     const double scale)
{
  for (const auto& fl : lines) {
    // Convert DBU to pixel coordinates
    int px0
        = static_cast<int>((fl.p1.x() - dbu_tile.xMin()) * scale);
    int py0 = 255
              - static_cast<int>((fl.p1.y() - dbu_tile.yMin()) * scale);
    int px1
        = static_cast<int>((fl.p2.x() - dbu_tile.xMin()) * scale);
    int py1 = 255
              - static_cast<int>((fl.p2.y() - dbu_tile.yMin()) * scale);

    // Rough bounding-box check: skip if line can't cross this tile
    int lx0 = std::min(px0, px1), lx1 = std::max(px0, px1);
    int ly0 = std::min(py0, py1), ly1 = std::max(py0, py1);
    if (lx1 < 0 || lx0 >= kTileSizeInPixel || ly1 < 0
        || ly0 >= kTileSizeInPixel) {
      continue;
    }

    Color c = fl.color;
    c.a = 220;
    drawLine(image, px0, py0, px1, py1, c);
  }
}

//------------------------------------------------------------------------------
// Timing path highlight shape collection
//------------------------------------------------------------------------------

static std::pair<odb::dbITerm*, odb::dbBTerm*> resolvePin(
    odb::dbBlock* block,
    const std::string& pin_name)
{
  odb::dbITerm* iterm = block->findITerm(pin_name.c_str());
  if (iterm) {
    return {iterm, nullptr};
  }
  return {nullptr, block->findBTerm(pin_name.c_str())};
}

static odb::Point getPinLocation(odb::dbITerm* iterm, odb::dbBTerm* bterm)
{
  if (iterm) {
    int x, y;
    if (iterm->getAvgXY(&x, &y)) {
      return {x, y};
    }
    // Fallback to instance center
    odb::Rect bbox = iterm->getInst()->getBBox()->getBox();
    return {(bbox.xMin() + bbox.xMax()) / 2, (bbox.yMin() + bbox.yMax()) / 2};
  }
  if (bterm) {
    for (odb::dbBPin* bpin : bterm->getBPins()) {
      odb::Rect r = bpin->getBBox();
      return {(r.xMin() + r.xMax()) / 2, (r.yMin() + r.yMax()) / 2};
    }
  }
  return {0, 0};
}

static void collectNetShapes(odb::dbNet* net,
                              odb::dbITerm* drv_iterm,
                              odb::dbBTerm* drv_bterm,
                              odb::dbITerm* snk_iterm,
                              odb::dbBTerm* snk_bterm,
                              const Color& color,
                              std::vector<ColoredRect>& rects,
                              std::vector<FlightLine>& lines)
{
  odb::dbWire* wire = net->getWire();
  if (wire) {
    odb::dbWireShapeItr itr;
    odb::dbShape shape;
    for (itr.begin(wire); itr.next(shape);) {
      if (shape.isVia()) {
        std::vector<odb::dbShape> via_boxes;
        odb::dbShape::getViaBoxes(shape, via_boxes);
        for (const auto& vbox : via_boxes) {
          odb::dbTechLayer* layer = vbox.getTechLayer();
          rects.push_back(
              {vbox.getBox(), color, layer ? layer->getName() : ""});
        }
      } else {
        odb::dbTechLayer* layer = shape.getTechLayer();
        rects.push_back(
            {shape.getBox(), color, layer ? layer->getName() : ""});
      }
    }
  } else {
    // Unrouted: draw flight line between driver and sink
    odb::Point p1 = getPinLocation(drv_iterm, drv_bterm);
    odb::Point p2 = getPinLocation(snk_iterm, snk_bterm);
    lines.push_back({p1, p2, color});
  }
}

static void collectTimingPathShapes(odb::dbBlock* block,
                                     const TimingPathSummary& path,
                                     std::vector<ColoredRect>& rects,
                                     std::vector<FlightLine>& lines)
{
  static const Color kLaunchClkColor{0, 255, 255, 180};   // Cyan (match GUI)
  static const Color kSignalColor{255, 0, 0, 180};         // Red (match GUI)
  static const Color kCaptureClkColor{0, 255, 0, 180};     // Green (match GUI)

  // Track nets already collected to avoid duplicates
  std::set<odb::dbNet*> seen_nets;

  auto processNodes = [&](const std::vector<TimingNode>& nodes,
                          const Color& clk_color,
                          const Color& data_color) {
    for (size_t i = 0; i + 1 < nodes.size(); i++) {
      auto [a_iterm, a_bterm] = resolvePin(block, nodes[i].pin_name);
      auto [b_iterm, b_bterm] = resolvePin(block, nodes[i + 1].pin_name);

      odb::dbNet* net_a
          = a_iterm ? a_iterm->getNet() : (a_bterm ? a_bterm->getNet() : nullptr);
      odb::dbNet* net_b
          = b_iterm ? b_iterm->getNet() : (b_bterm ? b_bterm->getNet() : nullptr);

      // Only draw when consecutive pins are on the same net (wire segment)
      if (net_a && net_a == net_b && seen_nets.insert(net_a).second) {
        const Color& c = nodes[i].is_clock ? clk_color : data_color;
        collectNetShapes(
            net_a, a_iterm, a_bterm, b_iterm, b_bterm, c, rects, lines);
      }
    }
  };

  // data_nodes: launch clock (is_clock=true) then signal portion
  processNodes(path.data_nodes, kLaunchClkColor, kSignalColor);

  // capture_nodes: capture clock path
  processNodes(path.capture_nodes, kCaptureClkColor, kCaptureClkColor);
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
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          // Escape other control characters as \u00XX
          char buf[8];
          snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
          out += buf;
        } else {
          out += c;
        }
        break;
    }
  }
  return out;
}

// Store a Selected in the clickables vector and return its index.
static int storeSelectable(std::vector<gui::Selected>& selectables,
                           const gui::Selected& sel)
{
  int id = static_cast<int>(selectables.size());
  selectables.push_back(sel);
  return id;
}

// Serialize a std::any value that might be a Selected.  If it is,
// write "value":"<name>", "select_id":<N> so the client can navigate.
// Otherwise just write "value":"<string>".
static void serializeAnyValue(std::stringstream& ss,
                              const char* field,
                              const std::any& value,
                              std::vector<gui::Selected>& selectables,
                              bool short_name = false)
{
  if (auto* sel = std::any_cast<gui::Selected>(&value)) {
    if (*sel) {
      std::string name = short_name ? sel->getShortName() : sel->getName();
      int id = storeSelectable(selectables, *sel);
      ss << "\"" << field << "\": \"" << json_escape(name) << "\", \""
         << field << "_select_id\": " << id;
      return;
    }
  }
  std::string str = gui::Descriptor::Property::toString(value);
  ss << "\"" << field << "\": \"" << json_escape(str) << "\"";
}

// Serialize a Descriptor::Property value to a JSON fragment.
// Leaf values: {"name":"...", "value":"..."}
// PropertyList / SelectionSet: {"name":"...", "children":[...]}
// Selected values get an additional select_id for click navigation.
static void serializeProperty(std::stringstream& ss,
                              const gui::Descriptor::Property& prop,
                              std::vector<gui::Selected>& selectables)
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
      ss << "{";
      serializeAnyValue(ss, "name", key, selectables, /*short_name=*/true);
      ss << ", ";
      serializeAnyValue(ss, "value", val, selectables);
      ss << "}";
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
      int id = storeSelectable(selectables, sel);
      ss << "{\"name\": \"" << json_escape(sel.getName())
         << "\", \"name_select_id\": " << id << "}";
      first = false;
    }
    ss << "]";
  } else if (auto* sel = std::any_cast<gui::Selected>(&prop.value)) {
    // Single Selected leaf value (e.g., "Master" → dbMaster*)
    if (*sel) {
      int id = storeSelectable(selectables, *sel);
      ss << ", \"value\": \"" << json_escape(sel->getName())
         << "\", \"value_select_id\": " << id;
    }
  } else {
    // Plain leaf value — convert to string.
    std::string val_str = prop.toString();
    ss << ", \"value\": \"" << json_escape(val_str) << "\"";
  }

  ss << "}";
}

//------------------------------------------------------------------------------
// TclEvaluator — thread-safe Tcl command evaluation with output capture.
// Uses Logger::redirectStringBegin/End to capture all output (puts, logger
// messages) without echoing to the server console.
//------------------------------------------------------------------------------

struct TclEvaluator
{
  Tcl_Interp* interp;
  utl::Logger* logger;
  std::mutex mutex;

  struct Result
  {
    std::string output;
    std::string result;
    bool is_error;
  };

  TclEvaluator(Tcl_Interp* interp, utl::Logger* logger)
      : interp(interp), logger(logger)
  {
  }

  Result eval(const std::string& cmd)
  {
    std::lock_guard<std::mutex> lock(mutex);
    logger->redirectStringBegin();
    int rc = Tcl_Eval(interp, cmd.c_str());
    Result r;
    r.output = logger->redirectStringEnd();
    r.result = Tcl_GetStringResult(interp);
    r.is_error = (rc != TCL_OK);
    return r;
  }
};

//------------------------------------------------------------------------------

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
    INSPECT,
    HOVER,
    TCL_EVAL,
    TIMING_REPORT,
    TIMING_HIGHLIGHT,
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

  // INSPECT / HOVER fields
  int select_id = -1;

  // TCL_EVAL fields
  std::string tcl_cmd;

  // TIMING_REPORT fields
  bool timing_is_setup = true;
  int timing_max_paths = 100;

  // TIMING_HIGHLIGHT fields
  int timing_path_index = -1;  // -1 = clear
  bool timing_highlight_setup = true;
  std::string timing_pin_name;  // optional: highlight this pin's net in yellow

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
  // Find closing quote, skipping escaped quotes
  std::string result;
  for (size_t i = quote_start + 1; i < json.size(); i++) {
    if (json[i] == '\\' && i + 1 < json.size()) {
      switch (json[i + 1]) {
        case '"': result += '"'; break;
        case '\\': result += '\\'; break;
        case 'n': result += '\n'; break;
        case 'r': result += '\r'; break;
        case 't': result += '\t'; break;
        default: result += json[i + 1]; break;
      }
      i++;  // skip the escaped char
    } else if (json[i] == '"') {
      break;  // closing quote
    } else {
      result += json[i];
    }
  }
  return result;
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
  } else if (type_str == "inspect") {
    req.type = WsRequest::INSPECT;
    req.select_id = extract_int(msg, "select_id");
  } else if (type_str == "hover") {
    req.type = WsRequest::HOVER;
    req.select_id = extract_int(msg, "select_id");
  } else if (type_str == "tcl_eval") {
    req.type = WsRequest::TCL_EVAL;
    req.tcl_cmd = extract_string(msg, "cmd");
  } else if (type_str == "timing_report") {
    req.type = WsRequest::TIMING_REPORT;
    req.timing_is_setup = extract_int_or(msg, "is_setup", 1);
    req.timing_max_paths = extract_int_or(msg, "max_paths", 100);
  } else if (type_str == "timing_highlight") {
    req.type = WsRequest::TIMING_HIGHLIGHT;
    req.timing_path_index = extract_int_or(msg, "path_index", -1);
    req.timing_highlight_setup = extract_int_or(msg, "is_setup", 1);
    req.timing_pin_name = extract_string(msg, "pin_name");
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
    const std::vector<odb::Rect>& highlight_rects = {},
    const std::vector<ColoredRect>& colored_rects = {},
    const std::vector<FlightLine>& flight_lines = {})
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
      for (const auto& name : gen->getLayers()) {
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
          req.layer, req.z, req.x, req.y, req.vis, highlight_rects,
          colored_rects, flight_lines);
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
  std::shared_ptr<TclEvaluator> tcl_eval_;
  std::shared_ptr<TimingReport> timing_report_;

  // Per-session highlight shapes (collected via descriptor->highlight())
  std::mutex selection_mutex_;
  std::vector<odb::Rect> highlight_rects_;
  std::vector<ColoredRect> timing_rects_;
  std::vector<FlightLine> timing_lines_;

  // Clickable objects from the last property response.
  // Index in this vector is the select_id sent to the client.
  std::mutex selectables_mutex_;
  std::vector<gui::Selected> selectables_;

  // Write serialization: strand + queue ensures one async_write at a time
  net::strand<net::any_io_executor> strand_;
  std::deque<std::vector<unsigned char>> write_queue_;
  bool writing_ = false;

 public:
  ws_session(tcp::socket&& socket,
             std::shared_ptr<TileGenerator> generator,
             std::shared_ptr<TclEvaluator> tcl_eval,
             std::shared_ptr<TimingReport> timing_report)
      : ws_(std::move(socket)),
        generator_(std::move(generator)),
        tcl_eval_(std::move(tcl_eval)),
        timing_report_(std::move(timing_report)),
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

          // Collect highlight shapes from the first selected instance
          {
            std::lock_guard<std::mutex> lock(self->selection_mutex_);
            self->highlight_rects_.clear();
            self->timing_rects_.clear();
            self->timing_lines_.clear();
            if (!results.empty()) {
              auto* registry = gui::DescriptorRegistry::instance();
              gui::Selected sel
                  = registry->makeSelected(std::any(results[0].inst));
              if (sel) {
                ShapeCollector collector;
                sel.highlight(collector);
                self->highlight_rects_ = std::move(collector.rects);
              }
            }
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
          std::vector<gui::Selected> new_selectables;
          if (!results.empty()) {
            // Top-level bbox for the toolbar zoom button
            const auto& r0 = results[0];
            ss << ", \"bbox\": [" << r0.bbox.xMin() << ", "
               << r0.bbox.yMin() << ", " << r0.bbox.xMax() << ", "
               << r0.bbox.yMax() << "]";

            auto* registry = gui::DescriptorRegistry::instance();
            gui::Selected sel
                = registry->makeSelected(std::any(r0.inst));
            if (sel) {
              auto props = sel.getProperties();
              ss << ", \"properties\": [";
              bool pfirst = true;
              for (const auto& prop : props) {
                if (!pfirst) {
                  ss << ", ";
                }
                serializeProperty(ss, prop, new_selectables);
                pfirst = false;
              }
              ss << "]";
            }
          }
          {
            std::lock_guard<std::mutex> lock(self->selectables_mutex_);
            self->selectables_ = std::move(new_selectables);
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
    } else if (req.type == WsRequest::INSPECT) {
      // Navigate to a previously-serialized Selected object by its ID
      net::post(ws_.get_executor(), [self, req]() {
        WsResponse resp;
        resp.id = req.id;
        try {
          gui::Selected sel;
          {
            std::lock_guard<std::mutex> lock(self->selectables_mutex_);
            if (req.select_id >= 0
                && req.select_id
                       < static_cast<int>(self->selectables_.size())) {
              sel = self->selectables_[req.select_id];
            }
          }

          // Collect highlight shapes from the inspected object
          {
            std::lock_guard<std::mutex> lock(self->selection_mutex_);
            self->highlight_rects_.clear();
            self->timing_rects_.clear();
            self->timing_lines_.clear();
            if (sel) {
              ShapeCollector collector;
              sel.highlight(collector);
              self->highlight_rects_ = std::move(collector.rects);
            }
          }

          resp.type = 0;
          std::stringstream ss;
          if (sel) {
            auto props = sel.getProperties();
            std::vector<gui::Selected> new_selectables;
            ss << "{\"name\": \"" << json_escape(sel.getName())
               << "\", \"type\": \"" << json_escape(sel.getTypeName())
               << "\", \"properties\": [";
            bool first = true;
            for (const auto& prop : props) {
              if (!first) {
                ss << ", ";
              }
              serializeProperty(ss, prop, new_selectables);
              first = false;
            }
            ss << "]";
            // Include bbox for highlight
            odb::Rect bbox;
            if (sel.getBBox(bbox)) {
              ss << ", \"bbox\": [" << bbox.xMin() << ", " << bbox.yMin()
                 << ", " << bbox.xMax() << ", " << bbox.yMax() << "]";
            }
            ss << "}";
            {
              std::lock_guard<std::mutex> lock(self->selectables_mutex_);
              self->selectables_ = std::move(new_selectables);
            }
          } else {
            ss << "{\"error\": \"invalid select_id\"}";
          }
          const std::string json = ss.str();
          resp.payload.assign(json.begin(), json.end());
        } catch (const std::exception& e) {
          resp.type = 2;
          std::string err = std::string("server error: ") + e.what();
          resp.payload.assign(err.begin(), err.end());
        }
        self->queue_response(resp);
      });
    } else if (req.type == WsRequest::HOVER) {
      // Return just the bbox for a selectable (no state changes)
      net::post(ws_.get_executor(), [self, req]() {
        WsResponse resp;
        resp.id = req.id;
        try {
          gui::Selected sel;
          {
            std::lock_guard<std::mutex> lock(self->selectables_mutex_);
            if (req.select_id >= 0
                && req.select_id
                       < static_cast<int>(self->selectables_.size())) {
              sel = self->selectables_[req.select_id];
            }
          }

          resp.type = 0;
          std::stringstream ss;
          if (sel) {
            ShapeCollector collector;
            sel.highlight(collector);
            if (!collector.rects.empty()) {
              ss << "{\"rects\": [";
              bool first = true;
              for (const auto& r : collector.rects) {
                if (!first) {
                  ss << ", ";
                }
                ss << "[" << r.xMin() << ", " << r.yMin() << ", "
                   << r.xMax() << ", " << r.yMax() << "]";
                first = false;
              }
              ss << "]}";
            } else {
              // Fall back to bbox if no shapes
              odb::Rect bbox;
              if (sel.getBBox(bbox)) {
                ss << "{\"rects\": [[" << bbox.xMin() << ", " << bbox.yMin()
                   << ", " << bbox.xMax() << ", " << bbox.yMax() << "]]}";
              } else {
                ss << "{\"error\": \"no bbox\"}";
              }
            }
          } else {
            ss << "{\"error\": \"invalid select_id\"}";
          }
          const std::string json = ss.str();
          resp.payload.assign(json.begin(), json.end());
        } catch (const std::exception& e) {
          resp.type = 2;
          std::string err = std::string("server error: ") + e.what();
          resp.payload.assign(err.begin(), err.end());
        }
        self->queue_response(resp);
      });
    } else if (req.type == WsRequest::TCL_EVAL) {
      // Evaluate a Tcl command and return the result + logger output
      net::post(ws_.get_executor(), [self, req]() {
        WsResponse resp;
        resp.id = req.id;
        resp.type = 0;
        try {
          auto result = self->tcl_eval_->eval(req.tcl_cmd);
          std::stringstream ss;
          ss << "{\"output\": \"" << json_escape(result.output)
             << "\", \"result\": \"" << json_escape(result.result)
             << "\", \"is_error\": "
             << (result.is_error ? "true" : "false") << "}";
          const std::string json = ss.str();
          resp.payload.assign(json.begin(), json.end());
        } catch (const std::exception& e) {
          resp.type = 2;
          std::string err = std::string("server error: ") + e.what();
          resp.payload.assign(err.begin(), err.end());
        }
        self->queue_response(resp);
      });
    } else if (req.type == WsRequest::TIMING_REPORT) {
      auto tr = timing_report_;
      auto tcl = tcl_eval_;
      net::post(ws_.get_executor(), [self, tr, tcl, req]() {
        WsResponse resp;
        resp.id = req.id;
        resp.type = 0;
        try {
          // STA is not thread-safe; serialize with the Tcl evaluator mutex
          std::lock_guard<std::mutex> lock(tcl->mutex);
          auto paths = tr->getReport(req.timing_is_setup, req.timing_max_paths);
          std::stringstream ss;
          ss << "{\"paths\": [";
          bool first_path = true;
          for (const auto& p : paths) {
            if (!first_path) {
              ss << ", ";
            }
            first_path = false;
            ss << "{\"start_clk\": \"" << json_escape(p.start_clk)
               << "\", \"end_clk\": \"" << json_escape(p.end_clk)
               << "\", \"required\": " << p.required
               << ", \"arrival\": " << p.arrival << ", \"slack\": " << p.slack
               << ", \"skew\": " << p.skew
               << ", \"path_delay\": " << p.path_delay
               << ", \"logic_depth\": " << p.logic_depth
               << ", \"fanout\": " << p.fanout << ", \"start_pin\": \""
               << json_escape(p.start_pin) << "\", \"end_pin\": \""
               << json_escape(p.end_pin) << "\", \"data_nodes\": [";
            bool first_node = true;
            for (const auto& n : p.data_nodes) {
              if (!first_node) {
                ss << ", ";
              }
              first_node = false;
              ss << "{\"pin\": \"" << json_escape(n.pin_name) << "\""
                 << ", \"fanout\": " << n.fanout
                 << ", \"rise\": " << (n.is_rising ? "true" : "false")
                 << ", \"clk\": " << (n.is_clock ? "true" : "false")
                 << ", \"time\": " << n.time << ", \"delay\": " << n.delay
                 << ", \"slew\": " << n.slew << ", \"load\": " << n.load
                 << "}";
            }
            ss << "], \"capture_nodes\": [";
            first_node = true;
            for (const auto& n : p.capture_nodes) {
              if (!first_node) {
                ss << ", ";
              }
              first_node = false;
              ss << "{\"pin\": \"" << json_escape(n.pin_name) << "\""
                 << ", \"fanout\": " << n.fanout
                 << ", \"rise\": " << (n.is_rising ? "true" : "false")
                 << ", \"clk\": " << (n.is_clock ? "true" : "false")
                 << ", \"time\": " << n.time << ", \"delay\": " << n.delay
                 << ", \"slew\": " << n.slew << ", \"load\": " << n.load
                 << "}";
            }
            ss << "]}";
          }
          ss << "]}";
          const std::string json = ss.str();
          resp.payload.assign(json.begin(), json.end());
        } catch (const std::exception& e) {
          resp.type = 2;
          std::string err = std::string("server error: ") + e.what();
          resp.payload.assign(err.begin(), err.end());
        }
        self->queue_response(resp);
      });
    } else if (req.type == WsRequest::TIMING_HIGHLIGHT) {
      auto gen = generator_;
      auto tr = timing_report_;
      auto tcl = tcl_eval_;
      net::post(ws_.get_executor(), [self, gen, tr, tcl, req]() {
        WsResponse resp;
        resp.id = req.id;
        resp.type = 0;
        try {
          std::vector<ColoredRect> new_rects;
          std::vector<FlightLine> new_lines;

          if (req.timing_path_index >= 0) {
            // Re-fetch timing paths to get the selected path's data
            std::lock_guard<std::mutex> sta_lock(tcl->mutex);
            auto paths = tr->getReport(req.timing_highlight_setup);
            if (req.timing_path_index < static_cast<int>(paths.size())) {
              odb::dbBlock* block = gen->getBlock();
              collectTimingPathShapes(
                  block, paths[req.timing_path_index], new_rects, new_lines);

              // If a specific pin is selected, highlight its net in yellow
              if (!req.timing_pin_name.empty()) {
                static const Color kStageColor{255, 255, 0, 180};
                auto [iterm, bterm]
                    = resolvePin(block, req.timing_pin_name);
                odb::dbNet* net = iterm   ? iterm->getNet()
                                  : bterm ? bterm->getNet()
                                          : nullptr;
                if (net) {
                  collectNetShapes(net, iterm, bterm, nullptr, nullptr,
                                   kStageColor, new_rects, new_lines);
                }
              }
            }
          }

          std::cerr << "TIMING_HIGHLIGHT: " << new_rects.size()
                    << " rects, " << new_lines.size() << " lines"
                    << " pin_name='" << req.timing_pin_name << "'\n";

          {
            std::lock_guard<std::mutex> lock(self->selection_mutex_);
            self->timing_rects_ = std::move(new_rects);
            self->timing_lines_ = std::move(new_lines);
            self->highlight_rects_.clear();
          }

          const std::string json = "{\"ok\": true}";
          resp.payload.assign(json.begin(), json.end());
        } catch (const std::exception& e) {
          resp.type = 2;
          std::string err = std::string("server error: ") + e.what();
          resp.payload.assign(err.begin(), err.end());
        }
        self->queue_response(resp);
      });
    } else {
      // Capture current highlight rects for tile rendering
      std::vector<odb::Rect> rects;
      std::vector<ColoredRect> colored;
      std::vector<FlightLine> lines;
      {
        std::lock_guard<std::mutex> lock(selection_mutex_);
        rects = highlight_rects_;
        colored = timing_rects_;
        lines = timing_lines_;
      }

      // Dispatch tile generation to the thread pool (not to the strand).
      // This lets multiple tiles render concurrently across all 32 threads.
      net::post(
          ws_.get_executor(),
          [self, gen, req, rects = std::move(rects),
           colored = std::move(colored), lines = std::move(lines)]() {
            WsResponse resp;
            try {
              resp = dispatch_request(req, gen, rects, colored, lines);
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
  std::shared_ptr<TclEvaluator> tcl_eval_;
  std::shared_ptr<TimingReport> timing_report_;
  http::request<http::string_body> req_;
  std::string doc_root_;

 public:
  detect_session(tcp::socket&& socket,
                 std::shared_ptr<TileGenerator> generator,
                 std::shared_ptr<TclEvaluator> tcl_eval,
                 std::shared_ptr<TimingReport> timing_report,
                 std::string doc_root)
      : stream_(std::move(socket)),
        generator_(std::move(generator)),
        tcl_eval_(std::move(tcl_eval)),
        timing_report_(std::move(timing_report)),
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
      auto ws = std::make_shared<ws_session>(
          stream_.release_socket(), generator_, tcl_eval_, timing_report_);
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
  std::shared_ptr<TclEvaluator> tcl_eval_;
  std::shared_ptr<TimingReport> timing_report_;
  std::string doc_root_;

 public:
  listener(net::io_context& ioc,
           tcp::endpoint endpoint,
           std::shared_ptr<TileGenerator> generator,
           std::shared_ptr<TclEvaluator> tcl_eval,
           std::shared_ptr<TimingReport> timing_report,
           std::string doc_root)
      : ioc_(ioc),
        acceptor_(ioc),
        generator_(generator),
        tcl_eval_(std::move(tcl_eval)),
        timing_report_(std::move(timing_report)),
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
      std::make_shared<detect_session>(
          std::move(socket), generator_, tcl_eval_, timing_report_, doc_root_)
          ->run();
    }
    do_accept();
  }
};

WebServer::WebServer(odb::dbDatabase* db,
                     sta::dbSta* sta,
                     utl::Logger* logger,
                     Tcl_Interp* interp)
    : db_(db), sta_(sta), logger_(logger), interp_(interp)
{
}

WebServer::~WebServer() = default;

void WebServer::serve(const std::string& doc_root)
{
  try {
    generator_ = std::make_shared<TileGenerator>(db_, sta_, logger_);
    auto timing_report = std::make_shared<TimingReport>(sta_);

    // Create Tcl evaluator with logger sink for output capture
    auto tcl_eval = std::make_shared<TclEvaluator>(interp_, logger_);

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
        ioc,
        tcp::endpoint{address, port},
        generator_,
        tcl_eval,
        timing_report,
        doc_root)
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
