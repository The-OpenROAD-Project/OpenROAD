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

#pragma once

#include <stdio.h>
#include <wire.h>

#include "ISdb.h"
#include "IZdcr.h"
#include "dbId.h"
#include "dbObject.h"
#include "dbTypes.h"
#include "odb.h"

namespace odb {

// class Ath__searchBox;

class dbBlockSearch
{
 private:
  dbBlock* _block;
  dbTech* _tech;
  uint _blockId;

  ZPtr<ISdb> _signalNetSdb;
  ZPtr<ISdb> _netSdb;
  ZPtr<ISdb> _netViaSdb;
  ZPtr<ISdb> _instSdb;
  ZPtr<ISdb> _trackSdb;

  ZPtr<IZdcr> _dcr;

  uint _blockMenuId;

  uint _block_bb_id;
  uint _block_pin_id;
  uint _block_obs_id;
  uint _block_track_id;

  uint _instMenuId;

  uint _inst_bb_id;
  uint _inst_pin_id;
  uint _inst_obs_id;

  uint _signalMenuId;

  uint _signal_wire_id;
  uint _signal_via_id;
  uint _powerMenuId;

  uint _power_wire_id;
  uint _power_via_id;

  bool _skipCutBoxes;
  utl::Logger* logger_;

 public:
  dbBlockSearch(dbBlock* blk, dbTech* tech, utl::Logger* logger);
  ~dbBlockSearch();

  ZPtr<ISdb> getSignalNetSdb(ZContext& context);
  ZPtr<ISdb> getSignalNetSdb();
  void makeSignalNetSdb(ZContext& context);
  void resetSignalNetSdb();

  void resetNetSdb();
  ZPtr<ISdb> getNetSdb(ZContext& context);
  ZPtr<ISdb> getNetSdb();

  void initSubMenus();
  void initMenuIds();
  uint getBbox(int* x1, int* y1, int* x2, int* y2);
  void setViaCutsFlag(bool skipViaCuts);
  void makeSearchDB(bool nets, bool insts, ZContext& context);
  void makeTrackSdb(ZContext& context);
  void makeNetSdb(ZContext& context);
  void makeNetViaSdb(ZContext& context);
  uint makeInstSearchDb();
  void makeNetWireSearchDb();

  uint addViaCoordsFromWire(uint menuId,
                            uint subMenuId,
                            uint netId,
                            uint shapeId);
  uint getViasFromWires(ZPtr<ISdb> sdb,
                        uint menuId,
                        uint subMenuId,
                        uint wireMenuId,
                        dbNet* targetNet,
                        bool excludeFlag);

  uint addViaBoxes(dbShape& s,
                   uint menuId,
                   uint subMenuId,
                   uint id,
                   uint shapeId);
  uint addViaBoxes(dbBox* s,
                   uint menuId,
                   uint subMenuId,
                   uint netId,
                   uint shapeId);

  uint getViaLevels(dbShape& s, uint& top);
  uint getViaLevels(dbBox* s, uint& top);

  uint addBtermBoxes(dbSet<dbBTerm>& bterms, bool ignoreFlag);
  uint getBtermBoxes(bool ignoreFlag);
  uint addBoxes(dbBTerm* bterm);

  void getInstBoxes(bool ignoreFlag);
  void getInstShapes(bool vias, bool pins, bool obs);
  uint getItermShapes(dbInst* inst, bool viaFlag);
  uint getItermShapes(dbITerm* term, bool viaFlag);
  uint getInstObs(dbInst* inst, bool viaFlag);
  uint addInstShapes(dbInst* inst, bool vias, bool pinFlag, bool obsFlag);
  uint getWiresAndVias_all(dbNet* targetNet, bool ignoreFlag);
  uint addWireViaCoords(uint menuId,
                        uint subMenuId,
                        bool wireVia,
                        uint wireId,
                        uint shapeId);
  uint getWireVias(uint menuId,
                   uint subMenuId,
                   bool wireVia,
                   dbNet* targetNet,
                   bool excludeFlag);
  int getShapeLevel(dbShape* s, bool wireVia);
  int getShapeLevel(dbSBox* s, bool wireVia);

  uint getItermShapesWithViaShapes(dbITerm* iterm);
  uint getItermShapesNoVias(dbITerm* iterm);

  dbNet* getNet(uint wireId, uint shapeId);
  uint selectNet();
  bool isSignalNet(dbNet* net);

  bool getWildCardName(const char* name, char* outName);

  void getNetBbox(dbNet* net, Rect& maxRect);
  uint getNetConnectivity(dbNet* net,
                          bool contextFlag,
                          uint clipMargin,
                          bool ignoreLayerFlags,
                          bool ignoreTypeFlags,
                          bool ignoreBB);
  uint getNetWires(dbNet* net,
                   bool contextFlag,
                   uint clipMargin,
                   bool ignoreZuiFlags,
                   bool ignoreBB);

