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

#include "dbFlatten.h"

#include "db.h"
#include "dbCapNode.h"
#include "dbInst.h"
#include "dbNet.h"
#include "dbObstruction.h"
#include "dbSWire.h"
#include "dbShape.h"
#include "dbTransform.h"
#include "dbWire.h"
#include "dbWireOpcode.h"
#include "utl/Logger.h"

namespace odb {

dbFlatten::dbFlatten()
    : _do_not_copy_power_wires(true),
      _copy_shields(true),
      _create_boundary_regions(false),
      _create_bterm_map(false),
      _copy_parasitics(false),
      _hier_d(0),
      _next_bterm_map_id(0)
{
}

dbFlatten::~dbFlatten()
{
}

bool dbFlatten::flatten(dbBlock* block, int level)
{
  _hier_d = block->getHierarchyDelimeter();
  _next_bterm_map_id = 0;
  dbProperty* bterm_map = NULL;

  if (_create_bterm_map) {
    dbProperty* p = dbProperty::find(block, "_ADS_BTERM_MAP");

    if (p != NULL)
      dbProperty::destroy(p);

    std::string name = block->getName();
    bterm_map = dbStringProperty::create(block, "_ADS_BTERM_MAP", name.c_str());
  }

  bool error = false;
  dbSet<dbInst> insts = block->getInsts();
  dbSet<dbInst>::iterator itr;

  for (itr = insts.begin(); itr != insts.end(); ++itr) {
    dbInst* inst = *itr;
    dbBlock* child = inst->getChild();

    if (child)
      if (!flatten(block, child, level, bterm_map))
        error = true;
  }

  for (itr = insts.begin(); itr != insts.end();) {
    dbInst* inst = *itr;
    dbBlock* child = inst->getChild();

    if (child) {
      inst->unbindBlock();
      dbBlock::destroy(child);
      itr = dbInst::destroy(itr);
    } else
      ++itr;
  }

  return !error;
}

/* // flatten = flatten child block into parent
//
//           ..........o Parent (inst/block)
//           .        / \
//           .       /   \
//           .      / ... \
//           .     /
//           .    o Child (inst/block)
//           .   / \
//           .  /   \
//           . / ... \
//           ./
//           o Grandchild (inst)
// */
bool dbFlatten::flatten(dbBlock* parent,
                        dbBlock* child,
                        int level,
                        dbProperty* bterm_map)
{
  bool error = false;

  if (level == 0)
    return true;

  if (bterm_map) {
    std::string name = child->getName();
    std::string propName("_ADS_BTERM_MAP");
    propName += "_";
    propName += name.c_str();
    bterm_map
        = dbStringProperty::create(bterm_map, propName.c_str(), name.c_str());
  }

  // Descend hierarchy and flatten
  dbSet<dbInst> insts = child->getInsts();
  dbSet<dbInst>::iterator itr;

  for (itr = insts.begin(); itr != insts.end(); ++itr) {
    dbInst* inst = *itr;
    dbBlock* grandchild = inst->getChild();

    if (grandchild)
      flatten(child, grandchild, level - 1, bterm_map);
  }

  child->getParentInst()->getTransform(_transform);

  ////////////////////////////
  // Copy child data to parent
  ////////////////////////////

  _net_map.clear();
  _via_map.clear();
  _inst_map.clear();
  _layer_rule_map.clear();
  _reg_map.clear();

  ////////////////////////////
  // Copy vias
  ////////////////////////////
  dbVia::copy(parent, child);
  dbSet<dbVia> vias = child->getVias();
  dbSet<dbVia>::iterator vitr;
  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    dbVia* v = *vitr;
    std::string name = v->getName();
    _via_map[v] = parent->findVia(name.c_str());
  }

  ////////////////////////////
  // Copy non-default_rules
  ////////////////////////////
  dbSet<dbTechNonDefaultRule> ndrules = child->getNonDefaultRules();
  dbSet<dbTechNonDefaultRule>::iterator nditr;
  for (nditr = ndrules.begin(); nditr != ndrules.end(); ++nditr) {
    dbTechNonDefaultRule* rule = *nditr;
    copyNonDefaultRule(parent, child->getParentInst(), rule);
  }

  ////////////////////////////
  // Copy nets
  ////////////////////////////
  dbSet<dbNet> nets = child->getNets();
  dbSet<dbNet>::iterator nitr;

  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    dbNet* src = *nitr;
    dbNet* dst = copyNet(parent, src);

    if (dst == NULL)
      error = true;

