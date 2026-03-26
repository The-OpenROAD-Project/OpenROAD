// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/lefout.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"
#include "spdlog/fmt/ostr.h"
#include "utl/scope.h"

namespace odb {

int lefout::determineBloat(dbTechLayer* layer) const
{
  int bloat = 0;

  const int pitch = layer->getPitch();
  if (pitch != 0) {
    bloat = pitch;
  } else {
    bloat = 2 * layer->getSpacing();
  }
  return bloat_factor_ * bloat;
}

void lefout::insertObstruction(dbBox* box, ObstructionMap& obstructions) const
{
  insertObstruction(box->getTechLayer(), box->getBox(), obstructions);
}

void lefout::insertObstruction(dbTechLayer* layer,
                               const Rect& rect,
                               ObstructionMap& obstructions) const
{
  if (layer->getType() == odb::dbTechLayerType::CUT) {
    return;
  }

  const int bloat = determineBloat(layer);
  boost::polygon::polygon_90_set_data<int> poly;
  poly = boost::polygon::rectangle_data<int>{
      rect.xMax(), rect.yMax(), rect.xMin(), rect.yMin()};
  using boost::polygon::operators::operator+=;
  obstructions[layer] += poly.bloat(bloat, bloat, bloat, bloat);
}

void lefout::writeVersion(std::ostream& out, const std::string& version)
{
  fmt::print(out, "VERSION {} ;\n", version);
}

template <typename GenericBox>
std::set<dbVia*> lefout::writeBoxes(std::ostream& out,
                                    dbBlock* block,
                                    dbSet<GenericBox>& boxes,
                                    const char* indent)
{
  dbTechLayer* cur_layer = nullptr;
  std::set<dbVia*> vias;

  for (GenericBox* generic_box : boxes) {
    if (generic_box == nullptr) {
      continue;
    }

    dbBox* box = generic_box;
    dbTechLayer* layer = box->getTechLayer();

    // Checks if the box is either a tech via or a block via.
    if (box->getTechVia() || box->getBlockVia()) {
      std::string via_name;
      if (box->getTechVia()) {
        via_name = box->getTechVia()->getName();
      }
      if (box->getBlockVia()) {
        via_name = block->getName() + "_" + box->getBlockVia()->getName();
        vias.insert(box->getBlockVia());
      }

      const Point pt = box->getViaXY();
      fmt::print(out,
                 "{}VIA {:.11g} {:.11g} {} ;\n",
                 indent,
                 lefdist(pt.getX()),
                 lefdist(pt.getY()),
                 via_name.c_str());
      cur_layer = nullptr;
    } else {
      std::string layer_name;
      if (use_alias_ && layer->hasAlias()) {
        layer_name = layer->getAlias();
      } else {
        layer_name = layer->getName();
      }

      if (cur_layer != layer) {
        fmt::print(out, "{}LAYER {} ;\n", indent, layer_name.c_str());
        cur_layer = layer;
      }

      writeBox(out, indent, box);
    }
  }

  return vias;
}

template <>
std::set<dbVia*> lefout::writeBoxes(std::ostream& out,
                                    dbBlock* block,
                                    dbSet<dbPolygon>& boxes,
                                    const char* indent)
{
  dbTechLayer* cur_layer = nullptr;

  for (dbPolygon* box : boxes) {
    if (box == nullptr) {
      continue;
    }

    dbTechLayer* layer = box->getTechLayer();

    std::string layer_name;
    if (use_alias_ && layer->hasAlias()) {
      layer_name = layer->getAlias();
    } else {
      layer_name = layer->getName();
    }

    if (cur_layer != layer) {
      fmt::print(out, "{}LAYER {} ;\n", indent, layer_name.c_str());
      cur_layer = layer;
    }

    writePolygon(out, indent, box);
  }

  return {};
}

void lefout::writeBox(std::ostream& out, const std::string& indent, dbBox* box)
{
  int x1 = box->xMin();
  int y1 = box->yMin();
  int x2 = box->xMax();
  int y2 = box->yMax();

  fmt::print(out,
             "{}  RECT  {:.11g} {:.11g} {:.11g} {:.11g} ;\n",
             indent.c_str(),
             lefdist(x1),
             lefdist(y1),
             lefdist(x2),
             lefdist(y2));
}

void lefout::writePolygon(std::ostream& out,
                          const std::string& indent,
                          dbPolygon* polygon)
{
  fmt::print(out, "{}  POLYGON  ", indent.c_str());

  for (const Point& pt : polygon->getPolygon().getPoints()) {
    int x = pt.x();
    int y = pt.y();
    fmt::print(out, "{:.11g} {:.11g} ", lefdist(x), lefdist(y));
  }

  fmt::print(out, ";\n");
}

void lefout::writeRect(std::ostream& out,
                       const std::string& indent,
                       const Rect& rect)
{
  fmt::print(out,
             "{}  RECT  {:.11g} {:.11g} {:.11g} {:.11g} ;\n",
             indent.c_str(),
             lefdist(rect.xMin()),
             lefdist(rect.yMin()),
             lefdist(rect.xMax()),
             lefdist(rect.yMax()));
}

void lefout::writeHeader(std::ostream& out, dbBlock* db_block)
{
  char left_bus_delimiter = 0;
  char right_bus_delimiter = 0;
  char hier_delimiter = db_block->getHierarchyDelimiter();

  db_block->getBusDelimiters(left_bus_delimiter, right_bus_delimiter);

  if (left_bus_delimiter == 0) {
    left_bus_delimiter = '[';
  }

  if (right_bus_delimiter == 0) {
    right_bus_delimiter = ']';
  }

  if (hier_delimiter == 0) {
    hier_delimiter = '|';
  }

  writeVersion(out, "5.8");
  writeBusBitChars(out, left_bus_delimiter, right_bus_delimiter);
  writeDividerChar(out, hier_delimiter);
  writeUnits(out, /*database_units = */ db_block->getDbUnitsPerMicron());
}

void lefout::writeObstructions(std::ostream& out, dbBlock* db_block)
{
  ObstructionMap obstructions;
  getObstructions(db_block, obstructions);

  fmt::print(out, "{}", "  OBS\n");
  dbBox* block_bounding_box = db_block->getBBox();
  for (const auto& [tech_layer, polySet] : obstructions) {
    fmt::print(out, "    LAYER {} ;\n", tech_layer->getName().c_str());

    if (bloat_occupied_layers_) {
      writeBox(out, "   ", block_bounding_box);
    } else {
      const int bloat = determineBloat(tech_layer);
      boost::polygon::polygon_90_set_data<int> shrink_poly = polySet;
      shrink_poly.shrink2(bloat, bloat, bloat, bloat);

      // Decompose the polygon set to rectanges in non-preferred direction
      std::vector<Rect> rects;
      if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
        shrink_poly.get_rectangles(rects, boost::polygon::VERTICAL);
      } else if (tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
        shrink_poly.get_rectangles(rects, boost::polygon::HORIZONTAL);
      } else if (tech_layer->getDirection() == odb::dbTechLayerDir::NONE) {
        shrink_poly.get_rectangles(rects);
      }

      for (const auto& rect : rects) {
        writeRect(out, "   ", rect);
      }
    }
  }
  fmt::print(out, "  END\n");
}

