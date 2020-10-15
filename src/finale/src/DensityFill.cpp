/////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, OpenRoad Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/polygon/polygon.hpp>

#include "DensityFill.h"
#include "opendb/dbShape.h"
#include "openroad/Error.hh"

namespace finale {

using namespace boost::polygon::operators;

using Rectangle = boost::polygon::rectangle_data<int>;
using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
using Polygon90Set = boost::polygon::polygon_90_set_data<int>;

namespace pt = boost::property_tree;

using namespace odb;

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
static Polygon90 makeRect(int xLo, int yLo, int xHi, int yHi)
{
  using Pt = Polygon90::point_type;
  std::array<Pt, 4> pts
      = {Pt(xLo, yLo), Pt(xHi, yLo), Pt(xHi, yHi), Pt(xLo, yHi)};

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

DensityFill::DensityFill(dbDatabase* db) : db_(db)
{
}

DensityFill::~DensityFill()  // must be in the .cpp due to forward decl
{
}

// Converts the user's JSON configuration file in per layer
// DensityFillLayerConfig objects.
//
// In the configuration you can group layers together that have
// similar rules.  This method expands such groupings into per layer
// values.  It also translates from microns to DBU and layer names
// to dbTechLayer*.
void DensityFill::read_and_expand_layers(dbTech* tech,
                                         boost::property_tree::ptree& tree)
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

      std::transform(widths.begin(),
                     widths.end(),
                     heights.begin(),
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

      std::transform(widths.begin(),
                     widths.end(),
                     heights.begin(),
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
          ord::error("Layer %s not found",
                     layer.get_child("name").data().c_str());
        }
        layers_[tech_layer] = cfg;
      }
    } else {
      // No expansion, just a single layer
      auto tech_layer = tech->findLayer(layer.get_child("name").data().c_str());
      if (!tech_layer) {
        ord::error("Layer %s not found",
                   layer.get_child("name").data().c_str());
      }
      layers_[tech_layer] = cfg;
    }
  }
}

void DensityFill::loadConfig(const char* cfg_filename, dbTech* tech)
{
  // Read the json config file using Boost's property_tree
  boost::property_tree::ptree tree;
  pt::json_parser::read_json(cfg_filename, tree);
  read_and_expand_layers(tech, tree);
}

// Insert into polygon_set any part of given shape on the given layer (shape may
// be a via)
static void insert_shape(const dbShape& shape,
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
static Polygon90Set or_non_fills(dbBlock* block, dbTechLayer* layer)
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
      insert_shape(shape, non_fill, layer);
    }
  }

  // Get shapes from special wires
  std::vector<dbShape> via_shapes;
  for (auto net : block->getNets()) {
    for (auto swire : net->getSWires()) {
      for (auto sbox : swire->getWires()) {
        if (sbox->isVia()) {
          dbVia* via = sbox->getBlockVia();
          Rect rect;
          sbox->getBox(rect);
          shape.setVia(via, rect);
          dbShape::getViaBoxes(shape, via_shapes);
          for (auto& via_shape : via_shapes) {
            insert_shape(via_shape, non_fill, layer);
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
      insert_shape(shape, non_fill, layer);
    }
  }

  return non_fill;
}

// Fill a polygon (area) on the given layer using the given configuration.
// Num_masks is used to color the generated fills.
// filled_area, if given, is an OR of the generated fills without bloating
static void fill_polygon(const Polygon90& area,
                         dbTechLayer* layer,
                         dbBlock* block,
                         const DensityFillShapesConfig& cfg,
                         int num_masks,
                         bool needs_opc,
                         Polygon90Set* filled_area = nullptr)
{
  // Convert the area polygon to a polygon set as we will remove areas
  // filled by one fill shape from consideration by future shapes,
  // which may result in the polygon breaking apart into a set of
  // remaining polygons
  Polygon90Set fill_area;
  fill_area += area;

  bool isH = layer->getDirection() == dbTechLayerDir::HORIZONTAL;
  int space_x = cfg.space_to_fill;
  int space_y = space_x;
  if (isH) {
    space_x = std::max(space_x, cfg.space_line_end);
  } else {
    space_y = std::max(space_y, cfg.space_line_end);
  }

  auto iter = cfg.shapes.begin();
  while (iter != cfg.shapes.end()) {
    auto [w, h] = *iter;
    bool last_shape = ++iter == cfg.shapes.end();
    // Ensure the longer direction is in the preferred direction
    if (isH && w < h || !isH && h < w) {
      std::swap(w, h);
    }

    std::vector<Polygon90> sub_fill_areas;
    fill_area.get(sub_fill_areas);
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

      // Remove filled area from use by future shapes
      if (!last_shape) {
        Polygon90Set tmp_fills(fills);
        fill_area -= bloat(tmp_fills, space_x, space_x, space_y, space_y);
      }

      // Insert fills into the db
      std::vector<Rectangle> polygons;
      fills.get_rectangles(polygons);
      const int num_mask = std::max(num_masks, 1);
      int cnt = 0;
      for (auto& f : polygons) {
        int mask = cnt++ % num_mask + 1;
        auto xLo = xl(f);
        auto yLo = yl(f);
        auto xHi = xh(f);
        auto yHi = yh(f);
        dbFill::create(block, needs_opc, mask, layer, xLo, yLo, xHi, yHi);
        if (filled_area) {
          *filled_area += makeRect(xLo, yLo, xHi, yHi);
        }
      }
    }
  }
}

// Fill the given layer
void DensityFill::fill_layer(dbBlock* block, dbTechLayer* layer)
{
  printf("Filling %s...\n", layer->getConstName());

  Polygon90Set non_fill = or_non_fills(block, layer);

  // We fill only the core area
  Rect core_area_rect;
  block->getCoreArea(core_area_rect);
  auto core_area = makeRect(core_area_rect.xMin(),
                            core_area_rect.yMin(),
                            core_area_rect.xMax(),
                            core_area_rect.yMax());

  const DensityFillLayerConfig& cfg = layers_[layer];

  std::vector<Polygon90> polygons;
  Polygon90Set opc_fill_area;

  // Do OPC fill
  if (cfg.has_opc) {
    Polygon90Set halo_area
        = (non_fill + cfg.opc_halo)
          & core_area - (non_fill + cfg.opc.space_to_non_fill);
    halo_area.get(polygons);
    printf("  Filling %d areas with OPC fill...\n", polygons.size());
    for (auto& polygon : polygons) {
      fill_polygon(
                   polygon, layer, block, cfg.opc, cfg.num_masks, true, &opc_fill_area);
    }
    polygons.clear();
  }

  // Do non-OPC fill
  Polygon90Set fill_area
      = core_area - (non_fill + cfg.non_opc.space_to_non_fill);
  if (cfg.has_opc) {
    fill_area -= (opc_fill_area + cfg.non_opc.space_to_fill);
  }
  fill_area.get(polygons);
  printf("  Filling %d areas with non-OPC fill...\n", polygons.size());
  for (auto& polygon : polygons) {
    fill_polygon(polygon, layer, block, cfg.non_opc, cfg.num_masks, false);
  }
}

// Fill the design according to the given cfg file
void DensityFill::fill(const char* cfg_filename)
{
  dbTech* tech = db_->getTech();
  loadConfig(cfg_filename, tech);

  dbChip* chip = db_->getChip();
  dbBlock* block = chip->getBlock();

  for (dbTechLayer* layer : tech->getLayers()) {
    auto it = layers_.find(layer);
    if (it == layers_.end()) {
      printf("skip layer %s\n", layer->getConstName());
      continue;
    }
    fill_layer(block, layer);
  }
}

}  // namespace finale
