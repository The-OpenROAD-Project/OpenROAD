// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "tile_generator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "color.h"
#include "db_sta/dbSta.hh"
#include "gui/heatMap.h"
#include "lodepng.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "request_handler.h"
#include "search.h"
#include "timing_report.h"
#include "utl/Logger.h"

namespace web {

namespace {

constexpr int kBitmapGlyphWidth = 5;
constexpr int kBitmapGlyphHeight = 7;
constexpr int kBitmapGlyphSpacing = 1;

const unsigned char* getBitmapGlyph(const char ch)
{
  // Minimal 5x7 bitmap font. Each byte is one row, only the low 5 bits are
  // used.
  switch (ch) {
    case '0': {
      static constexpr unsigned char glyph[]
          = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
      return glyph;
    }
    case '1': {
      static constexpr unsigned char glyph[]
          = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
      return glyph;
    }
    case '2': {
      static constexpr unsigned char glyph[]
          = {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F};
      return glyph;
    }
    case '3': {
      static constexpr unsigned char glyph[]
          = {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E};
      return glyph;
    }
    case '4': {
      static constexpr unsigned char glyph[]
          = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
      return glyph;
    }
    case '5': {
      static constexpr unsigned char glyph[]
          = {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E};
      return glyph;
    }
    case '6': {
      static constexpr unsigned char glyph[]
          = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
      return glyph;
    }
    case '7': {
      static constexpr unsigned char glyph[]
          = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
      return glyph;
    }
    case '8': {
      static constexpr unsigned char glyph[]
          = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
      return glyph;
    }
    case '9': {
      static constexpr unsigned char glyph[]
          = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C};
      return glyph;
    }
    case '.': {
      static constexpr unsigned char glyph[]
          = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C};
      return glyph;
    }
    case '-': {
      static constexpr unsigned char glyph[]
          = {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};
      return glyph;
    }
    case '/': {
      static constexpr unsigned char glyph[]
          = {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10};
      return glyph;
    }
    case '=': {
      static constexpr unsigned char glyph[]
          = {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00};
      return glyph;
    }
    case 'x': {
      static constexpr unsigned char glyph[]
          = {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11};
      return glyph;
    }
    case 'y': {
      static constexpr unsigned char glyph[]
          = {0x00, 0x00, 0x11, 0x0A, 0x04, 0x04, 0x04};
      return glyph;
    }
    case 'z': {
      static constexpr unsigned char glyph[]
          = {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F};
      return glyph;
    }
    default:
      return nullptr;
  }
}

int getBitmapGlyphAdvance(const char ch)
{
  if (ch == ' ') {
    return kBitmapGlyphWidth;
  }
  return kBitmapGlyphWidth + kBitmapGlyphSpacing;
}

}  // namespace

void TileVisibility::parseFromJson(const std::string& json)
{
  struct BoolField
  {
    const char* key;
    bool TileVisibility::*field;
    bool default_val;
  };

  // clang-format off
  // NOLINTBEGIN(modernize-use-designated-initializers)
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
    {"blockages",              &TileVisibility::blockages,              true},
    {"placement_blockages",    &TileVisibility::placement_blockages,    true},
    {"routing_obstructions",   &TileVisibility::routing_obstructions,   true},
    {"rows",                   &TileVisibility::rows,                   false},
    {"tracks_pref",            &TileVisibility::tracks_pref,            false},
    {"tracks_non_pref",        &TileVisibility::tracks_non_pref,        false},
    {"debug",                  &TileVisibility::debug,                  false},
  };
  // NOLINTEND(modernize-use-designated-initializers)
  // clang-format on

  for (const auto& f : fields) {
    this->*(f.field) = extract_int_or(json, f.key, f.default_val ? 1 : 0);
  }
  raw_json_ = json;
}

bool TileVisibility::isSiteVisible(const std::string& site_name) const
{
  if (!rows) {
    return false;
  }
  const std::string key = "site_" + site_name;
  return extract_int_or(raw_json_, key, 0);
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
  const odb::dbMasterType mtype = master->getType();

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
    : db_(db),
      sta_(sta),
      logger_(logger),
      search_(std::make_unique<Search>(logger))
{
  odb::dbChip* chip = db_->getChip();
  if (chip) {
    search_->setTopChip(chip);
  }
}

TileGenerator::~TileGenerator() = default;

void TileGenerator::eagerInit()
{
  odb::dbChip* chip = db_->getChip();
  if (chip) {
    search_->setTopChip(chip);
  }
  odb::dbBlock* block = getBlock();
  if (block) {
    search_->eagerInit(block);
  }
}

bool TileGenerator::shapesReady() const
{
  return search_->shapesReady();
}

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
                             const Color& c) const
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

