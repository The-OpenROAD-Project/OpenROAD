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
#include <dbRtTree.h>
#include <dbShape.h>
#include <dbWireCodec.h>
#include <stdio.h>

#include <algorithm>

#include "db.h"
#include "dbSearch.h"
#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "rcx/exttree.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

void extMain::setPowerExtOptions(bool skip_power_stubs,
                                 const char* exclude_cells,
                                 bool skip_m1_caps,
                                 const char* source_file)
{
  _power_extract_only = true;
  _skip_power_stubs = skip_power_stubs;
  _power_exclude_cell_list = exclude_cells;
  _skip_m1_caps = skip_m1_caps;
  _power_source_file = NULL;
  if (source_file != NULL)
    _power_source_file = strdup(source_file);
}
bool extMain::markExcludedCells()
{
  if (!_power_exclude_cell_list || _power_exclude_cell_list[0] == '\0')
    return false;

  std::vector<odb::dbMaster*> masters;
  if (!_block->findSomeMaster(_power_exclude_cell_list, masters))
    return false;

  uint cnt = masters.size();
  for (uint ii = 0; ii < cnt; ii++) {
    odb::dbMaster* m = masters[ii];
    m->setSpecialPower(true);
    logger_->info(
        RCX, 299, "Marked Master {} as special power cell", m->getConstName());
  }

  return true;
}

float extMain::getPowerViaRes(odb::dbBox* v, float val)
{
  bool dbg = false;
  if (dbg)
    writeViaName(stdout, v, 0, "\n");
  uint cutCount = 0;
  float res = val;
  uint level;
  uint width;
  odb::dbTechVia* tvia = v->getTechVia();
  if (tvia != NULL) {
    res = tvia->getResistance();
    if (dbg)
      fprintf(stdout, "\tDB Tech Via %s Res = %g\n", tvia->getConstName(), res);
    if (res <= 0.0) {
      res = computeViaResistance(v, cutCount);
      if (dbg)
        fprintf(stdout, "\tComputed Tech Via Res = %g\n", res);
      if (res <= 0.0) {
        level = tvia->getBottomLayer()->getRoutingLevel();
        width = tvia->getBottomLayer()->getWidth();
        res = getResistance(level, width, width, 0);
        if (cutCount > 0)
          res /= cutCount;
        if (dbg)
          fprintf(stdout, "\tSquare Via Res = %g\n", res);
      }
    }
  } else {
    odb::dbVia* bvia = v->getBlockVia();
    res = computeViaResistance(v, cutCount);
    if (dbg)
      fprintf(stdout,
              "\tComputed Block Via %s Res = %g\n",
              bvia->getConstName(),
              res);
    if (res <= 0.0) {
      level = bvia->getBottomLayer()->getRoutingLevel();
      width = bvia->getBottomLayer()->getWidth();
      res = getResistance(level, width, width, 0);
      if (cutCount > 0)
        res /= cutCount;
      if (dbg)
        fprintf(stdout, "\tSquare Block Via Res = %g\n", res);
    }
  }
  return res;
}
/*
class sortInst_y
{
public:
    bool operator()( odb::dbInst *i1, odb::dbInst *i2)
    {
        odb::dbBox *bb1= i1->getBBox();
        odb::dbBox *bb2= i2->getBBox();
        if ( bb1->yMin() != bb2->xMin())
                return bb1->xMin() < bb2->xMin();
        return bb1->xMax() < bb2->xMax();
    }
};
*/
class sortInst_y
{
 public:
  bool operator()(odb::dbInst* i1, odb::dbInst* i2)
  {
    odb::dbBox* bb1 = i1->getBBox();
    odb::dbBox* bb2 = i2->getBBox();
    return bb1->yMin() < bb2->yMin();
  }
};
/*
class sortInst_x
{
public:
    bool operator()( odb::dbInst *i1, odb::dbInst *i2)
    {
        odb::dbBox *bb1= i1->getBBox();
        odb::dbBox *bb2= i2->getBBox();
        //if ( bb1->xMin() != bb2->yMin())
                return bb1->yMin() < bb2->yMin();
        //return bb1->yMax() < bb2->yMax();
    }
};
*/
class sortInst_x
{
 public:
  bool operator()(odb::dbInst* i1, odb::dbInst* i2)
  {
    odb::dbBox* bb1 = i1->getBBox();
    odb::dbBox* bb2 = i2->getBBox();
    return bb1->xMin() < bb2->xMin();
  }
};
// static void print_wire( odb::dbWire * wire );
void extMain::writeViaRC(FILE* fp,
                         uint level,
                         odb::dbTechLayer* layer,
                         odb::dbBox* v,
                         odb::dbBox* w)
{
  uint top;
  uint bot = _block->getSearchDb()->getViaLevels(v, top);

  fprintf(fp, "R_%d_%d_v%d ", v->xMin(), v->yMin(), v->getId());
  writeViaName(fp, v, bot, " ");

  level = _block->getSearchDb()->getViaLevels(w, top);
  writeViaName(fp, w, top, " ");
  float res = getPowerViaRes(v, 0.1);
  fprintf(fp, " %g\n", res);

  _stackedViaResCnt++;
}
void extMain::writeResNode(char* nodeName, odb::dbCapNode* capNode, uint level)
{
  if (!_wireInfra)
    return writeResNodeRC(nodeName, capNode, level);
  if (capNode->isITerm()) {
    sprintf(nodeName, "I%d", capNode->getNode());
  } else if (capNode->isInternal()) {
    if (_junct2viaMap->geti(capNode->getNode()) == 0)
      // sprintf(nodeName, "n%d", capNode->getId());
      writeInternalNode_xy(capNode, nodeName);
    else {
      int vid = _junct2viaMap->geti(capNode->getNode());

      odb::dbBox* v = NULL;
      if (vid > 0) {
        v = odb::dbSBox::getSBox(_block, vid);

        odb::dbStringProperty* p = odb::dbStringProperty::find(v, "_inode");
        if (p != NULL)
          sprintf(nodeName, "%s_%d", p->getValue().c_str(), level);
        else
          writeViaName(nodeName, v, level, " ");
      } else {
        v = odb::dbBox::getBox(_tech, -vid);
        char* name = getPowerSourceName(level, -vid);
        sprintf(nodeName, "%s ", name);
      }
      if (level > 0)
        _via2JunctionMap->set(v->getId(), capNode->getId());
    }
  }
}
void extMain::writeNegativeCoords(char* buf,
                                  int netId,
                                  int x,
                                  int y,
                                  int level,
                                  const char* post)
{
  const char* mx = "";
  if (x < 0) {
    mx = "m";
    x = -x;
  }
  const char* my = "";
  if (y < 0) {
    my = "m";
    y = -y;
  }
  if (level < 0)
    sprintf(buf, "%sn%d_%s%d_%s%d", _node_blk_prefix, netId, mx, x, my, y);
  else
    sprintf(buf,
            "%sn%d_%s%d_%s%d_%d%s",
            _node_blk_prefix,
            netId,
            mx,
            x,
            my,
            y,
            level,
            post);
}
void extMain::createNode_xy(odb::dbCapNode* capNode,
                            int x,
                            int y,
                            int level,
                            odb::dbITerm* iterm)
{
  if (_junct2viaMap->geti(capNode->getNode()) != 0)
    return;
  if (capNode->isName())
    return;

  int netId = _netUtil->getCurrentNet()->getId();

  // HERE NODE_NAMING
  /*
          char *mx= "";
          if (x<0)
          {
                  mx="m";
                  x= -x;
          }
          char *my= "";
          if (y<0)
          {
                  my="m";
                  y= -y;
          }
          char buf[2048];
          sprintf(buf, "%sn%d_%s%d_%s%d_%d", _node_blk_prefix, netId, mx, x, my,
     y, level);
  */
  char buf[2048];
  writeNegativeCoords(buf, netId, x, y, level);

  odb::dbStringProperty::create(capNode, "_inode", buf);
  capNode->setNameFlag();
  if (iterm == NULL)
    return;

  odb::dbStringProperty* p = odb::dbStringProperty::find(iterm, "_inode");
  if (p != NULL) {
    odb::dbStringProperty::destroy(p);
    sprintf(buf, "%s %s", buf, p->getValue().c_str());
  }
  odb::dbStringProperty::create(iterm, "_inode", buf);
}
void extMain::writeInternalNode_xy(odb::dbCapNode* capNode, char* buff)
{
  if (!capNode->isName()) {
    sprintf(buff, " n%d ", capNode->getId());
  } else {
    odb::dbStringProperty* p = odb::dbStringProperty::find(capNode, "_inode");
    sprintf(buff, " %s ", p->getValue().c_str());
  }
}
void extMain::writeInternalNode_xy(odb::dbCapNode* capNode, FILE* fp)
{
  if (!capNode->isName()) {
    fprintf(fp, " n%d ", capNode->getId());
  } else {
    odb::dbStringProperty* p = odb::dbStringProperty::find(capNode, "_inode");
    fprintf(fp, " %s ", p->getValue().c_str());
  }
}
void extMain::writeResNode(FILE* fp, odb::dbCapNode* capNode, uint level)
{
  if (capNode->isITerm()) {
    fprintf(fp, " I%d ", capNode->getNode());
  } else if (capNode->isInternal()) {
    if (_junct2viaMap->geti(capNode->getNode()) == 0)
      // fprintf(fp, " n%d ", capNode->getId());
      writeInternalNode_xy(capNode, fp);
    else {
      int vid = _junct2viaMap->geti(capNode->getNode());
      odb::dbBox* v = NULL;
      if (vid > 0) {
        v = odb::dbSBox::getSBox(_block, vid);

        odb::dbStringProperty* p = odb::dbStringProperty::find(v, "_inode");
        if (p != NULL)
          fprintf(fp, "%s_%d ", p->getValue().c_str(), level);
        else
          writeViaName(fp, v, level, " ");
      } else {
        v = odb::dbBox::getBox(_tech, -vid);
        char* name = getPowerSourceName(level, -vid);
        fprintf(fp, "%s ", name);
        // sprintf(nodeName, "%s", name);
      }
      if (level > 0)
        _via2JunctionMap->set(v->getId(), capNode->getId());
    }
  }
}
void extMain::writeResNodeRC(char* capNodeName,
                             odb::dbCapNode* capNode,
                             uint level)
{
  if (capNode->isITerm()) {
    sprintf(capNodeName, " I%d ", capNode->getNode());
  } else if (capNode->isInternal()) {
    writeInternalNode_xy(capNode, capNodeName);
    uint vid = capNode->getNode();
    if (vid > 0) {  // viaid= nodeId
      if (level > 0)
        _via2JunctionMap->set(vid, capNode->getId());
    }
  }
}
void extMain::writeResNodeRC(FILE* fp, odb::dbCapNode* capNode, uint level)
{
  if (capNode->isITerm()) {
    // COMMENT 032613 -- use property to match via coords
    odb::dbITerm* iterm = odb::dbITerm::getITerm(_block, capNode->getNode());
    odb::dbStringProperty* p = odb::dbStringProperty::find(capNode, "_inode");
    if (p != NULL)
      fprintf(fp, " %s ", p->getValue().c_str());
  } else if (capNode->isInternal()) {
    writeInternalNode_xy(capNode, fp);
    uint vid = capNode->getNode();
    if (vid > 0) {  // viaid= nodeId
      if (level > 0)
        _via2JunctionMap->set(vid, capNode->getId());
    }
  }
}
void extMain::writeOneCapNode(FILE* fp,
                              odb::dbCapNode* capNode,
                              uint level,
                              bool onlyVias)
{
  if (capNode->isITerm()) {
    if (capNode->getNet()->isMarked())  // VDD
      _vddItermIdTable.push_back(capNode->getNode());
    else
      _gndItermIdTable.push_back(capNode->getNode());
  }
  // float cap= 1.0e-15;
  // float cap = capNode->getCapacitance();
  float cap = 1.0e-15 * _capNode_map[capNode->getId()];
  char capNodeName[128];
  if (!onlyVias) {
    writeResNodeRC(capNodeName, capNode, level);
    fprintf(fp, "C_0_%d %s 0 %g\n", capNode->getId(), capNodeName, cap);
    writeSubcktNode(capNodeName, level > 1, capNode->getNet()->isMarked());
  } else if (capNode->isInternal()) {
    // if (_junct2viaMap->geti(capNode->getNode())==0)
    // continue;

    writeResNodeRC(capNodeName, capNode, level);
    fprintf(fp, "C_0_%d %s 0 %g\n", capNode->getId(), capNodeName, cap);
    uint vid = capNode->getNode();
    if (vid > 0)
      // if (_junct2viaMap->geti(capNode->getNode())>0)
      writeSubcktNode(capNodeName, level > 1, capNode->getNet()->isMarked());
  }
}
void extMain::writeCapNodesRC(FILE* fp,
                              odb::dbNet* net,
                              uint level,
                              bool onlyVias,
                              std::vector<odb::dbCapNode*>& capNodeTable)
{
  for (uint ii = 0; ii < capNodeTable.size(); ii++) {
    odb::dbCapNode* capNode = capNodeTable[ii];
    writeOneCapNode(fp, capNode, level, onlyVias);
  }
}
void extMain::writeCapNodesRC(FILE* fp,
                              odb::dbNet* net,
                              uint level,
                              bool onlyVias,
                              bool skipFirst)
{
  odb::dbSet<odb::dbCapNode> cSet = net->getCapNodes();

  uint setCnt = cSet.size();
  if (setCnt == 0)
    return;
  cSet.reverse();
  odb::dbSet<odb::dbCapNode>::iterator rc_itr;

  uint cnt = 0;
  rc_itr = cSet.begin();
  if (skipFirst) {
    if (setCnt > 3) {
      cnt++;
      ++rc_itr;
    } else
      cnt--;
  }
  for (; rc_itr != cSet.end(); ++rc_itr) {
    odb::dbCapNode* capNode = *rc_itr;
    cnt++;
    if (cnt == setCnt - 1)
      continue;
    if (capNode->isITerm()) {
      if (capNode->getNet()->isMarked())  // VDD
        _vddItermIdTable.push_back(capNode->getNode());
      else
        _gndItermIdTable.push_back(capNode->getNode());
    }
    // float cap= 1.0e-15;
    // float cap = capNode->getCapacitance();
    float cap = 1.0e-15 * _capNode_map[capNode->getId()];
    char capNodeName[128];
    if (!onlyVias) {
      writeResNodeRC(capNodeName, capNode, level);
      fprintf(fp, "C_0_%d %s 0 %g\n", capNode->getId(), capNodeName, cap);
      writeSubcktNode(capNodeName, level > 1, capNode->getNet()->isMarked());
    } else if (capNode->isInternal()) {
      // if (_junct2viaMap->geti(capNode->getNode())==0)
      // continue;

      writeResNodeRC(capNodeName, capNode, level);
      fprintf(fp, "C_0_%d %s 0 %g\n", capNode->getId(), capNodeName, cap);
      uint vid = capNode->getNode();
      if (vid > 0)
        // if (_junct2viaMap->geti(capNode->getNode())>0)
        writeSubcktNode(capNodeName, level > 1, capNode->getNet()->isMarked());
    }
  }
}
void extMain::writeCapNodes(FILE* fp,
                            odb::dbNet* net,
                            uint level,
                            bool onlyVias,
                            bool skipFirst)
{
  odb::dbSet<odb::dbCapNode> cSet = net->getCapNodes();
  cSet.reverse();
  odb::dbSet<odb::dbCapNode>::iterator rc_itr;
  // Cpn_0_vM2_1     vM2_1 0 5.41778e-16
  rc_itr = cSet.begin();
  if (skipFirst)
    ++rc_itr;
  for (; rc_itr != cSet.end(); ++rc_itr) {
    odb::dbCapNode* capNode = *rc_itr;
    if (capNode->isITerm()) {
      if (capNode->getNet()->isMarked())  // VDD
        _vddItermIdTable.push_back(capNode->getNode());
      else
        _gndItermIdTable.push_back(capNode->getNode());
    }
    // float cap= 1.0e-15;
    // float cap = capNode->getCapacitance();
    float cap = 1.0e-15 * _capNode_map[capNode->getId()];
    char capNodeName[128];
    if (!onlyVias) {
      writeResNode(capNodeName, capNode, level);
      fprintf(fp, "Cpn_0_%d %s 0 %g\n", capNode->getId(), capNodeName, cap);
      writeSubcktNode(capNodeName, level > 1, capNode->getNet()->isMarked());
    } else if (capNode->isInternal()) {
      // if (_junct2viaMap->geti(capNode->getNode())==0)
      // continue;

      writeResNode(capNodeName, capNode, level);
      fprintf(fp, "Cpn_0_%d %s 0 %g\n", capNode->getId(), capNodeName, cap);
      if (_junct2viaMap->geti(capNode->getNode()) > 0)
        writeSubcktNode(capNodeName, level > 1, capNode->getNet()->isMarked());
    }
  }
}
void extMain::writeSubcktNode(char* capNodeName, bool highMetal, bool vdd)
{
  // vdd=true
  FILE* fp = _subCktNodeFP[highMetal][vdd];

  if (++_subCktNodeCnt[vdd][highMetal] % 8 == 0)
    fprintf(fp, " \\\n");

  fprintf(fp, " %s", capNodeName);
}
float extMain::micronCoords(int xy)
{
  float v = 1.0 * xy;
  v /= _block->getDbUnitsPerMicron();

  return v;
}
float extMain::distributeCap(FILE* fp, odb::dbNet* net)
{
  odb::dbSet<odb::dbCapNode> cSet = net->getCapNodes();
  odb::dbSet<odb::dbCapNode>::iterator c_itr;
  for (c_itr = cSet.begin(); c_itr != cSet.end(); ++c_itr) {
    odb::dbCapNode* capNode = *c_itr;
    _capNode_map[capNode->getId()] = 0.0;
  }
  float totCap = 0.0;

  odb::dbSet<odb::dbRSeg> rSet = net->getRSegs();
  odb::dbSet<odb::dbRSeg>::iterator rc_itr;

  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    odb::dbRSeg* rc = *rc_itr;
    float cap = rc->getCapacitance(0);
    // rc->getShapeId(), cap);

    odb::dbCapNode* src = rc->getSourceCapNode();
    odb::dbCapNode* tgt = rc->getTargetCapNode();
    /*
                    float c = _capNode_map[src->getId()];
                    _capNode_map[src->getId()]= c+cap/2;

                    c = _capNode_map[tgt->getId()];
                    _capNode_map[tgt->getId()]= c+cap/2;
    */
    _capNode_map[src->getId()] += cap / 2;
    _capNode_map[tgt->getId()] += cap / 2;

    if (fp != NULL)
      fprintf(fp,
              "R_%d_%d_s%d %g %g %g\n",
              net->getId(),
              rc->getId(),
              rc->getShapeId(),
              cap,
              _capNode_map[src->getId()],
              _capNode_map[tgt->getId()]);

    totCap += cap;
  }
  return totCap;
}
uint extMain::setNodeCoords_xy(odb::dbNet* net, int level)
{
  uint maxJunctionId = 0;
  bool RCflag = true;

  if (_dbgPowerFlow)
    logger_->info(RCX, 300, "setNodeCoords_xy Net= {}", net->getConstName());
  odb::dbWire* wire = net->getWire();

  if (!RCflag) {
    odb::dbWireShapeItr shapes;
    odb::dbShape s;
    for (shapes.begin(wire); shapes.next(s);) {
      uint sid = shapes.getShapeId();
      int vid;
      wire->getProperty(sid, vid);
      // if (_dbgPowerFlow)
      logger_->info(RCX,
                    301,
                    "sID= {} RC{}  {} {} {} {}",
                    sid,
                    vid,
                    s.xMin(),
                    s.yMin(),
                    s.xMax(),
                    s.yMax());
      if (vid == 0)
        continue;
      odb::dbRSeg* rc = odb::dbRSeg::getRSeg(net->getBlock(), vid);
      odb::dbCapNode* src = rc->getSourceCapNode();
      odb::dbCapNode* tgt = rc->getTargetCapNode();
      createNode_xy(src, s.xMin(), s.yMin(), level);
      createNode_xy(tgt, s.xMax(), s.yMax(), level);
    }
    if (_dbgPowerFlow)
      logger_->info(RCX, 302, "End Net= {}", net->getConstName());
    return 0;
  }
  odb::dbSet<odb::dbRSeg> rSet = net->getRSegs();
  odb::dbSet<odb::dbRSeg>::iterator rc_itr;

  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    odb::dbRSeg* rc = *rc_itr;
    if (rc->getShapeId() == 0) {
      odb::dbCapNode* src = rc->getSourceCapNode();
      odb::dbCapNode* tgt = rc->getTargetCapNode();
      continue;
    }
    int x = 0;
    int y = 0;
    rc->getCoords(x, y);
    odb::dbShape s;
    wire->getShape(rc->getShapeId(), s);

    odb::dbCapNode* src = rc->getSourceCapNode();
    if (!src->isName()) {
      // createNode_xy(src, x, y, level);
      createNode_xy(src, s.xMin(), s.yMin(), level);
    }

    char buff[64];
    writeInternalNode_xy(src, buff);

    odb::dbCapNode* tgt = rc->getTargetCapNode();

    if (!tgt->isInternal())
      continue;

    if (maxJunctionId < rc->getShapeId())
      maxJunctionId = rc->getShapeId();
    odb::dbITerm* iterm = _junct2iterm->geti(rc->getShapeId());
    // 102912 createNode_xy(tgt, s.xMax(), s.yMax(), level, iterm);
    createNode_xy(tgt, x, y, level, iterm);
    writeInternalNode_xy(tgt, buff);
  }
  return maxJunctionId;
}