void lefout::getObstructions(dbBlock* db_block,
                             ObstructionMap& obstructions) const
{
  for (dbObstruction* obs : db_block->getObstructions()) {
    insertObstruction(obs->getBBox(), obstructions);
  }

  findInstsObstructions(obstructions, db_block);

  for (dbNet* net : db_block->getNets()) {
    findSWireLayerObstructions(obstructions, net);
    findWireLayerObstructions(obstructions, net);
  }
}

void lefout::findInstsObstructions(ObstructionMap& obstructions,
                                   dbBlock* db_block) const
{  // Find all insts obsturctions and Iterms

  for (auto* inst : db_block->getInsts()) {
    const dbTransform trans = inst->getTransform();

    // Add insts obstructions
    for (auto* obs : inst->getMaster()->getObstructions()) {
      Rect obs_rect = obs->getBox();
      trans.apply(obs_rect);
      insertObstruction(obs->getTechLayer(), obs_rect, obstructions);
    }

    // Add inst Iterms to obstructions
    for (auto* iterm : inst->getITerms()) {
      dbShape shape;
      dbITermShapeItr iterm_shape_itr(/* expand_vias */ true);
      for (iterm_shape_itr.begin(iterm); iterm_shape_itr.next(shape);) {
        insertObstruction(shape.getTechLayer(), shape.getBox(), obstructions);
      }
    }
  }
}

void lefout::findSWireLayerObstructions(ObstructionMap& obstructions,
                                        dbNet* net) const
{  // Find all layers where an swire exists
  for (dbSWire* swire : net->getSWires()) {
    for (dbSBox* box : swire->getWires()) {
      if (box->isVia()) {
        // In certain power grid arrangements there may be a metal layer that
        // isn't directly used for straps or stripes just punching vias through.
        // In these cases the metal layer should still be blocked even though
        // we can't find any metal wires on the layer.
        // https://github.com/The-OpenROAD-Project/OpenROAD/pull/725#discussion_r669927312
        findLayerViaObstructions(obstructions, box);
      } else {
        insertObstruction(box, obstructions);
      }
    }
  }
}

void lefout::findLayerViaObstructions(ObstructionMap& obstructions,
                                      dbSBox* box) const
{
  std::vector<dbShape> via_shapes;
  box->getViaBoxes(via_shapes);
  for (dbShape db_shape : via_shapes) {
    if (db_shape.getTechLayer() == nullptr) {
      continue;
    }
    insertObstruction(db_shape.getTechLayer(), db_shape.getBox(), obstructions);
  }
}

void lefout::findWireLayerObstructions(ObstructionMap& obstructions,
                                       dbNet* net) const
{
  // Find all metal layers where a wire exists.
  dbWire* wire = net->getWire();

  if (wire == nullptr) {
    return;
  }

  dbWireShapeItr wire_shape_itr;
  dbShape shape;

  for (wire_shape_itr.begin(wire); wire_shape_itr.next(shape);) {
    if (shape.isVia()) {
      std::vector<dbShape> via_shapes;
      dbShape::getViaBoxes(shape, via_shapes);
      for (dbShape db_shape : via_shapes) {
        if (db_shape.getTechLayer() == nullptr) {
          continue;
        }
        insertObstruction(
            db_shape.getTechLayer(), db_shape.getBox(), obstructions);
      }
    } else {
      insertObstruction(shape.getTechLayer(), shape.getBox(), obstructions);
    }
  }
}

void lefout::writeHeader(std::ostream& out, dbLib* lib)
{
  dbTech* tech = lib->getTech();

  char left_bus_delimiter = 0;
  char right_bus_delimiter = 0;
  char hier_delimiter = lib->getHierarchyDelimiter();

  lib->getBusDelimiters(left_bus_delimiter, right_bus_delimiter);

  if (left_bus_delimiter == 0) {
    left_bus_delimiter = '[';
  }

  if (right_bus_delimiter == 0) {
    right_bus_delimiter = ']';
  }

  if (hier_delimiter == 0) {
    hier_delimiter = '|';
  }

  writeVersion(out, tech->getLefVersionStr());
  writeNameCaseSensitive(out, tech->getNamesCaseSensitive());
  writeBusBitChars(out, left_bus_delimiter, right_bus_delimiter);
  writeDividerChar(out, hier_delimiter);
  writePropertyDefinitions(out, lib);

  if (lib->getLefUnits()) {
    writeUnits(out, lib->getLefUnits());
  }
}

void lefout::writeDividerChar(std::ostream& out, char hier_delimiter)
{
  fmt::print(out, "DIVIDERCHAR \"{}\" ;\n", hier_delimiter);
}

void lefout::writeUnits(std::ostream& out, int database_units)
{
  fmt::print(out, "{}", "UNITS\n");
  fmt::print(out, "    DATABASE MICRONS {} ;\n", database_units);
  fmt::print(out, "{}", "END UNITS\n");
}

void lefout::writeBusBitChars(std::ostream& out,
                              char left_bus_delimiter,
                              char right_bus_delimiter)
{
  fmt::print(
      out, "BUSBITCHARS \"{}{}\" ;\n", left_bus_delimiter, right_bus_delimiter);
}

void lefout::writeNameCaseSensitive(std::ostream& out,
                                    const dbOnOffType on_off_type)
{
  fmt::print(out, "NAMESCASESENSITIVE {} ;\n", on_off_type.getString());
}