    _net_map[src] = dst;
  }

  ////////////////////////////
  // Copy instances
  ////////////////////////////
  for (itr = insts.begin(); itr != insts.end();) {
    dbInst* inst = *itr;
    dbBlock* grandchild = inst->getChild();

    if (grandchild) {
      inst->unbindBlock();
      dbBlock::destroy(grandchild);
      itr = dbInst::destroy(itr);
    } else {
      if (!copyInst(parent, child->getParentInst(), inst))
        error = true;

      ++itr;
    }
  }

  ////////////////////////////
  // Copy the wires seperately
  ////////////////////////////
  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    dbNet* src = *nitr;
    dbNet* dst = _net_map[src];

    if (dst)
      copyNetWires(dst, src, level, bterm_map);
  }

  ////////////////////////////
  // Copy obstructions
  ////////////////////////////
  dbSet<dbObstruction> obstructions = child->getObstructions();
  dbSet<dbObstruction>::iterator oitr;

  for (oitr = obstructions.begin(); oitr != obstructions.end(); ++oitr) {
    dbObstruction* o = *oitr;
    copyObstruction(parent, o);
  }

  ////////////////////////////
  // Copy blocakges
  ////////////////////////////
  dbSet<dbBlockage> blocakges = child->getBlockages();
  dbSet<dbBlockage>::iterator bitr;

  for (bitr = blocakges.begin(); bitr != blocakges.end(); ++bitr) {
    dbBlockage* b = *bitr;
    copyBlockage(parent, b);
  }

  ////////////////////////////
  // Copy regions
  ////////////////////////////
  dbSet<dbRegion> regions = child->getRegions();
  dbSet<dbRegion>::iterator rgitr;

  for (rgitr = regions.begin(); rgitr != regions.end(); ++rgitr) {
    dbRegion* rg = *rgitr;
    if (rg->getParent() == NULL)
      copyRegion(parent, child->getParentInst(), NULL, rg);
  }

  if (_create_boundary_regions) {
    std::string name = child->getParentInst()->getName();
    name += _hier_d;
    name += "ADS_BLOCK_REGION";
    dbRegion* block_region = dbRegion::create(parent, name.c_str());
    Rect bndry;
    child->getDieArea(bndry);
    _transform.apply(bndry);
    dbBox::create(
        block_region, bndry.xMin(), bndry.yMin(), bndry.xMax(), bndry.yMax());

    for (rgitr = regions.begin(); rgitr != regions.end(); ++rgitr) {
      dbRegion* rg = *rgitr;

      if (rg->getParent() == NULL)
        block_region->addChild(_reg_map[rg]);
    }

    for (itr = insts.begin(); itr != insts.end(); ++itr) {
      dbInst* inst = *itr;

      if (inst->getRegion() == NULL)
        block_region->addInst(_inst_map[inst]);
    }
  }

  _net_map.clear();
  _via_map.clear();
  _inst_map.clear();
  _layer_rule_map.clear();
  _reg_map.clear();
  _node_map.clear();
  return !error;
}

/*
// copyInst - This method copies the Grandchild from the Child to the Parent
node
//
//           ..........o Parent (inst/block)
//           .        / \
//           .       /   \
//           .      / ... \
//           .     /
//           .    o Child (inst/block)
//           .   / \
//           .  /   \
//           . / ... \
//           ./
//           o Grandchild (inst)
//
*/
bool dbFlatten::copyInst(dbBlock* parent, dbInst* child, dbInst* grandchild)
{
  std::string name = child->getName();
  name += _hier_d;
  name += grandchild->getName();

  dbInst* inst = dbInst::create(parent, grandchild->getMaster(), name.c_str());
  _inst_map[grandchild] = inst;

  // Copy placement
  dbTransform gt;
  grandchild->getTransform(gt);
  gt.concat(_transform);
  inst->setTransform(gt);
  inst->setPlacementStatus(child->getPlacementStatus());

  // Copy connections
  dbSet<dbITerm> iterms = grandchild->getITerms();
  dbSet<dbITerm>::iterator itr;

  for (itr = iterms.begin(); itr != iterms.end(); ++itr) {
    dbITerm* child_iterm = *itr;
    dbNet* child_net = child_iterm->getNet();

    if (child_net != NULL) {
      dbNet* parent_net = _net_map[child_net];

      if (parent_net == NULL)
        continue;  // error

      dbITerm* parent_iterm = inst->getITerm(child_iterm->getMTerm());
      dbITerm::connect(parent_iterm, parent_net);
    }
  }

  // Copy misc. attributes
  copyAttrs(inst, child);
  return true;
}

dbNet* dbFlatten::copyNet(dbBlock* parent_block, dbNet* child_net)
{
  dbSet<dbBTerm> bterms = child_net->getBTerms();

  if (!bterms.empty()) {
    // external net, find a parent
    dbSet<dbBTerm>::iterator itr;

    for (itr = bterms.begin(); itr != bterms.end(); ++itr) {
      dbBTerm* bterm = *itr;
      dbITerm* iterm = bterm->getITerm();
      dbNet* net = iterm->getNet();

      if (net)
        return net;
    }

    // None of the iterms were connected to a net in the parent-inst.
    // Create a new net?
    // TODO: Does this case need special consideration...
  }

  // internal net, export up hierarchy
  dbInst* inst = child_net->getBlock()->getParentInst();
  std::string name = inst->getName();
  name += _hier_d;
  name += child_net->getName();
  dbNet* net = dbNet::create(parent_block, name.c_str());

  if (net == NULL) {
    // TODO:
    parent_block->getImpl()->getLogger()->warn(
        utl::ODB, 271, "Failed to create net {}", name);
    return NULL;
  }

  // copy attrs
  copyAttrs(net, child_net);
  return net;
}

