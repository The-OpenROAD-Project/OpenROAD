// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definSNet.h"

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>

#include "create_box.h"
#include "definPolygon.h"
#include "definTypes.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

void definSNet::begin(const char* name)
{
  assert(_cur_net == nullptr);

  _cur_net = _block->findNet(name);

  if (_cur_net == nullptr) {
    _cur_net = dbNet::create(_block, name);
  }

  _cur_net->setSpecial();

  _snet_cnt++;
  _swire = nullptr;
}

void definSNet::connection(const char* iname,
                           const char* tname,
                           bool /* unused: synth */)
{
  if (_cur_net == nullptr) {
    return;
  }

  if (*iname == '*') {
    connect_all(_cur_net, tname);
    return;
  }

  if (iname[0] == 'P' || iname[0] == 'p') {
    if (iname[1] == 'I' || iname[1] == 'i') {
      if (iname[2] == 'N' || iname[2] == 'n') {
        if (iname[3] == 0) {
          dbBTerm* bterm = _block->findBTerm(tname);
          if (bterm == nullptr) {
            bterm = dbBTerm::create(_cur_net, tname);
          }
          bterm->setSpecial();

          return;
        }
      }
    }
  }

  dbInst* inst = _block->findInst(iname);

  if (inst == nullptr) {
    _logger->warn(
        utl::ODB, 157, "error: netlist component ({}) is not defined", iname);
    ++_errors;
    return;
  }

  dbMaster* master = inst->getMaster();
  dbMTerm* mterm = master->findMTerm(_block, tname);

  if (mterm == nullptr) {
    _logger->warn(utl::ODB,
                  158,
                  "error: netlist component-pin ({}, {}) is not defined",
                  iname,
                  tname);
    ++_errors;
    return;
  }

  dbITerm* iterm = inst->getITerm(mterm);
  iterm->connect(_cur_net);
  _snet_iterm_cnt++;
  iterm->setSpecial();
}

void definSNet::use(dbSigType type)
{
  if (_cur_net == nullptr) {
    return;
  }

  _cur_net->setSigType(type);
}

void definSNet::source(dbSourceType source)
{
  if (_cur_net == nullptr) {
    return;
  }

  _cur_net->setSourceType(source);
}

void definSNet::weight(int weight)
{
  if (_cur_net == nullptr) {
    return;
  }

  _cur_net->setWeight(weight);
}

void definSNet::fixedbump()
{
  if (_cur_net == nullptr) {
    return;
  }

  _cur_net->setFixedBump(true);
}

void definSNet::rect(const char* layer_name,
                     int x1,
                     int y1,
                     int x2,
                     int y2,
                     const char* type,
                     uint32_t mask)
{
  if (_swire == nullptr) {
    return;
  }

  dbTechLayer* layer = _tech->findLayer(layer_name);

  if (layer == nullptr) {
    _logger->warn(
        utl::ODB, 159, "error: undefined layer ({}) referenced", layer_name);
    return;
  }

  dbSBox* box = dbSBox::create(_swire,
                               layer,
                               dbdist(x1),
                               dbdist(y1),
                               dbdist(x2),
                               dbdist(y2),
                               dbWireShapeType(type));
  box->setLayerMask(mask);
}

void definSNet::polygon(const char* layer_name, std::vector<defPoint>& points)
{
  dbTechLayer* layer = _tech->findLayer(layer_name);

  if (layer == nullptr) {
    _logger->warn(
        utl::ODB, 160, "error: undefined layer ({}) referenced", layer_name);
    return;
  }

  std::vector<Point> P;
  translate(points, P);
  definPolygon polygon(P);
  std::vector<Rect> R;
  polygon.decompose(R);

  std::vector<Rect>::iterator itr;

  for (itr = R.begin(); itr != R.end(); ++itr) {
    Rect& r = *itr;
    dbSBox::create(_swire,
                   layer,
                   r.xMin(),
                   r.yMin(),
                   r.xMax(),
                   r.yMax(),
                   dbWireShapeType::NONE);
  }
}

