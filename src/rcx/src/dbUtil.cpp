// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rcx/dbUtil.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"
#include "utl/Logger.h"

using odb::dbBPin;
using odb::dbIoType;
using odb::dbPlacementStatus;
using odb::dbSet;
using odb::dbSigType;
using odb::dbSWire;
using odb::dbWire;
using odb::dbWireEncoder;
using odb::dbWirePath;
using odb::dbWirePathItr;
using odb::dbWirePathShape;
using odb::dbWireShapeType;
using odb::dbWireType;
using odb::Point;
using utl::RCX;

namespace rcx {

dbCreateNetUtil::dbCreateNetUtil(utl::Logger* logger)
    : _tech(nullptr),
      _block(nullptr),
      _ruleNameHint(0),
      _milosFormat(false),
      _currentNet(nullptr),
      _mapArray(nullptr),
      _mapCnt(0),
      _ecoCnt(0),
      logger_(logger),
      _skipPowerNets(true),
      _useLocation(false),
      _verbose(false)
{
}
dbCreateNetUtil::~dbCreateNetUtil()
{
  if (_mapArray != nullptr) {
    free(_mapArray);
  }
}
void dbCreateNetUtil::setCurrentNet(odb::dbNet* net)
{
  _currentNet = net;
}

odb::dbInst* dbCreateNetUtil::createInst(odb::dbInst* inst0)
{
  char instName[64];
  sprintf(instName, "N%d", inst0->getId());

  odb::dbInst* inst = odb::dbInst::create(_block, inst0->getMaster(), instName);
  if (inst == nullptr) {
    return nullptr;
  }

  inst->setOrient(inst0->getOrient());
  const Point origin = inst0->getOrigin();
  inst->setOrigin(origin.x(), origin.y());
  inst->setPlacementStatus(inst0->getPlacementStatus());

  return inst;
}

void dbCreateNetUtil::setBlock(odb::dbBlock* block, bool skipInit)
{
  _block = block;
  if (skipInit) {
    return;
  }

  _tech = block->getDb()->getTech();
  _ruleNameHint = 0;
  _routingLayers.clear();
  _rules.clear();

  int layerCount = _tech->getRoutingLayerCount();
  _rules.resize(layerCount + 1);
  _routingLayers.resize(layerCount + 1);

  dbSet<odb::dbTechLayer> layers = _tech->getLayers();
  dbSet<odb::dbTechLayer>::iterator itr;

  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    odb::dbTechLayer* layer = *itr;
    int rlevel = layer->getRoutingLevel();

    if (rlevel > 0) {
      _routingLayers[rlevel] = layer;
    }
  }

  // Build mapping table to rule widths
  dbSet<odb::dbTechNonDefaultRule> nd_rules = _tech->getNonDefaultRules();
  dbSet<odb::dbTechNonDefaultRule>::iterator nditr;
  // odb::dbTechNonDefaultRule  *wdth_rule = nullptr;

  for (nditr = nd_rules.begin(); nditr != nd_rules.end(); ++nditr) {
    odb::dbTechNonDefaultRule* nd_rule = *nditr;
    std::vector<odb::dbTechLayerRule*> layer_rules;
    nd_rule->getLayerRules(layer_rules);
    std::vector<odb::dbTechLayerRule*>::iterator lritr;

    for (lritr = layer_rules.begin(); lritr != layer_rules.end(); ++lritr) {
      odb::dbTechLayerRule* rule = *lritr;

      int rlevel = rule->getLayer()->getRoutingLevel();

      if (rlevel > 0) {
        odb::dbTechLayerRule*& r = _rules[rlevel][rule->getWidth()];

        if (r == nullptr) {  // Don't overide any existing rule.
          r = rule;
        }
      }
    }
  }

  _vias.clear();
  _vias.resize(layerCount + 1, layerCount + 1);

  dbSet<odb::dbTechVia> vias = _tech->getVias();
  dbSet<odb::dbTechVia>::iterator vitr;

  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    odb::dbTechVia* via = *vitr;
    odb::dbTechLayer* bot = via->getBottomLayer();
    odb::dbTechLayer* top = via->getTopLayer();

    int topR = top->getRoutingLevel();
    int botR = bot->getRoutingLevel();

