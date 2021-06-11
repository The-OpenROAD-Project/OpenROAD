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

#include "lefout.h"

#include <stdio.h>

#include <algorithm>
#include <unordered_map>

#include "db.h"
#include "dbTransform.h"

using namespace odb;

void lefout::writeVersion(const char* version)
{
  fprintf(_out, "VERSION %s ;\n", version);
}

void lefout::writeBoxes(void* boxes, const char* indent)
{
  dbSet<dbBox>* geoms = (dbSet<dbBox>*) boxes;
  dbSet<dbBox>::iterator bitr;
  dbTechLayer* cur_layer = NULL;

  for (bitr = geoms->begin(); bitr != geoms->end(); ++bitr) {
    dbBox* box = *bitr;
    dbTechLayer* layer = box->getTechLayer();

    if (box->isVia()) {
      dbTechVia* via = box->getTechVia();
      assert(via);
      int x, y;
      box->getViaXY(x, y);
      std::string n = via->getName();
      fprintf(_out,
              "%sVIA %g %g %s ;\n",
              indent,
              lefdist(x),
              lefdist(y),
              n.c_str());
      cur_layer = NULL;
    } else {
      int x1 = box->xMin();
      int y1 = box->yMin();
      int x2 = box->xMax();
      int y2 = box->yMax();

      std::string n;
      if (_use_alias && layer->hasAlias())
        n = layer->getAlias();
      else
        n = layer->getName();

      if (cur_layer != layer) {
        std::string n;

        if (_use_alias && layer->hasAlias())
          n = layer->getAlias();
        else
          n = layer->getName();

        fprintf(_out, "%sLAYER %s ;\n", indent, n.c_str());
        cur_layer = layer;
      }

      fprintf(_out,
              "%s  RECT  %g %g %g %g ;\n",
              indent,
              lefdist(x1),
              lefdist(y1),
              lefdist(x2),
              lefdist(y2));
    }
  }
}

void lefout::writeHeader(dbBlock* db_block)
{
  dbTech* tech = db_block->getDataBase()->getTech();

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

  writeVersion(tech->getLefVersionStr());
  writeNameCaseSensitive(tech->getNamesCaseSensitive());
  writeBusBitChars(left_bus_delimeter, right_bus_delimeter);
  writeDividerChar(hier_delimeter);

  if (tech->getLefUnits()) {
    writeUnits(tech->getLefUnits());
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

  dbSet<dbTechLayerSpacingRule> v54_rules;
  dbSet<dbTechLayerSpacingRule>::iterator ritr;

  std::vector<dbTechV55InfluenceEntry*> inf_rules;
  std::vector<dbTechV55InfluenceEntry*>::const_iterator infitr;

  if (layer->getV54SpacingRules(v54_rules)) {
    for (ritr = v54_rules.begin(); ritr != v54_rules.end(); ++ritr)
      (*ritr)->writeLef(*this);
  }

  if (layer->hasV55SpacingRules()) {
    layer->printV55SpacingRules(*this);
    if (layer->getV55InfluenceRules(inf_rules)) {
      fprintf(_out, "SPACINGTABLE INFLUENCE");
      for (infitr = inf_rules.begin(); infitr != inf_rules.end(); ++infitr)
        (*infitr)->writeLef(*this);
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

  if (layer->getResistance() != 0.0)
    fprintf(_out, "    RESISTANCE RPERSQ %g ;\n", layer->getResistance());

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
    writeBoxes(&boxes, "    ");
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
    writeBoxes(&obs, "      ");
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
      writeBoxes(&geoms, "            ");
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
    fprintf(stderr, "Cannot open LEF file %s\n", lef_file);
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
    fprintf(stderr, "Cannot open LEF file %s\n", lef_file);
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
    fprintf(stderr, "Cannot open LEF file %s\n", lef_file);
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
    fprintf(stderr, "Cannot open LEF file %s\n", lef_file);
    return false;
  }
}