void lefout::writeBlockVia(std::ostream& out, dbBlock* db_block, dbVia* via)
{
  std::string name = db_block->getName() + "_" + via->getName();

  if (via->isDefault()) {
    fmt::print(out, "\nVIA {} DEFAULT\n", name.c_str());
  } else {
    fmt::print(out, "\nVIA {}\n", name.c_str());
  }

  dbTechViaGenerateRule* rule = via->getViaGenerateRule();

  if (rule == nullptr) {
    dbSet<dbBox> boxes = via->getBoxes();
    writeBoxes(out, db_block, boxes, "    ");
  } else {
    std::string rname = rule->getName();
    fmt::print(out, "  VIARULE {} ;\n", rname.c_str());

    const dbViaParams P = via->getViaParams();

    fmt::print(out,
               "  CUTSIZE {:.11g} {:.11g} ;\n",
               lefdist(P.getXCutSize()),
               lefdist(P.getYCutSize()));
    std::string top = P.getTopLayer()->getName();
    std::string bot = P.getBottomLayer()->getName();
    std::string cut = P.getCutLayer()->getName();
    fmt::print(
        out, "  LAYERS {} {} {} ;\n", bot.c_str(), cut.c_str(), top.c_str());
    fmt::print(out,
               "  CUTSPACING {:.11g} {:.11g} ;\n",
               lefdist(P.getXCutSpacing()),
               lefdist(P.getYCutSpacing()));
    fmt::print(out,
               "  ENCLOSURE {:.11g} {:.11g} {:.11g} {:.11g} ;\n",
               lefdist(P.getXBottomEnclosure()),
               lefdist(P.getYBottomEnclosure()),
               lefdist(P.getXTopEnclosure()),
               lefdist(P.getYTopEnclosure()));

    if ((P.getNumCutRows() != 1) || (P.getNumCutCols() != 1)) {
      fmt::print(
          out, "  ROWCOL {} {} ;\n", P.getNumCutRows(), P.getNumCutCols());
    }

    if ((P.getXOrigin() != 0) || (P.getYOrigin() != 0)) {
      fmt::print(out,
                 "  ORIGIN {:.11g} {:.11g} ;\n",
                 lefdist(P.getXOrigin()),
                 lefdist(P.getYOrigin()));
    }

    if ((P.getXTopOffset() != 0) || (P.getYTopOffset() != 0)
        || (P.getXBottomOffset() != 0) || (P.getYBottomOffset() != 0)) {
      fmt::print(out,
                 "  OFFSET {:.11g} {:.11g} {:.11g} {:.11g} ;\n",
                 lefdist(P.getXBottomOffset()),
                 lefdist(P.getYBottomOffset()),
                 lefdist(P.getXTopOffset()),
                 lefdist(P.getYTopOffset()));
    }

    std::string pname = via->getPattern();
    if (strcmp(pname.c_str(), "") != 0) {
      fmt::print(out, "  PATTERNNAME {} ;\n", pname.c_str());
    }
  }

  fmt::print(out, "END {}\n", name.c_str());
}

void lefout::writeBlock(std::ostream& out, dbBlock* db_block)
{
  Rect die_area = db_block->getDieArea();
  double size_x = lefdist(die_area.xMax());
  double size_y = lefdist(die_area.yMax());

  std::ostringstream macro_stream;

  fmt::print(macro_stream, "\nMACRO {}\n", db_block->getName().c_str());
  fmt::print(macro_stream, "  FOREIGN {} 0 0 ;\n", db_block->getName().c_str());
  fmt::print(macro_stream, "  CLASS BLOCK ;\n");
  fmt::print(macro_stream, "  SIZE {:.11g} BY {:.11g} ;\n", size_x, size_y);
  const std::set<dbVia*> vias = writePins(macro_stream, db_block);

  writeObstructions(macro_stream, db_block);
  fmt::print(macro_stream, "END {}\n", db_block->getName().c_str());

  // Write vias before macro
  for (dbVia* via : vias) {
    writeBlockVia(out, db_block, via);
  }

  out << macro_stream.str();
}

std::set<dbVia*> lefout::writePins(std::ostream& out, dbBlock* db_block)
{
  std::set<dbVia*> vias;

  const auto power_vias = writePowerPins(out, db_block);
  vias.insert(power_vias.begin(), power_vias.end());

  const auto block_vias = writeBlockTerms(out, db_block);
  vias.insert(block_vias.begin(), block_vias.end());

  return vias;
}

std::set<dbVia*> lefout::writeBlockTerms(std::ostream& out, dbBlock* db_block)
{
  std::set<dbVia*> vias;
  for (dbBTerm* b_term : db_block->getBTerms()) {
    fmt::print(out, "  PIN {}\n", b_term->getName().c_str());
    fmt::print(out, "    DIRECTION {} ;\n", b_term->getIoType().getString());

    std::string sig_type = "SIGNAL";
    switch (b_term->getSigType().getValue()) {
      case odb::dbSigType::SIGNAL:
      case odb::dbSigType::SCAN:
      case odb::dbSigType::TIEOFF:
      case odb::dbSigType::RESET:
        // LEF Pins can only be pin : [USE { SIGNAL | ANALOG | POWER | GROUND |
        // CLOCK } ;] whereas nets can be net: [+ USE {ANALOG | CLOCK | GROUND |
        // POWER | RESET | SCAN | SIGNAL | TIEOFF}] BTerms get their sigType
        // from the net. So we map other sigtypes to SIGNAL in exported LEFs
        sig_type = "SIGNAL";
        break;
      case odb::dbSigType::ANALOG:
        sig_type = "ANALOG";
        break;
      case odb::dbSigType::POWER:
        sig_type = "POWER";
        break;
      case odb::dbSigType::GROUND:
        sig_type = "GROUND";
        break;
      case odb::dbSigType::CLOCK:
        sig_type = "CLOCK";
        break;
    }
    fmt::print(out, "    USE {} ;\n", sig_type);

    for (dbBPin* db_b_pin : b_term->getBPins()) {
      fmt::print(out, "{}", "    PORT\n");
      dbSet<dbBox> term_pins = db_b_pin->getBoxes();
      const auto boxvias = writeBoxes(out, db_block, term_pins, "      ");
      vias.insert(boxvias.begin(), boxvias.end());
      fmt::print(out, "{}", "    END\n");
    }
    fmt::print(out, "  END {}\n", b_term->getName().c_str());
  }

  return vias;
}

std::set<dbVia*> lefout::writePowerPins(std::ostream& out, dbBlock* db_block)
{
  std::set<dbVia*> vias;

  // Power Ground.
  for (dbNet* net : db_block->getNets()) {
    if (!net->getSigType().isSupply()) {
      continue;
    }
    if (net->get1stBTerm() != nullptr) {
      // net already has pins that will be added
      continue;
    }
    fmt::print(out, "  PIN {}\n", net->getName().c_str());
    fmt::print(out, "    USE {} ;\n", net->getSigType().getString());
    fmt::print(
        out, "    DIRECTION {} ;\n", dbIoType(dbIoType::INOUT).getString());
    for (dbSWire* special_wire : net->getSWires()) {
      fmt::print(out, "    PORT\n");
      dbSet<dbSBox> wires = special_wire->getWires();
      const auto boxvias
          = writeBoxes(out, db_block, wires, /*indent=*/"      ");
      vias.insert(boxvias.begin(), boxvias.end());
      fmt::print(out, "    END\n");
    }
    fmt::print(out, "  END {}\n", net->getName().c_str());
  }

  return vias;
}