double extMain::writeRes(FILE* fp,
                         odb::dbNet* net,
                         uint level,
                         uint width,
                         uint dir,
                         bool skipFirst)
{
  double totRes = 0.0;

  if ((level > 1) && (_globGeom != NULL)) {
    fprintf(_globGeom, "prop netName %s endprop\n", net->getConstName());
  }

  odb::dbSet<odb::dbRSeg> rSet = net->getRSegs();
  odb::dbSet<odb::dbRSeg>::iterator rc_itr;

  // 102912 for( rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr )
  rc_itr = rSet.begin();
  if (skipFirst)
    ++rc_itr;
  for (; rc_itr != rSet.end(); ++rc_itr) {
    odb::dbRSeg* rc = *rc_itr;
    // fprintf(fp, "R_%s_%d_s%d", net->getConstName(), rc->getId(),
    // rc->getShapeId());
    fprintf(fp, "R_%d_%d_s%d ", net->getId(), rc->getId(), rc->getShapeId());

    odb::dbCapNode* src = rc->getSourceCapNode();
    odb::dbCapNode* tgt = rc->getTargetCapNode();
    writeResNode(fp, src, level);
    writeResNode(fp, tgt, level);

    // double res= _resFactor * rc->getResistance(0);
    double res = rc->getResistance(0);
    totRes += res;
    fprintf(fp, " %g   ", res);
    fprintf(fp, " dtemp=0 tc1=0.00265     tc2=-2.641e-07\n");

    // if ((level>1) && (_globGeom!=NULL) && (rc->getShapeId()>0))
    if ((level > 1) && (_globGeom != NULL)) {
      odb::dbWire* wire = net->getWire();
      odb::dbShape s;
      wire->getShape(rc->getShapeId(), s);

      char srcNode[128];
      char dstNode[128];
      writeResNode(srcNode, src, level);
      writeResNode(dstNode, tgt, level);

      float W = micronCoords(width);
      fprintf(_globGeom, "path Metal%d %g %s %s ", level, W, srcNode, dstNode);
      fprintf(_globGeom,
              "((%g %g) ",
              micronCoords(s.xMin()),
              micronCoords(s.yMin()));
      if (dir > 0) {
        fprintf(_globGeom,
                " (%g %g)) endpath\n",
                micronCoords(s.xMax()),
                micronCoords(s.yMin()));
      } else {
        fprintf(_globGeom,
                " (%g %g)) endpath\n",
                micronCoords(s.xMin()),
                micronCoords(s.yMax()));
      }
    }
  }
  if (level == 1)
    return totRes;
  return totRes;
}
// COMMENT -- 032913
void extMain::replaceItermCoords(odb::dbNet* net, uint dir, int xy[2])
{
  odb::dbSet<odb::dbCapNode> cSet = net->getCapNodes();
  odb::dbSet<odb::dbCapNode>::iterator rc_itr = cSet.begin();
  for (; rc_itr != cSet.end(); ++rc_itr) {
    odb::dbCapNode* capNode = *rc_itr;

    if (capNode->isInternal())
      continue;
    if (capNode->isITerm()) {
      odb::dbStringProperty* p = odb::dbStringProperty::find(capNode, "_inode");
      if (p == NULL)
        continue;
      char buff[128];
      sprintf(buff, " %s ", p->getValue().c_str());

      Ath__parser parser;

      int n1 = parser.mkWords(buff, "_");
      int ixy[2];

      ixy[0] = parser.getInt(1);
      ixy[1] = parser.getInt(2);
      ixy[dir] = xy[dir];

      char buff2[128];
      sprintf(buff2,
              "%s_%d_%d_%d",
              parser.get(0),
              ixy[0],
              ixy[1],
              parser.getInt(3));

      p->setValue(buff2);

      odb::dbITerm* iterm = odb::dbITerm::getITerm(_block, capNode->getNode());
      odb::dbInst* inst = iterm->getInst();
      odb::dbMaster* master = inst->getMaster();

      fprintf(_blkInfo,
              "%s %s %s [%s] %s \n",
              inst->getConstName(),
              master->getConstName(),
              getBlockType(master),
              iterm->getMTerm()->getConstName(),
              buff2);

      odb::dbBox* bb = inst->getBBox();
      // fprintf(_coordsFP, "std_cell %d %d  %d %d   I%d %s\n",
      fprintf(_coordsFP,
              "%s %d %d  %d %d   %s %s\n",
              getBlockType(master),
              bb->xMin(),
              bb->yMin(),
              bb->getDX(),
              bb->getDY(),
              buff2,
              inst->getConstName());
    }
  }
}
void extMain::findViaMainCoord(odb::dbNet* net, char* buff)
{
  odb::dbSet<odb::dbRSeg> rSet = net->getRSegs();
  odb::dbSet<odb::dbRSeg>::iterator rc_itr;
  uint cnt = 0;
  rc_itr = rSet.begin();
  for (; rc_itr != rSet.end(); ++rc_itr) {
    odb::dbRSeg* rc = *rc_itr;

    odb::dbCapNode* src = rc->getSourceCapNode();
    odb::dbCapNode* tgt = rc->getTargetCapNode();
    if (tgt->isInternal()) {
      odb::dbStringProperty* p = odb::dbStringProperty::find(tgt, "_inode");
      if (p == NULL)
        continue;
      sprintf(buff, " %s ", p->getValue().c_str());
      return;
    }
  }
}
double extMain::writeResRC(FILE* fp,
                           odb::dbNet* net,
                           uint level,
                           uint width,
                           uint dir,
                           bool skipFirst,
                           bool reverse,
                           bool onlyVias,
                           bool writeCapNodes,
                           int xy[2])
{
  if ((level > 1) && (_globGeom != NULL))
    fprintf(_globGeom, "prop netName %s endprop\n", net->getConstName());

  odb::dbSet<odb::dbRSeg> rSet = net->getRSegs();
  uint setCnt = rSet.size();
  if (setCnt == 0)
    return 0;
  if (reverse)
    rSet.reverse();

  odb::dbSet<odb::dbRSeg>::iterator rc_itr;
  uint cnt = 0;
  rc_itr = rSet.begin();
  if (skipFirst) {
    if (setCnt > 1) {
      cnt++;
      ++rc_itr;
    } else
      cnt--;
  }
  uint n = 0;

  double totRes = 0.0;
  for (; rc_itr != rSet.end();) {
    odb::dbRSeg* rc = *rc_itr;

    odb::dbCapNode* src = rc->getSourceCapNode();
    odb::dbCapNode* tgt = rc->getTargetCapNode();

    if (writeCapNodes) {
      if (n == 0)
        writeOneCapNode(fp, src, level, onlyVias);

      writeOneCapNode(fp, tgt, level, onlyVias);
      n++;
    }
    fprintf(fp, "R_%d ", rc->getId());
    writeResNodeRC(fp, src, level);
    writeResNodeRC(fp, tgt, level);

    // double res= _resFactor * rc->getResistance(0);
    double res = rc->getResistance(0);
    totRes += res;
    fprintf(fp, " %g   ", res);
    fprintf(fp, " dtemp=0 tc1=0.00265 tc2=-2.641e-07\n");
    /* TODO
                    if ((level>1) && (_globGeom!=NULL))
                    {
                            odb::dbWire *wire= net->getWire();
                            odb::odb::dbShape s;
                            wire->getShape( rc->getShapeId(), s );

                            char srcNode[128];
                            char dstNode[128];
                            writeResNode(srcNode, src, level);
                            writeResNode(dstNode, tgt, level);

                            float W=micronCoords(width);
                            fprintf(_globGeom,"path Metal%d %g %s %s ", level,
       W,srcNode, dstNode); fprintf(_globGeom, "((%g %g) ",
                                    micronCoords(s.xMin()),
       micronCoords(s.yMin())); if (dir>0)
                            {
                                    fprintf(_globGeom, " (%g %g)) endpath\n",
                                    micronCoords(s.xMax()),
       micronCoords(s.yMin())); } else { fprintf(_globGeom, " (%g %g))
       endpath\n", micronCoords(s.xMin()), micronCoords(s.yMax()));
                            }
                    }
    */
    ++rc_itr;
    if (skipFirst) {
      if (rc_itr != rSet.end())
        continue;
    }
  }
  if (level == 1)
    return totRes;
  return totRes;
}

bool extMain::specialMasterType(odb::dbInst* inst)
{
  odb::dbMaster* master = inst->getMaster();
  if ((master->getType() == odb::dbMasterType::CORE_FEEDTHRU)
      || (master->getType() == odb::dbMasterType::PAD_SPACER))
  //(master->getType()==odb::dbMasterType::NONE))
  {
    return true;
  }
  if (inst->getUserFlag3())
    return true;

  return false;
}

const char* extMain::getBlockType(odb::dbMaster* m)
{
  const char* name = m->getConstName();
  if ((name[0] == 'F') && (name[1] == 'I') && (name[2] == 'L')
      && (name[3] == 'L'))
    return "is_filler_decap";

  if (m->isFiller())
    return "is_filler_decap";
  else
    return "is_std";
}

bool extMain::fisrt_markInst_UserFlag(odb::dbInst* inst, odb::dbNet* net)
{
  if (net->getSigType() == odb::dbSigType::POWER) {
    if (inst->getUserFlag1())
      return false;

    inst->setUserFlag1();
    return true;
  }
  if (net->getSigType() == odb::dbSigType::GROUND) {
    if (inst->getUserFlag2())
      return false;

    inst->setUserFlag2();
    return true;
  }

  throw ZException("Unexpected signal type");
}

odb::dbITerm* extMain::findConnect(odb::dbInst* inst,
                                   odb::dbNet* net,
                                   odb::dbNet* targetNet)
{
  odb::dbITerm* iterm = NULL;

  odb::dbMaster* master = inst->getMaster();
  if (specialMasterType(inst))
    return NULL;

  uint instId = inst->getId();

  odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
  odb::dbSet<odb::dbITerm>::iterator iterm_itr;

  odb::dbITerm* foundIterm = NULL;
  for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr) {
    odb::dbITerm* iterm = *iterm_itr;
    if (iterm->getNet() == NULL)
      continue;
    if (!((iterm->getSigType() == odb::dbSigType::GROUND)
          || (iterm->getSigType() == odb::dbSigType::POWER)))
      continue;

    if (net == iterm->getNet()) {
      if (master->isSpecialPower())
        return NULL;

      if (fisrt_markInst_UserFlag(inst, net)) {
        /* COMMENT -- tranfer to createPowerNode
                                fprintf(_blkInfo, "%s %s %s [%s] I%d\n",
           inst->getConstName(), master->getConstName(), getBlockType(master),
                                        iterm->getMTerm()->getConstName(),
           iterm->getId());

                                odb::dbBox *bb= inst->getBBox();
                                //fprintf(_coordsFP, "std_cell %d %d  %d %d I%d
           %s\n", fprintf(_coordsFP, "%s %d %d  %d %d   I%d %s\n",
                                        getBlockType(master),
                                        bb->xMin(), bb->yMin(),
                                        bb->getDX(), bb->getDY(),
                                        iterm->getId(),
                                        inst->getConstName());
        */
        foundIterm = iterm;
      }

      if (net->getSigType() == odb::dbSigType::POWER)
        inst->setUserFlag1();
      if (net->getSigType() == odb::dbSigType::GROUND)
        inst->setUserFlag2();
    }
  }
  if (foundIterm != NULL) {
    // no TIE lo/hi 3/26/2013  iterm2Vias_cells(inst, foundIterm);
    return foundIterm;
  }
  return NULL;

  odb::dbSet<odb::dbMTerm> mterms = master->getMTerms();
  odb::dbSet<odb::dbMTerm>::iterator itr;

  for (itr = mterms.begin(); itr != mterms.end(); ++itr) {
    odb::dbMTerm* mterm = (odb::dbMTerm*) *itr;
    if (mterm->getSigType() != net->getSigType())
      continue;

    iterm = odb::dbITerm::connect(inst, targetNet, mterm);
    break;
  }
  if (iterm == NULL) {
    logger_->warn(RCX,
                  303,
                  "Cannot connect to Inst: {} of {} for power net {}",
                  inst->getConstName(),
                  master->getConstName(),
                  net->getConstName());
    odb::dbBox* bb = inst->getBBox();
    logger_->warn(RCX,
                  304,
                  "   @ {} {}  {} {}",
                  bb->xMin(),
                  bb->yMin(),
                  bb->getDX(),
                  bb->getDY());
  } else {
    // fprintf(_blkInfo, "%s %s is_std [%s] I%d\n", inst->getConstName(),
    if (!master->isSpecialPower()) {
      if (fisrt_markInst_UserFlag(inst, net)) {
        fprintf(_blkInfo,
                "%s %s %s [%s] I%d\n",
                inst->getConstName(),
                master->getConstName(),
                getBlockType(master),
                iterm->getMTerm()->getConstName(),
                iterm->getId());
        odb::dbBox* bb = inst->getBBox();
        fprintf(_coordsFP,
                "%s %d %d  %d %d   I%d %s\n",
                getBlockType(master),
                bb->xMin(),
                bb->yMin(),
                bb->getDX(),
                bb->getDY(),
                iterm->getId(),
                inst->getConstName());
        if (net->getSigType() == odb::dbSigType::POWER)
          inst->setUserFlag1();
        if (net->getSigType() == odb::dbSigType::GROUND)
          inst->setUserFlag2();
      }
    }
  }
  return iterm;
}

uint extMain::getLayerSearchBoundaries(odb::dbTechLayer* layer,
                                       int* xyLo,
                                       int* xyHi,
                                       uint* pitch)
{
  odb::Rect maxRect;
  _block->getDieArea(maxRect);

  uint width = layer->getWidth();
  uint p = layer->getPitch();
  if (p <= 0)
    logger_->error(RCX, 467, "Layer %s has pitch %u !!\n", layer->getConstName(), *pitch);

  pitch[0] = 0;
  pitch[1] = 0;
  uint dir = 0;
  if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL)
    dir = 1;
  pitch[dir] = p;

  std::vector<int> trackXY(32000);
  odb::dbTrackGrid* tg = _block->findTrackGrid(layer);
  if (tg) {
    tg->getGridX(trackXY);
    xyLo[0] = trackXY[0] - width / 2;
    tg->getGridY(trackXY);
    xyLo[1] = trackXY[0] - width / 2;
  } else {
    xyLo[0] = maxRect.xMin();
    xyLo[1] = maxRect.yMin();
  }
  xyHi[0] = maxRect.xMax();
  xyHi[1] = maxRect.yMax();
  return dir;
}
uint extMain::mergeRails(uint dir,
                         std::vector<odb::dbBox*>& boxTable,
                         std::vector<odb::Rect*>& mergeTable)
{
  uint n = boxTable.size();
  for (uint ii = 0; ii < n; ii++) {
    odb::dbBox* w1 = boxTable[ii];
    if (w1->isVisited())
      continue;
    w1->setVisited(true);
    if (_dbgPowerFlow)
      fprintf(stdout,
              "\t\t\tmerge w1: %d %d %d %d\n",
              w1->xMin(),
              w1->yMin(),
              w1->xMax(),
              w1->yMax());

    odb::Rect* r1
        = new odb::Rect(w1->xMin(), w1->yMin(), w1->xMax(), w1->yMax());
    mergeTable.push_back(r1);
  }
  return mergeTable.size();
}
uint extMain::getITermConn2(uint dir,
                            odb::dbWireEncoder& encoder,
                            odb::dbWire* wire,
                            odb::dbNet* net,
                            int* xy,
                            int* xy2)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();

  int WIDTH2 = (xy2[dir] - xy[dir]);
  // odb::dbTechLayerRule *wRule=_netUtil->getRule(level,WIDTH2);

  std::vector<odb::dbInst*> instTable;
  uint instCnt
      = blkSearch->getInstBoxes(xy[0], xy[1], xy2[0], xy2[1], instTable);
  /*
  if (instCnt>0)
  if (_dbgPowerFlow)
  fprintf(stdout, "\tgetITermConn[%d]: %d %d  %d %d\n", instCnt, xy[0], xy[1],
  xy2[0], xy2[1]);
  */

  int prevX = xy[0];
  int prevY = xy[1];
  sortInst_x sort_by_x;
  std::sort(instTable.begin(), instTable.end(), sort_by_x);
  for (uint jj = 0; jj < instCnt; jj++) {
    odb::dbInst* inst = instTable[jj];
    int instId = inst->getId();
    odb::dbBox* bb = inst->getBBox();
    const char* debug = "";
    if (strcmp(inst->getConstName(), debug) == 0) {
      logger_->info(RCX,
                    401,
                    "inst= {} net={} \tgetITermConn[{}]: {} {}  {} {}",
                    inst->getConstName(),
                    net->getConstName(),
                    instCnt,
                    xy[0],
                    xy[1],
                    xy2[0],
                    xy2[1]);
      logger_->info(RCX,
                    398,
                    "inst BBox= X {} {}  Y {} {}",
                    bb->xMin(),
                    bb->xMax(),
                    bb->yMin(),
                    bb->yMax());
      logger_->info(RCX,
                    395,
                    "instCount={} ----- --------- Prev X {}  Y {} ",
                    instCnt,
                    prevX,
                    prevY);
      for (uint kk = 0; kk < instCnt; kk++) {
        odb::dbInst* ii = instTable[kk];
        int instId = inst->getId();
        odb::dbBox* b = ii->getBBox();
        logger_->info(RCX,
                      389,
                      "I[{}] {:10d} {:10d}  {:10d} {:10d}",
                      kk,
                      b->xMin(),
                      b->yMin(),
                      b->xMax(),
                      b->yMax());
      }
    }
    if ((net->getSigType() == odb::dbSigType::POWER) && inst->getUserFlag1())
      continue;
    if ((net->getSigType() == odb::dbSigType::GROUND) && inst->getUserFlag2())
      continue;

    if (dir > 0) {
      if (bb->xMin() > xy2[0] + WIDTH2 / 2)
        continue;
      if (bb->xMax() < (xy[0] - WIDTH2 / 2))
        continue;
    } else {
      if (bb->yMin() > xy2[1] + WIDTH2 / 2)
        continue;
      if (bb->yMax() < xy[1] - WIDTH2 / 2)
        continue;
    }
    int bbXY[2] = {bb->xMin(), bb->yMin()};
    int bbXY2[2] = {bb->xMax(), bb->yMax()};
    /*
                    if (dir>0)
                            bbXY[!dir]= (bb->xMin()+bb->xMax())/2;
                    else
                            bbXY[!dir]= (bb->yMin()+bb->yMax())/2;
    */

    int n = !dir;
    if (!(((bbXY[n] >= xy[n]) && (bbXY[n] < xy2[n]))
          || ((bbXY2[n] >= xy[n]) && (bbXY2[n] < xy2[n]))))
      continue;
    odb::dbITerm* iterm = findConnect(inst, net, wire->getNet());
    if (strcmp(inst->getConstName(), debug) == 0)
      fprintf(stdout, "\t Before Connection\n");
    if (iterm == NULL) {
      continue;
    }
    if (strcmp(inst->getConstName(), debug) == 0)
      fprintf(stdout, "\t\t Connected!!!\n");

    int ixy[2];
    ixy[0] = xy[0];
    ixy[1] = xy[1];
    if (dir > 0)
      ixy[!dir] = bb->xMin();
    else
      ixy[!dir] = bb->yMin();
    int BB[2];
    if (dir > 0)
      BB[0] = (bb->xMin() + bb->xMax()) / 2;
    else
      BB[1] = (bb->yMin() + bb->yMax()) / 2;

    sameJunctionPoint(ixy, BB, WIDTH2, dir);

    uint jid = encoder.addPoint(ixy[0], ixy[1]);
    encoder.addITerm(iterm);

    xy[!dir] = ixy[!dir];
  }
  uint cnt = 0;
  for (uint jj = 0; jj < instCnt; jj++) {
    if (cnt > 4)
      break;
    odb::dbInst* inst = instTable[jj];
    if (specialMasterType(inst))
      continue;
    if (inst->getMaster()->isSpecialPower())
      continue;

    if (((net->getSigType() == odb::dbSigType::POWER) && !inst->getUserFlag1())
        || ((net->getSigType() == odb::dbSigType::GROUND)
            && !inst->getUserFlag2())) {
      odb::dbBox* bb = inst->getBBox();
      cnt++;
    }
    // inst->clearUserFlag1();
    // inst->clearUserFlag2();
  }
  // if (cnt>0)
  // exit(0);

  return instCnt;
}
//#define DEBUG_INST_NAME "ledc_3.OPTHOLD_G_11905"
odb::dbCapNode* extMain::getITermConnRC(odb::dbCapNode* srcCapNode,
                                        uint level,
                                        uint dir,
                                        odb::dbNet* net,
                                        int* xy,
                                        int* xy2)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();

  int WIDTH2 = (xy2[dir] - xy[dir]);
  // odb::dbTechLayerRule *wRule=_netUtil->getRule(level,WIDTH2);

  std::vector<odb::dbInst*> instTable;
  uint instCnt
      = blkSearch->getInstBoxes(xy[0], xy[1], xy2[0], xy2[1], instTable);

  int prevX = xy[0];
  int prevY = xy[1];
  sortInst_x sort_by_x;
  std::sort(instTable.begin(), instTable.end(), sort_by_x);

  std::vector<odb::Rect> rectTable;

  for (uint kk = 0; kk < instCnt; kk++) {
    odb::dbInst* inst = instTable[kk];
    odb::dbBox* b = inst->getBBox();
    odb::Rect rb;
    b->getBox(rb);
    rectTable.push_back(rb);

#ifdef DEBUG_INST_NAME
    if (strcmp(inst->getConstName(), DEBUG_INST_NAME) == 0) {
      logger_->info(RCX,
                    403,
                    "inst= {} net={} \tgetITermConn[{}]: {} {}  {} {}",
                    inst->getConstName(),
                    net->getConstName(),
                    instCnt,
                    xy[0],
                    xy[1],
                    xy2[0],
                    xy2[1]);
      logger_->info(RCX,
                    399,
                    "inst BBox= X {} {}  Y {} {}",
                    rb.xMin(),
                    rb.xMax(),
                    rb.yMin(),
                    rb.yMax());
      logger_->info(RCX,
                    396,
                    "instCount={} ----- --------- Prev X {}  Y {} ",
                    instCnt,
                    prevX,
                    prevY);
      for (uint kk = 0; kk < instCnt; kk++) {
        odb::dbInst* ii = instTable[kk];
        int instId = inst->getId();
        odb::dbBox* b = ii->getBBox();
        logger_->info(RCX,
                      390,
                      "I[{}] {:10d} {:10d}  {:10d} {:10d}",
                      kk,
                      b->xMin(),
                      b->yMin(),
                      b->xMax(),
                      b->yMax());
      }
    }
#endif
  }
  /*
          for (uint jj= 0; jj<instCnt; jj++)
          {
                  odb::dbInst *inst= instTable[jj];
                  int instId= inst->getId();
                  odb::dbBox *b= inst->getBBox();
                  odb::Rect rb= rectTable[jj];

                  if (jj<instCnt-1)
                  {
                          odb::Rect rc= rectTable[jj+1];
                          if (rb.xMax()>rc.xMin()) // overallping
                          {
                                  if (rb.xMax()>rc.xMax()) // switch inndices
                                  {
                                          rb.set_xlo(rc.xMax());

                                          rectTable[jj]= rc;
                                          rectTable[jj+1]= rb;

                                          instTable[jj]= instTable[jj+1];
                                          instTable[jj+1]= inst;
                                  }
                                  else if (rb.xMax()<rc.xMax())
                                  {
                                          rc.set_xlo(rb.xMax());
                                  }
                                  else
                                  {
                                          rc.set_xlo( rc.xMin() + WIDTH2/2);
                                          rb.set_xhi( rc.xMax() - WIDTH2/2);
                                  }
                          }
                  }
          }
  */
  for (uint jj = 0; jj < instCnt; jj++) {
    odb::dbInst* inst = instTable[jj];
    int instId = inst->getId();
    odb::dbBox* b = inst->getBBox();
    odb::Rect rb = rectTable[jj];

    const char* debug = " ";
#ifdef DEBUG_INST_NAME
    debug = DEBUG_INST_NAME;
    if (strcmp(inst->getConstName(), debug) == 0) {
      logger_->info(RCX,
                    402,
                    "inst= {} net={} \tgetITermConn[{}]: {} {}  {} {}",
                    inst->getConstName(),
                    net->getConstName(),
                    instCnt,
                    xy[0],
                    xy[1],
                    xy2[0],
                    xy2[1]);
      logger_->info(RCX,
                    306,
                    "inst BBox= X {} {}  Y {} {}",
                    rb.xMin(),
                    rb.xMax(),
                    rb.yMin(),
                    rb.yMax());
      logger_->info(RCX,
                    397,
                    "instCount={} ----- --------- Prev X {}  Y {} ",
                    instCnt,
                    prevX,
                    prevY);
      for (uint kk = 0; kk < instCnt; kk++) {
        odb::Rect rrb = rectTable[kk];
        odb::dbInst* ii = instTable[kk];
        int instId = inst->getId();
        odb::dbBox* b = ii->getBBox();
        logger_->info(RCX,
                      391,
                      "I[{}] {:10d} {:10d}  {:10d} {:10d}",
                      kk,
                      b->xMin(),
                      b->yMin(),
                      b->xMax(),
                      b->yMax());
        logger_->info(RCX,
                      394,
                      "I[{}] {:10d} {:10d}  {:10d} {:10d}",
                      kk,
                      rrb.xMin(),
                      rrb.yMin(),
                      rrb.xMax(),
                      rrb.yMax());
      }
    }
#endif
    if ((net->getSigType() == odb::dbSigType::POWER) && inst->getUserFlag1())
      continue;
    if ((net->getSigType() == odb::dbSigType::GROUND) && inst->getUserFlag2())
      continue;

    int bbXY[2] = {rb.xMin(), rb.yMin()};
    int bbXY2[2] = {rb.xMax(), rb.yMax()};

    int n = !dir;

    odb::dbITerm* iterm = findConnect(inst, net, srcCapNode->getNet());
    if (iterm == NULL)
      continue;

    int ixy[2];
    ixy[0] = xy[0];
    ixy[1] = xy[1];
    if (dir > 0)
      ixy[!dir] = rb.xMin();
    else
      ixy[!dir] = rb.yMin();

    srcCapNode
        = makePowerRes(srcCapNode, dir, ixy, level, WIDTH2, iterm->getId(), 1);

    if ((_dbgPowerFlow)
        || ((debug != NULL) && (strcmp(inst->getConstName(), debug) == 0)))
      logger_->info(RCX,
                    309,
                    "\t\tITERM [{}]: {} {}   <{}> {}",
                    jj,
                    ixy[0],
                    ixy[1],
                    iterm->getId(),
                    inst->getConstName());

    xy[!dir] = ixy[!dir];
  }
  return srcCapNode;
}
uint extMain::getITermConn(uint dir,
                           odb::dbWireEncoder& encoder,
                           odb::dbWire* wire,
                           odb::dbNet* net,
                           int* xy,
                           int* xy2)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();

  int WIDTH2 = (xy2[dir] - xy[dir]);
  // odb::dbTechLayerRule *wRule=_netUtil->getRule(level,WIDTH2);

  std::vector<odb::dbInst*> instTable;
  uint instCnt
      = blkSearch->getInstBoxes(xy[0], xy[1], xy2[0], xy2[1], instTable);

  int prevX = xy[0];
  int prevY = xy[1];
  sortInst_x sort_by_x;
  std::sort(instTable.begin(), instTable.end(), sort_by_x);

  std::vector<odb::Rect> rectTable;
  for (uint kk = 0; kk < instCnt; kk++) {
    odb::dbInst* inst = instTable[kk];
    odb::dbBox* b = inst->getBBox();
    odb::Rect rb;
    b->getBox(rb);
    rectTable.push_back(rb);
  }
  for (uint jj = 0; jj < instCnt; jj++) {
    odb::dbInst* inst = instTable[jj];
    int instId = inst->getId();
    odb::dbBox* b = inst->getBBox();
    odb::Rect rb = rectTable[jj];

    if (jj < instCnt - 1) {
      odb::Rect rc = rectTable[jj + 1];
      if (rb.xMax() > rc.xMin())  // overallping
      {
        if (rb.xMax() > rc.xMax())  // switch inndices
        {
          rb.set_xlo(rc.xMax());

          rectTable[jj] = rc;
          rectTable[jj + 1] = rb;

          instTable[jj] = instTable[jj + 1];
          instTable[jj + 1] = inst;
        } else if (rb.xMax() < rc.xMax()) {
          rc.set_xlo(rb.xMax());
        } else {
          rc.set_xlo(rc.xMin() + WIDTH2 / 2);
          rb.set_xhi(rc.xMax() - WIDTH2 / 2);
        }
      }
    }
  }
  for (uint jj = 0; jj < instCnt; jj++) {
    odb::dbInst* inst = instTable[jj];
    int instId = inst->getId();
    odb::dbBox* b = inst->getBBox();
    odb::Rect rb = rectTable[jj];

    const char* debug = " ";
    if (strcmp(inst->getConstName(), debug) == 0) {
      logger_->info(RCX,
                    305,
                    "inst= {} net={} \tgetITermConn[{}]: {} {}  {} {}",
                    inst->getConstName(),
                    net->getConstName(),
                    instCnt,
                    xy[0],
                    xy[1],
                    xy2[0],
                    xy2[1]);
      logger_->info(RCX,
                    400,
                    "inst BBox= X {} {}  Y {} {}",
                    rb.xMin(),
                    rb.xMax(),
                    rb.yMin(),
                    rb.yMax());
      logger_->info(RCX,
                    307,
                    "instCount={} ----- --------- Prev X {}  Y {} ",
                    instCnt,
                    prevX,
                    prevY);
      for (uint kk = 0; kk < instCnt; kk++) {
        odb::Rect rrb = rectTable[kk];
        odb::dbInst* ii = instTable[kk];
        int instId = inst->getId();
        odb::dbBox* b = ii->getBBox();
        logger_->info(RCX,
                      392,
                      "I[{}] {:10d} {:10d}  {:10d} {:10d}",
                      kk,
                      b->xMin(),
                      b->yMin(),
                      b->xMax(),
                      b->yMax());
        logger_->info(RCX,
                      308,
                      "I[{}] {:10d} {:10d}  {:10d} {:10d}",
                      kk,
                      rrb.xMin(),
                      rrb.yMin(),
                      rrb.xMax(),
                      rrb.yMax());
      }
    }
    if ((net->getSigType() == odb::dbSigType::POWER) && inst->getUserFlag1())
      continue;
    if ((net->getSigType() == odb::dbSigType::GROUND) && inst->getUserFlag2())
      continue;

    if (strcmp(inst->getConstName(), debug) == 0)
      fprintf(stdout, "\t Before Ccheck\n");
    /*
                    if (dir>0) {
                            if (rb.xMin()>xy2[0]+WIDTH2/2)
                                    continue;
                            if (rb.xMax()<(xy[0]-WIDTH2/2))
                                    continue;
                    }
                    else {
                            if (rb.yMin()>xy2[1]+WIDTH2/2)
                                    continue;
                            if (rb.yMax()<xy[1]-WIDTH2/2)
                                    continue;
                    }
    */
    int bbXY[2] = {rb.xMin(), rb.yMin()};
    int bbXY2[2] = {rb.xMax(), rb.yMax()};

    // if (dir>0)
    // bbXY[!dir]= (bb->xMin()+bb->xMax())/2;
    // else
    // bbXY[!dir]= (bb->yMin()+bb->yMax())/2;

    int n = !dir;
    /*
                    if (!(  ( (bbXY[n]>=xy[n]) && (bbXY[n]<xy2[n])) ||
                    ((bbXY2[n]>=xy[n]) && (bbXY2[n]<xy2[n]))) )
                            continue;
    */

    odb::dbITerm* iterm = findConnect(inst, net, wire->getNet());
    if (strcmp(inst->getConstName(), debug) == 0)
      fprintf(stdout, "\t Before Connection\n");

    if (iterm == NULL) {
      continue;
    }

    if (strcmp(inst->getConstName(), debug) == 0)
      fprintf(stdout, "\t\t Connected!!!\n");

    int ixy[2];
    ixy[0] = xy[0];
    ixy[1] = xy[1];
    if (dir > 0)
      ixy[!dir] = rb.xMin();
    else
      ixy[!dir] = rb.yMin();
    int BB[2];
    if (dir > 0)
      BB[0] = (rb.xMin() + rb.xMax()) / 2;
    else
      BB[1] = (rb.yMin() + rb.yMax()) / 2;

    sameJunctionPoint(ixy, BB, WIDTH2, dir);
    // look at ..connect2( to see old code

    encoder.addPoint(ixy[0], ixy[1]);
    encoder.addITerm(iterm);

    if ((_dbgPowerFlow)
        || ((debug != NULL) && (strcmp(inst->getConstName(), debug) == 0)))
      logger_->info(RCX,
                    393,
                    "I[{}] {:10d} {:10d}  {:10d} {:10d}",
                    jj,
                    ixy[0],
                    ixy[1],
                    iterm->getId(),
                    inst->getConstName());

    xy[!dir] = ixy[!dir];
  }
  return instCnt;
}
class sortRect_y
{
 public:
  bool operator()(odb::Rect* bb1, odb::Rect* bb2)
  {
    if (bb1->yMin() != bb2->yMin())
      return bb1->yMin() < bb2->yMin();
    return bb1->yMax() < bb2->yMax();
  }
};
class sortRect_x
{
 public:
  bool operator()(odb::Rect* bb1, odb::Rect* bb2)
  {
    if (bb1->xMin() != bb2->xMin())
      return bb1->xMin() < bb2->xMin();
    return bb1->xMax() < bb2->xMax();
  }
};

