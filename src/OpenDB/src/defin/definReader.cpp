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

#include "definReader.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>

#include "db.h"
#include "dbShape.h"
#include "definBlockage.h"
#include "definComponent.h"
#include "definFill.h"
#include "definGCell.h"
#include "definNet.h"
#include "definNonDefaultRule.h"
#include "definPin.h"
#include "definPinProps.h"
#include "definPropDefs.h"
#include "definRegion.h"
#include "definRow.h"
#include "definSNet.h"
#include "definTracks.h"
#include "definVia.h"
#include "defzlib.hpp"
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

definReader::definReader(dbDatabase* db, utl::Logger* logger, defin::MODE mode)
{
  _db = db;
  _block_name = NULL;
  parent_ = nullptr;
  _continue_on_errors = false;
  version_ = nullptr;
  hier_delimeter_ = 0;
  left_bus_delimeter_ = 0;
  right_bus_delimeter_ = 0;

  definBase::setLogger(logger);
  definBase::setMode(mode);

  _blockageR = new definBlockage;
  _componentR = new definComponent;
  _fillR = new definFill;
  _gcellR = new definGCell;
  _netR = new definNet;
  _pinR = new definPin;
  _rowR = new definRow;
  _snetR = new definSNet;
  _tracksR = new definTracks;
  _viaR = new definVia;
  _regionR = new definRegion;
  _non_default_ruleR = new definNonDefaultRule;
  _prop_defsR = new definPropDefs;
  _pin_propsR = new definPinProps;

  _interfaces.push_back(_blockageR);
  _interfaces.push_back(_componentR);
  _interfaces.push_back(_fillR);
  _interfaces.push_back(_gcellR);
  _interfaces.push_back(_netR);
  _interfaces.push_back(_pinR);
  _interfaces.push_back(_rowR);
  _interfaces.push_back(_snetR);
  _interfaces.push_back(_tracksR);
  _interfaces.push_back(_viaR);
  _interfaces.push_back(_regionR);
  _interfaces.push_back(_non_default_ruleR);
  _interfaces.push_back(_prop_defsR);
  _interfaces.push_back(_pin_propsR);
  init();
}

definReader::~definReader()
{
  delete _blockageR;
  delete _componentR;
  delete _fillR;
  delete _gcellR;
  delete _netR;
  delete _pinR;
  delete _rowR;
  delete _snetR;
  delete _tracksR;
  delete _viaR;
  delete _regionR;
  delete _non_default_ruleR;
  delete _prop_defsR;
  delete _pin_propsR;

  if (_block_name)
    free((void*) _block_name);
}