void lefout::writeTechBody(std::ostream& out, dbTech* tech)
{
  assert(tech);

  if (tech->hasNoWireExtAtPin()) {
    fmt::print(out,
               "NOWIREEXTENSIONATPIN {} ;\n",
               tech->getNoWireExtAtPin().getString());
  }

  if (tech->hasClearanceMeasure()) {
    fmt::print(out,
               "CLEARANCEMEASURE {} ;\n",
               tech->getClearanceMeasure().getString());
  }

  if (tech->hasUseMinSpacingObs()) {
    fmt::print(out,
               "USEMINSPACING OBS {} ;\n",
               tech->getUseMinSpacingObs().getString());
  }

  if (tech->hasUseMinSpacingPin()) {
    fmt::print(out,
               "USEMINSPACING PIN {} ;\n",
               tech->getUseMinSpacingPin().getString());
  }

  if (tech->hasManufacturingGrid()) {
    fmt::print(out,
               "MANUFACTURINGGRID {:.11g} ;\n",
               lefdist(tech->getManufacturingGrid()));
  }

  dbSet<dbTechLayer> layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator litr;

  for (litr = layers.begin(); litr != layers.end(); ++litr) {
    dbTechLayer* layer = *litr;
    writeLayer(out, layer);
  }

  writeViaMap(out, tech, false);
  writeViaMap(out, tech, true);

  // VIA's not using generate rule and not default
  dbSet<dbTechVia> vias = tech->getVias();
  dbSet<dbTechVia>::iterator vitr;

  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    dbTechVia* via = *vitr;

    if (via->getNonDefaultRule() == nullptr) {
      if (via->getViaGenerateRule() == nullptr) {
        writeVia(out, via);
      }
    }
  }

  dbSet<dbTechViaRule> via_rules = tech->getViaRules();
  dbSet<dbTechViaRule>::iterator vritr;

  for (vritr = via_rules.begin(); vritr != via_rules.end(); ++vritr) {
    dbTechViaRule* rule = *vritr;
    writeTechViaRule(out, rule);
  }

  dbSet<dbTechViaGenerateRule> via_gen_rules = tech->getViaGenerateRules();
  dbSet<dbTechViaGenerateRule>::iterator vgritr;

  for (vgritr = via_gen_rules.begin(); vgritr != via_gen_rules.end();
       ++vgritr) {
    dbTechViaGenerateRule* rule = *vgritr;
    writeTechViaGenerateRule(out, rule);
  }

  // VIA's using generate rule
  vias = tech->getVias();

  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    dbTechVia* via = *vitr;

    if (via->getNonDefaultRule() == nullptr) {
      if (via->getViaGenerateRule() != nullptr) {
        writeVia(out, via);
      }
    }
  }

  std::vector<dbTechSameNetRule*> srules;
  tech->getSameNetRules(srules);

  if (srules.begin() != srules.end()) {
    fmt::print(out, "\nSPACING\n");

    std::vector<dbTechSameNetRule*>::iterator sritr;
    for (sritr = srules.begin(); sritr != srules.end(); ++sritr) {
      writeSameNetRule(out, *sritr);
    }

    fmt::print(out, "\nEND SPACING\n");
  }

  dbSet<dbTechNonDefaultRule> rules = tech->getNonDefaultRules();
  dbSet<dbTechNonDefaultRule>::iterator ritr;

  for (ritr = rules.begin(); ritr != rules.end(); ++ritr) {
    dbTechNonDefaultRule* rule = *ritr;
    writeNonDefaultRule(out, tech, rule);
  }
}

void lefout::writeViaMap(std::ostream& out,
                         dbTech* tech,
                         const bool use_via_cut_class)
{
  auto via_map_set = tech->getMetalWidthViaMap();
  bool found = false;
  for (auto via_map : via_map_set) {
    if (via_map->isViaCutClass() == use_via_cut_class) {
      found = true;
      break;
    }
  }
  if (!found) {
    return;
  }
  fmt::print(out, "PROPERTYDEFINITIONS\n");
  fmt::print(out, " LIBRARY LEF58_METALWIDTHVIAMAP STRING\n");
  fmt::print(out, "  \"METALWIDTHVIAMAP\n");
  if (use_via_cut_class) {
    fmt::print(out, "   USEVIACUTCLASS\n");
  }
  for (auto via_map : via_map_set) {
    if (via_map->isViaCutClass() != use_via_cut_class) {
      continue;
    }
    if (via_map->getBelowLayerWidthLow() == via_map->getBelowLayerWidthHigh()
        && via_map->getAboveLayerWidthLow()
               == via_map->getAboveLayerWidthHigh()) {
      fmt::print(out,
                 "   VIA {} {} {} {} {}\n",
                 via_map->getCutLayer()->getName(),
                 lefdist(via_map->getBelowLayerWidthLow()),
                 lefdist(via_map->getAboveLayerWidthLow()),
                 via_map->getViaName(),
                 via_map->isPgVia() ? "PGVIA" : "");
    } else {
      fmt::print(out,
                 "   VIA {} {} {} {} {} {} {}\n",
                 via_map->getCutLayer()->getName(),
                 lefdist(via_map->getBelowLayerWidthLow()),
                 lefdist(via_map->getBelowLayerWidthHigh()),
                 lefdist(via_map->getAboveLayerWidthLow()),
                 lefdist(via_map->getAboveLayerWidthHigh()),
                 via_map->getViaName(),
                 via_map->isPgVia() ? "PGVIA" : "");
    }
  }
  fmt::print(out, "   ;\n");
  fmt::print(out, " \" ;\n");
  fmt::print(out, "END PROPERTYDEFINITIONS\n");
}

void lefout::writeNonDefaultRule(std::ostream& out,
                                 dbTech* tech,
                                 dbTechNonDefaultRule* rule)
{
  std::string name = rule->getName();
  fmt::print(out, "\nNONDEFAULTRULE {}\n", name.c_str());

  if (rule->getHardSpacing()) {
    fmt::print(out, "{}", "HARDSPACING ;\n");
  }

  std::vector<dbTechLayerRule*> layer_rules;
  rule->getLayerRules(layer_rules);

  std::vector<dbTechLayerRule*>::iterator litr;
  for (litr = layer_rules.begin(); litr != layer_rules.end(); ++litr) {
    writeLayerRule(out, *litr);
  }

  std::vector<dbTechVia*> vias;
  rule->getVias(vias);

  std::vector<dbTechVia*>::iterator vitr;
  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    writeVia(out, *vitr);
  }

  std::vector<dbTechSameNetRule*> srules;
  rule->getSameNetRules(srules);

  if (srules.begin() != srules.end()) {
    fmt::print(out, "\nSPACING\n");

    std::vector<dbTechSameNetRule*>::iterator sritr;
    for (sritr = srules.begin(); sritr != srules.end(); ++sritr) {
      writeSameNetRule(out, *sritr);
    }

    fmt::print(out, "\nEND SPACING\n");
  }

  std::vector<dbTechVia*> use_vias;
  rule->getUseVias(use_vias);

  std::vector<dbTechVia*>::iterator uvitr;
  for (uvitr = use_vias.begin(); uvitr != use_vias.end(); ++uvitr) {
    dbTechVia* via = *uvitr;
    std::string vname = via->getName();
    fmt::print(out, "USEVIA {} ;\n", vname.c_str());
  }

  std::vector<dbTechViaGenerateRule*> use_rules;
  rule->getUseViaRules(use_rules);

  std::vector<dbTechViaGenerateRule*>::iterator uvritr;
  for (uvritr = use_rules.begin(); uvritr != use_rules.end(); ++uvritr) {
    dbTechViaGenerateRule* rule = *uvritr;
    std::string rname = rule->getName();
    fmt::print(out, "USEVIARULE {} ;\n", rname.c_str());
  }

  dbSet<dbTechLayer> layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator layitr;

  for (layitr = layers.begin(); layitr != layers.end(); ++layitr) {
    dbTechLayer* layer = *layitr;
    int count;

    if (rule->getMinCuts(layer, count)) {
      std::string lname = layer->getName();
      fmt::print(out, "MINCUTS {} {} ;\n", lname.c_str(), count);
    }
  }

  fmt::print(out, "\nEND {}\n", name.c_str());
}