void definSNet::wire(dbWireType type, const char* shield)
{
  if (_skip_special_wires) {
    return;
  }

  _wire_type = type;
  if (type == dbWireType::SHIELD) {
    _shield_net = _block->findNet(shield);

    if (_shield_net == nullptr) {
      _logger->warn(
          utl::ODB, 161, "error: SHIELD net ({}) does not exists.", shield);
      _wire_type = dbWireType::NONE;
      ++_errors;
    }
  }

  _swire = dbSWire::create(_cur_net, _wire_type, _shield_net);
}

void definSNet::path(const char* layer_name, int width)
{
  if (_skip_shields && (_wire_type == dbWireType::SHIELD)) {
    return;
  }

  _cur_layer = _tech->findLayer(layer_name);

  if (_cur_layer == nullptr) {
    _logger->warn(
        utl::ODB, 162, "error: undefined layer ({}) referenced", layer_name);
    ++_errors;
    dbSWire::destroy(_swire);
    _swire = nullptr;
    return;
  }

  if (_swire) {
    _prev_x = 0;
    _prev_y = 0;
    _prev_ext = 0;
    _has_prev_ext = false;
    _point_cnt = 0;
    _width = dbdist(width);
    _wire_shape_type = dbWireShapeType::NONE;
  }
}

void definSNet::pathShape(const char* shape)
{
  if (_skip_shields && (_wire_type == dbWireType::SHIELD)) {
    return;
  }

  _wire_shape_type = dbWireShapeType(shape);
}

void definSNet::pathPoint(int x, int y, int ext, uint32_t mask)
{
  if ((_skip_shields && (_wire_type == dbWireType::SHIELD))
      || (_skip_block_wires && (_wire_shape_type == dbWireShapeType::BLOCKWIRE))
      || (_skip_fill_wires
          && (_wire_shape_type == dbWireShapeType::FILLWIRE))) {
    return;
  }

  int cur_x = dbdist(x);
  int cur_y = dbdist(y);
  int cur_ext = dbdist(ext);

  if (_swire) {
    _point_cnt++;

    if (_point_cnt > 1) {
      create_box(_swire,
                 _wire_shape_type,
                 _cur_layer,
                 _prev_x,
                 _prev_y,
                 _prev_ext,
                 _has_prev_ext,
                 cur_x,
                 cur_y,
                 ext,
                 true,
                 _width,
                 mask,
                 _logger);
    }

    _prev_x = cur_x;
    _prev_y = cur_y;
    _prev_ext = cur_ext;
    _has_prev_ext = true;
  }
}

void definSNet::pathPoint(int x, int y, uint32_t mask)
{
  if ((_skip_shields && (_wire_type == dbWireType::SHIELD))
      || (_skip_block_wires && (_wire_shape_type == dbWireShapeType::BLOCKWIRE))
      || (_skip_fill_wires
          && (_wire_shape_type == dbWireShapeType::FILLWIRE))) {
    return;
  }

  int cur_x = dbdist(x);
  int cur_y = dbdist(y);

  if (_swire) {
    _point_cnt++;

    if (_point_cnt > 1) {
      create_box(_swire,
                 _wire_shape_type,
                 _cur_layer,
                 _prev_x,
                 _prev_y,
                 _prev_ext,
                 _has_prev_ext,
                 cur_x,
                 cur_y,
                 0,
                 false,
                 _width,
                 mask,
                 _logger);
    }

    _prev_x = cur_x;
    _prev_y = cur_y;
    _prev_ext = 0;
    _has_prev_ext = false;
  }
}

void definSNet::pathVia(const char* via_name,
                        uint32_t bottom_mask,
                        uint32_t cut_mask,
                        uint32_t top_mask)
{
  if ((_skip_shields && (_wire_type == dbWireType::SHIELD))
      || (_skip_block_wires && (_wire_shape_type == dbWireShapeType::BLOCKWIRE))
      || (_skip_fill_wires
          && (_wire_shape_type == dbWireShapeType::FILLWIRE))) {
    return;
  }

  dbTechVia* tech_via = _tech->findVia(via_name);

  if (tech_via != nullptr) {
    if (_swire) {
      _cur_layer = create_via_array(_swire,
                                    _wire_shape_type,
                                    _cur_layer,
                                    tech_via,
                                    _prev_x,
                                    _prev_y,
                                    1,
                                    1,
                                    0,
                                    0,
                                    bottom_mask,
                                    cut_mask,
                                    top_mask,
                                    _logger);
      if (_cur_layer == nullptr) {
        _errors++;
      }
    }
  } else {
    dbVia* via = _block->findVia(via_name);

    if (via == nullptr) {
      _logger->warn(
          utl::ODB, 163, "error: undefined via ({}) referenced", via_name);
      ++_errors;
      return;
    }

    if (_swire) {
      _cur_layer = create_via_array(_swire,
                                    _wire_shape_type,
                                    _cur_layer,
                                    via,
                                    _prev_x,
                                    _prev_y,
                                    1,
                                    1,
                                    0,
                                    0,
                                    bottom_mask,
                                    cut_mask,
                                    top_mask,
                                    _logger);
      if (_cur_layer == nullptr) {
        _errors++;
      }
    }
  }
}