    if (topR == 0 || botR == 0) {
      continue;
    }

    _vias(botR, topR).push_back(via);
  }
}

odb::dbTechLayerRule* dbCreateNetUtil::getRule(int routingLayer, int width)
{
  odb::dbTechLayerRule*& rule = _rules[routingLayer][width];

  if (rule != nullptr) {
    return rule;
  }

  // Create a non-default-rule for this width
  odb::dbTechNonDefaultRule* nd_rule = nullptr;
  char rule_name[64];

  while (_ruleNameHint >= 0) {
    snprintf(rule_name, 64, "ADS_ND_%d", _ruleNameHint++);
    nd_rule = odb::dbTechNonDefaultRule::create(_tech, rule_name);

    if (nd_rule) {
      break;
    }
  }

  if (nd_rule == nullptr) {
    return nullptr;
  }
  logger_->info(utl::RCX,
                299,
                "Create ND RULE {} for layer/width {},{}",
                rule_name,
                routingLayer,
                width);
  int i;
  for (i = 1; i <= _tech->getRoutingLayerCount(); i++) {
    odb::dbTechLayer* layer = _routingLayers[i];

    if (layer != nullptr) {
      odb::dbTechLayerRule* lr = odb::dbTechLayerRule::create(nd_rule, layer);
      lr->setWidth(width);
      lr->setSpacing(layer->getSpacing());

      odb::dbTechLayerRule*& r = _rules[i][width];
      if (r == nullptr) {
        r = lr;
      }
    }
  }

  // odb::dbTechVia  *curly_via;
  dbSet<odb::dbTechVia> all_vias = _tech->getVias();
  dbSet<odb::dbTechVia>::iterator viter;
  std::string nd_via_name;
  for (viter = all_vias.begin(); viter != all_vias.end(); ++viter) {
    if (((*viter)->getNonDefaultRule() == nullptr) && ((*viter)->isDefault())) {
      nd_via_name = std::string(rule_name) + std::string("_")
                    + std::string((*viter)->getName());
      // curly_via = odb::dbTechVia::clone(nd_rule, (*viter),
      // nd_via_name.c_str());
    }
  }

  assert(rule != nullptr);
  return rule;
}

odb::dbTechVia* dbCreateNetUtil::getVia(int l1, int l2, odb::Rect& bbox)
{
  int bot, top;

  if (l1 < l2) {
    bot = l1;
    top = l2;
  } else {
    bot = l2;
    top = l1;
  }

  uint32_t dx = bbox.dx();
  uint32_t dy = bbox.dy();

  odb::dbTechVia* def = nullptr;
  std::vector<odb::dbTechVia*>& vias = _vias(bot, top);
  std::vector<odb::dbTechVia*>::iterator itr;

  for (itr = vias.begin(); itr != vias.end(); ++itr) {
    odb::dbTechVia* via = *itr;

    if (via->isDefault()) {
      def = via;
    }

    odb::dbBox* bbox = via->getBBox();
    uint32_t vdx = bbox->getDX();
    uint32_t vdy = bbox->getDY();

    if (vdx == dx && vdy == dy) {  // This is a guess!
      return via;                  // There's no way to determine
    }
    // the via from the placed bbox.
    // There could be multiple vias with the same width and
    // height
  }

  return def;
}

