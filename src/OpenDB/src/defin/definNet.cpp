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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "db.h"
#include "dbShape.h"
#include "dbWireCodec.h"
#include "definNet.h"

#include "utility/Logger.h"
namespace odb {

inline uint get_net_dbid(const char* name)
{
  if (*name != 'N')
    return 0;

  ++name;

  if (*name == '\0')
    return 0;

  char* end;
  uint  dbid = strtoul(name, &end, 10);

  if (*end != '\0')
    return 0;

  return dbid;
}

definNet::definNet()
{
  init();
  _skip_signal_connections = false;
  _skip_wires              = false;
  _replace_wires           = false;
  _names_are_ids           = false;
  _assembly_mode           = false;
}

definNet::~definNet()
{
}

void definNet::init()
{
  definBase::init();
  _found_new_routing = false;
  _net_cnt           = 0;
  _update_cnt        = 0;
  _net_iterm_cnt     = 0;
  _cur_net           = NULL;
  _cur_layer         = NULL;
  _wire              = NULL;
  _wire_type         = dbWireType::NONE;
  _wire_shape_type   = dbWireShapeType::NONE;
  _prev_x            = 0;
  _prev_y            = 0;
  _width             = 0;
  _point_cnt         = 0;
  _taper_rule        = NULL;
  _non_default_rule  = NULL;
  _rule_for_path     = NULL;
  _rotated_vias.clear();
}

void definNet::begin(const char* name)
{
  assert(_cur_net == NULL);

  if (_replace_wires == false) {
    _cur_net = _block->findNet(name);

    if (_cur_net == NULL)
      _cur_net = dbNet::create(_block, name);

    _non_default_rule = NULL;
  } else {
    if (_names_are_ids == false)
      _cur_net = _block->findNet(name);
    else {
      uint netid = get_net_dbid(name);

      if (netid)
        _cur_net = dbNet::getNet(_block, netid);
    }

    if (_cur_net == NULL) {
      _logger->warn(utl::ODB, 96,  "net {} does not exist", name);
      ++_errors;
    } else {
      if (!_assembly_mode) {
        dbWire* wire = _cur_net->getWire();

        if (wire)
          dbWire::destroy(wire);
      }
    }

    // As per Glenn, in replace mode, use the current non-default-rule.
    // WRoute does not write the rule in the DEF.
    // This may cause problems with other routers.
    _non_default_rule = _cur_net->getNonDefaultRule();
  }
  if(_mode == FLOORPLAN)
    _update_cnt++;
  else
    _net_cnt++;

  _wire              = NULL;
  _rule_for_path     = NULL;
  _found_new_routing = false;
  if (_net_cnt != 0 && _net_cnt % 100000 == 0)
    _logger->info(utl::ODB, 97,  "\t\tCreated {} Nets", _net_cnt);
}

void definNet::beginMustjoin(const char* iname, const char* tname)
{
  assert(_cur_net == NULL);

  if (_replace_wires == false) {
    char buf[BUFSIZ];
    sprintf(buf, "__%d__mustjoin\n", _net_cnt);

    _cur_net = _block->findNet(buf);

    if (_cur_net == NULL) {
      _logger->warn(utl::ODB, 98,  "duplicate mustjoin net found ({})", buf);
      ++_errors;
    }

    _cur_net = dbNet::create(_block, buf);
    connection(iname, tname);
  }

  _net_cnt++;
  _wire             = NULL;
  _non_default_rule = NULL;
  _rule_for_path    = NULL;
}

void definNet::connection(const char* iname, const char* tname)
{
  if (_skip_signal_connections == true)
    return;

  if ((_cur_net == NULL) || (_replace_wires == true))
    return;

  if (iname[0] == 'P' || iname[0] == 'p') {
    if (iname[1] == 'I' || iname[1] == 'i') {
      if (iname[2] == 'N' || iname[2] == 'n') {
        if (iname[3] == 0) {
          if (_block->findBTerm(tname) == NULL) {
            dbBTerm::create(_cur_net, tname);
          }

          return;
        }
      }
    }
  }

  dbInst* inst = _block->findInst(iname);

  if (inst == NULL) {
    _logger->warn(utl::ODB, 99,  "error: netlist component ({}) is not defined", iname);
    ++_errors;
    return;
  }

  dbMaster* master = inst->getMaster();
  dbMTerm*  mterm  = master->findMTerm(_block, tname);

  if (mterm == NULL) {
    _logger->warn(utl::ODB, 100, 
"error: netlist component-pin ({}, {}) is not defined",iname,tname);
    ++_errors;
    return;
  }

  dbITerm::connect(inst, _cur_net, mterm);
  _net_iterm_cnt++;
}

dbTechNonDefaultRule* definNet::findNonDefaultRule(const char* name)
{
  dbTechNonDefaultRule* rule = _block->findNonDefaultRule(name);

  if (rule)
    return rule;

  rule = _tech->findNonDefaultRule(name);
  return rule;
}

void definNet::nonDefaultRule(const char* rule)
{
  if (_cur_net == NULL)
    return;

  if (_replace_wires == true) {
    // As per Glenn, in "replace" mode, ignore the
    // non default rule, because wroute does not write it (even if it exists).
    // Issue an error, if the rules do not match.
    dbTechNonDefaultRule* net_rule = _cur_net->getNonDefaultRule();
    dbTechNonDefaultRule* def_rule = findNonDefaultRule(rule);

    if (def_rule == NULL) {
      _logger->warn(utl::ODB, 101,  "error: undefined NONDEFAULTRULE ({}) referenced", rule);
      ++_errors;
    }

    else if (net_rule != def_rule) {
      std::string net_name      = _cur_net->getName();
      const char* net_rule_name = "(NULL)";
      std::string n;

      if (net_rule != NULL) {
        n             = net_rule->getName();
        net_rule_name = n.c_str();
      }

      _logger->warn(utl::ODB, 102, 
"error: NONDEFAULTRULE ({}) of net ({}) does not match DEF rule ""({}).",net_name.c_str(),net_rule_name,rule);
      ++_errors;
    }
  } else {
    _non_default_rule = findNonDefaultRule(rule);

    if (_non_default_rule == NULL) {
      _logger->warn(utl::ODB, 103,  "error: undefined NONDEFAULTRULE ({}) referenced", rule);
      ++_errors;
    } else
      _cur_net->setNonDefaultRule(_non_default_rule);
  }
}

void definNet::use(dbSigType type)
{
  if ((_cur_net == NULL) || (_replace_wires == true))
    return;

  _cur_net->setSigType(type);
}

void definNet::source(dbSourceType source)
{
  if ((_cur_net == NULL) || (_replace_wires == true))
    return;

  _cur_net->setSourceType(source);
}

void definNet::weight(int weight)
{
  if ((_cur_net == NULL) || (_replace_wires == true))
    return;

  _cur_net->setWeight(weight);
}

void definNet::fixedbump()
{
  if ((_cur_net == NULL) || (_replace_wires == true))
    return;

  _cur_net->setFixedBump(true);
}

void definNet::xtalk(int value)
{
  if ((_cur_net == NULL) || (_replace_wires == true))
    return;

  _cur_net->setXTalkClass(value);
}

void definNet::wire(dbWireType type)
{
  if (_skip_wires)
    return;

  if (_wire == NULL) {
    if (!_assembly_mode)
      _wire = dbWire::create(_cur_net);
    else {
      _wire = _cur_net->getWire();

      if (_wire == NULL)
        _wire = dbWire::create(_cur_net);
    }
    _wire_encoder.begin(_wire);
  }

  _wire_type  = type;
  _taper_rule = NULL;
}

void definNet::path(const char* layer_name)
{
  _rule_for_path = _non_default_rule;
  pathBegin(layer_name);
}

void definNet::pathBegin(const char* layer_name)
{
  if (_wire == NULL)
    return;

  _cur_layer = _tech->findLayer(layer_name);

  if (_cur_layer == NULL) {
    _logger->warn(utl::ODB, 104,  "error: undefined layer ({}) referenced", layer_name);
    ++_errors;
    dbWire::destroy(_wire);
    _wire = NULL;
    return;
  }

  _taper_rule = NULL;
  if (_rule_for_path) {
    _taper_rule = _rule_for_path->getLayerRule(_cur_layer);

    if (_taper_rule == NULL) {
      std::string lyr_name  = _cur_layer->getName();
      std::string rule_name = _non_default_rule->getName();
      _logger->warn(utl::ODB, 105, 
"error: RULE ({}) referenced for layer ({})",_rule_for_path->getName().c_str(),lyr_name.c_str());
      ++_errors;
    }
  }

  if (_taper_rule)
    _wire_encoder.newPath(_cur_layer, _wire_type, _taper_rule);
  else
    _wire_encoder.newPath(_cur_layer, _wire_type);
}

void definNet::pathTaper(const char* layer)
{
  _rule_for_path = NULL;  // turn off non-default-rule for this path
  pathBegin(layer);
}

void definNet::pathTaperRule(const char* layer_name, const char* rule_name)
{
  _rule_for_path = findNonDefaultRule(rule_name);

  if (_rule_for_path == NULL) {
    _logger->warn(utl::ODB, 106,  "error: undefined TAPER RULE ({}) referenced", rule_name);
    ++_errors;
    path(layer_name);
    return;
  }

  pathBegin(layer_name);
}

void definNet::pathPoint(int x, int y)
{
  if (_wire == NULL)
    return;

  _prev_x = dbdist(x);
  _prev_y = dbdist(y);

  _wire_encoder.addPoint(_prev_x, _prev_y);
}

void definNet::pathPoint(int x, int y, int ext)
{
  if (_wire == NULL)
    return;

  _prev_x = dbdist(x);
  _prev_y = dbdist(y);

  _wire_encoder.addPoint(_prev_x, _prev_y, dbdist(ext));
}

void definNet::getUniqueViaName(std::string& viaName)
{
  if ((_tech->findVia(viaName.c_str()) == NULL)
      && (_block->findVia(viaName.c_str()) == NULL))
    return;

  int cnt = 1;
  for (;; ++cnt) {
    char buffer[16];
    snprintf(buffer, 15, "%d", cnt);
    std::string vn(viaName);
    vn += buffer;

    if ((_tech->findVia(vn.c_str()) == NULL)
        && (_block->findVia(vn.c_str()) == NULL)) {
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
      throw ZException("Unknown orientation");
      break;
  }

  dbVia*& via = _rotated_vias[viaName];

  if (via != NULL)
    return via;

  getUniqueViaName(viaName);

  dbTechVia* tech_via = _tech->findVia(via_name);

  if (tech_via) {
    via = dbVia::create(_block, viaName.c_str(), tech_via, orient);
  }

  else {
    dbVia* block_via = _block->findVia(via_name);

    if (block_via == NULL) {
      _logger->warn(utl::ODB, 107,  "error: undefined via ({}) referenced", via_name);
      ++_errors;
      return NULL;
    }

    via = dbVia::create(_block, viaName.c_str(), block_via, orient);
  }

  return via;
}

void definNet::pathVia(const char* via_name, dbOrientType orient)
{
  if (_wire == NULL)
    return;

  dbVia* via = getRotatedVia(via_name, orient);

  if (via == NULL)
    return;

  _wire_encoder.addVia(via);
  dbTechLayer* top = via->getTopLayer();
  dbTechLayer* bot = via->getBottomLayer();

  if (top == _cur_layer)
    _cur_layer = bot;
  else if (bot == _cur_layer)
    _cur_layer = top;
  else {
    ++_errors;
    _logger->warn(utl::ODB, 108, 
"error: invalid VIA layers, cannot determine exit layer of path");
  }
}

void definNet::pathVia(const char* via_name)
{
  if (_wire == NULL)
    return;

  dbTechLayer* top;
  dbTechLayer* bot;
  dbTechVia*   tech_via = _tech->findVia(via_name);

  if (tech_via != NULL) {
    _wire_encoder.addTechVia(tech_via);
    top = tech_via->getTopLayer();
    bot = tech_via->getBottomLayer();
  } else {
    dbVia* via = _block->findVia(via_name);

    if (via == NULL) {
      _logger->warn(utl::ODB, 109,  "error: undefined via ({}) referenced", via_name);
      ++_errors;
      return;
    }

    _wire_encoder.addVia(via);
    top = via->getTopLayer();
    bot = via->getBottomLayer();
  }

  if (top == _cur_layer)
    _cur_layer = bot;
  else if (bot == _cur_layer)
    _cur_layer = top;
  else {
    ++_errors;
    _logger->warn(utl::ODB, 110, 
"error: invalid VIA layers, cannot determine exit layer of path");
  }
}

void definNet::pathRect(int deltaX1, int deltaY1, int deltaX2, int deltaY2)
{
  _wire_encoder.addRect(
      dbdist(deltaX1), dbdist(deltaY1), dbdist(deltaX2), dbdist(deltaY2));
}

void definNet::pathEnd()
{
  _cur_layer = NULL;
}

void definNet::wireEnd()
{
}

void definNet::property(const char* name, const char* value)
{
  if ((_cur_net == NULL) || _replace_wires)
    return;

  dbProperty* p = dbProperty::find(_cur_net, name);
  if (p)
    dbProperty::destroy(p);

  dbStringProperty::create(_cur_net, name, value);
}

void definNet::property(const char* name, int value)
{
  if ((_cur_net == NULL) || _replace_wires)
    return;

  dbProperty* p = dbProperty::find(_cur_net, name);
  if (p)
    dbProperty::destroy(p);

  dbIntProperty::create(_cur_net, name, value);
}

void definNet::property(const char* name, double value)
{
  if ((_cur_net == NULL) || _replace_wires)
    return;

  dbProperty* p = dbProperty::find(_cur_net, name);

  if (p)
    dbProperty::destroy(p);

  dbDoubleProperty::create(_cur_net, name, value);
}

void definNet::end()
{
  if (_cur_net == NULL)
    return;

  if (!_replace_wires) {
    dbSet<dbITerm> iterms = _cur_net->getITerms();

    if (iterms.reversible() && iterms.orderReversed())
      iterms.reverse();

    dbSet<dbProperty> props = dbProperty::getProperties(_cur_net);

    if (!props.empty() && props.orderReversed())
      props.reverse();
  }

  if (_wire) {
    if (_assembly_mode && !_found_new_routing) {
      //          notice(0,"CANCEL wiring for net %s ID: %d\n",
      //          _cur_net->getName().c_str(), _cur_net->getId());
      _wire_encoder.clear();
    } else {
      //          notice(0,"Committing wiring for net %s ID: %d\n",
      //          _cur_net->getName().c_str(), _cur_net->getId());
      _wire_encoder.end();

      if (_replace_wires)
        _cur_net->setWireAltered(true);
    }
  } else {
    //    notice(0,"NO WIRE for net %s ID: %d\n", _cur_net->getName().c_str(),
    //    _cur_net->getId());
  }

  _cur_net = NULL;
}

}  // namespace odb
