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

#include "rcx/dbUtil.h"

#include "odb/array1.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbWireCodec.h"
#include "utl/Logger.h"

namespace rcx {

using odb::Ath__array1D;
using odb::dbBPin;
using odb::dbIntProperty;
using odb::dbIoType;
using odb::dbMaster;
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
using utl::ODB;

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
void dbCreateNetUtil::setCurrentNet(dbNet* net)
{
  _currentNet = net;
}

dbInst* dbCreateNetUtil::createInst(dbInst* inst0)
{
  char instName[64];
  sprintf(instName, "N%d", inst0->getId());

  dbInst* inst = dbInst::create(_block, inst0->getMaster(), instName);
  if (inst == nullptr) {
    return nullptr;
  }

  inst->setOrient(inst0->getOrient());
  const Point origin = inst0->getOrigin();
  inst->setOrigin(origin.x(), origin.y());
  inst->setPlacementStatus(inst0->getPlacementStatus());

  return inst;
}

void dbCreateNetUtil::setBlock(dbBlock* block, bool skipInit)
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

  dbSet<dbTechLayer> layers = _tech->getLayers();
  dbSet<dbTechLayer>::iterator itr;

  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    dbTechLayer* layer = *itr;
    int rlevel = layer->getRoutingLevel();

    if (rlevel > 0) {
      _routingLayers[rlevel] = layer;
    }
  }

  // Build mapping table to rule widths
  dbSet<dbTechNonDefaultRule> nd_rules = _tech->getNonDefaultRules();
  dbSet<dbTechNonDefaultRule>::iterator nditr;
  // dbTechNonDefaultRule  *wdth_rule = nullptr;

  for (nditr = nd_rules.begin(); nditr != nd_rules.end(); ++nditr) {
    dbTechNonDefaultRule* nd_rule = *nditr;
    std::vector<dbTechLayerRule*> layer_rules;
    nd_rule->getLayerRules(layer_rules);
    std::vector<dbTechLayerRule*>::iterator lritr;

    for (lritr = layer_rules.begin(); lritr != layer_rules.end(); ++lritr) {
      dbTechLayerRule* rule = *lritr;

      int rlevel = rule->getLayer()->getRoutingLevel();

      if (rlevel > 0) {
        dbTechLayerRule*& r = _rules[rlevel][rule->getWidth()];

        if (r == nullptr) {  // Don't overide any existing rule.
          r = rule;
        }
      }
    }
  }

  _vias.clear();
  _vias.resize(layerCount + 1, layerCount + 1);

  dbSet<dbTechVia> vias = _tech->getVias();
  dbSet<dbTechVia>::iterator vitr;

  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    dbTechVia* via = *vitr;
    dbTechLayer* bot = via->getBottomLayer();
    dbTechLayer* top = via->getTopLayer();

    int topR = top->getRoutingLevel();
    int botR = bot->getRoutingLevel();

    if (topR == 0 || botR == 0) {
      continue;
    }

    _vias(botR, topR).push_back(via);
  }
}

dbTechLayerRule* dbCreateNetUtil::getRule(int routingLayer, int width)
{
  dbTechLayerRule*& rule = _rules[routingLayer][width];

  if (rule != nullptr) {
    return rule;
  }

  // Create a non-default-rule for this width
  dbTechNonDefaultRule* nd_rule = nullptr;
  char rule_name[64];

  while (_ruleNameHint >= 0) {
    snprintf(rule_name, 64, "ADS_ND_%d", _ruleNameHint++);
    nd_rule = dbTechNonDefaultRule::create(_tech, rule_name);

    if (nd_rule) {
      break;
    }
  }

  if (nd_rule == nullptr) {
    return nullptr;
  }
  logger_->info(utl::ODB,
                273,
                "Create ND RULE {} for layer/width {},{}",
                rule_name,
                routingLayer,
                width);

  int i;
  for (i = 1; i <= _tech->getRoutingLayerCount(); i++) {
    dbTechLayer* layer = _routingLayers[i];

    if (layer != nullptr) {
      dbTechLayerRule* lr = dbTechLayerRule::create(nd_rule, layer);
      lr->setWidth(width);
      lr->setSpacing(layer->getSpacing());

      dbTechLayerRule*& r = _rules[i][width];
      if (r == nullptr) {
        r = lr;
      }
    }
  }

  // dbTechVia  *curly_via;
  dbSet<dbTechVia> all_vias = _tech->getVias();
  dbSet<dbTechVia>::iterator viter;
  std::string nd_via_name;
  for (viter = all_vias.begin(); viter != all_vias.end(); ++viter) {
    if (((*viter)->getNonDefaultRule() == nullptr) && ((*viter)->isDefault())) {
      nd_via_name = std::string(rule_name) + std::string("_")
                    + std::string((*viter)->getName());
      // curly_via = dbTechVia::clone(nd_rule, (*viter), nd_via_name.c_str());
    }
  }

  assert(rule != nullptr);
  return rule;
}