odb::dbNet* dbCreateNetUtil::createNetSingleWire(const char* netName,
                                                 int x1,
                                                 int y1,
                                                 int x2,
                                                 int y2,
                                                 int routingLayer,
                                                 odb::dbTechLayerDir dir,
                                                 bool skipBterms)
{
  if (dir == odb::dbTechLayerDir::NONE) {
    return createNetSingleWire(
        netName, x1, y1, x2, y2, routingLayer, dir, skipBterms);
  }

  if ((netName == nullptr) || (routingLayer < 1)
      || (routingLayer > _tech->getRoutingLayerCount())) {
    if (netName == nullptr) {
      logger_->warn(
          RCX, 400, "Cannot create wire, because net name is nullptr\n");
    } else {
      logger_->warn(RCX,
                    401,
                    "Cannot create wire, because routing layer ({}) is invalid",
                    routingLayer);
    }

    return nullptr;
  }

  odb::Rect r(x1, y1, x2, y2);
  int width;
  Point p0, p1;

  if (dir == odb::dbTechLayerDir::VERTICAL) {
    uint32_t dx = r.dx();

    // This is dangerous!
    if (dx & 1) {
      r = odb::Rect(x1, y1, x2 + 1, y2);
      dx = r.dx();
    }

    width = (int) dx;
    int dw = dx / 2;
    p0.setX(r.xMin() + dw);
    p0.setY(r.yMin());
    p1.setX(r.xMax() - dw);
    p1.setY(r.yMax());
  } else {
    uint32_t dy = r.dy();

    // This is dangerous!
    if (dy & 1) {
      r = odb::Rect(x1, y1, x2, y2 + 1);
      dy = r.dy();
    }

    width = (int) dy;
    int dw = dy / 2;
    p0.setY(r.xMin());
    p0.setX(r.yMin() + dw);
    p1.setY(r.xMax());
    p1.setX(r.yMax() - dw);
  }

  odb::dbTechLayer* layer = _routingLayers[routingLayer];
  int minWidth = layer->getWidth();

  if (width < (int) minWidth) {
    std::string ln = layer->getName();
    logger_->warn(RCX,
                  402,
                  "Cannot create net %s, because wire width ({}) is less than "
                  "minWidth ({}) on layer {}",
                  netName,
                  width,
                  minWidth,
                  ln.c_str());
    return nullptr;
  }

  odb::dbNet* net = odb::dbNet::create(_block, netName);

  if (net == nullptr) {
    return nullptr;
  }

  net->setSigType(dbSigType::SIGNAL);

  std::pair<odb::dbBTerm*, odb::dbBTerm*> blutrms;

  if (!skipBterms) {
    blutrms = createTerms4SingleNet(
        net, r.xMin(), r.yMin(), r.xMax(), r.yMax(), layer);

    if ((blutrms.first == nullptr) || (blutrms.second == nullptr)) {
      odb::dbNet::destroy(net);
      logger_->warn(RCX,
                    403,
                    "Cannot create net {}, because failed to create bterms",
                    netName);
      return nullptr;
    }
  }

  odb::dbTechLayerRule* rule = nullptr;
  if ((int) layer->getWidth() != width) {
    rule = getRule(routingLayer, width);
  }

  dbWireEncoder encoder;
  encoder.begin(dbWire::create(net));

  if (rule == nullptr) {
    encoder.newPath(layer, dbWireType::ROUTED);
  } else {
    encoder.newPath(layer, dbWireType::ROUTED, rule);
  }

  encoder.addPoint(p0.x(), p0.y(), 0);

  if (!skipBterms) {
    encoder.addBTerm(blutrms.first);
  }

  encoder.addPoint(p1.x(), p1.y(), 0);

  if (!skipBterms) {
    encoder.addBTerm(blutrms.second);
  }

  encoder.end();

  return net;
}
odb::dbSBox* dbCreateNetUtil::createSpecialWire(odb::dbNet* mainNet,
                                                odb::Rect& r,
                                                odb::dbTechLayer* layer,
                                                uint32_t /* unused: sboxId */)
{
  dbSWire* swire = nullptr;
  if (mainNet == nullptr) {
    swire = _currentNet->getFirstSWire();
  } else {
    swire = mainNet->getFirstSWire();
  }

  return odb::dbSBox::create(swire,
                             layer,
                             r.xMin(),
                             r.yMin(),
                             r.xMax(),
                             r.yMax(),
                             dbWireShapeType::NONE);

  // MIGHT NOT care abour sboxId!!
}

uint32_t dbCreateNetUtil::getFirstShape(odb::dbNet* net, odb::dbShape& s)
{
  dbWirePath path;
  dbWirePathShape pshape;

  dbWirePathItr pitr;
  dbWire* wire = net->getWire();

  uint32_t status = 0;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    pitr.getNextShape(pshape);
    s = pshape.shape;
    status = pshape.junction_id;
    break;
  }
  return status;
}
bool dbCreateNetUtil::setFirstShapeProperty(odb::dbNet* net, uint32_t prop)
{
  if (net == nullptr) {
    return false;
  }

  odb::dbShape s;
  uint32_t jid = getFirstShape(net, s);
  net->getWire()->setProperty(jid, prop);

  return true;
}