void extMain::getSpecialItermShapes(odb::dbInst* inst,
                                    odb::dbNet* specialNet,
                                    uint dir,
                                    uint level,
                                    int* xy,
                                    int* xy2,
                                    std::vector<odb::Rect*>& rectTable,
                                    std::vector<odb::dbITerm*>& itermTable)
{
  odb::Rect bb(xy[0], xy[1], xy2[0], xy2[1]);

  odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
  odb::dbSet<odb::dbITerm>::iterator iterm_itr;

  for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr) {
    odb::dbITerm* iterm = *iterm_itr;

    if (iterm->getNet() == NULL)
      continue;
    if (iterm->getNet() != specialNet)
      continue;

    std::vector<odb::Rect*> table;
    odb::dbShape s;
    odb::dbITermShapeItr term_shapes;
    for (term_shapes.begin(iterm); term_shapes.next(s);) {
      if (s.isVia())
        continue;
      if (s.getTechLayer()->getRoutingLevel() != level)
        continue;

      // odb::Rect r(s.xMin(), s.yMin(), s.xMax(), s.yMax());
      odb::Rect* r = new odb::Rect(s.xMin(), s.yMin(), s.xMax(), s.yMax());

      if (bb.overlaps(*r))
        table.push_back(r);
      else if (r->overlaps(bb))
        table.push_back(r);
    }
    if (table.size() > 1) {
      if (dir > 0)  // horiz
      {
        sortRect_x sort_by_x;
        std::sort(table.begin(), table.end(), sort_by_x);
      } else {
        sortRect_y sort_by_y;
        std::sort(table.begin(), table.end(), sort_by_y);
      }
    }
    for (uint kk = 0; kk < table.size(); kk++) {
      odb::Rect* t = table[kk];
      rectTable.push_back(t);
      itermTable.push_back(iterm);
    }
  }
}

uint extMain::getITermPhysicalConn(uint dir,
                                   uint level,
                                   odb::dbWireEncoder& encoder,
                                   odb::dbWire* wire,
                                   odb::dbNet* net,
                                   int* xy,
                                   int* xy2)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();

  int WIDTH2 = (xy2[dir] - xy[dir]);
  // odb::dbTechLayerRule *wRule=_netUtil->getRule(level,WIDTH2);

  std::vector<odb::dbInst*> instTable;
  uint instCnt
      = blkSearch->getInstBoxes(xy[0], xy[1], xy2[0], xy2[1], instTable);

  int prevX = xy[0];
  int prevY = xy[1];

  if (dir > 0)  // horiz
  {
    sortInst_x sort_by_x;
    std::sort(instTable.begin(), instTable.end(), sort_by_x);
  } else {
    sortInst_y sort_by_y;
    std::sort(instTable.begin(), instTable.end(), sort_by_y);
  }

  std::vector<odb::Rect*> rectTable;
  std::vector<odb::dbITerm*> itermTable;
  for (uint kk = 0; kk < instCnt; kk++) {
    odb::dbInst* inst = instTable[kk];
    getSpecialItermShapes(
        inst, net, dir, level, xy, xy2, rectTable, itermTable);
  }

  if (_dbgPowerFlow)
  // if (true)
  {
    logger_->info(RCX,
                  310,
                  "getITermPhysicalConn dir={} net={} {}: {} {}  {} {}",
                  dir,
                  net->getConstName(),
                  instCnt,
                  xy[0],
                  xy[1],
                  xy2[0],
                  xy2[1]);
    for (uint k = 0; k < rectTable.size(); k++) {
      odb::Rect* rb = rectTable[k];
      odb::dbITerm* iterm = itermTable[k];
      logger_->info(RCX,
                    387,
                    "\tishape X {} {}  Y {} {} {} {} ",
                    rb->xMin(),
                    rb->xMax(),
                    rb->yMin(),
                    rb->yMax(),
                    iterm->getInst()->getConstName(),
                    iterm->getMTerm()->getConstName());
    }
  }
  for (uint k = 0; k < rectTable.size(); k++) {
    odb::Rect* rb = rectTable[k];
    odb::dbITerm* iterm = itermTable[k];
    int bbXY[2] = {rb->xMin(), rb->yMin()};
    int bbXY2[2] = {rb->xMax(), rb->yMax()};

    // if (dir>0)
    // bbXY[!dir]= (bb->xMin()+bb->xMax())/2;
    // else
    // bbXY[!dir]= (bb->yMin()+bb->yMax())/2;

    int n = !dir;
    // if (!(  ( (bbXY[n]>=xy[n]) && (bbXY[n]<xy2[n])) ||
    //((bbXY2[n]>=xy[n]) && (bbXY2[n]<xy2[n]))) )
    // continue;

    int ixy[2];
    ixy[0] = xy[0];
    ixy[1] = xy[1];
    if (dir > 0)
      ixy[!dir] = rb->xMin();
    else
      ixy[!dir] = rb->yMin();
    int BB[2];
    if (dir > 0)
      BB[0] = (rb->xMin() + rb->xMax()) / 2;
    else
      BB[1] = (rb->yMin() + rb->yMax()) / 2;

    // sameJunctionPoint(ixy, BB, WIDTH2, dir);
    // look at ..connect2( to see old code

    uint jid = encoder.addPoint(ixy[0], ixy[1]);
    // encoder.addITerm(iterm);
    _junct2iterm->set(jid, iterm);

    xy[!dir] = ixy[!dir];
  }
  return instCnt;
}
odb::dbCapNode* extMain::getITermPhysicalConnRC(odb::dbCapNode* srcCapNode,
                                                uint level,
                                                uint dir,
                                                odb::dbNet* net,
                                                int* xy,
                                                int* xy2,
                                                bool macro)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();

  int WIDTH2 = (xy2[dir] - xy[dir]);
  // odb::dbTechLayerRule *wRule=_netUtil->getRule(level,WIDTH2);

  std::vector<odb::dbInst*> instTable;
  uint instCnt
      = blkSearch->getInstBoxes(xy[0], xy[1], xy2[0], xy2[1], instTable);

  int prevX = xy[0];
  int prevY = xy[1];

  if (dir > 0)  // horiz
  {
    sortInst_x sort_by_x;
    std::sort(instTable.begin(), instTable.end(), sort_by_x);
  } else {
    sortInst_y sort_by_y;
    std::sort(instTable.begin(), instTable.end(), sort_by_y);
  }

  std::vector<odb::Rect*> rectTable;
  std::vector<odb::dbITerm*> itermTable;
  for (uint kk = 0; kk < instCnt; kk++) {
    odb::dbInst* inst = instTable[kk];
    getSpecialItermShapes(
        inst, net, dir, level, xy, xy2, rectTable, itermTable);
  }
  if (_dbgPowerFlow)
  // if (true)
  {
    logger_->info(RCX,
                  388,
                  "getITermPhysicalConn dir={} net={} {}: {} {}  {} {}",
                  dir,
                  net->getConstName(),
                  instCnt,
                  xy[0],
                  xy[1],
                  xy2[0],
                  xy2[1]);
    for (uint k = 0; k < rectTable.size(); k++) {
      odb::Rect* rb = rectTable[k];
      odb::dbITerm* iterm = itermTable[k];
      logger_->info(RCX,
                    311,
                    "\tishape X {} {}  Y {} {} {} {} ",
                    rb->xMin(),
                    rb->xMax(),
                    rb->yMin(),
                    rb->yMax(),
                    iterm->getInst()->getConstName(),
                    iterm->getMTerm()->getConstName());
    }
  }
  for (uint k = 0; k < rectTable.size(); k++) {
    odb::Rect* rb = rectTable[k];
    odb::dbITerm* iterm = itermTable[k];
    if (!((iterm->getSigType() == odb::dbSigType::POWER)
          || (iterm->getSigType() == odb::dbSigType::GROUND)))
      continue;  // COMMENT -- skip tie lo/hi -- 032613

    if (macro)  // if not hierarchical block but macro, check if for any
                // connection! only one is allowed
    {
      odb::dbStringProperty* p = odb::dbStringProperty::find(iterm, "_inode");
      if (p != NULL)
        continue;
    }

    int n = !dir;

    int ixy[2];
    ixy[0] = xy[0];
    ixy[1] = xy[1];
    if (dir > 0)
      ixy[!dir] = rb->xMin();
    else
      ixy[!dir] = rb->yMin();

    srcCapNode
        = makePowerRes(srcCapNode, dir, ixy, level, WIDTH2, iterm->getId(), 1);
    // TO DELETE _junct2iterm _junct2iterm->set(srcCapNode->getId(), iterm);

    xy[!dir] = ixy[!dir];
  }
  return srcCapNode;
}

class sortBox_y
{
 public:
  bool operator()(odb::dbBox* bb1, odb::dbBox* bb2)
  {
    return bb1->yMin() < bb2->yMin();
  }
};
class sortBox_x
{
 public:
  bool operator()(odb::dbBox* bb1, odb::dbBox* bb2)
  {
    // if ( bb1->xMin() != bb2->xMin())
    return bb1->xMin() < bb2->xMin();
    // return bb1->xMax() < bb2->xMax();
  }
};
class sortBox_level
{
  odb::dbBlockSearch* _blkSearch;

 public:
  sortBox_level(odb::dbBlockSearch* blkSearch) { _blkSearch = blkSearch; }

 public:
  bool operator()(odb::dbBox* bb1, odb::dbBox* bb2)
  {
    uint top;
    uint bot1 = _blkSearch->getViaLevels(bb1, top);
    uint bot2 = _blkSearch->getViaLevels(bb2, top);

    return bot1 < bot2;
  }
};