dbTechVia* dbCreateNetUtil::getVia(int l1, int l2, Rect& bbox)
{
  int bot, top;

  if (l1 < l2) {
    bot = l1;
    top = l2;
  } else {
    bot = l2;
    top = l1;
  }

  uint dx = bbox.dx();
  uint dy = bbox.dy();

  dbTechVia* def = nullptr;
  std::vector<dbTechVia*>& vias = _vias(bot, top);
  std::vector<dbTechVia*>::iterator itr;

  for (itr = vias.begin(); itr != vias.end(); ++itr) {
    dbTechVia* via = *itr;

    if (via->isDefault()) {
      def = via;
    }

    dbBox* bbox = via->getBBox();
    uint vdx = bbox->getDX();
    uint vdy = bbox->getDY();

    if (vdx == dx && vdy == dy) {  // This is a guess!
      return via;                  // There's no way to determine
    }
    // the via from the placed bbox.
    // There could be multiple vias with the same width and
    // height
  }

  return def;
}

dbNet* dbCreateNetUtil::createNetSingleWire(const char* netName,
                                            int x1,
                                            int y1,
                                            int x2,
                                            int y2,
                                            int routingLayer,
                                            dbTechLayerDir dir,
                                            bool skipBterms)
{
  if (dir == dbTechLayerDir::NONE) {
    return createNetSingleWire(
        netName, x1, y1, x2, y2, routingLayer, dir, skipBterms);
  }

  if ((netName == nullptr) || (routingLayer < 1)
      || (routingLayer > _tech->getRoutingLayerCount())) {
    if (netName == nullptr) {
      logger_->warn(
          ODB, 400, "Cannot create wire, because net name is nullptr\n");
    } else {
      logger_->warn(ODB,
                    401,
                    "Cannot create wire, because routing layer ({}) is invalid",
                    routingLayer);
    }

    return nullptr;
  }

  Rect r(x1, y1, x2, y2);
  int width;
  Point p0, p1;

  if (dir == dbTechLayerDir::VERTICAL) {
    uint dx = r.dx();

    // This is dangerous!
    if (dx & 1) {
      r = Rect(x1, y1, x2 + 1, y2);
      dx = r.dx();
    }

    width = (int) dx;
    int dw = dx / 2;
    p0.setX(r.xMin() + dw);
    p0.setY(r.yMin());
    p1.setX(r.xMax() - dw);
    p1.setY(r.yMax());
  } else {
    uint dy = r.dy();

    // This is dangerous!
    if (dy & 1) {
      r = Rect(x1, y1, x2, y2 + 1);
      dy = r.dy();
    }

    width = (int) dy;
    int dw = dy / 2;
    p0.setY(r.xMin());
    p0.setX(r.yMin() + dw);
    p1.setY(r.xMax());
    p1.setX(r.yMax() - dw);
  }

  dbTechLayer* layer = _routingLayers[routingLayer];
  int minWidth = layer->getWidth();

  if (width < (int) minWidth) {
    std::string ln = layer->getName();
    logger_->warn(ODB,
                  402,
                  "Cannot create net %s, because wire width ({}) is less than "
                  "minWidth ({}) on layer {}",
                  netName,
                  width,
                  minWidth,
                  ln.c_str());
    return nullptr;
  }

  dbNet* net = dbNet::create(_block, netName);

  if (net == nullptr) {
    return nullptr;
  }

  net->setSigType(dbSigType::SIGNAL);

  std::pair<dbBTerm*, dbBTerm*> blutrms;

  if (!skipBterms) {
    blutrms = createTerms4SingleNet(
        net, r.xMin(), r.yMin(), r.xMax(), r.yMax(), layer);

    if ((blutrms.first == nullptr) || (blutrms.second == nullptr)) {
      dbNet::destroy(net);
      logger_->warn(ODB,
                    403,
                    "Cannot create net {}, because failed to create bterms",
                    netName);
      return nullptr;
    }
  }

  dbTechLayerRule* rule = nullptr;
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
dbSBox* dbCreateNetUtil::createSpecialWire(dbNet* mainNet,
                                           Rect& r,
                                           dbTechLayer* layer,
                                           uint /* unused: sboxId */)
{
  dbSWire* swire = nullptr;
  if (mainNet == nullptr) {
    swire = _currentNet->getFirstSWire();
  } else {
    swire = mainNet->getFirstSWire();
  }

  return dbSBox::create(swire,
                        layer,
                        r.xMin(),
                        r.yMin(),
                        r.xMax(),
                        r.yMax(),
                        dbWireShapeType::NONE);

  // MIGHT NOT care abour sboxId!!
}

uint dbCreateNetUtil::getFirstShape(dbNet* net, dbShape& s)
{
  dbWirePath path;
  dbWirePathShape pshape;

  dbWirePathItr pitr;
  dbWire* wire = net->getWire();

  uint status = 0;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    pitr.getNextShape(pshape);
    s = pshape.shape;
    status = pshape.junction_id;
    break;
  }
  return status;
}
bool dbCreateNetUtil::setFirstShapeProperty(dbNet* net, uint prop)
{
  if (net == nullptr) {
    return false;
  }

  dbShape s;
  uint jid = getFirstShape(net, s);
  net->getWire()->setProperty(jid, prop);

  return true;
}

