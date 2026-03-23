// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definReader.h"

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "boost/algorithm/string/replace.hpp"
#include "defiBlockage.hpp"
#include "defiComponent.hpp"
#include "defiDefs.hpp"
#include "defiFill.hpp"
#include "defiGroup.hpp"
#include "defiMisc.hpp"
#include "defiNet.hpp"
#include "defiNonDefault.hpp"
#include "defiPath.hpp"
#include "defiPinCap.hpp"
#include "defiPinProp.hpp"
#include "defiProp.hpp"
#include "defiRegion.hpp"
#include "defiRowTrack.hpp"
#include "defiScanchain.hpp"
#include "defiSite.hpp"
#include "defiVia.hpp"
#include "definBase.h"
#include "definBlockage.h"
#include "definComponent.h"
#include "definComponentMaskShift.h"
#include "definFill.h"
#include "definGCell.h"
#include "definGroup.h"
#include "definNet.h"
#include "definNonDefaultRule.h"
#include "definPin.h"
#include "definPinProps.h"
#include "definPropDefs.h"
#include "definRegion.h"
#include "definRow.h"
#include "definSNet.h"
#include "definTracks.h"
#include "definTypes.h"
#include "definVia.h"
#include "defrReader.hpp"
#include "defzlib.hpp"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/geom.h"
#include "utl/Logger.h"

#define UNSUPPORTED(msg)              \
  reader->error((msg));               \
  if (!reader->_continue_on_errors) { \
    return PARSE_ERROR;               \
  }

#define CHECKBLOCK                                                        \
  if (reader->_block == nullptr) {                                        \
    reader->_logger->warn(utl::ODB, 260, "DESIGN is not defined in DEF"); \
    return PARSE_ERROR;                                                   \
  }
namespace odb {

namespace {

// Helper function to get the correct number of bits of a cell for scandef.
int calculateBitsForCellInScandef(const int bits, dbInst* inst)
{
  // -1 is no bits were provided in the scandef
  if (bits != -1) {
    return bits;
  }
  // -1 means that the bits are not set in the scandef
  // We need to check if the inst is sequential to decide what is a
  // reasonable default value
  dbMaster* master = inst->getMaster();
  if (master->isSequential()) {
    // the default number of bits for sequential elements is 1
    return 1;
  }
  // the default number of bits for combinational logic is 0
  return 0;
}

dbITerm* findScanITerm(definReader* reader,
                       dbInst* inst,
                       const char* pin_name,
                       const char* common_pin)
{
  if (!pin_name) {
    if (!common_pin) {
      reader->error(
          fmt::format("SCANCHAIN is missing either component pin or a "
                      "COMMONSCANPINS for instance {}",
                      inst->getName()));
      return nullptr;
    }
    // using the common pin name
    return inst->findITerm(common_pin);
  }
  return inst->findITerm(pin_name);
}

std::optional<std::variant<dbBTerm*, dbITerm*>> findScanTerm(
    definReader* reader,
    dbBlock* block,
    const char* type,
    const char* inst_name,
    const char* pin_name)
{
  if (inst_name && strcmp(inst_name, "PIN") != 0) {
    dbInst* inst = block->findInst(inst_name);
    if (inst) {
      dbITerm* iterm = inst->findITerm(pin_name);
      if (iterm) {
        return iterm;
      }
    }
  } else {
    dbBTerm* bterm = block->findBTerm(pin_name);
    if (bterm) {
      return bterm;
    }
  }
  std::string name;
  if (inst_name) {
    name = fmt::format("{}/{}", inst_name, pin_name);
  } else {
    name = pin_name;
  }
  reader->error(fmt::format("SCANCHAIN {} pin {} does not exist", type, name));
  return std::nullopt;
}

void populateScanInst(definReader* reader,
                      dbBlock* block,
                      DefParser::defiScanchain* scan_chain,
                      dbScanList* db_scan_list,
                      const char* inst_name,
                      const char* in_pin_name,
                      const char* out_pin_name,
                      const int bits)
{
  dbInst* inst = block->findInst(inst_name);
  if (!inst) {
    reader->error(fmt::format("SCANCHAIN Inst {} does not exist", inst_name));
    return;
  }

  dbScanInst* scan_inst = db_scan_list->add(inst);

  dbITerm* scan_in
      = findScanITerm(reader, inst, in_pin_name, scan_chain->commonInPin());
  if (!scan_in) {
    reader->error(fmt::format("SCANCHAIN IN pin {} does not exist in cell {}",
                              in_pin_name,
                              inst_name));
  }

  dbITerm* scan_out
      = findScanITerm(reader, inst, out_pin_name, scan_chain->commonOutPin());
  if (!scan_out) {
    reader->error(fmt::format("SCANCHAIN OUT pin {} does not exist in cell {}",
                              out_pin_name,
                              inst_name));
  }

  if (!scan_in || !scan_out) {
    return;
  }

  scan_inst->setAccessPins({.scan_in = scan_in, .scan_out = scan_out});
  scan_inst->setBits(calculateBitsForCellInScandef(bits, inst));
}

}  // namespace

definReader::definReader(dbDatabase* db, utl::Logger* logger, defin::MODE mode)
{
  _db = db;

  definBase::setLogger(logger);
  definBase::setMode(mode);

  init();
}

definReader::~definReader() = default;

int definReader::errors()
{
  int e = _errors;

  std::vector<definBase*>::iterator itr;
  for (itr = _interfaces.begin(); itr != _interfaces.end(); ++itr) {
    e += (*itr)->_errors;
  }

  return e;
}

void definReader::skipWires()
{
  _netR->skipWires();
}
void definReader::skipConnections()
{
  _netR->skipConnections();
}
void definReader::skipSpecialWires()
{
  _snetR->skipSpecialWires();
}

void definReader::skipShields()
{
  _snetR->skipShields();
}

void definReader::skipBlockWires()
{
  _snetR->skipBlockWires();
}

void definReader::skipFillWires()
{
  _snetR->skipFillWires();
}

void definReader::continueOnErrors()
{
  _continue_on_errors = true;
}

void definReader::useBlockName(const char* name)
{
  _block_name = name;
}

void definReader::init()
{
  auto make = [this](auto& interface) {
    using PtrType = std::remove_reference_t<decltype(interface)>;
    using Type = typename PtrType::element_type;
    interface = std::make_unique<Type>();
    interface->setLogger(_logger);
    interface->setMode(_mode);
    _interfaces.push_back(interface.get());
  };

  _interfaces.clear();
  make(_blockageR);
  make(_componentR);
  make(_componentMaskShift);
  make(_fillR);
  make(_gcellR);
  make(_netR);
  make(_pinR);
  make(_rowR);
  make(_snetR);
  make(_tracksR);
  make(_viaR);
  make(_regionR);
  make(_groupR);
  make(_non_default_ruleR);
  make(_prop_defsR);
  make(_pin_propsR);
}

void definReader::setTech(dbTech* tech)
{
  definBase::setTech(tech);

  std::vector<definBase*>::iterator itr;
  for (itr = _interfaces.begin(); itr != _interfaces.end(); ++itr) {
    (*itr)->setTech(tech);
  }
}

void definReader::setBlock(dbBlock* block)
{
  definBase::setBlock(block);
  std::vector<definBase*>::iterator itr;
  for (itr = _interfaces.begin(); itr != _interfaces.end(); ++itr) {
    (*itr)->setBlock(block);
  }
}

// Generic handler for transfering properties from the
// Si2 DEF parser object to the OpenDB callback
template <typename DEF_TYPE, typename CALLBACK>
static void handle_props(DEF_TYPE* def_obj, CALLBACK* callback)
{
  for (int i = 0; i < def_obj->numProps(); ++i) {
    switch (def_obj->propType(i)) {
      case 'R':
        callback->property(def_obj->propName(i), def_obj->propNumber(i));
        break;
      case 'I':
        callback->property(def_obj->propName(i), (int) def_obj->propNumber(i));
        break;
      case 'S': /* fallthru */
      case 'N': /* fallthru */
      case 'Q':
        callback->property(def_obj->propName(i), def_obj->propValue(i));
        break;
    }
  }
}

int definReader::versionCallback(
    DefParser::defrCallbackType_e type /* unused: type */,
    const char* value,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  reader->version_ = value;
  return PARSE_OK;
}

int definReader::divideCharCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    const char* value,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  reader->hier_delimiter_ = value[0];
  if (reader->hier_delimiter_ == 0) {
    reader->error("Syntax error in DIVIDERCHAR statment");
    return PARSE_ERROR;
  }
  return PARSE_OK;
}
int definReader::busBitCallback(
    DefParser::defrCallbackType_e type /* unused: type */,
    const char* value,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  reader->left_bus_delimiter_ = value[0];
  reader->right_bus_delimiter_ = value[1];
  if ((reader->left_bus_delimiter_ == 0)
      || (reader->right_bus_delimiter_ == 0)) {
    reader->error("Syntax error in BUSBITCHARS statment");
    return PARSE_ERROR;
  }
  return PARSE_OK;
}
int definReader::designCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    const char* design,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  std::string block_name;
  if (!reader->_block_name.empty()) {
    block_name = reader->_block_name;
  } else {
    block_name = design;
  }
  if (reader->_mode != defin::DEFAULT) {
    reader->_block = reader->chip_->getBlock();
  } else {
    reader->_block = dbBlock::create(
        reader->chip_, block_name.c_str(), reader->hier_delimiter_);
  }
  if (reader->_mode == defin::DEFAULT) {
    reader->_block->setBusDelimiters(reader->left_bus_delimiter_,
                                     reader->right_bus_delimiter_);
  }
  reader->_logger->info(utl::ODB, 128, "Design: {}", design);
  assert(reader->_block);
  reader->setBlock(reader->_block);
  return PARSE_OK;
}

