// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cassert>

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbWire.h"
#include "dbWireOpcode.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

#define DB_WIRE_DECODE_INVALID_OPCODE 0

//////////////////////////////////////////////////////////////////////////////////
//
// dbWirePathItr
//
//////////////////////////////////////////////////////////////////////////////////
dbWirePathItr::dbWirePathItr()
{
  opcode_ = dbWireDecoder::END_DECODE;
  prev_x_ = 0;
  prev_y_ = 0;
  prev_ext_ = 0;
  has_prev_ext_ = false;
  dw_ = 0;
  rule_ = nullptr;
  wire_ = nullptr;
}

void dbWirePathItr::begin(dbWire* wire)
{
  decoder_.begin(wire);
  opcode_ = decoder_.next();
  prev_x_ = 0;
  prev_y_ = 0;
  prev_ext_ = 0;
  has_prev_ext_ = false;
  dw_ = 0;
  rule_ = nullptr;
  wire_ = wire;
}

#define DB_WIRE_PATH_ITR_INVALID_OPCODE 0

bool dbWirePathItr::getNextPath(dbWirePath& path)
{
  if ((opcode_ == dbWireDecoder::PATH) || (opcode_ == dbWireDecoder::JUNCTION)
      || (opcode_ == dbWireDecoder::SHORT)
      || (opcode_ == dbWireDecoder::VWIRE)) {
    dbTechLayerRule* lyr_rule = nullptr;
    rule_ = nullptr;

    if (opcode_ == dbWireDecoder::JUNCTION) {
      path.junction_id = decoder_.getJunctionValue();
      path.is_branch = true;
    } else {
      path.is_branch = false;
    }

    if ((opcode_ == dbWireDecoder::SHORT)
        || (opcode_ == dbWireDecoder::VWIRE)) {
      path.is_short = true;
      path.short_junction = decoder_.getJunctionValue();
    } else {
      path.is_short = false;
      path.short_junction = 0;
    }

    path.iterm = nullptr;
    path.bterm = nullptr;
    path.layer = decoder_.getLayer();

  get_point:

    opcode_ = decoder_.next();

    if (opcode_ == dbWireDecoder::POINT) {
      decoder_.getPoint(prev_x_, prev_y_);
      prev_ext_ = 0;
      has_prev_ext_ = false;

      if (path.is_branch == false) {
        path.junction_id = decoder_.getJunctionId();
      }
    } else if (opcode_ == dbWireDecoder::POINT_EXT) {
      decoder_.getPoint(prev_x_, prev_y_, prev_ext_);
      has_prev_ext_ = true;

      if (path.is_branch == false) {
        path.junction_id = decoder_.getJunctionId();
      }

    } else if (opcode_ == dbWireDecoder::RULE) {
      lyr_rule = decoder_.getRule();
      rule_ = lyr_rule->getNonDefaultRule();
      goto get_point;  // rules preceded a point...
    } else {
      assert(DB_WIRE_PATH_ITR_INVALID_OPCODE);
    }

    path.point.setX(prev_x_);
    path.point.setY(prev_y_);

    // Check for sequence: (BTERM), (ITERM), (BTERM, ITERM) or, (ITERM BTERM)
    dbWireDecoder::OpCode next_opcode = decoder_.peek();

    if (next_opcode == dbWireDecoder::BTERM) {
      opcode_ = decoder_.next();
      path.bterm = decoder_.getBTerm();
    }

    else if (next_opcode == dbWireDecoder::ITERM) {
      opcode_ = decoder_.next();
      path.iterm = decoder_.getITerm();
    }

    next_opcode = decoder_.peek();

    if (next_opcode == dbWireDecoder::BTERM) {
      opcode_ = decoder_.next();
      path.bterm = decoder_.getBTerm();
    }

    else if (next_opcode == dbWireDecoder::ITERM) {
      opcode_ = decoder_.next();
      path.iterm = decoder_.getITerm();
    }

    path.rule = rule_;
    if (lyr_rule) {
      dw_ = lyr_rule->getWidth() >> 1;
    } else {
      dw_ = decoder_.getLayer()->getWidth() >> 1;
    }
    opcode_ = decoder_.next();
    return true;
  }

  return false;
}

inline void dbWirePathItr::getTerms(dbWirePathShape& s)
{
  // Check for sequence: (BTERM), (ITERM), (BTERM, ITERM) or, (ITERM BTERM)
  dbWireDecoder::OpCode next_opcode = decoder_.peek();

  if (next_opcode == dbWireDecoder::BTERM) {
    opcode_ = decoder_.next();
    s.bterm = decoder_.getBTerm();
  }

  else if (next_opcode == dbWireDecoder::ITERM) {
    opcode_ = decoder_.next();
    s.iterm = decoder_.getITerm();
  }

  next_opcode = decoder_.peek();

  if (next_opcode == dbWireDecoder::BTERM) {
    opcode_ = decoder_.next();
    s.bterm = decoder_.getBTerm();
  }

  else if (next_opcode == dbWireDecoder::ITERM) {
    opcode_ = decoder_.next();
    s.iterm = decoder_.getITerm();
  }
}

