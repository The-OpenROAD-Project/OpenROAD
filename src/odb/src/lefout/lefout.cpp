///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include "odb/lefout.h"

#include <spdlog/fmt/ostr.h>

#include <algorithm>
#include <boost/polygon/polygon.hpp>
#include <cstdio>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "utl/scope.h"

using namespace boost::polygon::operators;
using namespace odb;

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
  obstructions[layer] += poly.bloat(bloat, bloat, bloat, bloat);
}

void lefout::writeVersion(const std::string& version)
{
  fmt::print(_out, "VERSION {} ;\n", version);
}

template <typename GenericBox>
void lefout::writeBoxes(dbBlock* block,
                        dbSet<GenericBox>& boxes,
                        const char* indent)
{
  dbTechLayer* cur_layer = nullptr;

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
      }

      int x, y;
      box->getViaXY(x, y);
      fmt::print(_out,
                 "{}VIA {:.11g} {:.11g} {} ;\n",
                 indent,
                 lefdist(x),
                 lefdist(y),
                 via_name.c_str());
      cur_layer = nullptr;
    } else {
      std::string layer_name;
      if (_use_alias && layer->hasAlias()) {
        layer_name = layer->getAlias();
      } else {
        layer_name = layer->getName();
      }

      if (cur_layer != layer) {
        fmt::print(_out, "{}LAYER {} ;\n", indent, layer_name.c_str());
        cur_layer = layer;
      }

      writeBox(indent, box);
    }
  }
}

template <>
void lefout::writeBoxes(dbBlock* block,
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
    if (_use_alias && layer->hasAlias()) {
      layer_name = layer->getAlias();
    } else {
      layer_name = layer->getName();
    }

    if (cur_layer != layer) {
      fmt::print(_out, "{}LAYER {} ;\n", indent, layer_name.c_str());
      cur_layer = layer;
    }

    writePolygon(indent, box);
  }
}

void lefout::writeBox(const std::string& indent, dbBox* box)
{
  int x1 = box->xMin();
  int y1 = box->yMin();
  int x2 = box->xMax();
  int y2 = box->yMax();

  fmt::print(_out,
             "{}  RECT  {:.11g} {:.11g} {:.11g} {:.11g} ;\n",
             indent.c_str(),
             lefdist(x1),
             lefdist(y1),
             lefdist(x2),
             lefdist(y2));
}

void lefout::writePolygon(const std::string& indent, dbPolygon* polygon)
{
  fmt::print(_out, "{}  POLYGON  ", indent.c_str());

  for (const Point& pt : polygon->getPolygon().getPoints()) {
    int x = pt.x();
    int y = pt.y();
    fmt::print(_out, "{:.11g} {:.11g} ", lefdist(x), lefdist(y));
  }

  fmt::print(_out, ";\n");
}

void lefout::writeRect(const std::string& indent,
                       const boost::polygon::rectangle_data<int>& rect)
{
  int x1 = boost::polygon::xl(rect);
  int y1 = boost::polygon::yl(rect);
  int x2 = boost::polygon::xh(rect);
  int y2 = boost::polygon::yh(rect);

  fmt::print(_out,
             "{}  RECT  {:.11g} {:.11g} {:.11g} {:.11g} ;\n",
             indent.c_str(),
             lefdist(x1),
             lefdist(y1),
             lefdist(x2),
             lefdist(y2));
}

void lefout::writeHeader(dbBlock* db_block)
{
  char left_bus_delimeter = 0;
  char right_bus_delimeter = 0;
  char hier_delimeter = db_block->getHierarchyDelimeter();

  db_block->getBusDelimeters(left_bus_delimeter, right_bus_delimeter);

  if (left_bus_delimeter == 0) {
    left_bus_delimeter = '[';
  }

  if (right_bus_delimeter == 0) {
    right_bus_delimeter = ']';
  }

  if (hier_delimeter == 0) {
    hier_delimeter = '|';
  }

  writeVersion("5.8");
  writeBusBitChars(left_bus_delimeter, right_bus_delimeter);
  writeDividerChar(hier_delimeter);
  writeUnits(/*database_units = */ db_block->getDbUnitsPerMicron());
}

void lefout::writeObstructions(dbBlock* db_block)
{
  ObstructionMap obstructions;
  getObstructions(db_block, obstructions);

  fmt::print(_out, "{}", "  OBS\n");
  dbBox* block_bounding_box = db_block->getBBox();
  for (const auto& [tech_layer, polySet] : obstructions) {
    fmt::print(_out, "    LAYER {} ;\n", tech_layer->getName().c_str());

    if (bloat_occupied_layers_) {
      writeBox("   ", block_bounding_box);
    } else {
      const int bloat = determineBloat(tech_layer);
      boost::polygon::polygon_90_set_data<int> shrink_poly = polySet;
      shrink_poly.shrink2(bloat, bloat, bloat, bloat);

      // Decompose the polygon set to rectanges in non-preferred direction
      std::vector<boost::polygon::rectangle_data<int>> rects;
      if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
        shrink_poly.get_rectangles(rects, boost::polygon::VERTICAL);
      } else if (tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
        shrink_poly.get_rectangles(rects, boost::polygon::HORIZONTAL);
      } else if (tech_layer->getDirection() == odb::dbTechLayerDir::NONE) {
        shrink_poly.get_rectangles(rects);
      }

      for (const auto& rect : rects) {
        writeRect("   ", rect);
      }
    }
  }
  fmt::print(_out, "  END\n");
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
    if (db_shape.isViaBox()) {
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
      continue;
    }
    insertObstruction(shape.getTechLayer(), shape.getBox(), obstructions);
  }
}