int definReader::blockageCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    DefParser::defiBlockage* blockage,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definBlockage* blockageR = reader->_blockageR.get();

  if (blockage->hasMask()) {
    UNSUPPORTED("MASK on blockage is unsupported");
  }

  if (blockage->hasLayer()) {
    // routing blockage
    blockageR->blockageRoutingBegin(blockage->layerName());

    if (blockage->hasSlots()) {
      blockageR->blockageRoutingSlots();
    }

    if (blockage->hasFills()) {
      blockageR->blockageRoutingFills();
    }

    if (blockage->hasExceptpgnet()) {
      blockageR->blockageRoutingExceptPGNets();
    }

    if (blockage->hasPushdown()) {
      blockageR->blockageRoutingPushdown();
    }

    if (blockage->hasSpacing()) {
      blockageR->blockageRoutingMinSpacing(blockage->minSpacing());
    }

    if (blockage->hasDesignRuleWidth()) {
      blockageR->blockageRoutingEffectiveWidth(blockage->designRuleWidth());
    }

    if (blockage->hasComponent()) {
      blockageR->blockageRoutingComponent(blockage->placementComponentName());
    }

    for (int i = 0; i < blockage->numRectangles(); ++i) {
      blockageR->blockageRoutingRect(
          blockage->xl(i), blockage->yl(i), blockage->xh(i), blockage->yh(i));
    }

    for (int i = 0; i < blockage->numPolygons(); ++i) {
      DefParser::defiPoints defPoints = blockage->getPolygon(i);
      std::vector<Point> points;
      reader->translate(defPoints, points);
      blockageR->blockageRoutingPolygon(points);
    }

    blockageR->blockageRoutingEnd();
  } else {
    // placement blockage
    blockageR->blockagePlacementBegin();

    if (blockage->hasComponent()) {
      blockageR->blockagePlacementComponent(blockage->placementComponentName());
    }

    if (blockage->hasPushdown()) {
      blockageR->blockagePlacementPushdown();
    }

    if (blockage->hasSoft()) {
      blockageR->blockagePlacementSoft();
    }

    if (blockage->hasPartial()) {
      blockageR->blockagePlacementMaxDensity(blockage->placementMaxDensity());
    }

    for (int i = 0; i < blockage->numRectangles(); ++i) {
      blockageR->blockagePlacementRect(
          blockage->xl(i), blockage->yl(i), blockage->xh(i), blockage->yh(i));
    }

    blockageR->blockagePlacementEnd();
  }

  return PARSE_OK;
}

int definReader::componentsCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    DefParser::defiComponent* comp,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definComponent* componentR = reader->_componentR.get();
  std::string id = comp->id();
  if (reader->_mode != defin::DEFAULT) {
    if (reader->_block->findInst(id.c_str()) == nullptr) {
      // Try escaping the hierarchy and see if that matches
      boost::replace_all(id, "/", "\\/");
      if (reader->_block->findInst(id.c_str()) == nullptr) {
        std::string modeStr
            = reader->_mode == defin::FLOORPLAN ? "FLOORPLAN" : "INCREMENTAL";
        reader->_logger->warn(
            utl::ODB,
            248,
            "skipping undefined comp {} encountered in {} DEF",
            comp->id(),
            modeStr);
        return PARSE_OK;
      }
    }
  }

  if (comp->hasEEQ()) {
    UNSUPPORTED("EEQMASTER on component is unsupported");
  }

  if (comp->maskShiftSize() > 0) {
    UNSUPPORTED("MASKSHIFT on component is unsupported");
  }

  if (comp->hasRouteHalo() > 0) {
    UNSUPPORTED("ROUTEHALO on component is unsupported");
  }

  componentR->begin(id.c_str(), comp->name());
  if (comp->hasSource()) {
    componentR->source(dbSourceType(comp->source()));
  }
  if (comp->hasWeight()) {
    componentR->weight(comp->weight());
  }
  if (comp->hasRegionName()) {
    componentR->region(comp->regionName());
  }
  if (comp->hasHalo() > 0) {
    int left, bottom, right, top;
    comp->haloEdges(&left, &bottom, &right, &top);
    componentR->halo(left, bottom, right, top);
  }

  componentR->placement(comp->placementStatus(),
                        comp->placementX(),
                        comp->placementY(),
                        comp->placementOrient());

  handle_props(comp, componentR);

  componentR->end();

  return PARSE_OK;
}