dbNet* dbCreateNetUtil::createNetSingleWire(Rect& r,
                                            uint level,
                                            uint netId,
                                            uint shapeId)
{
  // bool skipBterms= false;
  char netName[128];

  if (_currentNet == nullptr) {
    sprintf(netName, "N%d", netId);

    dbShape s;
    dbNet* newNet = createNetSingleWire(netName,
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
  dbNet* newNet = createNetSingleWire(netName,
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
    dbNet::destroy(newNet);
  }
  return newNet;
}

dbNet* dbCreateNetUtil::createNetSingleWire(const char* netName,
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
          ODB, 404, "Cannot create wire, because net name is nullptr");
    } else {
      logger_->warn(ODB,
                    405,
                    "Cannot create wire, because routing layer ({}) is invalid",
                    routingLayer);
    }

    return nullptr;
  }

  dbTechLayer* layer = _routingLayers[routingLayer];
  Rect r(x1, y1, x2, y2);
  uint dx = r.dx();
  uint dy = r.dy();

  // This is dangerous!
  if ((dx & 1) && (dy & 1)) {
    r = Rect(x1, y1, x2, y2 + 1);
    dx = r.dx();
    dy = r.dy();
  }

  uint width;
  Point p0, p1;
  uint minWidth = layer->getWidth();
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

  dbNet* net = dbNet::create(_block, netName, skipExistsNet);

  if (net == nullptr) {
    logger_->warn(ODB, 406, "Cannot create net {}, duplicate net", netName);
    return nullptr;
  }

  net->setSigType(dbSigType::SIGNAL);

  std::pair<dbBTerm*, dbBTerm*> blutrms;

  if (!skipBterms) {
    blutrms = createTerms4SingleNet(
        net, r.xMin(), r.yMin(), r.xMax(), r.yMax(), layer);

    if ((blutrms.first == nullptr) || (blutrms.second == nullptr)) {
      dbNet::destroy(net);
      logger_->warn(ODB,
                    407,
                    "Cannot create net {}, because failed to create bterms",
                    netName);
      return nullptr;
    }
  }

  dbTechLayerRule* rule = nullptr;
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

std::pair<dbBTerm*, dbBTerm*> dbCreateNetUtil::createTerms4SingleNet(
    dbNet* net,
    int x1,
    int y1,
    int x2,
    int y2,
    dbTechLayer* inly)
{
  std::pair<dbBTerm*, dbBTerm*> retpr;
  retpr.first = nullptr;
  retpr.second = nullptr;

  std::string term_str(net->getName());
  term_str = term_str + "_BL";
  dbBTerm* blterm = dbBTerm::create(net, term_str.c_str());

  uint dx = x2 - x1;
  uint dy = y2 - y1;
  uint fwidth = dx < dy ? dx : dy;
  uint hwidth = fwidth / 2;
  if (!blterm) {
    return retpr;
  }

  term_str = net->getName();
  term_str = term_str + "_BU";
  dbBTerm* buterm = dbBTerm::create(net, term_str.c_str());

  if (!buterm) {
    dbBTerm::destroy(blterm);
    return retpr;
  }

  // TWG: Added bpins
  dbBPin* blpin = dbBPin::create(blterm);
  dbBPin* bupin = dbBPin::create(buterm);

  if (dx == fwidth) {
    int x = x1 + hwidth;
    dbBox::create(
        blpin, inly, -hwidth + x, -hwidth + y1, hwidth + x, hwidth + y1);
    dbBox::create(
        bupin, inly, -hwidth + x, -hwidth + y2, hwidth + x, hwidth + y2);
  } else {
    int y = y1 + hwidth;
    dbBox::create(
        blpin, inly, -hwidth + x1, -hwidth + y, hwidth + x1, hwidth + y);
    dbBox::create(
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