int definReader::errors()
{
  int e = _errors;

  std::vector<definBase*>::iterator itr;
  for (itr = _interfaces.begin(); itr != _interfaces.end(); ++itr)
    e += (*itr)->_errors;

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

void definReader::replaceWires()
{
  _netR->replaceWires();
  _snetR->replaceWires();
}

void definReader::namesAreDBIDs()
{
  _netR->namesAreDBIDs();
  _snetR->namesAreDBIDs();
}

void definReader::setAssemblyMode()
{
  _netR->setAssemblyMode();
}

void definReader::useBlockName(const char* name)
{
  if (_block_name)
    free((void*) _block_name);

  _block_name = strdup(name);
  assert(_block_name);
}

void definReader::init()
{
  std::vector<definBase*>::iterator itr;
  for (itr = _interfaces.begin(); itr != _interfaces.end(); ++itr) {
    (*itr)->init();
    (*itr)->setLogger(_logger);
    (*itr)->setMode(_mode);
  }
  _update = false;
}

void definReader::setTech(dbTech* tech)
{
  definBase::setTech(tech);

  std::vector<definBase*>::iterator itr;
  for (itr = _interfaces.begin(); itr != _interfaces.end(); ++itr)
    (*itr)->setTech(tech);
}

void definReader::setBlock(dbBlock* block)
{
  definBase::setBlock(block);
  std::vector<definBase*>::iterator itr;
  for (itr = _interfaces.begin(); itr != _interfaces.end(); ++itr)
    (*itr)->setBlock(block);
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

static std::string renameBlock(dbBlock* parent, const char* old_name)
{
  int cnt = 1;

  for (;; ++cnt) {
    char n[16];
    snprintf(n, 15, "_%d", cnt);
    std::string name(old_name);
    name += n;

    if (!parent->findChild(name.c_str()))
      return name;
  }
}

int definReader::versionCallback(defrCallbackType_e /* unused: type */,
                                 const char* value,
                                 defiUserData data)
{
  definReader* reader = (definReader*) data;
  reader->version_ = strdup(value);
  return PARSE_OK;
}

int definReader::divideCharCallback(defrCallbackType_e /* unused: type */,
                                    const char* value,
                                    defiUserData data)
{
  definReader* reader = (definReader*) data;
  reader->hier_delimeter_ = value[0];
  if (reader->hier_delimeter_ == 0) {
    reader->error("Syntax error in DIVIDERCHAR statment");
    return PARSE_ERROR;
  }
  return PARSE_OK;
}
int definReader::busBitCallback(defrCallbackType_e /* unused: type */,
                                const char* value,
                                defiUserData data)
{
  definReader* reader = (definReader*) data;
  reader->left_bus_delimeter_ = value[0];
  reader->right_bus_delimeter_ = value[1];
  if ((reader->left_bus_delimeter_ == 0)
      || (reader->right_bus_delimeter_ == 0)) {
    reader->error("Syntax error in BUSBITCHARS statment");
    return PARSE_ERROR;
  }
  return PARSE_OK;
}
int definReader::designCallback(defrCallbackType_e /* unused: type */,
                                const char* design,
                                defiUserData data)
{
  definReader* reader = (definReader*) data;
  std::string block_name;
  if (reader->_block_name)
    block_name = reader->_block_name;
  else
    block_name = design;
  if (reader->parent_ != nullptr) {
    if (reader->parent_->findChild(block_name.c_str())) {
      if (reader->_mode != defin::DEFAULT)
        reader->_block = reader->parent_->findChild(block_name.c_str());
      else {
        std::string new_name = renameBlock(reader->parent_, block_name.c_str());
        reader->_logger->warn(
            utl::ODB,
            261,
            "Block with name \"{}\" already exists, renaming too \"{}\"",
            block_name.c_str(),
            new_name.c_str());
        reader->_block = dbBlock::create(
            reader->parent_, new_name.c_str(), reader->hier_delimeter_);
      }
    } else
      reader->_block = dbBlock::create(
          reader->parent_, block_name.c_str(), reader->hier_delimeter_);
  } else {
    dbChip* chip = reader->_db->getChip();
    if (reader->_mode != defin::DEFAULT)
      reader->_block = chip->getBlock();
    else
      reader->_block
          = dbBlock::create(chip, block_name.c_str(), reader->hier_delimeter_);
  }
  if (reader->_mode == defin::DEFAULT)
    reader->_block->setBusDelimeters(reader->left_bus_delimeter_,
                                     reader->right_bus_delimeter_);
  reader->_logger->info(utl::ODB, 128, "Design: {}", design);
  assert(reader->_block);
  reader->setBlock(reader->_block);
  return PARSE_OK;
}

int definReader::blockageCallback(defrCallbackType_e /* unused: type */,
                                  defiBlockage* blockage,
                                  defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definBlockage* blockageR = reader->_blockageR;

  if (blockage->hasExceptpgnet()) {
    UNSUPPORTED("EXCEPTPGNET on blockage is unsupported");
  }

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
      defiPoints defPoints = blockage->getPolygon(i);
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

int definReader::componentsCallback(defrCallbackType_e /* unused: type */,
                                    defiComponent* comp,
                                    defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definComponent* componentR = reader->_componentR;
  if (reader->_mode != defin::DEFAULT
      && reader->_block->findInst(comp->id()) == nullptr) {
    std::string modeStr
        = reader->_mode == defin::FLOORPLAN ? "FLOORPLAN" : "INCREMENTAL";
    reader->_logger->warn(utl::ODB,
                          248,
                          "skipping undefined comp {} encountered in {} DEF",
                          comp->id(),
                          modeStr);
    return PARSE_OK;
  }

  if (comp->hasEEQ()) {
    UNSUPPORTED("EEQMASTER on component is unsupported");
  }

  if (comp->maskShiftSize() > 0) {
    UNSUPPORTED("MASKSHIFT on component is unsupported");
  }

  if (comp->hasHalo() > 0) {
    UNSUPPORTED("HALO on component is unsupported");
  }

  if (comp->hasRouteHalo() > 0) {
    UNSUPPORTED("ROUTEHALO on component is unsupported");
  }

  componentR->begin(comp->id(), comp->name());
  if (comp->hasSource()) {
    componentR->source(dbSourceType(comp->source()));
  }
  if (comp->hasWeight()) {
    componentR->weight(comp->weight());
  }
  if (comp->hasRegionName()) {
    componentR->region(comp->regionName());
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
    defrCallbackType_e /* unused: type */,
    defiComponentMaskShiftLayer* /* unused: shiftLayers */,
    defiUserData data)
{
  definReader* reader = (definReader*) data;
  UNSUPPORTED("COMPONENTMASKSHIFT is unsupported");
  return PARSE_OK;
}

int definReader::dieAreaCallback(defrCallbackType_e /* unused: type */,
                                 defiBox* box,
                                 defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  const defiPoints points = box->getPoint();

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
      reader->_logger->warn(
          utl::ODB,
          124,
          "warning: Polygon DIEAREA statement not supported.  The bounding "
          "box will be used instead");
      int xmin = INT_MAX;
      int ymin = INT_MAX;
      int xmax = INT_MIN;
      int ymax = INT_MIN;
      std::vector<Point>::iterator itr;

      for (itr = P.begin(); itr != P.end(); ++itr) {
        Point& p = *itr;
        int x = p.getX();
        int y = p.getY();

        if (x < xmin)
          xmin = x;

        if (y < ymin)
          ymin = y;

        if (x > xmax)
          xmax = x;

        if (y > ymax)
          ymax = y;
      }

      Rect r(xmin, ymin, xmax, ymax);
      reader->_block->setDieArea(r);
    }
  }
  return PARSE_OK;
}

int definReader::extensionCallback(defrCallbackType_e /* unused: type */,
                                   const char* /* unused: extension */,
                                   defiUserData data)
{
  definReader* reader = (definReader*) data;
  UNSUPPORTED("Syntax extensions (BEGINEXT/ENDEXT) are unsupported");
  return PARSE_OK;
}

int definReader::fillsCallback(defrCallbackType_e /* unused: type */,
                               int /* unused: count */,
                               defiUserData data)
{
  return PARSE_OK;
}

int definReader::fillCallback(defrCallbackType_e /* unused: type */,
                              defiFill* fill,
                              defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definFill* fillR = reader->_fillR;

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
    defiPoints defPoints = fill->getPolygon(i);
    std::vector<Point> points;
    reader->translate(defPoints, points);

    fillR->fillPolygon(points);
  }

  fillR->fillEnd();

  return PARSE_OK;
}

int definReader::gcellGridCallback(defrCallbackType_e /* unused: type */,
                                   defiGcellGrid* grid,
                                   defiUserData data)
{
  definReader* reader = (definReader*) data;
  defDirection dir = (grid->macro()[0] == 'X') ? DEF_X : DEF_Y;

  reader->_gcellR->gcell(dir, grid->x(), grid->xNum(), grid->xStep());

  return PARSE_OK;
}

int definReader::groupNameCallback(defrCallbackType_e /* unused: type */,
                                   const char* name,
                                   defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  reader->_regionR->begin(name, /* group */ true);
  return PARSE_OK;
}

int definReader::groupMemberCallback(defrCallbackType_e /* unused: type */,
                                     const char* member,
                                     defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  reader->_regionR->inst(member);
  return PARSE_OK;
}

int definReader::groupCallback(defrCallbackType_e /* unused: type */,
                               defiGroup* group,
                               defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definRegion* regionR = reader->_regionR;

  if (group->hasRegionName()) {
    regionR->parent(group->regionName());
  }
  handle_props(group, regionR);
  regionR->end();

  return PARSE_OK;
}

int definReader::historyCallback(defrCallbackType_e /* unused: type */,
                                 const char* /* unused: extension */,
                                 defiUserData data)
{
  definReader* reader = (definReader*) data;
  UNSUPPORTED("HISTORY is unsupported");
  return PARSE_OK;
}

int definReader::netCallback(defrCallbackType_e /* unused: type */,
                             defiNet* net,
                             defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definNet* netR = reader->_netR;
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
    defiWire* wire = net->wire(i);
    netR->wire(wire->wireType());

    for (int j = 0; j < wire->numPaths(); ++j) {
      defiPath* path = wire->path(j);

      path->initTraverse();

      int pathId;
      while ((pathId = path->next()) != DEFIPATH_DONE) {
        switch (pathId) {
          case DEFIPATH_LAYER: {
            // We need to peek ahead to see if there is a taper next
            const char* layer = path->getLayer();
            int nextId = path->next();
            if (nextId == DEFIPATH_TAPER) {
              netR->pathTaper(layer);
            } else if (nextId == DEFIPATH_TAPERRULE) {
              netR->pathTaperRule(layer, path->getTaperRule());
            } else {
              netR->path(layer);
              path->prev();  // put back the token
            }
            break;
          }

          case DEFIPATH_VIA: {
            // We need to peek ahead to see if there is a rotation next
            const char* viaName = path->getVia();
            int nextId = path->next();
            if (nextId == DEFIPATH_VIAROTATION) {
              netR->pathVia(viaName,
                            translate_orientation(path->getViaRotation()));
            } else {
              netR->pathVia(viaName);
              path->prev();  // put back the token
            }
            break;
          }

          case DEFIPATH_POINT: {
            int x;
            int y;
            path->getPoint(&x, &y);
            netR->pathPoint(x, y);
            break;
          }

          case DEFIPATH_FLUSHPOINT: {
            int x;
            int y;
            int ext;
            path->getFlushPoint(&x, &y, &ext);
            netR->pathPoint(x, y, ext);
            break;
          }

          case DEFIPATH_STYLE:
            UNSUPPORTED("styles are not supported on wires");
            break;

          case DEFIPATH_RECT: {
            int deltaX1;
            int deltaY1;
            int deltaX2;
            int deltaY2;
            path->getViaRect(&deltaX1, &deltaY1, &deltaX2, &deltaY2);
            netR->pathRect(deltaX1, deltaY1, deltaX2, deltaY2);
            break;
          }

          case DEFIPATH_VIRTUALPOINT:
            UNSUPPORTED("VIRTUAL in net's routing is unsupported");
            break;

          case DEFIPATH_MASK:
          case DEFIPATH_VIAMASK:
            UNSUPPORTED("MASK in net's routing is unsupported");
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

int definReader::nonDefaultRuleCallback(defrCallbackType_e /* unused: type */,
                                        defiNonDefault* rule,
                                        defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definNonDefaultRule* ruleR = reader->_non_default_ruleR;

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

int definReader::pinCallback(defrCallbackType_e /* unused: type */,
                             defiPin* pin,
                             defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definPin* pinR = reader->_pinR;
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
    pinR->pinDirection(pin->direction());
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
      defiPinPort* port = pin->pinPort(i);
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
        dbOrientType orient = reader->translate_orientation(port->orient());
        pinR->pinPlacement(
            type, port->placementX(), port->placementY(), orient);
      }

      // For a given port, add all boxes/shapes belonging to that port
      for (int i = 0; i < port->numLayer(); ++i) {
        if (port->layerMask(i) != 0) {
          UNSUPPORTED("MASK on pin's layer is unsupported");
        }

        int xl, yl, xh, yh;
        port->bounds(i, &xl, &yl, &xh, &yh);
        pinR->pinRect(port->layer(i), xl, yl, xh, yh);

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
      dbOrientType orient = reader->translate_orientation(pin->orient());
      pinR->pinPlacement(type, pin->placementX(), pin->placementY(), orient);
    }

    // Add boxes/shapes for the pin with single port
    for (int i = 0; i < pin->numLayer(); ++i) {
      if (pin->layerMask(i) != 0) {
        UNSUPPORTED("MASK on pin's layer is unsupported");
      }

      int xl, yl, xh, yh;
      pin->bounds(i, &xl, &yl, &xh, &yh);
      pinR->pinRect(pin->layer(i), xl, yl, xh, yh);

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

int definReader::pinsEndCallback(defrCallbackType_e /* unused: type */,
                                 void* /* unused: v */,
                                 defiUserData data)
{
  definReader* reader = (definReader*) data;
  reader->_pinR->pinsEnd();
  return PARSE_OK;
}

int definReader::pinPropCallback(defrCallbackType_e /* unused: type */,
                                 defiPinProp* prop,
                                 defiUserData data)
{
  definReader* reader = (definReader*) data;
  definPinProps* propR = reader->_pin_propsR;

  propR->begin(prop->isPin() ? "PIN" : prop->instName(), prop->pinName());
  handle_props(prop, propR);
  propR->end();

  return PARSE_OK;
}

int definReader::pinsStartCallback(defrCallbackType_e /* unused: type */,
                                   int number,
                                   defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  reader->_pinR->pinsBegin(number);
  return PARSE_OK;
}

int definReader::propCallback(defrCallbackType_e /* unused: type */,
                              defiProp* prop,
                              defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definPropDefs* prop_defsR = reader->_prop_defsR;

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

int definReader::propEndCallback(defrCallbackType_e /* unused: type */,
                                 void* /* unused: v */,
                                 defiUserData data)
{
  definReader* reader = (definReader*) data;
  reader->_prop_defsR->endDefinitions();
  return PARSE_OK;
}

int definReader::propStartCallback(defrCallbackType_e /* unused: type */,
                                   void* /* unused: v */,
                                   defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  reader->_prop_defsR->beginDefinitions();
  return PARSE_OK;
}

int definReader::regionCallback(defrCallbackType_e /* unused: type */,
                                defiRegion* region,
                                defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definRegion* regionR = reader->_regionR;

  regionR->begin(region->name(), /* is_group */ false);

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

int definReader::rowCallback(defrCallbackType_e /* unused: type */,
                             defiRow* row,
                             defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definRow* rowR = reader->_rowR;

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
              reader->translate_orientation(row->orient()),
              dir,
              num_sites,
              spacing);

  handle_props(row, rowR);

  reader->_rowR->end();

  return PARSE_OK;
}

int definReader::scanchainsCallback(defrCallbackType_e /* unused: type */,
                                    int /* unused: count */,
                                    defiUserData data)
{
  definReader* reader = (definReader*) data;
  UNSUPPORTED("SCANCHAINS are unsupported");
  return PARSE_OK;
}

int definReader::slotsCallback(defrCallbackType_e /* unused: type */,
                               int /* unused: count */,
                               defiUserData data)
{
  definReader* reader = (definReader*) data;
  UNSUPPORTED("SLOTS are unsupported");
  return PARSE_OK;
}

int definReader::stylesCallback(defrCallbackType_e /* unused: type */,
                                int /* unused: count */,
                                defiUserData data)
{
  definReader* reader = (definReader*) data;
  UNSUPPORTED("STYLES are unsupported");
  return PARSE_OK;
}

int definReader::technologyCallback(defrCallbackType_e /* unused: type */,
                                    const char* /* unused: name */,
                                    defiUserData data)
{
  definReader* reader = (definReader*) data;
  UNSUPPORTED("TECHNOLOGY is unsupported");
  return PARSE_OK;
}

int definReader::trackCallback(defrCallbackType_e /* unused: type */,
                               defiTrack* track,
                               defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  if (track->firstTrackMask() != 0) {
    UNSUPPORTED("MASK on track is unsupported");
  }

  defDirection dir = track->macro()[0] == 'X' ? DEF_X : DEF_Y;
  reader->_tracksR->tracksBegin(dir, track->x(), track->xNum(), track->xStep());

  for (int i = 0; i < track->numLayers(); ++i) {
    reader->_tracksR->tracksLayer(track->layer(i));
  }

  reader->_tracksR->tracksEnd();
  return PARSE_OK;
}

int definReader::unitsCallback(defrCallbackType_e, double d, defiUserData data)
{
  definReader* reader = (definReader*) data;

  // Truncation error
  if (d > reader->_tech->getDbUnitsPerMicron()) {
    char buf[256];
    sprintf(buf,
            "The DEF UNITS DISTANCE MICRONS convert factor (%d) is "
            "greater than the database units per micron (%d) value.",
            (int) d,
            reader->_tech->getDbUnitsPerMicron());
    UNSUPPORTED(buf);
  }

  reader->units(d);

  std::vector<definBase*>::iterator itr;
  for (itr = reader->_interfaces.begin(); itr != reader->_interfaces.end();
       ++itr)
    (*itr)->units(d);

  if (!reader->_update)
    reader->_block->setDefUnits(d);
  return PARSE_OK;
}

int definReader::viaCallback(defrCallbackType_e /* unused: type */,
                             defiVia* via,
                             defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definVia* viaR = reader->_viaR;

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

int definReader::specialNetCallback(defrCallbackType_e /* unused: type */,
                                    defiNet* net,
                                    defiUserData data)
{
  definReader* reader = (definReader*) data;
  CHECKBLOCK
  definSNet* snetR = reader->_snetR;
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
      snetR->wire(net->rectShapeType(i), net->rectRouteStatusShieldName(i));
      snetR->rect(
          net->rectName(i), net->xl(i), net->yl(i), net->xh(i), net->yh(i));
      snetR->wireEnd();
    }
  }

  for (int i = 0; i < net->numWires(); ++i) {
    defiWire* wire = net->wire(i);
    snetR->wire(wire->wireType(), wire->wireShieldNetName());

    for (int j = 0; j < wire->numPaths(); ++j) {
      defiPath* path = wire->path(j);

      path->initTraverse();

      std::string layerName;

      int pathId;
      while ((pathId = path->next()) != DEFIPATH_DONE) {
        switch (pathId) {
          case DEFIPATH_LAYER:
            layerName = path->getLayer();
            break;

          case DEFIPATH_VIA: {
            // We need to peek ahead to see if there is a rotation next
            const char* viaName = path->getVia();
            int nextId = path->next();
            if (nextId == DEFIPATH_VIAROTATION) {
              UNSUPPORTED("Rotated via in special net is unsupported");
              // TODO: Make this take and store rotation
              // snetR->pathVia(viaName,
              //                translate_orientation(path->getViaRotation()));
            } else {
              snetR->pathVia(viaName);
              path->prev();  // put back the token
            }
            break;
          }

          case DEFIPATH_WIDTH:
            assert(!layerName.empty());  // always "layerName routeWidth"
            snetR->path(layerName.c_str(), path->getWidth());
            break;

          case DEFIPATH_POINT: {
            int x;
            int y;
            path->getPoint(&x, &y);
            snetR->pathPoint(x, y);
            break;
          }

          case DEFIPATH_FLUSHPOINT: {
            int x;
            int y;
            int ext;
            path->getFlushPoint(&x, &y, &ext);
            snetR->pathPoint(x, y, ext);
            break;
          }

          case DEFIPATH_SHAPE:
            snetR->pathShape(path->getShape());
            break;

          case DEFIPATH_STYLE:
            UNSUPPORTED("styles are not supported on wires");
            break;

          case DEFIPATH_MASK:
          case DEFIPATH_VIAMASK:
            UNSUPPORTED("MASK in special net's routing is unsupported");

          default:
            UNSUPPORTED(
                "Unknown construct in special net's routing is unsupported");
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

void definReader::line(int line_num)
{
  _logger->info(utl::ODB, 125, "lines processed: {}", line_num);
}

void definReader::error(const char* msg)
{
  _logger->warn(utl::ODB, 126, "error: {}", msg);
  ++_errors;
}

void definReader::setLibs(std::vector<dbLib*>& libs)
{
  _componentR->setLibs(libs);
  _rowR->setLibs(libs);
}

dbChip* definReader::createChip(std::vector<dbLib*>& libs, const char* file)
{
  init();
  setLibs(libs);
  dbChip* chip = _db->getChip();
  if (_mode != defin::DEFAULT) {
    if (chip == nullptr)
      _logger->error(utl::ODB, 250, "Chip does not exist");
  } else if (chip != nullptr) {
    fprintf(stderr, "Error: Chip already exists\n");
    return NULL;
  } else
    chip = dbChip::create(_db);

  assert(chip);
  setTech(_db->getTech());
  _logger->info(utl::ODB, 127, "Reading DEF file: {}", file);

  if (!createBlock(file)) {
    dbChip::destroy(chip);
    _logger->warn(utl::ODB, 129, "Error: Failed to read DEF file");
    return NULL;
  }

  if (_pinR->_bterm_cnt)
    _logger->info(utl::ODB, 130, "    Created {} pins.", _pinR->_bterm_cnt);

  if (_pinR->_update_cnt)
    _logger->info(utl::ODB, 252, "    Updated {} pins.", _pinR->_update_cnt);

  if (_componentR->_inst_cnt)
    _logger->info(utl::ODB,
                  131,
                  "    Created {} components and {} component-terminals.",
                  _componentR->_inst_cnt,
                  _componentR->_iterm_cnt);

  if (_componentR->_update_cnt)
    _logger->info(
        utl::ODB, 253, "    Updated {} components.", _componentR->_update_cnt);

  if (_snetR->_snet_cnt)
    _logger->info(utl::ODB,
                  132,
                  "    Created {} special nets and {} connections.",
                  _snetR->_snet_cnt,
                  _snetR->_snet_iterm_cnt);

  if (_netR->_net_cnt)
    _logger->info(utl::ODB,
                  133,
                  "    Created {} nets and {} connections.",
                  _netR->_net_cnt,
                  _netR->_net_iterm_cnt);

  if (_netR->_update_cnt)
    _logger->info(utl::ODB,
                  254,
                  "    Updated {} nets and {} connections.",
                  _netR->_update_cnt,
                  _netR->_net_iterm_cnt);

  _logger->info(utl::ODB, 134, "Finished DEF file: {}", file);
  return chip;
}

dbBlock* definReader::createBlock(dbBlock* parent,
                                  std::vector<dbLib*>& libs,
                                  const char* def_file)
{
  init();
  setLibs(libs);
  parent_ = parent;
  setTech(_db->getTech());
  _logger->info(utl::ODB, 135, "Reading DEF file: {}", def_file);

  if (!createBlock(def_file)) {
    dbBlock::destroy(_block);
    _logger->warn(utl::ODB, 137, "Error: Failed to read DEF file");
    return NULL;
  }

  if (_pinR->_bterm_cnt)
    _logger->info(utl::ODB, 138, "    Created {} pins.", _pinR->_bterm_cnt);

  if (_componentR->_inst_cnt)
    _logger->info(utl::ODB,
                  139,
                  "    Created {} components and {} component-terminals.",
                  _componentR->_inst_cnt,
                  _componentR->_iterm_cnt);

  if (_snetR->_snet_cnt)
    _logger->info(utl::ODB,
                  140,
                  "    Created {} special nets and {} connections.",
                  _snetR->_snet_cnt,
                  _snetR->_snet_iterm_cnt);

  if (_netR->_net_cnt)
    _logger->info(utl::ODB,
                  141,
                  "    Created {} nets and {} connections.",
                  _netR->_net_cnt,
                  _netR->_net_iterm_cnt);

  _logger->info(utl::ODB, 142, "Finished DEF file: {}", def_file);

  return _block;
}

bool definReader::replaceWires(dbBlock* block, const char* def_file)
{
  init();
  setBlock(block);
  setTech(_db->getTech());

  _logger->info(utl::ODB, 143, "Reading DEF file: {}", def_file);

  if (!replaceWires(def_file)) {
    // dbBlock::destroy(_block);
    _logger->warn(utl::ODB, 144, "Error: Failed to read DEF file");
    return false;
  }

  if (_snetR->_snet_cnt)
    _logger->info(
        utl::ODB, 145, "    Processed {} special nets.", _snetR->_snet_cnt);

  if (_netR->_net_cnt)
    _logger->info(utl::ODB, 146, "    Processed {} nets.", _netR->_net_cnt);

  _logger->info(utl::ODB, 147, "Finished DEF file: {}", def_file);
  return errors() == 0;
}

static inline bool hasSuffix(const std::string& str, const std::string& suffix)
{
  return str.size() >= suffix.size()
         && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool definReader::createBlock(const char* file)
{
  defrInit();
  defrReset();

  defrInitSession();
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

  if (_mode == defin::DEFAULT || _mode == defin::FLOORPLAN) {
    defrSetDieAreaCbk(dieAreaCallback);
    defrSetTrackCbk(trackCallback);
    defrSetRowCbk(rowCallback);
    defrSetNetCbk(netCallback);
    defrSetSNetCbk(specialNetCallback);
  }

  if (_mode == defin::DEFAULT) {
    defrSetPropCbk(propCallback);
    defrSetPropDefEndCbk(propEndCallback);
    defrSetPropDefStartCbk(propStartCallback);
    defrSetBlockageCbk(blockageCallback);

    defrSetExtensionCbk(extensionCallback);
    defrSetFillStartCbk(fillsCallback);
    defrSetFillCbk(fillCallback);
    defrSetGcellGridCbk(gcellGridCallback);
    defrSetGroupCbk(groupCallback);
    defrSetGroupMemberCbk(groupMemberCallback);
    defrSetGroupNameCbk(groupNameCallback);
    defrSetHistoryCbk(historyCallback);

    defrSetNonDefaultCbk(nonDefaultRuleCallback);
    defrSetRegionCbk(regionCallback);

    defrSetScanchainsStartCbk(scanchainsCallback);
    defrSetSlotStartCbk(slotsCallback);

    defrSetStartPinsCbk(pinsStartCallback);
    defrSetStylesStartCbk(stylesCallback);
    defrSetTechnologyCbk(technologyCallback);

    defrSetViaCbk(viaCallback);

    defrSetAddPathToNet();
  }

  bool isZipped = hasSuffix(file, ".gz");
  int res;
  if (!isZipped) {
    FILE* f = fopen(file, "r");
    if (f == NULL) {
      _logger->warn(utl::ODB, 148, "error: Cannot open DEF file {}", file);
      return false;
    }
    res = defrRead(f, file, (defiUserData) this, /* case sensitive */ 1);
    fclose(f);
  } else {
    defrSetGZipReadFunction();
    defGZFile f = defrGZipOpen(file, "r");
    if (f == NULL) {
      _logger->warn(
          utl::ODB, 271, "error: Cannot open zipped DEF file {}", file);
      return false;
    }
    res = defrReadGZip(f, file, (defiUserData) this);
    defGZipClose(f);
  }

  if (res != 0 || _errors != 0) {
    _logger->warn(utl::ODB, 149, "DEF parser returns an error!");
    if (!_continue_on_errors) {
      exit(2);
    }
  }

  defrClear();

  return true;
  // 1220 return errors() == 0;
}

bool definReader::replaceWires(const char* file)
{
  FILE* f = fopen(file, "r");

  if (f == NULL) {
    _logger->warn(utl::ODB, 150, "error: Cannot open DEF file {}", file);
    return false;
  }

  replaceWires();

  defrInit();
  defrReset();

  defrInitSession();

  defrSetNetCbk(netCallback);
  defrSetSNetCbk(specialNetCallback);

  defrSetAddPathToNet();

  int res = defrRead(f, file, (defiUserData) this, /* case sensitive */ 1);
  if (res != 0) {
    _logger->warn(utl::ODB, 151, "DEF parser returns an error!");
    if (!_continue_on_errors) {
      exit(2);
    }
  }

  defrClear();

  return true;
}

}  // namespace odb