bool dbWirePathItr::getNextShape(dbWirePathShape& s)
{
  s.iterm = nullptr;
  s.bterm = nullptr;
nextOpCode:
  switch (opcode_) {
    case dbWireDecoder::PATH:
    case dbWireDecoder::JUNCTION:
    case dbWireDecoder::SHORT:
    case dbWireDecoder::VWIRE:
      return false;

    case dbWireDecoder::RECT: {
      // order wires doesn't understand RECTs so just hide them for now
      // they aren't required to establish connectivity in our flow
      opcode_ = decoder_.next();
      goto nextOpCode;
    }
    case dbWireDecoder::POINT: {
      int cur_x;
      int cur_y;
      decoder_.getPoint(cur_x, cur_y);
      s.junction_id = decoder_.getJunctionId();
      s.point.setX(cur_x);
      s.point.setY(cur_y);
      s.layer = decoder_.getLayer();
      s.shape.setSegment(prev_x_,
                         prev_y_,
                         prev_ext_,
                         has_prev_ext_,
                         cur_x,
                         cur_y,
                         0,
                         false,
                         dw_,
                         dw_,
                         s.layer);
      getTerms(s);

      prev_x_ = cur_x;
      prev_y_ = cur_y;
      prev_ext_ = 0;
      has_prev_ext_ = false;
      break;
    }

    case dbWireDecoder::POINT_EXT: {
      int cur_x;
      int cur_y;
      int cur_ext;
      decoder_.getPoint(cur_x, cur_y, cur_ext);

      //
      // By defintion a colinear-point with an extension must begin
      // a new path-segment
      //
      if ((cur_x == prev_x_) && (cur_y == prev_y_)) {
        prev_ext_ = cur_ext;
        has_prev_ext_ = true;
        opcode_ = decoder_.next();
        goto nextOpCode;
      }

      s.junction_id = decoder_.getJunctionId();
      s.point.setX(cur_x);
      s.point.setY(cur_y);
      s.layer = decoder_.getLayer();
      s.shape.setSegment(prev_x_,
                         prev_y_,
                         prev_ext_,
                         has_prev_ext_,
                         cur_x,
                         cur_y,
                         cur_ext,
                         true,
                         dw_,
                         dw_,
                         s.layer);
      getTerms(s);

      prev_x_ = cur_x;
      prev_y_ = cur_y;
      prev_ext_ = cur_ext;
      has_prev_ext_ = true;
      break;
    }

    case dbWireDecoder::VIA: {
      dbVia* via = decoder_.getVia();
      dbBox* box = via->getBBox();
      Rect b = box->getBox();
      int xmin = b.xMin() + prev_x_;
      int ymin = b.yMin() + prev_y_;
      int xmax = b.xMax() + prev_x_;
      int ymax = b.yMax() + prev_y_;
      Rect r(xmin, ymin, xmax, ymax);
      s.junction_id = decoder_.getJunctionId();
      s.point.setX(prev_x_);
      s.point.setY(prev_y_);
      s.layer = decoder_.getLayer();
      s.shape.setVia(via, r);
      getTerms(s);
      dw_ = decoder_.getLayer()->getWidth() >> 1;
      if (rule_) {
        dbTechLayerRule* lyr_rule = rule_->getLayerRule(decoder_.getLayer());
        if (lyr_rule) {
          dw_ = lyr_rule->getWidth() >> 1;
        }
      }
      prev_ext_ = 0;
      prev_ext_ = 0;
      has_prev_ext_ = false;
      break;
    }

    case dbWireDecoder::TECH_VIA: {
      dbTechVia* via = decoder_.getTechVia();
      dbBox* box = via->getBBox();
      Rect b = box->getBox();
      int xmin = b.xMin() + prev_x_;
      int ymin = b.yMin() + prev_y_;
      int xmax = b.xMax() + prev_x_;
      int ymax = b.yMax() + prev_y_;
      Rect r(xmin, ymin, xmax, ymax);
      s.junction_id = decoder_.getJunctionId();
      s.point.setX(prev_x_);
      s.point.setY(prev_y_);
      s.layer = decoder_.getLayer();
      s.shape.setVia(via, r);
      getTerms(s);
      dw_ = decoder_.getLayer()->getWidth() >> 1;
      if (rule_) {
        dbTechLayerRule* lyr_rule = rule_->getLayerRule(decoder_.getLayer());
        if (lyr_rule) {
          dw_ = lyr_rule->getWidth() >> 1;
        }
      }
      prev_ext_ = 0;
      has_prev_ext_ = false;
      break;
    }

    case dbWireDecoder::RULE: {
      dbTechLayerRule* rule = decoder_.getRule();
      dw_ = rule->getWidth() >> 1;
      opcode_ = decoder_.next();
      goto nextOpCode;
    }

    case dbWireDecoder::END_DECODE:
      return false;

    default:
      assert(DB_WIRE_PATH_ITR_INVALID_OPCODE);
      opcode_ = decoder_.next();
      goto nextOpCode;
  }

  opcode_ = decoder_.next();
  return true;
}

