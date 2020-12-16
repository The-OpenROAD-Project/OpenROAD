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

#include "db.h"
#include "dbBlock.h"
#include "dbNet.h"
#include "dbShape.h"
#include "dbTable.h"
#include "dbWire.h"
#include "dbWireCodec.h"
#include "dbWireOpcode.h"
#include "dbLogger.h"

namespace odb {

#define DB_WIRE_DECODE_INVALID_OPCODE 0

//////////////////////////////////////////////////////////////////////////////////
//
// dbWirePathItr
//
//////////////////////////////////////////////////////////////////////////////////
dbWirePathItr::dbWirePathItr()
{
  _opcode       = dbWireDecoder::END_DECODE;
  _prev_x       = 0;
  _prev_y       = 0;
  _prev_ext     = 0;
  _has_prev_ext = false;
  _dw           = 0;
  _rule         = NULL;
  _wire         = NULL;
}

dbWirePathItr::~dbWirePathItr()
{
}

void dbWirePathItr::begin(dbWire* wire)
{
  _decoder.begin(wire);
  _opcode       = _decoder.next();
  _prev_x       = 0;
  _prev_y       = 0;
  _prev_ext     = 0;
  _has_prev_ext = false;
  _dw           = 0;
  _rule         = NULL;
  _wire         = wire;
}

#define DB_WIRE_PATH_ITR_INVALID_OPCODE 0

bool dbWirePathItr::getNextPath(dbWirePath& path)
{
  if ((_opcode == dbWireDecoder::PATH) || (_opcode == dbWireDecoder::JUNCTION)
      || (_opcode == dbWireDecoder::SHORT)
      || (_opcode == dbWireDecoder::VWIRE)) {
    dbTechLayerRule* lyr_rule = NULL;
    _rule                     = NULL;

    if (_opcode == dbWireDecoder::JUNCTION) {
      path.junction_id = _decoder.getJunctionValue();
      path.is_branch   = true;
    } else {
      path.is_branch = false;
    }

    if ((_opcode == dbWireDecoder::SHORT)
        || (_opcode == dbWireDecoder::VWIRE)) {
      path.is_short       = true;
      path.short_junction = _decoder.getJunctionValue();
    } else {
      path.is_short       = false;
      path.short_junction = 0;
    }

    path.iterm = NULL;
    path.bterm = NULL;
    path.layer = _decoder.getLayer();

  get_point:

    _opcode = _decoder.next();

    if (_opcode == dbWireDecoder::POINT) {
      _decoder.getPoint(_prev_x, _prev_y);
      _prev_ext     = 0;
      _has_prev_ext = false;

      if (path.is_branch == false)
        path.junction_id = _decoder.getJunctionId();
    } else if (_opcode == dbWireDecoder::POINT_EXT) {
      _decoder.getPoint(_prev_x, _prev_y, _prev_ext);
      _has_prev_ext = true;

      if (path.is_branch == false)
        path.junction_id = _decoder.getJunctionId();

    } else if (_opcode == dbWireDecoder::RULE) {
      lyr_rule = _decoder.getRule();
      _rule    = lyr_rule->getNonDefaultRule();
      goto get_point;  // rules preceded a point...
    } else {
      assert(DB_WIRE_PATH_ITR_INVALID_OPCODE);
    }

    path.point.setX(_prev_x);
    path.point.setY(_prev_y);

    // Check for sequence: (BTERM), (ITERM), (BTERM, ITERM) or, (ITERM BTERM)
    dbWireDecoder::OpCode next_opcode = _decoder.peek();

    if (next_opcode == dbWireDecoder::BTERM) {
      _opcode    = _decoder.next();
      path.bterm = _decoder.getBTerm();
    }

    else if (next_opcode == dbWireDecoder::ITERM) {
      _opcode    = _decoder.next();
      path.iterm = _decoder.getITerm();
    }

    next_opcode = _decoder.peek();

    if (next_opcode == dbWireDecoder::BTERM) {
      _opcode    = _decoder.next();
      path.bterm = _decoder.getBTerm();
    }

    else if (next_opcode == dbWireDecoder::ITERM) {
      _opcode    = _decoder.next();
      path.iterm = _decoder.getITerm();
    }

    path.rule = _rule;
    if (lyr_rule)
      _dw = lyr_rule->getWidth() >> 1;
    else
      _dw = _decoder.getLayer()->getWidth() >> 1;
    _opcode = _decoder.next();
    return true;
  }

  return false;
}

inline void dbWirePathItr::getTerms(dbWirePathShape& s)
{
  // Check for sequence: (BTERM), (ITERM), (BTERM, ITERM) or, (ITERM BTERM)
  dbWireDecoder::OpCode next_opcode = _decoder.peek();

  if (next_opcode == dbWireDecoder::BTERM) {
    _opcode = _decoder.next();
    s.bterm = _decoder.getBTerm();
  }

  else if (next_opcode == dbWireDecoder::ITERM) {
    _opcode = _decoder.next();
    s.iterm = _decoder.getITerm();
  }

  next_opcode = _decoder.peek();

  if (next_opcode == dbWireDecoder::BTERM) {
    _opcode = _decoder.next();
    s.bterm = _decoder.getBTerm();
  }

  else if (next_opcode == dbWireDecoder::ITERM) {
    _opcode = _decoder.next();
    s.iterm = _decoder.getITerm();
  }
}

bool dbWirePathItr::getNextShape(dbWirePathShape& s)
{
  s.iterm = NULL;
  s.bterm = NULL;
nextOpCode:
  switch (_opcode) {
    case dbWireDecoder::PATH:
    case dbWireDecoder::JUNCTION:
    case dbWireDecoder::SHORT:
    case dbWireDecoder::VWIRE:
    case dbWireDecoder::RECT:
      return false;

    case dbWireDecoder::POINT: {
      int cur_x;
      int cur_y;
      _decoder.getPoint(cur_x, cur_y);
      s.junction_id = _decoder.getJunctionId();
      s.point.setX(cur_x);
      s.point.setY(cur_y);
      s.layer = _decoder.getLayer();
      s.shape.setSegment(_prev_x,
                         _prev_y,
                         _prev_ext,
                         _has_prev_ext,
                         cur_x,
                         cur_y,
                         0,
                         false,
                         _dw,
                         s.layer);
      getTerms(s);

      _prev_x       = cur_x;
      _prev_y       = cur_y;
      _prev_ext     = 0;
      _has_prev_ext = false;
      break;
    }

    case dbWireDecoder::POINT_EXT: {
      int cur_x;
      int cur_y;
      int cur_ext;
      _decoder.getPoint(cur_x, cur_y, cur_ext);

      //
      // By defintion a colinear-point with an extension must begin
      // a new path-segment
      //
      if ((cur_x == _prev_x) && (cur_y == _prev_y)) {
        _prev_ext     = cur_ext;
        _has_prev_ext = true;
        _opcode       = _decoder.next();
        goto nextOpCode;
      }

      s.junction_id = _decoder.getJunctionId();
      s.point.setX(cur_x);
      s.point.setY(cur_y);
      s.layer = _decoder.getLayer();
      s.shape.setSegment(_prev_x,
                         _prev_y,
                         _prev_ext,
                         _has_prev_ext,
                         cur_x,
                         cur_y,
                         cur_ext,
                         true,
                         _dw,
                         s.layer);
      getTerms(s);

      _prev_x       = cur_x;
      _prev_y       = cur_y;
      _prev_ext     = cur_ext;
      _has_prev_ext = true;
      break;
    }

    case dbWireDecoder::VIA: {
      dbVia* via = _decoder.getVia();
      dbBox* box = via->getBBox();
      Rect   b;
      box->getBox(b);
      int  xmin = b.xMin() + _prev_x;
      int  ymin = b.yMin() + _prev_y;
      int  xmax = b.xMax() + _prev_x;
      int  ymax = b.yMax() + _prev_y;
      Rect r(xmin, ymin, xmax, ymax);
      s.junction_id = _decoder.getJunctionId();
      s.point.setX(_prev_x);
      s.point.setY(_prev_y);
      s.layer = _decoder.getLayer();
      s.shape.setVia(via, r);
      getTerms(s);
      _dw = _decoder.getLayer()->getWidth() >> 1;
      if (_rule) {
        dbTechLayerRule* lyr_rule = _rule->getLayerRule(_decoder.getLayer());
        if (lyr_rule)
          _dw = lyr_rule->getWidth() >> 1;
      }
      _prev_ext     = 0;
      _prev_ext     = 0;
      _has_prev_ext = false;
      break;
    }

    case dbWireDecoder::TECH_VIA: {
      dbTechVia* via = _decoder.getTechVia();
      dbBox*     box = via->getBBox();
      Rect       b;
      box->getBox(b);
      int  xmin = b.xMin() + _prev_x;
      int  ymin = b.yMin() + _prev_y;
      int  xmax = b.xMax() + _prev_x;
      int  ymax = b.yMax() + _prev_y;
      Rect r(xmin, ymin, xmax, ymax);
      s.junction_id = _decoder.getJunctionId();
      s.point.setX(_prev_x);
      s.point.setY(_prev_y);
      s.layer = _decoder.getLayer();
      s.shape.setVia(via, r);
      getTerms(s);
      _dw = _decoder.getLayer()->getWidth() >> 1;
      if (_rule) {
        dbTechLayerRule* lyr_rule = _rule->getLayerRule(_decoder.getLayer());
        if (lyr_rule)
          _dw = lyr_rule->getWidth() >> 1;
      }
      _prev_ext     = 0;
      _has_prev_ext = false;
      break;
    }

    case dbWireDecoder::RULE: {
      dbTechLayerRule* rule = _decoder.getRule();
      _dw                   = rule->getWidth() >> 1;
      _opcode               = _decoder.next();
      goto nextOpCode;
    }

    case dbWireDecoder::END_DECODE:
      return false;

    default:
      ZASSERT(DB_WIRE_PATH_ITR_INVALID_OPCODE);
      _opcode = _decoder.next();
      goto nextOpCode;
  }

  _opcode = _decoder.next();
  return true;
}

//
// Routines for struct dbWirePath here
//
void dbWirePath::dump(const char* module_name, const char* tag) const
{
  debug(module_name,
        tag,
        "Path id: %d  at %d %d  ",
        junction_id,
        point.getX(),
        point.getY());
  if (layer)
    debug(module_name, tag, "layer %s  ", layer->getName().c_str());
  else
    debug(module_name, tag, "NO LAYER  ");

  if (rule)
    debug(module_name, tag, "non-default rule %s  ", rule->getName().c_str());

  if (iterm)
    debug(module_name,
          tag,
          "Connects to Iterm %s  ",
          iterm->getMTerm()->getName().c_str());

  if (bterm)
    debug(module_name, tag, "Connects to Bterm %s  ", bterm->getName().c_str());

  if (is_branch)
    debug(module_name, tag, "is branch  ");

  if (is_short)
    debug(module_name, tag, "is short to %d  ", short_junction);

  debug(module_name, tag, "\n");
}

//
// Routines for struct dbWirePathShape here
//
void dbWirePathShape::dump(const char* module_name, const char* tag) const
{
  debug(module_name,
        tag,
        "WireShape id: %d  at %d %d  ",
        junction_id,
        point.getX(),
        point.getY());

  if (layer)
    debug(module_name, tag, "layer %s  ", layer->getName().c_str());
  else
    debug(module_name, tag, "NO LAYER  ");

  if (iterm)
    debug(module_name,
          tag,
          "Connects to Iterm %d %s  ",
          iterm->getId(),
          iterm->getMTerm()->getName().c_str());

  if (bterm)
    debug(module_name,
          tag,
          "Connects to Bterm %d %s  ",
          bterm->getId(),
          bterm->getName().c_str());

  shape.dump(module_name, tag);

  debug(module_name, tag, " \n");
}

//
//  Inline routines of dbShape are in dbShape.h -- non-inlines are here
//
void dbShape::dump(const char* module_name, const char* tag) const
{
  debug(module_name,
        tag,
        "Shape at (%d %d) (%d %d) of type ",
        xMin(),
        yMin(),
        xMax(),
        yMax());

  switch (getType()) {
    case VIA: {
      debug(module_name, tag, "Block Via:  ");
      break;
    }

    case TECH_VIA: {
      debug(module_name, tag, "Tech Via %s  ", getTechVia()->getName().c_str());
      break;
    }

    case SEGMENT: {
      debug(module_name,
            tag,
            "Wire Segment on layer %s  ",
            getTechLayer()->getName().c_str());
      break;
    }
    default:
      break;
  }

  debug(module_name, tag, " \n");
}

//
// Utility to dump out wire path iterator for a net.
//
void dumpWirePaths4Net(dbNet* innet, const char* module_name, const char* tag)
{
  if (!innet)
    return;

  const char* prfx  = "dumpWirePaths:";
  dbWire*     wire0 = innet->getWire();
  if (!wire0) {
    warning(0, "%s No wires for net %s\n", prfx, innet->getName().c_str());
    return;
  }

  debug(module_name,
        tag,
        "\n%s Dumping wire paths for net %s\n",
        prfx,
        innet->getName().c_str());

  dbWirePathItr          pitr;
  struct dbWirePath      curpath;
  struct dbWirePathShape curshp;
  pitr.begin(wire0);

  while (pitr.getNextPath(curpath)) {
    curpath.dump(module_name, tag);
    while (pitr.getNextShape(curshp))
      curshp.dump(module_name, tag);
  }
}

}  // namespace odb