bool extMain::overlapWithMacro(odb::Rect& w)
{
  for (uint ii = 0; ii < _powerMacroTable.size(); ii++) {
    odb::dbInst* inst = _powerMacroTable[ii];
    odb::Rect r0;
    odb::dbBox* bb = inst->getBBox();
    bb->getBox(r0);

    if (r0.inside(w))
      return true;
  }
  return false;
}
bool extMain::skipSideMetal(std::vector<odb::dbBox*>& viaTable,
                            uint level,
                            odb::dbNet* net,
                            odb::Rect* w)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();
  int width = w->minDXDY();
  int len = w->maxDXDY();
  /*
          int xy1[2]= {w->xMin(), w->yMin()};
          int xy[2];
          xy[0]= xy1[0];
          xy[1]= xy1[1];
  */
  uint Met[2];
  odb::dbBox* vTable[2];
  uint vCnt = 0;

  uint viaCnt = viaTable.size();
  for (uint ii = 0; ii < viaCnt; ii++) {
    // int xy2[2]= {w->xMax(), w->yMax()};

    odb::dbBox* v = viaTable[ii];

    uint top;
    uint bot = blkSearch->getViaLevels(v, top);
    if (!((level == bot) || (level == top)))
      continue;

    if (vCnt > 1)
      return false;

    Met[vCnt] = bot;
    vTable[vCnt++] = v;
  }
  if (vCnt < 2)
    return false;

  odb::dbBox* v0 = vTable[0];
  odb::dbBox* v1 = vTable[1];
  odb::Rect r0;
  odb::Rect r1;
  v0->getBox(r0);
  v1->getBox(r1);
  bool overlap = r0.overlaps(r1);

  if ((Met[0] == Met[1])
      && ((Met[0] + 1) == level))  // conn from met1 rail to v12 to m2 to v12
                                   // tie lo/hi connectivity
  {
    if (overlap)
      return false;

    return true;  // COMMENT 03/26/13 -- small wire with 2 vias

    if ((_via_map[v0] != NULL) && (_via_map[v1] != NULL))
      return false;

    if ((_via_map[v0] != NULL) && (!overlapWithMacro(r1)))
      return true;
    if ((_via_map[v1] != NULL) && (!overlapWithMacro(r0)))
      return true;
  }
  if (Met[0] != Met[1])  // small metal between 2 stacked vias : satisfy DRC ??
  {
    if (overlap)
      return true;
  }
  return false;
}
void extMain::sortViasXY(uint dir, std::vector<odb::dbBox*>& viaTable)
{
  if (dir > 0)  // horiz
  {
    sortBox_x sort_by_x;
    std::sort(viaTable.begin(), viaTable.end(), sort_by_x);
  } else {
    sortBox_y sort_by_y;
    std::sort(viaTable.begin(), viaTable.end(), sort_by_y);
  }
}
char* extMain::getViaResNode(odb::dbBox* v, const char* propName)
{
  // odb::dbIntProperty *p= (odb::dbIntProperty*) odb::dbIntProperty::find(v,
  // propName);
  odb::dbIntProperty* p = odb::dbIntProperty::find(v, propName);
  if (p == NULL)
    return NULL;
  uint capId = p->getValue();
  odb::dbCapNode* capNode = odb::dbCapNode::getCapNode(_block, capId);

  char buf[2048];
  odb::dbStringProperty* ppp = odb::dbStringProperty::find(capNode, "_inode");
  if (ppp == NULL)
    return NULL;

  char* retStr = new char[128];
  strcpy(retStr, ppp->getValue().c_str());
  return retStr;
}
void extMain::addUpperVia(uint ii, odb::dbBox* v)
{
  _viaUpperTable[ii].push_back(v);
}
void extMain::writeViaResistorsRC(FILE* fp, uint ii, FILE* fp1)
{
  if (fp == NULL)
    return;
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();

  uint vCnt = _viaUpperTable[ii].size();
  for (uint jj = 0; jj < vCnt; jj++) {
    odb::dbBox* v = _viaUpperTable[ii][jj];
    if (v->isMarked())
      continue;

    v->setMarked(true);

    uint top;
    uint bot = blkSearch->getViaLevels(v, top);
    // 10312012
    float res = getPowerViaRes(v, 0.1);
    char bufRes[128];
    sprintf(bufRes, "%g", res);
    odb::dbStringProperty* p = odb::dbStringProperty::find(v, "_Res");
    if (p != NULL)
      sprintf(bufRes, "%s", p->getValue().c_str());

    char* srcNode = getViaResNode(v, "_up_node");
    char* dstNode = getViaResNode(v, "_down_node");

    // if ((srcNode==NULL)&&(dstNode==NULL))
    if ((srcNode == NULL) || (dstNode == NULL)) {
      continue;
    }
    if (dstNode == NULL) {
      continue;
    }

    if (srcNode == NULL) {
      logger_->info(RCX, 312, "src is NULL R_M{}M{}_{}", bot, top, v->getId());
      srcNode = new char[64];
      sprintf(srcNode, "%s_%d", dstNode, bot);
    }
    fprintf(fp, "R_M%dM%d_%d ", bot, top, v->getId());
    fprintf(fp, " %s %s %s\n", srcNode, dstNode, bufRes);
    if (fp1 != NULL) {
      fprintf(fp1, "R_M%dM%d_%d ", bot, top, v->getId());
      fprintf(fp1, " %s %s %s\n", srcNode, dstNode, bufRes);
    }
  }
  for (uint jj = 0; jj < vCnt; jj++) {
    odb::dbBox* v = _viaUpperTable[ii][jj];
    v->setMarked(false);
  }
  _viaUpperTable[ii].clear();
}
void extMain::writeViaResistors(FILE* fp, uint ii, FILE* fp1, bool skipWireConn)
{
  if (skipWireConn)
    return writeViaResistorsRC(fp, ii, fp1);

  odb::dbBlockSearch* blkSearch = _block->getSearchDb();

  uint vCnt = _viaUpperTable[ii].size();
  /*
          for (uint jj= 0; jj<vCnt; jj++) {
                  odb::dbBox *v= _viaUpperTable[ii][jj];
                  v->setMarked(false);
          }
  */
  for (uint jj = 0; jj < vCnt; jj++) {
    odb::dbBox* v = _viaUpperTable[ii][jj];
    if (v->isMarked())
      continue;

    uint top;
    uint bot = blkSearch->getViaLevels(v, top);
    // 10312012
    float res = getPowerViaRes(v, 0.1);

    fprintf(fp, "R_M%dM%d_%d ", bot, top, v->getId());
    if (!_nodeCoords) {
      fprintf(fp, "vM%dM%d_%d_%d", bot, top, top, v->getId());
      fprintf(fp, " vM%dM%d_%d_%d", bot, top, bot, v->getId());
      fprintf(fp, " %g\n", res);
      if (fp1 != NULL) {
        fprintf(fp1, "R_M%dM%d_%d_%d ", bot, top, v->xMin(), v->yMin());
        fprintf(fp1, "vM%dM%d_%d_%d", bot, top, top, v->getId());
        fprintf(fp1, " vM%dM%d_%d_%d", bot, top, bot, v->getId());
        fprintf(fp1, " %g\n", res);
      }
    } else {
      char srcNode[128];
      char dstNode[128];
      writeViaName_xy(srcNode, v, bot, top, top);
      writeViaName_xy(dstNode, v, bot, top, bot);

      // fprintf(fp, " %s %s %g\n", srcNode, dstNode, res);
      char buf[2048];
      odb::dbStringProperty* p = odb::dbStringProperty::find(v, "_Res");
      if (p != NULL)
        fprintf(fp, " %s %s %s\n", srcNode, dstNode, p->getValue().c_str());
      else
        fprintf(fp, " %s %s %g\n", srcNode, dstNode, res);

      if (fp1 != NULL) {
        fprintf(fp1, "R_M%dM%d_%d_%d ", bot, top, v->xMin(), v->yMin());
        if (p != NULL)
          fprintf(fp1, " %s %s %s\n", srcNode, dstNode, p->getValue().c_str());
        else
          fprintf(fp1, " %s %s %g\n", srcNode, dstNode, res);
      }
    }
    v->setMarked(true);
  }
}
bool extMain::sameJunctionPoint(int xy[2], int BB[2], uint width, uint dir)
{
  width = 0;
  int dd[2] = {xy[0] - _prevX, xy[1] - _prevY};
  if ((dir > 0) && (dd[0] <= width / 2)) {
    xy[0] = BB[0];
    return true;
  } else if ((dir == 0) && (dd[1] <= width / 2)) {
    xy[1] = BB[1];
    return true;
  }
  _prevX = xy[0];
  _prevY = xy[1];
  return false;
}
uint extMain::addGroupVias(uint level,
                           odb::Rect* w,
                           std::vector<odb::dbBox*>& viaTable)
{
  uint cnt = 0;
  for (uint ll = 0; ll < _multiViaBoxTable[level].size(); ll++) {
    odb::dbBox* v = _multiViaBoxTable[level][ll];
    odb::Rect vr;
    v->getBox(vr);
    if (!w->overlaps(vr))
      continue;
    odb::dbTechVia* via = v->getTechVia();

    viaTable.push_back(v);
    cnt++;
  }
  return cnt;
}
void extMain::createNode_xy_RC(char* buf,
                               odb::dbCapNode* capNode,
                               int x,
                               int y,
                               int level)
{
  int netId = _netUtil->getCurrentNet()->getId();
  writeNegativeCoords(buf, netId, x, y, level);

  odb::dbStringProperty::create(capNode, "_inode", buf);
  capNode->setNameFlag();
}
odb::dbCapNode* extMain::getPowerCapNode(odb::dbNet* net, int xy, uint level)
{
  odb::dbCapNode* cap = odb::dbCapNode::create(net, 0, _foreign);

  cap->setInternalFlag();
  cap->setBranchFlag();

  cap->setNode(0);

  char buf[2048];
  createNode_xy_RC(buf, cap, _last_node_xy[0], _last_node_xy[1], level);

  return cap;
}
odb::dbCapNode* extMain::makePowerRes(odb::dbCapNode* srcCap,
                                      uint dir,
                                      int xy[2],
                                      uint level,
                                      uint width,
                                      uint objId,
                                      int type)
{
  bool dbg = false;
  bool foreign = false;
  odb::dbCapNode* dstCap = odb::dbCapNode::create(srcCap->getNet(), 0, foreign);
  dstCap->setNode(objId);  // it can be 0 if first/last node of the rail

  char CapNodeName[2048];
  createNode_xy_RC(CapNodeName, dstCap, xy[0], xy[1], level);

  if (type < 0)  // internal node
  {
    dstCap->setInternalFlag();
    dstCap->setBranchFlag();
  } else if (type == 0)  // via
  {
    dstCap->setInternalFlag();
    dstCap->setBranchFlag();
  } else {
    dstCap->setITermFlag();
    odb::dbITerm* iterm = odb::dbITerm::getITerm(_block, objId);
    odb::dbStringProperty* p = odb::dbStringProperty::find(iterm, "_inode");
    if (p != NULL) {
      odb::dbStringProperty::destroy(p);
      // sprintf(buf, "%s %s",  buf, p->getValue().c_str());
    }
    odb::dbStringProperty::create(iterm, "_inode", strdup(CapNodeName));
  }
  uint pathDir = 0;
  odb::dbRSeg* rc
      = odb::dbRSeg::create(srcCap->getNet(), xy[0], xy[1], pathDir, true);

  uint rid = rc->getId();

  rc->setSourceNode(srcCap->getId());
  rc->setTargetNode(dstCap->getId());

  int len = xy[!dir] - _last_node_xy[!dir];
  len -= width;
  if (len <= 0)
    len += width;
  if (len <= 0)
    len = width;

  double res = getResistance(level, width, len, 0);
  rc->setResistance(res);

  if (dbg) {
    if (xy[!dir] - _last_node_xy[!dir] <= 0) {
      logger_->info(
          RCX,
          313,
          "R_{} level{} dir={}   xy[{}]={:10d}  _last_node_xy[{}]={:10d}   "
          "  len= {:10d} width= {:d} calc_len= {:d} res={:g}",
          rid,
          level,
          dir,
          !dir,
          xy[!dir],
          !dir,
          _last_node_xy[!dir],
          xy[!dir] - _last_node_xy[!dir],
          width,
          len,
          res);
    }
  }

  double cap = _modelTable->get(0)->getTotCapOverSub(level) * len;
  rc->setCapacitance(cap);

  _last_node_xy[0] = xy[0];
  _last_node_xy[1] = xy[1];

  return dstCap;
}
void extMain::viaTagByCapNode(odb::dbBox* v, odb::dbCapNode* cap)
{
  odb::dbIntProperty* p = odb::dbIntProperty::find(v, "_up_node");
  if (p == NULL)
    odb::dbIntProperty::create(v, "_up_node", cap->getId());
  else
    odb::dbIntProperty::create(v, "_down_node", cap->getId());
}
uint extMain::viaAndInstConnRC(uint dir,
                               uint width,
                               odb::dbTechLayer* layer,
                               odb::dbNet* net,
                               odb::dbNet* orig_power_net,
                               odb::Rect* w,
                               bool skipSideMetalFlag)
{
  int EXT = 0;
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();
  // ----------------------------
  // --------------------------
  // -i-V--i---i---i--V---i---V--i-
  //    -------------------------
  std::vector<odb::dbBox*> viaTable;
  int level = layer->getRoutingLevel();

  uint viaCnt = blkSearch->getPowerWiresAndVias(w->xMin(),
                                                w->yMin(),
                                                w->xMax(),
                                                w->yMax(),
                                                level,
                                                orig_power_net,
                                                false,
                                                viaTable);
  uint wireCnt = 0;
  if (skipSideMetalFlag) {
    if ((level == 2) && (skipSideMetal(viaTable, level, net, w)))
      return 0;
    if ((level == 3) && (skipSideMetal(viaTable, level, net, w)))
      return 0;
    if ((level == 5) && (skipSideMetal(viaTable, level, net, w)))
      return 0;
  }
  bool isVDDnet = false;
  if (orig_power_net->getSigType() == odb::dbSigType::POWER)
    isVDDnet = true;

  int LAST_VIA_MAX[2] = {w->xMin(), w->yMin()};

  int xy1[2] = {w->xMin(), w->yMin()};

  int xy[2];
  xy[0] = xy1[0];
  xy[1] = xy1[1];
  int lastXY[2] = {w->xMin(), w->yMin()};

  _last_node_xy[0] = xy[0];
  _last_node_xy[1] = xy[1];
  odb::dbCapNode* srcCapNode = NULL;
  if (level >= 1)  // for first rcseg
  {
    _last_node_xy[!dir] -= width / 2;
    srcCapNode = getPowerCapNode(net, xy[!dir] - width / 2, level);  // pseudo
    srcCapNode = makePowerRes(srcCapNode, dir, xy, level, width, 0, 0);
  } else
    srcCapNode = getPowerCapNode(net, xy[!dir], level);

  std::vector<odb::dbBox*> processedViaTable;

  uint gridViaCnt = viaCnt;
  uint powerSrcCnt = addPowerSources(viaTable, isVDDnet, level, w);
  uint gViaCnt = addGroupVias(level + 1, w, viaTable);
  gViaCnt += addGroupVias(level, w, viaTable);
  sortViasXY(dir, viaTable);
  uint jid = 0;
  viaCnt = viaTable.size();
  if (_dbgPowerFlow)
    // if (level==5)
    logger_->info(RCX,
                  386,
                  "viaAndInstConnRC[{}={}+{}+{}]: {} {}  {} {}",
                  viaCnt,
                  gViaCnt,
                  gridViaCnt,
                  powerSrcCnt,
                  w->xMin(),
                  w->yMin(),
                  w->xMax(),
                  w->yMax());
  for (uint ii = 0; ii < viaCnt; ii++) {
    int xy2[2] = {w->xMax(), w->yMax()};

    odb::dbBox* v = viaTable[ii];

    int viaXY[2] = {v->xMin(), v->yMin()};

    uint top;
    uint bot = blkSearch->getViaLevels(v, top);

    odb::Rect vr;
    v->getBox(vr);
    if (!w->overlaps(vr))
      continue;

    char* sname = NULL;
    if (v->isVisited())
      continue;

    if (!((level == bot) || (level == top))) {
      if ((bot == 0) && (top == 0)) {
        sname = getPowerSourceName(isVDDnet, level, v->getId());
        logger_->info(RCX, 315, "    powerLocName= {}", sname);

        if (sname == NULL)
          continue;
      } else {
        continue;
      }
    }

    v->setVisited(true);
    processedViaTable.push_back(v);
    if (dir > 0)
      xy2[!dir] = (v->xMin() + v->xMax()) / 2;
    // xy2[!dir]= v->xMin();
    else
      xy2[!dir] = (v->yMin() + v->yMax()) / 2;
    // xy2[!dir]= v->yMin();

    // V--I---I---I--V---
    uint itermCnt = 0;

    if (level == 1) {
      // TODO if (!skipSideMetalFlag && (_via_id_map[v->getId()]==NULL))
      // continue;
      srcCapNode
          = getITermConnRC(srcCapNode, level, dir, orig_power_net, xy, xy2);
    } else if (topHierBlock()) {
      srcCapNode = getITermPhysicalConnRC(
          srcCapNode, level, dir, orig_power_net, xy, xy2, false);
    } else  // 03/03/13 -- connectivity to the power wire only -- will mark
            // iterm!
    {
      bool macro = true;

      // 03132013 -- the wire is trying to connect! -- the via should connect
      // with iterm2vias
      srcCapNode = getITermPhysicalConnRC(
          srcCapNode, level, dir, orig_power_net, xy, xy2, macro);
    }
    LAST_VIA_MAX[0] = v->xMax();  //
    LAST_VIA_MAX[1] = v->yMax();  //

    srcCapNode
        = makePowerRes(srcCapNode, dir, xy2, level, width, v->getId(), 0);
    viaTagByCapNode(v, srcCapNode);

    if (_dbgPowerFlow) {
      char* prop = getViaResNode(v, "_up_node");
      logger_->info(RCX,
                    316,
                    "M{} via[{}]: {}  {} {}  {} {}",
                    level,
                    v->getId(),
                    prop,
                    v->xMin(),
                    v->yMin(),
                    v->xMax(),
                    v->yMax());
    }

    xy[!dir] = xy2[!dir];
    xy2[dir] = xy1[dir];
    lastXY[dir] = xy2[dir];
    lastXY[!dir] = xy2[!dir];
    if (level == 1) {
      _viaM1Table->push_back(v);
      if (skipSideMetalFlag)
        _via_map[v] = net;
    } else {
      if (sname == NULL)
        addUpperVia(isVDDnet, v);
      else {
        odb::dbStringProperty* p
            = odb::dbStringProperty::find(srcCapNode, "_inode");
        if (p != NULL)
          p->setValue(sname);
        else
          odb::dbStringProperty::create(srcCapNode, "_inode", sname);

        addPowerSourceName(isVDDnet, sname);
      }
    }
  }
  for (uint ii = 0; ii < processedViaTable.size(); ii++)
    processedViaTable[ii]->setVisited(false);

  int lastXY2[2] = {w->xMax(), w->yMax()};
  if (topHierBlock()) {
    srcCapNode = getITermPhysicalConnRC(
        srcCapNode, level, dir, orig_power_net, lastXY, lastXY2, false);
  } else if ((level == 1) && (lastXY[!dir] < lastXY2[!dir])) {
    srcCapNode = getITermConnRC(
        srcCapNode, level, dir, orig_power_net, lastXY, lastXY2);
    // if (itermCnt>0)
    // fprintf(stdout, "Last Wire iterm= %d\n", itermCnt);
  } else if (level > 1)  // 03/03/13 -- connectivity to the power wire only --
                         // will mark iterm!
  {
    srcCapNode = getITermPhysicalConnRC(
        srcCapNode, level, dir, orig_power_net, lastXY, lastXY2, true);
  }
  /* TODO
          if (lastXY[!dir]<lastXY2[!dir]) {
                  lastXY2[dir]= lastXY[dir];
                  srcCapNode= makePowerRes(srcCapNode, dir, lastXY2, level,
     width, 0, -1);
          }
  */
  return lastXY[!dir];
}
uint extMain::viaAndInstConn(uint dir,
                             uint width,
                             odb::dbTechLayer* layer,
                             odb::dbWireEncoder& encoder,
                             odb::dbWire* wire,
                             odb::dbNet* net,
                             odb::Rect* w,
                             bool skipSideMetalFlag)
{
  int EXT = 0;
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();
  // ----------------------------
  // --------------------------
  // -i-V--i---i---i--V---i---V--i-
  //    -------------------------
  std::vector<odb::dbBox*> viaTable;
  std::vector<odb::dbBox*> crossWireTable;
  int level = layer->getRoutingLevel();

  // int width= w->minDXDY();

  uint viaCnt = blkSearch->getPowerWiresAndVias(
      w->xMin(), w->yMin(), w->xMax(), w->yMax(), level, net, false, viaTable);

  uint wireCnt = 0;
  if (skipSideMetalFlag) {
    if ((level == 2) && (skipSideMetal(viaTable, level, net, w)))
      return 0;
    if ((level == 3) && (skipSideMetal(viaTable, level, net, w)))
      return 0;
    if ((level == 5) && (skipSideMetal(viaTable, level, net, w)))
      return 0;
  }
  bool isVDDnet = false;
  if (net->getSigType() == odb::dbSigType::POWER)
    isVDDnet = true;
  odb::dbTechLayerRule* wRule = _netUtil->getRule(level, width);
  // wRule->setWireExtension(width);
  encoder.newPath(layer, odb::dbWireType::ROUTED, wRule);

  int xy1[2] = {w->xMin(), w->yMin()};
  /*
          if (dir==0) // vertical
                  xy1[0]= w->xMin()+width/2;
          else
                  xy1[1]= w->yMin()+width/2;
  */
  int xy[2];
  xy[0] = xy1[0];
  xy[1] = xy1[1];
  int lastXY[2] = {w->xMin(), w->yMin()};

  encoder.addPoint(w->xMin(), w->yMin(), 0, 0);

  if (_dbgPowerFlow)
    // if (level==7)
    logger_->info(RCX,
                  314,
                  "viaAndInstConn[{}]: {} {}  {} {}",
                  viaCnt,
                  w->xMin(),
                  w->yMin(),
                  w->xMax(),
                  w->yMax());

  std::vector<odb::dbBox*> processedViaTable;

  uint powerSrcCnt = addPowerSources(viaTable, isVDDnet, level, w);
  uint gViaCnt = addGroupVias(level + 1, w, viaTable);
  gViaCnt += addGroupVias(level, w, viaTable);
  sortViasXY(dir, viaTable);
  uint jid = 0;
  viaCnt = viaTable.size();
  for (uint ii = 0; ii < viaCnt; ii++) {
    int xy2[2] = {w->xMax(), w->yMax()};

    odb::dbBox* v = viaTable[ii];

    odb::Rect vr;
    v->getBox(vr);
    if (!w->overlaps(vr)) {
      continue;
    }

    char* sname = NULL;
    if (v->isVisited())
      continue;

    v->setVisited(true);
    processedViaTable.push_back(v);

    uint top;
    uint bot = blkSearch->getViaLevels(v, top);
    if (!((level == bot) || (level == top))) {
      if ((bot == 0) && (top == 0)) {
        sname = getPowerSourceName(isVDDnet, level, v->getId());
        if (sname == NULL)
          continue;
      } else {
        continue;
      }
    }
    if (dir > 0)
      xy2[!dir] = (v->xMin() + v->xMax()) / 2;
    else
      xy2[!dir] = (v->yMin() + v->yMax()) / 2;
    // V--I---I---I--V---
    uint itermCnt = 0;

    if (level == 1) {
      if (!skipSideMetalFlag && (_via_id_map[v->getId()] == NULL))
        continue;
      itermCnt = getITermConn(dir, encoder, wire, net, xy, xy2);
    } else if (topHierBlock()) {
      itermCnt = getITermPhysicalConn(dir, level, encoder, wire, net, xy, xy2);
    }

    xy[!dir] = xy2[!dir];

    xy2[dir] = xy1[dir];
    int BB[2] = {v->xMax(), v->yMax()};
    sameJunctionPoint(xy2, BB, width, dir);
    jid = encoder.addPoint(xy2[0], xy2[1], v->getId());  // viapoint
    if (_dbgPowerFlow) {
      logger_->info(RCX,
                    317,
                    "\t\tencoder.addPoint [{}]: {} {} VIA={}",
                    ii,
                    xy2[0],
                    xy2[1],
                    v->getId());
      logger_->info(RCX,
                    318,
                    "\t\tvia: id={} {} {}  {} {}",
                    v->getId(),
                    v->xMin(),
                    v->yMin(),
                    v->xMax(),
                    v->yMax());
    }
    if (sname == NULL)
      _junct2viaMap->set(jid, v->getId());
    else
      _junct2viaMap->set(jid, -v->getId());

    lastXY[dir] = xy2[dir];
    lastXY[!dir] = xy2[!dir];
    if (level == 1) {
      _viaM1Table->push_back(v);
      if (skipSideMetalFlag)
        _via_map[v] = wire->getNet();
    } else {
      if (sname == NULL)
        addUpperVia(isVDDnet, v);
      else
        addPowerSourceName(isVDDnet, sname);
    }
  }
  for (uint ii = 0; ii < processedViaTable.size(); ii++) {
    /* added DKF 09052012 - begin
    odb::dbBox *v= viaTable[ii];
    uint top;
    uint bot= blkSearch->getViaLevels(v, top);
    if (bot==1)
    // added DKF 09052012 - end */
    processedViaTable[ii]->setVisited(false);
  }

  int lastXY2[2] = {w->xMax(), w->yMax()};
  if (topHierBlock()) {
    uint itermCnt
        = getITermPhysicalConn(dir, level, encoder, wire, net, lastXY, lastXY2);
  } else if ((level == 1) && (lastXY[!dir] < lastXY2[!dir])) {
    uint itermCnt = getITermConn(dir, encoder, wire, net, lastXY, lastXY2);
    // if (itermCnt>0)
    // fprintf(stdout, "Last Wire iterm= %d\n", itermCnt);
  }
  if (lastXY[!dir] < lastXY2[!dir]) {
    lastXY2[dir] = lastXY[dir];
    //       jid= encoder.addPoint(lastXY2[0], lastXY2[1]); // viapoint
    jid = encoder.addPoint(lastXY2[0], lastXY2[1]);  // viapoint
  }

  // encoder.end();
  return lastXY[!dir];
}
odb::dbNet* extMain::createRailNet(odb::dbNet* pnet,
                                   odb::dbTechLayer* layer,
                                   odb::Rect* w)
{
  char buf_name[128];
  sprintf(buf_name,
          "%s_%s_%d_%d_%d_%d",
          pnet->getConstName(),
          layer->getConstName(),
          w->xMin(),
          w->yMin(),
          w->xMax(),
          w->yMax());

  odb::dbNet* power_net = odb::dbNet::create(_block, buf_name);
  // fprintf(stdout, "Created net %d %s\n", power_net->getId(),
  // power_net->getConstName());
  return power_net;
}
void extMain::powerWireConnRC(odb::Rect* w,
                              uint dir,
                              odb::dbTechLayer* layer,
                              odb::dbNet* net)
{
  int length = w->maxDXDY();
  int width = w->minDXDY();

  odb::dbNet* power_net = createRailNet(net, layer, w);
  if (net->getSigType() == odb::dbSigType::POWER)
    power_net->setMark(true);

  _junct2viaMap->clear(0);
  uint jid = 0;

  uint len = w->maxDXDY();
  uint level = layer->getRoutingLevel();

  double Res = getResistance(level, width, len, 0);

  FILE* cirFP = _globCir;
  if (level == 1)
    cirFP = _stdCir;
  fprintf(cirFP,
          "\n\n*** EST. RAIL [M%d] : L=%g W=%g R=%g coords: %g %g %g %g\n",
          level,
          micronCoords(len),
          micronCoords(width),
          Res,
          micronCoords(w->xMin()),
          micronCoords(w->yMin()),
          micronCoords(w->xMax()),
          micronCoords(w->yMax()));

  bool skipSideMetalFlag = true;  // COMMENT 032613 skip tie lo/hi
  viaAndInstConnRC(dir, width, layer, power_net, net, w, skipSideMetalFlag);

  bool skipEnds = true;
  // if (level==1)
  // skipEnds= false;
  bool onlyVias = false;

  bool writeCapNodes = false;
  bool reverseRs = true;
  float totCap = 0;
  fprintf(cirFP,
          "\n*layer:M%d,%s net:%d\n\n",
          level,
          net->getConstName(),
          net->getId());
  char buff[120];
  findViaMainCoord(power_net, buff);
  Ath__parser parser;

  int n1 = parser.mkWords(buff, "_");
  int xy[2];
  bool ok2replace = true;
  if (n1 > 2) {
    xy[0] = parser.getInt(1);
    xy[1] = parser.getInt(2);
  } else {
    ok2replace = false;
  }

  if ((level > 1) || (!_skip_m1_caps)) {
    totCap = distributeCap(NULL, power_net);
    writeCapNodes = true;

    // writeResRC(cirFP, power_net, level, width, dir, skipEnds, reverseRs,
    // onlyVias, true); reverseRs= false;

    // writeCapNodesRC(cirFP, power_net, level, onlyVias, capNodeTable);
    // writeCapNodesRC(cirFP, power_net, level, onlyVias, skipEnds);
  }
  if (ok2replace)
    replaceItermCoords(power_net, dir, xy);

  double totRes = writeResRC(cirFP,
                             power_net,
                             level,
                             width,
                             dir,
                             skipEnds,
                             reverseRs,
                             onlyVias,
                             writeCapNodes,
                             xy);

  fprintf(
      cirFP,
      "*** COMP. BlockOnRail [M%d] resistance : %g capacitance : %g -- %s\n\n",
      level,
      totRes,
      totCap,
      power_net->getConstName());
  if (level > 1)  // macros
    writeMacroItermConns(power_net);
}
void extMain::writeMacroItermConns(odb::dbNet* net)
{
  odb::dbSet<odb::dbCapNode> cSet = net->getCapNodes();
  // cSet.reverse();
  odb::dbSet<odb::dbCapNode>::iterator rc_itr;

  rc_itr = cSet.begin();
  for (; rc_itr != cSet.end(); ++rc_itr) {
    odb::dbCapNode* capNode = *rc_itr;
    if (!capNode->isITerm())
      continue;

    odb::dbITerm* iterm = odb::dbITerm::getITerm(_block, capNode->getNode());
    if (iterm == NULL)
      continue;

    odb::dbStringProperty* p = odb::dbStringProperty::find(iterm, "_inode");

    if (p == NULL)
      continue;

    fprintf(_blkInfo,
            "%s %s %s [%s] %s\n",
            iterm->getInst()->getConstName(),
            iterm->getInst()->getMaster()->getConstName(),
            "is_macro",
            iterm->getMTerm()->getConstName(),
            p->getValue().c_str());

    odb::dbStringProperty::destroy(p);
  }
}