void dbFlatten::printShapes(FILE* fp, dbWire* wire, bool skip_rcSegs)
{
  dbWireShapeItr shapes;
  dbShape s;

  for (shapes.begin(wire); shapes.next(s);) {
    uint sid = shapes.getShapeId();
    Rect r;
    s.getBox(r);
    fprintf(fp,
            "J%d -- %d %d %d %d\n",
            sid,
            r.xMin(),
            r.yMin(),
            r.xMax(),
            r.yMax());
  }
  if (skip_rcSegs)
    return;
  fprintf(fp, "\ncapNOdes\n");
  dbSet<dbCapNode> nodeSet = wire->getNet()->getCapNodes();
  dbSet<dbCapNode>::iterator rc_itr;
  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    dbCapNode* node = *rc_itr;

    uint node_num = node->getNode();
    uint sid = node->getShapeId();
    fprintf(fp, "cap%d  node%d  J%d\n", node->getId(), node_num, sid);
  }
  dbBlock* block = wire->getNet()->getBlock();
  dbSet<dbRSeg> rSet = wire->getNet()->getRSegs();
  dbSet<dbRSeg>::iterator rcitr;

  fprintf(fp, "\nRSEGS\n");
  for (rcitr = rSet.begin(); rcitr != rSet.end(); ++rcitr) {
    dbRSeg* rc = *rcitr;
    dbCapNode* capNode = dbCapNode::getCapNode(block, rc->getTargetNode());

    uint shapeId = rc->getShapeId();
    dbCapNode* srcNode = rc->getSourceCapNode();

    fprintf(fp, "%d J%d rc%d ", srcNode->getNode(), shapeId, rc->getId());

    // if (!_foreign && shapeId==0)
    if (shapeId == 0) {
      fprintf(fp, "\n");
      continue;
    }

    dbShape s;
    wire->getShape(shapeId, s);
    if (s.isVia())
      continue;

    Rect r;
    s.getBox(r);

    fprintf(fp, "--  %d %d %d %d --", r.xMin(), r.yMin(), r.xMax(), r.yMax());

    if (capNode->isITerm() || capNode->isBTerm()) {
      fprintf(fp, "B%d I%d -- ", capNode->isBTerm(), capNode->isITerm());
      // continue;
    }

    int rsegId;
    wire->getProperty(shapeId, rsegId);

    fprintf(fp,
            " prop%d tgt%d node%d\n",
            rsegId,
            rc->getTargetNode(),
            capNode->getId());
  }
}
void dbFlatten::setOldShapeIds(dbWire* wire)
{
  dbWireShapeItr shapes;
  dbShape s;

  for (shapes.begin(wire); shapes.next(s);) {
    uint sid = shapes.getShapeId();
    dbShape s;
    wire->getShape(sid, s);
    if (s.isVia())
      continue;
    wire->setProperty(sid, -sid);
  }
}
void dbFlatten::mapOld2newIds(dbWire* wire, FILE* fp)
{
  if (fp != NULL)
    fprintf(fp, "\nmapOld2newIds\n");
  dbWireShapeItr shapes;
  dbShape s;

  for (shapes.begin(wire); shapes.next(s);) {
    uint sid = shapes.getShapeId();
    dbShape s;
    wire->getShape(sid, s);
    if (s.isVia())
      continue;
    int n;
    wire->getProperty(sid, n);
    if (n >= 0)
      continue;
    _shape_rc_map[-n] = sid;
    if (fp != NULL)
      fprintf(fp, "old shapeId %d to new %d\n", -n, sid);
  }
}
void dbFlatten::setShapeProperties(dbWire* wire)
{
  dbBlock* block = wire->getNet()->getBlock();
  dbSet<dbRSeg> rSet = wire->getNet()->getRSegs();
  dbSet<dbRSeg>::iterator rcitr;
  for (rcitr = rSet.begin(); rcitr != rSet.end(); ++rcitr) {
    dbRSeg* rc = *rcitr;
    dbCapNode* capNode = dbCapNode::getCapNode(block, rc->getTargetNode());
    if (capNode->isITerm() || capNode->isBTerm())
      continue;

    uint shapeId = rc->getShapeId();

    // if (!_foreign && shapeId==0)
    if (shapeId == 0)
      continue;

    wire->setProperty(shapeId, -rc->getId());
  }
}

FILE* dbFlatten::debugNetWires(FILE* fp,
                               dbNet* dst,
                               dbNet* src,
                               const char* msg)
{
  if (fp == NULL) {
    char buf[32];
    sprintf(buf, "%d", dst->getId());
    fp = fopen(buf, "w");

    if (fp == NULL) {
      src->getImpl()->getLogger()->error(
          utl::ODB, 26, "Cannot Open file {} to write", buf);
      return nullptr;
    }
  }
  fprintf(fp, "%s  -------------------------------\n", msg);

  if (src != NULL) {
    fprintf(
        fp, "Rsegs for SRC : %s --------------------\n", src->getConstName());
    printRSegs(fp, src);
    fprintf(fp, "\tsrc Wire -- net %s\n\n", src->getConstName());
    if (src->getWire() != NULL) {
      src->getWire()->printWire(fp, 0, 0);
      printShapes(fp, src->getWire());
    }
  }
  if (dst != NULL) {
    fprintf(
        fp, "Rsegs for DST : %s --------------------\n", dst->getConstName());
    printRSegs(fp, dst);
    fprintf(fp, "\n\tdst Wire -- net %s\n\n", dst->getConstName());
    if (dst->getWire() != NULL) {
      dst->getWire()->printWire(fp, 0, 0);
      printShapes(fp, dst->getWire());
    }
  }
  return fp;
}
void dbFlatten::copyNetWires(dbNet* dst,
                             dbNet* src,
                             int level,
                             dbProperty* bterm_map)
{
  FILE* fp = NULL;
  if (isDebug("FLATTEN", "R"))
    fp = debugNetWires(NULL, dst, src, "Before CopyWires");

  copyWires(dst, src, level, bterm_map, _copy_parasitics);
  copySWires(dst, src);

  if (fp != NULL) {
    debugNetWires(fp, dst, NULL, "After CopyWires");
    fclose(fp);
  }
}

void dbFlatten::copyAttrs(dbInst* dst_, dbInst* src_)
{
  _dbInst* dst = (_dbInst*) dst_;
  _dbInst* src = (_dbInst*) src_;

  dst->_flags._user_flag_1 = src->_flags._user_flag_1;
  dst->_flags._user_flag_2 = src->_flags._user_flag_2;
  dst->_flags._user_flag_3 = src->_flags._user_flag_3;
  dst->_flags._size_only = src->_flags._size_only;
  dst->_flags._dont_touch = src->_flags._dont_touch;
  dst->_flags._dont_size = src->_flags._dont_size;
  dst->_flags._source = src->_flags._source;
  dst->_weight = src->_weight;
}