odb::dbNet* dbCreateNetUtil::createNetSingleWire(odb::Rect& r,
                                                 uint32_t level,
                                                 uint32_t netId,
                                                 uint32_t shapeId)
{
  // bool skipBterms= false;
  char netName[128];

  if (_currentNet == nullptr) {
    sprintf(netName, "N%d", netId);

    odb::dbNet* newNet = createNetSingleWire(netName,
                                             r.xMin(),
                                             r.yMin(),
                                             r.xMax(),
                                             r.yMax(),
                                             level,
                                             true /*skipBterms*/,
                                             true);

    if (shapeId > 0) {
      setFirstShapeProperty(newNet, shapeId);
    }

    _currentNet = newNet;
    if (_mapArray != nullptr) {
      _mapArray[netId] = _currentNet;
    }
    return newNet;
  }
  sprintf(netName, "N%d_%d", netId, shapeId);
  odb::dbNet* newNet = createNetSingleWire(netName,
                                           r.xMin(),
                                           r.yMin(),
                                           r.xMax(),
                                           r.yMax(),
                                           level,
                                           true /*skipBterms*/,
                                           true);

  if (newNet != nullptr) {
    if (shapeId > 0) {
      setFirstShapeProperty(newNet, shapeId);
    }

    _currentNet->getWire()->append(newNet->getWire(), true);
    odb::dbNet::destroy(newNet);
  }
  return newNet;
}

odb::dbNet* dbCreateNetUtil::createNetSingleWire(const char* netName,
                                                 int x1,
                                                 int y1,
                                                 int x2,
                                                 int y2,
                                                 int routingLayer,
                                                 bool skipBterms,
                                                 bool skipExistsNet,
                                                 uint8_t color)
{
  if ((netName == nullptr) || (routingLayer < 1)
      || (routingLayer > _tech->getRoutingLayerCount())) {
    if (netName == nullptr) {
      logger_->warn(
          RCX, 408, "Cannot create wire, because net name is nullptr");
    } else {
      logger_->warn(RCX,
                    414,
                    "Cannot create wire, because routing layer ({}) is invalid",
                    routingLayer);
    }

    return nullptr;
  }

  odb::dbTechLayer* layer = _routingLayers[routingLayer];
  odb::Rect r(x1, y1, x2, y2);
  uint32_t dx = r.dx();
  uint32_t dy = r.dy();

  // This is dangerous!
  if ((dx & 1) && (dy & 1)) {
    r = odb::Rect(x1, y1, x2, y2 + 1);
    dx = r.dx();
    dy = r.dy();
  }

  uint32_t width;
  Point p0, p1;
  uint32_t minWidth = layer->getWidth();
  bool make_vertical = false;

  if (((dx & 1) == 0) && ((dy & 1) == 1))  // dx == even & dy == odd
  {
    make_vertical = true;
  } else if (((dx & 1) == 1) && ((dy & 1) == 0))  // dx == odd & dy == even
  {
    make_vertical = false;
  } else if ((dx < minWidth) && (dy < minWidth)) {
    if (dx > dy) {
      make_vertical = true;
    } else if (dx < dy) {
      make_vertical = false;
    } else {
      make_vertical = true;
    }
  } else if (dx == minWidth) {
    make_vertical = true;
  } else if (dy == minWidth) {
    make_vertical = false;
  } else if (dx < minWidth) {
    make_vertical = false;
  } else if (dy < minWidth) {
    make_vertical = true;
  } else if (dx < dy) {
    make_vertical = true;
  } else if (dx > dy) {
    make_vertical = false;
  } else  // square (make vertical)
  {
    make_vertical = true;
  }

  if (make_vertical) {
    width = dx;
    int dw = width / 2;
    p0.setX(r.xMin() + dw);
    p0.setY(r.yMin());
    p1.setX(r.xMax() - dw);
    p1.setY(r.yMax());
  } else {
    width = dy;
    int dw = width / 2;
    p0.setX(r.xMin());
    p0.setY(r.yMin() + dw);
    p1.setX(r.xMax());
    p1.setY(r.yMax() - dw);
  }

  odb::dbNet* net = odb::dbNet::create(_block, netName, skipExistsNet);

  if (net == nullptr) {
    logger_->warn(RCX, 409, "Cannot create net {}, duplicate net", netName);
    return nullptr;
  }

  net->setSigType(dbSigType::SIGNAL);

  std::pair<odb::dbBTerm*, odb::dbBTerm*> blutrms;

  if (!skipBterms) {
    blutrms = createTerms4SingleNet(
        net, r.xMin(), r.yMin(), r.xMax(), r.yMax(), layer);

    if ((blutrms.first == nullptr) || (blutrms.second == nullptr)) {
      odb::dbNet::destroy(net);
      logger_->warn(RCX,
                    411,
                    "Cannot create net {}, because failed to create bterms",
                    netName);
      return nullptr;
    }
  }

  odb::dbTechLayerRule* rule = nullptr;
  if (layer->getWidth() != width) {
    rule = getRule(routingLayer, width);
  }

  dbWireEncoder encoder;
  encoder.begin(dbWire::create(net));

  // 0 is not a valid value, and is the same as no color.
  if (color != 0) {
    encoder.setColor(color);
  }

  if (rule == nullptr) {
    encoder.newPath(layer, dbWireType::ROUTED);
  } else {
    encoder.newPath(layer, dbWireType::ROUTED, rule);
  }

  encoder.addPoint(p0.x(), p0.y(), 0);

  if (!skipBterms) {
    encoder.addBTerm(blutrms.first);
  }

  encoder.addPoint(p1.x(), p1.y(), 0);

  if (!skipBterms) {
    encoder.addBTerm(blutrms.second);
  }

  encoder.end();

  return net;
}