void extMain::powerWireConn(odb::Rect* w,
                            uint dir,
                            odb::dbTechLayer* layer,
                            odb::dbNet* net)
{
  int length = w->maxDXDY();
  int width = w->minDXDY();

  odb::dbNet* power_net = createRailNet(net, layer, w);
  if (net->getSigType() == odb::dbSigType::POWER)
    power_net->setMark(true);

  odb::dbWire* wire = odb::dbWire::create(power_net);
  odb::dbWireEncoder encoder;
  encoder.begin(wire);

  double railResTot = 0.0;

  _junct2viaMap->clear(0);
  uint jid = 0;

  uint len = w->maxDXDY();
  uint level = layer->getRoutingLevel();

  double Res = getResistance(level, width, len, 0);

  FILE* outFP = _globCir;
  if (level == 1)
    outFP = _stdCir;
  fprintf(outFP,
          "\n\n*** EST. RAIL [M%d] : L=%g W=%g R=%g coords: %g %g %g %g\n",
          level,
          micronCoords(len),
          micronCoords(width),
          Res,
          micronCoords(w->xMin()),
          micronCoords(w->yMin()),
          micronCoords(w->xMax()),
          micronCoords(w->yMax()));

  jid = viaAndInstConn(dir, width, layer, encoder, wire, net, w, false);

  // add last piece

  encoder.end();

  // wire->printWire();
  // print_shapes(wire);

  // int wireLength= power_net->getWire()->length();
  set_adjust_colinear(true);
  bool skipStartWarning = true;
  makeNetRCsegs(power_net, skipStartWarning);
  set_adjust_colinear(false);
  uint maxShapeId = setNodeCoords_xy(power_net, level);

  bool skipFirst = true;
  bool onlyVias = false;
  if (level == 1) {
    fprintf(_stdCir,
            "\n*layer:M%d,%s net:%d\n\n",
            level,
            net->getConstName(),
            net->getId());
    onlyVias = true;
    // 042711
    float totCap = 0;
    if (!_skip_m1_caps) {
      // totCap= distributeCap(_stdCir, power_net);
      totCap = distributeCap(NULL, power_net);
      writeCapNodes(_stdCir, power_net, level, onlyVias, skipFirst);
    }
    double totRes = writeRes(_stdCir, power_net, level, width, dir, skipFirst);

    fprintf(_stdCir,
            "*** COMP. BlockOnRail [M%d] resistance : %g capacitance : %g -- "
            "%s\n\n",
            level,
            totRes,
            totCap,
            power_net->getConstName());
  } else {
    fprintf(_globCir,
            "\n*layer:M%d,%s net:%d\n\n",
            level,
            net->getConstName(),
            net->getId());
    // float totCap= distributeCap(_globCir, power_net);
    float totCap = distributeCap(NULL, power_net);
    writeCapNodes(_globCir, power_net, level, onlyVias, skipFirst);
    double totRes = writeRes(_globCir, power_net, level, width, dir, skipFirst);

    fprintf(_globCir,
            "*** COMP. BlockOnRail [M%d] resistance : %g capacitance : %g fF "
            "-- %s\n\n",
            level,
            totRes,
            totCap,
            power_net->getConstName());
    // HERE wire->printWire();
    // print_shapes(stdout, wire);
    for (uint ii = 1; ii <= maxShapeId; ii++) {
      odb::dbITerm* iterm = _junct2iterm->geti(ii);
      if (iterm == NULL)
        continue;
      odb::dbStringProperty* p = odb::dbStringProperty::find(iterm, "_inode");
      _junct2iterm->set(ii, NULL);
      if (p == NULL)
        continue;

      fprintf(_blkInfo,
              "%s %s %s [%s] %s\n",
              iterm->getInst()->getConstName(),
              iterm->getInst()->getMaster()->getConstName(),
              "is_macro",
              iterm->getMTerm()->getConstName(),
              p->getValue().c_str());

      odb::dbStringProperty::destroy(p);
    }
    // getProp
    // write glob.info
  }

  // wire->printWire();
  // print_shapes(wire);
  // R_0_0_0_0       cM1_100 cM1_66924       0.0875  dtemp=0 tc1=0.00265
  // tc2=-2.641e-07
}
/*
class sortBox_x
{
public:
    bool operator()( odb::dbBox *i1, odb::dbBox *i2)
    {
        return i1->xMin() < i2->xMin();
    }
};
class sortBox_y
{
public:
    bool operator()( odb::dbBox *i1, odb::dbBox *i2)
    {
        return i1->yMin() < i2->yMin();
    }
};
*/

bool extMain::matchLayerDir(odb::dbBox* rail,
                            odb::dbTechLayerDir layerDir,
                            int level,
                            bool debug)
{
  int dir = rail->getDir();
  if (layerDir == odb::dbTechLayerDir::HORIZONTAL) {
    if (dir == 1)
      return true;
  } else {
    if (dir == 0)
      return true;
  }
  if (debug)
    logger_->info(RCX,
                  319,
                  "R{} DIR={} {} {} {} {} -- {} {}",
                  level,
                  dir,
                  rail->xMin(),
                  rail->yMin(),
                  rail->xMax(),
                  rail->yMax(),
                  rail->getDX(),
                  rail->getDY());
  return false;
}
uint extMain::addSboxesOnSearch(odb::dbNet* net)
{
  Ath__array1D<uint>* table = new Ath__array1D<uint>();

  uint cnt = 0;
  odb::dbSet<odb::dbSWire> swires = net->getSWires();
  odb::dbSet<odb::dbSWire>::iterator itr;

  for (itr = swires.begin(); itr != swires.end(); ++itr) {
    odb::dbSWire* swire = *itr;
    odb::dbSet<odb::dbSBox> wires = swire->getWires();
    odb::dbSet<odb::dbSBox>::iterator box_itr;

    for (box_itr = wires.begin(); box_itr != wires.end(); ++box_itr) {
      odb::dbSBox* s = *box_itr;
      if (s->isVia())
        continue;

      odb::Rect r;
      s->getBox(r);
      cnt++;

      uint wtype = 11;
      uint level = s->getTechLayer()->getRoutingLevel();

      int trackNum = _search->addBox(
          r.xMin(), r.yMin(), r.xMax(), r.yMax(), level, s->getId(), 0, wtype);

      if (false) {
        for (uint dir = 0; dir < 2; dir++) {
          Ath__grid* grid = _search->getGrid(dir, level);
          if (grid == NULL)
            continue;
          table->resetCnt();
          uint boxCnt = grid->getBoxes(trackNum, table);

          if (boxCnt > 0) {
            odb::Rect* v = getRect_SBox(table, 0, false, 0, wtype);
            logger_->info(RCX,
                          320,
                          "track[{}] {} AddBox {} {} {} {} dir={}",
                          level,
                          trackNum,
                          v->xMin(),
                          v->yMin(),
                          v->xMax(),
                          v->yMax(),
                          dir);
          }
        }
      }
    }
  }
  return cnt;
}
bool extMain::filterPowerGeoms(odb::dbSBox* s, uint targetDir, uint& maxWidth)
{
  bool dbg = false;

  uint w = s->getWidth(targetDir);

  if (dbg)
    logger_->info(RCX,
                  321,
                  "targetDir={}  maxWidth={}   D={} W={} L={}-- {} {}   {} {}",
                  targetDir,
                  maxWidth,
                  s->getDir(),
                  w,
                  s->getLength(targetDir),
                  s->xMin(),
                  s->yMin(),
                  s->xMax(),
                  s->yMax());

  if ((s->getDir() == -1) || (s->getDir() != targetDir))  // -1 == square
  {
    if (dbg)
      logger_->info(
          RCX,
          322,
          "filterPowerGeoms: targetDiri<> {} maxDith={} -- {} {}   {} {}",
          s->getDir(),
          maxWidth,
          s->xMin(),
          s->yMin(),
          s->xMax(),
          s->yMax());
    return true;
  }

  if (maxWidth < w)
    maxWidth = w;

  if (w < maxWidth) {
    if (dbg)
      logger_->info(
          RCX,
          323,
          "filterPowerGeoms: smallWidth<> {} maxDith={} -- {} {}   {} {}",
          w,
          maxWidth,
          s->xMin(),
          s->yMin(),
          s->xMax(),
          s->yMax());
    return true;
  }
  return false;
}
odb::Rect* extMain::getRect_SBox(Ath__array1D<uint>* table,
                                 uint ii,
                                 bool filter,
                                 uint targetDir,
                                 uint& maxWidth)
{
  uint boxId = table->get(ii);

  if (_sbox_id_map[boxId] != NULL)
    return NULL;

  odb::dbSBox* s = odb::dbSBox::getSBox(_block, boxId);
  if (filter) {
    if (filterPowerGeoms(s, targetDir, maxWidth))
      return NULL;
  }

  odb::Rect* r = new odb::Rect();
  s->getBox(*r);

  _sbox_id_map[boxId] = s;

  return r;
}