void lefout::writeLayerRule(std::ostream& out, dbTechLayerRule* rule)
{
  dbTechLayer* layer = rule->getLayer();
  std::string name;
  if (use_alias_ && layer->hasAlias()) {
    name = layer->getAlias();
  } else {
    name = layer->getName();
  }
  fmt::print(out, "\nLAYER {}\n", name.c_str());

  if (rule->getWidth()) {
    fmt::print(out, "    WIDTH {:.11g} ;\n", lefdist(rule->getWidth()));
  }

  if (rule->getSpacing()) {
    fmt::print(out, "    SPACING {:.11g} ;\n", lefdist(rule->getSpacing()));
  }

  if (rule->getWireExtension() != 0.0) {
    fmt::print(out,
               "    WIREEXTENSION {:.11g} ;\n",
               lefdist(rule->getWireExtension()));
  }

  if (rule->getResistance() != 0.0) {
    fmt::print(out, "    RESISTANCE RPERSQ {:.11g} ;\n", rule->getResistance());
  }

  if (rule->getCapacitance() != 0.0) {
    fmt::print(
        out, "    CAPACITANCE CPERSQDIST {:.11g} ;\n", rule->getCapacitance());
  }

  if (rule->getEdgeCapacitance() != 0.0) {
    fmt::print(
        out, "      EDGECAPACITANCE {:.11g} ;\n", rule->getEdgeCapacitance());
  }

  fmt::print(out, "END {}\n", name.c_str());
}

void lefout::writeTechViaRule(std::ostream& out, dbTechViaRule* rule)
{
  std::string name = rule->getName();
  fmt::print(out, "\nVIARULE {}\n", name.c_str());

  uint32_t idx;

  for (idx = 0; idx < rule->getViaLayerRuleCount(); ++idx) {
    dbTechViaLayerRule* layrule = rule->getViaLayerRule(idx);
    dbTechLayer* layer = layrule->getLayer();
    std::string lname = layer->getName();
    fmt::print(out, "    LAYER {} ;\n", lname.c_str());

    if (layrule->getDirection() == dbTechLayerDir::VERTICAL) {
      fmt::print(out, "      DIRECTION VERTICAL ;\n");
    } else if (layrule->getDirection() == dbTechLayerDir::HORIZONTAL) {
      fmt::print(out, "      DIRECTION HORIZONTAL ;\n");
    }

    if (layrule->hasWidth()) {
      int minW, maxW;
      layrule->getWidth(minW, maxW);
      fmt::print(out,
                 "      WIDTH {:.11g} TO {:.11g} ;\n",
                 lefdist(minW),
                 lefdist(maxW));
    }
  }

  for (idx = 0; idx < rule->getViaCount(); ++idx) {
    dbTechVia* via = rule->getVia(idx);
    std::string vname = via->getName();
    fmt::print(out, "    VIA {} ;\n", vname.c_str());
  }

  fmt::print(out, "END {}\n", name.c_str());
}

void lefout::writeTechViaGenerateRule(std::ostream& out,
                                      dbTechViaGenerateRule* rule)
{
  std::string name = rule->getName();

  if (rule->isDefault()) {
    fmt::print(out, "\nVIARULE {} GENERATE DEFAULT\n", name.c_str());
  } else {
    fmt::print(out, "\nVIARULE {} GENERATE \n", name.c_str());
  }

  uint32_t idx;

  for (idx = 0; idx < rule->getViaLayerRuleCount(); ++idx) {
    dbTechViaLayerRule* layrule = rule->getViaLayerRule(idx);
    dbTechLayer* layer = layrule->getLayer();
    std::string lname = layer->getName();
    fmt::print(out, "    LAYER {} ;\n", lname.c_str());

    if (layrule->getDirection() == dbTechLayerDir::VERTICAL) {
      fmt::print(out, "      DIRECTION VERTICAL ;\n");
    } else if (layrule->getDirection() == dbTechLayerDir::HORIZONTAL) {
      fmt::print(out, "      DIRECTION HORIZONTAL ;\n");
    }

    if (layrule->hasOverhang()) {
      fmt::print(
          out, "      OVERHANG {:.11g} ;\n", lefdist(layrule->getOverhang()));
    }

    if (layrule->hasMetalOverhang()) {
      fmt::print(out,
                 "      METALOVERHANG {:.11g} ;\n",
                 lefdist(layrule->getMetalOverhang()));
    }

    if (layrule->hasEnclosure()) {
      int overhang1, overhang2;
      layrule->getEnclosure(overhang1, overhang2);
      fmt::print(out,
                 "      ENCLOSURE {:.11g} {:.11g} ;\n",
                 lefdist(overhang1),
                 lefdist(overhang2));
    }

    if (layrule->hasWidth()) {
      int minW, maxW;
      layrule->getWidth(minW, maxW);
      fmt::print(out,
                 "      WIDTH {:.11g} TO {:.11g} ;\n",
                 lefdist(minW),
                 lefdist(maxW));
    }

    if (layrule->hasRect()) {
      Rect r;
      layrule->getRect(r);
      fmt::print(out,
                 "      RECT  {:.11g} {:.11g}  {:.11g} {:.11g}  ;\n",
                 lefdist(r.xMin()),
                 lefdist(r.yMin()),
                 lefdist(r.xMax()),
                 lefdist(r.yMax()));
    }

    if (layrule->hasSpacing()) {
      int spacing_x, spacing_y;
      layrule->getSpacing(spacing_x, spacing_y);
      fmt::print(out,
                 "      SPACING {:.11g} BY {:.11g} ;\n",
                 lefdist(spacing_x),
                 lefdist(spacing_y));
    }

    if (layrule->hasResistance()) {
      fmt::print(out, "      RESISTANCE {:.11g} ;\n", layrule->getResistance());
    }
  }

  fmt::print(out, "END {}\n", name.c_str());
}