void TileGenerator::fillPolygon(std::vector<unsigned char>& image,
                                const odb::Polygon& poly,
                                const odb::Rect& dbu_tile,
                                const double scale,
                                const Color& color,
                                const bool blend) const
{
  const auto& points = poly.getPoints();
  const int n = static_cast<int>(points.size());
  if (n < 3) {
    return;
  }

  // Convert polygon points to pixel coordinates (floating point for precision).
  std::vector<double> px(n), py(n);
  for (int i = 0; i < n; ++i) {
    px[i] = (points[i].x() - dbu_tile.xMin()) * scale;
    py[i] = (points[i].y() - dbu_tile.yMin()) * scale;
  }

  // Compute pixel bounding box, clamped to tile.
  const double min_py = std::ranges::min(py);
  const double max_py = std::ranges::max(py);
  const int iy_min = std::max(0, static_cast<int>(min_py));
  const int iy_max
      = std::min(kTileSizeInPixel, static_cast<int>(std::ceil(max_py)));

  // Scanline fill: for each row, find edge intersections and fill between
  // pairs.
  std::vector<double> x_intercepts;
  for (int iy = iy_min; iy < iy_max; ++iy) {
    const double scanline = iy + 0.5;  // test at pixel center
    x_intercepts.clear();

    for (int i = 0, j = n - 1; i < n; j = i++) {
      // Skip degenerate (horizontal) edges and only process edges that
      // straddle the scanline.
      if ((py[i] <= scanline) == (py[j] <= scanline)) {
        continue;
      }
      const double x
          = px[i] + (scanline - py[i]) / (py[j] - py[i]) * (px[j] - px[i]);
      x_intercepts.push_back(x);
    }

    std::ranges::sort(x_intercepts);

    for (size_t k = 0; k + 1 < x_intercepts.size(); k += 2) {
      const int ix_min = std::max(0, static_cast<int>(x_intercepts[k]));
      const int ix_max = std::min(
          kTileSizeInPixel, static_cast<int>(std::ceil(x_intercepts[k + 1])));
      const int draw_y = 255 - iy;
      for (int ix = ix_min; ix < ix_max; ++ix) {
        if (blend) {
          blendPixel(image, ix, draw_y, color);
        } else {
          setPixel(image, ix, draw_y, color);
        }
      }
    }
  }
}

odb::Rect TileGenerator::getBounds() const
{
  odb::Rect bounds;
  if (odb::dbBlock* block = getBlock()) {
    bounds = block->getBBox()->getBox();
  }
  return bounds;
}

std::vector<std::string> TileGenerator::getLayers() const
{
  std::vector<std::string> layers;
  odb::dbTech* tech = db_->getTech();
  if (!tech) {
    return layers;
  }
  for (odb::dbTechLayer* layer : tech->getLayers()) {
    if (layer->getRoutingLevel() > 0
        || layer->getType() == odb::dbTechLayerType::CUT) {
      layers.push_back(layer->getName());
    }
  }
  return layers;
}

std::vector<std::string> TileGenerator::getSites() const
{
  std::set<std::string> seen;
  std::vector<std::string> sites;
  odb::dbBlock* block = getBlock();
  if (!block) {
    return sites;
  }
  for (odb::dbRow* row : block->getRows()) {
    odb::dbSite* site = row->getSite();
    if (site && seen.insert(site->getName()).second) {
      sites.push_back(site->getName());
    }
  }
  return sites;
}

TileGenerator::SnapResult TileGenerator::snapAt(
    const int dbu_x,
    const int dbu_y,
    const int search_radius,
    const int point_snap_threshold,
    const bool horizontal,
    const bool vertical,
    const TileVisibility& vis,
    const std::set<std::string>& visible_layers) const
{
  odb::dbBlock* block = getBlock();
  if (!block) {
    return {};
  }
  auto sr = search_->searchNearestEdge(block,
                                       odb::Point(dbu_x, dbu_y),
                                       search_radius,
                                       point_snap_threshold,
                                       horizontal,
                                       vertical,
                                       vis,
                                       visible_layers);
  SnapResult result;
  result.edge = sr.edge;
  result.distance = sr.distance;
  result.found = sr.found;
  return result;
}

