// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "DensityFill.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "boost/lexical_cast.hpp"
#include "boost/polygon/polygon.hpp"
#include "graphics.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "polygon.h"
#include "utl/Logger.h"

namespace fin {

using utl::FIN;

namespace pt = boost::property_tree;

using odb::dbBlock;
using odb::dbChip;
using odb::dbDatabase;
using odb::dbFill;
using odb::dbInstShapeItr;
using odb::dbShape;
using odb::dbTech;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbTechVia;
using odb::dbVia;
using odb::dbWire;
using odb::dbWireShapeItr;
using odb::Rect;

// The rules for OPC or non-OPC shapes on a layer from the JSON config
struct DensityFillShapesConfig
{
  // width & height of fill shapes to try (in order)
  std::vector<std::pair<int, int>> shapes;
  int space_to_fill;
  int space_to_non_fill;
  int space_line_end;
};

// The rules for a layer from the JSON config
struct DensityFillLayerConfig
{
  int space_to_outline;
  int num_masks;
  int opc_halo;
  bool has_opc;

  DensityFillShapesConfig opc;
  DensityFillShapesConfig non_opc;
};

// Make a boost polygon representing a rectangle
static Polygon90 makeRect(int x_lo, int y_lo, int x_hi, int y_hi)
{
  using Pt = Polygon90::point_type;
  std::array<Pt, 4> pts
      = {Pt(x_lo, y_lo), Pt(x_hi, y_lo), Pt(x_hi, y_hi), Pt(x_lo, y_hi)};

  Polygon90 poly;
  poly.set(pts.begin(), pts.end());
  return poly;
}

static double getValue(pt::ptree& tree)
{
  return boost::lexical_cast<double>(tree.data());
}

static double getValue(const char* key, pt::ptree& tree)
{
  return getValue(tree.get_child(key));
}

////////////////////////////////////////////////////////////////

DensityFill::DensityFill(dbDatabase* db, utl::Logger* logger, bool debug)
    : db_(db), logger_(logger)
{
  if (debug && Graphics::guiActive()) {
    graphics_ = std::make_unique<Graphics>();
  }
}

// must be in the .cpp due to forward decl
DensityFill::~DensityFill() = default;

// Converts the user's JSON configuration file in per layer
// DensityFillLayerConfig objects.
//
// In the configuration you can group layers together that have
// similar rules.  This method expands such groupings into per layer
// values.  It also translates from microns to DBU and layer names
// to dbTechLayer*.
void DensityFill::readAndExpandLayers(dbTech* tech, pt::ptree& tree)
{
  int dbu = tech->getDbUnitsPerMicron();

  auto& layers = tree.get_child("layers");
  for (auto& [name, layer] : layers) {
    DensityFillLayerConfig cfg;
    cfg.space_to_outline = getValue("space_to_outline", layer) * dbu;

    // non-OPC data
    {
      auto& non_opc = layer.get_child("non-opc");
      auto& scfg = cfg.non_opc;
      scfg.space_to_fill = getValue("space_to_fill", non_opc) * dbu;
      scfg.space_to_non_fill = getValue("space_to_non_fill", non_opc) * dbu;
      scfg.space_line_end = 0;  // n/a for non-OPC
      cfg.num_masks = non_opc.get_child("datatype").size();

      auto widths = non_opc.get_child("width");
      auto heights = non_opc.get_child("height");

      std::ranges::transform(widths,
                             heights,
                             std::back_inserter(scfg.shapes),
                             [dbu](auto& w, auto& h) {
                               return std::make_pair(getValue(w.second) * dbu,
                                                     getValue(h.second) * dbu);
                             });
    }

    // OPC data, if any
    auto opc_it = layer.find("opc");
    cfg.has_opc = opc_it != layer.not_found();
    if (cfg.has_opc) {
      auto& opc = layer.get_child("opc");
      auto& scfg = cfg.opc;
      cfg.opc_halo = getValue("halo", opc) * dbu;
      scfg.space_to_fill = getValue("space_to_fill", opc) * dbu;
      scfg.space_to_non_fill = getValue("space_to_non_fill", opc) * dbu;
      if (opc.find("space_line_end") != opc.not_found()) {
        scfg.space_line_end = getValue("space_line_end", opc) * dbu;
      } else {
        scfg.space_line_end = 0;
      }

      auto widths = opc.get_child("width");
      auto heights = opc.get_child("height");

      std::ranges::transform(widths,
                             heights,
                             std::back_inserter(scfg.shapes),
                             [dbu](auto& w, auto& h) {
                               return std::make_pair(getValue(w.second) * dbu,
                                                     getValue(h.second) * dbu);
                             });
    }

    auto it = layer.find("names");
    if (it != layer.not_found()) {
      // Expand names
      for (auto& [name, layer_name] : layer.get_child("names")) {
        auto tech_layer = tech->findLayer(layer_name.data().c_str());
        if (!tech_layer) {
          logger_->error(FIN,
                         1,
                         "Layer {} in names was not found.",
                         layer.get_child("name").data());
        }
        layers_[tech_layer] = cfg;
      }
    } else {
      // No expansion, just a single layer
      auto tech_layer = tech->findLayer(layer.get_child("name").data().c_str());
      if (!tech_layer) {
        logger_->error(
            FIN, 2, "Layer {} not found.", layer.get_child("name").data());
      }
      layers_[tech_layer] = std::move(cfg);
    }
  }
}

void DensityFill::loadConfig(const char* cfg_filename, dbTech* tech)
{
  // Read the json config file using Boost's property_tree
  pt::ptree tree;
  pt::json_parser::read_json(cfg_filename, tree);
  readAndExpandLayers(tech, tree);
}

// Insert into polygon_set any part of given shape on the given layer (shape may
// be a via)
static void insertShape(const dbShape& shape,
                        Polygon90Set& polygon_set,
                        dbTechLayer* layer)
{
  auto type = shape.getType();
  switch (type) {
    case dbShape::VIA:
    case dbShape::TECH_VIA: {
      dbTechLayer* top;
      dbTechLayer* bottom;
      if (type == dbShape::VIA) {
        dbVia* via = shape.getVia();
        top = via->getTopLayer();
        bottom = via->getBottomLayer();
      } else {
        dbTechVia* via = shape.getTechVia();
        top = via->getTopLayer();
        bottom = via->getBottomLayer();
      }

      if (top != layer && bottom != layer) {
        return;
      }
      std::vector<dbShape> boxes;
      dbShape::getViaBoxes(shape, boxes);
      for (auto& box : boxes) {
        if (box.getTechLayer() == layer) {
          polygon_set.insert(
              makeRect(box.xMin(), box.yMin(), box.xMax(), box.yMax()));
        }
      }
      break;
    }
    case dbShape::SEGMENT:
      if (shape.getTechLayer() == layer) {
        polygon_set.insert(
            makeRect(shape.xMin(), shape.yMin(), shape.xMax(), shape.yMax()));
      }
      break;
    case dbShape::TECH_VIA_BOX:
    case dbShape::VIA_BOX:
      if (shape.getTechLayer() == layer) {
        polygon_set.insert(
            makeRect(shape.xMin(), shape.yMin(), shape.xMax(), shape.yMax()));
      }
      break;
  }
}

// Build a polygon set out of all the non-fill shape on the given layer
// including wires, special wires, and instances' pins & OBS
static Polygon90Set orNonFills(dbBlock* block, dbTechLayer* layer)
{
  Polygon90Set non_fill;  // The result
  dbShape shape;          // Shared temp

  // Get shapes from regular wires
  dbWireShapeItr shapes;
  for (auto net : block->getNets()) {
    dbWire* wire = net->getWire();
    if (!wire) {
      continue;
    }
    for (shapes.begin(wire); shapes.next(shape);) {
      insertShape(shape, non_fill, layer);
    }
  }

  // Get shapes from special wires
  std::vector<dbShape> via_shapes;
  for (auto net : block->getNets()) {
    for (auto swire : net->getSWires()) {
      for (auto sbox : swire->getWires()) {
        if (sbox->isVia()) {
          dbVia* via = sbox->getBlockVia();
          Rect rect = sbox->getBox();
          shape.setVia(via, rect);
          dbShape::getViaBoxes(shape, via_shapes);
          for (auto& via_shape : via_shapes) {
            insertShape(via_shape, non_fill, layer);
          }
        } else if (sbox->getTechLayer() == layer) {
          non_fill.insert(
              makeRect(sbox->xMin(), sbox->yMin(), sbox->xMax(), sbox->yMax()));
        }
      }
    }
  }

  // Get shapes from instances
  dbInstShapeItr insts(/* expand_vias */ false);
  for (auto inst : block->getInsts()) {
    for (insts.begin(inst, dbInstShapeItr::ALL); insts.next(shape);) {
      insertShape(shape, non_fill, layer);
    }
  }

  return non_fill;
}

static std::pair<int, int> getSpacing(dbTechLayer* layer,
                                      const DensityFillShapesConfig& cfg)
{
  bool is_horiz = layer->getDirection() == dbTechLayerDir::HORIZONTAL;
  int space_x = cfg.space_to_fill;
  int space_y = space_x;
  if (is_horiz) {
    space_x = std::max(space_x, cfg.space_line_end);
  } else {
    space_y = std::max(space_y, cfg.space_line_end);
  }

  return std::make_pair(space_x, space_y);
}

// Two different polygons might be less than min space apart and this
// can lead to DRVs when they are filled independently.  To avoid this
// we exclude a min-space area around each polygon.  This is somewhat
// conservative as we may not actually put a fill where a DRV would be
// caused but is much faster than updating the fill area after every
// polygon is filled.
static void prune(Polygon90Set& fill_area,
                  dbTechLayer* layer,
                  const DensityFillShapesConfig& cfg,
                  Graphics* graphics)
{
  auto [space_x, space_y] = getSpacing(layer, cfg);

  // From Boost on grow_and:
  //   Same as bloating non-overlapping regions and then applying self
  //   intersect to retain only the overlaps introduced by the bloat.
  Polygon90Set pruned(fill_area);
  grow_and(pruned, space_x, space_x, space_y, space_y);
  fill_area -= pruned;
}

// Fill a polygon (area) on the given layer using the given configuration.
// Num_masks is used to color the generated fills.
// filled_area, if given, is an OR of the generated fills without bloating
static void fillPolygon(const Polygon90& area,
                        dbTechLayer* layer,
                        dbBlock* block,
                        const DensityFillShapesConfig& cfg,
                        int num_masks,
                        bool needs_opc,
                        Graphics* graphics,
                        Polygon90Set* filled_area = nullptr)
{
  // Convert the area polygon to a polygon set as we will remove areas
  // filled by one fill shape from consideration by future shapes,
  // which may result in the polygon breaking apart into a set of
  // remaining polygons
  Polygon90Set fill_area;
  fill_area += area;

  bool is_horiz = layer->getDirection() == dbTechLayerDir::HORIZONTAL;
  auto [space_x, space_y] = getSpacing(layer, cfg);

  auto iter = cfg.shapes.begin();
  while (iter != cfg.shapes.end()) {
    auto [w, h] = *iter++;
    // Ensure the longer direction is in the preferred direction
    if ((is_horiz && w < h) || (!is_horiz && h < w)) {
      std::swap(w, h);
    }

    // Use a shrink/bloat cycle to remove any areas that are too small to fill
    // with this fill shape.  A benefit is that it helps break up big polygons
    // which makes it easier to fill them
    Polygon90Set pruned_fill_area = fill_area;

    int ew_sizing = w / 2 - 1;
    int ns_sizing = h / 2 - 1;
    shrink(pruned_fill_area, ew_sizing, ew_sizing, ns_sizing, ns_sizing);
    bloat(pruned_fill_area, ew_sizing, ew_sizing, ns_sizing, ns_sizing);

    // The polygon may break into parts that could be less than min-space
    // apart so prune the result.
    prune(pruned_fill_area, layer, cfg, graphics);

    if (graphics) {
      graphics->status("Fill Area for " + std::to_string(w) + " "
                       + std::to_string(h));
      graphics->drawPolygon90Set(pruned_fill_area);
    }

    Polygon90Set all_iter_fills;
    std::vector<Polygon90> sub_fill_areas;
    pruned_fill_area.get(sub_fill_areas);
    for (auto& sub_fill_area : sub_fill_areas) {
      Rectangle bounds;
      extents(bounds, sub_fill_area);

      // Tile a set of fills to cover the bounds.  (KLayout allows a
      // sweep on the origin of the tile set looking for maximum fill.
      // We could try that in the future.)
      Polygon90Set all_fills;
      for (int x = xl(bounds); x < xh(bounds); x += w + space_x) {
        for (int y = yl(bounds); y < yh(bounds); y += h + space_y) {
          all_fills.insert(makeRect(x, y, x + w, y + h));
        }
      }

      // Intersect fills with the sub area and keep only whole fill shapes
      Polygon90Set fills = all_fills & sub_fill_area;
      keep(fills, w * h, w * h, w - 1, w, h - 1, h);

      Polygon90Set tmp_fills(fills);
      all_iter_fills += bloat(tmp_fills, space_x, space_x, space_y, space_y);

      // Insert fills into the db
      std::vector<Rectangle> polygons;
      fills.get_rectangles(polygons);
      const int num_mask = std::max(num_masks, 1);
      int cnt = 0;
      for (auto& f : polygons) {
        int mask;
        if (num_mask == 1) {
          mask = 0;  // don't write a mask for single mask layers
        } else {
          mask = cnt++ % num_mask + 1;
        }
        auto x_lo = xl(f);
        auto y_lo = yl(f);
        auto x_hi = xh(f);
        auto y_hi = yh(f);
        dbFill::create(block, needs_opc, mask, layer, x_lo, y_lo, x_hi, y_hi);
        if (filled_area) {
          *filled_area += makeRect(x_lo, y_lo, x_hi, y_hi);
        }
      }
    }
    // Remove filled area from use by future shapes
    fill_area -= all_iter_fills;
  }
}

// Fill the given layer
void DensityFill::fillLayer(dbBlock* block,
                            dbTechLayer* layer,
                            const odb::Rect& fill_bounds_rect)
{
  logger_->info(FIN, 3, "Filling layer {}.", layer->getConstName());

  Polygon90Set non_fill = orNonFills(block, layer);

  auto fill_bounds = makeRect(fill_bounds_rect.xMin(),
                              fill_bounds_rect.yMin(),
                              fill_bounds_rect.xMax(),
                              fill_bounds_rect.yMax());

  const DensityFillLayerConfig& cfg = layers_[layer];

  std::vector<Polygon90> polygons;

  // Do non-OPC fill
  Polygon90Set fill_area
      = fill_bounds - (non_fill + cfg.non_opc.space_to_non_fill);

  if (graphics_) {
    graphics_->status("Non-OPC Area");
    graphics_->drawPolygon90Set(fill_area);
  }

  prune(fill_area, layer, cfg.non_opc, graphics_.get());

  fill_area.get(polygons);
  logger_->info(FIN, 9, "Filling {} areas with non-OPC fill.", polygons.size());

  Polygon90Set non_opc_fill_area;
  for (auto& polygon : polygons) {
    fillPolygon(polygon,
                layer,
                block,
                cfg.non_opc,
                cfg.num_masks,
                false,
                graphics_.get(),
                &non_opc_fill_area);
  }
  logger_->info(FIN, 4, "Total fills: {}.", block->getFills().size());

  if (!cfg.has_opc) {
    return;
  }

  Polygon90Set opc_fill_area
      = fill_bounds - (non_fill + cfg.opc.space_to_non_fill)
        - (non_opc_fill_area + cfg.non_opc.space_to_fill);

  if (graphics_) {
    graphics_->status("OPC Area");
    graphics_->drawPolygon90Set(opc_fill_area);
  }

  prune(opc_fill_area, layer, cfg.opc, graphics_.get());

  polygons.clear();
  opc_fill_area.get(polygons);
  logger_->info(FIN, 5, "Filling {} areas with OPC fill.", polygons.size());
  for (auto& polygon : polygons) {
    fillPolygon(
        polygon, layer, block, cfg.opc, cfg.num_masks, true, graphics_.get());
  }

  logger_->info(FIN, 6, "Total fills: {}.", block->getFills().size());

  if (graphics_) {
    graphics_->status("OPC Area");
    graphics_->drawPolygon90Set(opc_fill_area);
  }
}

// Fill the design according to the given cfg file
void DensityFill::fill(const char* cfg_filename, const odb::Rect& fill_area)
{
  dbTech* tech = db_->getTech();
  loadConfig(cfg_filename, tech);

  dbChip* chip = db_->getChip();
  dbBlock* block = chip->getBlock();

  for (dbTechLayer* layer : tech->getLayers()) {
    auto it = layers_.find(layer);
    if (it == layers_.end()) {
      logger_->warn(FIN, 10, "Skipping layer {}.", layer->getConstName());
      continue;
    }
    fillLayer(block, layer, fill_area);
  }
}

}  // namespace fin