//
// Routines for struct dbWirePath here
//
void dbWirePath::dump(utl::Logger* logger, const char* group, int level) const
{
  debugPrint(logger,
             utl::ODB,
             group,
             level,
             "Path id: {}  at {} {}  ",
             junction_id,
             point.getX(),
             point.getY());
  if (layer) {
    debugPrint(logger, utl::ODB, group, level, "layer {}  ", layer->getName());
  } else {
    debugPrint(logger, utl::ODB, group, level, "NO LAYER  ");
  }

  if (rule) {
    debugPrint(logger,
               utl::ODB,
               group,
               level,
               "non-default rule {} ",
               rule->getName());
  }

  if (iterm) {
    debugPrint(logger,
               utl::ODB,
               group,
               level,
               "Connects to Iterm {}  ",
               iterm->getMTerm()->getName());
  }

  if (bterm) {
    debugPrint(logger,
               utl::ODB,
               group,
               level,
               "Connects to Bterm {}  ",
               bterm->getName());
  }

  if (is_branch) {
    debugPrint(logger, utl::ODB, group, level, "is branch  ");
  }

  if (is_short) {
    debugPrint(
        logger, utl::ODB, group, level, "is short to {} ", short_junction);
  }
}

//
// Routines for struct dbWirePathShape here
//
void dbWirePathShape::dump(utl::Logger* logger,
                           const char* group,
                           int level) const
{
  debugPrint(logger,
             utl::ODB,
             group,
             level,
             "WireShape id: {}  at {} {}  ",
             junction_id,
             point.getX(),
             point.getY());

  if (layer) {
    debugPrint(logger, utl::ODB, group, level, "layer {}  ", layer->getName());
  } else {
    debugPrint(logger, utl::ODB, group, level, "NO LAYER  ");
  }

  if (iterm) {
    debugPrint(logger,
               utl::ODB,
               group,
               level,
               "Connects to Iterm {} {}  ",
               iterm->getId(),
               iterm->getMTerm()->getName());
  }

  if (bterm) {
    debugPrint(logger,
               utl::ODB,
               group,
               level,
               "Connects to Bterm {} {}  ",
               bterm->getId(),
               bterm->getName());
  }

  shape.dump(logger, group, level);
}

//
//  Inline routines of dbShape are in dbShape.h -- non-inlines are here
//
void dbShape::dump(utl::Logger* logger, const char* group, int level) const
{
  debugPrint(logger,
             utl::ODB,
             group,
             level,
             "Shape at ({} {}) ({} {}) of type ",
             xMin(),
             yMin(),
             xMax(),
             yMax());

  switch (getType()) {
    case VIA: {
      debugPrint(logger, utl::ODB, group, level, "Block Via:  ");
      break;
    }

    case TECH_VIA: {
      debugPrint(logger,
                 utl::ODB,
                 group,
                 level,
                 "Tech Via {}  ",
                 getTechVia()->getName());
      break;
    }

    case SEGMENT: {
      debugPrint(logger,
                 utl::ODB,
                 group,
                 level,
                 "Wire Segment on layer {}  ",
                 getTechLayer()->getName());
      break;
    }
    default:
      break;
  }
}

//
// Utility to dump out wire path iterator for a net.
//
void dumpWirePaths4Net(dbNet* innet, const char* group, int level)
{
  if (!innet) {
    return;
  }
  utl::Logger* logger = innet->getImpl()->getLogger();

  const char* prfx = "dumpWirePaths:";
  dbWire* wire0 = innet->getWire();
  if (!wire0) {
    logger->warn(
        utl::ODB, 87, "{} No wires for net {}", prfx, innet->getName());
    return;
  }
  debugPrint(logger,
             utl::ODB,
             group,
             level,
             "{} Dumping wire paths for net {}",
             prfx,
             innet->getName());
  dbWirePathItr pitr;
  struct dbWirePath curpath;
  struct dbWirePathShape curshp;
  pitr.begin(wire0);

  while (pitr.getNextPath(curpath)) {
    curpath.dump(logger, group, level);
    while (pitr.getNextShape(curshp)) {
      curshp.dump(logger, group, level);
    }
  }
}

}  // namespace odb