std::vector<SelectionResult> TileGenerator::selectAt(
    const int dbu_x,
    const int dbu_y,
    const int zoom,
    const TileVisibility& vis,
    const std::set<std::string>& visible_layers)
{
  std::vector<SelectionResult> results;
  odb::dbBlock* block = getBlock();
  if (!block) {
    return results;
  }
  // Compute a search margin of 2 pixels at the current zoom level.
  // This accounts for coordinate conversion rounding between the client's
  // Leaflet CRS.Simple coordinates and the server's DBU space.
  const int num_tiles = 1 << std::max(0, zoom);
  const int margin
      = std::max(1, getBounds().maxDXDY() / (kTileSizeInPixel * num_tiles) * 2);
  debugPrint(logger_,
             utl::WEB,
             "select",
             1,
             "selectAt dbu=({},{}) zoom={} margin={}",
             dbu_x,
             dbu_y,
             zoom,
             margin);

  const int x_lo = dbu_x - margin;
  const int y_lo = dbu_y - margin;
  const int x_hi = dbu_x + margin;
  const int y_hi = dbu_y + margin;
  const odb::Point click_pt(dbu_x, dbu_y);

  // Search instances
  for (odb::dbInst* inst :
       search_->searchInsts(block, x_lo, y_lo, x_hi, y_hi)) {
    const odb::Rect bbox = inst->getBBox()->getBox();
    if (bbox.intersects(click_pt) && vis.isInstVisible(inst, sta_)) {
      results.push_back({inst, inst->getName(), "Inst", bbox});
    }
  }
  // Sort instances by area descending so larger instances (macros) come first
  std::ranges::sort(results, [](const auto& a, const auto& b) {
    return a.bbox.area() > b.bbox.area();
  });

  // Search nets via routing shapes on each layer
  std::set<odb::dbNet*> seen_nets;
  odb::dbTech* tech = db_->getTech();
  for (odb::dbTechLayer* layer : tech->getLayers()) {
    if (layer->getRoutingLevel() <= 0
        && layer->getType() != odb::dbTechLayerType::CUT) {
      continue;
    }
    if (!visible_layers.empty() && !visible_layers.contains(layer->getName())) {
      continue;
    }

    // Regular routing shapes (wires, vias, bterms)
    if (vis.routing) {
      for (const auto& shape :
           search_->searchBoxShapes(block, layer, x_lo, y_lo, x_hi, y_hi)) {
        odb::dbNet* net = std::get<2>(shape);
        if (seen_nets.contains(net)) {
          continue;
        }
        const odb::Rect& box = std::get<0>(shape);
        if (box.intersects(click_pt) && vis.isNetVisible(net)) {
          seen_nets.insert(net);
          results.push_back({net, net->getName(), "Net", net->getTermBBox()});
        }
      }
    }

    // Special net shapes (power/ground straps)
    if (vis.special_nets) {
      for (const auto& shape :
           search_->searchSNetViaShapes(block, layer, x_lo, y_lo, x_hi, y_hi)) {
        odb::dbNet* net = std::get<1>(shape);
        if (seen_nets.contains(net)) {
          continue;
        }
        const odb::Rect box = std::get<0>(shape)->getBox();
        if (box.intersects(click_pt) && vis.isNetVisible(net)) {
          seen_nets.insert(net);
          results.push_back({net, net->getName(), "Net", net->getTermBBox()});
        }
      }

      for (const auto& shape :
           search_->searchSNetShapes(block, layer, x_lo, y_lo, x_hi, y_hi)) {
        odb::dbNet* net = std::get<2>(shape);
        if (seen_nets.contains(net)) {
          continue;
        }
        const odb::Rect box = std::get<0>(shape)->getBox();
        if (box.intersects(click_pt) && vis.isNetVisible(net)) {
          seen_nets.insert(net);
          results.push_back({net, net->getName(), "Net", net->getTermBBox()});
        }
      }
    }
  }

  debugPrint(logger_,
             utl::WEB,
             "select",
             1,
             "  selected={} (insts={}, nets={})",
             results.size(),
             results.size() - seen_nets.size(),
             seen_nets.size());
  return results;
}

odb::dbBlock* TileGenerator::getBlock() const
{
  odb::dbChip* chip = db_->getChip();
  return chip ? chip->getBlock() : nullptr;
}

