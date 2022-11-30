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

#include <stdio.h>

#include <algorithm>
#include <boost/polygon/polygon.hpp>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"

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

void lefout::writeVersion(const char* version)
{
  fprintf(_out, "VERSION %s ;\n", version);
}

template <typename GenericBox>
void lefout::writeBoxes(dbSet<GenericBox>& boxes, const char* indent)
{
  dbTechLayer* cur_layer = NULL;

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
        via_name = box->getBlockVia()->getName();
      }

      int x, y;
      box->getViaXY(x, y);
      fprintf(_out,
              "%sVIA %g %g %s ;\n",
              indent,
              lefdist(x),
              lefdist(y),
              via_name.c_str());
      cur_layer = NULL;
    } else {
      std::string layer_name;
      if (_use_alias && layer->hasAlias())
        layer_name = layer->getAlias();
      else
        layer_name = layer->getName();

      if (cur_layer != layer) {
        fprintf(_out, "%sLAYER %s ;\n", indent, layer_name.c_str());
        cur_layer = layer;
      }

      writeBox(indent, box);
    }
  }
}

void lefout::writeBox(const std::string& indent, dbBox* box)
{
  int x1 = box->xMin();
  int y1 = box->yMin();
  int x2 = box->xMax();
  int y2 = box->yMax();

  fprintf(_out,
          "%s  RECT  %g %g %g %g ;\n",
          indent.c_str(),
          lefdist(x1),
          lefdist(y1),
          lefdist(x2),
          lefdist(y2));
}

