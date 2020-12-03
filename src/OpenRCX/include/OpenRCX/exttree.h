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

#ifndef ADS_EXTRCTREE_H
#define ADS_EXTRCTREE_H

#include "odb.h"

#include "db.h"
#include "dbShape.h"
#include "util.h"

#include "ZObject.h"

namespace OpenRCX {

class extRCnode
{
  friend class extRcTree;

  // private:
 public:
  double _gndcap[ADS_MAX_CORNER];
  double _cap[ADS_MAX_CORNER];
  double _res[ADS_MAX_CORNER];
  int    _x;
  int    _y;
  uint   _firstChild;
  int    _termMap;
  uint   _netId;
  uint   _junctionId;
  uint   _capndId;
  uint   _splitCnt;

 public:
  void reset(uint cornerCnt);
};
class extTnode
{
 public:
  double     _gndcap[ADS_MAX_CORNER];
  double     _cap[ADS_MAX_CORNER];
  double     _res[ADS_MAX_CORNER];
  int        _x;
  int        _y;
  int        _termMap;
  uint       _netId;
  uint       _junctionId;
  uint       _capndId;
  uint       _splitCnt;
  uint       _childCnt;
  extTnode** _child;

  extTnode(extRCnode* m, uint cornerCnt);
  extTnode(void){};
  void printTnodes(char* type, uint cornerCnt);
};

class extRcTree
{
 public:
  extRcTree(odb::dbBlock* blk);
  ~extRcTree();
  extTnode*  makeTree(odb::dbNet* net,
                      double      max_cap,
                      uint        test,
                      bool        resetFlag,
                      bool        addDummyJunctions,
                      uint&       tnodeCnt,
                      double      mcf           = 1.0,
                      bool        for_buffering = false,
                      uint        extCorner     = 0,
                      bool        is_rise       = false,
                      bool        is_min        = false);
  extRCnode* makeTree2(odb::dbNet* net,
                       double      max_cap,
                       uint        test,
                       bool        resetFlag,
                       bool        addDummyJunctions = true,
                       double      mcf               = 1.0);
  extRCnode* makeTree(odb::dbNet*         net,
                      double              max_cap,
                      uint                test,
                      bool                resetFlag,
                      bool                addDummyJunctions = true,
                      double              mcf               = 1.0,
                      odb::dbBlockSearch* blk               = NULL,
                      bool                for_buffering     = false,
                      uint                extCorner         = 0,
                      bool                is_rise           = false,
                      bool                is_min            = false);
  extTnode*  makeTree(uint   netId,
                      double max_cap,
                      uint   test,
                      bool   resetFlag,
                      bool   addDummyJunctions,
                      uint&  tnodeCnt,
                      double mcf           = 1.0,
                      char*  printTag      = NULL,
                      bool   for_buffering = false);
  void       makeTree(double max_cap, uint test, bool for_buffering = false);
  uint       getDriverITermId();
  uint       getDriverBTermId();
  uint       getChildrenCnt(uint firstChild);
  extTnode*  makeTnode(uint nodeId, uint& n);
  uint       makeGraph(uint netId);

 private:
  void       initLocalCapNodeTable(odb::dbSet<odb::dbRSeg>& rSet);
  uint       netLocalCn(uint capNodeNum);
  extRCnode* init(odb::dbRSeg*             zrc,
                  odb::dbRSeg*             rc,
                  odb::dbSet<odb::dbRSeg>& rSet,
                  bool                     recycleFlag,
                  uint*                    id);
  void       getCoords(odb::dbNet* net,
                       uint        shapeId,
                       int*        x1,
                       int*        y1,
                       int*        x2,
                       int*        y2);
  extRCnode* allocNode(uint  childrenCnt,
                       uint* id,
                       bool  allocateChildren = true);
  uint       addChild(extRCnode* node, uint child);
  uint       printTree(FILE* fp, uint netId, const char* msg);
  bool       isTree(odb::dbNet* net);
  int        dfs(uint i, int* vis, odb::dbNet* net, uint l);
  uint       makeChildren(extRCnode* node, uint childrenCnt);
  uint       insertZeroJunctions();
  void       duplicateJunction(extRCnode* jnode, uint cnt);
  uint       getChildrenCnt(extRCnode* jnode);
  void       setDriverXY();
  extRCnode* makeFirstNode(odb::dbRSeg*    zrc,
                           odb::dbRSeg*    rc,
                           odb::dbCapNode* capNode,
                           uint            index);
  void       printTree(FILE* fp, uint netId, uint tnodeCnt);
  bool getCoords(odb::dbNet* net, uint shapeId, int* ll, int* ur, uint& length);
  extRCnode*   makeNode(uint            startingNodeId,
                        uint            endingNodeId,
                        odb::dbCapNode* tgtNode,
                        bool            fractionFlag,
                        int             x,
                        int             y,
                        double*         res,
                        double*         gndcap,
                        double*         totalcap,
                        extRCnode*      prevNode,
                        FILE*           dbgFP = NULL);
  uint         checkAndInit(odb::dbNet*              net,
                            bool                     resetFlag,
                            odb::dbSet<odb::dbRSeg>& rSet);
  bool         isDangling(odb::dbCapNode* node);
  void         printTest2(FILE*           dbgFP,
                          odb::dbCapNode* tgtNode,
                          extRCnode*      node,
                          uint            nodeId);
  odb::dbRSeg* getFirstRC(odb::dbSet<odb::dbRSeg>& rSet);
  FILE* openFile(odb::dbNet* net, const char* postfix, const char* permissions);

 public:
  AthPool<extRCnode>* _rcPool;  // recycle pool for extRCnode

  Ath__array1D<extRCnode*>* _junctionNodeTable;  // to figure out junctions
  Ath__array1D<extRCnode*>* _nodeTable;          // keeps pointers
  Ath__array1D<int>* _indexTable;  // keeps the children indices to _nodeTable
  Ath__array1D<int>* _itermIndexTable;  // keeps indices to iterm leaf nodes
  Ath__array1D<int>* _btermIndexTable;  // keeps indices to bterm leaf nodes
  Ath__array1D<int>* _map;
  extTnode**         _tnodeTable;
  uint               _tnodeCnt;

  odb::dbNet*   _net;
  odb::dbNet*   _cornerNet;
  uint          _cornerCnt;
  uint          _extCorner;
  uint          _blockCornerIndex;
  odb::dbBlock* _block;
  odb::dbBlock* _cornerBlock;
  uint          _btermId;
  uint          _itermId;

  uint _firstBTermIndex;
  uint _firstITermIndex;
  uint _itermCnt;
  uint _btermCnt;
  uint _driverNodeId;
  bool _printChildN;

  std::vector<uint> _cncpy;
  std::vector<uint> _netCapNodeNum;
  std::vector<uint> _netCapNodeMapBack;

  bool _foreign;

  bool _debug;

  static void free_exttree(extTnode* driver);
};

}  // namespace OpenRCX

#endif