std::vector<unsigned char> TileGenerator::generateTile(
    const std::string& layer,
    const int z,
    const int x,
    int y,
    const TileVisibility& vis,
    const std::vector<odb::Rect>& highlight_rects,
    const std::vector<odb::Polygon>& highlight_polys,
    const std::vector<ColoredRect>& colored_rects,
    const std::vector<FlightLine>& flight_lines,
    const std::map<uint32_t, Color>* module_colors,
    const std::set<uint32_t>* focus_net_ids,
    const std::set<uint32_t>* route_guide_net_ids) const
{
  static_assert(sizeof(Color) == 4);
  constexpr int buffer_size = kTileSizeInPixel * kTileSizeInPixel * 4;
  std::vector<unsigned char> image_buffer(buffer_size, 0);

  // No design loaded — return blank tile.
  if (!getBlock()) {
    std::vector<unsigned char> png_data;
    lodepng::encode(png_data, image_buffer, kTileSizeInPixel, kTileSizeInPixel);
    return png_data;
  }

  // Per-layer colors: routing level 1=blue, 2=red, then distinct hues
  static const Color palette[] = {
      // clang-format off
      // NOLINTBEGIN(modernize-use-designated-initializers)
      { 70, 130, 210, 180},  // moderate blue
      {200,  50,  50, 180},   // red
      { 50, 180,  80, 180},   // green
      {200, 160,  40, 180},  // amber
      {160,  60, 200, 180},  // purple
      { 40, 190, 190, 180},  // teal
      {220, 120,  50, 180},  // orange
      {180,  70, 150, 180},  // magenta
      // NOLINTEND(modernize-use-designated-initializers)
      // clang-format on
  };
  static constexpr int palette_size = sizeof(palette) / sizeof(palette[0]);

  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* tech_layer = tech->findLayer(layer.c_str());

  int layer_index = 0;
  if (tech_layer) {
    const auto all_layers = getLayers();
    const auto it = std::ranges::find(all_layers, layer);
    if (it != all_layers.end()) {
      layer_index = std::distance(all_layers.begin(), it);
    }
  }
  const Color color = palette[layer_index % palette_size];
  const Color obs_color = color.lighter();

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

    odb::dbBlock* block = getBlock();

    // Special "_modules" layer: draw filled module-colored rectangles
    const bool modules_layer
        = (layer == "_modules" && module_colors && !module_colors->empty());
    if (modules_layer) {
      for (odb::dbInst* inst : search_->searchInsts(
               block, dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max)) {
        odb::Rect inst_bbox = inst->getBBox()->getBox();
        if (!dbu_tile.overlaps(inst_bbox)) {
          continue;
        }
        odb::dbModule* mod = inst->getModule();
        if (!mod) {
          continue;
        }
        auto it = module_colors->find(mod->getId());
        if (it == module_colors->end()) {
          continue;
        }
        const Color& c = it->second;
        const int pxl
            = std::max(0, (int) ((inst_bbox.xMin() - dbu_x_min) * scale));
        const int pyl
            = std::max(0, (int) ((inst_bbox.yMin() - dbu_y_min) * scale));
        const int pxh = std::min(
            255, (int) std::ceil((inst_bbox.xMax() - dbu_x_min) * scale));
        const int pyh = std::min(
            255, (int) std::ceil((inst_bbox.yMax() - dbu_y_min) * scale));
        for (int iy = pyl; iy < pyh; ++iy) {
          for (int ix = pxl; ix < pxh; ++ix) {
            blendPixel(image_buffer, ix, 255 - iy, c);
          }
        }
      }
    }

    // Special "_instances" layer: only draw instance borders, no routing
    const bool instances_only = (layer == "_instances");

    // "_modules" layer only draws filled module-color rects (already done
    // above); skip all other drawing (instances, routing, etc.)
    if (!modules_layer) {
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
          const Color gray{.r = 128, .g = 128, .b = 128, .a = 255};
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
            for (odb::dbPolygon* poly_obs : master->getPolygonObstructions()) {
              if (tech_layer && poly_obs->getTechLayer() != tech_layer) {
                continue;
              }
              odb::Polygon poly = poly_obs->getPolygon();
              inst->getTransform().apply(poly);
              fillPolygon(image_buffer, poly, dbu_tile, scale, obs_color);
            }
            for (odb::dbBox* obs : master->getObstructions(false)) {
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

              drawFilledRect(image_buffer, draw, obs_color);
            }
          }

          if (vis.pins) {
            for (odb::dbMTerm* mterm : master->getMTerms()) {
              for (odb::dbMPin* mpin : mterm->getMPins()) {
                for (odb::dbPolygon* poly_geom : mpin->getPolygonGeometry()) {
                  if (tech_layer && poly_geom->getTechLayer() != tech_layer) {
                    continue;
                  }
                  odb::Polygon poly = poly_geom->getPolygon();
                  inst->getTransform().apply(poly);
                  fillPolygon(image_buffer, poly, dbu_tile, scale, color);
                }
                for (odb::dbBox* geom : mpin->getGeometry(false)) {
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

                  drawFilledRect(image_buffer, draw, color);
                }
              }
            }
          }
        }
      }

      // Draw routing shapes (wires, vias, bterms) on top of instances
      if (!instances_only && tech_layer && vis.routing && shapesReady()) {
        for (const auto& shape : search_->searchBoxShapes(block,
                                                          tech_layer,
                                                          dbu_x_min,
                                                          dbu_y_min,
                                                          dbu_x_max,
                                                          dbu_y_max)) {
          odb::dbNet* net = std::get<2>(shape);
          if (!vis.isNetVisible(net)) {
            continue;
          }
          if (focus_net_ids && !focus_net_ids->empty()
              && focus_net_ids->find(net->getId()) == focus_net_ids->end()) {
            continue;
          }
          const odb::Rect& box = std::get<0>(shape);
          if (!box.overlaps(dbu_tile)) {
            continue;
          }
          const odb::Rect overlap = box.intersect(dbu_tile);
          const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

          drawFilledRect(image_buffer, draw, color);
        }
      }

      // Draw special net shapes (power/ground straps) on top of instances
      if (!instances_only && tech_layer && vis.special_nets && shapesReady()) {
        for (const auto& shape : search_->searchSNetShapes(block,
                                                           tech_layer,
                                                           dbu_x_min,
                                                           dbu_y_min,
                                                           dbu_x_max,
                                                           dbu_y_max)) {
          odb::dbNet* snet = std::get<2>(shape);
          if (!vis.isNetVisible(snet)) {
            continue;
          }
          if (focus_net_ids && !focus_net_ids->empty()
              && focus_net_ids->find(snet->getId()) == focus_net_ids->end()) {
            continue;
          }
          const odb::Rect box = std::get<0>(shape)->getBox();
          if (!box.overlaps(dbu_tile)) {
            continue;
          }
          const odb::Polygon& poly = std::get<1>(shape);
          fillPolygon(image_buffer, poly, dbu_tile, scale, color);
        }
      }

      // Draw special net vias — decompose into individual cut boxes
      if (!instances_only && tech_layer && vis.special_nets && shapesReady()) {
        for (const auto& shape : search_->searchSNetViaShapes(block,
                                                              tech_layer,
                                                              dbu_x_min,
                                                              dbu_y_min,
                                                              dbu_x_max,
                                                              dbu_y_max)) {
          odb::dbNet* via_net = std::get<1>(shape);
          if (!vis.isNetVisible(via_net)) {
            continue;
          }
          if (focus_net_ids && !focus_net_ids->empty()
              && focus_net_ids->find(via_net->getId())
                     == focus_net_ids->end()) {
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
            drawFilledRect(image_buffer, draw, color);
          }
        }
      }

      // Draw via enclosures from adjacent cut layers onto this metal layer.
      // Vias are indexed by their cut layer in the search structure.  When
      // rendering a routing layer we look up the cut layers immediately above
      // and below, search for vias there, and draw only the enclosure boxes
      // that belong to the current routing layer.
      if (!instances_only && tech_layer && vis.special_nets && shapesReady()
          && tech_layer->getType() == odb::dbTechLayerType::ROUTING) {
        odb::dbTechLayer* adj_cuts[2]
            = {tech_layer->getLowerLayer(), tech_layer->getUpperLayer()};
        for (odb::dbTechLayer* cut_layer : adj_cuts) {
          if (!cut_layer || cut_layer->getType() != odb::dbTechLayerType::CUT) {
            continue;
          }
          for (const auto& shape : search_->searchSNetViaShapes(block,
                                                                cut_layer,
                                                                dbu_x_min,
                                                                dbu_y_min,
                                                                dbu_x_max,
                                                                dbu_y_max)) {
            odb::dbNet* via_net = std::get<1>(shape);
            if (!vis.isNetVisible(via_net)) {
              continue;
            }
            if (focus_net_ids && !focus_net_ids->empty()
                && !focus_net_ids->contains(via_net->getId())) {
              continue;
            }
            odb::dbSBox* sbox = std::get<0>(shape);
            odb::dbSet<odb::dbBox> via_boxes;
            if (auto tech_via = sbox->getTechVia()) {
              via_boxes = tech_via->getBoxes();
            } else if (auto block_via = sbox->getBlockVia()) {
              via_boxes = block_via->getBoxes();
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
              drawFilledRect(image_buffer, draw, color);
            }
          }
        }
      }

      // Draw placement blockages (dbBlockage) on the _instances layer.
      // Diagonal white hash lines in pixel space, with period anchored in dbu
      // coordinates so the pattern is seamless across tile boundaries.
      if (instances_only && vis.placement_blockages) {
        const Color hash_color{.r = 255, .g = 255, .b = 255, .a = 180};
        constexpr int kPixelPeriod = 20;  // pixels between line centers
        constexpr int kLineWidth = 2;     // pixels wide
        for (odb::dbBlockage* blk : search_->searchBlockages(
                 block, dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max)) {
          odb::Rect box = blk->getBBox()->getBox();
          if (!box.overlaps(dbu_tile)) {
            continue;
          }
          const odb::Rect overlap = box.intersect(dbu_tile);
          const odb::Rect draw = toPixels(scale, overlap, dbu_tile);
          // Offset in absolute pixel coordinates for seamless tiling
          const int ox = (int) (dbu_x_min * scale);
          const int oy = (int) (dbu_y_min * scale);
          for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
            for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
              if (((ix + ox) + (iy + oy)) % kPixelPeriod < kLineWidth) {
                blendPixel(image_buffer, ix, 255 - iy, hash_color);
              }
            }
          }
        }
      }

      // Draw routing obstructions (dbObstruction) on per-layer tiles.
      // Same diagonal white hash lines.
      if (!instances_only && tech_layer && vis.routing_obstructions) {
        const Color hash_color{.r = 255, .g = 255, .b = 255, .a = 180};
        constexpr int kPixelPeriod = 20;
        constexpr int kLineWidth = 2;
        for (odb::dbObstruction* obs : search_->searchObstructions(block,
                                                                   tech_layer,
                                                                   dbu_x_min,
                                                                   dbu_y_min,
                                                                   dbu_x_max,
                                                                   dbu_y_max)) {
          odb::Rect box = obs->getBBox()->getBox();
          if (!box.overlaps(dbu_tile)) {
            continue;
          }
          const odb::Rect overlap = box.intersect(dbu_tile);
          const odb::Rect draw = toPixels(scale, overlap, dbu_tile);
          const int ox = (int) (dbu_x_min * scale);
          const int oy = (int) (dbu_y_min * scale);
          for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
            for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
              if (((ix + ox) + (iy + oy)) % kPixelPeriod < kLineWidth) {
                blendPixel(image_buffer, ix, 255 - iy, hash_color);
              }
            }
          }
        }
      }

      // Draw rows as outlines on the _instances layer
      if (instances_only && vis.rows) {
        const Color row_color{
            .r = 60, .g = 180, .b = 60, .a = 180};  // green outlines
        for (const auto& [row_rect, row] : search_->searchRows(
                 block, dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max)) {
          if (!row_rect.overlaps(dbu_tile)) {
            continue;
          }
          odb::dbSite* site = row->getSite();
          if (site && !vis.isSiteVisible(site->getName())) {
            continue;
          }
          const odb::Rect draw = toPixels(scale, row_rect, dbu_tile);
          // Draw outline only (top, bottom, left, right edges)
          for (int ix = draw.xMin(); ix <= draw.xMax(); ++ix) {
            blendPixel(image_buffer, ix, 255 - draw.yMin(), row_color);
            blendPixel(image_buffer, ix, 255 - draw.yMax(), row_color);
          }
          for (int iy = draw.yMin(); iy <= draw.yMax(); ++iy) {
            blendPixel(image_buffer, draw.xMin(), 255 - iy, row_color);
            blendPixel(image_buffer, draw.xMax(), 255 - iy, row_color);
          }
        }
      }

      // Draw tracks on per-layer tiles
      if (!instances_only && tech_layer
          && (vis.tracks_pref || vis.tracks_non_pref)) {
        odb::dbTrackGrid* grid = block->findTrackGrid(tech_layer);
        debugPrint(logger_,
                   utl::WEB,
                   "tile",
                   1,
                   "tracks: layer={} grid={} pref={} non_pref={}",
                   layer,
                   grid != nullptr,
                   vis.tracks_pref,
                   vis.tracks_non_pref);
        if (grid) {
          Color track_color = color;
          track_color.a = 150;
          const bool is_horizontal
              = tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;

          // X-direction tracks (vertical lines on screen)
          // Preferred for vertical layers, non-preferred for horizontal layers
          if ((!is_horizontal && vis.tracks_pref)
              || (is_horizontal && vis.tracks_non_pref)) {
            std::vector<int> x_grid;
            grid->getGridX(x_grid);
            debugPrint(logger_,
                       utl::WEB,
                       "tile",
                       1,
                       "  x_tracks: count={} tile=[{},{},{},{}]",
                       x_grid.size(),
                       dbu_x_min,
                       dbu_y_min,
                       dbu_x_max,
                       dbu_y_max);
            for (int tx : x_grid) {
              if (tx < dbu_x_min || tx > dbu_x_max) {
                continue;
              }
              const int px = static_cast<int>((tx - dbu_x_min) * scale);
              if (px >= 0 && px < kTileSizeInPixel) {
                for (int py = 0; py < kTileSizeInPixel; ++py) {
                  blendPixel(image_buffer, px, py, track_color);
                }
              }
            }
          }

          // Y-direction tracks (horizontal lines on screen)
          // Preferred for horizontal layers, non-preferred for vertical layers
          if ((is_horizontal && vis.tracks_pref)
              || (!is_horizontal && vis.tracks_non_pref)) {
            std::vector<int> y_grid;
            grid->getGridY(y_grid);
            debugPrint(logger_,
                       utl::WEB,
                       "tile",
                       1,
                       "  y_tracks: count={}",
                       y_grid.size());
            for (int ty : y_grid) {
              if (ty < dbu_y_min || ty > dbu_y_max) {
                continue;
              }
              const int py = 255 - static_cast<int>((ty - dbu_y_min) * scale);
              if (py >= 0 && py < kTileSizeInPixel) {
                for (int px = 0; px < kTileSizeInPixel; ++px) {
                  blendPixel(image_buffer, px, py, track_color);
                }
              }
            }
          }
        }
      }

    }  // end if (!modules_layer)

    if (!highlight_rects.empty() || !highlight_polys.empty()) {
      drawHighlight(
          image_buffer, highlight_rects, highlight_polys, dbu_tile, scale);
    }
    if (!colored_rects.empty()) {
      drawColoredHighlight(image_buffer, colored_rects, layer, dbu_tile, scale);
    }
    if (!flight_lines.empty()) {
      drawFlightLines(image_buffer, flight_lines, dbu_tile, scale);
    }
    if (route_guide_net_ids && !route_guide_net_ids->empty() && tech_layer) {
      drawRouteGuides(
          image_buffer, *route_guide_net_ids, layer, color, dbu_tile, scale);
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

  if (logger_->debugCheck(utl::WEB, "tile_generator", 1)) {
    std::string filename = "/tmp/tile_" + layer + "_" + std::to_string(z) + "_"
                           + std::to_string(x) + "_" + std::to_string(y)
                           + ".png";
    lodepng::save_file(png_data, filename);
  }

  return png_data;
}