void lefout::writeRect(const std::string& indent,
                       const boost::polygon::rectangle_data<int>& rect)
{
  int x1 = boost::polygon::xl(rect);
  int y1 = boost::polygon::yl(rect);
  int x2 = boost::polygon::xh(rect);
  int y2 = boost::polygon::yh(rect);

  fprintf(_out,
          "%s  RECT  %g %g %g %g ;\n",
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

  if (left_bus_delimeter == 0)
    left_bus_delimeter = '[';

  if (right_bus_delimeter == 0)
    right_bus_delimeter = ']';

  if (hier_delimeter == 0)
    hier_delimeter = '|';

  writeVersion("5.8");
  writeBusBitChars(left_bus_delimeter, right_bus_delimeter);
  writeDividerChar(hier_delimeter);
  writeUnits(/*database_units = */ db_block->getDbUnitsPerMicron());
}

void lefout::writeObstructions(dbBlock* db_block)
{
  ObstructionMap obstructions;
  getObstructions(db_block, obstructions);

  fprintf(_out, "  OBS\n");
  dbBox* block_bounding_box = db_block->getBBox();
  for (const auto& [tech_layer, polySet] : obstructions) {
    fprintf(_out, "    LAYER %s ;\n", tech_layer->getName().c_str());

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
  fprintf(_out, "  END\n");
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
    dbTransform trans;
    inst->getTransform(trans);

    // Add insts obstructions
    for (auto* obs : inst->getMaster()->getObstructions()) {
      Rect obs_rect = obs->getBox();
      trans.apply(obs_rect);
      insertObstruction(obs->getTechLayer(), obs_rect, obstructions);
    }

    // Add inst Iterms to obstructions
    for (auto* iterm : inst->getITerms()) {
      dbShape shape;
      dbITermShapeItr iterm_shape_itr;
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
  dbTech* tech = lib->getDb()->getTech();

  char left_bus_delimeter = 0;
  char right_bus_delimeter = 0;
  char hier_delimeter = lib->getHierarchyDelimeter();

  lib->getBusDelimeters(left_bus_delimeter, right_bus_delimeter);

  if (left_bus_delimeter == 0)
    left_bus_delimeter = '[';

  if (right_bus_delimeter == 0)
    right_bus_delimeter = ']';

  if (hier_delimeter == 0)
    hier_delimeter = '|';

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
  fprintf(_out, "DIVIDERCHAR \"%c\" ;\n", hier_delimeter);
}

void lefout::writeUnits(int database_units)
{
  fprintf(_out, "UNITS\n");
  fprintf(_out, "    DATABASE MICRONS %d ;\n", database_units);
  fprintf(_out, "END UNITS\n");
}

void lefout::writeBusBitChars(char left_bus_delimeter, char right_bus_delimeter)
{
  fprintf(_out,
          "BUSBITCHARS \"%c%c\" ;\n",
          left_bus_delimeter,
          right_bus_delimeter);
}

void lefout::writeNameCaseSensitive(const dbOnOffType on_off_type)
{
  fprintf(_out, "NAMESCASESENSITIVE %s ;\n", on_off_type.getString());
}

void lefout::writeBlock(dbBlock* db_block)
{
  dbBox* bounding_box = db_block->getBBox();
  double origin_x = lefdist(bounding_box->xMin());
  double origin_y = lefdist(bounding_box->yMin());
  double size_x = lefdist(bounding_box->getDX());
  double size_y = lefdist(bounding_box->getDY());

  fprintf(_out, "MACRO %s\n", db_block->getName().c_str());
  fprintf(_out, "  FOREIGN %s 0 0 ;\n", db_block->getName().c_str());
  fprintf(_out, "  CLASS BLOCK ;\n");
  fprintf(_out, "  ORIGIN %g %g ;\n", origin_x, origin_y);
  fprintf(_out, "  SIZE %g BY %g ;\n", size_x, size_y);
  writePins(db_block);
  writeObstructions(db_block);
  fprintf(_out, "END %s\n", db_block->getName().c_str());
}

void lefout::writePins(dbBlock* db_block)
{
  writePowerPins(db_block);
  writeBlockTerms(db_block);
}

void lefout::writeBlockTerms(dbBlock* db_block)
{
  for (dbBTerm* b_term : db_block->getBTerms()) {
    fprintf(_out, "  PIN %s\n", b_term->getName().c_str());
    fprintf(_out, "    DIRECTION %s ;\n", b_term->getIoType().getString());
    fprintf(_out, "    USE %s ;\n", b_term->getSigType().getString());
    for (dbBPin* db_b_pin : b_term->getBPins()) {
      fprintf(_out, "    PORT\n");
      dbSet<dbBox> term_pins = db_b_pin->getBoxes();
      writeBoxes(term_pins, "      ");
      fprintf(_out, "    END\n");
    }
    fprintf(_out, "  END %s\n", b_term->getName().c_str());
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
    fprintf(_out, "  PIN %s\n", net->getName().c_str());
    fprintf(_out, "    USE %s ;\n", net->getSigType().getString());
    fprintf(
        _out, "    DIRECTION %s ;\n", dbIoType(dbIoType::INOUT).getString());
    for (dbSWire* special_wire : net->getSWires()) {
      fprintf(_out, "    PORT\n");
      dbSet<dbSBox> wires = special_wire->getWires();
      writeBoxes(wires, /*indent=*/"      ");
      fprintf(_out, "    END\n");
    }
    fprintf(_out, "  END %s\n", net->getName().c_str());
  }
}

void lefout::writeTech(dbTech* tech)
{
  assert(tech);

  if (tech->hasNoWireExtAtPin())
    fprintf(_out,
            "NOWIREEXTENSIONATPIN %s ;\n",
            tech->getNoWireExtAtPin().getString());

  if (tech->hasClearanceMeasure())
    fprintf(_out,
            "CLEARANCEMEASURE %s ;\n",
            tech->getClearanceMeasure().getString());

  if (tech->hasUseMinSpacingObs())
    fprintf(_out,
            "USEMINSPACING OBS %s ;\n",
            tech->getUseMinSpacingObs().getString());

  if (tech->hasUseMinSpacingPin())
    fprintf(_out,
            "USEMINSPACING PIN %s ;\n",
            tech->getUseMinSpacingPin().getString());

  if (tech->hasManufacturingGrid())
    fprintf(_out,
            "MANUFACTURINGGRID %.4g ;\n",
            lefdist(tech->getManufacturingGrid()));

  dbSet<dbTechLayer> layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator litr;

  for (litr = layers.begin(); litr != layers.end(); ++litr) {
    dbTechLayer* layer = *litr;
    writeLayer(layer);
  }

  // VIA's not using generate rule and not default
  dbSet<dbTechVia> vias = tech->getVias();
  dbSet<dbTechVia>::iterator vitr;

  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    dbTechVia* via = *vitr;

    if (via->getNonDefaultRule() == NULL)
      if (via->getViaGenerateRule() == NULL)
        writeVia(via);
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

    if (via->getNonDefaultRule() == NULL)
      if (via->getViaGenerateRule() != NULL)
        writeVia(via);
  }

  std::vector<dbTechSameNetRule*> srules;
  tech->getSameNetRules(srules);

  if (srules.begin() != srules.end()) {
    fprintf(_out, "\nSPACING\n");

    std::vector<dbTechSameNetRule*>::iterator sritr;
    for (sritr = srules.begin(); sritr != srules.end(); ++sritr)
      writeSameNetRule(*sritr);

    fprintf(_out, "\nEND SPACING\n");
  }

  dbSet<dbTechNonDefaultRule> rules = tech->getNonDefaultRules();
  dbSet<dbTechNonDefaultRule>::iterator ritr;

  for (ritr = rules.begin(); ritr != rules.end(); ++ritr) {
    dbTechNonDefaultRule* rule = *ritr;
    writeNonDefaultRule(tech, rule);
  }
}

void lefout::writeNonDefaultRule(dbTech* tech, dbTechNonDefaultRule* rule)
{
  std::string name = rule->getName();
  fprintf(_out, "\nNONDEFAULTRULE %s\n", name.c_str());

  if (rule->getHardSpacing())
    fprintf(_out, "HARDSPACING ;\n");

  std::vector<dbTechLayerRule*> layer_rules;
  rule->getLayerRules(layer_rules);

  std::vector<dbTechLayerRule*>::iterator litr;
  for (litr = layer_rules.begin(); litr != layer_rules.end(); ++litr)
    writeLayerRule(*litr);

  std::vector<dbTechVia*> vias;
  rule->getVias(vias);

  std::vector<dbTechVia*>::iterator vitr;
  for (vitr = vias.begin(); vitr != vias.end(); ++vitr)
    writeVia(*vitr);

  std::vector<dbTechSameNetRule*> srules;
  rule->getSameNetRules(srules);

  if (srules.begin() != srules.end()) {
    fprintf(_out, "\nSPACING\n");

    std::vector<dbTechSameNetRule*>::iterator sritr;
    for (sritr = srules.begin(); sritr != srules.end(); ++sritr)
      writeSameNetRule(*sritr);

    fprintf(_out, "\nEND SPACING\n");
  }

  std::vector<dbTechVia*> use_vias;
  rule->getUseVias(use_vias);

  std::vector<dbTechVia*>::iterator uvitr;
  for (uvitr = use_vias.begin(); uvitr != use_vias.end(); ++uvitr) {
    dbTechVia* via = *uvitr;
    std::string vname = via->getName();
    fprintf(_out, "USEVIA %s ;\n", vname.c_str());
  }

  std::vector<dbTechViaGenerateRule*> use_rules;
  rule->getUseViaRules(use_rules);

  std::vector<dbTechViaGenerateRule*>::iterator uvritr;
  for (uvritr = use_rules.begin(); uvritr != use_rules.end(); ++uvritr) {
    dbTechViaGenerateRule* rule = *uvritr;
    std::string rname = rule->getName();
    fprintf(_out, "USEVIARULE %s ;\n", rname.c_str());
  }

  dbSet<dbTechLayer> layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator layitr;

  for (layitr = layers.begin(); layitr != layers.end(); ++layitr) {
    dbTechLayer* layer = *layitr;
    int count;

    if (rule->getMinCuts(layer, count)) {
      std::string lname = layer->getName();
      fprintf(_out, "MINCUTS %s %d ;\n", lname.c_str(), count);
    }
  }

  fprintf(_out, "\nEND %s\n", name.c_str());
}

void lefout::writeLayerRule(dbTechLayerRule* rule)
{
  dbTechLayer* layer = rule->getLayer();
  std::string name;
  if (_use_alias && layer->hasAlias())
    name = layer->getAlias();
  else
    name = layer->getName();
  fprintf(_out, "\nLAYER %s\n", name.c_str());

  if (rule->getWidth())
    fprintf(_out, "    WIDTH %g ;\n", lefdist(rule->getWidth()));

  if (rule->getSpacing())
    fprintf(_out, "    SPACING %g ;\n", lefdist(rule->getSpacing()));

  if (rule->getWireExtension() != 0.0)
    fprintf(
        _out, "    WIREEXTENSION %g ;\n", lefdist(rule->getWireExtension()));

  if (rule->getResistance() != 0.0)
    fprintf(_out, "    RESISTANCE RPERSQ %g ;\n", rule->getResistance());

  if (rule->getCapacitance() != 0.0)
    fprintf(_out, "    CAPACITANCE CPERSQDIST %g ;\n", rule->getCapacitance());

  if (rule->getEdgeCapacitance() != 0.0)
    fprintf(_out, "      EDGECAPACITANCE %g ;\n", rule->getEdgeCapacitance());

  fprintf(_out, "END %s\n", name.c_str());
}

void lefout::writeTechViaRule(dbTechViaRule* rule)
{
  std::string name = rule->getName();
  fprintf(_out, "\nVIARULE %s\n", name.c_str());

  uint idx;

  for (idx = 0; idx < rule->getViaLayerRuleCount(); ++idx) {
    dbTechViaLayerRule* layrule = rule->getViaLayerRule(idx);
    dbTechLayer* layer = layrule->getLayer();
    std::string lname = layer->getName();
    fprintf(_out, "    LAYER %s ;\n", lname.c_str());

    if (layrule->getDirection() == dbTechLayerDir::VERTICAL)
      fprintf(_out, "      DIRECTION VERTICAL ;\n");
    else if (layrule->getDirection() == dbTechLayerDir::HORIZONTAL)
      fprintf(_out, "      DIRECTION HORIZONTAL ;\n");

    if (layrule->hasWidth()) {
      int minW, maxW;
      layrule->getWidth(minW, maxW);
      fprintf(_out, "      WIDTH %g TO %g ;\n", lefdist(minW), lefdist(maxW));
    }
  }

  for (idx = 0; idx < rule->getViaCount(); ++idx) {
    dbTechVia* via = rule->getVia(idx);
    std::string vname = via->getName();
    fprintf(_out, "    VIA %s ;\n", vname.c_str());
  }

  fprintf(_out, "END %s\n", name.c_str());
}

void lefout::writeTechViaGenerateRule(dbTechViaGenerateRule* rule)
{
  std::string name = rule->getName();

  if (rule->isDefault())
    fprintf(_out, "\nVIARULE %s GENERATE DEFAULT\n", name.c_str());
  else
    fprintf(_out, "\nVIARULE %s GENERATE \n", name.c_str());

  uint idx;

  for (idx = 0; idx < rule->getViaLayerRuleCount(); ++idx) {
    dbTechViaLayerRule* layrule = rule->getViaLayerRule(idx);
    dbTechLayer* layer = layrule->getLayer();
    std::string lname = layer->getName();
    fprintf(_out, "    LAYER %s ;\n", lname.c_str());

    if (layrule->getDirection() == dbTechLayerDir::VERTICAL)
      fprintf(_out, "      DIRECTION VERTICAL ;\n");
    else if (layrule->getDirection() == dbTechLayerDir::HORIZONTAL)
      fprintf(_out, "      DIRECTION HORIZONTAL ;\n");

    if (layrule->hasOverhang())
      fprintf(_out, "      OVERHANG %g ;\n", lefdist(layrule->getOverhang()));

    if (layrule->hasMetalOverhang())
      fprintf(_out,
              "      METALOVERHANG %g ;\n",
              lefdist(layrule->getMetalOverhang()));

    if (layrule->hasEnclosure()) {
      int overhang1, overhang2;
      layrule->getEnclosure(overhang1, overhang2);
      fprintf(_out,
              "      ENCLOSURE %g %g ;\n",
              lefdist(overhang1),
              lefdist(overhang2));
    }

    if (layrule->hasWidth()) {
      int minW, maxW;
      layrule->getWidth(minW, maxW);
      fprintf(_out, "      WIDTH %g TO %g ;\n", lefdist(minW), lefdist(maxW));
    }

    if (layrule->hasRect()) {
      Rect r;
      layrule->getRect(r);
      fprintf(_out,
              "      RECT  %g %g  %g %g  ;\n",
              lefdist(r.xMin()),
              lefdist(r.yMin()),
              lefdist(r.xMax()),
              lefdist(r.yMax()));
    }

    if (layrule->hasSpacing()) {
      int spacing_x, spacing_y;
      layrule->getSpacing(spacing_x, spacing_y);
      fprintf(_out,
              "      SPACING %g BY %g ;\n",
              lefdist(spacing_x),
              lefdist(spacing_y));
    }

    if (layrule->hasResistance())
      fprintf(_out, "      RESISTANCE %g ;\n", layrule->getResistance());
  }

  fprintf(_out, "END %s\n", name.c_str());
}

void lefout::writeSameNetRule(dbTechSameNetRule* rule)
{
  dbTechLayer* l1 = rule->getLayer1();
  dbTechLayer* l2 = rule->getLayer2();

  std::string n1;
  if (_use_alias && l1->hasAlias())
    n1 = l1->getAlias();
  else
    n1 = l1->getName();

  std::string n2;
  if (_use_alias && l2->hasAlias())
    n2 = l2->getAlias();
  else
    n2 = l2->getName();

  if (rule->getAllowStackedVias())
    fprintf(_out,
            "  SAMENET %s %s %g STACK ;\n",
            n1.c_str(),
            n2.c_str(),
            lefdist(rule->getSpacing()));
  else
    fprintf(_out,
            "  SAMENET %s %s %g ;\n",
            n1.c_str(),
            n2.c_str(),
            lefdist(rule->getSpacing()));
}

void lefout::writeLayer(dbTechLayer* layer)
{
  std::string name;
  if (_use_alias && layer->hasAlias())
    name = layer->getAlias();
  else
    name = layer->getName();

  fprintf(_out, "\nLAYER %s\n", name.c_str());
  fprintf(_out, "    TYPE %s ;\n", layer->getType().getString());

  if (layer->getNumMasks() > 1)
    fprintf(_out, "    MASK %u ;\n", layer->getNumMasks());

  if (layer->getPitch())
    fprintf(_out, "    PITCH %g ;\n", lefdist(layer->getPitch()));

  if (layer->getWidth())
    fprintf(_out, "    WIDTH %g ;\n", lefdist(layer->getWidth()));

  if (layer->getWireExtension() != 0.0)
    fprintf(
        _out, "    WIREEXTENSION %g ;\n", lefdist(layer->getWireExtension()));

  if (layer->hasArea())
    fprintf(_out, "    AREA %g ;\n", layer->getArea());

  uint thickness;
  if (layer->getThickness(thickness))
    fprintf(_out, "    THICKNESS %.3f ;\n", lefdist(thickness));

  if (layer->hasMaxWidth())
    fprintf(_out, "    MAXWIDTH %.3f ;\n", lefdist(layer->getMaxWidth()));

  if (layer->hasMinStep())
    fprintf(_out, "    MINSTEP %.3f ;\n", lefdist(layer->getMinStep()));

  if (layer->hasProtrusion())
    fprintf(_out,
            "    PROTRUSIONWIDTH %.3f  LENGTH %.3f  WIDTH %.3f ;\n",
            lefdist(layer->getProtrusionWidth()),
            lefdist(layer->getProtrusionLength()),
            lefdist(layer->getProtrusionFromWidth()));

  for (auto rule : layer->getV54SpacingRules()) {
    rule->writeLef(*this);
  }

  if (layer->hasV55SpacingRules()) {
    layer->printV55SpacingRules(*this);
    auto inf_rules = layer->getV55InfluenceRules();
    if (!inf_rules.empty()) {
      fprintf(_out, "SPACINGTABLE INFLUENCE");
      for (auto rule : inf_rules)
        rule->writeLef(*this);
      fprintf(_out, " ;\n");
    }
  }

  std::vector<dbTechMinCutRule*> cut_rules;
  std::vector<dbTechMinCutRule*>::const_iterator citr;
  if (layer->getMinimumCutRules(cut_rules)) {
    for (citr = cut_rules.begin(); citr != cut_rules.end(); citr++)
      (*citr)->writeLef(*this);
  }

  std::vector<dbTechMinEncRule*> enc_rules;
  std::vector<dbTechMinEncRule*>::const_iterator eitr;
  if (layer->getMinEnclosureRules(enc_rules)) {
    for (eitr = enc_rules.begin(); eitr != enc_rules.end(); eitr++)
      (*eitr)->writeLef(*this);
  }

  layer->writeAntennaRulesLef(*this);

  if (layer->getDirection() != dbTechLayerDir::NONE)
    fprintf(_out, "    DIRECTION %s ;\n", layer->getDirection().getString());

  if (layer->getResistance() != 0.0) {
    if (layer->getType() == dbTechLayerType::CUT) {
      fprintf(_out, "    RESISTANCE %g ;\n", layer->getResistance());
    } else {
      fprintf(_out, "    RESISTANCE RPERSQ %g ;\n", layer->getResistance());
    }
  }

  if (layer->getCapacitance() != 0.0)
    fprintf(_out, "    CAPACITANCE CPERSQDIST %g ;\n", layer->getCapacitance());

  if (layer->getEdgeCapacitance() != 0.0)
    fprintf(_out, "    EDGECAPACITANCE %g ;\n", layer->getEdgeCapacitance());

  dbProperty::writeProperties(layer, _out);

  fprintf(_out, "END %s\n", name.c_str());
}

void lefout::writeVia(dbTechVia* via)
{
  std::string name = via->getName();

  if (via->isDefault())
    fprintf(_out, "\nVIA %s DEFAULT\n", name.c_str());
  else
    fprintf(_out, "\nVIA %s\n", name.c_str());

  if (via->isTopOfStack())
    fprintf(_out, "    TOPOFSTACKONLY\n");

  if (via->getResistance() != 0.0)
    fprintf(_out, "    RESISTANCE %g ;\n", via->getResistance());

  dbTechViaGenerateRule* rule = via->getViaGenerateRule();

  if (rule == NULL) {
    dbSet<dbBox> boxes = via->getBoxes();
    writeBoxes(boxes, "    ");
  } else {
    std::string rname = rule->getName();
    fprintf(_out, "\n    VIARULE %s \n", rname.c_str());

    dbViaParams P;
    via->getViaParams(P);

    fprintf(_out,
            " + CUTSIZE %g %g ",
            lefdist(P.getXCutSize()),
            lefdist(P.getYCutSize()));
    std::string top = P.getTopLayer()->getName();
    std::string bot = P.getBottomLayer()->getName();
    std::string cut = P.getCutLayer()->getName();
    fprintf(_out, " + LAYERS %s %s %s ", bot.c_str(), cut.c_str(), top.c_str());
    fprintf(_out,
            " + CUTSPACING %g %g ",
            lefdist(P.getXCutSpacing()),
            lefdist(P.getYCutSpacing()));
    fprintf(_out,
            " + ENCLOSURE %g %g %g %g ",
            lefdist(P.getXBottomEnclosure()),
            lefdist(P.getYBottomEnclosure()),
            lefdist(P.getXTopEnclosure()),
            lefdist(P.getYTopEnclosure()));

    if ((P.getNumCutRows() != 1) || (P.getNumCutCols() != 1))
      fprintf(_out, " + ROWCOL %d %d ", P.getNumCutRows(), P.getNumCutCols());

    if ((P.getXOrigin() != 0) || (P.getYOrigin() != 0))
      fprintf(_out,
              " + ORIGIN %g %g ",
              lefdist(P.getXOrigin()),
              lefdist(P.getYOrigin()));

    if ((P.getXTopOffset() != 0) || (P.getYTopOffset() != 0)
        || (P.getXBottomOffset() != 0) || (P.getYBottomOffset() != 0))
      fprintf(_out,
              " + OFFSET %g %g %g %g ",
              lefdist(P.getXBottomOffset()),
              lefdist(P.getYBottomOffset()),
              lefdist(P.getXTopOffset()),
              lefdist(P.getYTopOffset()));

    std::string pname = via->getPattern();
    if (strcmp(pname.c_str(), "") != 0)
      fprintf(_out, " + PATTERNNAME %s", pname.c_str());
  }

  fprintf(_out, "END %s\n", name.c_str());
}

void lefout::writeLib(dbLib* lib)
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
    if (_write_marked_masters && !master->isMarked())
      continue;
    writeMaster(master);
  }
}