int definReader::componentMaskShiftCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    DefParser::defiComponentMaskShiftLayer* shiftLayers,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  for (int i = 0; i < shiftLayers->numMaskShiftLayers(); i++) {
    reader->_componentMaskShift->addLayer(shiftLayers->maskShiftLayer(i));
  }

  reader->_componentMaskShift->setLayers();

  return PARSE_OK;
}

int definReader::dieAreaCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    DefParser::defiBox* box,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  const DefParser::defiPoints points = box->getPoint();

  if (reader->_mode == defin::DEFAULT || reader->_mode == defin::FLOORPLAN) {
    std::vector<Point> P;
    reader->translate(points, P);

    if (P.size() < 2) {
      UNSUPPORTED("Invalid DIEAREA statement, missing point(s)");
    }

    if (P.size() == 2) {
      Point p0 = P[0];
      Point p1 = P[1];
      Rect r(p0.getX(), p0.getY(), p1.getX(), p1.getY());
      reader->_block->setDieArea(r);
    } else {
      Polygon die_area_poly(P);
      reader->_block->setDieArea(die_area_poly);
    }
  }
  return PARSE_OK;
}

int definReader::extensionCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    const char* /* unused: extension */,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  UNSUPPORTED("Syntax extensions (BEGINEXT/ENDEXT) are unsupported");
  return PARSE_OK;
}

int definReader::fillsCallback(DefParser::defrCallbackType_e /* unused: type */,
                               int /* unused: count */,
                               DefParser::defiUserData data)
{
  return PARSE_OK;
}

int definReader::fillCallback(DefParser::defrCallbackType_e /* unused: type */,
                              DefParser::defiFill* fill,
                              DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definFill* fillR = reader->_fillR.get();

  if (fill->hasVia() || fill->hasViaOpc()) {
    UNSUPPORTED("Via fill is unsupported");
  }

  if (fill->numPolygons() > 0) {
    UNSUPPORTED("Polygon fill is unsupported");
  }

  if (fill->hasLayer()) {
    fillR->fillBegin(fill->layerName(), fill->hasLayerOpc(), fill->layerMask());
  }

  for (int i = 0; i < fill->numRectangles(); ++i) {
    fillR->fillRect(fill->xl(i), fill->yl(i), fill->xh(i), fill->yh(i));
  }

  for (int i = 0; i < fill->numPolygons(); ++i) {
    DefParser::defiPoints defPoints = fill->getPolygon(i);
    std::vector<Point> points;
    reader->translate(defPoints, points);

    fillR->fillPolygon(points);
  }

  fillR->fillEnd();

  return PARSE_OK;
}

int definReader::gcellGridCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    DefParser::defiGcellGrid* grid,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  defDirection dir = (grid->macro()[0] == 'X') ? DEF_X : DEF_Y;

  reader->_gcellR->gcell(dir, grid->x(), grid->xNum(), grid->xStep());

  return PARSE_OK;
}

int definReader::groupNameCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    const char* name,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  reader->_groupR->begin(name);
  return PARSE_OK;
}

int definReader::groupMemberCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    const char* member,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  reader->_groupR->inst(member);
  return PARSE_OK;
}

int definReader::groupCallback(DefParser::defrCallbackType_e /* unused: type */,
                               DefParser::defiGroup* group,
                               DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definGroup* groupR = reader->_groupR.get();
  if (group->hasRegionName()) {
    groupR->region(group->regionName());
  }
  handle_props(group, groupR);
  groupR->end();

  return PARSE_OK;
}

int definReader::historyCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    const char* /* unused: extension */,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  UNSUPPORTED("HISTORY is unsupported");
  return PARSE_OK;
}

int definReader::netCallback(DefParser::defrCallbackType_e /* unused: type */,
                             DefParser::defiNet* net,
                             DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definNet* netR = reader->_netR.get();
  if (reader->_mode == defin::FLOORPLAN
      && reader->_block->findNet(net->name()) == nullptr) {
    reader->_logger->warn(
        utl::ODB,
        275,
        "skipping undefined net {} encountered in FLOORPLAN DEF",
        net->name());
    return PARSE_OK;
  }
  if (net->numShieldNets() > 0) {
    UNSUPPORTED("SHIELDNET on net is unsupported");
  }

  if (net->numVpins() > 0) {
    UNSUPPORTED("VPIN on net is unsupported");
  }

  if (net->hasSubnets()) {
    UNSUPPORTED("SUBNET on net is unsupported");
  }

  if (net->hasXTalk()) {
    UNSUPPORTED("XTALK on net is unsupported");
  }

  if (net->hasFrequency()) {
    UNSUPPORTED("FREQUENCY on net is unsupported");
  }

  if (net->hasOriginal()) {
    UNSUPPORTED("ORIGINAL on net is unsupported");
  }

  if (net->hasPattern()) {
    UNSUPPORTED("PATTERN on net is unsupported");
  }

  if (net->hasCap()) {
    UNSUPPORTED("ESTCAP on net is unsupported");
  }

  netR->begin(net->name());

  if (net->hasUse()) {
    netR->use(net->use());
  }

  if (net->hasSource()) {
    netR->source(net->source());
  }

  if (net->hasFixedbump()) {
    netR->fixedbump();
  }

  if (net->hasWeight()) {
    netR->weight(net->weight());
  }

  if (net->hasNonDefaultRule()) {
    netR->nonDefaultRule(net->nonDefaultRule());
  }

  for (int i = 0; i < net->numConnections(); ++i) {
    if (net->pinIsSynthesized(i)) {
      UNSUPPORTED("SYNTHESIZED on net's connection is unsupported");
    }

    if (net->pinIsMustJoin(i)) {
      netR->beginMustjoin(net->instance(i), net->pin(i));
    } else {
      netR->connection(net->instance(i), net->pin(i));
    }
  }

  for (int i = 0; i < net->numWires(); ++i) {
    DefParser::defiWire* wire = net->wire(i);
    netR->wire(wire->wireType());

    for (int j = 0; j < wire->numPaths(); ++j) {
      DefParser::defiPath* path = wire->path(j);

      path->initTraverse();

      int pathId;
      while ((pathId = path->next()) != DefParser::DEFIPATH_DONE) {
        switch (pathId) {
          case DefParser::DEFIPATH_LAYER: {
            // We need to peek ahead to see if there is a taper next
            const char* layer = path->getLayer();
            int nextId = path->next();
            if (nextId == DefParser::DEFIPATH_TAPER) {
              netR->pathTaper(layer);
            } else if (nextId == DefParser::DEFIPATH_TAPERRULE) {
              netR->pathTaperRule(layer, path->getTaperRule());
            } else {
              netR->path(layer);
              path->prev();  // put back the token
            }
            break;
          }

          case DefParser::DEFIPATH_VIA: {
            // We need to peek ahead to see if there is a rotation next
            const char* viaName = path->getVia();
            int nextId = path->next();
            if (nextId == DefParser::DEFIPATH_VIAROTATION) {
              netR->pathVia(viaName,
                            translate_orientation(path->getViaRotation()));
            } else {
              netR->pathVia(viaName);
              path->prev();  // put back the token
            }
            break;
          }

          case DefParser::DEFIPATH_POINT: {
            int x;
            int y;
            path->getPoint(&x, &y);
            netR->pathPoint(x, y);
            break;
          }

          case DefParser::DEFIPATH_FLUSHPOINT: {
            int x;
            int y;
            int ext;
            path->getFlushPoint(&x, &y, &ext);
            netR->pathPoint(x, y, ext);
            break;
          }

          case DefParser::DEFIPATH_STYLE:
            UNSUPPORTED("styles are not supported on wires");
            break;

          case DefParser::DEFIPATH_RECT: {
            int deltaX1;
            int deltaY1;
            int deltaX2;
            int deltaY2;
            path->getViaRect(&deltaX1, &deltaY1, &deltaX2, &deltaY2);
            netR->pathRect(deltaX1, deltaY1, deltaX2, deltaY2);
            break;
          }

          case DefParser::DEFIPATH_VIRTUALPOINT:
            UNSUPPORTED("VIRTUAL in net's routing is unsupported");
            break;

          case DefParser::DEFIPATH_MASK:
            netR->pathColor(path->getMask());
            break;

          case DefParser::DEFIPATH_VIAMASK:
            netR->pathViaColor(path->getViaBottomMask(),
                               path->getViaCutMask(),
                               path->getViaTopMask());
            break;

          default:
            UNSUPPORTED("Unknown construct in net's routing is unsupported");
            break;
        }
      }
      netR->pathEnd();
    }

    netR->wireEnd();
  }

  handle_props(net, netR);

  netR->end();

  return PARSE_OK;
}