uint extMain::overlapPowerWires(std::vector<odb::Rect*>& mergeTableHi,
                                std::vector<odb::Rect*>& mergeTableLo,
                                std::vector<odb::Rect*>& resultTable)
{
  uint cnt = 0;
  bool dbg = false;
  for (uint ii = 0; ii < mergeTableHi.size(); ii++) {
    odb::Rect* r = mergeTableHi[ii];
    if (dbg)
      r->print("\n--- ");
    for (uint jj = 0; jj < mergeTableLo.size(); jj++) {
      odb::Rect* s = mergeTableLo[jj];
      if (dbg)
        s->print("i   ");
      odb::Rect* v = new odb::Rect();
      if (r->overlaps(*s)) {
        r->intersection(*s, *v);
        if (dbg)
          v->print("o   ");
        cnt++;
        resultTable.push_back(v);
      }
    }
  }
  return cnt;
}
uint extMain::mergePowerWires(uint dir,
                              uint level,
                              std::vector<odb::Rect*>& mergeTable)
{
  bool dbg = false;
  uint cnt = 0;
  Ath__grid* grid = _search->getGrid(dir, level);
  // odb::dbBlockSearch *blkSearch= _block->getSearchDb();
  // Ath__grid *grid = blkSearch->getGrid(dir, level);
  grid->setSearchDomain(0);

  Ath__array1D<uint>* table = new Ath__array1D<uint>();
  uint trackCnt = grid->getTrackCnt();

  uint maxWidth = 0;
  for (uint ii = 0; ii < trackCnt; ii++) {
    table->resetCnt();
    uint boxCnt = grid->getBoxes(ii, table);
    if (boxCnt == 0)
      continue;

    bool filterSmallGeoms = level == 1;
    odb::Rect* v = NULL;
    uint kk = 0;
    odb::Rect* r = NULL;
    for (; kk < boxCnt; kk++) {
      v = getRect_SBox(table, kk, filterSmallGeoms, dir, maxWidth);
      if (v == NULL)
        continue;
      break;
    }

    if (v == NULL)
      continue;

    odb::Rect* a = new odb::Rect(*v);
    // a->print("First to merge--");
    // if (filterSmallGeoms) a->print("First to merge--");

    cnt++;
    mergeTable.push_back(a);
    for (uint jj = kk + 1; jj < boxCnt; jj++) {
      odb::Rect* r = getRect_SBox(table, jj, filterSmallGeoms, dir, maxWidth);
      if (r == NULL)
        continue;

      // if (filterSmallGeoms) r->print("before merging--");

      if (r->intersects(*a)) {
        // r->merge(*a);
        a->merge(*r);  // TODO
        // fprintf(stdout, " ...... MERGED!\n");
      } else {
        // fprintf(stdout, " ...... START MERGE!\n");
        if (dbg)
          a->print("R--");
        a = new odb::Rect(*r);
        if (dbg)
          a->print("\n---");
        mergeTable.push_back(a);
        cnt++;
      }
    }
    // if (filterSmallGeoms) a->print("merged after filetering --");
  }
  return cnt;
}
odb::dbBox* extMain::createMultiVia(uint top, uint bot, odb::Rect* r)
{
  odb::dbTech* tech = _block->getDb()->getTech();

  char source_name[256];
  sprintf(source_name, "v%d_%d__", r->xMin(), r->yMin());

  odb::dbTechVia* techVia = odb::dbTechVia::create(tech, source_name);

  odb::dbTechLayer* toplayer = tech->findRoutingLayer(top);
  odb::dbTechLayer* botlayer = tech->findRoutingLayer(bot);

  // odb::dbBox *v = _netUtil->createTechVia(x, y, x+1, y+1, level, level+1);

  odb::dbBox* topb = odb::dbBox::create(
      techVia, toplayer, r->xMin(), r->yMin(), r->xMax(), r->yMax());
  odb::dbBox* botb = odb::dbBox::create(
      techVia, botlayer, r->xMin(), r->yMin(), r->xMax(), r->yMax());

  odb::dbVia* via
      = odb::dbVia::create(_block, source_name, techVia, odb::dbOrientType::R0);
  odb::dbBox* v = via->getBBox();
  // odb::dbTechVia *tvia= v->getTechVia();

  return v;
}
void extMain::mergeViasOnMetal_1(odb::Rect* w,
                                 odb::dbNet* pNet,
                                 uint level,
                                 std::vector<odb::dbBox*>& viaTable)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();

  for (uint kk = 0; kk < viaTable.size(); kk++)  // all vias on rail
  {
    odb::dbBox* v = viaTable[kk];
    uint top;
    uint bot = blkSearch->getViaLevels(v, top);
    if (bot > 1)
      continue;

    odb::Rect vr;
    v->getBox(vr);
    if (!w->overlaps(vr))
      continue;

    v->setVisited(false);
    std::vector<odb::dbBox*> upViaTable;
    uint cnt = blkSearch->getPowerWiresAndVias(v->xMin(),
                                               v->yMin(),
                                               v->xMax(),
                                               v->yMax(),
                                               level,
                                               pNet,
                                               false,
                                               upViaTable);

    if (mergeStackedViasOpt(_globCir, pNet, upViaTable, v, _viaStackGlobCir)
        > 0)
      _via_id_map[v->getId()] = pNet;
    else
      _via_id_map[v->getId()] = NULL;

    v->setVisited(false);
  }
}
void extMain::formOverlapVias(std::vector<odb::Rect*> mergeTable[16],
                              odb::dbNet* pNet)
{
  bool dbg = false;
  std::vector<odb::Rect*> overlapTable[16];

  odb::dbBlockSearch* blkSearch = _block->getSearchDb();
  int layerCount = _tech->getRoutingLayerCount();

  for (uint level = layerCount; level > 1; level--) {
    if (mergeTable[level].size() == 0)
      continue;

    if (dbg)
      logger_->info(RCX, 324, "OVERLAPS ---- LEVEL= {} ----", level);
    uint ocnt = overlapPowerWires(
        mergeTable[level], mergeTable[level - 1], overlapTable[level]);
    _overlapPowerWireCnt += overlapTable[level].size();
    for (uint jj = 0; jj < overlapTable[level].size(); jj++) {
      odb::Rect* w = overlapTable[level][jj];
      if (dbg)
        w->print("--- ");

      std::vector<odb::dbBox*> viaTable;
      uint viaCnt = blkSearch->getPowerWiresAndVias(w->xMin(),
                                                    w->yMin(),
                                                    w->xMax(),
                                                    w->yMax(),
                                                    level,
                                                    pNet,
                                                    false,
                                                    viaTable);

      std::vector<odb::dbBox*> overlapviaTable;
      uint vCnt = 0;
      for (uint ii = 0; ii < viaCnt; ii++) {
        odb::dbBox* v = viaTable[ii];
        uint top;
        uint bot = blkSearch->getViaLevels(v, top);
        if (!((level == bot) || (level == top)))
          continue;

        odb::Rect vr;
        v->getBox(vr);
        if (!w->overlaps(vr))
          continue;

        overlapviaTable.push_back(v);  // ADD 111312

        vCnt++;
      }
      if (vCnt > 1) {
        for (uint ii = 0; ii < vCnt; ii++) {
          odb::dbBox* w1 = overlapviaTable[ii];
          w1->setVisited(true);
        }
        odb::dbBox* W = overlapviaTable[0];  // ADD 111312

        // TODO SET RESISTANCE writeNegativeCoords(buf, pNet->getId(),
        // w->xMin(), w->yMin(), level, "MULTI");
        char buf[128];
        float res = getPowerViaRes(W, 0.1);
        writeNegativeCoords(buf, pNet->getId(), w->xMin(), w->yMin(), -1, "");
        odb::dbStringProperty::create(W, "_inode", buf);

        sprintf(buf, "%g", res / vCnt);
        odb::dbStringProperty::create(W, "_Res", buf);
        /*
                                        fprintf(stdout, "W   %12d %12d %12d %12d
           -- %d %d : id=%d\n", W->xMin(), W->yMin(), W->xMax(), W->yMax(),
           W->getDX(), W->getDY(), W->getId());
        */

        /*
                                        odb::dbVia *mv0= W->getBlockVia();
                                        odb::dbTechLayer *routelayer=
           tech->findRoutingLayer(level); odb::dbBox::create(mv0, routelayer,
           w->xMin(), w->yMin(), w->xMax(), w->yMax() ); W= mv0->getBBox();

                                        odb::dbTechVia *mv1= W->getTechVia();
                                        fprintf(stdout, "WMod   %12d %12d %12d
           %12d -- %d %d : id=%d\n", W->xMin(), W->yMin(), W->xMax(), W->yMax(),
           W->getDX(), W->getDY(), W->getId());

        */
        /* 12032012
                                        _multiViaTable[level].push_back(w);
                                        _viaOverlapPowerCnt += vCnt;
                                        odb::dbBox *v= createMultiVia(level,
           level-1, w);
        */
        //_multiViaBoxTable[level].push_back(v);

        _multiViaBoxTable[level].push_back(W);
      }
    }
    if (dbg)
      logger_->info(RCX, 325, "NEW VIAS ---- LEVEL= {} ----", level);
    for (uint kk = 0; kk < _multiViaBoxTable[level].size(); kk++) {
      // odb::Rect *w= _multiViaTable[level][kk];
      odb::dbBox* v = _multiViaBoxTable[level][kk];
      if (dbg) {
        // w->print("V     ");
        fprintf(stdout,
                "B%d    %12d %12d %12d %12d\n",
                v->isVisited(),
                v->xMin(),
                v->yMin(),
                v->xMax(),
                v->yMax());
      }
    }
    _multiViaCnt += _multiViaTable[level].size();
  }
}
void extMain::railConnOpt(odb::dbNet* pNet)
{
  bool dbg = false;
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();
  odb::dbTech* tech = _block->getDb()->getTech();
  odb::dbBox* bb = _block->getBBox();
  odb::Rect BB;
  bb->getBox(BB);

  initPowerSearch();

  _powerWireCnt += addSboxesOnSearch(pNet);
  int layerCount = _tech->getRoutingLayerCount();
  std::vector<odb::Rect*> mergeTable[16];
  std::vector<odb::Rect*> mergeTableNOT[16];
  for (uint level = layerCount; level > 0; level--) {
    odb::dbTechLayer* routelayer = tech->findRoutingLayer(level);
    odb::dbTechLayerDir layerDir = routelayer->getDirection();
    uint dir = 0;
    if (layerDir == odb::dbTechLayerDir::HORIZONTAL)
      dir = 1;

    if (dbg)
      logger_->info(
          RCX, 326, "--- MERGE ---- DIR={} - LEVEL= {} ----", dir, level);
    uint powerWireCnt = mergePowerWires(dir, level, mergeTable[level]);
    _mergedPowerWireCnt += powerWireCnt;
    powerWireCnt = mergePowerWires(!dir, level, mergeTableNOT[level]);
    _mergedPowerWireCnt += powerWireCnt;
    for (uint jj = 0; jj < mergeTable[level].size(); jj++) {
      odb::Rect* r = mergeTable[level][jj];
      if (dbg)
        r->print("--- ");
      // if (level==1) r->print("AFTER MERGE --- ");
    }
    for (uint jj = 0; jj < mergeTableNOT[level].size(); jj++) {
      odb::Rect* r = mergeTableNOT[level][jj];
      if (dbg)
        r->print("--- ");
      // if (level==1) r->print("AFTER MERGE --- ");
    }
  }
  std::vector<odb::Rect*> overlapTable[16];

  for (uint level = layerCount; level > 1; level--) {
    _multiViaTable[level].clear();
    _multiViaBoxTable[level].clear();
  }
  formOverlapVias(mergeTable, pNet);
  formOverlapVias(mergeTableNOT, pNet);

  for (uint level = layerCount; level > 1; level--) {
    for (uint kk = 0; kk < _multiViaTable[level].size(); kk++) {
      odb::dbBox* v = _multiViaBoxTable[level][kk];
      v->setVisited(false);
      // fprintf(stdout, "B%d L%d %d  %12d %12d %12d %12d\n",
      // v->isVisited(), level, v->getId(), v->xMin(), v->yMin(), v->xMax(),
      // v->yMax());
    }
  }
  if (_overlapPowerWireCnt > 0)
    logger_->info(
        RCX,
        327,
        "After Net: {} {} -- {} power Wires, into {} merged wires, {} wire "
        "overlaps, {} multi-via objects ==> created {} compound Via Objects",
        pNet->getId(),
        pNet->getConstName(),
        _powerWireCnt,
        _mergedPowerWireCnt,
        _overlapPowerWireCnt,
        _viaOverlapPowerCnt,
        _multiViaCnt);

  for (uint level = layerCount; level > 0; level--) {
    odb::dbTechLayer* routelayer = tech->findRoutingLayer(level);
    odb::dbTechLayerDir layerDir = routelayer->getDirection();
    uint dir = 0;
    if (layerDir == odb::dbTechLayerDir::HORIZONTAL)
      dir = 1;

    if (_wireInfra) {
      if (level == 1) {
        for (uint kk = 0; kk < mergeTable[1].size(); kk++) {
          odb::Rect* w = mergeTable[1][kk];
          std::vector<odb::dbBox*> viaTable;
          uint viaCnt = blkSearch->getPowerWiresAndVias(w->xMin(),
                                                        w->yMin(),
                                                        w->xMax(),
                                                        w->yMax(),
                                                        1,
                                                        pNet,
                                                        false,
                                                        viaTable);

          mergeViasOnMetal_1(w, pNet, 1, viaTable);
        }
      }
    }
    for (uint kk = 0; kk < mergeTable[level].size(); kk++) {
      odb::Rect* r = mergeTable[level][kk];
      // HERE
      if (level == 0) {
        logger_->info(RCX,
                      384,
                      "powerWireConn: M{}  {} {} {} {} -- {} {}",
                      level,
                      r->xMin(),
                      r->yMin(),
                      r->xMax(),
                      r->yMax(),
                      r->dx(),
                      r->dy());
      }
      if (_wireInfra)
        powerWireConn(r, dir, routelayer, pNet);
      else
        powerWireConnRC(r, dir, routelayer, pNet);
    }
    for (uint kk = 0; kk < mergeTableNOT[level].size(); kk++) {
      odb::Rect* r = mergeTableNOT[level][kk];
      // HERE
      if (level == 0) {
        logger_->info(RCX,
                      328,
                      "powerWireConn: M{}  {} {} {} {} -- {} {}",
                      level,
                      r->xMin(),
                      r->yMin(),
                      r->xMax(),
                      r->yMax(),
                      r->dx(),
                      r->dy());
      }
      if (_wireInfra)
        powerWireConn(r, !dir, routelayer, pNet);
      else
        powerWireConnRC(r, !dir, routelayer, pNet);
    }
  }
  if (!_wireInfra) {
    for (uint kk = 0; kk < _viaM1Table->size(); kk++)
    // for (uint kk= 0; kk<_viaM1_VDDtable.size(); kk++)
    {
      // odb::dbBox *v= _viaM1_VDDtable[kk];
      odb::dbBox* v = (*_viaM1Table)[kk];

      v->setVisited(false);
      std::vector<odb::dbBox*> upViaTable;
      uint cnt = blkSearch->getPowerWiresAndVias(v->xMin(),
                                                 v->yMin(),
                                                 v->xMax(),
                                                 v->yMax(),
                                                 1,
                                                 pNet,
                                                 false,
                                                 upViaTable);

      if (mergeStackedViasOpt(_globCir, pNet, upViaTable, v, _viaStackGlobCir)
          > 0)
        _via_id_map[v->getId()] = pNet;

      v->setVisited(false);
    }
  }
}
void extMain::railConn(odb::dbNet* pNet)
{
  bool debug = false;
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();
  odb::dbTech* tech = _block->getDb()->getTech();
  odb::dbBox* bb = _block->getBBox();
  odb::Rect BB;
  bb->getBox(BB);

  logger_->info(RCX,
                329,
                "BBOX: {} {} {} {} {} {}",
                bb->xMin(),
                bb->yMin(),
                bb->xMax(),
                bb->yMax(),
                BB.dx(),
                BB.dy());

  std::vector<odb::dbBox*> mergeTable[16];

  std::vector<odb::dbBox*> boxTable;
  int layerCount = _tech->getRoutingLayerCount();
  for (uint level = layerCount; level > 0; level--) {
    boxTable.clear();
    uint n = blkSearch->getPowerWires(
        bb->xMin(), bb->yMin(), bb->xMax(), bb->yMax(), level, pNet, boxTable);
    if (n == 0)
      continue;

    odb::dbTechLayer* routelayer = tech->findRoutingLayer(level);
    odb::dbTechLayerDir layerDir = routelayer->getDirection();

    uint maxLen = 0;
    logger_->info(RCX,
                  330,
                  "{} M{} power wires of net {} found:",
                  n,
                  level,
                  pNet->getConstName());
    int dirCnt[2];
    dirCnt[0] = 0;
    dirCnt[1] = 0;

    uint dir = 0;
    if (layerDir == odb::dbTechLayerDir::HORIZONTAL) {
      sortBox_y sort_by_y;
      std::sort(boxTable.begin(), boxTable.end(), sort_by_y);
      dir = 1;
    } else {
      sortBox_x sort_by_x;
      std::sort(boxTable.begin(), boxTable.end(), sort_by_x);
    }

    // uint m= mergeTable[level].size();
    uint m = boxTable.size();
    if (debug)
      logger_->info(RCX, 331, "{} power RAILS found", m);
    for (uint jj = 0; jj < m; jj++) {
      // odb::dbBox *rail= mergeTable[level][jj];
      odb::dbBox* rail = boxTable[jj];
      if (debug)
        logger_->info(RCX,
                      385,
                      "R{} DIR={} {} {} {} {} -- {} {}",
                      level,
                      dir,
                      rail->xMin(),
                      rail->yMin(),
                      rail->xMax(),
                      rail->yMax(),
                      rail->getDX(),
                      rail->getDY());
      std::vector<odb::dbBox*> railMergeTable;
      if (!matchLayerDir(rail, layerDir, level, false))  // squares not
        continue;
      railMergeTable.push_back(rail);
      if (layerDir == odb::dbTechLayerDir::HORIZONTAL) {
        int midXY = (rail->yMin() + rail->yMax()) / 2;
        jj++;
        for (; jj < m; jj++) {
          odb::dbBox* w = boxTable[jj];
          if (!matchLayerDir(w, layerDir, level, false))  // squares not
            continue;
          int mid = (w->yMin() + w->yMax()) / 2;
          if ((w->yMax() == rail->yMax()) || (w->yMin() == rail->yMin())
              || ((mid <= midXY + 0.5 * w->getDY()) && (mid >= midXY))
              || ((mid >= midXY - 0.5 * w->getDY()) && (mid <= midXY)))
            railMergeTable.push_back(w);
          else {
            jj--;
            break;
          }
        }
        sortBox_x sort_by_x;
        std::sort(railMergeTable.begin(), railMergeTable.end(), sort_by_x);
      } else {
        int midXY = (rail->xMin() + rail->xMax()) / 2;
        jj++;
        for (; jj < m; jj++) {
          odb::dbBox* w = boxTable[jj];
          if (!matchLayerDir(w, layerDir, level, false))  // squares not
            continue;
          int mid = (w->xMin() + w->xMax()) / 2;
          if ((w->xMax() == rail->xMax()) || (w->xMin() == rail->xMin())
              || ((mid <= midXY + 0.5 * w->getDX()) && (mid >= midXY))
              || ((mid >= midXY - 0.5 * w->getDX()) && (mid <= midXY)))
            railMergeTable.push_back(w);
          else {
            jj--;
            break;
          }
        }
        sortBox_y sort_by_y;
        std::sort(railMergeTable.begin(), railMergeTable.end(), sort_by_y);
      }
      for (uint kk = 0; kk < railMergeTable.size(); kk++) {
        odb::dbBox* w = railMergeTable[kk];
        if (debug) {
          logger_->info(RCX,
                        332,
                        "\t{} {} {} {} -- {} {}",
                        w->xMin(),
                        w->yMin(),
                        w->xMax(),
                        w->yMax(),
                        w->getDX(),
                        w->getDY());
        }
        odb::Rect r;
        w->getBox(r);
        kk++;
        for (; kk < railMergeTable.size(); kk++) {
          odb::dbBox* v = railMergeTable[kk];
          // if (layerDir==odb::dbTechLayerDir::HORIZONTAL)
          //{
          odb::Rect a;
          v->getBox(a);
          if (r.intersects(a)) {
            r.merge(a);  // TODO
            if (debug) {
              logger_->info(RCX,
                            333,
                            "\t\tM {} {} {} {} ",
                            v->xMin(),
                            v->yMin(),
                            v->xMax(),
                            v->yMax());
            }
          } else {
            kk--;
            break;
          }
          //}
        }
        if (debug)
          logger_->info(RCX,
                        334,
                        "Merged: {} {} {} {} -- {} {}",
                        r.xMin(),
                        r.yMin(),
                        r.xMax(),
                        r.yMax(),
                        r.dx(),
                        r.dy());
        if (level == 1) {
          std::vector<odb::dbBox*> viaTable;
          uint viaCnt = blkSearch->getPowerWiresAndVias(r.xMin(),
                                                        r.yMin(),
                                                        r.xMax(),
                                                        r.yMax(),
                                                        level,
                                                        pNet,
                                                        false,
                                                        viaTable);

          for (uint kk = 0; kk < viaCnt; kk++)  // all vias on rail
          {
            odb::dbBox* v = viaTable[kk];
            uint top;
            uint bot = blkSearch->getViaLevels(v, top);
            if (bot > 1)
              continue;

            v->setVisited(false);
            std::vector<odb::dbBox*> upViaTable;
            uint cnt = blkSearch->getPowerWiresAndVias(v->xMin(),
                                                       v->yMin(),
                                                       v->xMax(),
                                                       v->yMax(),
                                                       level,
                                                       pNet,
                                                       false,
                                                       upViaTable);

            if (mergeStackedVias(
                    _globCir, pNet, upViaTable, v, _viaStackGlobCir)
                > 0)
              _via_id_map[v->getId()] = pNet;

            v->setVisited(false);
          }
        }
        if (layerDir == odb::dbTechLayerDir::HORIZONTAL) {
          if (r.dx() <= r.dy()) {
            if (debug)
              logger_->info(RCX,
                            382,
                            "Skip: M{}  {} {} {} {} -- {} {}",
                            level,
                            r.xMin(),
                            r.yMin(),
                            r.xMax(),
                            r.yMax(),
                            r.dx(),
                            r.dy());
            continue;
          }
        } else {
          if (r.dy() <= r.dx()) {
            if (debug)
              logger_->info(RCX,
                            335,
                            "Skip: M{}  {} {} {} {} -- {} {}",
                            level,
                            r.xMin(),
                            r.yMin(),
                            r.xMax(),
                            r.yMax(),
                            r.dx(),
                            r.dy());
            continue;
          }
        }
        // if (debug)
        logger_->info(RCX,
                      383,
                      "powerWireConn: M{}  {} {} {} {} -- {} {}",
                      level,
                      r.xMin(),
                      r.yMin(),
                      r.xMax(),
                      r.yMax(),
                      r.dx(),
                      r.dy());
        powerWireConn(&r, dir, routelayer, pNet);
      }
    }
  }
}
uint extMain::print_shapes(FILE* fp, odb::dbWire* wire)
{
  odb::dbWireShapeItr shapes;
  odb::dbShape s;

  uint tLen = 0;
  for (shapes.begin(wire); shapes.next(s);) {
    uint width = s.getDY();
    uint len = s.getDX();
    if (width > s.getDX()) {
      width = s.getDX();
      len = s.getDY();
    }
    tLen += len;
    fprintf(fp,
            "L=%d sid=%d  m=%d  %d %d  %d %d\n",
            len,
            shapes.getShapeId(),
            s.getTechLayer()->getRoutingLevel(),
            s.xMin(),
            s.yMin(),
            s.xMax(),
            s.yMax());
  }
  return tLen;
}
void extMain::setupNanoFiles(odb::dbNet* net)
{
  odb::dbSigType type = net->getSigType();

  if (type == odb::dbSigType::POWER) {
    _coordsFP = _coordsVDD;
    _blkInfo = _blkInfoVDD;
    _viaInfo = _viaInfoVDD;
    _globGeom = _globGeomVDD;
    _globCir = _globCirVDD;
    _viaStackGlobCir = _viaStackGlobVDD;
    _stdCir = _stdCirVDD;
    _globCirHead = _globCirHeadVDD;
    _stdCirHead = _stdCirHeadVDD;
    _viaM1Table = &_viaM1_VDDtable;
    _viaUpTable = &_viaUp_VDDtable;
  } else if (type == odb::dbSigType::GROUND) {
    _coordsFP = _coordsGND;
    _blkInfo = _blkInfoGND;
    _viaInfo = _viaInfoGND;
    _globCir = _globCirGND;
    _viaStackGlobCir = _viaStackGlobGND;
    _globGeom = _globGeomGND;
    _stdCir = _stdCirGND;
    _globCirHead = _globCirHeadGND;
    _stdCirHead = _stdCirHeadGND;
    _viaM1Table = &_viaM1_GNDtable;
    _viaUpTable = &_viaUp_GNDtable;
  }
}
void extMain::setPrefix(char* prefix)
{
  sprintf(prefix, "");
  if (strlen(_node_blk_prefix) > 0)
    sprintf(prefix, "%s/%s", _node_blk_prefix, _node_inst_prefix);
}
FILE* extMain::openNanoFile(const char* name,
                            const char* name2,
                            const char* suffix,
                            const char* perms)
{
  char prefix[256];
  sprintf(prefix, "");
  if (strlen(_node_blk_prefix) > 0) {
    char syscmd[256];
    sprintf(syscmd, "mkdir -p %s", _node_blk_prefix);
    system(syscmd);
    setPrefix(prefix);
    // sprintf(prefix, "%s/", _node_blk_prefix);
  }
  char buf[1024];
  sprintf(buf, "%s%s_%s.%s", prefix, name, name2, suffix);

  FILE* fp = fopen(buf, perms);
  if (fp == NULL) {
    fprintf(stderr, "Cannot open file %s with %s\n", buf, perms);
    exit(1);
  }
  return fp;
}
void extMain::netDirPrefix(char* prefix, char* netName)
{
  sprintf(prefix, "%s/%s/%s", _node_blk_dir, netName, _node_inst_prefix);
  char syscmd[2056];
  sprintf(syscmd, "mkdir -p %s/%s", _node_blk_dir, netName);
  system(syscmd);
}
FILE* extMain::openNanoFileNet(char* netname,
                               const char* name,
                               const char* name2,
                               const char* suffix,
                               const char* perms)
{
  char hier_prefix[2048];
  netDirPrefix(hier_prefix, netname);

  char buf[2048];
  sprintf(buf, "%s%s_%s.%s", hier_prefix, name, name2, suffix);

  FILE* fp = fopen(buf, perms);
  if (fp == NULL) {
    fprintf(stderr, "Cannot open file %s with %s\n", buf, perms);
    exit(1);
  }
  return fp;
}
void extMain::writeSubckt(FILE* fp,
                          const char* keyword,
                          const char* vdd,
                          const char* std,
                          const char* cont)
{
  // fprintf(fp, "\n*** Nefelus RC - 2/16/10\n");
  fprintf(
      fp, "\n%s %s_%s_%s %s ", keyword, vdd, std, _block->getConstName(), cont);
}
void extMain::writeGeomHeader(FILE* fp, const char* vdd)
{
  fprintf(fp, "VeloceRF\n");
  fprintf(fp, "startgeom\n");
  fprintf(fp, "top_level_pins %c\n", '%');
  fprintf(fp, "obj %s  %c\n", vdd, '%');
  fprintf(fp, "prop netName %s  endprop\n", vdd);
}
void extMain::openNanoFilesDomain(odb::dbNet* pNet)
{
  const char* vdd = "GND";
  const char* ptype = "ground";
  int ii = 0;
  if (pNet->getSigType() == odb::dbSigType::POWER) {
    vdd = "VDD";
    ii = 1;
    ptype = "power";
  }
  char* netName = (char*) pNet->getConstName();
  char sysCmd[4000];
  if (strcmp(_node_blk_dir, "./") == 0)
    sprintf(sysCmd, "echo %s %s %s >> Extract.info", netName, netName, ptype);
  else
    sprintf(sysCmd,
            "echo %s %s/%s %s >> Extract.info",
            netName,
            _node_blk_dir,
            netName,
            ptype);
  system(sysCmd);

  _subCktNodeFP[0][ii] = openNanoFileNet(netName, "std", vdd, "subckt", "w");
  _subCktNodeFP[1][ii] = openNanoFileNet(netName, "glob", vdd, "subckt", "w");

  writeSubckt(_subCktNodeFP[0][ii], ".subckt", vdd, "std", " \\\n");
  writeSubckt(_subCktNodeFP[1][ii], ".subckt", vdd, "glob", " \\\n");

  _subCktNodeCnt[0][0] = 0;
  _subCktNodeCnt[0][1] = 0;
  _subCktNodeCnt[1][0] = 0;
  _subCktNodeCnt[1][1] = 0;

  if (pNet->getSigType() == odb::dbSigType::POWER) {
    _blkInfoVDD = openNanoFileNet(netName, "Block", "VDD", "info", "w");
    fprintf(_blkInfoVDD, "%c\n", _block->getHierarchyDelimeter());

    _viaInfoVDD = openNanoFileNet(netName, "Via", "VDD", "info", "w");
    _stdCirVDD = openNanoFileNet(netName, "std", "VDD", "cir", "w");
    _globGeomVDD = openNanoFileNet(netName, "glob", "VDD", "geom", "w");
    writeGeomHeader(_globGeomVDD, "VDD");

    _globCirVDD = openNanoFileNet(netName, "glob", "VDD", "cir", "w");

    _coordsVDD = openNanoFileNet(netName, "ViaBlock", "VDD", "coords", "w");
    fprintf(_coordsVDD, "DbUnitsPerMicron %d\n", _block->getDbUnitsPerMicron());

    _viaStackGlobVDD = openNanoFileNet(netName, "glob_vias", "VDD", "cir", "w");

    _blkInfoGND = NULL;
    _viaInfoGND = NULL;
    _stdCirGND = NULL;
    _globGeomGND = NULL;
    _globCirGND = NULL;
    _coordsGND = NULL;
    _viaStackGlobGND = NULL;
  } else {
    _blkInfoGND = openNanoFileNet(netName, "Block", "GND", "info", "w");
    fprintf(_blkInfoGND, "%c\n", _block->getHierarchyDelimeter());
    _viaInfoGND = openNanoFileNet(netName, "Via", "GND", "info", "w");
    _stdCirGND = openNanoFileNet(netName, "std", "GND", "cir", "w");

    _globGeomGND = openNanoFileNet(netName, "glob", "GND", "geom", "w");
    writeGeomHeader(_globGeomGND, "GND");

    _globCirGND = openNanoFileNet(netName, "glob", "GND", "cir", "w");
    _coordsGND = openNanoFileNet(netName, "ViaBlock", "GND", "coords", "w");
    fprintf(_coordsGND, "DbUnitsPerMicron %d\n", _block->getDbUnitsPerMicron());

    _viaStackGlobGND = openNanoFileNet(netName, "glob_vias", "GND", "cir", "w");

    _blkInfoVDD = NULL;
    _viaInfoVDD = NULL;
    _stdCirVDD = NULL;
    _globGeomVDD = NULL;
    _globCirVDD = NULL;
    _coordsVDD = NULL;
    _viaStackGlobVDD = NULL;
  }
}
void extMain::openNanoFiles()
{
  _subCktNodeFP[0][0] = openNanoFile("std", "GND", "subckt", "w");
  _subCktNodeFP[1][0] = openNanoFile("glob", "GND", "subckt", "w");
  _subCktNodeFP[0][1] = openNanoFile("std", "VDD", "subckt", "w");
  _subCktNodeFP[1][1] = openNanoFile("glob", "VDD", "subckt", "w");

  writeSubckt(_subCktNodeFP[0][1], ".subckt", "VDD", "std", " \\\n");
  writeSubckt(_subCktNodeFP[1][1], ".subckt", "VDD", "glob", " \\\n");
  writeSubckt(_subCktNodeFP[0][0], ".subckt", "GND", "std", " \\\n");
  writeSubckt(_subCktNodeFP[1][0], ".subckt", "GND", "glob", " \\\n");

  _subCktNodeCnt[0][0] = 0;
  _subCktNodeCnt[0][1] = 0;
  _subCktNodeCnt[1][0] = 0;
  _subCktNodeCnt[1][1] = 0;

  _blkInfoVDD = openNanoFile("Block", "VDD", "info", "w");
  fprintf(_blkInfoVDD, "%c\n", _block->getHierarchyDelimeter());

  _viaInfoVDD = openNanoFile("Via", "VDD", "info", "w");

  _blkInfoGND = openNanoFile("Block", "GND", "info", "w");
  // fprintf(_blkInfoGND, ".\n");
  fprintf(_blkInfoGND, "%c\n", _block->getHierarchyDelimeter());

  _viaInfoGND = openNanoFile("Via", "GND", "info", "w");

  _stdCirVDD = openNanoFile("std", "VDD", "cir", "w");
  // writeSubckt(_stdCirVDD, ".subckt", "VDD", "std", "");

  _globGeomVDD = openNanoFile("glob", "VDD", "geom", "w");
  writeGeomHeader(_globGeomVDD, "VDD");

  _globCirVDD = openNanoFile("glob", "VDD", "cir", "w");
  // writeSubckt(_globCirVDD, ".subckt", "VDD", "glob", "\\");

  _stdCirGND = openNanoFile("std", "GND", "cir", "w");
  // writeSubckt(_stdCirGND, ".subckt", "GND", "std", "");

  _globGeomGND = openNanoFile("glob", "GND", "geom", "w");
  writeGeomHeader(_globGeomGND, "GND");

  _globCirGND = openNanoFile("glob", "GND", "cir", "w");
  // writeSubckt(_globCirGND, ".subckt", "GND", "glob", "\\");

  _coordsVDD = openNanoFile("ViaBlock", "VDD", "coords", "w");
  _coordsGND = openNanoFile("ViaBlock", "GND", "coords", "w");

  fprintf(_coordsGND, "DbUnitsPerMicron %d\n", _block->getDbUnitsPerMicron());
  fprintf(_coordsVDD, "DbUnitsPerMicron %d\n", _block->getDbUnitsPerMicron());
  _viaStackGlobGND = openNanoFile("glob_vias", "GND", "cir", "w");
  _viaStackGlobVDD = openNanoFile("glob_vias", "VDD", "cir", "w");
}
void extMain::printItermNodeSubCkt(FILE* fp, std::vector<uint>& iTable)
{
  uint n = iTable.size();
  for (uint ii = 0; ii < n; ii++) {
    uint id = iTable[ii];

    if ((ii + 1) % 20 == 0)
      fprintf(fp, "\\ \n");
    fprintf(fp, "I%d ", id);
  }
  fprintf(fp, " \\ \n");
}
void extMain::printViaNodeSubCkt(FILE* fp, std::vector<odb::dbBox*>& viaTable)
{
  uint viaCnt = viaTable.size();
  for (uint ii = 0; ii < viaCnt; ii++) {
    odb::dbBox* v = viaTable[ii];
    v->setVisited(false);
  }
  for (uint ii = 0; ii < viaCnt; ii++) {
    odb::dbBox* v = viaTable[ii];

    if (v->isVisited())
      continue;
    v->setVisited(true);

    writeViaName(fp, v, 0, " ");
    if ((ii + 1) % 20 == 0)
      fprintf(fp, "\\ \n");
  }
}