void lefout::writeHeader(dbLib* lib)
{
  dbTech* tech = lib->getTech();

  char left_bus_delimeter = 0;
  char right_bus_delimeter = 0;
  char hier_delimeter = lib->getHierarchyDelimeter();

  lib->getBusDelimeters(left_bus_delimeter, right_bus_delimeter);

  if (left_bus_delimeter == 0) {
    left_bus_delimeter = '[';
  }

  if (right_bus_delimeter == 0) {
    right_bus_delimeter = ']';
  }

  if (hier_delimeter == 0) {
    hier_delimeter = '|';
  }

  writeVersion(tech->getLefVersionStr());
  writeNameCaseSensitive(tech->getNamesCaseSensitive());
  writeBusBitChars(left_bus_delimeter, right_bus_delimeter);
  writeDividerChar(hier_delimeter);
  writePropertyDefinitions(lib);

  if (lib->getLefUnits()) {
    writeUnits(lib->getLefUnits());
  }
}

void lefout::writeDividerChar(char hier_delimeter)
{
  fmt::print(_out, "DIVIDERCHAR \"{}\" ;\n", hier_delimeter);
}

void lefout::writeUnits(int database_units)
{
  fmt::print(_out, "{}", "UNITS\n");
  fmt::print(_out, "    DATABASE MICRONS {} ;\n", database_units);
  fmt::print(_out, "{}", "END UNITS\n");
}

void lefout::writeBusBitChars(char left_bus_delimeter, char right_bus_delimeter)
{
  fmt::print(_out,
             "BUSBITCHARS \"{}{}\" ;\n",
             left_bus_delimeter,
             right_bus_delimeter);
}

void lefout::writeNameCaseSensitive(const dbOnOffType on_off_type)
{
  fmt::print(_out, "NAMESCASESENSITIVE {} ;\n", on_off_type.getString());
}

void lefout::writeBlockVia(dbBlock* db_block, dbVia* via)
{
  std::string name = db_block->getName() + "_" + via->getName();

  if (via->isDefault()) {
    fmt::print(_out, "\nVIA {} DEFAULT\n", name.c_str());
  } else {
    fmt::print(_out, "\nVIA {}\n", name.c_str());
  }

  dbTechViaGenerateRule* rule = via->getViaGenerateRule();

  if (rule == nullptr) {
    dbSet<dbBox> boxes = via->getBoxes();
    writeBoxes(db_block, boxes, "    ");
  } else {
    std::string rname = rule->getName();
    fmt::print(_out, "  VIARULE {} ;\n", rname.c_str());

    const dbViaParams P = via->getViaParams();

    fmt::print(_out,
               "  CUTSIZE {:.11g} {:.11g} ;\n",
               lefdist(P.getXCutSize()),
               lefdist(P.getYCutSize()));
    std::string top = P.getTopLayer()->getName();
    std::string bot = P.getBottomLayer()->getName();
    std::string cut = P.getCutLayer()->getName();
    fmt::print(
        _out, "  LAYERS {} {} {} ;\n", bot.c_str(), cut.c_str(), top.c_str());
    fmt::print(_out,
               "  CUTSPACING {:.11g} {:.11g} ;\n",
               lefdist(P.getXCutSpacing()),
               lefdist(P.getYCutSpacing()));
    fmt::print(_out,
               "  ENCLOSURE {:.11g} {:.11g} {:.11g} {:.11g} ;\n",
               lefdist(P.getXBottomEnclosure()),
               lefdist(P.getYBottomEnclosure()),
               lefdist(P.getXTopEnclosure()),
               lefdist(P.getYTopEnclosure()));

    if ((P.getNumCutRows() != 1) || (P.getNumCutCols() != 1)) {
      fmt::print(
          _out, "  ROWCOL {} {} ;\n", P.getNumCutRows(), P.getNumCutCols());
    }

    if ((P.getXOrigin() != 0) || (P.getYOrigin() != 0)) {
      fmt::print(_out,
                 "  ORIGIN {:.11g} {:.11g} ;\n",
                 lefdist(P.getXOrigin()),
                 lefdist(P.getYOrigin()));
    }

    if ((P.getXTopOffset() != 0) || (P.getYTopOffset() != 0)
        || (P.getXBottomOffset() != 0) || (P.getYBottomOffset() != 0)) {
      fmt::print(_out,
                 "  OFFSET {:.11g} {:.11g} {:.11g} {:.11g} ;\n",
                 lefdist(P.getXBottomOffset()),
                 lefdist(P.getYBottomOffset()),
                 lefdist(P.getXTopOffset()),
                 lefdist(P.getYTopOffset()));
    }

    std::string pname = via->getPattern();
    if (strcmp(pname.c_str(), "") != 0) {
      fmt::print(_out, "  PATTERNNAME {} ;\n", pname.c_str());
    }
  }

  fmt::print(_out, "END {}\n", name.c_str());
}

void lefout::writeBlock(dbBlock* db_block)
{
  Rect die_area = db_block->getDieArea();
  double size_x = lefdist(die_area.xMax());
  double size_y = lefdist(die_area.yMax());

  for (auto via : db_block->getVias()) {
    writeBlockVia(db_block, via);
  }

  fmt::print(_out, "\nMACRO {}\n", db_block->getName().c_str());
  fmt::print(_out, "  FOREIGN {} 0 0 ;\n", db_block->getName().c_str());
  fmt::print(_out, "  CLASS BLOCK ;\n");
  fmt::print(_out, "  SIZE {:.11g} BY {:.11g} ;\n", size_x, size_y);
  writePins(db_block);
  writeObstructions(db_block);
  fmt::print(_out, "END {}\n", db_block->getName().c_str());
}