int definReader::nonDefaultRuleCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    DefParser::defiNonDefault* rule,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definNonDefaultRule* ruleR = reader->_non_default_ruleR.get();

  ruleR->beginRule(rule->name());

  if (rule->hasHardspacing()) {
    ruleR->hardSpacing();
  }

  for (int i = 0; i < rule->numLayers(); ++i) {
    if (rule->hasLayerDiagWidth(i)) {
      UNSUPPORTED("DIAGWIDTH on non-default rule is unsupported");
    }

    ruleR->beginLayerRule(rule->layerName(i), rule->layerWidthVal(i));

    if (rule->hasLayerSpacing(i)) {
      ruleR->spacing(rule->layerSpacingVal(i));
    }

    if (rule->hasLayerWireExt(i)) {
      ruleR->wireExt(rule->layerWireExtVal(i));
    }

    ruleR->endLayerRule();
  }

  for (int i = 0; i < rule->numVias(); ++i) {
    ruleR->via(rule->viaName(i));
  }

  for (int i = 0; i < rule->numViaRules(); ++i) {
    ruleR->viaRule(rule->viaRuleName(i));
  }

  for (int i = 0; i < rule->numMinCuts(); ++i) {
    ruleR->minCuts(rule->cutLayerName(i), rule->numCuts(i));
  }

  handle_props(rule, ruleR);

  ruleR->endRule();

  return PARSE_OK;
}

int definReader::pinCallback(DefParser::defrCallbackType_e /* unused: type */,
                             DefParser::defiPin* pin,
                             DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definPin* pinR = reader->_pinR.get();
  if (reader->_mode != defin::DEFAULT
      && reader->_block->findBTerm(pin->pinName()) == nullptr) {
    std::string modeStr
        = reader->_mode == defin::FLOORPLAN ? "FLOORPLAN" : "INCREMENTAL";
    reader->_logger->warn(utl::ODB,
                          247,
                          "skipping undefined pin {} encountered in {} DEF",
                          pin->pinName(),
                          modeStr);
    return PARSE_OK;
  }

  if (pin->numVias() > 0) {
    UNSUPPORTED("VIA in pins is unsupported");
  }

  if (pin->hasNetExpr()) {
    UNSUPPORTED("NETEXPR on pin is unsupported");
  }

  if (pin->hasAPinPartialMetalArea() || pin->hasAPinPartialMetalSideArea()
      || pin->hasAPinDiffArea() || pin->hasAPinPartialCutArea()
      || pin->numAntennaModel() > 0) {
    UNSUPPORTED("Antenna data on pin is unsupported");
  }

  if (pin->numPolygons() > 0) {
    // The db does support polygons but the callback code seems incorrect to me
    // (ignores layers!).  Delaying support until I can fix it.
    UNSUPPORTED("polygons in pins are not supported");
  }

  pinR->pinBegin(pin->pinName(), pin->netName());

  if (pin->hasSpecial()) {
    pinR->pinSpecial();
  }

  if (pin->hasUse()) {
    pinR->pinUse(pin->use());
  }

  if (pin->hasDirection()) {
    if (reader->_mode == defin::FLOORPLAN) {
      if (!pinR->checkPinDirection(pin->direction())) {
        reader->_logger->warn(
            utl::ODB,
            437,
            "Mismatched pin direction between verilog netlist and floorplan "
            "DEF, ignoring floorplan DEF direction.");
      }
    } else {
      pinR->pinDirection(pin->direction());
    }
  }

  if (pin->hasSupplySensitivity()) {
    pinR->pinSupplyPin(pin->supplySensitivity());
  }

  if (pin->hasGroundSensitivity()) {
    pinR->pinGroundPin(pin->groundSensitivity());
  }

  // Add all ports associated with the pin above
  if (pin->hasPort()) {
    // 5.7 .. Multiple ports each with multiple boxes/shapes
    for (int i = 0; i < pin->numPorts(); ++i) {
      DefParser::defiPinPort* port = pin->pinPort(i);
      pinR->portBegin();

      // Configure placement for port
      if (port->hasPlacement()) {
        defPlacement type = DEF_PLACEMENT_UNPLACED;
        if (port->isPlaced()) {
          type = DEF_PLACEMENT_PLACED;
        } else if (port->isCover()) {
          type = DEF_PLACEMENT_COVER;
        } else if (port->isFixed()) {
          type = DEF_PLACEMENT_FIXED;
        } else {
          assert(0);
        }
        dbOrientType orient
            = odb::definReader::translate_orientation(port->orient());
        pinR->pinPlacement(
            type, port->placementX(), port->placementY(), orient);
      }

      // For a given port, add all boxes/shapes belonging to that port
      for (int i = 0; i < port->numLayer(); ++i) {
        uint32_t mask = port->layerMask(i);

        int xl, yl, xh, yh;
        port->bounds(i, &xl, &yl, &xh, &yh);
        pinR->pinRect(port->layer(i), xl, yl, xh, yh, mask);

        if (port->hasLayerSpacing(i)) {
          pinR->pinMinSpacing(port->layerSpacing(i));
        }

        if (port->hasLayerDesignRuleWidth(i)) {
          pinR->pinEffectiveWidth(port->layerDesignRuleWidth(i));
        }
      }

      pinR->portEnd();
    }

  } else {
    // 5.6 .. All boxes implicity belong to one port
    pinR->portBegin();

    // Configure placement for pin
    if (pin->hasPlacement()) {
      defPlacement type = DEF_PLACEMENT_UNPLACED;
      if (pin->isPlaced()) {
        type = DEF_PLACEMENT_PLACED;
      } else if (pin->isCover()) {
        type = DEF_PLACEMENT_COVER;
      } else if (pin->isFixed()) {
        type = DEF_PLACEMENT_FIXED;
      } else {
        assert(0);
      }
      dbOrientType orient
          = odb::definReader::translate_orientation(pin->orient());
      pinR->pinPlacement(type, pin->placementX(), pin->placementY(), orient);
    }

    // Add boxes/shapes for the pin with single port
    for (int i = 0; i < pin->numLayer(); ++i) {
      uint32_t mask = pin->layerMask(i);

      int xl, yl, xh, yh;
      pin->bounds(i, &xl, &yl, &xh, &yh);
      pinR->pinRect(pin->layer(i), xl, yl, xh, yh, mask);

      if (pin->hasLayerSpacing(i)) {
        pinR->pinMinSpacing(pin->layerSpacing(i));
      }

      if (pin->hasLayerDesignRuleWidth(i)) {
        pinR->pinEffectiveWidth(pin->layerDesignRuleWidth(i));
      }
    }
    pinR->portEnd();
  }

  pinR->pinEnd();

  return PARSE_OK;
}

