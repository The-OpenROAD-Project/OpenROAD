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

#include <list>
#include <vector>

#include "dbCore.h"
#include "dbHashTable.h"
#include "dbIntHashTable.h"
#include "dbPagedVector.h"
#include "dbVector.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/odb.h"

namespace odb {

template <class T>
class dbTable;
template <class T>
class dbArrayTable;
class _dbProperty;
class dbPropertyItr;
class _dbNameCache;
class _dbChip;
class _dbBox;
class _dbBTerm;
class _dbITerm;
class _dbNet;
class _dbInst;
class _dbInstHdr;
class _dbWire;
class _dbVia;
class _dbGCellGrid;
class _dbTrackGrid;
class _dbBlockage;
class _dbObstruction;
class _dbWire;
class _dbSWire;
class _dbSBox;
class _dbCapNode;  // DKF
class _dbRSeg;
class _dbCCSeg;
class _dbDatabase;
class _dbRow;
class _dbFill;
class _dbRegion;
class _dbHier;
class _dbBPin;
class _dbTech;
class _dbTechLayer;
class _dbTechLayerRule;
class _dbTechNonDefaultRule;
class _dbModule;
class _dbPowerDomain;
class _dbLogicPort;
class _dbPowerSwitch;
class _dbIsolation;
class _dbLevelShifter;
class _dbModInst;
class _dbModITerm;
class _dbModBTerm;
class _dbModNet;
class _dbBusPort;
class _dbGroup;
class _dbAccessPoint;
class _dbGlobalConnect;
class _dbGuide;
class _dbNetTrack;
class dbJournal;

class dbNetBTermItr;
class dbBPinItr;
class dbNetITermItr;
class dbInstITermItr;
class dbRegionInstItr;
class dbModuleInstItr;
class dbModuleModInstItr;
class dbModuleModBTermItr;
class dbModuleModInstModITermItr;
class dbModuleModNetItr;
class dbModuleModNetModBTermItr;
class dbModuleModNetModITermItr;
class dbModuleModNetITermItr;
class dbModuleModNetBTermItr;
class dbRegionGroupItr;
class dbGlobalConnect;
class dbGroupItr;
class dbGroupInstItr;
class dbGroupModInstItr;
class dbGroupPowerNetItr;
class dbGroupGroundNetItr;
class dbSWireItr;
class dbNameServer;
class dbBoxItr;
class dbSBoxItr;
class dbCapNodeItr;  // DKF
class dbRSegItr;     // DKF
class dbCCSegItr;
class dbExtControl;
class dbIStream;
class dbOStream;
class dbDiff;
class dbBlockSearch;
class dbBlockCallBackObj;
class dbGuideItr;
class dbNetTrackItr;
class _dbDft;

struct _dbBlockFlags
{
  uint _valid_bbox : 1;
  uint _spare_bits : 31;
};

class _dbBlock : public _dbObject
{
 public:
  enum Field  // dbJournal field names
  {
    CORNERCOUNT,
    WRITEDB
  };

  // PERSISTANT-MEMBERS
  _dbBlockFlags _flags;
  int _def_units;
  int _dbu_per_micron;  // cached value from dbTech
  char _hier_delimeter;
  char _left_bus_delimeter;
  char _right_bus_delimeter;
  unsigned char _num_ext_corners;
  uint _corners_per_block;
  char* _corner_name_list;
  char* _name;
  Rect _die_area;
  std::vector<Rect> _blocked_regions_for_pins;
  dbId<_dbTech> _tech;
  dbId<_dbChip> _chip;
  dbId<_dbBox> _bbox;
  dbId<_dbBlock> _parent;
  dbId<_dbBlock> _next_block;
  dbId<_dbGCellGrid> _gcell_grid;
  dbId<_dbBlock> _parent_block;  // Up hierarchy: TWG
  dbId<_dbInst> _parent_inst;    // Up hierarchy: TWG
  dbId<_dbModule> _top_module;
  dbHashTable<_dbNet> _net_hash;
  dbHashTable<_dbInst> _inst_hash;
  dbHashTable<_dbModule> _module_hash;
  dbHashTable<_dbModInst> _modinst_hash;
  dbHashTable<_dbPowerDomain> _powerdomain_hash;
  dbHashTable<_dbLogicPort> _logicport_hash;
  dbHashTable<_dbPowerSwitch> _powerswitch_hash;
  dbHashTable<_dbIsolation> _isolation_hash;
  dbHashTable<_dbModBTerm> _modbterm_hash;
  dbHashTable<_dbModITerm> _moditerm_hash;
  dbHashTable<_dbModNet> _modnet_hash;
  dbHashTable<_dbBusPort> _busport_hash;

  dbHashTable<_dbLevelShifter> _levelshifter_hash;
  dbHashTable<_dbGroup> _group_hash;
  dbIntHashTable<_dbInstHdr> _inst_hdr_hash;
  dbHashTable<_dbBTerm> _bterm_hash;
  uint _maxCapNodeId;
  uint _maxRSegId;
  uint _maxCCSegId;
  dbVector<dbId<_dbBlock>> _children;
  dbVector<dbId<_dbTechLayer>> _component_mask_shift;
  uint _currentCcAdjOrder;
  dbId<_dbDft> _dft;
  int _min_routing_layer;
  int _max_routing_layer;
  int _min_layer_for_clock;
  int _max_layer_for_clock;

