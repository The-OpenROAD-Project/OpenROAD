// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <list>
#include <map>
#include <string>
#include <vector>

#include "dbCore.h"
#include "dbHashTable.h"
#include "dbIntHashTable.h"
#include "dbPagedVector.h"
#include "dbVector.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/odb.h"

namespace odb {

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
class _dbScanInst;
class dbScanListScanInstItr;
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
class _dbMarkerCategory;
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
template <uint page_size>
class dbBoxItr;
class dbSBoxItr;
class dbCapNodeItr;
class dbRSegItr;
class dbCCSegItr;
class dbExtControl;
class dbIStream;
class dbOStream;
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

struct _dbBTermGroup
{
  std::vector<dbId<_dbBTerm>> bterms;
  bool order = false;
};

struct _dbBTermTopLayerGrid
{
  dbId<_dbTechLayer> layer;
  int x_step = 0;
  int y_step = 0;
  Polygon region;
  int pin_width = 0;
  int pin_height = 0;
  int keepout = 0;
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
  char _hier_delimiter;
  char _left_bus_delimiter;
  char _right_bus_delimiter;
  unsigned char _num_ext_corners;
  uint _corners_per_block;
  char* _corner_name_list;
  char* _name;
  Polygon _die_area;
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
  dbHashTable<_dbMarkerCategory> _marker_category_hash;

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
  std::vector<_dbBTermGroup> _bterm_groups;
  _dbBTermTopLayerGrid _bterm_top_layer_grid;

  // NON-PERSISTANT-STREAMED-MEMBERS
  dbTable<_dbBTerm>* _bterm_tbl;
  dbTable<_dbITerm, 1024>* _iterm_tbl;
  dbTable<_dbNet>* _net_tbl;
  dbTable<_dbInstHdr>* _inst_hdr_tbl;
  dbTable<_dbInst>* _inst_tbl;
  dbTable<_dbScanInst>* _scan_inst_tbl;
  dbTable<_dbBox, 1024>* _box_tbl;
  dbTable<_dbVia, 1024>* _via_tbl;
  dbTable<_dbGCellGrid>* _gcell_grid_tbl;
  dbTable<_dbTrackGrid>* _track_grid_tbl;
  dbTable<_dbObstruction>* _obstruction_tbl;
  dbTable<_dbBlockage>* _blockage_tbl;
  dbTable<_dbWire>* _wire_tbl;
  dbTable<_dbSWire>* _swire_tbl;
  dbTable<_dbSBox>* _sbox_tbl;
  dbTable<_dbRow>* _row_tbl;
  dbTable<_dbFill>* _fill_tbl;
  dbTable<_dbRegion, 32>* _region_tbl;
  dbTable<_dbHier, 16>* _hier_tbl;
  dbTable<_dbBPin>* _bpin_tbl;
  dbTable<_dbTechNonDefaultRule, 16>* _non_default_rule_tbl;
  dbTable<_dbTechLayerRule, 16>* _layer_rule_tbl;
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
  dbTable<_dbDft, 4096>* _dft_tbl;
  dbTable<_dbMarkerCategory>* _marker_categories_tbl;

  dbPagedVector<float, 4096, 12>* _r_val_tbl;
  dbPagedVector<float, 4096, 12>* _c_val_tbl;
  dbPagedVector<float, 4096, 12>* _cc_val_tbl;

  dbTable<_dbModBTerm>* _modbterm_tbl;
  dbTable<_dbModITerm>* _moditerm_tbl;
  dbTable<_dbModNet>* _modnet_tbl;
  dbTable<_dbBusPort>* _busport_tbl;

  dbTable<_dbCapNode, 4096>* _cap_node_tbl;
  dbTable<_dbRSeg, 4096>* _r_seg_tbl;
  dbTable<_dbCCSeg, 4096>* _cc_seg_tbl;
  dbExtControl* _extControl;

  // NON-PERSISTANT-NON-STREAMED-MEMBERS
  dbNetBTermItr* _net_bterm_itr;
  dbNetITermItr* _net_iterm_itr;
  dbInstITermItr* _inst_iterm_itr;
  dbScanListScanInstItr* _scan_list_scan_inst_itr;
  dbBoxItr<1024>* _box_itr;
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

  std::unordered_map<std::string, int> _module_name_id_map;
  std::unordered_map<std::string, int> _inst_name_id_map;
  std::unordered_map<dbId<_dbInst>, dbId<_dbScanInst>> _inst_scan_inst_map;

  unsigned char _num_ext_dbs;

  std::list<dbBlockCallBackObj*> _callbacks;
  void* _extmi;

  dbJournal* _journal;
  dbJournal* _journal_pending;

  _dbBlock(_dbDatabase* db);
  ~_dbBlock();
  void add_rect(const Rect& rect);
  void add_oct(const Oct& oct);
  void remove_rect(const Rect& rect);
  void invalidate_bbox() { _flags._valid_bbox = 0; }
  void initialize(_dbChip* chip,
                  _dbTech* tech,
                  _dbBlock* parent,
                  const char* name,
                  char delimiter);

  bool operator==(const _dbBlock& rhs) const;
  bool operator!=(const _dbBlock& rhs) const { return !operator==(rhs); }

  int globalConnect(const std::vector<dbGlobalConnect*>& connects);
  _dbTech* getTech();

  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);
  void clearSystemBlockagesAndObstructions();
  void ensureConstraintRegion(const Direction2D& edge, int& begin, int& end);
};

dbOStream& operator<<(dbOStream& stream, const _dbBlock& block);
dbIStream& operator>>(dbIStream& stream, _dbBlock& block);

dbOStream& operator<<(dbOStream& stream, const _dbBTermGroup& obj);
dbIStream& operator>>(dbIStream& stream, _dbBTermGroup& obj);

dbOStream& operator<<(dbOStream& stream, const _dbBTermTopLayerGrid& obj);
dbIStream& operator>>(dbIStream& stream, _dbBTermTopLayerGrid& obj);

}  // namespace odb