void dbFlatten::copyAttrs(dbNet* dst_, dbNet* src_)
{
  _dbNet* dst = (_dbNet*) dst_;
  _dbNet* src = (_dbNet*) src_;

  dst->_flags._sig_type = src->_flags._sig_type;
  dst->_flags._wire_type = src->_flags._wire_type;
  dst->_flags._special = src->_flags._special;
  dst->_flags._wild_connect = src->_flags._wild_connect;
  dst->_flags._wire_ordered = src->_flags._wire_ordered;
  dst->_flags._buffered = src->_flags._buffered;
  dst->_flags._disconnected = src->_flags._disconnected;
  dst->_flags._spef = src->_flags._spef;
  dst->_flags._select = src->_flags._select;
  dst->_flags._mark = src->_flags._mark;
  dst->_flags._mark_1 = src->_flags._mark_1;
  dst->_flags._wire_altered = src->_flags._wire_altered;
  dst->_flags._extracted = src->_flags._extracted;
  dst->_flags._rc_graph = src->_flags._rc_graph;
  dst->_flags._reduced = src->_flags._reduced;
  dst->_flags._set_io = src->_flags._set_io;
  dst->_flags._io = src->_flags._io;
  dst->_flags._dont_touch = src->_flags._dont_touch;
  dst->_flags._size_only = src->_flags._size_only;
  dst->_flags._fixed_bump = src->_flags._fixed_bump;
  dst->_flags._source = src->_flags._source;
  dst->_flags._rc_disconnected = src->_flags._rc_disconnected;
  dst->_weight = src->_weight;
  dst->_xtalk = src->_xtalk;
  dst->_non_default_rule = src->_non_default_rule;
}

void dbFlatten::copyWires(dbNet* dst_,
                          dbNet* src_,
                          int level,
                          dbProperty* bterm_map,
                          bool copyParasitics)
{
  //_dbNet * dst = (_dbNet *) dst_;
  _dbNet* src = (_dbNet*) src_;

  if (src->_wire) {
    _dbWire* src_wire = (_dbWire*) src_->getWire();

    if (canCopyWire((dbWire*) src_wire, src->_flags._sig_type)) {
      if (copyParasitics)
        setOldShapeIds(src_->getWire());

      if (dst_->getWire()) {
        dbVector<unsigned char> opcodes;
        dbVector<int> data;
        opcodes.reserve(src_wire->_opcodes.size());
        opcodes = src_wire->_opcodes;
        data.reserve(src_wire->_data.size());
        data = src_wire->_data;
        fixWire(opcodes, data, src_->getBlock(), level, bterm_map);
        appendWire(opcodes, data, dst_->getWire());
      } else {
        _dbWire* dst_wire = (_dbWire*) dbWire::create(dst_);
        dst_wire->_opcodes.reserve(src_wire->_opcodes.size());
        dst_wire->_opcodes = src_wire->_opcodes;
        dst_wire->_data.reserve(src_wire->_data.size());
        dst_wire->_data = src_wire->_data;
        fixWire(dst_wire->_opcodes,
                dst_wire->_data,
                src_->getBlock(),
                level,
                bterm_map);
      }
      if (copyParasitics) {
        dbSet<dbRSeg> rSet = dst_->getRSegs();
        rSet.reverse();

        rSet = dst_->getRSegs();
        rSet.reverse();
      }
    }
  }

  if (src->_global_wire) {
    _dbWire* src_wire = (_dbWire*) src_->getGlobalWire();

    if (canCopyWire((dbWire*) src_wire, src->_flags._sig_type)) {
      if (dst_->getGlobalWire()) {
        dbVector<unsigned char> opcodes;
        dbVector<int> data;
        opcodes.reserve(src_wire->_opcodes.size());
        opcodes = src_wire->_opcodes;
        data.reserve(src_wire->_data.size());
        data = src_wire->_data;
        fixWire(opcodes, data, src_->getBlock(), level, bterm_map);
        appendWire(opcodes, data, dst_->getGlobalWire());
      } else {
        _dbWire* dst_wire = (_dbWire*) dbWire::create(dst_, true);
        dst_wire->_opcodes.reserve(src_wire->_opcodes.size());
        dst_wire->_opcodes = src_wire->_opcodes;
        dst_wire->_data.reserve(src_wire->_data.size());
        dst_wire->_data = src_wire->_data;
        fixWire(dst_wire->_opcodes,
                dst_wire->_data,
                src_->getBlock(),
                level,
                bterm_map);
      }
    }
  }
}