std::vector<unsigned char> TileGenerator::generateHeatMapTile(
    gui::HeatMapDataSource& source,
    const int z,
    const int x,
    int y) const
{
  constexpr int buffer_size = kTileSizeInPixel * kTileSizeInPixel * 4;
  std::vector<unsigned char> image_buffer(buffer_size, 0);

  const double num_tiles_at_zoom = pow(2, z);
  if (x < 0 || y < 0 || x >= num_tiles_at_zoom || y >= num_tiles_at_zoom) {
    return {};
  }

  y = num_tiles_at_zoom - 1 - y;
  const double tile_dbu_size = getBounds().maxDXDY() / num_tiles_at_zoom;
  const int dbu_x_min = x * tile_dbu_size;
  const int dbu_y_min = y * tile_dbu_size;
  const int dbu_x_max = std::ceil((x + 1) * tile_dbu_size);
  const int dbu_y_max = std::ceil((y + 1) * tile_dbu_size);
  const odb::Rect dbu_tile(dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max);
  const double scale = kTileSizeInPixel / tile_dbu_size;
  constexpr double text_rect_margin = 0.8;
  constexpr int text_scale = 2;
  const Color text_color{.r = 255, .g = 255, .b = 255, .a = 255};

  for (const auto& map_point : source.getVisibleMap(dbu_tile, scale)) {
    if (!map_point.rect.overlaps(dbu_tile)) {
      continue;
    }
    const odb::Rect overlap = map_point.rect.intersect(dbu_tile);
    const odb::Rect draw = toPixels(scale, overlap, dbu_tile);
    const Color color{.r = static_cast<uint8_t>(map_point.color.r),
                      .g = static_cast<uint8_t>(map_point.color.g),
                      .b = static_cast<uint8_t>(map_point.color.b),
                      .a = static_cast<uint8_t>(map_point.color.a)};

    for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        blendPixel(image_buffer, ix, 255 - iy, color);
      }
    }

    if (!source.getShowNumbers() || !map_point.has_value) {
      continue;
    }

    const std::string text = source.formatValue(map_point.value, false);
    const int text_width = getBitmapTextWidth(text, text_scale);
    const int text_height = getBitmapTextHeight(text_scale);
    const double rect_width = map_point.rect.dx() * scale;
    const double rect_height = map_point.rect.dy() * scale;
    if (text_width >= text_rect_margin * rect_width
        || text_height >= text_rect_margin * rect_height) {
      continue;
    }

    const double center_x
        = 0.5 * (map_point.rect.xMin() + map_point.rect.xMax());
    const double center_y
        = 0.5 * (map_point.rect.yMin() + map_point.rect.yMax());
    if (center_x < dbu_tile.xMin() || center_x >= dbu_tile.xMax()
        || center_y < dbu_tile.yMin() || center_y >= dbu_tile.yMax()) {
      continue;
    }

    const int pixel_x = std::lround((center_x - dbu_tile.xMin()) * scale);
    const int pixel_y = 255 - std::lround((center_y - dbu_tile.yMin()) * scale);
    drawBitmapText(image_buffer,
                   pixel_x - text_width / 2,
                   pixel_y - text_height / 2,
                   text,
                   text_scale,
                   text_color);
  }

  std::vector<unsigned char> png_data;
  unsigned error = lodepng::encode(
      png_data, image_buffer, kTileSizeInPixel, kTileSizeInPixel);
  if (error) {
    logger_->report("PNG encoder error: {}", lodepng_error_text(error));
  }
  return png_data;
}