void lefout::writePins(dbBlock* db_block)
{
  writePowerPins(db_block);
  writeBlockTerms(db_block);
}

void lefout::writeBlockTerms(dbBlock* db_block)
{
  for (dbBTerm* b_term : db_block->getBTerms()) {
    fmt::print(_out, "  PIN {}\n", b_term->getName().c_str());
    fmt::print(_out, "    DIRECTION {} ;\n", b_term->getIoType().getString());

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
    fmt::print(_out, "    USE {} ;\n", sig_type);

    for (dbBPin* db_b_pin : b_term->getBPins()) {
      fmt::print(_out, "{}", "    PORT\n");
      dbSet<dbBox> term_pins = db_b_pin->getBoxes();
      writeBoxes(db_block, term_pins, "      ");
      fmt::print(_out, "{}", "    END\n");
    }
    fmt::print(_out, "  END {}\n", b_term->getName().c_str());
  }
}

void lefout::writePowerPins(dbBlock* db_block)
{  // Power Ground.
  for (dbNet* net : db_block->getNets()) {
    if (!net->getSigType().isSupply()) {
      continue;
    }
    if (net->get1stBTerm() != nullptr) {
      // net already has pins that will be added
      continue;
    }
    fmt::print(_out, "  PIN {}\n", net->getName().c_str());
    fmt::print(_out, "    USE {} ;\n", net->getSigType().getString());
    fmt::print(
        _out, "    DIRECTION {} ;\n", dbIoType(dbIoType::INOUT).getString());
    for (dbSWire* special_wire : net->getSWires()) {
      fmt::print(_out, "    PORT\n");
      dbSet<dbSBox> wires = special_wire->getWires();
      writeBoxes(db_block, wires, /*indent=*/"      ");
      fmt::print(_out, "    END\n");
    }
    fmt::print(_out, "  END {}\n", net->getName().c_str());
  }
}

void lefout::writeTechBody(dbTech* tech)
{
  assert(tech);

  if (tech->hasNoWireExtAtPin()) {
    fmt::print(_out,
               "NOWIREEXTENSIONATPIN {} ;\n",
               tech->getNoWireExtAtPin().getString());
  }

  if (tech->hasClearanceMeasure()) {
    fmt::print(_out,
               "CLEARANCEMEASURE {} ;\n",
               tech->getClearanceMeasure().getString());
  }

  if (tech->hasUseMinSpacingObs()) {
    fmt::print(_out,
               "USEMINSPACING OBS {} ;\n",
               tech->getUseMinSpacingObs().getString());
  }

  if (tech->hasUseMinSpacingPin()) {
    fmt::print(_out,
               "USEMINSPACING PIN {} ;\n",
               tech->getUseMinSpacingPin().getString());
  }

  if (tech->hasManufacturingGrid()) {
    fmt::print(_out,
               "MANUFACTURINGGRID {:.11g} ;\n",
               lefdist(tech->getManufacturingGrid()));
  }

  dbSet<dbTechLayer> layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator litr;

  for (litr = layers.begin(); litr != layers.end(); ++litr) {
    dbTechLayer* layer = *litr;
    writeLayer(layer);
  }

  writeViaMap(tech, false);
  writeViaMap(tech, true);

  // VIA's not using generate rule and not default
  dbSet<dbTechVia> vias = tech->getVias();
  dbSet<dbTechVia>::iterator vitr;

  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    dbTechVia* via = *vitr;

    if (via->getNonDefaultRule() == nullptr) {
      if (via->getViaGenerateRule() == nullptr) {
        writeVia(via);
      }
    }
  }

  dbSet<dbTechViaRule> via_rules = tech->getViaRules();
  dbSet<dbTechViaRule>::iterator vritr;

  for (vritr = via_rules.begin(); vritr != via_rules.end(); ++vritr) {
    dbTechViaRule* rule = *vritr;
    writeTechViaRule(rule);
  }

  dbSet<dbTechViaGenerateRule> via_gen_rules = tech->getViaGenerateRules();
  dbSet<dbTechViaGenerateRule>::iterator vgritr;

  for (vgritr = via_gen_rules.begin(); vgritr != via_gen_rules.end();
       ++vgritr) {
    dbTechViaGenerateRule* rule = *vgritr;
    writeTechViaGenerateRule(rule);
  }

  // VIA's using generate rule
  vias = tech->getVias();

  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    dbTechVia* via = *vitr;

    if (via->getNonDefaultRule() == nullptr) {
      if (via->getViaGenerateRule() != nullptr) {
        writeVia(via);
      }
    }
  }

  std::vector<dbTechSameNetRule*> srules;
  tech->getSameNetRules(srules);

  if (srules.begin() != srules.end()) {
    fmt::print(_out, "\nSPACING\n");

    std::vector<dbTechSameNetRule*>::iterator sritr;
    for (sritr = srules.begin(); sritr != srules.end(); ++sritr) {
      writeSameNetRule(*sritr);
    }

    fmt::print(_out, "\nEND SPACING\n");
  }

  dbSet<dbTechNonDefaultRule> rules = tech->getNonDefaultRules();
  dbSet<dbTechNonDefaultRule>::iterator ritr;

  for (ritr = rules.begin(); ritr != rules.end(); ++ritr) {
    dbTechNonDefaultRule* rule = *ritr;
    writeNonDefaultRule(tech, rule);
  }
}