void dbFlatten::fixWire(dbVector<unsigned char>& opcodes,
                        dbVector<int>& data,
                        dbBlock* src,
                        int level,
                        dbProperty* bterm_map)
{
  uint i;
  uint n = opcodes.size();
  uint point_cnt = 0;
  int curX = 0;
  int curY = 0;
  std::vector<Point> jct_points;
  jct_points.resize(opcodes.size());

  for (i = 0; i < n; ++i) {
    unsigned char opcode = opcodes[i] & WOP_OPCODE_MASK;

    switch (opcode) {
      case WOP_PATH:
      case WOP_SHORT:
      case WOP_VWIRE: {
        point_cnt = 0;
        break;
      }

      case WOP_JUNCTION: {
        Point p = jct_points[data[i]];
        curX = p.x();
        curY = p.y();
        point_cnt = 0;
        break;
      }

      case WOP_X: {
        curX = data[i];

        if (point_cnt != 0) {
          Point p(curX, curY);
          jct_points[i] = p;
          _transform.apply(p);
          data[i] = p.x();
        } else {
          curY = data[++i];
          Point p(curX, curY);
          jct_points[i - 1] = p;
          jct_points[i] = p;
          _transform.apply(p);
          data[i - 1] = p.x();
          data[i] = p.y();
        }

        point_cnt++;
        break;
      }

      case WOP_Y: {
        point_cnt++;
        curY = data[i];
        Point p(curX, curY);
        jct_points[i] = p;
        _transform.apply(p);
        data[i] = p.y();
        break;
      }

      case WOP_COLINEAR: {
        point_cnt++;
        jct_points[i] = Point(curX, curY);
        break;
      }

      case WOP_VIA: {
        uint vid = data[i];
        dbVia* src_via = dbVia::getVia(src, vid);
        dbVia* dst_via = _via_map[src_via];
        data[i] = dst_via->getImpl()->getOID();
        break;
      }

      case WOP_ITERM: {
        dbITerm* src_iterm = dbITerm::getITerm(src, data[i]);
        /*notice(0, "WOP_ITERM: src_iterm= %d I%d/%s %s\n",
                src_iterm->getId(), src_iterm->getInst()->getId(),
                src_iterm->getMTerm()->getConstName(),
           src_iterm->getInst()->getConstName());
        */

        // Check for hierarchical iterm, if iterm's block will be flattened then
        // discard this iterm
        if (src_iterm->getBTerm()) {
          // notice(0, "------------------------------> WOP_ITERM:\n");
          if (level > 1) {
            opcodes[i] = WOP_NOP;
            data[i] = 0;
            break;
          }
        }

        dbInst* src_inst = src_iterm->getInst();
        dbInst* dst_inst = _inst_map[src_inst];
        assert(dst_inst);
        dbMTerm* mterm = src_iterm->getMTerm();
        dbITerm* dst_iterm = dst_inst->getITerm(mterm);
        data[i] = dst_iterm->getImpl()->getOID();
        break;
      }

      case WOP_BTERM: {
        assert(bterm_map == NULL);
        opcodes[i] = WOP_NOP;
        data[i] = 0;
        break;
      }

      case WOP_RULE: {
        if (opcodes[i] & WOP_BLOCK_RULE) {
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(src, data[i]);
          data[i] = _layer_rule_map[rule]->getImpl()->getOID();
        }
      }
    }
  }
}

void dbFlatten::appendWire(dbVector<unsigned char>& opcodes,
                           dbVector<int>& data,
                           dbWire* dst_)
{
  _dbWire* dst = (_dbWire*) dst_;

  uint sz = dst->_opcodes.size();
  dst->_opcodes.insert(dst->_opcodes.end(), opcodes.begin(), opcodes.end());
  dst->_data.insert(dst->_data.end(), data.begin(), data.end());

  // Fix up the junction-ids
  int i;
  int n = dst->_opcodes.size();

  for (i = sz; i < n; ++i) {
    unsigned char opcode = dst->_opcodes[i] & WOP_OPCODE_MASK;
    if ((opcode == WOP_SHORT) || (opcode == WOP_JUNCTION)
        || (opcode == WOP_VWIRE))
      dst->_data[i] += sz;
  }
}

void dbFlatten::copySWires(dbNet* dst, dbNet* src)
{
  dbSet<dbSWire> swires = src->getSWires();
  dbSet<dbSWire>::iterator itr;

  for (itr = swires.begin(); itr != swires.end(); ++itr) {
    dbSWire* src_wire = *itr;

    if (canCopySWire(src_wire, src->getSigType()))
      copySWire(dst, src, src_wire);
  }
}

void dbFlatten::copySWire(dbNet* dst, dbNet* src, dbSWire* src_swire)
{
  dbSWire* dst_swire
      = dbSWire::create(dst, src->getWireType(), src_swire->getShield());

  dbSet<dbSBox> wires = src_swire->getWires();

  dbSet<dbSBox>::iterator itr;

  for (itr = wires.begin(); itr != wires.end(); ++itr) {
    dbSBox* w = *itr;

    if (!w->isVia()) {
      Rect r;
      w->getBox(r);
      _transform.apply(r);
      dbSBox::create(dst_swire,
                     w->getTechLayer(),
                     r.xMin(),
                     r.yMin(),
                     r.xMax(),
                     r.yMax(),
                     w->getWireShapeType());
    } else {
      int x, y;
      w->getViaXY(x, y);
      Point p(x, y);
      _transform.apply(p);

      if (w->getTechVia())
        dbSBox::create(
            dst_swire, (dbTechVia*) w, p.x(), p.y(), w->getWireShapeType());
      else {
        dbVia* v = _via_map[(dbVia*) w];
        dbSBox::create(dst_swire, v, p.x(), p.y(), w->getWireShapeType());
      }
    }
  }
}

bool dbFlatten::canCopyWire(dbWire* /* unused: wire_ */,
                            dbSigType::Value sig_type)
{
  //_dbWire * wire = (_dbWire *) wire_;

  if (_do_not_copy_power_wires) {
    if (sig_type == dbSigType::POWER || sig_type == dbSigType::GROUND)
      return false;
  }

  return true;
}

bool dbFlatten::canCopySWire(dbSWire* wire_, dbSigType::Value sig_type)
{
  _dbSWire* wire = (_dbSWire*) wire_;

  if (_do_not_copy_power_wires) {
    if (wire->_flags._wire_type == dbWireType::SHIELD && _copy_shields)
      return true;

    if (sig_type == dbSigType::POWER || sig_type == dbSigType::GROUND)
      return false;
  }

  return true;
}