void lefout::writeSite(dbSite* site)
{
  std::string n = site->getName();

  fprintf(_out, "SITE %s\n", n.c_str());
  dbSiteClass sclass = site->getClass();
  fprintf(_out, "    CLASS %s ;\n", sclass.getString());

  if (site->getSymmetryX() || site->getSymmetryY() || site->getSymmetryR90()) {
    fprintf(_out, "    SYMMETRY");

    if (site->getSymmetryX())
      fprintf(_out, " X");

    if (site->getSymmetryY())
      fprintf(_out, " Y");

    if (site->getSymmetryR90())
      fprintf(_out, " R90");

    fprintf(_out, " ;\n");
  }

  if (site->getWidth() || site->getHeight())
    fprintf(_out,
            "    SIZE %g BY %g ;\n",
            lefdist(site->getWidth()),
            lefdist(site->getHeight()));

  fprintf(_out, "END %s\n", n.c_str());
}

void lefout::writeMaster(dbMaster* master)
{
  std::string name = master->getName();

  if (_use_master_ids)
    fprintf(_out, "\nMACRO M%u\n", master->getMasterId());
  else
    fprintf(_out, "\nMACRO %s\n", name.c_str());

  if (master->getType() != dbMasterType::NONE)
    fprintf(_out, "    CLASS %s ;\n", master->getType().getString());

  int x, y;
  master->getOrigin(x, y);

  if ((x != 0) || (y != 0))
    fprintf(_out, "    ORIGIN %g %g ;\n", lefdist(x), lefdist(y));

  if (master->getEEQ()) {
    std::string eeq = master->getEEQ()->getName();
    if (_use_master_ids)
      fprintf(_out, "    EEQ M%u ;\n", master->getEEQ()->getMasterId());
    else
      fprintf(_out, "    EEQ %s ;\n", eeq.c_str());
  }

  if (master->getLEQ()) {
    std::string leq = master->getLEQ()->getName();
    if (_use_master_ids)
      fprintf(_out, "    LEQ M%u ;\n", master->getLEQ()->getMasterId());
    else
      fprintf(_out, "    LEQ %s ;\n", leq.c_str());
  }

  int w = master->getWidth();
  int h = master->getHeight();

  if ((w != 0) || (h != 0))
    fprintf(_out, "    SIZE %g BY %g ;\n", lefdist(w), lefdist(h));

  if (master->getSymmetryX() || master->getSymmetryY()
      || master->getSymmetryR90()) {
    fprintf(_out, "    SYMMETRY");

    if (master->getSymmetryX())
      fprintf(_out, " X");

    if (master->getSymmetryY())
      fprintf(_out, " Y");

    if (master->getSymmetryR90())
      fprintf(_out, " R90");

    fprintf(_out, " ;\n");
  }

  if ((x != 0) || (y != 0)) {
    dbTransform t(Point(-x, -y));
    master->transform(t);
  }

  if (master->getSite()) {
    std::string site = master->getSite()->getName();
    fprintf(_out, "    SITE %s ;\n", site.c_str());
  }

  dbSet<dbMTerm> mterms = master->getMTerms();
  dbSet<dbMTerm>::iterator mitr;

  for (mitr = mterms.begin(); mitr != mterms.end(); ++mitr) {
    dbMTerm* mterm = *mitr;
    writeMTerm(mterm);
  }

  dbSet<dbBox> obs = master->getObstructions();

  if (obs.begin() != obs.end()) {
    fprintf(_out, "    OBS\n");
    writeBoxes(obs, "      ");
    fprintf(_out, "    END\n");
  }

  if ((x != 0) || (y != 0)) {
    dbTransform t(Point(x, y));
    master->transform(t);
  }

  if (_use_master_ids)
    fprintf(_out, "END M%u\n", master->getMasterId());
  else
    fprintf(_out, "END %s\n", name.c_str());
}