int definReader::pinsEndCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    void* /* unused: v */,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  reader->_pinR->pinsEnd();
  return PARSE_OK;
}

int definReader::pinPropCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    DefParser::defiPinProp* prop,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definPinProps* propR = reader->_pin_propsR.get();

  propR->begin(prop->isPin() ? "PIN" : prop->instName(), prop->pinName());
  handle_props(prop, propR);
  propR->end();

  return PARSE_OK;
}

int definReader::pinsStartCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    int number,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  reader->_pinR->pinsBegin(number);
  return PARSE_OK;
}

int definReader::propCallback(DefParser::defrCallbackType_e /* unused: type */,
                              DefParser::defiProp* prop,
                              DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definPropDefs* prop_defsR = reader->_prop_defsR.get();

  defPropType data_type;
  switch (prop->dataType()) {
    case 'I':
      data_type = DEF_INTEGER;
      break;
    case 'R':
      data_type = DEF_REAL;
      break;
    case 'S': /* fallthru */
    case 'N': /* fallthru */
    case 'Q':
      data_type = DEF_STRING;
      break;
    default:
      assert(0);
      return PARSE_ERROR;
  }

  // The prop type should be upper case for consistency
  std::string prop_type(prop->propType());
  for (auto& c : prop_type) {
    c = toupper(c);
  }

  prop_defsR->begin(prop_type.c_str(), prop->propName(), data_type);

  if (prop->hasRange()) {
    if (data_type == DEF_INTEGER) {
      // Call the integer overload
      prop_defsR->range((int) prop->left(), (int) prop->right());
    } else {
      assert(data_type == DEF_REAL);
      // Call the double overload
      prop_defsR->range(prop->left(), prop->right());
    }
  }

  switch (data_type) {
    case DEF_INTEGER:
      if (prop->hasNumber()) {
        prop_defsR->value((int) prop->number());  // int overload
      }
      break;
    case DEF_REAL:
      if (prop->hasNumber()) {
        prop_defsR->value(prop->number());  // double overload
      }
      break;
    case DEF_STRING:
      if (prop->hasString()) {
        prop_defsR->value(prop->string());  // string overload
      }
      break;
  }

  prop_defsR->end();

  return PARSE_OK;
}

int definReader::propEndCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    void* /* unused: v */,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  reader->_prop_defsR->endDefinitions();
  return PARSE_OK;
}

int definReader::propStartCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    void* /* unused: v */,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  reader->_prop_defsR->beginDefinitions();
  return PARSE_OK;
}

int definReader::regionCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    DefParser::defiRegion* region,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definRegion* regionR = reader->_regionR.get();

  regionR->begin(region->name());

  for (int i = 0; i < region->numRectangles(); ++i) {
    regionR->boundary(
        region->xl(i), region->yl(i), region->xh(i), region->yh(i));
  }

  if (region->hasType()) {
    const char* type = region->type();
    if (strcmp(type, "FENCE") == 0) {
      regionR->type(DEF_FENCE);
    } else {
      assert(strcmp(type, "GUIDE") == 0);
      regionR->type(DEF_GUIDE);
    }
  }

  handle_props(region, regionR);

  regionR->end();

  return PARSE_OK;
}

int definReader::rowCallback(DefParser::defrCallbackType_e /* unused: type */,
                             DefParser::defiRow* row,
                             DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definRow* rowR = reader->_rowR.get();

  defRow dir = DEF_HORIZONTAL;
  int num_sites = 1;
  int spacing = 0;

  if (row->hasDo()) {
    if (row->yNum() == 1) {
      dir = DEF_HORIZONTAL;
      num_sites = row->xNum();
      if (row->hasDoStep()) {
        spacing = row->xStep();
      }
    } else {
      dir = DEF_VERTICAL;
      num_sites = row->yNum();
      if (row->hasDoStep()) {
        spacing = row->yStep();
      }
    }
  }

  rowR->begin(row->name(),
              row->macro(),
              row->x(),
              row->y(),
              odb::definReader::translate_orientation(row->orient()),
              dir,
              num_sites,
              spacing);

  handle_props(row, rowR);

  reader->_rowR->end();

  return PARSE_OK;
}

int definReader::scanchainsStartCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    int chain_count,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  // unused callback. see scanchainsCallback
  return PARSE_OK;
}