void dbFlatten::copyObstruction(dbBlock* dst_block, dbObstruction* src_)
{
  _dbObstruction* src = (_dbObstruction*) src_;

  dbBox* box = src_->getBBox();
  Rect r;
  box->getBox(r);
  _transform.apply(r);

  _dbObstruction* dst
      = (_dbObstruction*) dbObstruction::create(dst_block,
                                                box->getTechLayer(),
                                                r.xMin(),
                                                r.yMin(),
                                                r.xMax(),
                                                r.yMax(),
                                                _inst_map[src_->getInstance()]);
  dst->_flags = src->_flags;
  dst->_min_spacing = src->_min_spacing;
  dst->_effective_width = src->_effective_width;
}

void dbFlatten::copyBlockage(dbBlock* dst_block, dbBlockage* src)
{
  dbBox* box = src->getBBox();
  Rect r;
  box->getBox(r);
  _transform.apply(r);

  // dbBlockage * dst = dbBlockage::create( dst_block,
  dbBlockage::create(dst_block,
                     r.xMin(),
                     r.yMin(),
                     r.xMax(),
                     r.yMax(),
                     _inst_map[src->getInstance()]);
}

//
// Copy "src" region of the child_inst to parent_block.
//
void dbFlatten::copyRegion(dbBlock* parent_block,
                           dbInst* child_inst,
                           dbRegion* parent_region,
                           dbRegion* src)
{
  std::string name = child_inst->getName();
  name += _hier_d;
  name += src->getName();

  dbRegion* dst;

  if (parent_region)
    dst = dbRegion::create(parent_region, name.c_str());
  else
    dst = dbRegion::create(parent_block, name.c_str());

  if (dst == NULL) {
    // TODO:
    parent_block->getImpl()->getLogger()->warn(
        utl::ODB, 272, "Failed to create region {}", name);
    return;
  }

  _reg_map[src] = dst;
  dst->setRegionType(src->getRegionType());
  dst->setInvalid(src->isInvalid());

  dbSet<dbBox> boxes = src->getBoundaries();
  dbSet<dbBox>::iterator bitr = boxes.begin();

  for (; bitr != boxes.end(); ++bitr) {
    dbBox* box = *bitr;
    Rect r;
    box->getBox(r);
    _transform.apply(r);
    dbBox::create(dst, r.xMin(), r.yMin(), r.xMax(), r.yMax());
  }

  dbSet<dbInst> insts = src->getRegionInsts();
  dbSet<dbInst>::iterator iitr = insts.begin();

  for (; iitr != insts.end(); ++iitr) {
    dbInst* inst = *iitr;
    dst->addInst(_inst_map[inst]);
  }

  dbSet<dbRegion> children = src->getChildren();
  dbSet<dbRegion>::iterator citr = children.begin();

  for (; citr != children.end(); ++citr) {
    dbRegion* child = *citr;
    copyRegion(parent_block, child_inst, dst, child);
  }
}

