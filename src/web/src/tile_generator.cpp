// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "tile_generator.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
#include <vector>

#include "color.h"
#include "db_sta/dbSta.hh"
#include "lodepng.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "request_handler.h"
#include "search.h"
#include "timing_report.h"
#include "utl/Logger.h"

namespace web {

void TileVisibility::parseFromJson(const std::string& json)
{
  struct BoolField
  {
    const char* key;
    bool TileVisibility::*field;
    bool default_val;
  };

  // clang-format off
  static const BoolField fields[] = {
    {"stdcells",           &TileVisibility::stdcells,           true},
    {"macros",             &TileVisibility::macros,             true},
    {"pad_input",          &TileVisibility::pad_input,          true},
    {"pad_output",         &TileVisibility::pad_output,         true},
    {"pad_inout",          &TileVisibility::pad_inout,          true},
    {"pad_power",          &TileVisibility::pad_power,          true},
    {"pad_spacer",         &TileVisibility::pad_spacer,         true},
    {"pad_areaio",         &TileVisibility::pad_areaio,         true},
    {"pad_other",          &TileVisibility::pad_other,          true},
    {"phys_fill",          &TileVisibility::phys_fill,          true},
    {"phys_endcap",        &TileVisibility::phys_endcap,        true},
    {"phys_welltap",       &TileVisibility::phys_welltap,       true},
    {"phys_tie",           &TileVisibility::phys_tie,           true},
    {"phys_antenna",       &TileVisibility::phys_antenna,       true},
    {"phys_cover",         &TileVisibility::phys_cover,         true},
    {"phys_bump",          &TileVisibility::phys_bump,          true},
    {"phys_other",         &TileVisibility::phys_other,         true},
    {"std_bufinv",         &TileVisibility::std_bufinv,         true},
    {"std_bufinv_timing",  &TileVisibility::std_bufinv_timing,  true},
    {"std_clock_bufinv",   &TileVisibility::std_clock_bufinv,   true},
    {"std_clock_gate",     &TileVisibility::std_clock_gate,     true},
    {"std_level_shift",    &TileVisibility::std_level_shift,    true},
    {"std_sequential",     &TileVisibility::std_sequential,     true},
    {"std_combinational",  &TileVisibility::std_combinational,  true},
    {"net_signal",         &TileVisibility::net_signal,         true},
    {"net_power",          &TileVisibility::net_power,          true},
    {"net_ground",         &TileVisibility::net_ground,         true},
    {"net_clock",          &TileVisibility::net_clock,          true},
    {"net_reset",          &TileVisibility::net_reset,          true},
    {"net_tieoff",         &TileVisibility::net_tieoff,         true},
    {"net_scan",           &TileVisibility::net_scan,           true},
    {"net_analog",         &TileVisibility::net_analog,         true},
    {"routing",            &TileVisibility::routing,            true},
    {"special_nets",       &TileVisibility::special_nets,       true},
    {"pins",               &TileVisibility::pins,               true},
    {"blockages",          &TileVisibility::blockages,          true},
    {"debug",              &TileVisibility::debug,              false},
  };
  // clang-format on

  for (const auto& f : fields) {
    this->*(f.field) = extract_int_or(json, f.key, f.default_val ? 1 : 0);
  }
}

bool TileVisibility::isNetVisible(odb::dbNet* net) const
{
  switch (net->getSigType().getValue()) {
    case odb::dbSigType::SIGNAL:
      return net_signal;
    case odb::dbSigType::POWER:
      return net_power;
    case odb::dbSigType::GROUND:
      return net_ground;
    case odb::dbSigType::CLOCK:
      return net_clock;
    case odb::dbSigType::RESET:
      return net_reset;
    case odb::dbSigType::TIEOFF:
      return net_tieoff;
    case odb::dbSigType::SCAN:
      return net_scan;
    case odb::dbSigType::ANALOG:
      return net_analog;
  }
  return true;
}

bool TileVisibility::isInstVisible(odb::dbInst* inst, sta::dbSta* sta) const
{
  odb::dbMaster* master = inst->getMaster();
  odb::dbMasterType mtype = master->getType();

  if (sta) {
    using IT = sta::dbSta::InstType;
    switch (sta->getInstanceType(inst)) {
      case IT::BLOCK:
        return macros;
      case IT::PAD_INPUT:
        return pad_input;
      case IT::PAD_OUTPUT:
        return pad_output;
      case IT::PAD_INOUT:
        return pad_inout;
      case IT::PAD_POWER:
        return pad_power;
      case IT::PAD_SPACER:
        return pad_spacer;
      case IT::PAD_AREAIO:
        return pad_areaio;
      case IT::PAD:
        return pad_other;
      case IT::ENDCAP:
        return phys_endcap;
      case IT::FILL:
        return phys_fill;
      case IT::TAPCELL:
        return phys_welltap;
      case IT::TIE:
        return phys_tie;
      case IT::ANTENNA:
        return phys_antenna;
      case IT::COVER:
        return phys_cover;
      case IT::BUMP:
        return phys_bump;
      case IT::LEF_OTHER:
        return phys_other;
      case IT::STD_BUF:
      case IT::STD_INV:
        return std_bufinv;
      case IT::STD_BUF_TIMING_REPAIR:
      case IT::STD_INV_TIMING_REPAIR:
        return std_bufinv_timing;
      case IT::STD_BUF_CLK_TREE:
      case IT::STD_INV_CLK_TREE:
        return std_clock_bufinv;
      case IT::STD_CLOCK_GATE:
        return std_clock_gate;
      case IT::STD_LEVEL_SHIFT:
        return std_level_shift;
      case IT::STD_SEQUENTIAL:
        return std_sequential;
      case IT::STD_COMBINATIONAL:
        return std_combinational;
      case IT::STD_CELL:
      case IT::STD_PHYSICAL:
      case IT::STD_OTHER:
      default:
        return stdcells;
    }
  }

  // Fallback: dbMasterType-only classification (no Liberty)
  if (mtype.isBlock()) {
    return macros;
  }
  if (mtype.isPad()) {
    if (mtype == odb::dbMasterType::PAD_INPUT) {
      return pad_input;
    }
    if (mtype == odb::dbMasterType::PAD_OUTPUT) {
      return pad_output;
    }
    if (mtype == odb::dbMasterType::PAD_INOUT) {
      return pad_inout;
    }
    if (mtype == odb::dbMasterType::PAD_POWER) {
      return pad_power;
    }
    if (mtype == odb::dbMasterType::PAD_SPACER) {
      return pad_spacer;
    }
    if (mtype == odb::dbMasterType::PAD_AREAIO) {
      return pad_areaio;
    }
    return pad_other;
  }
  if (mtype.isEndCap()) {
    return phys_endcap;
  }
  if (master->isFiller()) {
    return phys_fill;
  }
  if (mtype == odb::dbMasterType::CORE_WELLTAP) {
    return phys_welltap;
  }
  if (mtype == odb::dbMasterType::CORE_TIEHIGH
      || mtype == odb::dbMasterType::CORE_TIELOW) {
    return phys_tie;
  }
  if (mtype == odb::dbMasterType::CORE_ANTENNACELL) {
    return phys_antenna;
  }
  if (mtype.isCover()) {
    if (mtype == odb::dbMasterType::COVER_BUMP) {
      return phys_bump;
    }
    return phys_cover;
  }
  if (mtype == odb::dbMasterType::CORE_SPACER
      || inst->getSourceType() == odb::dbSourceType::DIST) {
    return phys_other;
  }
  return stdcells;
}

//////////////////////////////////////////////////

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
      = std::max(1,
                 static_cast<int>(getBounds().maxDXDY()
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
    const bool visible = vis.isInstVisible(inst, sta_);
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

      if (!vis.isInstVisible(inst, sta_)) {
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
        const odb::Point origin((sbox->xMin() + sbox->xMax()) / 2,
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

void TileGenerator::drawColoredHighlight(std::vector<unsigned char>& image,
                                         const std::vector<ColoredRect>& rects,
                                         const std::string& current_layer,
                                         const odb::Rect& dbu_tile,
                                         const double scale)
{
  const bool is_instances_layer = (current_layer == "_instances");
  for (const auto& cr : rects) {
    // Layer filtering: draw on _instances (overview) or matching layer
    if (!is_instances_layer && !cr.layer.empty() && cr.layer != current_layer) {
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
    int px0 = static_cast<int>((fl.p1.x() - dbu_tile.xMin()) * scale);
    int py0 = 255 - static_cast<int>((fl.p1.y() - dbu_tile.yMin()) * scale);
    int px1 = static_cast<int>((fl.p2.x() - dbu_tile.xMin()) * scale);
    int py1 = 255 - static_cast<int>((fl.p2.y() - dbu_tile.yMin()) * scale);

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

std::pair<odb::dbITerm*, odb::dbBTerm*> resolvePin(odb::dbBlock* block,
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

void collectNetShapes(odb::dbNet* net,
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
        rects.push_back({shape.getBox(), color, layer ? layer->getName() : ""});
      }
    }
  } else {
    // Unrouted: draw flight line between driver and sink
    odb::Point p1 = getPinLocation(drv_iterm, drv_bterm);
    odb::Point p2 = getPinLocation(snk_iterm, snk_bterm);
    lines.push_back({p1, p2, color});
  }
}

void collectTimingPathShapes(odb::dbBlock* block,
                             const TimingPathSummary& path,
                             std::vector<ColoredRect>& rects,
                             std::vector<FlightLine>& lines)
{
  static const Color kLaunchClkColor{0, 255, 255, 180};  // Cyan (match GUI)
  static const Color kSignalColor{255, 0, 0, 180};       // Red (match GUI)
  static const Color kCaptureClkColor{0, 255, 0, 180};   // Green (match GUI)

  // Track nets already collected to avoid duplicates
  std::set<odb::dbNet*> seen_nets;

  auto processNodes = [&](const std::vector<TimingNode>& nodes,
                          const Color& clk_color,
                          const Color& data_color) {
    for (size_t i = 0; i + 1 < nodes.size(); i++) {
      auto [a_iterm, a_bterm] = resolvePin(block, nodes[i].pin_name);
      auto [b_iterm, b_bterm] = resolvePin(block, nodes[i + 1].pin_name);

      odb::dbNet* net_a = a_iterm ? a_iterm->getNet()
                                  : (a_bterm ? a_bterm->getNet() : nullptr);
      odb::dbNet* net_b = b_iterm ? b_iterm->getNet()
                                  : (b_bterm ? b_bterm->getNet() : nullptr);

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

}  // namespace web