int definReader::scanchainsCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    DefParser::defiScanchain* scan_chain,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK

  dbBlock* block = reader->_block;
  dbDft* dft = block->getDft();

  dbScanChain* db_scan_chain = dbScanChain::create(dft);
  db_scan_chain->setName(scan_chain->name());

  dbScanPartition* db_scan_partition = dbScanPartition::create(db_scan_chain);
  db_scan_partition->setName(scan_chain->partitionName());

  char* start_inst_name;
  char* stop_inst_name;
  char* start_pin_name;
  char* stop_pin_name;
  scan_chain->start(&start_inst_name, &start_pin_name);
  scan_chain->stop(&stop_inst_name, &stop_pin_name);

  auto scan_in_pin
      = findScanTerm(reader, block, "START", start_inst_name, start_pin_name);
  auto scan_out_pin
      = findScanTerm(reader, block, "STOP", stop_inst_name, stop_pin_name);
  if (!scan_in_pin.has_value() || !scan_out_pin.has_value()) {
    if (reader->_continue_on_errors) {
      return PARSE_OK;
    }
    return PARSE_ERROR;
  }

  std::visit([db_scan_chain](auto&& pin) { db_scan_chain->setScanIn(pin); },
             *scan_in_pin);
  std::visit([db_scan_chain](auto&& pin) { db_scan_chain->setScanOut(pin); },
             *scan_out_pin);

  // Get floating elements, each floating element is in its own dbScanList
  int floating_size = 0;
  char** floating_inst = nullptr;
  char** floating_in_pin = nullptr;
  char** floating_out_pin = nullptr;
  int* floating_bits = nullptr;
  scan_chain->floating(&floating_size,
                       &floating_inst,
                       &floating_in_pin,
                       &floating_out_pin,
                       &floating_bits);

  for (int i = 0; i < floating_size; ++i) {
    dbScanList* db_scan_list = dbScanList::create(db_scan_partition);
    const char* inst_name = floating_inst[i];
    const char* in_pin_name = floating_in_pin[i];
    const char* out_pin_name = floating_out_pin[i];
    populateScanInst(reader,
                     block,
                     scan_chain,
                     db_scan_list,
                     inst_name,
                     in_pin_name,
                     out_pin_name,
                     floating_bits[i]);
  }

  // Get the ordered elements
  const int number_ordered = scan_chain->numOrderedLists();
  for (int index = 0; index < number_ordered; ++index) {
    int size = 0;
    char** insts = nullptr;
    char** in_pins = nullptr;
    char** out_pins = nullptr;
    int* bits = nullptr;
    scan_chain->ordered(index, &size, &insts, &in_pins, &out_pins, &bits);

    if (size == 0) {
      continue;
    }

    dbScanList* db_scan_list = dbScanList::create(db_scan_partition);

    // creating a dbScanList with the components
    for (int i = 0; i < size; ++i) {
      const char* inst_name = insts[i];
      const char* in_pin_name = in_pins[i];
      const char* out_pin_name = out_pins[i];
      populateScanInst(reader,
                       block,
                       scan_chain,
                       db_scan_list,
                       inst_name,
                       in_pin_name,
                       out_pin_name,
                       bits[i]);
    }

    dbSet<dbScanInst> db_scan_insts = db_scan_list->getScanInsts();

    if (db_scan_insts.reversible() && db_scan_insts.orderReversed()) {
      db_scan_insts.reverse();
    }
  }

  return PARSE_OK;
}

int definReader::slotsCallback(DefParser::defrCallbackType_e /* unused: type */,
                               int /* unused: count */,
                               DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  UNSUPPORTED("SLOTS are unsupported");
  return PARSE_OK;
}

int definReader::stylesCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    int /* unused: count */,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  UNSUPPORTED("STYLES are unsupported");
  return PARSE_OK;
}

int definReader::technologyCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    const char* /* unused: name */,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  reader->_logger->warn(utl::ODB, 293, "TECHNOLOGY is ignored");
  return PARSE_OK;
}

int definReader::trackCallback(DefParser::defrCallbackType_e /* unused: type */,
                               DefParser::defiTrack* track,
                               DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK

  defDirection dir = track->macro()[0] == 'X' ? DEF_X : DEF_Y;
  reader->_tracksR->tracksBegin(dir,
                                track->x(),
                                track->xNum(),
                                track->xStep(),
                                track->firstTrackMask(),
                                track->sameMask() == 1);

  for (int i = 0; i < track->numLayers(); ++i) {
    reader->_tracksR->tracksLayer(track->layer(i));
  }

  reader->_tracksR->tracksEnd();
  return PARSE_OK;
}

int definReader::unitsCallback(DefParser::defrCallbackType_e type,
                               double d,
                               DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK

  // Truncation error
  if (d > reader->_tech->getDbUnitsPerMicron()) {
    UNSUPPORTED(
        fmt::format("The DEF UNITS DISTANCE MICRONS convert factor ({}) is "
                    "greater than the database units per micron ({}) value.",
                    d,
                    reader->_tech->getDbUnitsPerMicron()));
  }

  reader->units(d);

  std::vector<definBase*>::iterator itr;
  for (itr = reader->_interfaces.begin(); itr != reader->_interfaces.end();
       ++itr) {
    (*itr)->units(d);
  }

  reader->_block->setDefUnits(d);
  return PARSE_OK;
}

int definReader::viaCallback(DefParser::defrCallbackType_e /* unused: type */,
                             DefParser::defiVia* via,
                             DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definVia* viaR = reader->_viaR.get();

  if (via->numPolygons() > 0) {
    UNSUPPORTED("POLYGON in via is unsupported");
  }

  viaR->viaBegin(via->name());

  if (via->hasViaRule()) {
    char* viaRuleName;
    int xSize;
    int ySize;
    char* botLayer;
    char* cutLayer;
    char* topLayer;
    int xCutSpacing;
    int yCutSpacing;
    int xBotEnc;
    int yBotEnc;
    int xTopEnc;
    int yTopEnc;
    via->viaRule(&viaRuleName,
                 &xSize,
                 &ySize,
                 &botLayer,
                 &cutLayer,
                 &topLayer,
                 &xCutSpacing,
                 &yCutSpacing,
                 &xBotEnc,
                 &yBotEnc,
                 &xTopEnc,
                 &yTopEnc);
    viaR->viaRule(viaRuleName);
    viaR->viaCutSize(xSize, ySize);
    if (!viaR->viaLayers(botLayer, cutLayer, topLayer)) {
      if (!reader->_continue_on_errors) {
        return PARSE_ERROR;
      }
    }
    viaR->viaCutSpacing(xCutSpacing, yCutSpacing);
    viaR->viaEnclosure(xBotEnc, yBotEnc, xTopEnc, yTopEnc);

    if (via->hasRowCol()) {
      int numCutRows;
      int numCutCols;
      via->rowCol(&numCutRows, &numCutCols);
      viaR->viaRowCol(numCutRows, numCutCols);
    }

    if (via->hasOrigin()) {
      int xOffset;
      int yOffset;
      via->origin(&xOffset, &yOffset);
      viaR->viaOrigin(xOffset, yOffset);
    }

    if (via->hasOffset()) {
      int xBotOffset;
      int yBotOffset;
      int xTopOffset;
      int yTopOffset;
      via->offset(&xBotOffset, &yBotOffset, &xTopOffset, &yTopOffset);
      viaR->viaOffset(xBotOffset, yBotOffset, xTopOffset, yTopOffset);
    }

    if (via->hasCutPattern()) {
      viaR->viaPattern(via->cutPattern());
    }
  }

  for (int i = 0; i < via->numLayers(); ++i) {
    if (via->hasRectMask(i)) {
      UNSUPPORTED("MASK on via rect is unsupported");
    }

    char* layer;
    int xl;
    int yl;
    int xh;
    int yh;
    via->layer(i, &layer, &xl, &yl, &xh, &yh);
    viaR->viaRect(layer, xl, yl, xh, yh);
  }

  viaR->viaEnd();

  return PARSE_OK;
}