void lefout::writeSameNetRule(std::ostream& out, dbTechSameNetRule* rule)
{
  dbTechLayer* l1 = rule->getLayer1();
  dbTechLayer* l2 = rule->getLayer2();

  std::string n1;
  if (use_alias_ && l1->hasAlias()) {
    n1 = l1->getAlias();
  } else {
    n1 = l1->getName();
  }

  std::string n2;
  if (use_alias_ && l2->hasAlias()) {
    n2 = l2->getAlias();
  } else {
    n2 = l2->getName();
  }

  if (rule->getAllowStackedVias()) {
    fmt::print(out,
               "  SAMENET {} {} {:.11g} STACK ;\n",
               n1.c_str(),
               n2.c_str(),
               lefdist(rule->getSpacing()));
  } else {
    fmt::print(out,
               "  SAMENET {} {} {:.11g} ;\n",
               n1.c_str(),
               n2.c_str(),
               lefdist(rule->getSpacing()));
  }
}

void lefout::writeLayer(std::ostream& out, dbTechLayer* layer)
{
  std::string name;
  if (use_alias_ && layer->hasAlias()) {
    name = layer->getAlias();
  } else {
    name = layer->getName();
  }

  fmt::print(out, "\nLAYER {}\n", name.c_str());
  fmt::print(out, "    TYPE {} ;\n", layer->getType().getString());

  if (layer->getNumMasks() > 1) {
    fmt::print(out, "    MASK {} ;\n", layer->getNumMasks());
  }

  if (layer->getPitch()) {
    fmt::print(out, "    PITCH {:.11g} ;\n", lefdist(layer->getPitch()));
  }

  if (layer->getWidth()) {
    fmt::print(out, "    WIDTH {:.11g} ;\n", lefdist(layer->getWidth()));
  }

  if (layer->getWireExtension() != 0.0) {
    fmt::print(out,
               "    WIREEXTENSION {:.11g} ;\n",
               lefdist(layer->getWireExtension()));
  }

  if (layer->hasArea()) {
    fmt::print(out, "    AREA {:.11g} ;\n", layer->getArea());
  }

  uint32_t thickness;
  if (layer->getThickness(thickness)) {
    fmt::print(out, "    THICKNESS {:.11g} ;\n", lefdist(thickness));
  }

  if (layer->hasMaxWidth()) {
    fmt::print(out, "    MAXWIDTH {:.11g} ;\n", lefdist(layer->getMaxWidth()));
  }

  if (layer->hasMinStep()) {
    fmt::print(out, "    MINSTEP {:.11g} ;\n", lefdist(layer->getMinStep()));
  }

  if (layer->hasProtrusion()) {
    fmt::print(out,
               "    PROTRUSIONWIDTH {:.11g}  LENGTH {:.11g}  WIDTH {:.11g} ;\n",
               lefdist(layer->getProtrusionWidth()),
               lefdist(layer->getProtrusionLength()),
               lefdist(layer->getProtrusionFromWidth()));
  }

  for (auto rule : layer->getV54SpacingRules()) {
    rule->writeLef(*this);
  }

  if (layer->hasV55SpacingRules()) {
    layer->printV55SpacingRules(*this);
    auto inf_rules = layer->getV55InfluenceRules();
    if (!inf_rules.empty()) {
      fmt::print(out, "SPACINGTABLE INFLUENCE");
      for (auto rule : inf_rules) {
        rule->writeLef(*this);
      }
      fmt::print(out, " ;\n");
    }
  }

  std::vector<dbTechMinCutRule*> cut_rules;
  std::vector<dbTechMinCutRule*>::const_iterator citr;
  if (layer->getMinimumCutRules(cut_rules)) {
    for (citr = cut_rules.begin(); citr != cut_rules.end(); citr++) {
      (*citr)->writeLef(*this);
    }
  }

  std::vector<dbTechMinEncRule*> enc_rules;
  std::vector<dbTechMinEncRule*>::const_iterator eitr;
  if (layer->getMinEnclosureRules(enc_rules)) {
    for (eitr = enc_rules.begin(); eitr != enc_rules.end(); eitr++) {
      (*eitr)->writeLef(*this);
    }
  }

  layer->writeAntennaRulesLef(*this);

  if (layer->getDirection() != dbTechLayerDir::NONE) {
    fmt::print(out, "    DIRECTION {} ;\n", layer->getDirection().getString());
  }

  if (layer->getResistance() != 0.0) {
    if (layer->getType() == dbTechLayerType::CUT) {
      fmt::print(out, "    RESISTANCE {:.11g} ;\n", layer->getResistance());
    } else {
      fmt::print(
          out, "    RESISTANCE RPERSQ {:.11g} ;\n", layer->getResistance());
    }
  }

  if (layer->getCapacitance() != 0.0) {
    fmt::print(
        out, "    CAPACITANCE CPERSQDIST {:.11g} ;\n", layer->getCapacitance());
  }

  if (layer->getEdgeCapacitance() != 0.0) {
    fmt::print(
        out, "    EDGECAPACITANCE {:.11g} ;\n", layer->getEdgeCapacitance());
  }

  fmt::print(out, "{}", dbProperty::writeProperties(layer));

  fmt::print(out, "END {}\n", name.c_str());
}

void lefout::writeVia(std::ostream& out, dbTechVia* via)
{
  std::string name = via->getName();

  if (via->isDefault()) {
    fmt::print(out, "\nVIA {} DEFAULT\n", name.c_str());
  } else {
    fmt::print(out, "\nVIA {}\n", name.c_str());
  }

  if (via->isTopOfStack()) {
    fmt::print(out, "    TOPOFSTACKONLY\n");
  }

  if (via->getResistance() != 0.0) {
    fmt::print(out, "    RESISTANCE {:.11g} ;\n", via->getResistance());
  }

  dbTechViaGenerateRule* rule = via->getViaGenerateRule();

  if (rule == nullptr) {
    dbSet<dbBox> boxes = via->getBoxes();
    writeBoxes(out, nullptr, boxes, "    ");
  } else {
    std::string rname = rule->getName();
    fmt::print(out, "\n    VIARULE {} \n", rname.c_str());

    const dbViaParams P = via->getViaParams();

    fmt::print(out,
               " + CUTSIZE {:.11g} {:.11g}\n",
               lefdist(P.getXCutSize()),
               lefdist(P.getYCutSize()));
    std::string top = P.getTopLayer()->getName();
    std::string bot = P.getBottomLayer()->getName();
    std::string cut = P.getCutLayer()->getName();
    fmt::print(
        out, " + LAYERS {} {} {}\n", bot.c_str(), cut.c_str(), top.c_str());
    fmt::print(out,
               " + CUTSPACING {:.11g} {:.11g}\n",
               lefdist(P.getXCutSpacing()),
               lefdist(P.getYCutSpacing()));
    fmt::print(out,
               " + ENCLOSURE {:.11g} {:.11g} {:.11g} {:.11g}\n",
               lefdist(P.getXBottomEnclosure()),
               lefdist(P.getYBottomEnclosure()),
               lefdist(P.getXTopEnclosure()),
               lefdist(P.getYTopEnclosure()));

    if ((P.getNumCutRows() != 1) || (P.getNumCutCols() != 1)) {
      fmt::print(
          out, " + ROWCOL {} {}\n", P.getNumCutRows(), P.getNumCutCols());
    }

    if ((P.getXOrigin() != 0) || (P.getYOrigin() != 0)) {
      fmt::print(out,
                 " + ORIGIN {:.11g} {:.11g}\n",
                 lefdist(P.getXOrigin()),
                 lefdist(P.getYOrigin()));
    }

    if ((P.getXTopOffset() != 0) || (P.getYTopOffset() != 0)
        || (P.getXBottomOffset() != 0) || (P.getYBottomOffset() != 0)) {
      fmt::print(out,
                 " + OFFSET {:.11g} {:.11g} {:.11g} {:.11g}\n",
                 lefdist(P.getXBottomOffset()),
                 lefdist(P.getYBottomOffset()),
                 lefdist(P.getXTopOffset()),
                 lefdist(P.getYTopOffset()));
    }

    std::string pname = via->getPattern();
    if (strcmp(pname.c_str(), "") != 0) {
      fmt::print(out, " + PATTERNNAME {}\n", pname.c_str());
    }
  }

  fmt::print(out, "END {}\n", name.c_str());
}