void TileGenerator::drawDebugOverlay(std::vector<unsigned char>& image,
                                     const int z,
                                     const int x,
                                     const int y) const
{
  const Color yellow{.r = 255, .g = 255, .b = 0, .a = 255};
  const int last = kTileSizeInPixel - 1;

  // Draw 1-pixel yellow border
  for (int i = 0; i < kTileSizeInPixel; ++i) {
    setPixel(image, i, 0, yellow);
    setPixel(image, i, last, yellow);
    setPixel(image, 0, i, yellow);
    setPixel(image, last, i, yellow);
  }

  // Build the label string "z=<zoom> <x>/<y>"
  const std::string label = "z=" + std::to_string(z) + " " + std::to_string(x)
                            + "/" + std::to_string(y);

  drawBitmapText(image, 4, 4, label, 3, yellow);
}

/* static */
int TileGenerator::getBitmapTextWidth(const std::string_view text,
                                      const int scale)
{
  if (text.empty()) {
    return 0;
  }

  int width = 0;
  for (const char ch : text) {
    width += getBitmapGlyphAdvance(ch);
  }

  return (width - kBitmapGlyphSpacing) * scale;
}

/* static */
int TileGenerator::getBitmapTextHeight(const int scale)
{
  return kBitmapGlyphHeight * scale;
}