  // NON-PERSISTANT-STREAMED-MEMBERS
  dbTable<_dbBTerm>* _bterm_tbl;
  dbTable<_dbITerm>* _iterm_tbl;
  dbTable<_dbNet>* _net_tbl;
  dbTable<_dbInstHdr>* _inst_hdr_tbl;
  dbTable<_dbInst>* _inst_tbl;
  dbTable<_dbBox>* _box_tbl;
  dbTable<_dbVia>* _via_tbl;
  dbTable<_dbGCellGrid>* _gcell_grid_tbl;
  dbTable<_dbTrackGrid>* _track_grid_tbl;
  dbTable<_dbObstruction>* _obstruction_tbl;
  dbTable<_dbBlockage>* _blockage_tbl;
  dbTable<_dbWire>* _wire_tbl;
  dbTable<_dbSWire>* _swire_tbl;
  dbTable<_dbSBox>* _sbox_tbl;
  dbTable<_dbRow>* _row_tbl;
  dbTable<_dbFill>* _fill_tbl;
  dbTable<_dbRegion>* _region_tbl;
  dbTable<_dbHier>* _hier_tbl;
  dbTable<_dbBPin>* _bpin_tbl;
  dbTable<_dbTechNonDefaultRule>* _non_default_rule_tbl;
  dbTable<_dbTechLayerRule>* _layer_rule_tbl;
  dbTable<_dbProperty>* _prop_tbl;
  dbTable<_dbModule>* _module_tbl;
  dbTable<_dbPowerDomain>* _powerdomain_tbl;
  dbTable<_dbLogicPort>* _logicport_tbl;
  dbTable<_dbPowerSwitch>* _powerswitch_tbl;
  dbTable<_dbIsolation>* _isolation_tbl;
  dbTable<_dbLevelShifter>* _levelshifter_tbl;
  dbTable<_dbModInst>* _modinst_tbl;
  dbTable<_dbGroup>* _group_tbl;
  dbTable<_dbAccessPoint>* ap_tbl_;
  dbTable<_dbGlobalConnect>* global_connect_tbl_;
  dbTable<_dbGuide>* _guide_tbl;
  dbTable<_dbNetTrack>* _net_tracks_tbl;
  _dbNameCache* _name_cache;
  dbTable<_dbDft>* _dft_tbl;

  dbPagedVector<float, 4096, 12>* _r_val_tbl;
  dbPagedVector<float, 4096, 12>* _c_val_tbl;
  dbPagedVector<float, 4096, 12>* _cc_val_tbl;

  dbTable<_dbModBTerm>* _modbterm_tbl;
  dbTable<_dbModITerm>* _moditerm_tbl;
  dbTable<_dbModNet>* _modnet_tbl;
  dbTable<_dbBusPort>* _busport_tbl;

  dbTable<_dbCapNode>* _cap_node_tbl;
  dbTable<_dbRSeg>* _r_seg_tbl;
  dbTable<_dbCCSeg>* _cc_seg_tbl;
  dbExtControl* _extControl;

  // NON-PERSISTANT-NON-STREAMED-MEMBERS
  dbNetBTermItr* _net_bterm_itr;
  dbNetITermItr* _net_iterm_itr;
  dbInstITermItr* _inst_iterm_itr;
  dbBoxItr* _box_itr;
  dbSWireItr* _swire_itr;
  dbSBoxItr* _sbox_itr;
  dbCapNodeItr* _cap_node_itr;
  dbRSegItr* _r_seg_itr;
  dbCCSegItr* _cc_seg_itr;
  dbRegionInstItr* _region_inst_itr;
  dbModuleInstItr* _module_inst_itr;
  dbModuleModInstItr* _module_modinst_itr;
  dbModuleModBTermItr* _module_modbterm_itr;
  dbModuleModInstModITermItr* _module_modinstmoditerm_itr;

  dbModuleModNetItr* _module_modnet_itr;
  dbModuleModNetModITermItr* _module_modnet_moditerm_itr;
  dbModuleModNetModBTermItr* _module_modnet_modbterm_itr;
  dbModuleModNetITermItr* _module_modnet_iterm_itr;
  dbModuleModNetBTermItr* _module_modnet_bterm_itr;

  dbRegionGroupItr* _region_group_itr;
  dbGroupItr* _group_itr;
  dbGuideItr* _guide_itr;
  dbNetTrackItr* _net_track_itr;
  dbGroupInstItr* _group_inst_itr;
  dbGroupModInstItr* _group_modinst_itr;
  dbGroupPowerNetItr* _group_power_net_itr;
  dbGroupGroundNetItr* _group_ground_net_itr;
  dbBPinItr* _bpin_itr;
  dbPropertyItr* _prop_itr;
  dbBlockSearch* _searchDb;

  unsigned char _num_ext_dbs;

  std::list<dbBlockCallBackObj*> _callbacks;
  void* _extmi;

  dbJournal* _journal;
  dbJournal* _journal_pending;

  _dbBlock(_dbDatabase* db);
  _dbBlock(_dbDatabase* db, const _dbBlock& block);
  ~_dbBlock();
  void add_rect(const Rect& rect);
  void add_oct(const Oct& oct);
  void remove_rect(const Rect& rect);
  void invalidate_bbox() { _flags._valid_bbox = 0; }
  void initialize(_dbChip* chip,
                  _dbTech* tech,
                  _dbBlock* parent,
                  const char* name,
                  char delimeter);

  bool operator==(const _dbBlock& rhs) const;
  bool operator!=(const _dbBlock& rhs) const { return !operator==(rhs); }
  void differences(dbDiff& diff, const char* field, const _dbBlock& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  int globalConnect(const std::vector<dbGlobalConnect*>& connects);
  _dbTech* getTech();

  dbObjectTable* getObjectTable(dbObjectType type);
};

dbOStream& operator<<(dbOStream& stream, const _dbBlock& block);
dbIStream& operator>>(dbIStream& stream, _dbBlock& block);

}  // namespace odb