void extMain::closeNanoFilesDomainVDD(char* netName)
{
  fclose(_subCktNodeFP[0][1]);
  fclose(_subCktNodeFP[1][1]);

  FILE* vddFP = openNanoFileNet(netName, "std", "VDD", "SUBCKT", "w");
  printItermNodeSubCkt(vddFP, _vddItermIdTable);
  printViaNodeSubCkt(vddFP, _viaM1_VDDtable);
  fclose(vddFP);

  vddFP = openNanoFileNet(netName, "glob", "VDD", "SUBCKT", "w");
  printViaNodeSubCkt(vddFP, _viaUp_VDDtable);
  fclose(vddFP);

  fclose(_blkInfoVDD);
  fclose(_viaInfoVDD);

  writeSubckt(_stdCirVDD, ".ends", "VDD", "std", "\n");
  fclose(_stdCirVDD);

  writeSubckt(_globCirVDD, ".ends", "VDD", "glob", "\n");
  fclose(_globCirVDD);

  fclose(_coordsVDD);

  addSubcktStatementDomain("glob_VDD.cir", "glob_VDD.subckt", netName);
  addSubcktStatementDomain("std_VDD.cir", "std_VDD.subckt", netName);
  fclose(_viaStackGlobVDD);
}
void extMain::closeNanoFilesDomainGND(char* netName)
{
  fclose(_subCktNodeFP[0][0]);
  fclose(_subCktNodeFP[1][0]);

  FILE* gndFP = openNanoFileNet(netName, "std", "GND", "SUBCKT", "w");
  printItermNodeSubCkt(gndFP, _gndItermIdTable);
  printViaNodeSubCkt(gndFP, _viaM1_GNDtable);
  fclose(gndFP);

  gndFP = openNanoFileNet(netName, "glob", "GND", "SUBCKT", "w");
  printViaNodeSubCkt(gndFP, _viaUp_GNDtable);
  fclose(gndFP);

  fclose(_blkInfoGND);
  fclose(_viaInfoGND);

  writeSubckt(_stdCirGND, ".ends", "GND", "std", "\n");
  fclose(_stdCirGND);

  writeSubckt(_globCirGND, ".ends", "GND", "glob", "\n");
  fclose(_globCirGND);

  fclose(_coordsGND);

  addSubcktStatementDomain("glob_GND.cir", "glob_GND.subckt", netName);
  addSubcktStatementDomain("std_GND.cir", "std_GND.subckt", netName);
  fclose(_viaStackGlobGND);
}
void extMain::closeNanoFiles()
{
  fclose(_subCktNodeFP[0][0]);
  fclose(_subCktNodeFP[1][0]);
  fclose(_subCktNodeFP[0][1]);
  fclose(_subCktNodeFP[1][1]);

  FILE* vddFP = openNanoFile("std", "VDD", "SUBCKT", "w");
  printItermNodeSubCkt(vddFP, _vddItermIdTable);
  printViaNodeSubCkt(vddFP, _viaM1_VDDtable);
  fclose(vddFP);

  vddFP = openNanoFile("glob", "VDD", "SUBCKT", "w");
  printViaNodeSubCkt(vddFP, _viaUp_VDDtable);
  fclose(vddFP);

  FILE* gndFP = openNanoFile("std", "GND", "SUBCKT", "w");
  printItermNodeSubCkt(gndFP, _gndItermIdTable);
  printViaNodeSubCkt(gndFP, _viaM1_GNDtable);
  fclose(gndFP);

  gndFP = openNanoFile("glob", "GND", "SUBCKT", "w");
  printViaNodeSubCkt(gndFP, _viaUp_GNDtable);
  fclose(gndFP);

  fclose(_blkInfoVDD);
  fclose(_viaInfoVDD);
  fclose(_blkInfoGND);
  fclose(_viaInfoGND);

  writeSubckt(_stdCirVDD, ".ends", "VDD", "std", "\n");
  fclose(_stdCirVDD);

  writeSubckt(_globCirVDD, ".ends", "VDD", "glob", "\n");
  fclose(_globCirVDD);

  writeSubckt(_stdCirGND, ".ends", "GND", "std", "\n");
  fclose(_stdCirGND);

  writeSubckt(_globCirGND, ".ends", "GND", "glob", "\n");
  fclose(_globCirGND);

  fclose(_coordsVDD);
  fclose(_coordsGND);

  addSubcktStatement("glob_VDD.cir", "glob_VDD.subckt");
  addSubcktStatement("glob_GND.cir", "glob_GND.subckt");
  addSubcktStatement("std_VDD.cir", "std_VDD.subckt");
  addSubcktStatement("std_GND.cir", "std_GND.subckt");
  /*
          system("mv glob_VDD.cir glob_VDD.cir.TMP ; cat glob_VDD.subckt
     glob_VDD.cir.TMP > glob_VDD.cir"); system("mv glob_GND.cir glob_GND.cir.TMP
     ; cat std_GND.subckt glob_GND.cir.TMP > glob_GND.cir"); system("mv
     std_VDD.cir std_VDD.cir.TMP ; cat std_VDD.subckt std_VDD.cir.TMP >
     std_VDD.cir"); system("mv std_GND.cir std_GND.cir.TMP ; cat std_GND.subckt
     std_GND.cir.TMP > std_GND.cir");
          */
  //.ends VDD_std_jpeg_e_ring_bad
  fclose(_viaStackGlobGND);
  fclose(_viaStackGlobVDD);
}
void extMain::addSubcktStatementDomain(const char* cirFile1,
                                       const char* subcktFile1,
                                       const char* netName)
{
  char cirFile[256];
  char subcktFile[256];
  if (strlen(_node_blk_prefix) > 0) {
    char prefix[256];
    sprintf(cirFile,
            "%s/%s/%s%s",
            _node_blk_dir,
            netName,
            _node_inst_prefix,
            cirFile1);
    sprintf(subcktFile,
            "%s/%s/%s%s",
            _node_blk_dir,
            netName,
            _node_inst_prefix,
            subcktFile1);
  } else {
    sprintf(cirFile, "%s/%s", netName, cirFile1);
    sprintf(subcktFile, "%s/%s", netName, subcktFile1);
  }

  char cmd[2048];
  sprintf(cmd,
          "mv %s %s.TMP ; cat %s %s.TMP > %s",
          cirFile,
          cirFile,
          subcktFile,
          cirFile,
          cirFile);

  system(cmd);

  // system("mv glob_VDD.cir glob_VDD.cir.TMP ; cat glob_VDD.subckt
  // glob_VDD.cir.TMP > glob_VDD.cir");
}
void extMain::addSubcktStatement(const char* cirFile1, const char* subcktFile1)
{
  char prefix[256];
  char cirFile[256];
  char subcktFile[256];
  if (strlen(_node_blk_prefix) > 0) {
    setPrefix(prefix);

    sprintf(cirFile, "%s%s", prefix, cirFile1);
    sprintf(subcktFile, "%s%s", prefix, subcktFile1);
  } else {
    sprintf(cirFile, "%s", cirFile1);
    sprintf(subcktFile, "%s", subcktFile1);
  }

  char cmd[2048];
  sprintf(cmd,
          "mv %s %s.TMP ; cat %s %s.TMP > %s",
          cirFile,
          cirFile,
          subcktFile,
          cirFile,
          cirFile);
  system(cmd);

  // system("mv glob_VDD.cir glob_VDD.cir.TMP ; cat glob_VDD.subckt
  // glob_VDD.cir.TMP > glob_VDD.cir");
}
bool extMain::isSignalNet(odb::dbNet* net)
{
  odb::dbSigType type = net->getSigType();

  return ((type == odb::dbSigType::POWER) || (type == odb::dbSigType::GROUND))
             ? false
             : true;
}
// COMMENT 04/17/2013
// Start Connecting from the Top Layers and connect to the first Iterm Shape
uint extMain::iterm2Vias(odb::dbInst* inst, odb::dbNet* net)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();

  const char* ptype = "POWER";
  if (net->getSigType() == odb::dbSigType::GROUND)
    ptype = "GROUND";

  std::vector<odb::dbBox*> connShapeTable[32];
  std::vector<odb::dbITerm*> connItermTable[32];

  odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
  odb::dbSet<odb::dbITerm>::iterator iitr;

  for (iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
    odb::dbITerm* tr = *iitr;

    if (tr->getSigType() != net->getSigType())
      continue;
    if (tr->getNet() != net)
      continue;

    odb::dbStringProperty* p = odb::dbStringProperty::find(tr, "_inode");
    if (p != NULL)
      continue;  // already connected thru wire connection

    uint vcnt = 0;

    odb::dbITermShapeItr term_shapes;
    odb::dbShape s;
    for (term_shapes.begin(tr); term_shapes.next(s);) {
      if (s.isVia())
        continue;

      std::vector<odb::dbBox*> viaTable;
      uint level = s.getTechLayer()->getRoutingLevel();
      if (level == 1)
        continue;

      uint viaCnt = blkSearch->getPowerWiresAndVias(
          s.xMin(), s.yMin(), s.xMax(), s.yMax(), level, net, false, viaTable);

      if (viaCnt == 0)
        continue;

      for (uint ii = 0; ii < viaCnt; ii++) {
        odb::dbBox* w = viaTable[ii];
        odb::dbSWire* swire = (odb::dbSWire*) w->getBoxOwner();
        if (swire->getNet() != net)
          continue;

        uint top;
        uint bot = blkSearch->getViaLevels(w, top);
        if (top < 3)
          continue;
        if (!((level == bot) || (level == top)))
          continue;

        if (w->xMin() > s.xMax())
          continue;
        if (w->yMin() > s.yMax())
          continue;
        if (w->xMax() < s.xMin())
          continue;
        if (w->yMax() < s.yMin())
          continue;

        connShapeTable[top].push_back(w);
        connItermTable[top].push_back(tr);
      }
    }
  }
  for (int level = 31; level > 1; level--) {
    if (connShapeTable[level].size() == 0)
      continue;

    for (int jj = 0; jj < connShapeTable[level].size(); jj++) {
      odb::dbBox* w = connShapeTable[level][jj];
      odb::dbITerm* tr = connItermTable[level][jj];

      if (tr->isSetMark())
        continue;

      if (!inst->getMaster()->isSpecialPower()) {
        char* srcNode = getViaResNode(w, "_up_node");
        if (srcNode == NULL)
          continue;

        // fprintf(_blkInfo, "%d %d %d %d i2v  %s %s %s [%s] ",
        // s.xMin(),s.yMin(),s.xMax(),s.yMax(),
        fprintf(_blkInfo,
                "%s %s %s [%s] ",
                inst->getConstName(),
                inst->getMaster()->getConstName(),
                // getBlockType(inst->getMaster()),
                "is_macro",
                tr->getMTerm()->getConstName());

        fprintf(_blkInfo, "%s\n", srcNode);
        _viaUpTable->push_back(w);
        // connect upwards with vias that coonect to the upper grid
        w->setVisited(false);
        std::vector<odb::dbBox*> upViaTable;
        uint cnt = blkSearch->getPowerWiresAndVias(w->xMin(),
                                                   w->yMin(),
                                                   w->xMax(),
                                                   w->yMax(),
                                                   level,
                                                   net,
                                                   false,
                                                   upViaTable);

        if (mergeStackedViasOpt(
                _globCir, net, upViaTable, w, _viaStackGlobCir, level)
            > 0)
          _via_id_map[w->getId()] = net;
        else
          _via_id_map[w->getId()] = NULL;

        logger_->info(RCX,
                      336,
                      "\tTERM {} is Connected at {}",
                      tr->getMTerm()->getConstName(),
                      srcNode);
        tr->setMark(1);
        break;
      }

      // w->setUserFlag1();
    }
  }
  return 0;
}
/* COMMENT 04/17/2013
uint extMain::iterm2Vias(odb::dbInst *inst, odb::dbNet *net)
{
        odb::dbBlockSearch *blkSearch= _block->getSearchDb();

        const char *ptype= "POWER";
        if (net->getSigType()==odb::dbSigType::GROUND)
                ptype= "GROUND";

        std::vector<odb::dbBox *> connShapeTable[32];

        odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
        odb::dbSet<odb::dbITerm>::iterator iitr;

        for(iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
                odb::dbITerm *tr = *iitr;

                if (tr->getSigType() != net->getSigType())
                        continue;
                if (tr->getNet() != net)
                        continue;

                odb::dbStringProperty *p= odb::dbStringProperty::find(tr,
"_inode"); if (p!=NULL) continue; // already connected thru wire connection

                uint vcnt= 0;

                odb::dbITermShapeItr term_shapes;
                odb::odb::dbShape s;
                for( term_shapes.begin(tr); term_shapes.next(s); )
                {
                        if (s.isVia())
                                continue;

                        std::vector<odb::dbBox *> viaTable;
                        uint level= s.getTechLayer()->getRoutingLevel();
                        if (level==1)
                                continue;

                        uint viaCnt= blkSearch->getPowerWiresAndVias(
                                s.xMin(),s.yMin(),s.xMax(),s.yMax(),
                                        level, net, false, viaTable);

                        if (viaCnt==0)
                                continue;

                        for (uint ii= 0; ii<viaCnt; ii++)
                        {
                                odb::dbBox *w= viaTable[ii];

                                //if (w->getUserFlag1())
                                        //continue;

                                uint top;
                                uint bot= blkSearch->getViaLevels(w, top);
                                if (top<3)
                                        continue;
                                if (!((level==bot) || (level==top)))
                                        continue;

                                if (w->xMin()>s.xMax())
                                        continue;
                                if (w->yMin()>s.yMax())
                                        continue;
                                if (w->xMax()<s.xMin())
                                        continue;
                                if (w->yMax()<s.yMin())
                                        continue;
//ADD resistor between iterm and Via
//have to mark vias for double counting

                                if (!inst->getMaster()->isSpecialPower())
                                {
                                        char * srcNode= getViaResNode(w,
"_up_node"); if (srcNode==NULL) continue;

                                        //fprintf(_blkInfo, "%d %d %d %d i2v  %s
%s %s [%s] ",
//s.xMin(),s.yMin(),s.xMax(),s.yMax(),
                                        fprintf(_blkInfo, "%s %s [%s] ",
                                                inst->getConstName(),
                                                inst->getMaster()->getConstName(),
                                                getBlockType(inst->getMaster()),
                                                tr->getMTerm()->getConstName());

                                                fprintf(_blkInfo, "%s\n",
srcNode); _viaUpTable->push_back(w);
                                }

                                //w->setUserFlag1();

                                tr->setMark(1);
                                vcnt ++;
                        }
                }
        }
        return 0;
}
*/
uint extMain::iterm2Vias_cells(odb::dbInst* inst,
                               odb::dbITerm* connectedPowerIterm)
{
  if (inst->getMaster()->isSpecialPower())
    return 0;

  odb::dbBlockSearch* blkSearch = _block->getSearchDb();

  odb::dbNet* net = connectedPowerIterm->getNet();

  const char* ptype = "POWER";
  if (net->getSigType() == odb::dbSigType::GROUND)
    ptype = "GROUND";

  odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
  odb::dbSet<odb::dbITerm>::iterator iitr;

  for (iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
    odb::dbITerm* tr = *iitr;

    if (tr->isSetMark())
      continue;
    if (tr->getNet() != net)
      continue;
    if (tr == connectedPowerIterm)
      continue;

    odb::dbStringProperty* p = odb::dbStringProperty::find(tr, "_inode");
    if (p != NULL)
      continue;  // already connected thru wire connection

    uint vcnt = 0;

    odb::dbITermShapeItr term_shapes;
    odb::dbShape s;
    for (term_shapes.begin(tr); term_shapes.next(s);) {
      if (s.isVia())
        continue;

      std::vector<odb::dbBox*> viaTable;
      uint level = s.getTechLayer()->getRoutingLevel();

      uint viaCnt = blkSearch->getPowerWiresAndVias(
          s.xMin(), s.yMin(), s.xMax(), s.yMax(), level, net, false, viaTable);

      if (viaCnt == 0)
        continue;

      for (uint ii = 0; ii < viaCnt; ii++) {
        odb::dbBox* w = viaTable[ii];

        // if (w->getUserFlag1())
        // continue;

        uint top;
        uint bot = blkSearch->getViaLevels(w, top);

        if (!((level == bot) || (level == top)))
          continue;

        if (w->xMin() > s.xMax())
          continue;
        if (w->yMin() > s.yMax())
          continue;
        if (w->xMax() < s.xMin())
          continue;
        if (w->yMax() < s.yMin())
          continue;
        // ADD resistor between iterm and Via
        // have to mark vias for double counting

        fprintf(_blkInfo,
                "%s %s %s [%s] ",
                inst->getConstName(),
                inst->getMaster()->getConstName(),
                getBlockType(inst->getMaster()),
                tr->getMTerm()->getConstName());

        writeViaName(_blkInfo, w, 0, "\n");
        // fprintf(_blkInfo, " I%d\n", tr->getId());
        _viaUpTable->push_back(w);

        // w->setUserFlag1();

        tr->setMark(1);
        vcnt++;
      }
      if (tr->isSetMark())
        break;
    }
  }
  return 0;
}
uint extMain::mergeStackedViasOpt(FILE* fp,
                                  odb::dbNet* net,
                                  std::vector<odb::dbBox*>& viaSearchTable,
                                  odb::dbBox* botVia,
                                  FILE* fp1,
                                  uint viaStackLevel)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();
  odb::dbBox* v = botVia;

  char srcNode[128];
  char resName[128];
  char dstNode[128];

  float totRes = 0;
  if (!botVia->isVisited()) {
    totRes = getPowerViaRes(botVia, 0.1);
    botVia->setVisited(true);
    _stackedViaResCnt++;
  }
  float res = 0.0;
  uint viaCnt = viaSearchTable.size();

  std::vector<odb::dbBox*> viaLevelTable[16];
  for (uint ii = 0; ii < viaCnt; ii++) {
    odb::dbBox* w = viaSearchTable[ii];

    odb::Rect vr;
    v->getBox(vr);

    odb::Rect wr;
    w->getBox(wr);

    if (!wr.overlaps(vr))
      continue;

    uint top;
    uint bot = blkSearch->getViaLevels(w, top);
    viaLevelTable[bot].push_back(w);
  }

  odb::dbBox* topVia = NULL;
  int stackLevel = 0;
  int stackCnt = 0;
  std::vector<odb::dbBox*> viaTable;
  for (uint jj = viaStackLevel; jj < 16; jj++) {
    if (viaLevelTable[jj].size() == 0)
      continue;

    stackCnt++;

    odb::dbBox* a = viaLevelTable[jj][0];

    viaTable.push_back(a);

    if (_wireInfra && !(_via2JunctionMap->geti(a->getId()) > 0))
      continue;
    if (!_wireInfra) {
      if (jj == 1)
        continue;

      char* topNode = getViaResNode(a, "_down_node");
      if (topNode == NULL)
        topNode = getViaResNode(a, "_up_node");
      if (topNode == NULL)
        continue;
    }

    stackLevel = jj;
    topVia = a;
    break;
    // fprintf(stdout, "\tstack[%d] %d %d v%d\n ", jj, a->xMin(), a->yMin(),
    // a->getId());
  }
  viaCnt = viaTable.size();
  for (uint ii = 0; ii < viaCnt; ii++) {
    odb::dbBox* w = viaTable[ii];
    w->setVisited(false);
  }
  if ((topVia != NULL) && (stackCnt == stackLevel)) {
    // fprintf(stdout, "\tFOUND TOP stack[%d] %d %d v%d\n ", stackCnt,
    // topVia->xMin(), topVia->yMin(), topVia->getId());
  } else if (topVia == NULL) {
    if ((stackCnt == 0) || (viaTable.size() > stackCnt)) {
      return 0;
    }
    topVia = viaTable[viaTable.size() - 1];
  }

  for (uint ii = 0; ii < viaCnt; ii++) {
    odb::dbBox* w = viaTable[ii];
    uint vid = w->getId();

    res = getPowerViaRes(w, 0.1);
    totRes += res;
  }
  if (topVia == NULL) {
    logger_->warn(RCX,
                  337,
                  "Via_{} {} {} has no overlaps from above!",
                  botVia->getId(),
                  botVia->xMin(),
                  botVia->yMin());
    return 0;
  }
  if (botVia == topVia)  // TO_TEST????
  {
    /*
                    sprintf(srcNode,"");
                    char * botNode= getViaResNode(botVia, "_up_node");
                    if (botNode!=NULL)
                    sprintf(srcNode,"%s", botNode);
    */
    return 0;
  }

  uint top;
  uint bot = blkSearch->getViaLevels(botVia, top);

  sprintf(
      resName, "R_%d_%d_v%d ", botVia->xMin(), botVia->yMin(), botVia->getId());
  bot = blkSearch->getViaLevels(topVia, top);
  if (_wireInfra) {
    writeViaName(srcNode, botVia, bot, " ");
    writeViaName(dstNode, topVia, top, " ");
  } else {
    char* botNode = getViaResNode(botVia, "_up_node");
    sprintf(srcNode, "%s", botNode);
    char* topNode = getViaResNode(topVia, "_down_node");
    if (topNode == NULL)
      topNode = getViaResNode(topVia, "_up_node");
    if (topNode == NULL) {
      logger_->warn(RCX,
                    338,
                    "NOT CONNECTED M1: Via_{} {} {} {} {}",
                    botVia->getId(),
                    botVia->xMin(),
                    botVia->yMin(),
                    botVia->xMax(),
                    botVia->yMax());
      return 0;
    }
    sprintf(dstNode, "%s", topNode);
  }
  // fprintf(stdout, "\t\t%s %s %s %g\n", resName, srcNode, dstNode, totRes);
  fprintf(fp, "%s %s %s %g\n", resName, srcNode, dstNode, totRes);

  _totViaResCnt++;

  topVia->setMarked(true);

  if (fp1 != NULL) {
    fprintf(fp1, "%s %s %s %g\n", resName, srcNode, dstNode, totRes);
  }
  return 1;
}

uint extMain::mergeStackedVias(FILE* fp,
                               odb::dbNet* net,
                               std::vector<odb::dbBox*>& viaSearchTable,
                               odb::dbBox* botVia,
                               FILE* fp1)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();
  odb::dbBox* v = botVia;
  odb::dbBox* w = NULL;

  float totRes = 0;
  if (!botVia->isVisited()) {
    totRes = getPowerViaRes(botVia, 0.1);
    botVia->setVisited(true);
    _stackedViaResCnt++;
  }
  float res = 0.0;
  uint viaCnt = viaSearchTable.size();

  std::vector<odb::dbBox*> viaLevelTable[16];
  for (uint ii = 0; ii < viaCnt; ii++) {
    w = viaSearchTable[ii];

    uint top;
    uint bot = blkSearch->getViaLevels(w, top);
    viaLevelTable[bot].push_back(w);
    // uint vid= w->getId();
  }

  std::vector<odb::dbBox*> viaTable;
  for (uint jj = 1; jj < 16; jj++) {
    if (viaLevelTable[jj].size() == 0)
      continue;

    odb::dbBox* a = viaLevelTable[jj][0];
    viaTable.push_back(a);
  }
  fprintf(stdout,
          "M1: Via_%d %d %d \n",
          botVia->getId(),
          botVia->xMin(),
          botVia->yMin());
  viaCnt = viaTable.size();
  for (uint ii = 0; ii < viaCnt; ii++) {
    w = viaTable[ii];
    uint vid = w->getId();

    fprintf(stdout,
            "\t ---- mergeStackedVias %d %d v%d\n ",
            w->xMin(),
            w->yMin(),
            botVia->getId());
    if (w->getId() == 283584) {
    }
    if (w->isVisited())
      continue;

    if (w->xMin() > v->xMax())
      continue;
    if (w->yMin() > v->yMax())
      continue;
    if (w->xMax() < v->xMin())
      continue;
    if (w->yMax() < v->yMin())
      continue;

    fprintf(stdout,
            "\tmergeStackedVias %d %d v%d\n ",
            w->xMin(),
            w->yMin(),
            botVia->getId());
    res = getPowerViaRes(w, 0.1);
    totRes += res;

    v = w;
    _stackedViaResCnt++;

    if (_via2JunctionMap->geti(w->getId()) > 0) {
      /*
                      fprintf(stdout, "M1: Via_%d %d %d \n",
                              botVia->getId(), botVia->xMin(), botVia->yMin());
      fprintf(stdout, "mergeStackedVias %d %d v%d\n ", w->xMin(), w->yMin(),
      botVia->getId());
      */
      break;
    }
    w->setVisited(true);
  }
  for (uint ii = 0; ii < viaCnt; ii++) {
    w = viaTable[ii];
    w->setVisited(false);
  }
  if (w == NULL) {
    logger_->warn(RCX,
                  339,
                  "Via_{} {} {} has no overlaps from above!",
                  botVia->getId(),
                  botVia->xMin(),
                  botVia->yMin());
    return 0;
  }
  if (botVia == w)  // TO_TEST????
    return 0;

  uint top;
  uint bot = blkSearch->getViaLevels(botVia, top);
  char resName[128];
  char srcNode[128];
  char dstNode[128];

  sprintf(
      resName, "R_%d_%d_v%d ", botVia->xMin(), botVia->yMin(), botVia->getId());
  writeViaName(srcNode, botVia, bot, " ");
  bot = blkSearch->getViaLevels(w, top);
  writeViaName(dstNode, w, top, " ");
  fprintf(stdout, "\t\t%s %s %s %g\n", resName, srcNode, dstNode, totRes);
  fprintf(fp, "%s %s %s %g\n", resName, srcNode, dstNode, totRes);
  /*
          fprintf(fp, "R_%d_%d_v%d ", botVia->xMin(), botVia->yMin(),
     botVia->getId()); writeViaName(fp, botVia, bot, " ");

          bot= blkSearch->getViaLevels(w, top);
          writeViaName(fp, w, top, " ");
          fprintf(fp, " %g\n", totRes);
  */
  _totViaResCnt++;

  w->setMarked(true);

  if (fp1 != NULL) {
    fprintf(fp1, "%s %s %s %g\n", resName, srcNode, dstNode, totRes);
  }
  return 1;
}