/* static */
void TileGenerator::drawBitmapText(std::vector<unsigned char>& image,
                                   const int x,
                                   const int y,
                                   const std::string_view text,
                                   const int scale,
                                   const Color& color)
{
  int cursor_x = x;
  for (const char ch : text) {
    if (ch == ' ') {
      cursor_x += getBitmapGlyphAdvance(ch) * scale;
      continue;
    }

    const unsigned char* glyph = getBitmapGlyph(ch);
    if (glyph == nullptr) {
      cursor_x += getBitmapGlyphAdvance(ch) * scale;
      continue;
    }

    for (int row = 0; row < kBitmapGlyphHeight; ++row) {
      const unsigned char bits = glyph[row];
      for (int col = 0; col < kBitmapGlyphWidth; ++col) {
        if ((bits & (0x10 >> col)) == 0) {
          continue;
        }
        for (int sy = 0; sy < scale; ++sy) {
          for (int sx = 0; sx < scale; ++sx) {
            blendPixel(image,
                       cursor_x + col * scale + sx,
                       y + row * scale + sy,
                       color);
          }
        }
      }
    }

    cursor_x += getBitmapGlyphAdvance(ch) * scale;
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
  const float src_a = c.a / 255.0f;
  const float dst_a = image[i + 3] / 255.0f;
  const float out_a = src_a + dst_a * (1.0f - src_a);

  if (out_a <= 0.0f) {
    image[i + 0] = 0;
    image[i + 1] = 0;
    image[i + 2] = 0;
    image[i + 3] = 0;
    return;
  }

  const auto blend_channel = [&](const int src, const int dst) {
    const float out = (src * src_a + dst * dst_a * (1.0f - src_a)) / out_a;
    return static_cast<unsigned char>(std::lround(out));
  };

  image[i + 0] = blend_channel(c.r, image[i + 0]);
  image[i + 1] = blend_channel(c.g, image[i + 1]);
  image[i + 2] = blend_channel(c.b, image[i + 2]);
  image[i + 3] = static_cast<unsigned char>(std::lround(out_a * 255.0f));
}

void TileGenerator::drawFilledRect(std::vector<unsigned char>& buffer,
                                   const odb::Rect& rect,
                                   const Color& color) const
{
  for (int iy = rect.yMin(); iy < rect.yMax(); ++iy) {
    for (int ix = rect.xMin(); ix < rect.xMax(); ++ix) {
      const int draw_y = (255 - iy);
      setPixel(buffer, ix, draw_y, color);
    }
  }
}

void TileGenerator::drawHighlight(std::vector<unsigned char>& image,
                                  const std::vector<odb::Rect>& rects,
                                  const std::vector<odb::Polygon>& polys,
                                  const odb::Rect& dbu_tile,
                                  const double scale) const
{
  const Color fill{.r = 255, .g = 255, .b = 0, .a = 30};
  const Color border{.r = 255, .g = 255, .b = 0, .a = 255};

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
    if (full_draw.xMax() > 0 && full_draw.xMax() <= kTileSizeInPixel) {
      const int rx = full_draw.xMax() - 1;
      for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
        setPixel(image, rx, 255 - iy, border);
      }
    }
    if (full_draw.yMin() >= 0 && full_draw.yMin() < kTileSizeInPixel) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        setPixel(image, ix, 255 - full_draw.yMin(), border);
      }
    }
    if (full_draw.yMax() > 0 && full_draw.yMax() <= kTileSizeInPixel) {
      const int ty = full_draw.yMax() - 1;
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        setPixel(image, ix, 255 - ty, border);
      }
    }
  }

  // Polygon highlights (octilinear shapes)
  for (const odb::Polygon& poly : polys) {
    const odb::Rect bbox = poly.getEnclosingRect();
    if (!dbu_tile.overlaps(bbox)) {
      continue;
    }

    // Semi-transparent yellow fill
    fillPolygon(image, poly, dbu_tile, scale, fill, /*blend=*/true);

    // Solid yellow border — draw each edge
    const auto& points = poly.getPoints();
    const int n = static_cast<int>(points.size());
    for (int i = 0; i < n - 1; ++i) {
      const int px0
          = static_cast<int>((points[i].x() - dbu_tile.xMin()) * scale);
      const int py0
          = 255 - static_cast<int>((points[i].y() - dbu_tile.yMin()) * scale);
      const int px1
          = static_cast<int>((points[i + 1].x() - dbu_tile.xMin()) * scale);
      const int py1
          = 255
            - static_cast<int>((points[i + 1].y() - dbu_tile.yMin()) * scale);
      drawLine(image, px0, py0, px1, py1, border);
    }
  }
}