void lefout::writeViaMap(dbTech* tech, const bool use_via_cut_class)
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
  fmt::print(_out, "PROPERTYDEFINITIONS\n");
  fmt::print(_out, " LIBRARY LEF58_METALWIDTHVIAMAP STRING\n");
  fmt::print(_out, "  \"METALWIDTHVIAMAP\n");
  if (use_via_cut_class) {
    fmt::print(_out, "   USEVIACUTCLASS\n");
  }
  for (auto via_map : via_map_set) {
    if (via_map->isViaCutClass() != use_via_cut_class) {
      continue;
    }
    if (via_map->getBelowLayerWidthLow() == via_map->getBelowLayerWidthHigh()
        && via_map->getAboveLayerWidthLow()
               == via_map->getAboveLayerWidthHigh()) {
      fmt::print(_out,
                 "   VIA {} {} {} {} {}\n",
                 via_map->getCutLayer()->getName(),
                 lefdist(via_map->getBelowLayerWidthLow()),
                 lefdist(via_map->getAboveLayerWidthLow()),
                 via_map->getViaName(),
                 via_map->isPgVia() ? "PGVIA" : "");
    } else {
      fmt::print(_out,
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
  fmt::print(_out, "   ;\n");
  fmt::print(_out, " \" ;\n");
  fmt::print(_out, "END PROPERTYDEFINITIONS\n");
}

void lefout::writeNonDefaultRule(dbTech* tech, dbTechNonDefaultRule* rule)
{
  std::string name = rule->getName();
  fmt::print(_out, "\nNONDEFAULTRULE {}\n", name.c_str());

  if (rule->getHardSpacing()) {
    fmt::print(_out, "{}", "HARDSPACING ;\n");
  }

  std::vector<dbTechLayerRule*> layer_rules;
  rule->getLayerRules(layer_rules);

  std::vector<dbTechLayerRule*>::iterator litr;
  for (litr = layer_rules.begin(); litr != layer_rules.end(); ++litr) {
    writeLayerRule(*litr);
  }

  std::vector<dbTechVia*> vias;
  rule->getVias(vias);

  std::vector<dbTechVia*>::iterator vitr;
  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    writeVia(*vitr);
  }

  std::vector<dbTechSameNetRule*> srules;
  rule->getSameNetRules(srules);

  if (srules.begin() != srules.end()) {
    fmt::print(_out, "\nSPACING\n");

    std::vector<dbTechSameNetRule*>::iterator sritr;
    for (sritr = srules.begin(); sritr != srules.end(); ++sritr) {
      writeSameNetRule(*sritr);
    }

    fmt::print(_out, "\nEND SPACING\n");
  }

  std::vector<dbTechVia*> use_vias;
  rule->getUseVias(use_vias);

  std::vector<dbTechVia*>::iterator uvitr;
  for (uvitr = use_vias.begin(); uvitr != use_vias.end(); ++uvitr) {
    dbTechVia* via = *uvitr;
    std::string vname = via->getName();
    fmt::print(_out, "USEVIA {} ;\n", vname.c_str());
  }

  std::vector<dbTechViaGenerateRule*> use_rules;
  rule->getUseViaRules(use_rules);

  std::vector<dbTechViaGenerateRule*>::iterator uvritr;
  for (uvritr = use_rules.begin(); uvritr != use_rules.end(); ++uvritr) {
    dbTechViaGenerateRule* rule = *uvritr;
    std::string rname = rule->getName();
    fmt::print(_out, "USEVIARULE {} ;\n", rname.c_str());
  }

  dbSet<dbTechLayer> layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator layitr;

  for (layitr = layers.begin(); layitr != layers.end(); ++layitr) {
    dbTechLayer* layer = *layitr;
    int count;

    if (rule->getMinCuts(layer, count)) {
      std::string lname = layer->getName();
      fmt::print(_out, "MINCUTS {} {} ;\n", lname.c_str(), count);
    }
  }

  fmt::print(_out, "\nEND {}\n", name.c_str());
}

void lefout::writeLayerRule(dbTechLayerRule* rule)
{
  dbTechLayer* layer = rule->getLayer();
  std::string name;
  if (_use_alias && layer->hasAlias()) {
    name = layer->getAlias();
  } else {
    name = layer->getName();
  }
  fmt::print(_out, "\nLAYER {}\n", name.c_str());

  if (rule->getWidth()) {
    fmt::print(_out, "    WIDTH {:.11g} ;\n", lefdist(rule->getWidth()));
  }

  if (rule->getSpacing()) {
    fmt::print(_out, "    SPACING {:.11g} ;\n", lefdist(rule->getSpacing()));
  }

  if (rule->getWireExtension() != 0.0) {
    fmt::print(_out,
               "    WIREEXTENSION {:.11g} ;\n",
               lefdist(rule->getWireExtension()));
  }

  if (rule->getResistance() != 0.0) {
    fmt::print(
        _out, "    RESISTANCE RPERSQ {:.11g} ;\n", rule->getResistance());
  }

  if (rule->getCapacitance() != 0.0) {
    fmt::print(
        _out, "    CAPACITANCE CPERSQDIST {:.11g} ;\n", rule->getCapacitance());
  }

  if (rule->getEdgeCapacitance() != 0.0) {
    fmt::print(
        _out, "      EDGECAPACITANCE {:.11g} ;\n", rule->getEdgeCapacitance());
  }

  fmt::print(_out, "END {}\n", name.c_str());
}

void lefout::writeTechViaRule(dbTechViaRule* rule)
{
  std::string name = rule->getName();
  fmt::print(_out, "\nVIARULE {}\n", name.c_str());

  uint idx;

  for (idx = 0; idx < rule->getViaLayerRuleCount(); ++idx) {
    dbTechViaLayerRule* layrule = rule->getViaLayerRule(idx);
    dbTechLayer* layer = layrule->getLayer();
    std::string lname = layer->getName();
    fmt::print(_out, "    LAYER {} ;\n", lname.c_str());

    if (layrule->getDirection() == dbTechLayerDir::VERTICAL) {
      fmt::print(_out, "      DIRECTION VERTICAL ;\n");
    } else if (layrule->getDirection() == dbTechLayerDir::HORIZONTAL) {
      fmt::print(_out, "      DIRECTION HORIZONTAL ;\n");
    }

    if (layrule->hasWidth()) {
      int minW, maxW;
      layrule->getWidth(minW, maxW);
      fmt::print(_out,
                 "      WIDTH {:.11g} TO {:.11g} ;\n",
                 lefdist(minW),
                 lefdist(maxW));
    }
  }

  for (idx = 0; idx < rule->getViaCount(); ++idx) {
    dbTechVia* via = rule->getVia(idx);
    std::string vname = via->getName();
    fmt::print(_out, "    VIA {} ;\n", vname.c_str());
  }

  fmt::print(_out, "END {}\n", name.c_str());
}

void lefout::writeTechViaGenerateRule(dbTechViaGenerateRule* rule)
{
  std::string name = rule->getName();

  if (rule->isDefault()) {
    fmt::print(_out, "\nVIARULE {} GENERATE DEFAULT\n", name.c_str());
  } else {
    fmt::print(_out, "\nVIARULE {} GENERATE \n", name.c_str());
  }

  uint idx;

  for (idx = 0; idx < rule->getViaLayerRuleCount(); ++idx) {
    dbTechViaLayerRule* layrule = rule->getViaLayerRule(idx);
    dbTechLayer* layer = layrule->getLayer();
    std::string lname = layer->getName();
    fmt::print(_out, "    LAYER {} ;\n", lname.c_str());

    if (layrule->getDirection() == dbTechLayerDir::VERTICAL) {
      fmt::print(_out, "      DIRECTION VERTICAL ;\n");
    } else if (layrule->getDirection() == dbTechLayerDir::HORIZONTAL) {
      fmt::print(_out, "      DIRECTION HORIZONTAL ;\n");
    }

    if (layrule->hasOverhang()) {
      fmt::print(
          _out, "      OVERHANG {:.11g} ;\n", lefdist(layrule->getOverhang()));
    }

    if (layrule->hasMetalOverhang()) {
      fmt::print(_out,
                 "      METALOVERHANG {:.11g} ;\n",
                 lefdist(layrule->getMetalOverhang()));
    }

    if (layrule->hasEnclosure()) {
      int overhang1, overhang2;
      layrule->getEnclosure(overhang1, overhang2);
      fmt::print(_out,
                 "      ENCLOSURE {:.11g} {:.11g} ;\n",
                 lefdist(overhang1),
                 lefdist(overhang2));
    }

    if (layrule->hasWidth()) {
      int minW, maxW;
      layrule->getWidth(minW, maxW);
      fmt::print(_out,
                 "      WIDTH {:.11g} TO {:.11g} ;\n",
                 lefdist(minW),
                 lefdist(maxW));
    }

    if (layrule->hasRect()) {
      Rect r;
      layrule->getRect(r);
      fmt::print(_out,
                 "      RECT  {:.11g} {:.11g}  {:.11g} {:.11g}  ;\n",
                 lefdist(r.xMin()),
                 lefdist(r.yMin()),
                 lefdist(r.xMax()),
                 lefdist(r.yMax()));
    }

    if (layrule->hasSpacing()) {
      int spacing_x, spacing_y;
      layrule->getSpacing(spacing_x, spacing_y);
      fmt::print(_out,
                 "      SPACING {:.11g} BY {:.11g} ;\n",
                 lefdist(spacing_x),
                 lefdist(spacing_y));
    }

    if (layrule->hasResistance()) {
      fmt::print(
          _out, "      RESISTANCE {:.11g} ;\n", layrule->getResistance());
    }
  }

  fmt::print(_out, "END {}\n", name.c_str());
}

void lefout::writeSameNetRule(dbTechSameNetRule* rule)
{
  dbTechLayer* l1 = rule->getLayer1();
  dbTechLayer* l2 = rule->getLayer2();

  std::string n1;
  if (_use_alias && l1->hasAlias()) {
    n1 = l1->getAlias();
  } else {
    n1 = l1->getName();
  }

  std::string n2;
  if (_use_alias && l2->hasAlias()) {
    n2 = l2->getAlias();
  } else {
    n2 = l2->getName();
  }

  if (rule->getAllowStackedVias()) {
    fmt::print(_out,
               "  SAMENET {} {} {:.11g} STACK ;\n",
               n1.c_str(),
               n2.c_str(),
               lefdist(rule->getSpacing()));
  } else {
    fmt::print(_out,
               "  SAMENET {} {} {:.11g} ;\n",
               n1.c_str(),
               n2.c_str(),
               lefdist(rule->getSpacing()));
  }
}

void lefout::writeLayer(dbTechLayer* layer)
{
  std::string name;
  if (_use_alias && layer->hasAlias()) {
    name = layer->getAlias();
  } else {
    name = layer->getName();
  }

  fmt::print(_out, "\nLAYER {}\n", name.c_str());
  fmt::print(_out, "    TYPE {} ;\n", layer->getType().getString());

  if (layer->getNumMasks() > 1) {
    fmt::print(_out, "    MASK {} ;\n", layer->getNumMasks());
  }

  if (layer->getPitch()) {
    fmt::print(_out, "    PITCH {:.11g} ;\n", lefdist(layer->getPitch()));
  }

  if (layer->getWidth()) {
    fmt::print(_out, "    WIDTH {:.11g} ;\n", lefdist(layer->getWidth()));
  }

  if (layer->getWireExtension() != 0.0) {
    fmt::print(_out,
               "    WIREEXTENSION {:.11g} ;\n",
               lefdist(layer->getWireExtension()));
  }

  if (layer->hasArea()) {
    fmt::print(_out, "    AREA {:.11g} ;\n", layer->getArea());
  }

  uint thickness;
  if (layer->getThickness(thickness)) {
    fmt::print(_out, "    THICKNESS {:.11g} ;\n", lefdist(thickness));
  }

  if (layer->hasMaxWidth()) {
    fmt::print(_out, "    MAXWIDTH {:.11g} ;\n", lefdist(layer->getMaxWidth()));
  }

  if (layer->hasMinStep()) {
    fmt::print(_out, "    MINSTEP {:.11g} ;\n", lefdist(layer->getMinStep()));
  }

  if (layer->hasProtrusion()) {
    fmt::print(_out,
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
      fmt::print(_out, "SPACINGTABLE INFLUENCE");
      for (auto rule : inf_rules) {
        rule->writeLef(*this);
      }
      fmt::print(_out, " ;\n");
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
    fmt::print(_out, "    DIRECTION {} ;\n", layer->getDirection().getString());
  }

  if (layer->getResistance() != 0.0) {
    if (layer->getType() == dbTechLayerType::CUT) {
      fmt::print(_out, "    RESISTANCE {:.11g} ;\n", layer->getResistance());
    } else {
      fmt::print(
          _out, "    RESISTANCE RPERSQ {:.11g} ;\n", layer->getResistance());
    }
  }

  if (layer->getCapacitance() != 0.0) {
    fmt::print(_out,
               "    CAPACITANCE CPERSQDIST {:.11g} ;\n",
               layer->getCapacitance());
  }

  if (layer->getEdgeCapacitance() != 0.0) {
    fmt::print(
        _out, "    EDGECAPACITANCE {:.11g} ;\n", layer->getEdgeCapacitance());
  }

  fmt::print(_out, "{}", dbProperty::writeProperties(layer));

  fmt::print(_out, "END {}\n", name.c_str());
}

void lefout::writeVia(dbTechVia* via)
{
  std::string name = via->getName();

  if (via->isDefault()) {
    fmt::print(_out, "\nVIA {} DEFAULT\n", name.c_str());
  } else {
    fmt::print(_out, "\nVIA {}\n", name.c_str());
  }

  if (via->isTopOfStack()) {
    fmt::print(_out, "    TOPOFSTACKONLY\n");
  }

  if (via->getResistance() != 0.0) {
    fmt::print(_out, "    RESISTANCE {:.11g} ;\n", via->getResistance());
  }

  dbTechViaGenerateRule* rule = via->getViaGenerateRule();

  if (rule == nullptr) {
    dbSet<dbBox> boxes = via->getBoxes();
    writeBoxes(nullptr, boxes, "    ");
  } else {
    std::string rname = rule->getName();
    fmt::print(_out, "\n    VIARULE {} \n", rname.c_str());

    const dbViaParams P = via->getViaParams();

    fmt::print(_out,
               " + CUTSIZE {:.11g} {:.11g}\n",
               lefdist(P.getXCutSize()),
               lefdist(P.getYCutSize()));
    std::string top = P.getTopLayer()->getName();
    std::string bot = P.getBottomLayer()->getName();
    std::string cut = P.getCutLayer()->getName();
    fmt::print(
        _out, " + LAYERS {} {} {}\n", bot.c_str(), cut.c_str(), top.c_str());
    fmt::print(_out,
               " + CUTSPACING {:.11g} {:.11g}\n",
               lefdist(P.getXCutSpacing()),
               lefdist(P.getYCutSpacing()));
    fmt::print(_out,
               " + ENCLOSURE {:.11g} {:.11g} {:.11g} {:.11g}\n",
               lefdist(P.getXBottomEnclosure()),
               lefdist(P.getYBottomEnclosure()),
               lefdist(P.getXTopEnclosure()),
               lefdist(P.getYTopEnclosure()));

    if ((P.getNumCutRows() != 1) || (P.getNumCutCols() != 1)) {
      fmt::print(
          _out, " + ROWCOL {} {}\n", P.getNumCutRows(), P.getNumCutCols());
    }

    if ((P.getXOrigin() != 0) || (P.getYOrigin() != 0)) {
      fmt::print(_out,
                 " + ORIGIN {:.11g} {:.11g}\n",
                 lefdist(P.getXOrigin()),
                 lefdist(P.getYOrigin()));
    }

    if ((P.getXTopOffset() != 0) || (P.getYTopOffset() != 0)
        || (P.getXBottomOffset() != 0) || (P.getYBottomOffset() != 0)) {
      fmt::print(_out,
                 " + OFFSET {:.11g} {:.11g} {:.11g} {:.11g}\n",
                 lefdist(P.getXBottomOffset()),
                 lefdist(P.getYBottomOffset()),
                 lefdist(P.getXTopOffset()),
                 lefdist(P.getYTopOffset()));
    }

    std::string pname = via->getPattern();
    if (strcmp(pname.c_str(), "") != 0) {
      fmt::print(_out, " + PATTERNNAME {}\n", pname.c_str());
    }
  }

  fmt::print(_out, "END {}\n", name.c_str());
}

void lefout::writeLibBody(dbLib* lib)
{
  dbSet<dbSite> sites = lib->getSites();
  dbSet<dbSite>::iterator site_itr;

  for (site_itr = sites.begin(); site_itr != sites.end(); ++site_itr) {
    dbSite* site = *site_itr;
    writeSite(site);
  }

  dbSet<dbMaster> masters = lib->getMasters();
  dbSet<dbMaster>::iterator master_itr;

  for (master_itr = masters.begin(); master_itr != masters.end();
       ++master_itr) {
    dbMaster* master = *master_itr;
    if (_write_marked_masters && !master->isMarked()) {
      continue;
    }
    writeMaster(master);
  }
}

void lefout::writeSite(dbSite* site)
{
  std::string n = site->getName();

  fmt::print(_out, "SITE {}\n", n.c_str());
  dbSiteClass sclass = site->getClass();
  fmt::print(_out, "    CLASS {} ;\n", sclass.getString());

  if (site->getSymmetryX() || site->getSymmetryY() || site->getSymmetryR90()) {
    fmt::print(_out, "{}", "    SYMMETRY");

    if (site->getSymmetryX()) {
      fmt::print(_out, "{}", " X");
    }

    if (site->getSymmetryY()) {
      fmt::print(_out, "{}", " Y");
    }

    if (site->getSymmetryR90()) {
      fmt::print(_out, "{}", " R90");
    }

    fmt::print(_out, "{}", " ;\n");
  }

  if (site->getWidth() || site->getHeight()) {
    fmt::print(_out,
               "    SIZE {:.11g} BY {:.11g} ;\n",
               lefdist(site->getWidth()),
               lefdist(site->getHeight()));
  }

  fmt::print(_out, "END {}\n", n.c_str());
}

void lefout::writeMaster(dbMaster* master)
{
  std::string name = master->getName();

  if (_use_master_ids) {
    fmt::print(_out,
               "\nMACRO M{}\n",
               static_cast<std::uint32_t>(master->getMasterId()));
  } else {
    fmt::print(_out, "\nMACRO {}\n", name.c_str());
  }

  fmt::print(_out, "    CLASS {} ;\n", master->getType().getString());

  const odb::Point origin = master->getOrigin();

  if (origin != Point()) {
    fmt::print(_out,
               "    ORIGIN {:.11g} {:.11g} ;\n",
               lefdist(origin.x()),
               lefdist(origin.y()));
  }

  if (master->getEEQ()) {
    std::string eeq = master->getEEQ()->getName();
    if (_use_master_ids) {
      fmt::print(_out,
                 "    EEQ M{} ;\n",
                 static_cast<std::uint32_t>(master->getEEQ()->getMasterId()));
    } else {
      fmt::print(_out, "    EEQ {} ;\n", eeq.c_str());
    }
  }

  if (master->getLEQ()) {
    std::string leq = master->getLEQ()->getName();
    if (_use_master_ids) {
      fmt::print(_out,
                 "    LEQ M{} ;\n",
                 static_cast<std::uint32_t>(master->getLEQ()->getMasterId()));
    } else {
      fmt::print(_out, "    LEQ {} ;\n", leq.c_str());
    }
  }

  int w = master->getWidth();
  int h = master->getHeight();

  if ((w != 0) || (h != 0)) {
    fmt::print(_out, "    SIZE {:.11g} BY {:.11g} ;\n", lefdist(w), lefdist(h));
  }

  if (master->getSymmetryX() || master->getSymmetryY()
      || master->getSymmetryR90()) {
    fmt::print(_out, "{}", "    SYMMETRY");

    if (master->getSymmetryX()) {
      fmt::print(_out, "{}", " X");
    }

    if (master->getSymmetryY()) {
      fmt::print(_out, "{}", " Y");
    }

    if (master->getSymmetryR90()) {
      fmt::print(_out, "{}", " R90");
    }

    fmt::print(_out, "{}", " ;\n");
  }

  if (origin != Point()) {
    dbTransform t(Point(-origin.x(), -origin.y()));
    master->transform(t);
  }

  if (master->getSite()) {
    std::string site = master->getSite()->getName();
    fmt::print(_out, "    SITE {} ;\n", site.c_str());
  }

  dbSet<dbMTerm> mterms = master->getMTerms();
  dbSet<dbMTerm>::iterator mitr;

  for (mitr = mterms.begin(); mitr != mterms.end(); ++mitr) {
    dbMTerm* mterm = *mitr;
    writeMTerm(mterm);
  }

  dbSet<dbPolygon> poly_obs = master->getPolygonObstructions();
  dbSet<dbBox> obs = master->getObstructions(false);

  if (poly_obs.begin() != poly_obs.end() || obs.begin() != obs.end()) {
    fmt::print(_out, "{}", "    OBS\n");
    writeBoxes(nullptr, poly_obs, "      ");
    writeBoxes(nullptr, obs, "      ");
    fmt::print(_out, "{}", "    END\n");
  }

  if (origin != Point()) {
    dbTransform t(origin);
    master->transform(t);
  }

  if (_use_master_ids) {
    fmt::print(
        _out, "END M{}\n", static_cast<std::uint32_t>(master->getMasterId()));
  } else {
    fmt::print(_out, "END {}\n", name.c_str());
  }
}

void lefout::writeMTerm(dbMTerm* mterm)
{
  std::string name = mterm->getName();

  fmt::print(_out, "    PIN {}\n", name.c_str());
  fmt::print(_out, "        DIRECTION {} ; \n", mterm->getIoType().getString());
  fmt::print(_out, "        USE {} ; \n", mterm->getSigType().getString());

  mterm->writeAntennaLef(*this);
  dbSet<dbMPin> pins = mterm->getMPins();
  dbSet<dbMPin>::iterator pitr;

  for (pitr = pins.begin(); pitr != pins.end(); ++pitr) {
    dbMPin* pin = *pitr;

    dbSet<dbPolygon> poly_geoms = pin->getPolygonGeometry();
    dbSet<dbBox> geoms = pin->getGeometry(false);

    if (poly_geoms.begin() != poly_geoms.end()
        || geoms.begin() != geoms.end()) {
      fmt::print(_out, "        PORT\n");
      writeBoxes(nullptr, poly_geoms, "            ");
      writeBoxes(nullptr, geoms, "            ");
      fmt::print(_out, "        END\n");
    }
  }

  fmt::print(_out, "    END {}\n", name.c_str());
}

void lefout::writePropertyDefinition(dbProperty* prop)
{
  std::string propName = prop->getName();
  dbObjectType owner_type = prop->getPropOwner()->getObjectType();
  dbProperty::Type prop_type = prop->getType();
  std::string objectType, propType, value;
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
  fmt::print(_out,
             "    {} {} {} ",
             objectType.c_str(),
             propName.c_str(),
             propType.c_str());
  if (owner_type == dbLibObj) {
    fmt::print(_out, "{}", "\n        ");
    fmt::print(_out, "{}", dbProperty::writePropValue(prop));
    fmt::print(_out, "{}", "\n    ");
  }

  fmt::print(_out, "{}", ";\n");
}

inline void lefout::writeObjectPropertyDefinitions(
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
    writePropertyDefinition(prop);
  }
}

void lefout::writePropertyDefinitions(dbLib* lib)
{
  std::unordered_map<std::string, int16_t> propertiesMap;
  dbTech* tech = lib->getTech();

  fmt::print(_out, "{}", "\nPROPERTYDEFINITIONS\n");

  // writing property definitions of objectType LAYER
  for (dbTechLayer* layer : tech->getLayers()) {
    writeObjectPropertyDefinitions(layer, propertiesMap);
  }

  // writing property definitions of objectType LIBRARY
  writeObjectPropertyDefinitions(lib, propertiesMap);

  // writing property definitions of objectType MACRO
  for (dbMaster* master : lib->getMasters()) {
    writeObjectPropertyDefinitions(master, propertiesMap);
    for (dbMTerm* term : master->getMTerms()) {
      for (dbMPin* pin : term->getMPins()) {
        writeObjectPropertyDefinitions(pin, propertiesMap);
      }
    }
  }

  // writing property definitions of objectType VIA
  for (dbTechVia* via : tech->getVias()) {
    writeObjectPropertyDefinitions(via, propertiesMap);
  }

  // writing property definitions of objectType VIARULE
  for (dbTechViaRule* vrule : tech->getViaRules()) {
    writeObjectPropertyDefinitions(vrule, propertiesMap);
  }

  // writing property definitions of objectType NONDEFAULTRULE
  for (dbTechNonDefaultRule* nrule : tech->getNonDefaultRules()) {
    writeObjectPropertyDefinitions(nrule, propertiesMap);
  }

  fmt::print(_out, "{}", "END PROPERTYDEFINITIONS\n\n");
}

void lefout::writeTech(dbTech* tech)
{
  _dist_factor = 1.0 / tech->getDbUnitsPerMicron();
  _area_factor = _dist_factor * _dist_factor;
  writeTechBody(tech);

  fmt::print(_out, "END LIBRARY\n");
}

void lefout::writeLib(dbLib* lib)
{
  _dist_factor = 1.0 / lib->getDbUnitsPerMicron();
  _area_factor = _dist_factor * _dist_factor;
  writeHeader(lib);
  writeLibBody(lib);
  fmt::print(_out, "END LIBRARY\n");
}

void lefout::writeTechAndLib(dbLib* lib)
{
  _dist_factor = 1.0 / lib->getDbUnitsPerMicron();
  _area_factor = _dist_factor * _dist_factor;
  dbTech* tech = lib->getTech();
  writeHeader(lib);
  writeTechBody(tech);
  writeLibBody(lib);
  fmt::print(_out, "END LIBRARY\n");
}

void lefout::writeAbstractLef(dbBlock* db_block)
{
  utl::SetAndRestore set_dist(_dist_factor,
                              1.0 / db_block->getDbUnitsPerMicron());
  utl::SetAndRestore set_area(_area_factor, _dist_factor * _dist_factor);

  writeHeader(db_block);
  writeBlock(db_block);
  fmt::print(_out, "END LIBRARY\n");
}