dbTechNonDefaultRule* dbFlatten::copyNonDefaultRule(
    dbBlock* parent,
    dbInst* child_inst,
    dbTechNonDefaultRule* src_rule)
{
  std::string name = child_inst->getName();
  name += _hier_d;
  name += src_rule->getName();

  dbTechNonDefaultRule* dst_rule
      = dbTechNonDefaultRule::create(parent, name.c_str());

  if (dst_rule == NULL)
    return NULL;

  dst_rule->setHardSpacing(src_rule->getHardSpacing());

  std::vector<dbTechVia*> vias;
  src_rule->getUseVias(vias);

  std::vector<dbTechVia*>::iterator vitr;

  for (vitr = vias.begin(); vitr != vias.end(); ++vitr)
    dst_rule->addUseVia(*vitr);

  std::vector<dbTechViaGenerateRule*> rules;
  src_rule->getUseViaRules(rules);

  std::vector<dbTechViaGenerateRule*>::iterator ritr;

  for (ritr = rules.begin(); ritr != rules.end(); ++ritr)
    dst_rule->addUseViaRule(*ritr);

  dbTech* tech = parent->getDb()->getTech();
  dbSet<dbTechLayer> layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator layitr;

  for (layitr = layers.begin(); layitr != layers.end(); ++layitr) {
    dbTechLayer* layer = *layitr;
    int count;

    if (src_rule->getMinCuts(layer, count))
      dst_rule->setMinCuts(layer, count);
  }

  std::vector<dbTechLayerRule*> layer_rules;
  src_rule->getLayerRules(layer_rules);

  std::vector<dbTechLayerRule*>::iterator layer_rules_itr;

  for (layer_rules_itr = layer_rules.begin();
       layer_rules_itr != layer_rules.end();
       ++layer_rules_itr) {
    dbTechLayerRule* src_lay_rule = *layer_rules_itr;
    dbTechLayerRule* dst_lay_rule
        = dbTechLayerRule::create(src_rule, src_lay_rule->getLayer());
    _layer_rule_map[src_lay_rule] = dst_lay_rule;

    dst_lay_rule->setWidth(src_lay_rule->getWidth());
    dst_lay_rule->setSpacing(src_lay_rule->getSpacing());
    dst_lay_rule->setWireExtension(src_lay_rule->getWireExtension());
  }

  return dst_rule;
}
bool dbFlatten::createParentCapNode(dbCapNode* node, dbNet* dstNet)
{
  bool foreign = false;

  debugPrint(node->getImpl()->getLogger(),
             utl::ODB,
             "FLATTEN",
             3,
             "Cap {} num {}",
             node->getId(),
             node->getNode());

  dbCapNode* cap = NULL;
  if (!node->isBTerm()) {  //
    cap = dbCapNode::create(dstNet, 0, foreign);
    _node_map[node->getId()] = cap->getId();
  }
  if (node->isInternal()) {  //
    cap->setInternalFlag();
    uint nodeNum = _shape_rc_map[node->getNode()];
    cap->setNode(nodeNum);
    debugPrint(node->getImpl()->getLogger(),
               utl::ODB,
               "FLATTEN",
               3,
               "\t--> {}     {}",
               cap->getId(),
               cap->getNode());
  } else if (node->isITerm()) {  //
    dbITerm* src_iterm = node->getITerm();
    dbInst* src_inst = src_iterm->getInst();

    dbInst* dst_inst = _inst_map[src_inst];
    assert(dst_inst);
    dbMTerm* mterm = src_iterm->getMTerm();
    dbITerm* dst_iterm = dst_inst->getITerm(mterm);
    cap->setNode(dst_iterm->getId());
    cap->setITermFlag();
    debugPrint(node->getImpl()->getLogger(),
               utl::ODB,
               "FLATTEN",
               3,
               "\t--> {}     {}",
               cap->getId(),
               cap->getNode());
  } else if (node->isBTerm()) {  //

    dbBTerm* bterm = node->getBTerm();
    dbITerm* iterm = bterm->getITerm();
    uint parentId = adjustParentNode2(dstNet, iterm->getId());
    _node_map[node->getId()] = parentId;

    debugPrint(node->getImpl()->getLogger(),
               utl::ODB,
               "FLATTEN",
               3,
               "\t\tG BTerm {} --> {} <-- {} ==> {} {}",
               node->getId(),
               bterm->getConstName(),
               iterm->getId(),
               _node_map[node->getId()],
               parentId);
  }
  return true;
}
uint dbFlatten::adjustParentNode2(dbNet* dstNet, uint srcTermId)
{
  dbSet<dbCapNode> capNodes = dstNet->getCapNodes();
  dbSet<dbCapNode>::iterator cap_node_itr = capNodes.begin();
  for (; cap_node_itr != capNodes.end(); ++cap_node_itr) {
    dbCapNode* node = *cap_node_itr;
    if (!node->isITerm())
      continue;

    uint nodeNum = node->getNode();
    dbITerm* iterm = node->getITerm();
    if (iterm->getId() != srcTermId)
      continue;

    uint jid = node->getShapeId();
    debugPrint(node->getImpl()->getLogger(),
               utl::ODB,
               "FLATTEN",
               3,
               "\tadjustParentNode {} J{} N{} srcTermId={}",
               node->getId(),
               jid,
               nodeNum,
               srcTermId);

    node->resetITermFlag();
    node->setInternalFlag();

    node->setNode(jid);

    return node->getId();
  }
  return 0;
}
dbCapNode* dbFlatten::checkNode(dbCapNode* src, uint srcTermId)
{
  if (src == NULL)
    return NULL;
  if (!src->isITerm())
    return NULL;

  debugPrint(src->getImpl()->getLogger(),
             utl::ODB,
             "FLATTEN",
             3,
             "\tcheckNode node={} i{} rcTermId={}",
             src->getId(),
             src->getITerm()->getId(),
             srcTermId);

  if (src->getITerm()->getId() == srcTermId)
    return src;

  return NULL;
}
uint dbFlatten::adjustParentNode(dbNet* dstNet, uint srcTermId)
{
  dbSet<dbRSeg> rsegs = dstNet->getRSegs();
  dbSet<dbRSeg>::iterator rseg_itr = rsegs.begin();
  for (; rseg_itr != rsegs.end(); ++rseg_itr) {
    dbRSeg* rseg = *rseg_itr;

    uint jid = rseg->getShapeId();
    debugPrint(dstNet->getImpl()->getLogger(),
               utl::ODB,
               "FLATTEN",
               3,
               "\tadjustParentNode J{} rseg{} {} {} srcTermId={}",
               jid,
               rseg->getId(),
               rseg->getSourceNode(),
               rseg->getTargetNode(),
               srcTermId);
    if ((rseg->getSourceNode() > 0) && (rseg->getTargetNode() > 0))
      continue;
    dbCapNode* src = rseg->getSourceCapNode();
    dbCapNode* tgt = rseg->getTargetCapNode();

    dbCapNode* node = checkNode(src, srcTermId);
    if (node == NULL) {
      node = checkNode(tgt, srcTermId);
      if (node == NULL)
        continue;
    }

    // uint jid= rseg->getShapeId();

    debugPrint(dstNet->getImpl()->getLogger(),
               utl::ODB,
               "FLATTEN",
               3,
               "\tadjustParentNode rseg{} J{} {} {} srcTermId={}",
               rseg->getId(),
               jid,
               src->getId(),
               tgt->getId(),
               srcTermId);

    node->resetITermFlag();
    node->setInternalFlag();

    node->setNode(jid);

    return node->getId();
  }
  return 0;
}

void dbFlatten::createTop1stRseg(dbNet* /* unused: src */, dbNet* dst)
{
  if (dst->getWire() != NULL)
    return;

  dbCapNode* cap = dbCapNode::create(dst, 0, /*_foreign*/ false);
  // cap->setNode(iterm->getId());
  cap->setInternalFlag();
  dbRSeg* rc = dbRSeg::create(dst, 0, 0, 0, true);
  rc->setTargetNode(cap->getId());
}

uint dbFlatten::createCapNodes(dbNet* src, dbNet* dst, bool noDstWires)
{
  _shape_rc_map.clear();
  mapOld2newIds(dst->getWire(), NULL);

  if (noDstWires)
    createTop1stRseg(src, dst);

  // uint maxCap= dst->maxInternalCapNum()+1;

  debugPrint(src->getImpl()->getLogger(),
             utl::ODB,
             "FLATTEN",
             3,
             "\tCapNodes: {} {}",
             src->getConstName(),
             dst->getConstName());

  uint gCnt = 0;
  dbSet<dbCapNode> capNodes = src->getCapNodes();
  dbSet<dbCapNode>::iterator cap_node_itr = capNodes.begin();
  for (; cap_node_itr != capNodes.end(); ++cap_node_itr) {
    dbCapNode* node = *cap_node_itr;

    gCnt += createParentCapNode(node, dst);
  }
  return gCnt;
}

