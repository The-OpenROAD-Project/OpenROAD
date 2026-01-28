// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "defout_impl.h"

#include <sys/stat.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ios>
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "odb/db.h"
#include "odb/dbMap.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "odb/defout.h"
#include "odb/geom.h"
#include "utl/Logger.h"
#include "utl/ScopedTemporaryFile.h"

namespace odb {

namespace {

std::string getPinName(dbBTerm* bterm)
{
  return bterm->getName();
}

std::string getPinName(dbITerm* iterm)
{
  return iterm->getMTerm()->getName();
}

static const int max_name_length = 256;

template <typename T>
std::vector<T*> sortedSet(dbSet<T>& to_sort)
{
  std::vector<T*> sorted(to_sort.begin(), to_sort.end());
  std::sort(sorted.begin(), sorted.end(), [](T* a, T* b) {
    return a->getName() < b->getName();
  });
  return sorted;
}

const char* defOrient(const dbOrientType& orient)
{
  switch (orient.getValue()) {
    case dbOrientType::R0:
      return "N";

    case dbOrientType::R90:
      return "W";

    case dbOrientType::R180:
      return "S";

    case dbOrientType::R270:
      return "E";

    case dbOrientType::MY:
      return "FN";

    case dbOrientType::MYR90:
      return "FE";

    case dbOrientType::MX:
      return "FS";

    case dbOrientType::MXR90:
      return "FW";
  }

  return "N";
}

const char* defSigType(const dbSigType& type)
{
  return type.getString();
}
const char* defIoType(const dbIoType& type)
{
  return type.getString();
}

}  // namespace

void DefOut::Impl::selectNet(dbNet* net)
{
  if (!net) {
    return;
  }
  _select_net_list.push_back(net);
}

void DefOut::Impl::selectInst(dbInst* inst)
{
  if (!inst) {
    return;
  }
  _select_inst_list.push_back(inst);
}

bool DefOut::Impl::writeBlock(dbBlock* block, std::ostream& stream)
{
  if (!_select_net_list.empty()) {
    _select_net_map = new dbMap<dbNet, char>(block->getNets());
    for (dbNet* net : _select_net_list) {
      (*_select_net_map)[net] = 1;
      if (net->isSpecial() || net->isMark_1ed()) {
        continue;
      }
      if (!_select_inst_map) {
        _select_inst_map = new dbMap<dbInst, char>(block->getInsts());
      }
      for (dbITerm* iterm : net->getITerms()) {
        dbInst* inst = iterm->getInst();
        (*_select_inst_map)[inst] = 1;
      }
    }
  }

  if (!_select_inst_list.empty()) {
    if (!_select_inst_map) {
      _select_inst_map = new dbMap<dbInst, char>(block->getInsts());
    }
    for (dbInst* inst : _select_inst_list) {
      (*_select_inst_map)[inst] = 1;
    }
  }

  _dist_factor
      = (double) block->getDefUnits() / (double) block->getDbUnitsPerMicron();

  _out = &stream;

  // By default C File*'s are line buffered which means they get dumped on every
  // newline, which is nominally pretty expensive. This makes it so that the
  // writes are buffered according to the block size which on modern systems can
  // be as much as 16kb. DEF's have a lot of newlines, and are large in size
  // which makes writing them really slow with line buffering.
  //
  // The following line disables automatic flushing of the buffer.
  *_out << std::nounitbuf;

  if (_version == DefOut::DEF_5_3) {
    *_out << "VERSION 5.3 ;\n";
  } else if (_version == DefOut::DEF_5_4) {
    *_out << "VERSION 5.4 ;\n";
  } else if (_version == DefOut::DEF_5_5) {
    *_out << "VERSION 5.5 ;\n";
  } else if (_version == DefOut::DEF_5_6) {
    *_out << "VERSION 5.6 ;\n";
  } else if (_version == DefOut::DEF_5_7) {
    *_out << "VERSION 5.7 ;\n";
  } else if (_version == DefOut::DEF_5_8) {
    *_out << "VERSION 5.8 ;\n";
  }
  if (_version < DefOut::DEF_5_6) {
    *_out << "NAMESCASESENSITIVE ON ;\n";
  }
  char hd = block->getHierarchyDelimiter();

  if (hd == 0) {
    hd = '|';
  }

  *_out << "DIVIDERCHAR \"" << hd << "\" ;\n";

  char left_bus, right_bus;
  block->getBusDelimiters(left_bus, right_bus);

  if ((left_bus == 0) || (right_bus == 0)) {
    left_bus = '[';
    right_bus = ']';
  }

  *_out << "BUSBITCHARS \"" << left_bus << right_bus << "\" ;\n";

  std::string bname = block->getName();
  *_out << "DESIGN " << bname << " ;\n";

  *_out << "UNITS DISTANCE MICRONS " << block->getDefUnits() << " ;\n";

  writePropertyDefinitions(block);

  Polygon die_area = block->getDieAreaPolygon();

  if (die_area.isRect()) {
    Rect r = die_area.getEnclosingRect();
    int x1 = defdist(r.xMin());
    int y1 = defdist(r.yMin());
    int x2 = defdist(r.xMax());
    int y2 = defdist(r.yMax());

    if ((x1 != 0) || (y1 != 0) || (x2 != 0) || (y2 != 0)) {
      *_out << "DIEAREA ( " << x1 << " " << y1 << " ) ( " << x2 << " " << y2
            << " ) ;\n";
    }
  } else {
    *_out << "DIEAREA ";
    std::vector<odb::Point> points = die_area.getPoints();
    // ODB ends polygons with a copy of 0 index vertex, in DEF there
    // is an implicit rule that the last vertex is connected to the
    // 0th index. So we skip the last point.
    for (int i = 0; i < points.size() - 1; i++) {
      int x = defdist(points[i].x());
      int y = defdist(points[i].y());
      *_out << "( " << x << " " << y << " ) ";
    }
    *_out << ";\n";
  }

  writeRows(block);
  writeTracks(block);
  writeGCells(block);
  writeVias(block);
  writeNonDefaultRules(block);
  writeRegions(block);
  if (_version == DefOut::DEF_5_8) {
    writeComponentMaskShift(block);
  }
  writeInsts(block);
  writeBTerms(block);
  writePinProperties(block);
  writeBlockages(block);
  writeFills(block);
  writeNets(block);
  writeGroups(block);
  writeScanChains(block);

  *_out << "END DESIGN\n";

  delete _select_net_map;
  delete _select_inst_map;

  _out = nullptr;

  return true;
}

bool DefOut::Impl::writeBlock(dbBlock* block, const char* def_file)
{
  utl::OutStreamHandler stream_handler(def_file, false);
  return writeBlock(block, stream_handler.getStream());
}

void DefOut::Impl::writeRows(dbBlock* block)
{
  for (dbRow* row : block->getRows()) {
    std::string n = row->getName();
    Point origin = row->getOrigin();
    int s = row->getSpacing();
    int c = row->getSiteCount();
    dbSite* site = row->getSite();
    std::string sn = site->getName();
    const char* o = defOrient(row->getOrient());

    *_out << "ROW " << n << " " << sn << " " << defdist(origin.x()) << " "
          << defdist(origin.y()) << " " << o << " ";

    if (row->getDirection() == dbRowDir::VERTICAL) {
      *_out << "DO 1 BY " << c << " STEP 0 " << defdist(s);
    } else {
      *_out << "DO " << c << " BY 1 STEP " << defdist(s) << " 0";
    }

    if (hasProperties(row, ROW)) {
      *_out << " + PROPERTY ";
      writeProperties(row);
    }

    *_out << " ;\n";
  }
}

void DefOut::Impl::writeTracks(dbBlock* block)
{
  for (dbTrackGrid* grid : block->getTrackGrids()) {
    dbTechLayer* layer = grid->getTechLayer();

    const std::string lname = layer->getName();

    for (int i = 0; i < grid->getNumGridPatternsX(); ++i) {
      int orgX, count, step, firstmask;
      bool samemask;
      grid->getGridPatternX(i, orgX, count, step, firstmask, samemask);
      *_out << "TRACKS X " << defdist(orgX) << " DO " << count << " STEP "
            << defdist(step);
      if (firstmask != 0) {
        *_out << " MASK " << firstmask;
        if (samemask) {
          *_out << " SAMEMASK";
        }
      }
      *_out << " LAYER " << lname << " ;\n";
    }

    for (int i = 0; i < grid->getNumGridPatternsY(); ++i) {
      int orgY, count, step, firstmask;
      bool samemask;
      grid->getGridPatternY(i, orgY, count, step, firstmask, samemask);
      *_out << "TRACKS Y " << defdist(orgY) << " DO " << count << " STEP "
            << defdist(step);
      if (firstmask != 0) {
        *_out << " MASK " << firstmask;
        if (samemask) {
          *_out << " SAMEMASK";
        }
      }
      *_out << " LAYER " << lname << " ;\n";
    }
  }
}

void DefOut::Impl::writeGCells(dbBlock* block)
{
  dbGCellGrid* grid = block->getGCellGrid();

  if (grid == nullptr) {
    return;
  }

  int i;

  for (i = 0; i < grid->getNumGridPatternsX(); ++i) {
    int orgX, count, step;
    grid->getGridPatternX(i, orgX, count, step);
    *_out << "GCELLGRID X " << defdist(orgX) << " DO " << count << " STEP "
          << defdist(step) << " ;\n";
  }

  for (i = 0; i < grid->getNumGridPatternsY(); ++i) {
    int orgY, count, step;
    grid->getGridPatternY(i, orgY, count, step);
    *_out << "GCELLGRID Y " << defdist(orgY) << " DO " << count << " STEP "
          << defdist(step) << " ;\n";
  }
}

void DefOut::Impl::writeVias(dbBlock* block)
{
  dbSet<dbVia> vias = block->getVias();

  if (vias.empty()) {
    return;
  }

  int cnt = 0;

  for (dbVia* via : vias) {
    if ((_version >= DefOut::DEF_5_6) && via->isViaRotated()) {
      continue;
    }

    ++cnt;
  }

  *_out << "VIAS " << cnt << " ;\n";

  for (dbVia* via : vias) {
    if ((_version >= DefOut::DEF_5_6) && via->isViaRotated()) {
      continue;
    }

    writeVia(via);
  }

  *_out << "END VIAS\n";
}

void DefOut::Impl::writeVia(dbVia* via)
{
  std::string vname = via->getName();
  *_out << "    - " << vname;
  dbTechViaGenerateRule* rule = via->getViaGenerateRule();

  if ((_version >= DefOut::DEF_5_6) && via->hasParams() && (rule != nullptr)) {
    std::string rname = rule->getName();
    *_out << " + VIARULE " << rname;

    const dbViaParams P = via->getViaParams();

    *_out << " + CUTSIZE " << defdist(P.getXCutSize()) << " "
          << defdist(P.getYCutSize()) << " ";
    std::string top = P.getTopLayer()->getName();
    std::string bot = P.getBottomLayer()->getName();
    std::string cut = P.getCutLayer()->getName();
    *_out << " + LAYERS " << bot << " " << cut << " " << top << " ";
    *_out << " + CUTSPACING " << defdist(P.getXCutSpacing()) << " "
          << defdist(P.getYCutSpacing()) << " ";
    *_out << " + ENCLOSURE " << defdist(P.getXBottomEnclosure()) << " "
          << defdist(P.getYBottomEnclosure()) << " "
          << defdist(P.getXTopEnclosure()) << " "
          << defdist(P.getYTopEnclosure()) << " ";

    if ((P.getNumCutRows() != 1) || (P.getNumCutCols() != 1)) {
      *_out << " + ROWCOL " << P.getNumCutRows() << " " << P.getNumCutCols()
            << " ";
    }

    if ((P.getXOrigin() != 0) || (P.getYOrigin() != 0)) {
      *_out << " + ORIGIN " << defdist(P.getXOrigin()) << " "
            << defdist(P.getYOrigin()) << " ";
    }

    if ((P.getXTopOffset() != 0) || (P.getYTopOffset() != 0)
        || (P.getXBottomOffset() != 0) || (P.getYBottomOffset() != 0)) {
      *_out << " + OFFSET " << defdist(P.getXBottomOffset()) << " "
            << defdist(P.getYBottomOffset()) << " "
            << defdist(P.getXTopOffset()) << " " << defdist(P.getYTopOffset())
            << " ";
    }

    std::string pname = via->getPattern();
    if (strcmp(pname.c_str(), "") != 0) {
      *_out << " + PATTERNNAME " << pname;
    }
  } else {
    std::string pname = via->getPattern();
    if (strcmp(pname.c_str(), "") != 0) {
      *_out << " + PATTERNNAME " << pname;
    }

    int i = 0;

    for (dbBox* box : via->getBoxes()) {
      dbTechLayer* layer = box->getTechLayer();
      int x1 = defdist(box->xMin());
      int y1 = defdist(box->yMin());
      int x2 = defdist(box->xMax());
      int y2 = defdist(box->yMax());

      if ((++i & 7) == 0) {
        *_out << "\n      ";
      }

      *_out << " + RECT " << layer->getName() << " ( " << x1 << " " << y1
            << " ) ( " << x2 << " " << y2 << " )";
    }
  }

  *_out << " ;\n";
}

void DefOut::Impl::writeComponentMaskShift(dbBlock* block)
{
  const std::vector<dbTechLayer*> layers = block->getComponentMaskShift();

  if (layers.empty()) {
    return;
  }

  *_out << "COMPONENTMASKSHIFT ";
  for (dbTechLayer* layer : layers) {
    *_out << layer->getConstName() << " ";
  }
  *_out << ";\n";
}

void DefOut::Impl::writeInsts(dbBlock* block)
{
  dbSet<dbInst> insts = block->getInsts();

  auto sorted_insts = sortedSet(insts);

  int inst_cnt = 0;
  for (dbInst* inst : sorted_insts) {
    if (_select_inst_map && !(*_select_inst_map)[inst]) {
      continue;
    }
    inst_cnt++;
  }

  *_out << "COMPONENTS " << inst_cnt << " ;\n";

  // Sort the components for consistent output
  for (dbInst* inst : sorted_insts) {
    if (_select_inst_map && !(*_select_inst_map)[inst]) {
      continue;
    }
    writeInst(inst);
  }

  *_out << "END COMPONENTS\n";
}

void DefOut::Impl::writeNonDefaultRules(dbBlock* block)
{
  dbSet<dbTechNonDefaultRule> rules = block->getNonDefaultRules();

  if (rules.empty()) {
    return;
  }

  *_out << "NONDEFAULTRULES " << rules.size() << " ;\n";

  for (dbTechNonDefaultRule* rule : rules) {
    writeNonDefaultRule(rule);
  }

  *_out << "END NONDEFAULTRULES\n";
}

void DefOut::Impl::writeNonDefaultRule(dbTechNonDefaultRule* rule)
{
  std::string name = rule->getName();
  *_out << "    - " << name << "\n";

  if (rule->getHardSpacing()) {
    *_out << "      + HARDSPACING\n";
  }

  std::vector<dbTechLayerRule*> layer_rules;
  rule->getLayerRules(layer_rules);

  for (dbTechLayerRule* rule : layer_rules) {
    writeLayerRule(rule);
  }

  std::vector<dbTechVia*> use_vias;
  rule->getUseVias(use_vias);

  for (dbTechVia* via : use_vias) {
    std::string vname = via->getName();
    *_out << "      + VIA " << vname << "\n";
  }

  std::vector<dbTechViaGenerateRule*> use_rules;
  rule->getUseViaRules(use_rules);

  for (dbTechViaGenerateRule* rule : use_rules) {
    std::string rname = rule->getName();
    *_out << "      + VIARULE " << rname << "\n";
  }

  for (dbTechLayer* layer : rule->getDb()->getTech()->getLayers()) {
    int count;

    if (rule->getMinCuts(layer, count)) {
      std::string lname = layer->getName();
      *_out << "      + MINCUTS " << lname << " " << count << "\n";
    }
  }

  if (hasProperties(rule, NONDEFAULTRULE)) {
    *_out << "    + PROPERTY ";
    writeProperties(rule);
  }

  *_out << "    ;\n";
}

void DefOut::Impl::writeLayerRule(dbTechLayerRule* rule)
{
  dbTechLayer* layer = rule->getLayer();
  std::string name = layer->getName();

  *_out << "      + LAYER " << name;

  *_out << " WIDTH " << defdist(rule->getWidth());

  if (rule->getSpacing()) {
    *_out << " SPACING " << defdist(rule->getSpacing());
  }

  if (rule->getWireExtension() != 0) {
    *_out << " WIREEXTENSION " << defdist(rule->getWireExtension());
  }

  *_out << "\n";
}

void DefOut::Impl::writeInst(dbInst* inst)
{
  dbMaster* master = inst->getMaster();
  std::string mname = master->getName();

  *_out << "    - " << inst->getName() << " " << mname;

  dbSourceType source = inst->getSourceType();

  switch (source.getValue()) {
    case dbSourceType::NONE:
      break;

    case dbSourceType::NETLIST:
      *_out << " + SOURCE NETLIST";
      break;

    case dbSourceType::DIST:
      *_out << " + SOURCE DIST";
      break;

    case dbSourceType::USER:
      *_out << " + SOURCE USER";
      break;

    case dbSourceType::TIMING:
      *_out << " + SOURCE TIMING";
      break;

    case dbSourceType::TEST:
      break;
  }

  int x, y;
  inst->getLocation(x, y);
  x = defdist(x);
  y = defdist(y);

  const char* orient = defOrient(inst->getOrient());
  dbPlacementStatus status = inst->getPlacementStatus();

  switch (status.getValue()) {
    case dbPlacementStatus::NONE:
      break;

    case dbPlacementStatus::UNPLACED: {
      *_out << " + UNPLACED";
      break;
    }

    case dbPlacementStatus::SUGGESTED:
    case dbPlacementStatus::PLACED: {
      *_out << " + PLACED ( " << x << " " << y << " ) " << orient;
      break;
    }

    case dbPlacementStatus::LOCKED:
    case dbPlacementStatus::FIRM: {
      *_out << " + FIXED ( " << x << " " << y << " ) " << orient;
      break;
    }

    case dbPlacementStatus::COVER: {
      *_out << " + COVER ( " << x << " " << y << " ) " << orient;
      break;
    }
  }

  if (inst->getWeight() != 0) {
    *_out << " + WEIGHT " << inst->getWeight();
  }

  dbRegion* region = inst->getRegion();

  if (region) {
    if (!region->getBoundaries().empty()) {
      std::string rname = region->getName();
      *_out << " + REGION " << rname;
    }
  }

  if (hasProperties(inst, COMPONENT)) {
    *_out << " + PROPERTY ";
    writeProperties(inst);
  }

  if (_version >= DefOut::DEF_5_6) {
    dbBox* box = inst->getHalo();

    if (box) {
      int left = defdist(box->xMin());
      int bottom = defdist(box->yMin());
      int right = defdist(box->xMax());
      int top = defdist(box->yMax());

      *_out << " + HALO " << left << " " << bottom << " " << right << " "
            << top;
    }
  }

  *_out << " ;\n";
}

void DefOut::Impl::writeBTerms(dbBlock* block)
{
  dbSet<dbBTerm> bterms = block->getBTerms();

  if (bterms.empty()) {
    return;
  }

  uint32_t n = 0;

  for (dbBTerm* bterm : bterms) {
    dbNet* net = bterm->getNet();

    if (net && _select_net_map && !(*_select_net_map)[net]) {
      continue;
    }

    ++n;
  }

  *_out << "PINS " << n << " ;\n";

  for (dbBTerm* bterm : sortedSet(bterms)) {
    dbNet* net = bterm->getNet();
    if (net && _select_net_map && !(*_select_net_map)[net]) {
      continue;
    }
    writeBTerm(bterm);
  }

  *_out << "END PINS\n";
}

void DefOut::Impl::writeRegions(dbBlock* block)
{
  dbSet<dbRegion> regions = block->getRegions();

  uint32_t cnt = 0;

  for (dbRegion* region : regions) {
    dbSet<dbBox> boxes = region->getBoundaries();

    if (!boxes.empty()) {
      ++cnt;
    }
  }

  if (cnt == 0) {
    return;
  }

  *_out << "REGIONS " << cnt << " ;\n";

  for (dbRegion* region : regions) {
    dbSet<dbBox> boxes = region->getBoundaries();

    if (boxes.empty()) {
      continue;
    }

    std::string name = region->getName();
    *_out << "    - " << name;

    int cnt = 0;

    for (dbBox* box : boxes) {
      if ((cnt & 0x3) == 0x3) {
        *_out << "\n        ";
      }

      *_out << " ( " << defdist(box->xMin()) << " " << defdist(box->yMin())
            << " ) ( " << defdist(box->xMax()) << " " << defdist(box->yMax())
            << " )";
      ++cnt;
    }

    switch ((dbRegionType::Value) region->getRegionType()) {
      case dbRegionType::INCLUSIVE:
        break;

      case dbRegionType::EXCLUSIVE:
        *_out << " + TYPE FENCE";
        break;

      case dbRegionType::SUGGESTED:
        *_out << " + TYPE GUIDE";
        break;
    }

    if (hasProperties(region, REGION)) {
      *_out << " + PROPERTY ";
      writeProperties(region);
    }

    *_out << " ;\n";
  }

  *_out << "END REGIONS\n";
}

void DefOut::Impl::writeGroups(dbBlock* block)
{
  auto groups = block->getGroups();
  uint32_t cnt = 0;
  for (auto group : groups) {
    if (!group->getInsts().empty()) {
      cnt++;
    }
  }
  if (cnt == 0) {
    return;
  }
  *_out << "GROUPS " << cnt << " ;\n";

  for (auto group : groups) {
    if (group->getInsts().empty()) {
      continue;
    }
    std::string name = group->getName();
    *_out << "    - " << name;

    cnt = 0;

    for (dbInst* inst : group->getInsts()) {
      if ((cnt & 0x3) == 0x3) {
        *_out << "\n        ";
      }

      std::string name = inst->getName();

      *_out << " " << name;
      ++cnt;
    }

    dbRegion* parent = group->getRegion();

    // The semantic is: if the parent region has boundaries then it is a DEF
    // region.
    if (parent) {
      dbSet<dbBox> rboxes = parent->getBoundaries();

      if (!rboxes.empty()) {
        std::string rname = parent->getName();
        *_out << " + REGION " << rname;
      }
    }

    if (hasProperties(group, GROUP)) {
      *_out << " + PROPERTY ";
      writeProperties(group);
    }

    *_out << " ;\n";
  }

  *_out << "END GROUPS\n";
}

void DefOut::Impl::writeScanChains(dbBlock* block)
{
  dbDft* dft = block->getDft();
  dbSet<dbScanChain> scan_chains = dft->getScanChains();
  if (scan_chains.empty()) {
    // If we don't have scan chains we have nothing to print
    return;
  }
  *_out << "\nSCANCHAINS " << scan_chains.size() << " ;\n\n";

  for (dbScanChain* scan_chain : dft->getScanChains()) {
    dbSet<dbScanPartition> scan_partitions = scan_chain->getScanPartitions();
    int chain_suffix = 0;
    for (dbScanPartition* scan_partition : scan_partitions) {
      bool already_printed_floating = false;
      bool already_printed_ordered = false;
      const std::string chain_name
          = scan_partitions.size() == 1
                ? scan_chain->getName()
                : fmt::format("{}_{}", scan_chain->getName(), chain_suffix);

      const std::string start_pin_name = std::visit(
          [](auto&& pin) { return pin->getName(); }, scan_chain->getScanIn());
      const std::string stop_pin_name = std::visit(
          [](auto&& pin) { return pin->getName(); }, scan_chain->getScanOut());

      *_out << "- " << chain_name << "\n";
      *_out << "+ START PIN " << start_pin_name << "\n";

      for (dbScanList* scan_list : scan_partition->getScanLists()) {
        dbSet<dbScanInst> scan_insts = scan_list->getScanInsts();
        if (scan_insts.size() == 1 && !already_printed_floating) {
          *_out << "+ FLOATING\n";
          already_printed_floating = true;
          already_printed_ordered = false;
        } else if (scan_insts.size() > 1 && !already_printed_ordered) {
          *_out << "+ ORDERED\n";
          already_printed_floating = false;
          already_printed_ordered = true;
        }

        for (dbScanInst* scan_inst : scan_insts) {
          dbScanInst::AccessPins access_pins = scan_inst->getAccessPins();
          const std::string scan_in_name = std::visit(
              [](auto&& pin) { return getPinName(pin); }, access_pins.scan_in);
          const std::string scan_out_name = std::visit(
              [](auto&& pin) { return getPinName(pin); }, access_pins.scan_out);
          *_out << "  " << scan_inst->getInst()->getName() << " ( IN "
                << scan_in_name << " ) ( OUT " << scan_out_name << " )\n";
        }
      }
      *_out << "+ PARTITION " << scan_partition->getName() << "\n";
      *_out << "+ STOP PIN " << stop_pin_name << " ;\n\n";
      ++chain_suffix;
    }
  }

  *_out << "END SCANCHAINS\n\n";
}

void DefOut::Impl::writeBTerm(dbBTerm* bterm)
{
  dbNet* net = bterm->getNet();
  if (net) {
    dbSet<dbBPin> bpins = bterm->getBPins();

    if (!bpins.empty()) {
      int cnt = 0;

      for (dbBPin* bpin : bpins) {
        writeBPin(bpin, cnt++);
      }

      *_out << " ;\n";

      return;
    }

    std::string bname = bterm->getName();

    std::string nname = net->getName();
    *_out << "    - " << bname << " + NET " << nname;

    if (bterm->isSpecial()) {
      *_out << " + SPECIAL";
    }

    *_out << " + DIRECTION " << defIoType(bterm->getIoType());

    if (_version >= DefOut::DEF_5_6) {
      dbBTerm* supply = bterm->getSupplyPin();

      if (supply) {
        std::string pname = supply->getName();
        *_out << " + SUPPLYSENSITIVITY " << pname;
      }

      dbBTerm* ground = bterm->getGroundPin();

      if (ground) {
        std::string pname = ground->getName();
        *_out << " + GROUNDSENSITIVITY " << pname;
      }
    }

    const char* sig_type = defSigType(bterm->getSigType());
    *_out << " + USE " << sig_type;

    *_out << " ;\n";
  } else {
    _logger->warn(utl::ODB,
                  173,
                  "warning: pin {} skipped because it has no net",
                  bterm->getConstName());
  }
}

void DefOut::Impl::writeBPin(dbBPin* bpin, int cnt)
{
  dbBTerm* bterm = bpin->getBTerm();
  dbNet* net = bterm->getNet();
  std::string bname = bterm->getName();

  if (cnt == 0 || _version <= DefOut::DEF_5_6) {
    std::string nname = net->getName();
    if (cnt == 0) {
      *_out << "    - " << bname << " + NET " << nname;
    } else {
      *_out << "    - " << bname << ".extra" << cnt << " + NET " << nname;
    }

    if (bterm->isSpecial()) {
      *_out << " + SPECIAL";
    }

    *_out << " + DIRECTION " << defIoType(bterm->getIoType());

    if (_version >= DefOut::DEF_5_6) {
      dbBTerm* supply = bterm->getSupplyPin();

      if (supply) {
        std::string pname = supply->getName();
        *_out << " + SUPPLYSENSITIVITY " << pname;
      }

      dbBTerm* ground = bterm->getGroundPin();

      if (ground) {
        std::string pname = ground->getName();
        *_out << " + GROUNDSENSITIVITY " << pname;
      }
    }

    *_out << " + USE " << defSigType(bterm->getSigType());
  }

  *_out << "\n      ";

  if (_version > DefOut::DEF_5_6) {
    *_out << "+ PORT";
  }

  bool isFirst = true;
  int dw, dh, x = 0, y = 0;
  int xMin, yMin, xMax, yMax;

  for (dbBox* box : bpin->getBoxes()) {
    dw = defdist(int(box->getDX() / 2));
    dh = defdist(int(box->getDY() / 2));

    if (isFirst) {
      isFirst = false;
      x = defdist(box->xMin()) + dw;
      y = defdist(box->yMin()) + dh;
    }

    xMin = defdist(box->xMin()) - x;
    yMin = defdist(box->yMin()) - y;
    xMax = defdist(box->xMax()) - x;
    yMax = defdist(box->yMax()) - y;
    dbTechLayer* layer = box->getTechLayer();
    std::string lname = layer->getName();

    *_out << "\n       ";
    if (_version == DefOut::DEF_5_5) {
      *_out << " + LAYER " << lname << " ( " << xMin << " " << yMin << " ) ( "
            << xMax << " " << yMax << " )";
    } else {
      if (_version == DefOut::DEF_5_8) {
        uint32_t mask = box->getLayerMask();
        if (mask != 0) {
          // add mask information to layer name
          lname += " MASK " + std::to_string(mask);
        }
      }
      if (bpin->hasEffectiveWidth()) {
        int w = defdist(bpin->getEffectiveWidth());
        *_out << " + LAYER " << lname << " DESIGNRULEWIDTH " << w << " ( "
              << xMin << " " << yMin << " ) ( " << xMax << " " << yMax << " )";
      } else if (bpin->hasMinSpacing()) {
        int s = defdist(bpin->getMinSpacing());
        *_out << " + LAYER " << lname << " SPACING " << s << " ( " << xMin
              << " " << yMin << " ) ( " << xMax << " " << yMax << " )";
      } else {
        *_out << " + LAYER " << lname << " ( " << xMin << " " << yMin << " ) ( "
              << xMax << " " << yMax << " )";
      }
    }
  }

  dbPlacementStatus status = bpin->getPlacementStatus();

  switch (status.getValue()) {
    case dbPlacementStatus::NONE:
    case dbPlacementStatus::UNPLACED:
      break;

    case dbPlacementStatus::SUGGESTED:
    case dbPlacementStatus::PLACED: {
      *_out << "\n        + PLACED ( " << x << " " << y << " ) N";
      break;
    }

    case dbPlacementStatus::LOCKED:
    case dbPlacementStatus::FIRM: {
      *_out << "\n        + FIXED ( " << x << " " << y << " ) N";
      break;
    }

    case dbPlacementStatus::COVER: {
      *_out << "\n        + COVER ( " << x << " " << y << " ) N";
      break;
    }
  }
}

void DefOut::Impl::writeBlockages(dbBlock* block)
{
  dbSet<dbObstruction> obstructions_raw = block->getObstructions();
  dbSet<dbBlockage> blockages_raw = block->getBlockages();

  std::vector<dbObstruction*> obstructions;
  std::vector<dbBlockage*> blockages;

  for (const auto& obstruction : obstructions_raw) {
    if (obstruction->isSystemReserved()) {
      continue;
    }
    obstructions.push_back(obstruction);
  }

  for (const auto& blockage : blockages_raw) {
    if (blockage->isSystemReserved()) {
      continue;
    }
    blockages.push_back(blockage);
  }

  int bcnt = obstructions.size() + blockages.size();

  if (bcnt == 0) {
    return;
  }

  bool first = true;

  std::vector<dbObstruction*> sorted_obs(obstructions.begin(),
                                         obstructions.end());
  std::ranges::sort(sorted_obs, [](dbObstruction* a, dbObstruction* b) {
    dbBox* bbox_a = a->getBBox();
    dbTechLayer* layer_a = bbox_a->getTechLayer();

    dbBox* bbox_b = b->getBBox();
    dbTechLayer* layer_b = bbox_a->getTechLayer();
    if (layer_a != layer_b) {
      return layer_a->getNumber() < layer_b->getNumber();
    }

    Rect rect_a = bbox_a->getBox();
    Rect rect_b = bbox_b->getBox();
    return rect_a < rect_b;
  });
  for (dbObstruction* obs : sorted_obs) {
    dbInst* inst = obs->getInstance();
    if (inst && _select_inst_map && !(*_select_inst_map)[inst]) {
      continue;
    }

    if (first) {
      first = false;
      *_out << "BLOCKAGES " << bcnt << " ;\n";
    }

    dbBox* bbox = obs->getBBox();
    dbTechLayer* layer = bbox->getTechLayer();

    *_out << "    - LAYER " << layer->getName();

    if (inst) {
      std::string iname = inst->getName();
      *_out << " + COMPONENT " << iname;
    }

    if (obs->isSlotObstruction()) {
      *_out << " + SLOTS";
    }

    if (obs->isFillObstruction()) {
      *_out << " + FILLS";
    }

    if (obs->isPushedDown()) {
      *_out << " + PUSHDOWN";
    }

    if (_version >= DefOut::DEF_5_6) {
      if (obs->hasEffectiveWidth()) {
        int w = defdist(obs->getEffectiveWidth());
        *_out << " + DESIGNRULEWIDTH " << w;
      } else if (obs->hasMinSpacing()) {
        int s = defdist(obs->getMinSpacing());
        *_out << " + SPACING " << s;
      }
    }

    int x1 = defdist(bbox->xMin());
    int y1 = defdist(bbox->yMin());
    int x2 = defdist(bbox->xMax());
    int y2 = defdist(bbox->yMax());

    *_out << " RECT ( " << x1 << " " << y1 << " ) ( " << x2 << " " << y2
          << " ) ;\n";
  }

  std::vector<dbBlockage*> sorted_blockages(blockages.begin(), blockages.end());
  std::ranges::sort(sorted_blockages, [](dbBlockage* a, dbBlockage* b) {
    dbBox* bbox_a = a->getBBox();
    dbBox* bbox_b = b->getBBox();
    Rect rect_a = bbox_a->getBox();
    Rect rect_b = bbox_b->getBox();
    return rect_a < rect_b;
  });

  for (dbBlockage* blk : sorted_blockages) {
    dbInst* inst = blk->getInstance();
    if (inst && _select_inst_map && !(*_select_inst_map)[inst]) {
      continue;
    }

    if (first) {
      first = false;
      *_out << "BLOCKAGES " << bcnt << " ;\n";
    }

    *_out << "    - PLACEMENT";

    if (blk->isSoft()) {
      *_out << " + SOFT";
    }

    if (blk->getMaxDensity() > 0) {
      *_out << " + PARTIAL " << fmt::format("{:f}", blk->getMaxDensity());
    }

    if (inst) {
      std::string iname = inst->getName();
      *_out << " + COMPONENT " << iname;
    }

    if (blk->isPushedDown()) {
      *_out << " + PUSHDOWN";
    }

    dbBox* bbox = blk->getBBox();
    int x1 = defdist(bbox->xMin());
    int y1 = defdist(bbox->yMin());
    int x2 = defdist(bbox->xMax());
    int y2 = defdist(bbox->yMax());

    *_out << " RECT ( " << x1 << " " << y1 << " ) ( " << x2 << " " << y2
          << " ) ;\n";
  }

  if (!first) {
    *_out << "END BLOCKAGES\n";
  }
}

void DefOut::Impl::writeFills(dbBlock* block)
{
  dbSet<dbFill> fills = block->getFills();
  int num_fills = fills.size();

  if (num_fills == 0) {
    return;
  }

  *_out << "FILLS " << num_fills << " ;\n";

  for (dbFill* fill : fills) {
    *_out << "    - LAYER " << fill->getTechLayer()->getName();

    uint32_t mask = fill->maskNumber();
    if (mask != 0) {
      *_out << " + MASK " << mask;
    }

    if (fill->needsOPC()) {
      *_out << " + OPC";
    }

    Rect r;
    fill->getRect(r);

    int x1 = defdist(r.xMin());
    int y1 = defdist(r.yMin());
    int x2 = defdist(r.xMax());
    int y2 = defdist(r.yMax());

    *_out << " RECT ( " << x1 << " " << y1 << " ) ( " << x2 << " " << y2
          << " ) ;\n";
  }

  *_out << "END FILLS\n";
}

void DefOut::Impl::writeNets(dbBlock* block)
{
  dbSet<dbNet> nets = block->getNets();

  int net_cnt = 0;
  int snet_cnt = 0;

  dbSet<dbNet>::iterator itr;
  dbMap<dbNet, char> regular_net(nets);

  auto sorted_nets = sortedSet(nets);

  // Build map of mterm names and associated nets
  std::unordered_map<std::string, std::set<dbNet*>> snet_term_map;
  for (auto* inst : block->getInsts()) {
    for (auto* iterm : inst->getITerms()) {
      snet_term_map[iterm->getMTerm()->getName()].insert(iterm->getNet());
    }
  }

  for (dbNet* net : sorted_nets) {
    if (_select_net_map) {
      if (!(*_select_net_map)[net]) {
        continue;
      }
    }

    if (!net->isSpecial()) {
      regular_net[net] = 1;
      net_cnt++;
    } else {
      regular_net[net] = 0;
      snet_cnt++;

      // Check for non-special iterms.
      for (dbITerm* iterm : net->getITerms()) {
        if (!iterm->isSpecial()) {
          regular_net[net] = 1;
          net_cnt++;
          break;
        }
      }
    }
  }

  if (snet_cnt > 0) {
    *_out << "SPECIALNETS " << snet_cnt << " ;\n";

    for (dbNet* net : sorted_nets) {
      if (_select_net_map && !(*_select_net_map)[net]) {
        continue;
      }
      if (net->isSpecial()) {
        writeSNet(net, snet_term_map);
      }
    }

    *_out << "END SPECIALNETS\n";
  }

  *_out << "NETS " << net_cnt << " ;\n";

  for (dbNet* net : sorted_nets) {
    if (_select_net_map && !(*_select_net_map)[net]) {
      continue;
    }

    if (regular_net[net] == 1) {
      writeNet(net);
    }
  }

  *_out << "END NETS\n";
}

void DefOut::Impl::writeSNet(
    dbNet* net,
    const std::unordered_map<std::string, std::set<dbNet*>>& snet_term_map)
{
  std::string nname = net->getName();
  *_out << "    - " << nname;

  int i = 0;

  for (dbBTerm* bterm : net->getBTerms()) {
    if ((++i & 7) == 0) {
      *_out << "\n    ";
    }
    *_out << " ( PIN " << bterm->getName() << " )";
  }

  char ttname[max_name_length];
  std::set<std::string> wild_names;
  for (dbITerm* iterm : net->getITerms()) {
    if (!iterm->isSpecial()) {
      continue;
    }

    dbInst* inst = iterm->getInst();
    dbMTerm* mterm = iterm->getMTerm();
    char* mtname = mterm->getName(inst, &ttname[0]);
    bool iswildcard = false;
    if (snet_term_map.at(mterm->getName()).size() == 1) {
      // mterm is unique to this net, so we can use wildcard
      iswildcard = true;
    }
    if (iswildcard) {
      if (wild_names.find(mtname) == wild_names.end()) {
        *_out << " ( * " << mtname << " )";
        ++i;
        wild_names.insert(mtname);
      }
    } else {
      if ((++i & 7) == 0) {
        std::string iname = inst->getName();
        *_out << "\n      ( " << iname << " " << mtname << " )";
      } else {
        std::string iname = inst->getName();
        *_out << " ( " << iname << " " << mtname << " )";
      }
    }
  }

  const char* sig_type = defSigType(net->getSigType());
  *_out << " + USE " << sig_type;

  _non_default_rule = nullptr;

  for (dbSWire* swire : net->getSWires()) {
    writeSWire(swire);
  }

  dbSourceType source = net->getSourceType();

  switch (source.getValue()) {
    case dbSourceType::NONE:
      break;

    case dbSourceType::NETLIST:
      *_out << " + SOURCE NETLIST";
      break;

    case dbSourceType::DIST:
      *_out << " + SOURCE DIST";
      break;

    case dbSourceType::USER:
      *_out << " + SOURCE USER";
      break;

    case dbSourceType::TIMING:
      *_out << " + SOURCE TIMING";
      break;

    case dbSourceType::TEST:
      break;
  }

  if (net->hasFixedBump()) {
    *_out << " + FIXEDBUMP";
  }

  if (net->getWeight() != 1) {
    *_out << " + WEIGHT " << net->getWeight();
  }

  if (hasProperties(net, SPECIALNET)) {
    *_out << " + PROPERTY ";
    writeProperties(net);
  }

  *_out << " ;\n";
}

void DefOut::Impl::writeWire(dbWire* wire)
{
  dbWireDecoder decode;
  dbTechLayer* layer;
  dbWireType prev_wire_type = dbWireType::NONE;
  int point_cnt = 0;
  int path_cnt = 0;
  int prev_x = std::numeric_limits<int>::max();
  int prev_y = std::numeric_limits<int>::max();

  for (decode.begin(wire);;) {
    dbWireDecoder::OpCode opcode = decode.next();
    std::optional<uint8_t> color = decode.getColor();
    std::optional<dbWireDecoder::ViaColor> viacolor = decode.getViaColor();

    switch (opcode) {
      case dbWireDecoder::PATH:
      case dbWireDecoder::SHORT:
      case dbWireDecoder::VWIRE:
      case dbWireDecoder::JUNCTION: {
        layer = decode.getLayer();
        const std::string lname = layer->getName();

        dbWireType wire_type = decode.getWireType();
        if (wire->getNet()->getWireType() == dbWireType::FIXED) {
          wire_type = dbWireType::FIXED;
        }

        if ((path_cnt == 0) || (wire_type != prev_wire_type)) {
          *_out << "\n      + " << wire_type.getString() << " " << lname;
        } else {
          *_out << "\n      NEW " << lname;
        }

        if (_non_default_rule && (decode.peek() != dbWireDecoder::RULE)) {
          *_out << " TAPER";
        }

        prev_wire_type = wire_type;
        point_cnt = 0;
        ++path_cnt;
        break;
      }

      case dbWireDecoder::POINT: {
        int x, y;
        decode.getPoint(x, y);
        x = defdist(x);
        y = defdist(y);

        if ((++point_cnt & 7) == 0) {
          *_out << "\n    ";
        }

        std::string mask_statement;
        if (point_cnt % 2 == 0 && color) {
          mask_statement = fmt::format("MASK {}", color.value());
        }

        if (point_cnt == 1) {
          *_out << " ( " << x << " " << y << " )";
        } else if (x == prev_x) {
          *_out << mask_statement << " ( * " << y << " )";
        } else if (y == prev_y) {
          *_out << mask_statement << " ( " << x << " * )";
        }

        prev_x = x;
        prev_y = y;
        break;
      }

      case dbWireDecoder::POINT_EXT: {
        int x, y, ext;
        decode.getPoint(x, y, ext);
        x = defdist(x);
        y = defdist(y);
        ext = defdist(ext);

        if ((++point_cnt & 7) == 0) {
          *_out << "\n    ";
        }

        if (point_cnt == 1) {
          *_out << " ( " << x << " " << y << " " << ext << " )";
        } else if ((x == prev_x) && (y == prev_y)) {
          *_out << " ( * * " << ext << " )";
        } else if (x == prev_x) {
          *_out << " ( * " << y << " " << ext << " )";
        } else if (y == prev_y) {
          *_out << " ( " << x << " * " << ext << " )";
        }

        prev_x = x;
        prev_y = y;
        break;
      }

      case dbWireDecoder::VIA: {
        if ((++point_cnt & 7) == 0) {
          *_out << "\n    ";
        }

        dbVia* via = decode.getVia();

        std::string via_mask_statement;
        if ((_version >= DefOut::DEF_5_8) && viacolor) {
          via_mask_statement = fmt::format("MASK {}{}{} ",
                                           viacolor.value().top_color,
                                           viacolor.value().cut_color,
                                           viacolor.value().bottom_color);
        }

        if ((_version >= DefOut::DEF_5_6) && via->isViaRotated()) {
          std::string vname;

          if (via->getTechVia()) {
            vname = via->getTechVia()->getName();
          } else {
            vname = via->getBlockVia()->getName();
          }

          *_out << " " << via_mask_statement << vname << " "
                << defOrient(via->getOrient());
        } else {
          std::string vname = via->getName();
          *_out << " " << via_mask_statement << vname;
        }
        break;
      }

      case dbWireDecoder::TECH_VIA: {
        if ((++point_cnt & 7) == 0) {
          *_out << "\n    ";
        }

        std::string via_mask_statement;
        if ((_version >= DefOut::DEF_5_8) && viacolor) {
          via_mask_statement = fmt::format("MASK {}{}{} ",
                                           viacolor.value().top_color,
                                           viacolor.value().cut_color,
                                           viacolor.value().bottom_color);
        }

        dbTechVia* via = decode.getTechVia();
        std::string vname = via->getName();
        *_out << " " << via_mask_statement << vname;
        break;
      }

      case dbWireDecoder::ITERM:
      case dbWireDecoder::BTERM:
        break;

      case dbWireDecoder::RULE: {
        if (point_cnt == 0) {
          dbTechLayerRule* rule = decode.getRule();
          dbTechNonDefaultRule* taper_rule = rule->getNonDefaultRule();

          if (_non_default_rule == nullptr) {
            std::string name = taper_rule->getName();
            *_out << " TAPERRULE " << name << " ";
          } else if (_non_default_rule != taper_rule) {
            std::string name = taper_rule->getName();
            *_out << " TAPERRULE " << name << " ";
          }
        }
        break;
      }

      case dbWireDecoder::RECT: {
        if ((++point_cnt & 7) == 0) {
          *_out << "\n    ";
        }

        int deltaX1;
        int deltaY1;
        int deltaX2;
        int deltaY2;
        decode.getRect(deltaX1, deltaY1, deltaX2, deltaY2);
        deltaX1 = defdist(deltaX1);
        deltaY1 = defdist(deltaY1);
        deltaX2 = defdist(deltaX2);
        deltaY2 = defdist(deltaY2);
        if (color.has_value()) {
          *_out << " RECT MASK " << color.value() << " ( " << deltaX1 << " "
                << deltaY1 << " " << deltaX2 << " " << deltaY2 << " ) ";

        } else {
          *_out << " RECT ( " << deltaX1 << " " << deltaY1 << " " << deltaX2
                << " " << deltaY2 << " ) ";
        }
        break;
      }

      case dbWireDecoder::END_DECODE:
        return;
    }
  }
}

void DefOut::Impl::writeSWire(dbSWire* wire)
{
  switch (wire->getWireType().getValue()) {
    case dbWireType::COVER:
      *_out << "\n      + COVER";
      break;

    case dbWireType::FIXED:
      *_out << "\n      + FIXED";
      break;

    case dbWireType::ROUTED:
      *_out << "\n      + ROUTED";
      break;

    case dbWireType::SHIELD: {
      dbNet* s = wire->getShield();
      if (s) {
        std::string n = s->getName();
        *_out << "\n      + SHIELD " << n;
      } else {
        _logger->warn(utl::ODB, 174, "warning: missing shield net");
        *_out << "\n      + ROUTED";
      }
      break;
    }

    default:
      *_out << "\n      + ROUTED";
      break;
  }

  int i = 0;

  for (dbSBox* box : wire->getWires()) {
    if (i++ > 0) {
      *_out << "\n      NEW";
    }

    if (!box->isVia()) {
      writeSpecialPath(box);

    } else if (box->getTechVia()) {
      dbWireShapeType type = box->getWireShapeType();
      dbTechVia* v = box->getTechVia();
      std::string vn = v->getName();
      dbTechLayer* l = v->getBottomLayer();
      const std::string ln = l->getName();

      const Point pt = box->getViaXY();

      if (box->hasViaLayerMasks()) {
        vn = fmt::format("MASK {}{}{} {}",
                         box->getViaTopLayerMask(),
                         box->getViaCutLayerMask(),
                         box->getViaBottomLayerMask(),
                         vn);
      }

      if (type.getValue() == dbWireShapeType::NONE) {
        *_out << " " << ln << " 0 ( " << defdist(pt.getX()) << " "
              << defdist(pt.getY()) << " ) " << vn;
      } else {
        *_out << " " << ln << " 0 + SHAPE " << type.getString() << " ( "
              << defdist(pt.getX()) << " " << defdist(pt.getY()) << " ) " << vn;
      }
    } else if (box->getBlockVia()) {
      dbWireShapeType type = box->getWireShapeType();
      dbVia* v = box->getBlockVia();
      std::string vn = v->getName();
      dbTechLayer* l = v->getBottomLayer();
      const std::string ln = l->getName();

      const Point pt = box->getViaXY();

      if (box->hasViaLayerMasks()) {
        vn = fmt::format("MASK {}{}{} {}",
                         box->getViaTopLayerMask(),
                         box->getViaCutLayerMask(),
                         box->getViaBottomLayerMask(),
                         vn);
      }

      if (type.getValue() == dbWireShapeType::NONE) {
        *_out << " " << ln << " 0 ( " << defdist(pt.getX()) << " "
              << defdist(pt.getY()) << " ) " << vn;
      } else {
        *_out << " " << ln << " 0 + SHAPE " << type.getString() << " ( "
              << defdist(pt.getX()) << " " << defdist(pt.getY()) << " ) " << vn;
      }
    }
  }
}

void DefOut::Impl::writeSpecialPath(dbSBox* box)
{
  dbTechLayer* l = box->getTechLayer();
  const std::string ln = l->getName();

  int x1 = box->xMin();
  int y1 = box->yMin();
  int x2 = box->xMax();
  int y2 = box->yMax();
  uint32_t dx = x2 - x1;
  uint32_t dy = y2 - y1;
  uint32_t w;
  uint32_t mask = box->getLayerMask();

  switch (box->getDirection()) {
    case dbSBox::UNDEFINED: {
      bool dx_even = ((dx & 1) == 0);
      bool dy_even = ((dy & 1) == 0);

      if (dx_even && dy_even) {
        if (dy < dx) {
          w = dy;
          uint32_t dw = dy >> 1;
          y1 += dw;
          y2 -= dw;
          assert(y1 == y2);
        } else {
          w = dx;
          uint32_t dw = dx >> 1;
          x1 += dw;
          x2 -= dw;
          assert(x1 == x2);
        }
      } else if (dx_even) {
        w = dx;
        uint32_t dw = dx >> 1;
        x1 += dw;
        x2 -= dw;
        assert(x1 == x2);
      } else if (dy_even) {
        w = dy;
        uint32_t dw = dy >> 1;
        y1 += dw;
        y2 -= dw;
        assert(y1 == y2);
      } else {
        throw std::runtime_error("odd dimension in both directions");
      }

      break;
    }

    case dbSBox::HORIZONTAL: {
      w = dy;
      uint32_t dw = dy >> 1;
      y1 += dw;
      y2 -= dw;
      assert(y1 == y2);
      break;
    }

    case dbSBox::VERTICAL: {
      w = dx;
      uint32_t dw = dx >> 1;
      x1 += dw;
      x2 -= dw;
      assert(x1 == x2);
      break;
    }
    case dbSBox::OCTILINEAR: {
      Oct oct = box->getOct();
      x1 = oct.getCenterLow().getX();
      y1 = oct.getCenterLow().getY();
      x2 = oct.getCenterHigh().getX();
      y2 = oct.getCenterHigh().getY();
      w = oct.getWidth();
      break;
    }
    default:
      throw std::runtime_error("unknown direction");
      break;
  }

  dbWireShapeType type = box->getWireShapeType();

  *_out << " " << ln << " " << defdist(w);
  if (type.getValue() != dbWireShapeType::NONE) {
    *_out << " + SHAPE " << type.getString();
  }
  *_out << " ( " << defdist(x1) << " " << defdist(y1) << " )";
  if (mask != 0) {
    *_out << " MASK " << mask;
  }
  *_out << " ( " << defdist(x2) << " " << defdist(y2) << " )";
}

void DefOut::Impl::writeNet(dbNet* net)
{
  std::string nname = net->getName();
  *_out << "    - " << nname;

  char ttname[max_name_length];
  int i = 0;

  for (dbBTerm* bterm : net->getBTerms()) {
    const char* pin_name = bterm->getConstName();
    if ((++i & 7) == 0) {
      *_out << "\n     ";
    }
    *_out << " ( PIN " << pin_name << " )";
  }

  for (dbITerm* iterm : net->getITerms()) {
    if (iterm->isSpecial()) {
      continue;
    }

    dbInst* inst = iterm->getInst();
    if (_select_inst_map && !(*_select_inst_map)[inst]) {
      continue;  // for power nets in regular net section, tie-lo/hi
    }
    dbMTerm* mterm = iterm->getMTerm();
    // std::string mtname = mterm->getName();
    char* mtname = mterm->getName(inst, &ttname[0]);

    if ((++i & 7) == 0) {
      *_out << "\n     ";
    }

    std::string iname = inst->getName();
    *_out << " ( " << iname << " " << mtname << " )";
  }

  if (net->getXTalkClass() != 0) {
    *_out << " + XTALK " << net->getXTalkClass();
  }

  const char* sig_type = defSigType(net->getSigType());
  *_out << " + USE " << sig_type;

  _non_default_rule = net->getNonDefaultRule();

  if (_non_default_rule) {
    std::string n = _non_default_rule->getName();
    *_out << " + NONDEFAULTRULE " << n;
  }

  dbWire* wire = net->getWire();

  if (wire) {
    writeWire(wire);
  }

  dbSourceType source = net->getSourceType();

  switch (source.getValue()) {
    case dbSourceType::NONE:
      break;

    case dbSourceType::NETLIST:
      *_out << " + SOURCE NETLIST";
      break;

    case dbSourceType::DIST:
      *_out << " + SOURCE DIST";
      break;

    case dbSourceType::USER:
      *_out << " + SOURCE USER";
      break;

    case dbSourceType::TIMING:
      *_out << " + SOURCE TIMING";
      break;

    case dbSourceType::TEST:
      *_out << " + SOURCE TEST";
      break;
  }

  if (net->hasFixedBump()) {
    *_out << " + FIXEDBUMP";
  }

  if (net->getWeight() != 1) {
    *_out << " + WEIGHT " << net->getWeight();
  }

  if (hasProperties(net, NET)) {
    *_out << " + PROPERTY ";
    writeProperties(net);
  }

  *_out << " ;\n";
}

//
// See defin/definProDefs.h
//
void DefOut::Impl::writePropertyDefinitions(dbBlock* block)
{
  dbProperty* defs
      = dbProperty::find(block, "__ADS_DEF_PROPERTY_DEFINITIONS__");

  if (defs == nullptr) {
    return;
  }

  *_out << "PROPERTYDEFINITIONS\n";

  for (dbProperty* obj : dbProperty::getProperties(defs)) {
    std::string objType = obj->getName();

    ObjType obj_type;

    if (strcmp(objType.c_str(), "COMPONENT") == 0) {
      obj_type = COMPONENT;
    } else if (strcmp(objType.c_str(), "COMPONENTPIN") == 0) {
      obj_type = COMPONENTPIN;
    } else if (strcmp(objType.c_str(), "DESIGN") == 0) {
      obj_type = DESIGN;
    } else if (strcmp(objType.c_str(), "GROUP") == 0) {
      obj_type = GROUP;
    } else if (strcmp(objType.c_str(), "NET") == 0) {
      obj_type = NET;
    } else if (strcmp(objType.c_str(), "NONDEFAULTRULE") == 0) {
      obj_type = NONDEFAULTRULE;
    } else if (strcmp(objType.c_str(), "REGION") == 0) {
      obj_type = REGION;
    } else if (strcmp(objType.c_str(), "ROW") == 0) {
      obj_type = ROW;
    } else if (strcmp(objType.c_str(), "SPECIALNET") == 0) {
      obj_type = SPECIALNET;
    } else {
      continue;
    }

    std::map<std::string, bool>& defs_map = _prop_defs[obj_type];

    for (dbProperty* prop : dbProperty::getProperties(obj)) {
      std::string name = prop->getName();
      defs_map[std::string(name)] = true;
      switch (prop->getType()) {
        case dbProperty::STRING_PROP:
          *_out << objType << " " << name << " STRING ";
          break;

        case dbProperty::INT_PROP:
          *_out << objType << " " << name << " INTEGER ";
          break;

        case dbProperty::DOUBLE_PROP:
          *_out << objType << " " << name << " REAL ";
          break;

        default:
          continue;
      }

      dbProperty* minV = dbProperty::find(prop, "MIN");
      dbProperty* maxV = dbProperty::find(prop, "MAX");

      if (minV && maxV) {
        *_out << "RANGE ";
        writePropValue(minV);
        writePropValue(maxV);
      }

      dbProperty* value = dbProperty::find(prop, "VALUE");

      if (value) {
        writePropValue(value);
      }

      *_out << ";\n";
    }
  }

  *_out << "END PROPERTYDEFINITIONS\n";
}

void DefOut::Impl::writePropValue(dbProperty* prop)
{
  switch (prop->getType()) {
    case dbProperty::STRING_PROP: {
      dbStringProperty* p = (dbStringProperty*) prop;
      std::string v = p->getValue();
      *_out << "\"" << v << "\" ";
      break;
    }

    case dbProperty::INT_PROP: {
      dbIntProperty* p = (dbIntProperty*) prop;
      int v = p->getValue();
      *_out << v << " ";
      break;
    }

    case dbProperty::DOUBLE_PROP: {
      dbDoubleProperty* p = (dbDoubleProperty*) prop;
      double v = p->getValue();
      *_out << fmt::format("{:g} ", v);
    }

    default:
      break;
  }
}

void DefOut::Impl::writeProperties(dbObject* object)
{
  int cnt = 0;

  for (dbProperty* prop : dbProperty::getProperties(object)) {
    if (cnt && ((cnt & 3) == 0)) {
      *_out << "\n    ";
    }

    std::string name = prop->getName();
    *_out << name << " ";
    writePropValue(prop);
    ++cnt;
  }
}

bool DefOut::Impl::hasProperties(dbObject* object, ObjType type)
{
  for (dbProperty* prop : dbProperty::getProperties(object)) {
    std::string name = prop->getName();

    if (_prop_defs[type].find(name) != _prop_defs[type].end()) {
      return true;
    }
  }

  return false;
}

void DefOut::Impl::writePinProperties(dbBlock* block)
{
  uint32_t cnt = 0;

  dbSet<dbBTerm> bterms = block->getBTerms();

  for (dbBTerm* bterm : bterms) {
    if (hasProperties(bterm, COMPONENTPIN)) {
      ++cnt;
    }
  }

  dbSet<dbITerm> iterms = block->getITerms();

  for (dbITerm* iterm : iterms) {
    if (hasProperties(iterm, COMPONENTPIN)) {
      ++cnt;
    }
  }

  if (cnt == 0) {
    return;
  }

  *_out << "PINPROPERTIES " << cnt << " ;\n";

  for (dbBTerm* bterm : bterms) {
    if (hasProperties(bterm, COMPONENTPIN)) {
      std::string name = bterm->getName();
      *_out << "  - PIN " << name << " + PROPERTY ";
      writeProperties(bterm);
      *_out << " ;\n";
    }
  }

  char ttname[max_name_length];
  for (dbITerm* iterm : iterms) {
    if (hasProperties(iterm, COMPONENTPIN)) {
      dbInst* inst = iterm->getInst();
      dbMTerm* mterm = iterm->getMTerm();
      std::string iname = inst->getName();
      // std::string mtname = mterm->getName();
      char* mtname = mterm->getName(inst, &ttname[0]);
      *_out << "  - " << iname << " " << mtname << " + PROPERTY ";
      writeProperties(iterm);
      *_out << " ;\n";
    }
  }

  *_out << "END PINPROPERTIES\n";
}

}  // namespace odb