void lefout::writeMTerm(dbMTerm* mterm)
{
  std::string name = mterm->getName();

  fprintf(_out, "    PIN %s\n", name.c_str());
  fprintf(_out, "        DIRECTION %s ; \n", mterm->getIoType().getString());
  fprintf(_out, "        USE %s ; \n", mterm->getSigType().getString());

  mterm->writeAntennaLef(*this);
  dbSet<dbMPin> pins = mterm->getMPins();
  dbSet<dbMPin>::iterator pitr;

  for (pitr = pins.begin(); pitr != pins.end(); ++pitr) {
    dbMPin* pin = *pitr;

    dbSet<dbBox> geoms = pin->getGeometry();

    if (geoms.begin() != geoms.end()) {
      fprintf(_out, "        PORT\n");
      writeBoxes(geoms, "            ");
      fprintf(_out, "        END\n");
    }
  }

  fprintf(_out, "    END %s\n", name.c_str());
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
  fprintf(_out,
          "    %s %s %s ",
          objectType.c_str(),
          propName.c_str(),
          propType.c_str());
  if (owner_type == dbLibObj) {
    fprintf(_out, "\n        ");
    prop->writePropValue(prop, _out);
    fprintf(_out, "\n    ");
  }

  fprintf(_out, ";\n");
}