uint dbFlatten::setCorrectRsegIds(dbNet* dst)
{
  dbWire* wire = dst->getWire();

  dbSet<dbRSeg> rsegs = dst->getRSegs();

  uint rCnt = 0;
  dbSet<dbRSeg>::iterator rseg_itr = rsegs.begin();
  for (; rseg_itr != rsegs.end(); ++rseg_itr) {
    dbRSeg* rseg = *rseg_itr;

    uint sid = rseg->getShapeId();
    if (sid == 0) {
      dst->getImpl()->getLogger()->warn(utl::ODB,
                                        27,
                                        "rsegId {} has zero shape : {}",
                                        rseg->getId(),
                                        dst->getConstName());
      continue;
    }
    if (!rseg->getTargetCapNode()->isInternal())
      continue;

    int rsegId;
    wire->getProperty(sid, rsegId);
    if (rsegId > 0)
      continue;
    rCnt++;
    // notice(0, "old %d new %d\n", rsegId, rseg->getId());
    wire->setProperty(sid, rseg->getId());
  }
  return rCnt;
}
uint dbFlatten::createRSegs(dbNet* src, dbNet* dst)
{
  debugPrint(src->getImpl()->getLogger(),
             utl::ODB,
             "FLATTEN",
             18,
             "\tRSegs: {} {}",
             src->getConstName(),
             dst->getConstName());

  dbBlock* block = dst->getBlock();
  // extMain::printRSegs(parentNet);

  dbSet<dbRSeg> rsegs = src->getRSegs();

  uint rCnt = 0;
  dbSet<dbRSeg>::iterator rseg_itr = rsegs.begin();
  for (; rseg_itr != rsegs.end(); ++rseg_itr) {
    dbRSeg* rseg = *rseg_itr;

    // int x, y;
    // TODO rseg->getCoords(x, y);
    // TODO uint pathDir= rseg->pathLowToHigh() ? 0 : 1;
    dbRSeg* rc = dbRSeg::create(dst, 0, 0, 0, true);

    uint tgtId = rseg->getTargetNode();
    uint srcId = rseg->getSourceNode();

    rc->setSourceNode(_node_map[srcId]);
    rc->setTargetNode(_node_map[tgtId]);

    for (int corner = 0; corner < block->getCornerCount(); corner++) {
      double res = rseg->getResistance(corner);
      double cap = rseg->getCapacitance(corner);

      rc->setResistance(res, corner);
      rc->setCapacitance(cap, corner);
      debugPrint(src->getImpl()->getLogger(),
                 utl::ODB,
                 "FLATTEN",
                 18,
                 "\t\tsrc:{}->{} - tgt:{}->{} - {}  {}",
                 srcId,
                 _node_map[srcId],
                 tgtId,
                 _node_map[tgtId],
                 res,
                 cap);
    }
    rCnt++;
  }
  setCorrectRsegIds(dst);
  return rCnt;
}
uint dbFlatten::printRSegs(FILE* fp, dbNet* net)
{
  if (fp == NULL)
    net->getImpl()->getLogger()->info(
        utl::ODB, 28, "\t\t\tprintRSegs: {}", net->getConstName());
  else
    fprintf(fp, "\t\t\tprintRSegs: %s\n", net->getConstName());

  dbSet<dbRSeg> rsegs = net->getRSegs();

  uint rCnt = 0;
  dbSet<dbRSeg>::iterator rseg_itr = rsegs.begin();
  for (; rseg_itr != rsegs.end(); ++rseg_itr) {
    dbRSeg* rseg = *rseg_itr;

    if (fp == NULL)
      net->getImpl()->getLogger()->info(utl::ODB,
                                        29,
                                        "\t\t\t\t\trsegId: {} J{} -- ",
                                        rseg->getId(),
                                        rseg->getShapeId());
    else
      fprintf(fp,
              "\t\t\t\t\trsegId: %d J%d -- ",
              rseg->getId(),
              rseg->getShapeId());

    dbCapNode* src = rseg->getSourceCapNode();
    if (fp == NULL) {
      if (src == NULL)
        net->getImpl()->getLogger()->info(utl::ODB, 30, " 0 --");
      else
        net->getImpl()->getLogger()->info(utl::ODB,
                                          31,
                                          " {} I{} B{} -- ",
                                          src->getNode(),
                                          src->isITerm(),
                                          src->isBTerm());
    } else {
      if (src == NULL)
        fprintf(fp, " 0 --");
      else
        fprintf(fp,
                " %d I%d B%d -- ",
                src->getNode(),
                src->isITerm(),
                src->isBTerm());
    }
    dbCapNode* tgt = rseg->getTargetCapNode();
    if (fp == NULL) {
      if (tgt == NULL)
        net->getImpl()->getLogger()->info(utl::ODB, 32, " 0");
      else
        net->getImpl()->getLogger()->info(utl::ODB,
                                          33,
                                          " {} I{} B{}",
                                          tgt->getNode(),
                                          tgt->isITerm(),
                                          tgt->isBTerm());

    } else {
      if (tgt == NULL)
        fprintf(fp, " 0\n");
      else
        fprintf(fp,
                " %d I%d B%d\n",
                tgt->getNode(),
                tgt->isITerm(),
                tgt->isBTerm());
    }
    rCnt++;
  }
  return rCnt;
}
}  // namespace odb