std::pair<odb::dbBTerm*, odb::dbBTerm*> dbCreateNetUtil::createTerms4SingleNet(
    odb::dbNet* net,
    int x1,
    int y1,
    int x2,
    int y2,
    odb::dbTechLayer* inly)
{
  std::pair<odb::dbBTerm*, odb::dbBTerm*> retpr;
  retpr.first = nullptr;
  retpr.second = nullptr;

  std::string term_str(net->getName());
  term_str = term_str + "_BL";
  odb::dbBTerm* blterm = odb::dbBTerm::create(net, term_str.c_str());

  uint32_t dx = x2 - x1;
  uint32_t dy = y2 - y1;
  uint32_t fwidth = dx < dy ? dx : dy;
  uint32_t hwidth = fwidth / 2;
  if (!blterm) {
    return retpr;
  }

  term_str = net->getName();
  term_str = term_str + "_BU";
  odb::dbBTerm* buterm = odb::dbBTerm::create(net, term_str.c_str());

  if (!buterm) {
    odb::dbBTerm::destroy(blterm);
    return retpr;
  }

  // TWG: Added bpins
  dbBPin* blpin = dbBPin::create(blterm);
  dbBPin* bupin = dbBPin::create(buterm);

  if (dx == fwidth) {
    int x = x1 + hwidth;
    odb::dbBox::create(
        blpin, inly, -hwidth + x, -hwidth + y1, hwidth + x, hwidth + y1);
    odb::dbBox::create(
        bupin, inly, -hwidth + x, -hwidth + y2, hwidth + x, hwidth + y2);
  } else {
    int y = y1 + hwidth;
    odb::dbBox::create(
        blpin, inly, -hwidth + x1, -hwidth + y, hwidth + x1, hwidth + y);
    odb::dbBox::create(
        bupin, inly, -hwidth + x2, -hwidth + y, hwidth + x2, hwidth + y);
  }

  blterm->setSigType(dbSigType::SIGNAL);
  buterm->setSigType(dbSigType::SIGNAL);
  blterm->setIoType(dbIoType::INPUT);
  buterm->setIoType(dbIoType::OUTPUT);
  blpin->setPlacementStatus(dbPlacementStatus::PLACED);
  bupin->setPlacementStatus(dbPlacementStatus::PLACED);

  retpr.first = blterm;
  retpr.second = buterm;
  return retpr;
}

}  // namespace rcx