inline void lefout::writeObjectPropertyDefinitions(
    dbObject* obj,
    std::unordered_map<std::string, short>& propertiesMap)
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
    if (propertiesMap[prop->getName()] & 0x1 << bitNumber)
      continue;
    propertiesMap[prop->getName()] |= 0x1 << bitNumber;
    writePropertyDefinition(prop);
  }
}

void lefout::writePropertyDefinitions(dbLib* lib)
{
  std::unordered_map<std::string, short> propertiesMap;
  dbTech* tech = lib->getDb()->getTech();

  fprintf(_out, "\nPROPERTYDEFINITIONS\n");

  // writing property definitions of objectType LAYER
  for (dbTechLayer* layer : tech->getLayers())
    writeObjectPropertyDefinitions(layer, propertiesMap);

  // writing property definitions of objectType LIBRARY
  writeObjectPropertyDefinitions(lib, propertiesMap);

  // writing property definitions of objectType MACRO
  for (dbMaster* master : lib->getMasters()) {
    writeObjectPropertyDefinitions(master, propertiesMap);
    for (dbMTerm* term : master->getMTerms())
      for (dbMPin* pin : term->getMPins())
        writeObjectPropertyDefinitions(pin, propertiesMap);
  }

  // writing property definitions of objectType VIA
  for (dbTechVia* via : tech->getVias())
    writeObjectPropertyDefinitions(via, propertiesMap);

  // writing property definitions of objectType VIARULE
  for (dbTechViaRule* vrule : tech->getViaRules())
    writeObjectPropertyDefinitions(vrule, propertiesMap);

  // writing property definitions of objectType NONDEFAULTRULE
  for (dbTechNonDefaultRule* nrule : tech->getNonDefaultRules())
    writeObjectPropertyDefinitions(nrule, propertiesMap);

  fprintf(_out, "END PROPERTYDEFINITIONS\n\n");
}