  uint getNetFromDb(dbNet* net, bool ignoreZuiFlags, bool ignoreBB);
  uint addNetShapes(dbNet* net, bool wireVia, uint menuId, uint subMenuId);

  uint addWireCoords(dbShape& s,
                     uint menuId,
                     uint subMenuId,
                     uint netId,
                     uint shapeId);
  uint addViaCoords(dbShape& viaShape,
                    uint menuId,
                    uint subMenuId,
                    uint netId,
                    uint shapeId);

  uint addInstBoxes(dbNet* net, bool ignoreFlags);
  uint addInstBox(dbInst* inst);
  dbNet* getNetAndShape(dbShape& s, uint* shapeId, uint* level);
  void selectInst();
  void selectIterm2Net(uint itermId);
  uint selectIterm();

  uint addInstBoxes(dbInst* inst,
                    bool instBoxes,
                    bool termShapes,
                    bool instObs,
                    bool vias);
  void addInstConnList(dbInst* inst, bool ignoreFlags);
  uint getConnectivityWires(dbInst* inst, bool ignoreZuiFlags);
  void selectBterm2Net(uint btermId);
  uint selectBterm();

  uint addSBox(uint menuId, uint subMenuId, bool wireVia, uint wireId);
  void addNetSBoxes(dbNet* net, uint wtype, bool skipVias);

  uint getFirstShape(dbITerm* iterm,
                     bool viaFlag,
                     int& x1,
                     int& y1,
                     int& x2,
                     int& y2);
  uint getFirstInstObsShape(dbInst* inst,
                            bool viaFlag,
                            int& x1,
                            int& y1,
                            int& x2,
                            int& y2);
  uint getInstBoxes(int x1,
                    int y1,
                    int x2,
                    int y2,
                    std::vector<dbInst*>& result);  // 8/25/05

  dbRSeg* getRSeg(dbNet* net, uint shapeId);

  uint getWiresClipped(dbNet* targetNet, uint halo, bool ignoreFlag);

  uint getPowerWires(int x1,
                     int y1,
                     int x2,
                     int y2,
                     int layer,
                     dbNet* targetNet,
                     std::vector<dbBox*>& viaTable);

  uint getQuadCnt(int modSize, int xy1, int xy2);
  uint makeQuadTable(uint rowCnt, uint colCnt, uint metH, uint metV);
  uint assignTracks(uint metH, uint metV);

  //	uint getShapeCoords(dbShape *s, Ath__searchBox *bb, uint id1, uint id2,
  // uint level); 	dbNet* getShapeCoords(bool wireVia, Ath__searchBox *bb,
  // uint wireId, uint shapeId);

  uint createTblocks();
  //	dbNet* getNetAndCoords(Ath__searchBox *bb, bool skipVia, bool signal,
  // uint wid);
  uint getNetMaxBox(dbNet* net, Ath__searchBox* maxBB);
  void getWires(int x1, int y1, int x2, int y2);

  void addNetSBoxesOnSearch(dbNet* net, bool skipVias);
  void addNetShapesOnSearch(dbNet* net, bool skipVias);
  void makeGrouteSearchDb();

  uint getViaLevel(dbShape* s);
  uint getViaLevel(dbSBox* s);

  uint getNetSBoxes(dbNet* net, bool skipVias);

  void getNetSBoxesFromSearch(dbNet* net,
                              Ath__array1D<uint>* idTable,
                              Ath__searchBox* maxBox);
  void getNetShapesFromSearch(dbNet* net,
                              Ath__array1D<uint>* idTable,
                              Ath__searchBox* maxBox);

  uint addBlockagesOnSearch(dbBlock* block);
  uint getBlockObs(bool ignoreFlag);

  bool setInterface2SdbAtDB();  // ISDB
  bool connect2BlockSdb();      // ISDB

  uint makeTrackSearchDb();
  uint getTracks(bool ignoreLayers);
  uint addTracks(dbTrackGrid* g, uint dir, uint level, int lo[2], int hi[2]);

  int getTrackXY(int boxXY, int org, int count, int step);
  uint getTracks(dbTrackGrid* grid,
                 int* bb_ll,
                 int* bb_ur,
                 uint level,
                 uint dir);
  uint getTracks(dbTrackGrid* g,
                 int org,
                 int count,
                 int step,
                 int* bb_ll,
                 int* bb_ur,
                 uint level,
                 uint dir);

  // 2/10/2011
  uint getPowerWiresAndVias(int x1,
                            int y1,
                            int x2,
                            int y2,
                            int layer,
                            dbNet* targetNet,
                            bool power_wires,
                            std::vector<dbBox*>& viaTable);
  uint getPowerWireVias(ZPtr<ISdb> sdb,
                        dbNet* targetNet,
                        bool vias,
                        std::vector<dbBox*>& viaTable);
};

}  // namespace odb