void definSNet::pathViaArray(const char* via_name,
                             int numX,
                             int numY,
                             int stepX,
                             int stepY)
{
  if ((_skip_shields && (_wire_type == dbWireType::SHIELD))
      || (_skip_block_wires && (_wire_shape_type == dbWireShapeType::BLOCKWIRE))
      || (_skip_fill_wires
          && (_wire_shape_type == dbWireShapeType::FILLWIRE))) {
    return;
  }

  dbTechVia* tech_via = _tech->findVia(via_name);

  if (tech_via != nullptr) {
    stepX = dbdist(stepX);
    stepY = dbdist(stepY);

    if (_swire) {
      _cur_layer = create_via_array(_swire,
                                    _wire_shape_type,
                                    _cur_layer,
                                    tech_via,
                                    _prev_x,
                                    _prev_y,
                                    numX,
                                    numY,
                                    stepX,
                                    stepY,
                                    /* bottom_mask */ 0,
                                    /* cut_mask */ 0,
                                    /* top_mask */ 0,
                                    _logger);
    }
  } else {
    dbVia* via = _block->findVia(via_name);

    if (via == nullptr) {
      _logger->warn(
          utl::ODB, 164, "error: undefined via ({}) referenced", via_name);
      ++_errors;
      return;
    }

    stepX = dbdist(stepX);
    stepY = dbdist(stepY);

    if (_swire) {
      _cur_layer = create_via_array(_swire,
                                    _wire_shape_type,
                                    _cur_layer,
                                    via,
                                    _prev_x,
                                    _prev_y,
                                    numX,
                                    numY,
                                    stepX,
                                    stepY,
                                    /* bottom_mask */ 0,
                                    /* cut_mask */ 0,
                                    /* top_mask */ 0,
                                    _logger);
    }
  }
}

void definSNet::pathEnd()
{
  _cur_layer = nullptr;
}

void definSNet::wireEnd()
{
  if (_swire) {
    dbSet<dbSBox> wires = _swire->getWires();

    if (wires.reversible() && wires.orderReversed()) {
      wires.reverse();
    }
  }

  _swire = nullptr;
}

void definSNet::property(const char* name, const char* value)
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

void definSNet::property(const char* name, int value)
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

void definSNet::property(const char* name, double value)
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

void definSNet::end()
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

  dbSet<dbSWire> swires = _cur_net->getSWires();

  if (swires.reversible() && swires.orderReversed()) {
    swires.reverse();
  }

  _cur_net = nullptr;
}

void definSNet::connect_all(dbNet* net, const char* term)
{
  dbSet<dbITerm> iterms = _block->getITerms();

  net->setWildConnected();
  std::map<dbMTerm*, int> matched_mterms;
  std::vector<dbMaster*> masters;
  _block->getMasters(masters);
  std::vector<dbMaster*>::iterator mitr;

  for (mitr = masters.begin(); mitr != masters.end(); ++mitr) {
    dbMaster* master = *mitr;
    dbMTerm* mterm = master->findMTerm(_block, term);

    if (mterm != nullptr) {
      matched_mterms[mterm] = 1;
    }
  }

  dbSet<dbITerm>::iterator itr;

  for (itr = iterms.begin(); itr != iterms.end(); ++itr) {
    dbITerm* iterm = *itr;

    dbMTerm* mterm = iterm->getMTerm();

    if (matched_mterms.find(mterm) != matched_mterms.end()) {
      iterm->connect(net);
      iterm->setSpecial();
      _snet_iterm_cnt++;
    }
  }
}

}  // namespace odb