int definReader::specialNetCallback(
    DefParser::defrCallbackType_e /* unused: type */,
    DefParser::defiNet* net,
    DefParser::defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definSNet* snetR = reader->_snetR.get();
  if (reader->_mode == defin::FLOORPLAN
      && reader->_block->findNet(net->name()) == nullptr) {
    reader->_logger->warn(
        utl::ODB,
        249,
        "skipping undefined net {} encountered in FLOORPLAN DEF",
        net->name());
    return PARSE_OK;
  }
  if (net->hasCap()) {
    UNSUPPORTED("ESTCAP on special net is unsupported");
  }

  if (net->hasPattern()) {
    UNSUPPORTED("PATTERN on special net is unsupported");
  }

  if (net->hasOriginal()) {
    UNSUPPORTED("ORIGINAL on special net is unsupported");
  }

  if (net->numShieldNets() > 0) {
    UNSUPPORTED("SHIELDNET on special net is unsupported");
  }

  if (net->hasVoltage()) {
    UNSUPPORTED("VOLTAGE on special net is unsupported");
  }

  if (net->numPolygons() > 0) {
    // The db does support polygons but the callback code seems incorrect to me
    // (ignores layers!).  Delaying support until I can fix it.
    UNSUPPORTED("polygons in special nets are not supported");
  }

  if (net->numViaSpecs() > 0) {
    UNSUPPORTED("VIA in special net is unsupported");
  }

  snetR->begin(net->name());

  if (net->hasUse()) {
    snetR->use(net->use());
  }

  if (net->hasSource()) {
    snetR->source(net->source());
  }

  if (net->hasFixedbump()) {
    snetR->fixedbump();
  }

  if (net->hasWeight()) {
    snetR->weight(net->weight());
  }

  for (int i = 0; i < net->numConnections(); ++i) {
    snetR->connection(net->instance(i), net->pin(i), net->pinIsSynthesized(i));
  }

  if (net->numRectangles()) {
    for (int i = 0; i < net->numRectangles(); i++) {
      snetR->wire(net->rectRouteStatus(i), net->rectRouteStatusShieldName(i));
      snetR->rect(net->rectName(i),
                  net->xl(i),
                  net->yl(i),
                  net->xh(i),
                  net->yh(i),
                  net->rectShapeType(i),
                  net->rectMask(i));
      snetR->wireEnd();
    }
  }

  for (int i = 0; i < net->numWires(); ++i) {
    DefParser::defiWire* wire = net->wire(i);
    snetR->wire(wire->wireType(), wire->wireShieldNetName());

    for (int j = 0; j < wire->numPaths(); ++j) {
      DefParser::defiPath* path = wire->path(j);

      path->initTraverse();

      std::string layerName;

      int pathId;
      uint32_t next_mask = 0;
      uint32_t next_via_bottom_mask = 0;
      uint32_t next_via_cut_mask = 0;
      uint32_t next_via_top_mask = 0;
      while ((pathId = path->next()) != DefParser::DEFIPATH_DONE) {
        switch (pathId) {
          case DefParser::DEFIPATH_LAYER:
            layerName = path->getLayer();
            break;

          case DefParser::DEFIPATH_VIA: {
            // We need to peek ahead to see if there is a rotation next
            const char* viaName = path->getVia();
            int nextId = path->next();
            if (nextId == DefParser::DEFIPATH_VIAROTATION) {
              UNSUPPORTED("Rotated via in special net is unsupported");
              // TODO: Make this take and store rotation
              // snetR->pathVia(viaName,
              //                translate_orientation(path->getViaRotation()));
            } else if (nextId == DefParser::DEFIPATH_VIADATA) {
              int numX, numY, stepX, stepY;
              path->getViaData(&numX, &numY, &stepX, &stepY);
              snetR->pathViaArray(viaName, numX, numY, stepX, stepY);
            } else {
              snetR->pathVia(viaName,
                             next_via_bottom_mask,
                             next_via_cut_mask,
                             next_via_top_mask);
              path->prev();  // put back the token
            }
            break;
          }

          case DefParser::DEFIPATH_WIDTH:
            assert(!layerName.empty());  // always "layerName routeWidth"
            snetR->path(layerName.c_str(), path->getWidth());
            break;

          case DefParser::DEFIPATH_POINT: {
            int x;
            int y;
            path->getPoint(&x, &y);
            snetR->pathPoint(x, y, next_mask);
            break;
          }

          case DefParser::DEFIPATH_FLUSHPOINT: {
            int x;
            int y;
            int ext;
            path->getFlushPoint(&x, &y, &ext);
            snetR->pathPoint(x, y, ext, next_mask);
            break;
          }

          case DefParser::DEFIPATH_SHAPE:
            snetR->pathShape(path->getShape());
            break;

          case DefParser::DEFIPATH_STYLE:
            UNSUPPORTED("styles are not supported on wires");
            break;

          case DefParser::DEFIPATH_MASK:
            next_mask = path->getMask();
            break;

          case DefParser::DEFIPATH_VIAMASK:
            next_via_bottom_mask = path->getViaBottomMask();
            next_via_cut_mask = path->getViaCutMask();
            next_via_top_mask = path->getViaTopMask();
            break;

          default:
            UNSUPPORTED(
                "Unknown construct in special net's routing is unsupported");
        }
        if (pathId != DefParser::DEFIPATH_MASK) {
          next_mask = 0;
        }
        if (pathId != DefParser::DEFIPATH_VIAMASK) {
          next_via_bottom_mask = 0;
          next_via_cut_mask = 0;
          next_via_top_mask = 0;
        }
      }
      snetR->pathEnd();
    }

    snetR->wireEnd();
  }

  handle_props(net, snetR);

  snetR->end();

  return PARSE_OK;
}

void definReader::contextLogFunctionCallback(DefParser::defiUserData data,
                                             const char* msg)
{
  definReader* reader = (definReader*) data;
  reader->_logger->warn(utl::ODB, 3, msg);
}

void definReader::contextWarningLogFunctionCallback(
    DefParser::defiUserData data,
    const char* msg)
{
  definReader* reader = (definReader*) data;
  reader->_logger->warn(utl::ODB, 4, msg);
}

