// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definNet.h"

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "odb/defin.h"
#include "utl/Logger.h"

namespace odb {

inline uint32_t get_net_dbid(const char* name)
{
  if (*name != 'N') {
    return 0;
  }

  ++name;

  if (*name == '\0') {
    return 0;
  }

  char* end;
  uint32_t dbid = strtoul(name, &end, 10);

  if (*end != '\0') {
    return 0;
  }

  return dbid;
}

void definNet::begin(const char* name)
{
  assert(_cur_net == nullptr);

  _cur_net = _block->findNet(name);

  if (_cur_net == nullptr) {
    _cur_net = dbNet::create(_block, name);
  } else if (!_skip_wires) {
    dbWire* wire = _cur_net->getWire();
    if (wire) {
      dbWire::destroy(wire);
    }
  }

  _non_default_rule = nullptr;

  if (_mode == defin::FLOORPLAN) {
    _update_cnt++;
  } else {
    _net_cnt++;
  }

  _wire = nullptr;
  _rule_for_path = nullptr;
  if (_net_cnt != 0 && _net_cnt % 100000 == 0) {
    _logger->info(utl::ODB, 97, "\t\tCreated {} Nets", _net_cnt);
  }
}

void definNet::beginMustjoin(const char* iname, const char* tname)
{
  assert(_cur_net == nullptr);

  char buf[BUFSIZ];
  sprintf(buf, "__%d__mustjoin\n", _net_cnt);

  _cur_net = _block->findNet(buf);

  if (_cur_net == nullptr) {
    _logger->warn(utl::ODB, 98, "duplicate must-join net found ({})", buf);
    ++_errors;
  }

  _cur_net = dbNet::create(_block, buf);
  connection(iname, tname);

  _net_cnt++;
  _wire = nullptr;
  _non_default_rule = nullptr;
  _rule_for_path = nullptr;
}

void definNet::connection(const char* iname, const char* tname)
{
  if (_skip_signal_connections == true) {
    return;
  }

  if (_cur_net == nullptr) {
    return;
  }

  if (iname[0] == 'P' || iname[0] == 'p') {
    if (iname[1] == 'I' || iname[1] == 'i') {
      if (iname[2] == 'N' || iname[2] == 'n') {
        if (iname[3] == 0) {
          if (_block->findBTerm(tname) == nullptr) {
            dbBTerm::create(_cur_net, tname);
          }

          return;
        }
      }
    }
  }

  dbInst* inst = _block->findInst(iname);

  if (inst == nullptr) {
    _logger->warn(
        utl::ODB, 99, "error: netlist component ({}) is not defined", iname);
    ++_errors;
    return;
  }

  dbMaster* master = inst->getMaster();
  dbMTerm* mterm = master->findMTerm(_block, tname);

  if (mterm == nullptr) {
    _logger->warn(utl::ODB,
                  100,
                  "error: netlist component-pin ({}, {}) is not defined",
                  iname,
                  tname);
    ++_errors;
    return;
  }

  inst->getITerm(mterm)->connect(_cur_net);
  _net_iterm_cnt++;
}

dbTechNonDefaultRule* definNet::findNonDefaultRule(const char* name)
{
  dbTechNonDefaultRule* rule = _block->findNonDefaultRule(name);

  if (rule) {
    return rule;
  }

  rule = _tech->findNonDefaultRule(name);
  return rule;
}

void definNet::nonDefaultRule(const char* rule)
{
  if (_cur_net == nullptr) {
    return;
  }

  _non_default_rule = findNonDefaultRule(rule);

  if (_non_default_rule == nullptr) {
    _logger->warn(
        utl::ODB, 103, "error: undefined NONDEFAULTRULE ({}) referenced", rule);
    ++_errors;
  } else {
    _cur_net->setNonDefaultRule(_non_default_rule);
  }
}

void definNet::use(dbSigType type)
{
  if (_cur_net == nullptr) {
    return;
  }

  _cur_net->setSigType(type);
}

void definNet::source(dbSourceType source)
{
  if (_cur_net == nullptr) {
    return;
  }

  _cur_net->setSourceType(source);
}

void definNet::weight(int weight)
{
  if (_cur_net == nullptr) {
    return;
  }

  _cur_net->setWeight(weight);
}

void definNet::fixedbump()
{
  if (_cur_net == nullptr) {
    return;
  }

  _cur_net->setFixedBump(true);
}

void definNet::xtalk(int value)
{
  if (_cur_net == nullptr) {
    return;
  }

  _cur_net->setXTalkClass(value);
}

void definNet::wire(dbWireType type)
{
  if (_skip_wires) {
    return;
  }

  if (_wire == nullptr) {
    _wire = dbWire::create(_cur_net);
    _wire_encoder.begin(_wire);
  }

  _wire_type = type;
  _taper_rule = nullptr;
}

void definNet::path(const char* layer_name)
{
  _rule_for_path = _non_default_rule;
  pathBegin(layer_name);
}

void definNet::pathBegin(const char* layer_name)
{
  if (_wire == nullptr) {
    return;
  }

  _cur_layer = _tech->findLayer(layer_name);

  if (_cur_layer == nullptr) {
    _logger->warn(
        utl::ODB, 104, "error: undefined layer ({}) referenced", layer_name);
    ++_errors;
    dbWire::destroy(_wire);
    _wire = nullptr;
    return;
  }

  _taper_rule = nullptr;
  if (_rule_for_path) {
    _taper_rule = _rule_for_path->getLayerRule(_cur_layer);

    if (_taper_rule == nullptr) {
      std::string lyr_name = _cur_layer->getName();
      _logger->warn(utl::ODB,
                    105,
                    "error: RULE ({}) referenced for layer ({})",
                    _rule_for_path->getName().c_str(),
                    lyr_name.c_str());
      ++_errors;
    }
  }

  if (_taper_rule) {
    _wire_encoder.newPath(_cur_layer, _wire_type, _taper_rule);
  } else {
    _wire_encoder.newPath(_cur_layer, _wire_type);
  }
}

void definNet::pathTaper(const char* layer)
{
  _rule_for_path = nullptr;  // turn off non-default-rule for this path
  pathBegin(layer);
}

void definNet::pathTaperRule(const char* layer_name, const char* rule_name)
{
  _rule_for_path = findNonDefaultRule(rule_name);

  if (_rule_for_path == nullptr) {
    _logger->warn(utl::ODB,
                  106,
                  "error: undefined TAPER RULE ({}) referenced",
                  rule_name);
    ++_errors;
    path(layer_name);
    return;
  }

  pathBegin(layer_name);
}

void definNet::pathPoint(int x, int y)
{
  if (_wire == nullptr) {
    return;
  }

  _prev_x = dbdist(x);
  _prev_y = dbdist(y);

  _wire_encoder.addPoint(_prev_x, _prev_y);
}

void definNet::pathPoint(int x, int y, int ext)
{
  if (_wire == nullptr) {
    return;
  }

  _prev_x = dbdist(x);
  _prev_y = dbdist(y);

  _wire_encoder.addPoint(_prev_x, _prev_y, dbdist(ext));
}

void definNet::getUniqueViaName(std::string& viaName)
{
  if ((_tech->findVia(viaName.c_str()) == nullptr)
      && (_block->findVia(viaName.c_str()) == nullptr)) {
    return;
  }

  int cnt = 1;
  for (;; ++cnt) {
    char buffer[16];
    snprintf(buffer, 15, "%d", cnt);
    std::string vn(viaName);
    vn += buffer;

    if ((_tech->findVia(vn.c_str()) == nullptr)
        && (_block->findVia(vn.c_str()) == nullptr)) {
      viaName = vn;
      return;
    }
  }
}

dbVia* definNet::getRotatedVia(const char* via_name, dbOrientType orient)
{
  std::string viaName(via_name);

  switch (orient.getValue()) {
    case dbOrientType::R0:
      viaName += "_N";
      break;

    case dbOrientType::R180:
      viaName += "_S";
      break;

    case dbOrientType::R270:
      viaName += "_E";
      break;

    case dbOrientType::R90:
      viaName += "_W";
      break;

    case dbOrientType::MY:
      viaName += "_FN";
      break;

    case dbOrientType::MX:
      viaName += "_FS";
      break;

    case dbOrientType::MYR90:
      viaName += "_FE";
      break;

    case dbOrientType::MXR90:
      viaName += "_FW";
      break;

    default:
      throw std::runtime_error("Unknown orientation");
      break;
  }

  dbVia*& via = _rotated_vias[viaName];

  if (via != nullptr) {
    return via;
  }

  getUniqueViaName(viaName);

  dbTechVia* tech_via = _tech->findVia(via_name);

  if (tech_via) {
    via = dbVia::create(_block, viaName.c_str(), tech_via, orient);
  }

  else {
    dbVia* block_via = _block->findVia(via_name);

    if (block_via == nullptr) {
      _logger->warn(
          utl::ODB, 107, "error: undefined via ({}) referenced", via_name);
      ++_errors;
      return nullptr;
    }

    via = dbVia::create(_block, viaName.c_str(), block_via, orient);
  }

  return via;
}

void definNet::pathVia(const char* via_name, dbOrientType orient)
{
  if (_wire == nullptr) {
    return;
  }

  dbVia* via = getRotatedVia(via_name, orient);

  if (via == nullptr) {
    return;
  }

  _wire_encoder.addVia(via);
  dbTechLayer* top = via->getTopLayer();
  dbTechLayer* bot = via->getBottomLayer();

  if (top == _cur_layer) {
    _cur_layer = bot;
  } else if (bot == _cur_layer) {
    _cur_layer = top;
  } else {
    ++_errors;
    _logger->warn(
        utl::ODB,
        108,
        "error: invalid VIA layers, cannot determine exit layer of path");
  }
}

void definNet::pathVia(const char* via_name)
{
  if (_wire == nullptr) {
    return;
  }

  dbTechLayer* top;
  dbTechLayer* bot;
  dbTechVia* tech_via = _tech->findVia(via_name);

  if (tech_via != nullptr) {
    _wire_encoder.addTechVia(tech_via);
    top = tech_via->getTopLayer();
    bot = tech_via->getBottomLayer();
  } else {
    dbVia* via = _block->findVia(via_name);

    if (via == nullptr) {
      _logger->warn(
          utl::ODB, 109, "error: undefined via ({}) referenced", via_name);
      ++_errors;
      return;
    }

    _wire_encoder.addVia(via);
    top = via->getTopLayer();
    bot = via->getBottomLayer();
  }

  if (top == _cur_layer) {
    _cur_layer = bot;
  } else if (bot == _cur_layer) {
    _cur_layer = top;
  } else {
    ++_errors;
    _logger->warn(utl::ODB,
                  110,
                  "error: invalid VIA layers in {} in net {}, currently on "
                  "layer {} at ({}, {}), cannot determine exit "
                  "layer of path",
                  via_name,
                  _cur_net->getName(),
                  _cur_layer->getName(),
                  _prev_x,
                  _prev_y);
  }
}

void definNet::pathRect(int deltaX1, int deltaY1, int deltaX2, int deltaY2)
{
  _wire_encoder.addRect(
      dbdist(deltaX1), dbdist(deltaY1), dbdist(deltaX2), dbdist(deltaY2));
}

void definNet::pathColor(int color)
{
  _wire_encoder.setColor(static_cast<uint8_t>(color));
}

void definNet::pathViaColor(int bottom_color, int cut_color, int top_color)
{
  _wire_encoder.setViaColor(static_cast<uint8_t>(bottom_color),
                            static_cast<uint8_t>(cut_color),
                            static_cast<uint8_t>(top_color));
}

void definNet::pathEnd()
{
  _cur_layer = nullptr;
}

void definNet::wireEnd()
{
}

void definNet::property(const char* name, const char* value)
{
  if (_cur_net == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_net, name);
  if (p) {
    dbProperty::destroy(p);
  }

  dbStringProperty::create(_cur_net, name, value);
}

void definNet::property(const char* name, int value)
{
  if (_cur_net == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_net, name);
  if (p) {
    dbProperty::destroy(p);
  }

  dbIntProperty::create(_cur_net, name, value);
}

void definNet::property(const char* name, double value)
{
  if (_cur_net == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_net, name);

  if (p) {
    dbProperty::destroy(p);
  }

  dbDoubleProperty::create(_cur_net, name, value);
}

void definNet::end()
{
  if (_cur_net == nullptr) {
    return;
  }

  dbSet<dbITerm> iterms = _cur_net->getITerms();

  if (iterms.reversible() && iterms.orderReversed()) {
    iterms.reverse();
  }

  dbSet<dbProperty> props = dbProperty::getProperties(_cur_net);

  if (!props.empty() && props.orderReversed()) {
    props.reverse();
  }

  if (_wire) {
    _wire_encoder.end();
  }
  _cur_net = nullptr;
}

}  // namespace odb