void TileGenerator::drawColoredHighlight(std::vector<unsigned char>& image,
                                         const std::vector<ColoredRect>& rects,
                                         const std::string& current_layer,
                                         const odb::Rect& dbu_tile,
                                         const double scale) const
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
                                    const double scale) const
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

void TileGenerator::drawRouteGuides(std::vector<unsigned char>& image,
                                    const std::set<uint32_t>& net_ids,
                                    const std::string& layer,
                                    const Color& layer_color,
                                    const odb::Rect& dbu_tile,
                                    const double scale) const
{
  odb::dbBlock* block = getBlock();
  if (!block) {
    return;
  }

  const Color fill{
      .r = layer_color.r, .g = layer_color.g, .b = layer_color.b, .a = 50};
  const Color border{
      .r = layer_color.r, .g = layer_color.g, .b = layer_color.b, .a = 180};

  for (const uint32_t net_id : net_ids) {
    odb::dbNet* net = odb::dbNet::getNet(block, net_id);
    if (!net) {
      continue;
    }
    for (odb::dbGuide* guide : net->getGuides()) {
      if (guide->getLayer()->getName() != layer) {
        continue;
      }
      const odb::Rect box = guide->getBox();
      if (!dbu_tile.overlaps(box)) {
        continue;
      }
      const odb::Rect overlap = box.intersect(dbu_tile);
      const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

      // Semi-transparent fill
      for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
        for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
          blendPixel(image, ix, 255 - iy, fill);
        }
      }

      // Border (only where guide edge is within this tile)
      const odb::Rect full_draw = toPixels(scale, box, dbu_tile);
      if (full_draw.xMin() >= 0 && full_draw.xMin() < kTileSizeInPixel) {
        for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
          blendPixel(image, full_draw.xMin(), 255 - iy, border);
        }
      }
      if (full_draw.xMax() > 0 && full_draw.xMax() <= kTileSizeInPixel) {
        const int rx = full_draw.xMax() - 1;
        for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
          blendPixel(image, rx, 255 - iy, border);
        }
      }
      if (full_draw.yMin() >= 0 && full_draw.yMin() < kTileSizeInPixel) {
        for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
          blendPixel(image, ix, 255 - full_draw.yMin(), border);
        }
      }
      if (full_draw.yMax() > 0 && full_draw.yMax() <= kTileSizeInPixel) {
        const int ty = full_draw.yMax() - 1;
        for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
          blendPixel(image, ix, 255 - ty, border);
        }
      }
    }
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

static odb::dbNet* getNetFromPin(odb::dbITerm* iterm, odb::dbBTerm* bterm)
{
  if (iterm) {
    return iterm->getNet();
  }
  if (bterm) {
    return bterm->getNet();
  }
  return nullptr;
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
  static const Color kLaunchClkColor{
      .r = 0, .g = 255, .b = 255, .a = 180};                            // Cyan
  static const Color kSignalColor{.r = 255, .g = 0, .b = 0, .a = 180};  // Red
  static const Color kCaptureClkColor{
      .r = 0, .g = 255, .b = 0, .a = 180};  // Green

  // Track nets already collected to avoid duplicates
  std::set<odb::dbNet*> seen_nets;

  auto processNodes = [&](const std::vector<TimingNode>& nodes,
                          const Color& clk_color,
                          const Color& data_color) {
    for (size_t i = 0; i + 1 < nodes.size(); i++) {
      auto [a_iterm, a_bterm] = resolvePin(block, nodes[i].pin_name);
      auto [b_iterm, b_bterm] = resolvePin(block, nodes[i + 1].pin_name);

      odb::dbNet* net_a = getNetFromPin(a_iterm, a_bterm);
      odb::dbNet* net_b = getNetFromPin(b_iterm, b_bterm);

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