void definReader::line(int line_num)
{
  _logger->info(utl::ODB, 125, "lines processed: {}", line_num);
}

void definReader::error(std::string_view msg)
{
  _logger->warn(utl::ODB, 126, "error: {}", msg);
  ++_errors;
}

void definReader::setLibs(std::vector<dbLib*>& lib_names)
{
  _componentR->setLibs(lib_names);
  _rowR->setLibs(lib_names);
}

void definReader::readChip(std::vector<dbLib*>& libs,
                           const char* file,
                           dbChip* chip,
                           const bool issue_callback)
{
  init();
  setLibs(libs);
  chip_ = chip;
  if (chip_ == nullptr) {
    _logger->error(utl::ODB, 250, "Chip does not exist");
  }
  if (_mode == defin::DEFAULT && chip_->getBlock() != nullptr) {
    _logger->error(utl::ODB, 251, "Chip already has a block");
  }

  assert(chip_);
  setTech(chip_->getTech());
  _logger->info(utl::ODB, 127, "Reading DEF file: {}", file);

  if (!createBlock(file)) {
    dbChip::destroy(chip);
    _logger->warn(utl::ODB, 129, "Error: Failed to read DEF file");
    return;
  }

  if (_pinR->_bterm_cnt) {
    _logger->info(utl::ODB, 130, "    Created {} pins.", _pinR->_bterm_cnt);
  }

  if (_pinR->_update_cnt) {
    _logger->info(utl::ODB, 252, "    Updated {} pins.", _pinR->_update_cnt);
  }

  if (_componentR->_inst_cnt) {
    _logger->info(utl::ODB,
                  131,
                  "    Created {} components and {} component-terminals.",
                  _componentR->_inst_cnt,
                  _componentR->_iterm_cnt);
  }

  if (_componentR->_update_cnt) {
    _logger->info(
        utl::ODB, 253, "    Updated {} components.", _componentR->_update_cnt);
  }

  if (_snetR->_snet_cnt) {
    _logger->info(utl::ODB,
                  132,
                  "    Created {} special nets and {} connections.",
                  _snetR->_snet_cnt,
                  _snetR->_snet_iterm_cnt);
  }

  if (_netR->_net_cnt) {
    _logger->info(utl::ODB,
                  133,
                  "    Created {} nets and {} connections.",
                  _netR->_net_cnt,
                  _netR->_net_iterm_cnt);
  }

  if (_netR->_update_cnt) {
    _logger->info(utl::ODB,
                  254,
                  "    Updated {} nets and {} connections.",
                  _netR->_update_cnt,
                  _netR->_net_iterm_cnt);
  }

  _logger->info(utl::ODB, 134, "Finished DEF file: {}", file);

  if (issue_callback) {
    _db->triggerPostReadDef(_block, _mode == defin::FLOORPLAN);
  }
}

static inline bool hasSuffix(const std::string& str, const std::string& suffix)
{
  return str.size() >= suffix.size()
         && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool definReader::createBlock(const char* file)
{
  DefParser::defrInit();
  DefParser::defrReset();

  DefParser::defrInitSession();
  // FOR DEFAULT || FLOORPLAN || INCREMENTAL
  defrSetVersionStrCbk(versionCallback);
  defrSetBusBitCbk(busBitCallback);
  defrSetDividerCbk(divideCharCallback);
  defrSetDesignCbk(designCallback);
  defrSetUnitsCbk(unitsCallback);
  defrSetComponentCbk(componentsCallback);
  defrSetComponentMaskShiftLayerCbk(componentMaskShiftCallback);
  defrSetPinCbk(pinCallback);
  defrSetPinEndCbk(pinsEndCallback);
  defrSetPinPropCbk(pinPropCallback);
  DefParser::defrSetContextLogFunction(contextLogFunctionCallback);
  DefParser::defrSetContextWarningLogFunction(
      contextWarningLogFunctionCallback);

  if (_mode == defin::DEFAULT || _mode == defin::FLOORPLAN) {
    defrSetDieAreaCbk(dieAreaCallback);
    defrSetTrackCbk(trackCallback);
    defrSetRowCbk(rowCallback);
    defrSetNetCbk(netCallback);
    defrSetSNetCbk(specialNetCallback);
    defrSetViaCbk(viaCallback);
    defrSetBlockageCbk(blockageCallback);
    defrSetNonDefaultCbk(nonDefaultRuleCallback);

    DefParser::defrSetAddPathToNet();
  }

  if (_mode == defin::DEFAULT) {
    defrSetPropCbk(propCallback);
    defrSetPropDefEndCbk(propEndCallback);
    defrSetPropDefStartCbk(propStartCallback);

    defrSetExtensionCbk(extensionCallback);
    defrSetFillStartCbk(fillsCallback);
    defrSetFillCbk(fillCallback);
    defrSetGcellGridCbk(gcellGridCallback);
    defrSetGroupCbk(groupCallback);
    defrSetGroupMemberCbk(groupMemberCallback);
    defrSetGroupNameCbk(groupNameCallback);
    defrSetHistoryCbk(historyCallback);

    defrSetRegionCbk(regionCallback);
    defrSetSlotStartCbk(slotsCallback);

    defrSetStartPinsCbk(pinsStartCallback);
    defrSetStylesStartCbk(stylesCallback);
    defrSetTechnologyCbk(technologyCallback);
  }

  if (_mode == defin::INCREMENTAL || _mode == defin::DEFAULT) {
    defrSetScanchainsStartCbk(scanchainsStartCallback);
    defrSetScanchainCbk(scanchainsCallback);
  }

  bool isZipped = hasSuffix(file, ".gz");
  int res;
  if (!isZipped) {
    FILE* f = fopen(file, "r");
    if (f == nullptr) {
      _logger->warn(utl::ODB, 148, "error: Cannot open DEF file {}", file);
      return false;
    }
    res = DefParser::defrRead(
        f, file, (DefParser::defiUserData) this, /* case sensitive */ 1);
    fclose(f);
  } else {
    DefParser::defrSetGZipReadFunction();
    DefParser::defGZFile f = DefParser::defrGZipOpen(file, "r");
    if (f == nullptr) {
      _logger->warn(
          utl::ODB, 271, "error: Cannot open zipped DEF file {}", file);
      return false;
    }
    res = DefParser::defrReadGZip(f, file, (DefParser::defiUserData) this);
    DefParser::defGZipClose(f);
  }

  if (res != 0 || errors() != 0) {
    if (!_continue_on_errors) {
      _logger->error(utl::ODB, 421, "DEF parser returns an error!");
    } else {
      _logger->warn(utl::ODB, 149, "DEF parser returns an error!");
    }
  }

  DefParser::defrClear();

  return true;
  // 1220 return errors() == 0;
}

}  // namespace odb