uint extMain::getPowerNets(std::vector<odb::dbNet*>& powerNetTable)
{
  odb::dbSet<odb::dbNet> nets = _block->getNets();
  odb::dbSet<odb::dbNet>::iterator net_itr;
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    odb::dbNet* net = *net_itr;

    if (isSignalNet(net))
      continue;

    powerNetTable.push_back(net);
  }
  return powerNetTable.size();
}
uint extMain::findHighLevelPinMacros(std::vector<odb::dbInst*>& instTable)
{
  odb::dbSet<odb::dbInst> insts = _block->getInsts();
  odb::dbSet<odb::dbInst>::iterator i_itr;
  for (i_itr = insts.begin(); i_itr != insts.end(); ++i_itr) {
    odb::dbInst* inst = *i_itr;
    /*
                    odb::dbBox *bb= inst->getBBox();
    */

    inst->clearUserFlag3();
    if (inst->getMaster()->getMTermCount() > 16) {
      instTable.push_back(inst);
      inst->setUserFlag3();
    }
  }
  return instTable.size();
}
bool extMain::topHierBlock()
{
  odb::dbSet<odb::dbBlock> children = _block->getChildren();
  if (children.size() > 0)
    return true;
  else
    return false;
}
void extMain::allocMappingTables(int n1, int n2, int n3)
{
  _junct2iterm = new Ath__array1D<odb::dbITerm*>(n1);
  _junct2viaMap = new Ath__array1D<int>(n2);
  _via2JunctionMap = new Ath__array1D<int>(n3);
}
void extMain::initMappingTables()
{
  uint n = _junct2iterm->getSize();
  for (uint kk = 0; kk < n; kk++)
    _junct2iterm->set(kk, NULL);

  n = _junct2viaMap->getSize();
  for (uint kk = 0; kk < n; kk++)
    _junct2viaMap->set(kk, 0);

  n = _via2JunctionMap->getSize();
  for (uint kk = 0; kk < n; kk++)
    _via2JunctionMap->set(kk, 0);
}
uint extMain::initPowerSearch()
{
  uint sigtype = 9;
  uint pwrtype = 11;

  uint pitchTable[16];
  uint widthTable[16];
  for (uint ii = 0; ii < 16; ii++) {
    pitchTable[ii] = 0;
    widthTable[ii] = 0;
  }
  uint dirTable[16];
  int baseX[16];
  int baseY[16];

  // if (_search!=NULL)
  // delete _search;
  // odb::Rect extRect;
  uint layerCnt = initSearchForNets(
      baseX, baseY, pitchTable, widthTable, dirTable, _extMaxRect, false);

  return layerCnt;
}
void extMain::setupDirNaming()
{
  sprintf(_node_blk_dir, "./");
  sprintf(_node_blk_prefix, "");
  sprintf(_node_inst_prefix, "");
  odb::dbInst* ii = _block->getParentInst();
  if (ii != NULL) {
    logger_->info(RCX,
                  340,
                  "Extract Hier Block {} name= {} of Inst {} {}",
                  _block->getId(),
                  _block->getConstName(),
                  ii->getId(),
                  ii->getConstName());

    _block->getDieArea(_extMaxRect);
    // odb::dbBox *bb= ii->getBBox();
    // bb->getBox(_extMaxRect);

    sprintf(_node_blk_dir, "B%d", _block->getId());
    sprintf(_node_blk_prefix, "B%d_", _block->getId());
    sprintf(_node_inst_prefix, "%s_", ii->getConstName());
  } else {
    odb::dbSet<odb::dbBlock> children = _block->getChildren();
    if (children.size() > 0) {
      sprintf(_node_blk_dir, "B%d", _block->getId());
      sprintf(_node_blk_prefix, "B%d_", _block->getId());
      sprintf(_node_inst_prefix, "%s_", _block->getConstName());

      logger_->info(RCX,
                    341,
                    "Extract Top Block {} name= {} ",
                    _block->getId(),
                    _block->getConstName());

    } else {
      logger_->info(RCX,
                    342,
                    "Extract Block {} name= {}",
                    _block->getId(),
                    _block->getConstName());
    }
    _block->getDieArea(_extMaxRect);
  }
  logger_->info(RCX,
                343,
                "nodeBlockPrefix={} nodeInstPrefix={}",
                _node_blk_prefix,
                _node_inst_prefix);
}

// START
uint extMain::powerRCGen()
{
  _wireInfra = false;

  _powerWireCnt = 0;
  _mergedPowerWireCnt = 0;
  _overlapPowerWireCnt = 0;
  _viaOverlapPowerCnt = 0;
  _multiViaCnt = 0;

  _nodeCoords = true;  // coord based node naming
  _netUtil = new odb::dbCreateNetUtil();
  _netUtil->setBlock(_block, false);
  _dbgPowerFlow = false;

  allocMappingTables(100000, 10000, 10000000);

  setupDirNaming();

  logger_->info(RCX,
                344,
                "Extract Bounding Box {} {} {} {}",
                _extMaxRect.xMin(),
                _extMaxRect.yMin(),
                _extMaxRect.xMax(),
                _extMaxRect.yMax());

  _supplyViaMap[0] = NULL;  // check to see whether
  if (_power_source_file != NULL)
    readPowerSupplyCoords(_power_source_file);

  odb::dbSet<odb::dbTechLayer> layers = _tech->getLayers();
  odb::dbSet<odb::dbTechLayer>::iterator itr;

  // openNanoFiles();
  //_junct2iterm= new Ath__array1D<odb::dbITerm*>(100000);
  //_junct2viaMap= new Ath__array1D<int>(10000);
  //_via2JunctionMap= new Ath__array1D<int>(10000000);

  findHighLevelPinMacros(_powerMacroTable);
  logger_->info(RCX, 345, "Found {:lu} macro blocks", _powerMacroTable.size());

  markExcludedCells();

  std::vector<odb::dbNet*> powerNetTable;
  uint pNetCnt = getPowerNets(powerNetTable);

  // odb::dbBox *bb = _block->getBBox();
  char msg1[1024];
  sprintf(msg1, "\nExtracting %d nets ", pNetCnt);
  AthResourceLog(msg1, 0);

  // for (int ii= pNetCnt-1; ii>=0; ii--)
  for (uint ii = 0; ii < pNetCnt; ii++) {
    _sbox_id_map.clear();

    _viaM1_VDDtable.clear();
    _viaUp_VDDtable.clear();
    _viaM1_GNDtable.clear();
    _viaUp_GNDtable.clear();

    _viaUpperTable[0].clear();
    _viaUpperTable[1].clear();

    initMappingTables();

    odb::dbNet* net = powerNetTable[ii];

    openNanoFilesDomain(net);
    setupNanoFiles(net);
    _netUtil->setCurrentNet(net);

    logger_->info(RCX, 346, "Extracting net {} ... ", net->getConstName());

    // 103012 Release railConn(net);

    railConnOpt(net);

    if (_nodeCoords) {
      writeViaResistors(_globCirVDD, 1, _viaStackGlobVDD, !_wireInfra);
      writeViaResistors(_globCirGND, 0, _viaStackGlobGND, !_wireInfra);
    }
    sprintf(msg1, "\nFinished %s ", net->getConstName());
    AthResourceLog(msg1, 0);

    for (uint jj = 0; jj < _powerMacroTable.size(); jj++) {
      odb::dbInst* inst = _powerMacroTable[jj];
      logger_->info(RCX,
                    347,
                    "--- Connectivity of macro {} with net {} ... ",
                    inst->getConstName(),
                    net->getConstName());
      iterm2Vias(inst, net);
      AthResourceLog("-- Finished Macro", 0);
    }

    bool m1Vias = true;
    writeViasAndClose(net, m1Vias);
  }
  if (!_nodeCoords) {
    writeViaResistors(_globCirVDD, 1, _viaStackGlobVDD);
    writeViaResistors(_globCirGND, 0, _viaStackGlobGND);
  }

  return 0;
}
void extMain::writeViasAndClose(odb::dbNet* net, bool m1Vias)
{
  writeViaInfo(_viaInfoVDD, true);
  writeViaInfo(_viaInfoGND, false);
  writeViaInfo(_viaInfoVDD, _viaUp_VDDtable, !m1Vias, true);
  writeViaInfo(_viaInfoVDD, _viaM1_VDDtable, m1Vias, true);

  writeViaInfo(_viaInfoGND, _viaUp_GNDtable, !m1Vias, false);
  writeViaInfo(_viaInfoGND, _viaM1_GNDtable, m1Vias, false);

  writeViaCoords(_coordsGND, _viaM1_GNDtable, m1Vias);
  writeViaCoords(_coordsVDD, _viaM1_VDDtable, m1Vias);
  writeViaCoords(_coordsGND, _viaUp_GNDtable, !m1Vias);
  writeViaCoords(_coordsVDD, _viaUp_VDDtable, !m1Vias);

  if (net->getSigType() == odb::dbSigType::POWER)
    closeNanoFilesDomainVDD((char*) net->getConstName());
  else
    closeNanoFilesDomainGND((char*) net->getConstName());
}
void extMain::writeViaName_xy(char* nodeName,
                              odb::dbBox* v,
                              uint bot,
                              uint top,
                              uint level,
                              const char* post)
{
  odb::dbSWire* s = (odb::dbSWire*) v->getBoxOwner();
  odb::dbNet* net = s->getNet();

  bool change = true;
  if (change) {
    // HERE NODE_NAMING
    if (level <= 2)
      level = 1;

    char* srcNode = getViaResNode(v, "_up_node");

    char buf[2048];
    odb::dbStringProperty* p = odb::dbStringProperty::find(v, "_inode");
    if (p != NULL)
      sprintf(nodeName, "%s_%d%s", p->getValue().c_str(), level, post);
    else
      writeNegativeCoords(
          nodeName, net->getId(), v->xMin(), v->yMin(), level, post);
    return;
  }

  if (level == 1)
    sprintf(nodeName, "n%d_%d_%d%s ", net->getId(), v->xMin(), v->yMin(), post);
  else if (level > 1) {
    if (bot == level)
      sprintf(
          nodeName, "n%d_%d_%d%s ", net->getId(), v->xMin(), v->yMin(), post);
    else
      sprintf(
          nodeName, "n%d_%d_%d%s ", net->getId(), v->xMax(), v->yMax(), post);
  } else
    sprintf(nodeName, "n%d_%d_%d ", net->getId(), v->xMin(), v->yMin());
}
void extMain::writeViaName(char* nodeName,
                           odb::dbBox* v,
                           uint level,
                           const char* post)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();
  uint top;
  uint bot = blkSearch->getViaLevels(v, top);
  if (_nodeCoords) {
    writeViaName_xy(nodeName, v, bot, top, level, post);
    return;
  }
  if (level == 1)
    sprintf(nodeName, "v%d_%d%s", v->getId(), level, post);
  else if (level > 1)
    sprintf(nodeName, "vM%dM%d_%d_%d%s", bot, top, level, v->getId(), post);
  else
    sprintf(nodeName, "vM%dM%d_%d%s", bot, top, v->getId(), post);
}
void extMain::writeViaName(FILE* fp,
                           odb::dbBox* v,
                           uint level,
                           const char* post)
{
  if (!_wireInfra) {
    char* srcNode = getViaResNode(v, "_up_node");
    if (srcNode != NULL)
      fprintf(fp, "%s%s", srcNode, post);

    return;
  }
  char nodeName[64];
  writeViaName(nodeName, v, level, post);
  fprintf(fp, "%s", nodeName);
}
void extMain::writeViaNameCoords(FILE* fp, odb::dbBox* v)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();
  uint top;
  uint bot = blkSearch->getViaLevels(v, top);

  writeViaName(fp, v, 0, "");
  fprintf(fp, " %d %d  %d %d\n", v->xMin(), v->yMin(), v->getDX(), v->getDY());
  writeViaName(fp, v, bot, "");
  fprintf(fp, " %d %d  %d %d\n", v->xMin(), v->yMin(), v->getDX(), v->getDY());
  writeViaName(fp, v, top, "");
  fprintf(fp, " %d %d  %d %d\n", v->xMin(), v->yMin(), v->getDX(), v->getDY());
}
void extMain::writeViaInfo(FILE* fp, bool power)
{
  if (fp == NULL)
    return;  // called from GND for VDD net ; 01282013
  if (_powerSourceTable[power].size() == 0)
    return;

  for (uint ii = 0; ii < _powerSourceTable[power].size(); ii++) {
    char* sname = _powerSourceTable[power][ii];
    fprintf(fp, "@%s\n", sname);
  }
  for (uint ii = 0; ii < _powerSourceTable[power].size(); ii++) {
    char* sname = _powerSourceTable[power][ii];
    fprintf(fp, "%s\n", sname);
  }
}
uint extMain::writeViaInfo(FILE* fp,
                           std::vector<odb::dbBox*>& viaTable,
                           bool m1Vias,
                           bool power)
{
  uint viaCnt = viaTable.size();
  if (viaCnt == 0)
    return 0;

  odb::dbBlockSearch* blkSearch = _block->getSearchDb();

  for (uint ii = 0; ii < viaCnt; ii++) {
    odb::dbBox* v = viaTable[ii];
    v->setVisited(false);
  }

  if (m1Vias) {
    if (_powerSourceTable[power].size() == 0) {
      odb::dbBox* v = viaTable[0];
      fprintf(fp, "@");

      uint top;
      uint bot = blkSearch->getViaLevels(v, top);

      writeViaName(fp, v, bot, "\n");
    }
  }

  for (uint ii = 0; ii < viaCnt; ii++) {
    odb::dbBox* v = viaTable[ii];
    if (v->isVisited())
      continue;
    v->setVisited(true);

    if (m1Vias) {
      if (_via_id_map[v->getId()] != NULL) {
        uint top;
        uint bot = blkSearch->getViaLevels(v, top);
        writeViaName(fp, v, bot, "\n");
      }
      continue;
    }
    fprintf(fp, "[");
    writeViaName(fp, v, 0, "]\n");
  }
  fprintf(fp, "\n");
  return viaCnt;
}
uint extMain::writeViaCoords(FILE* fp,
                             std::vector<odb::dbBox*>& viaTable,
                             bool m1Vias)
{
  uint viaCnt = viaTable.size();
  if (viaCnt == 0)
    return 0;

  for (uint ii = 0; ii < viaCnt; ii++) {
    odb::dbBox* v = viaTable[ii];
    if (m1Vias) {
      fprintf(fp, "std_via ");
      writeViaNameCoords(fp, v);
      continue;
    }
    fprintf(fp, "glob_via ");
    writeViaNameCoords(fp, v);
  }
  fprintf(fp, "\n");
  return viaCnt;
}
uint extMain::writeViaInfo_old(FILE* fp,
                               std::vector<odb::dbBox*>& viaTable,
                               bool m1Vias)
{
  odb::dbBlockSearch* blkSearch = _block->getSearchDb();

  uint viaCnt = viaTable.size();
  if (viaCnt == 0)
    return 0;

  if (m1Vias) {
    odb::dbBox* v = viaTable[0];
    // fprintf(fp, "@vM1_%d\n", v->getId());
    uint top;
    uint bot = blkSearch->getViaLevels(v, top);
    fprintf(
        fp, "@vM%dM%d_%d_%d_%d\n", bot, top, v->getId(), v->xMin(), v->yMin());
  }

  for (uint ii = 0; ii < viaCnt; ii++) {
    odb::dbBox* v = viaTable[ii];
    uint top;
    uint bot = blkSearch->getViaLevels(v, top);
    if (m1Vias) {
      // fprintf(fp, "vM1_%d\n", v->getId());
      fprintf(
          fp, "vM%dM%d_%d_%d_%d\n", bot, top, v->getId(), v->xMin(), v->yMin());
      continue;
    }
    fprintf(
        fp, "[vM%dM%d_%d_%d_%d]\n", bot, top, v->getId(), v->xMin(), v->yMin());
  }
  fprintf(fp, "\n");
  return viaCnt;
}

float extMain::computeViaResistance(odb::dbBox* viaBox, uint& cutCount)
{
  cutCount = 1;
  std::vector<odb::dbShape> shapes;
  viaBox->getViaBoxes(shapes);

  if (shapes.size() == 0)
    return 0;

  std::vector<odb::dbShape>::iterator shape_itr;

  uint cut_count = 0;
  float totRes = 0.0;
  for (shape_itr = shapes.begin(); shape_itr != shapes.end(); ++shape_itr) {
    odb::dbShape s = *shape_itr;

    if (s.getTechLayer()->getType() != odb::dbTechLayerType::CUT)
      continue;

    cut_count++;
    totRes += s.getTechLayer()->getResistance();
  }
  float avgCutRes = totRes / cut_count;

  float Res = avgCutRes / cut_count;
  cutCount = cut_count;
  return Res;
}
uint extMain::readPowerSupplyCoords(char* filename)
{
  int layerCount = _tech->getRoutingLayerCount() + 1;
  for (uint ii = 0; ii < 2; ii++) {
    _supplyViaMap[ii] = new Ath__array1D<char*>*[layerCount];
    _supplyViaTable[ii] = new Ath__array1D<odb::dbBox*>*[layerCount];
    for (uint jj = 0; jj < layerCount; jj++) {
      _supplyViaMap[ii][jj] = new Ath__array1D<char*>(100000);
      _supplyViaTable[ii][jj] = new Ath__array1D<odb::dbBox*>(1024);
    }
  }
  int cnt = 0;
  int db_units = _block->getDbUnitsPerMicron();
  Ath__parser parser;
  // parser.addSeparator("\r");
  parser.openFile(filename);
  while (parser.parseNextLine() > 0) {
    if (parser.getWordCnt() < 5) {
      logger_->warn(
          RCX, 348, "Less than 5 tokens in line {}", parser.getLineNum());
      parser.printWords(NULL);
      continue;
    }
    char* layer_name = parser.get(3);
    odb::dbTechLayer* layer = _tech->findLayer(layer_name);
    if (layer == NULL) {
      logger_->warn(
          RCX, 349, "Layer {} might be undefined in LEF at line: ", layer_name);
      parser.printWords(NULL);
      continue;
    }
    if (layer->getRoutingLevel() >= _tech->getRoutingLayerCount()) {
      logger_->warn(RCX,
                    350,
                    "Layer Name {} cannot be the top layer at line: ",
                    layer_name);
      parser.printWords(NULL);
      logger_->warn(RCX, 351, "the above line will be skipped!");
      continue;
    }
    parser.printWords(NULL);
    bool power = false;
    if (parser.isKeyword(4, "POWER")) {
      power = true;
    }
    char* source_name = parser.get(0);
    double x_microns = parser.getDouble(1);
    int x = Ath__double2int(db_units * x_microns);
    double y_microns = parser.getDouble(2);
    int y = Ath__double2int(db_units * y_microns);

    int level = layer->getRoutingLevel();
    odb::dbTechLayer* layer_plus_1 = _tech->findRoutingLayer(level + 1);

    odb::dbTechVia* techVia = odb::dbTechVia::create(_tech, source_name);
    odb::dbBox* bot = odb::dbBox::create(techVia, layer, x, y, x + 1, y + 1);
    odb::dbBox* top
        = odb::dbBox::create(techVia, layer_plus_1, x, y, x + 1, y + 1);

    // odb::dbBox *v = _netUtil->createTechVia(x, y, x+1, y+1, level, level+1);
    odb::dbBox* v = techVia->getBBox();

    _supplyViaTable[power][level]->add(v);
    _supplyViaMap[power][level]->set(v->getId(), strdup(source_name));

    cnt++;
    char* sname = _supplyViaMap[power][level]->geti(v->getId());
    logger_->info(RCX,
                  352,
                  "{} {} {} {} {} -- viaId= {} viaBoxId={} {} {}",
                  sname,
                  x,
                  y,
                  level,
                  power,
                  techVia->getId(),
                  v->getId(),
                  bot->getId(),
                  top->getId());
  }
  logger_->info(RCX, 353, "Have read {} power/ground sources", cnt);
  return cnt;
}
char* extMain::getPowerSourceName(bool power, uint level, uint vid)
{
  if (_supplyViaMap[0] == NULL)
    return NULL;

  return _supplyViaMap[power][level]->geti(vid);
}
uint extMain::addPowerSources(std::vector<odb::dbBox*>& viaTable,
                              bool power,
                              uint level,
                              odb::Rect* powerWire)
{
  if (_supplyViaTable[0] == NULL)
    return 0;

  uint n = _supplyViaTable[power][level]->getCnt();
  uint cnt = 0;
  for (uint ii = 0; ii < n; ii++) {
    odb::dbBox* v = _supplyViaTable[power][level]->get(ii);
    odb::Rect via;
    v->getBox(via);
    if (!powerWire->contains(via))
      continue;
    // v->setMarked(true);
    viaTable.push_back(v);
    cnt++;
    logger_->info(RCX,
                  354,
                  "    viaSource@ {} {}  {} {}",
                  v->xMin(),
                  v->yMin(),
                  v->xMax(),
                  v->yMax());
    logger_->info(RCX,
                  355,
                  "    connected with power wire at level {} :  {} {}  {} {}",
                  level,
                  powerWire->xMin(),
                  powerWire->yMin(),
                  powerWire->xMax(),
                  powerWire->yMax());
  }
  if (cnt > 0)
    logger_->info(RCX,
                  356,
                  "added {} [type={}] power/ground sources on level {}",
                  cnt,
                  power,
                  level);
  return n;
}
char* extMain::getPowerSourceName(uint level, uint vid)
{
  odb::dbNet* net = _netUtil->getCurrentNet();
  bool isVDDnet = false;
  if (net->getSigType() == odb::dbSigType::POWER)
    isVDDnet = true;

  char* name = getPowerSourceName(isVDDnet, level, vid);
  return name;
}
void extMain::addPowerSourceName(uint ii, char* sname)
{
  _powerSourceTable[ii].push_back(sname);
}

}  // namespace rcx
