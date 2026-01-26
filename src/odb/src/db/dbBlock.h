// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "dbCore.h"
#include "dbHashTable.h"
#include "dbIntHashTable.h"
#include "dbPagedVector.h"
#include "dbVector.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/isotropy.h"

namespace odb {

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
template <uint32_t page_size>
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
  uint32_t valid_bbox : 1;
  uint32_t spare_bits : 31;
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
    kCornerCount,
    kWriteDb
  };

  _dbBlock(_dbDatabase* db);
  ~_dbBlock();
  void add_rect(const Rect& rect);
  void add_oct(const Oct& oct);
  void remove_rect(const Rect& rect);
  void invalidate_bbox() { flags_.valid_bbox = 0; }
  void initialize(_dbChip* chip,
                  _dbBlock* parent,
                  const char* name,
                  char delimiter);

  bool operator==(const _dbBlock& rhs) const;
  bool operator!=(const _dbBlock& rhs) const { return !operator==(rhs); }

  int globalConnect(const std::vector<dbGlobalConnect*>& connects,
                    bool force,
                    bool verbose);
  _dbTech* getTech();

  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);
  void clearSystemBlockagesAndObstructions();
  void ensureConstraintRegion(const Direction2D& edge, int& begin, int& end);
  void ComputeBBox();
  std::string makeNewName(dbModInst* parent,
                          const char* base_name,
                          const dbNameUniquifyType& uniquify,
                          uint32_t& unique_index,
                          const std::function<bool(const char*)>& exists);

  // PERSISTANT-MEMBERS
  _dbBlockFlags flags_;
  int def_units_;
  int dbu_per_micron_;  // cached value from dbTech
  char hier_delimiter_;
  char left_bus_delimiter_;
  char right_bus_delimiter_;
  unsigned char num_ext_corners_;
  uint32_t corners_per_block_;
  char* corner_name_list_;
  char* name_;
  Polygon die_area_;
  Polygon core_area_;
  std::vector<Rect> blocked_regions_for_pins_;
  dbId<_dbChip> chip_;
  dbId<_dbBox> bbox_;
  dbId<_dbBlock> parent_;
  dbId<_dbBlock> next_block_;
  dbId<_dbGCellGrid> gcell_grid_;
  dbId<_dbBlock> parent_block_;  // Up hierarchy: TWG
  dbId<_dbInst> parent_inst_;    // Up hierarchy: TWG
  dbId<_dbModule> top_module_;
  dbHashTable<_dbNet> net_hash_;
  dbHashTable<_dbInst> inst_hash_;
  dbHashTable<_dbModule> module_hash_;
  dbHashTable<_dbModInst> modinst_hash_;
  dbHashTable<_dbPowerDomain> powerdomain_hash_;
  dbHashTable<_dbLogicPort> logicport_hash_;
  dbHashTable<_dbPowerSwitch> powerswitch_hash_;
  dbHashTable<_dbIsolation> isolation_hash_;

  dbHashTable<_dbLevelShifter> levelshifter_hash_;
  dbHashTable<_dbGroup> group_hash_;
  dbIntHashTable<_dbInstHdr> inst_hdr_hash_;
  dbHashTable<_dbBTerm> bterm_hash_;
  uint32_t max_cap_node_id_;
  uint32_t max_rseg_id_;
  uint32_t max_cc_seg_id_;
  dbVector<dbId<_dbBlock>> children_;
  dbVector<dbId<_dbTechLayer>> component_mask_shift_;
  uint32_t currentCcAdjOrder_;
  dbId<_dbDft> dft_;
  int min_routing_layer_;
  int max_routing_layer_;
  int min_layer_for_clock_;
  int max_layer_for_clock_;
  std::vector<_dbBTermGroup> bterm_groups_;
  _dbBTermTopLayerGrid bterm_top_layer_grid_;
  uint32_t unique_net_index_{1};  // unique index used to create a new net name
  uint32_t unique_inst_index_{
      1};  // unique index used to create a new inst name

  // NON-PERSISTANT-STREAMED-MEMBERS
  dbTable<_dbBTerm>* bterm_tbl_;
  dbTable<_dbITerm, 1024>* iterm_tbl_;
  dbTable<_dbNet>* net_tbl_;
  dbTable<_dbInstHdr>* inst_hdr_tbl_;
  dbTable<_dbInst>* inst_tbl_;
  dbTable<_dbScanInst>* scan_inst_tbl_;
  dbTable<_dbBox, 1024>* box_tbl_;
  dbTable<_dbVia, 1024>* via_tbl_;
  dbTable<_dbGCellGrid>* gcell_grid_tbl_;
  dbTable<_dbTrackGrid>* track_grid_tbl_;
  dbTable<_dbObstruction>* obstruction_tbl_;
  dbTable<_dbBlockage>* blockage_tbl_;
  dbTable<_dbWire>* wire_tbl_;
  dbTable<_dbSWire>* swire_tbl_;
  dbTable<_dbSBox>* sbox_tbl_;
  dbTable<_dbRow>* row_tbl_;
  dbTable<_dbFill>* fill_tbl_;
  dbTable<_dbRegion, 32>* region_tbl_;
  dbTable<_dbHier, 16>* hier_tbl_;
  dbTable<_dbBPin>* bpin_tbl_;
  dbTable<_dbTechNonDefaultRule, 16>* non_default_rule_tbl_;
  dbTable<_dbTechLayerRule, 16>* layer_rule_tbl_;
  dbTable<_dbProperty>* prop_tbl_;
  dbTable<_dbModule>* module_tbl_;
  dbTable<_dbPowerDomain>* powerdomain_tbl_;
  dbTable<_dbLogicPort>* logicport_tbl_;
  dbTable<_dbPowerSwitch>* powerswitch_tbl_;
  dbTable<_dbIsolation>* isolation_tbl_;
  dbTable<_dbLevelShifter>* levelshifter_tbl_;
  dbTable<_dbModInst>* modinst_tbl_;
  dbTable<_dbGroup>* group_tbl_;
  dbTable<_dbAccessPoint>* ap_tbl_;
  dbTable<_dbGlobalConnect>* global_connect_tbl_;
  dbTable<_dbGuide>* guide_tbl_;
  dbTable<_dbNetTrack>* net_tracks_tbl_;
  _dbNameCache* name_cache_;
  dbTable<_dbDft, 4096>* dft_tbl_;

  dbPagedVector<float, 4096, 12>* r_val_tbl_;
  dbPagedVector<float, 4096, 12>* c_val_tbl_;
  dbPagedVector<float, 4096, 12>* cc_val_tbl_;

  dbTable<_dbModBTerm>* modbterm_tbl_;
  dbTable<_dbModITerm>* moditerm_tbl_;
  dbTable<_dbModNet>* modnet_tbl_;
  dbTable<_dbBusPort>* busport_tbl_;

  dbTable<_dbCapNode, 4096>* cap_node_tbl_;
  dbTable<_dbRSeg, 4096>* r_seg_tbl_;
  dbTable<_dbCCSeg, 4096>* cc_seg_tbl_;
  dbExtControl* ext_control_;

  // NON-PERSISTANT-NON-STREAMED-MEMBERS
  dbNetBTermItr* net_bterm_itr_;
  dbNetITermItr* net_iterm_itr_;
  dbInstITermItr* inst_iterm_itr_;
  dbScanListScanInstItr* scan_list_scan_inst_itr_;
  dbBoxItr<1024>* box_itr_;
  dbSWireItr* swire_itr_;
  dbSBoxItr* sbox_itr_;
  dbCapNodeItr* cap_node_itr_;
  dbRSegItr* r_seg_itr_;
  dbCCSegItr* cc_seg_itr_;
  dbRegionInstItr* region_inst_itr_;
  dbModuleInstItr* module_inst_itr_;
  dbModuleModInstItr* module_modinst_itr_;
  dbModuleModBTermItr* module_modbterm_itr_;
  dbModuleModInstModITermItr* module_modinstmoditerm_itr_;

  dbModuleModNetItr* module_modnet_itr_;
  dbModuleModNetModITermItr* module_modnet_moditerm_itr_;
  dbModuleModNetModBTermItr* module_modnet_modbterm_itr_;
  dbModuleModNetITermItr* module_modnet_iterm_itr_;
  dbModuleModNetBTermItr* module_modnet_bterm_itr_;

  dbRegionGroupItr* region_group_itr_;
  dbGroupItr* group_itr_;
  dbGuideItr* guide_itr_;
  dbNetTrackItr* net_track_itr_;
  dbGroupInstItr* group_inst_itr_;
  dbGroupModInstItr* group_modinst_itr_;
  dbGroupPowerNetItr* group_power_net_itr_;
  dbGroupGroundNetItr* group_ground_net_itr_;
  dbBPinItr* bpin_itr_;
  dbPropertyItr* prop_itr_;
  dbBlockSearch* search_db_;

  std::unordered_map<std::string, int> module_name_id_map_;
  std::unordered_map<std::string, int> inst_name_id_map_;
  std::unordered_map<dbId<_dbInst>, dbId<_dbScanInst>> inst_scan_inst_map_;

  unsigned char num_ext_dbs_;

  std::list<dbBlockCallBackObj*> callbacks_;
  void* extmi_;

  dbJournal* journal_;
  std::stack<dbJournal*> journal_stack_;
};

dbOStream& operator<<(dbOStream& stream, const _dbBlock& block);
dbIStream& operator>>(dbIStream& stream, _dbBlock& block);

dbOStream& operator<<(dbOStream& stream, const _dbBTermGroup& obj);
dbIStream& operator>>(dbIStream& stream, _dbBTermGroup& obj);

dbOStream& operator<<(dbOStream& stream, const _dbBTermTopLayerGrid& obj);
dbIStream& operator>>(dbIStream& stream, _dbBTermTopLayerGrid& obj);

}  // namespace odb