void lefout::writeLibBody(std::ostream& out, dbLib* lib)
{
  dbSet<dbSite> sites = lib->getSites();
  dbSet<dbSite>::iterator site_itr;

  for (site_itr = sites.begin(); site_itr != sites.end(); ++site_itr) {
    dbSite* site = *site_itr;
    writeSite(out, site);
  }

  dbSet<dbMaster> masters = lib->getMasters();
  dbSet<dbMaster>::iterator master_itr;

  for (master_itr = masters.begin(); master_itr != masters.end();
       ++master_itr) {
    dbMaster* master = *master_itr;
    if (write_marked_masters_ && !master->isMarked()) {
      continue;
    }
    writeMaster(out, master);
  }
}

void lefout::writeSite(std::ostream& out, dbSite* site)
{
  std::string n = site->getName();

  fmt::print(out, "SITE {}\n", n.c_str());
  dbSiteClass sclass = site->getClass();
  fmt::print(out, "    CLASS {} ;\n", sclass.getString());

  if (site->getSymmetryX() || site->getSymmetryY() || site->getSymmetryR90()) {
    fmt::print(out, "{}", "    SYMMETRY");

    if (site->getSymmetryX()) {
      fmt::print(out, "{}", " X");
    }

    if (site->getSymmetryY()) {
      fmt::print(out, "{}", " Y");
    }

    if (site->getSymmetryR90()) {
      fmt::print(out, "{}", " R90");
    }

    fmt::print(out, "{}", " ;\n");
  }

  if (site->getWidth() || site->getHeight()) {
    fmt::print(out,
               "    SIZE {:.11g} BY {:.11g} ;\n",
               lefdist(site->getWidth()),
               lefdist(site->getHeight()));
  }

  fmt::print(out, "END {}\n", n.c_str());
}

void lefout::writeMaster(std::ostream& out, dbMaster* master)
{
  std::string name = master->getName();

  if (use_master_ids_) {
    fmt::print(out,
               "\nMACRO M{}\n",
               static_cast<std::uint32_t>(master->getMasterId()));
  } else {
    fmt::print(out, "\nMACRO {}\n", name.c_str());
  }

  fmt::print(out, "    CLASS {} ;\n", master->getType().getString());

  const odb::Point origin = master->getOrigin();

  if (origin != Point()) {
    fmt::print(out,
               "    ORIGIN {:.11g} {:.11g} ;\n",
               lefdist(origin.x()),
               lefdist(origin.y()));
  }

  if (master->getEEQ()) {
    std::string eeq = master->getEEQ()->getName();
    if (use_master_ids_) {
      fmt::print(out,
                 "    EEQ M{} ;\n",
                 static_cast<std::uint32_t>(master->getEEQ()->getMasterId()));
    } else {
      fmt::print(out, "    EEQ {} ;\n", eeq.c_str());
    }
  }

  if (master->getLEQ()) {
    std::string leq = master->getLEQ()->getName();
    if (use_master_ids_) {
      fmt::print(out,
                 "    LEQ M{} ;\n",
                 static_cast<std::uint32_t>(master->getLEQ()->getMasterId()));
    } else {
      fmt::print(out, "    LEQ {} ;\n", leq.c_str());
    }
  }

  int w = master->getWidth();
  int h = master->getHeight();

  if ((w != 0) || (h != 0)) {
    fmt::print(out, "    SIZE {:.11g} BY {:.11g} ;\n", lefdist(w), lefdist(h));
  }

  if (master->getSymmetryX() || master->getSymmetryY()
      || master->getSymmetryR90()) {
    fmt::print(out, "{}", "    SYMMETRY");

    if (master->getSymmetryX()) {
      fmt::print(out, "{}", " X");
    }

    if (master->getSymmetryY()) {
      fmt::print(out, "{}", " Y");
    }

    if (master->getSymmetryR90()) {
      fmt::print(out, "{}", " R90");
    }

    fmt::print(out, "{}", " ;\n");
  }

  if (origin != Point()) {
    dbTransform t(Point(-origin.x(), -origin.y()));
    master->transform(t);
  }

  if (master->getSite()) {
    std::string site = master->getSite()->getName();
    fmt::print(out, "    SITE {} ;\n", site.c_str());
  }

  dbSet<dbMTerm> mterms = master->getMTerms();
  dbSet<dbMTerm>::iterator mitr;

  for (mitr = mterms.begin(); mitr != mterms.end(); ++mitr) {
    dbMTerm* mterm = *mitr;
    writeMTerm(out, mterm);
  }

  dbSet<dbPolygon> poly_obs = master->getPolygonObstructions();
  dbSet<dbBox> obs = master->getObstructions(false);

  if (poly_obs.begin() != poly_obs.end() || obs.begin() != obs.end()) {
    fmt::print(out, "{}", "    OBS\n");
    writeBoxes(out, nullptr, poly_obs, "      ");
    writeBoxes(out, nullptr, obs, "      ");
    fmt::print(out, "{}", "    END\n");
  }

  if (origin != Point()) {
    dbTransform t(origin);
    master->transform(t);
  }

  if (use_master_ids_) {
    fmt::print(
        out, "END M{}\n", static_cast<std::uint32_t>(master->getMasterId()));
  } else {
    fmt::print(out, "END {}\n", name.c_str());
  }
}