bool lefout::writeTech(dbTech* tech, const char* lef_file)
{
  _out = fopen(lef_file, "w");

  if (_out == NULL) {
    logger_->error(utl::ODB, 1015, "Cannot open LEF file %s\n", lef_file);
    return false;
  }

  _dist_factor = 1.0 / (double) tech->getDbUnitsPerMicron();
  _area_factor = _dist_factor * _dist_factor;
  writeTech(tech);

  fprintf(_out, "END LIBRARY\n");
  fclose(_out);
  return true;
}

bool lefout::writeLib(dbLib* lib, const char* lef_file)
{
  _out = fopen(lef_file, "w");

  if (_out == NULL) {
    logger_->error(utl::ODB, 1033, "Cannot open LEF file %s\n", lef_file);
    return false;
  }

  _dist_factor = 1.0 / (double) lib->getDbUnitsPerMicron();
  _area_factor = _dist_factor * _dist_factor;
  writeHeader(lib);
  writeLib(lib);
  fprintf(_out, "END LIBRARY\n");
  fclose(_out);
  return true;
}

bool lefout::writeTechAndLib(dbLib* lib, const char* lef_file)
{
  _out = fopen(lef_file, "w");

  if (_out == NULL) {
    logger_->error(utl::ODB, 1051, "Cannot open LEF file %s\n", lef_file);
    return false;
  }

  _dist_factor = 1.0 / (double) lib->getDbUnitsPerMicron();
  _area_factor = _dist_factor * _dist_factor;
  dbTech* tech = lib->getDb()->getTech();
  writeHeader(lib);
  writeTech(tech);
  writeLib(lib);
  fprintf(_out, "END LIBRARY\n");
  fclose(_out);

  return true;
}

bool lefout::writeAbstractLef(dbBlock* db_block, const char* lef_file)
{
  _out = fopen(lef_file, "w");
  if (_out == nullptr) {
    logger_->error(utl::ODB, 1072, "Cannot open LEF file %s\n", lef_file);
  }

  double temporary_dist_factor = _dist_factor;
  _dist_factor = 1.0L / db_block->getDbUnitsPerMicron();
  _area_factor = _dist_factor * _dist_factor;

  writeHeader(db_block);
  writeBlock(db_block);
  fprintf(_out, "END LIBRARY\n");
  fclose(_out);

  _dist_factor = temporary_dist_factor;
  _area_factor = _dist_factor * _dist_factor;

  return true;
}