void lefout::writeMTerm(std::ostream& out, dbMTerm* mterm)
{
  std::string name = mterm->getName();

  fmt::print(out, "    PIN {}\n", name.c_str());
  fmt::print(out, "        DIRECTION {} ; \n", mterm->getIoType().getString());
  fmt::print(out, "        USE {} ; \n", mterm->getSigType().getString());

  mterm->writeAntennaLef(*this);
  dbSet<dbMPin> pins = mterm->getMPins();
  dbSet<dbMPin>::iterator pitr;

  for (pitr = pins.begin(); pitr != pins.end(); ++pitr) {
    dbMPin* pin = *pitr;

    dbSet<dbPolygon> poly_geoms = pin->getPolygonGeometry();
    dbSet<dbBox> geoms = pin->getGeometry(false);

    if (poly_geoms.begin() != poly_geoms.end()
        || geoms.begin() != geoms.end()) {
      fmt::print(out, "        PORT\n");
      writeBoxes(out, nullptr, poly_geoms, "            ");
      writeBoxes(out, nullptr, geoms, "            ");
      fmt::print(out, "        END\n");
    }
  }

  fmt::print(out, "    END {}\n", name.c_str());
}

void lefout::writePropertyDefinition(std::ostream& out, dbProperty* prop)
{
  std::string propName = prop->getName();
  dbObjectType owner_type = prop->getPropOwner()->getObjectType();
  dbProperty::Type prop_type = prop->getType();
  std::string objectType, propType;
  switch (owner_type) {
    case dbTechLayerObj:
      objectType = "LAYER";
      break;
    case dbLibObj:
      objectType = "LIBRARY";
      break;
    case dbMasterObj:
      objectType = "MACRO";
      break;
    case dbMPinObj:
      objectType = "PIN";
      break;
    case dbTechViaObj:
      objectType = "VIA";
      break;
    case dbTechViaRuleObj:
      objectType = "VIARULE";
      break;
    case dbTechNonDefaultRuleObj:
      objectType = "NONDEFAULTRULE";
      break;
    default:
      return;
  }

  switch (prop_type) {
    case dbProperty::INT_PROP:
    case dbProperty::BOOL_PROP:
      propType = "INTEGER";
      break;
    case dbProperty::DOUBLE_PROP:
      propType = "REAL";
      break;
    case dbProperty::STRING_PROP:
      propType = "STRING";
      break;
    default:
      return;
  }
  fmt::print(out,
             "    {} {} {} ",
             objectType.c_str(),
             propName.c_str(),
             propType.c_str());
  if (owner_type == dbLibObj) {
    fmt::print(out, "{}", "\n        ");
    fmt::print(out, "{}", dbProperty::writePropValue(prop));
    fmt::print(out, "{}", "\n    ");
  }

  fmt::print(out, "{}", ";\n");
}

inline void lefout::writeObjectPropertyDefinitions(
    std::ostream& out,
    dbObject* obj,
    std::unordered_map<std::string, int16_t>& propertiesMap)
{
  int bitNumber;
  switch (obj->getObjectType()) {
    case dbTechLayerObj:
      bitNumber = 0;
      break;
    case dbLibObj:
      bitNumber = 1;
      break;
    case dbMasterObj:
      bitNumber = 2;
      break;
    case dbMPinObj:
      bitNumber = 3;
      break;
    case dbTechViaObj:
      bitNumber = 4;
      break;
    case dbTechViaRuleObj:
      bitNumber = 5;
      break;
    case dbTechNonDefaultRuleObj:
      bitNumber = 6;
      break;
    default:
      return;
  }
  dbSet<dbProperty> properties = dbProperty::getProperties(obj);
  dbSet<dbProperty>::iterator pitr;
  for (pitr = properties.begin(); pitr != properties.end(); ++pitr) {
    dbProperty* prop = *pitr;
    if (propertiesMap[prop->getName()] & 0x1 << bitNumber) {
      continue;
    }
    propertiesMap[prop->getName()] |= 0x1 << bitNumber;
    writePropertyDefinition(out, prop);
  }
}

void lefout::writePropertyDefinitions(std::ostream& out, dbLib* lib)
{
  std::unordered_map<std::string, int16_t> propertiesMap;
  dbTech* tech = lib->getTech();

  fmt::print(out, "{}", "\nPROPERTYDEFINITIONS\n");

  // writing property definitions of objectType LAYER
  for (dbTechLayer* layer : tech->getLayers()) {
    writeObjectPropertyDefinitions(out, layer, propertiesMap);
  }

  // writing property definitions of objectType LIBRARY
  writeObjectPropertyDefinitions(out, lib, propertiesMap);

  // writing property definitions of objectType MACRO
  for (dbMaster* master : lib->getMasters()) {
    writeObjectPropertyDefinitions(out, master, propertiesMap);
    for (dbMTerm* term : master->getMTerms()) {
      for (dbMPin* pin : term->getMPins()) {
        writeObjectPropertyDefinitions(out, pin, propertiesMap);
      }
    }
  }

  // writing property definitions of objectType VIA
  for (dbTechVia* via : tech->getVias()) {
    writeObjectPropertyDefinitions(out, via, propertiesMap);
  }

  // writing property definitions of objectType VIARULE
  for (dbTechViaRule* vrule : tech->getViaRules()) {
    writeObjectPropertyDefinitions(out, vrule, propertiesMap);
  }

  // writing property definitions of objectType NONDEFAULTRULE
  for (dbTechNonDefaultRule* nrule : tech->getNonDefaultRules()) {
    writeObjectPropertyDefinitions(out, nrule, propertiesMap);
  }

  fmt::print(out, "{}", "END PROPERTYDEFINITIONS\n\n");
}

void lefout::writeTech(dbTech* tech)
{
  dist_factor_ = 1.0 / tech->getDbUnitsPerMicron();
  area_factor_ = dist_factor_ * dist_factor_;
  writeTechBody(_out, tech);

  fmt::print(_out, "END LIBRARY\n");
}

void lefout::writeLib(dbLib* lib)
{
  dist_factor_ = 1.0 / lib->getDbUnitsPerMicron();
  area_factor_ = dist_factor_ * dist_factor_;
  writeHeader(_out, lib);
  writeLibBody(_out, lib);
  fmt::print(_out, "END LIBRARY\n");
}

void lefout::writeTechAndLib(dbLib* lib)
{
  dist_factor_ = 1.0 / lib->getDbUnitsPerMicron();
  area_factor_ = dist_factor_ * dist_factor_;
  dbTech* tech = lib->getTech();
  writeHeader(_out, lib);
  writeTechBody(_out, tech);
  writeLibBody(_out, lib);
  fmt::print(_out, "END LIBRARY\n");
}

void lefout::writeAbstractLef(dbBlock* db_block)
{
  utl::SetAndRestore set_dist(dist_factor_,
                              1.0 / db_block->getDbUnitsPerMicron());
  utl::SetAndRestore set_area(area_factor_, dist_factor_ * dist_factor_);

  writeHeader(_out, db_block);
  writeBlock(_out, db_block);
  fmt::print(_out, "END LIBRARY\n");
}

}  // namespace odb
