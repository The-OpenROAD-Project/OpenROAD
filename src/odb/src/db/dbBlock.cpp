// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBlock.h"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <ios>
#include <iterator>
#include <list>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dbAccessPoint.h"
#include "dbBPin.h"
#include "dbBPinItr.h"
#include "dbBTerm.h"
#include "dbBTermItr.h"
#include "dbBlockItr.h"
#include "dbBlockage.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbBusPort.h"
#include "dbCCSeg.h"
#include "dbCCSegItr.h"
#include "dbCapNode.h"
#include "dbCapNodeItr.h"
#include "dbChip.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbDft.h"
#include "dbFill.h"
#include "dbGCellGrid.h"
#include "dbGlobalConnect.h"
#include "dbGroup.h"
#include "dbGroupGroundNetItr.h"
#include "dbGroupInstItr.h"
#include "dbGroupItr.h"
#include "dbGroupModInstItr.h"
#include "dbGroupPowerNetItr.h"
#include "dbGuide.h"
#include "dbGuideItr.h"
#include "dbHashTable.h"
#include "dbHashTable.hpp"
#include "dbHier.h"
#include "dbITerm.h"
#include "dbITermItr.h"
#include "dbInst.h"
#include "dbInstHdr.h"
#include "dbIntHashTable.h"
#include "dbIntHashTable.hpp"
#include "dbIsolation.h"
#include "dbJournal.h"
#include "dbLevelShifter.h"
#include "dbLogicPort.h"
#include "dbMarkerCategory.h"
#include "dbModBTerm.h"
#include "dbModITerm.h"
#include "dbModInst.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbModuleInstItr.h"
#include "dbModuleModBTermItr.h"
#include "dbModuleModInstItr.h"
#include "dbModuleModInstModITermItr.h"
#include "dbModuleModNetBTermItr.h"
#include "dbModuleModNetITermItr.h"
#include "dbModuleModNetItr.h"
#include "dbModuleModNetModBTermItr.h"
#include "dbModuleModNetModITermItr.h"
#include "dbNameCache.h"
#include "dbNet.h"
#include "dbNetTrack.h"
#include "dbNetTrackItr.h"
#include "dbObstruction.h"
#include "dbPagedVector.h"
#include "dbPowerDomain.h"
#include "dbPowerSwitch.h"
#include "dbProperty.h"
#include "dbPropertyItr.h"
#include "dbRSeg.h"
#include "dbRSegItr.h"
#include "dbRegion.h"
#include "dbRegionGroupItr.h"
#include "dbRegionInstItr.h"
#include "dbRow.h"
#include "dbSBox.h"
#include "dbSBoxItr.h"
#include "dbSWire.h"
#include "dbSWireItr.h"
#include "dbScanInst.h"
#include "dbScanListScanInstItr.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechLayerRule.h"
#include "dbTechNonDefaultRule.h"
#include "dbTrackGrid.h"
#include "dbVia.h"
#include "dbWire.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbExtControl.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbStream.h"
#include "odb/dbTypes.h"
#include "odb/defout.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"
#include "odb/isotropy.h"
#include "odb/lefout.h"
#include "odb/poly_decomp.h"
#include "utl/Logger.h"

namespace odb {

struct OldTransform
{
  int orient;
  int originX;
  int originY;
  int sizeX;
  int sizeY;
};

static void unlink_child_from_parent(_dbBlock* child, _dbBlock* parent);

// TODO: Bounding box updates...
template class dbTable<_dbBlock>;

template class dbHashTable<_dbNet>;
template class dbHashTable<_dbInst>;
template class dbIntHashTable<_dbInstHdr>;
template class dbHashTable<_dbBTerm>;
template class dbHashTable<_dbMarkerCategory>;

_dbBlock::_dbBlock(_dbDatabase* db)
{
  flags_.valid_bbox = 0;
  flags_.spare_bits = 0;
  def_units_ = 100;
  dbu_per_micron_ = 1000;
  hier_delimiter_ = '/';
  left_bus_delimiter_ = 0;
  right_bus_delimiter_ = 0;
  num_ext_corners_ = 0;
  corners_per_block_ = 0;
  corner_name_list_ = nullptr;
  name_ = nullptr;
  die_area_ = Rect(0, 0, 0, 0);
  core_area_ = Rect(0, 0, 0, 0);
  max_cap_node_id_ = 0;
  max_rseg_id_ = 0;
  max_cc_seg_id_ = 0;
  min_routing_layer_ = 2;
  max_routing_layer_ = -1;
  min_layer_for_clock_ = -1;
  max_layer_for_clock_ = -2;

  currentCcAdjOrder_ = 0;
  bterm_tbl_ = new dbTable<_dbBTerm>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbBTermObj);

  iterm_tbl_ = new dbTable<_dbITerm, 1024>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbITermObj);

  net_tbl_ = new dbTable<_dbNet>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbNetObj);

  inst_hdr_tbl_ = new dbTable<_dbInstHdr>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbInstHdrObj);

  inst_tbl_ = new dbTable<_dbInst>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbInstObj);

  scan_inst_tbl_ = new dbTable<_dbScanInst>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbScanInstObj);

  module_tbl_ = new dbTable<_dbModule>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbModuleObj);

  modinst_tbl_ = new dbTable<_dbModInst>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbModInstObj);

  modbterm_tbl_ = new dbTable<_dbModBTerm>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbModBTermObj);

  moditerm_tbl_ = new dbTable<_dbModITerm>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbModITermObj);

  modnet_tbl_ = new dbTable<_dbModNet>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbModNetObj);

  busport_tbl_ = new dbTable<_dbBusPort>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbBusPortObj);

  powerdomain_tbl_ = new dbTable<_dbPowerDomain>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbPowerDomainObj);

  logicport_tbl_ = new dbTable<_dbLogicPort>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbLogicPortObj);

  powerswitch_tbl_ = new dbTable<_dbPowerSwitch>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbPowerSwitchObj);

  isolation_tbl_ = new dbTable<_dbIsolation>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbIsolationObj);

  levelshifter_tbl_ = new dbTable<_dbLevelShifter>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbLevelShifterObj);

  group_tbl_ = new dbTable<_dbGroup>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbGroupObj);

  ap_tbl_ = new dbTable<_dbAccessPoint>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbAccessPointObj);

  global_connect_tbl_ = new dbTable<_dbGlobalConnect>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbGlobalConnectObj);

  guide_tbl_ = new dbTable<_dbGuide>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbGuideObj);

  net_tracks_tbl_ = new dbTable<_dbNetTrack>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbNetTrackObj);

  box_tbl_ = new dbTable<_dbBox, 1024>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbBoxObj);

  via_tbl_ = new dbTable<_dbVia, 1024>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbViaObj);

  gcell_grid_tbl_ = new dbTable<_dbGCellGrid>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbGCellGridObj);

  track_grid_tbl_ = new dbTable<_dbTrackGrid>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbTrackGridObj);

  obstruction_tbl_ = new dbTable<_dbObstruction>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbObstructionObj);

  blockage_tbl_ = new dbTable<_dbBlockage>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbBlockageObj);

  wire_tbl_ = new dbTable<_dbWire>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbWireObj);

  swire_tbl_ = new dbTable<_dbSWire>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbSWireObj);

  sbox_tbl_ = new dbTable<_dbSBox>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbSBoxObj);

  row_tbl_ = new dbTable<_dbRow>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbRowObj);

  fill_tbl_ = new dbTable<_dbFill>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbFillObj);

  region_tbl_ = new dbTable<_dbRegion, 32>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbRegionObj);

  hier_tbl_ = new dbTable<_dbHier, 16>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbHierObj);

  bpin_tbl_ = new dbTable<_dbBPin>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbBPinObj);

  non_default_rule_tbl_ = new dbTable<_dbTechNonDefaultRule, 16>(
      db,
      this,
      (GetObjTbl_t) &_dbBlock::getObjectTable,
      dbTechNonDefaultRuleObj);

  layer_rule_tbl_ = new dbTable<_dbTechLayerRule, 16>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbTechLayerRuleObj);

  prop_tbl_ = new dbTable<_dbProperty>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbPropertyObj);

  name_cache_
      = new _dbNameCache(db, this, (GetObjTbl_t) &_dbBlock::getObjectTable);

  r_val_tbl_ = new dbPagedVector<float, 4096, 12>();
  r_val_tbl_->push_back(0.0);

  c_val_tbl_ = new dbPagedVector<float, 4096, 12>();
  c_val_tbl_->push_back(0.0);

  cc_val_tbl_ = new dbPagedVector<float, 4096, 12>();
  cc_val_tbl_->push_back(0.0);

  cap_node_tbl_ = new dbTable<_dbCapNode, 4096>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbCapNodeObj);

  // We need to allocate the first cap-node (id == 1) to resolve a problem with
  // the extraction code (Hopefully this is temporary)
  cap_node_tbl_->create();

  r_seg_tbl_ = new dbTable<_dbRSeg, 4096>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbRSegObj);

  cc_seg_tbl_ = new dbTable<_dbCCSeg, 4096>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbCCSegObj);

  ext_control_ = new dbExtControl();

  dft_tbl_ = new dbTable<_dbDft, 4096>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbDftObj);
  _dbDft* dft_ptr = dft_tbl_->create();
  dft_ptr->initialize();
  dft_ = dft_ptr->getId();

  net_hash_.setTable(net_tbl_);
  inst_hash_.setTable(inst_tbl_);
  module_hash_.setTable(module_tbl_);
  modinst_hash_.setTable(modinst_tbl_);
  powerdomain_hash_.setTable(powerdomain_tbl_);
  logicport_hash_.setTable(logicport_tbl_);
  powerswitch_hash_.setTable(powerswitch_tbl_);
  isolation_hash_.setTable(isolation_tbl_);
  levelshifter_hash_.setTable(levelshifter_tbl_);
  group_hash_.setTable(group_tbl_);
  inst_hdr_hash_.setTable(inst_hdr_tbl_);
  bterm_hash_.setTable(bterm_tbl_);

  net_bterm_itr_ = new dbNetBTermItr(bterm_tbl_);

  net_iterm_itr_ = new dbNetITermItr(iterm_tbl_);

  inst_iterm_itr_ = new dbInstITermItr(iterm_tbl_);

  scan_list_scan_inst_itr_ = new dbScanListScanInstItr(scan_inst_tbl_);

  box_itr_ = new dbBoxItr<1024>(box_tbl_, nullptr, false);

  swire_itr_ = new dbSWireItr(swire_tbl_);

  sbox_itr_ = new dbSBoxItr(sbox_tbl_);

  cap_node_itr_ = new dbCapNodeItr(cap_node_tbl_);

  r_seg_itr_ = new dbRSegItr(r_seg_tbl_);

  cc_seg_itr_ = new dbCCSegItr(cc_seg_tbl_);

  region_inst_itr_ = new dbRegionInstItr(inst_tbl_);

  module_inst_itr_ = new dbModuleInstItr(inst_tbl_);

  module_modinst_itr_ = new dbModuleModInstItr(modinst_tbl_);

  module_modinstmoditerm_itr_ = new dbModuleModInstModITermItr(moditerm_tbl_);

  module_modbterm_itr_ = new dbModuleModBTermItr(modbterm_tbl_);

  module_modnet_itr_ = new dbModuleModNetItr(modnet_tbl_);

  module_modnet_modbterm_itr_ = new dbModuleModNetModBTermItr(modbterm_tbl_);
  module_modnet_moditerm_itr_ = new dbModuleModNetModITermItr(moditerm_tbl_);
  module_modnet_iterm_itr_ = new dbModuleModNetITermItr(iterm_tbl_);
  module_modnet_bterm_itr_ = new dbModuleModNetBTermItr(bterm_tbl_);

  region_group_itr_ = new dbRegionGroupItr(group_tbl_);

  group_itr_ = new dbGroupItr(group_tbl_);

  guide_itr_ = new dbGuideItr(guide_tbl_);

  net_track_itr_ = new dbNetTrackItr(net_tracks_tbl_);

  group_inst_itr_ = new dbGroupInstItr(inst_tbl_);

  group_modinst_itr_ = new dbGroupModInstItr(modinst_tbl_);

  group_power_net_itr_ = new dbGroupPowerNetItr(net_tbl_);

  group_ground_net_itr_ = new dbGroupGroundNetItr(net_tbl_);

  bpin_itr_ = new dbBPinItr(bpin_tbl_);

  prop_itr_ = new dbPropertyItr(prop_tbl_);

  num_ext_dbs_ = 1;
  search_db_ = nullptr;
  extmi_ = nullptr;
  journal_ = nullptr;
}

_dbBlock::~_dbBlock()
{
  if (name_) {
    free((void*) name_);
  }
  free(corner_name_list_);

  delete bterm_tbl_;
  delete iterm_tbl_;
  delete net_tbl_;
  delete inst_hdr_tbl_;
  delete inst_tbl_;
  delete scan_inst_tbl_;
  delete module_tbl_;
  delete modinst_tbl_;
  delete modbterm_tbl_;
  delete moditerm_tbl_;
  delete modnet_tbl_;
  delete busport_tbl_;
  delete powerdomain_tbl_;
  delete logicport_tbl_;
  delete powerswitch_tbl_;
  delete isolation_tbl_;
  delete levelshifter_tbl_;
  delete group_tbl_;
  delete ap_tbl_;
  delete global_connect_tbl_;
  delete guide_tbl_;
  delete net_tracks_tbl_;
  delete box_tbl_;
  delete via_tbl_;
  delete gcell_grid_tbl_;
  delete track_grid_tbl_;
  delete obstruction_tbl_;
  delete blockage_tbl_;
  delete wire_tbl_;
  delete swire_tbl_;
  delete sbox_tbl_;
  delete row_tbl_;
  delete fill_tbl_;
  delete region_tbl_;
  delete hier_tbl_;
  delete bpin_tbl_;
  delete non_default_rule_tbl_;
  delete layer_rule_tbl_;
  delete prop_tbl_;
  delete name_cache_;
  delete r_val_tbl_;
  delete c_val_tbl_;
  delete cc_val_tbl_;
  delete cap_node_tbl_;
  delete r_seg_tbl_;
  delete cc_seg_tbl_;
  delete ext_control_;
  delete net_bterm_itr_;
  delete net_iterm_itr_;
  delete inst_iterm_itr_;
  delete scan_list_scan_inst_itr_;
  delete box_itr_;
  delete swire_itr_;
  delete sbox_itr_;
  delete cap_node_itr_;
  delete r_seg_itr_;
  delete cc_seg_itr_;
  delete region_inst_itr_;
  delete module_inst_itr_;
  delete module_modinst_itr_;
  delete module_modinstmoditerm_itr_;
  delete module_modbterm_itr_;
  delete module_modnet_itr_;
  delete module_modnet_modbterm_itr_;
  delete module_modnet_moditerm_itr_;
  delete module_modnet_iterm_itr_;
  delete module_modnet_bterm_itr_;
  delete region_group_itr_;
  delete group_itr_;
  delete guide_itr_;
  delete net_track_itr_;
  delete group_inst_itr_;
  delete group_modinst_itr_;
  delete group_power_net_itr_;
  delete group_ground_net_itr_;
  delete bpin_itr_;
  delete prop_itr_;
  delete dft_tbl_;

  while (!callbacks_.empty()) {
    auto _cbitr = callbacks_.begin();
    (*_cbitr)->removeOwner();
  }
  delete journal_;
}

void dbBlock::clear()
{
  _dbBlock* block = (_dbBlock*) this;
  _dbDatabase* db = block->getDatabase();
  _dbBlock* parent = (_dbBlock*) getParent();
  _dbChip* chip = (_dbChip*) getChip();

  // save a copy of the name
  char* name = safe_strdup(block->name_);

  // save a copy of the delimiter
  char delimiter = block->hier_delimiter_;

  std::list<dbBlockCallBackObj*> callbacks;

  // save callbacks
  callbacks.swap(block->callbacks_);

  // unlink the child from the parent
  if (parent) {
    unlink_child_from_parent(block, parent);
  }

  // destroy the block contents
  block->~_dbBlock();

  // call in-place new to create new block
  new (block) _dbBlock(db);

  // initialize the
  block->initialize(chip, parent, name, delimiter);

  // restore callbacks
  block->callbacks_.swap(callbacks);

  free((void*) name);

  if (block->journal_) {
    delete block->journal_;
    block->journal_ = nullptr;
  }
}

void _dbBlock::initialize(_dbChip* chip,
                          _dbBlock* parent,
                          const char* name,
                          char delimiter)
{
  name_ = safe_strdup(name);

  _dbBox* box = box_tbl_->create();
  box->flags_.owner_type = dbBoxOwner::BLOCK;
  box->owner_ = getOID();
  box->shape_.rect.reset(INT_MAX, INT_MAX, INT_MIN, INT_MIN);
  bbox_ = box->getOID();
  chip_ = chip->getOID();
  hier_delimiter_ = delimiter;
  // create top module
  _dbModule* _top = (_dbModule*) dbModule::create((dbBlock*) this, name);
  top_module_ = _top->getOID();
  if (parent) {
    def_units_ = parent->def_units_;
    dbu_per_micron_ = parent->dbu_per_micron_;
    parent_ = parent->getOID();
    parent->children_.push_back(getOID());
    num_ext_corners_ = parent->num_ext_corners_;
    corners_per_block_ = parent->corners_per_block_;
  }
}

dbObjectTable* _dbBlock::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbInstHdrObj:
      return inst_hdr_tbl_;

    case dbInstObj:
      return inst_tbl_;

    case dbModuleObj:
      return module_tbl_;

    case dbModInstObj:
      return modinst_tbl_;

    case dbPowerDomainObj:
      return powerdomain_tbl_;

    case dbLogicPortObj:
      return logicport_tbl_;

    case dbPowerSwitchObj:
      return powerswitch_tbl_;

    case dbIsolationObj:
      return isolation_tbl_;

    case dbLevelShifterObj:
      return levelshifter_tbl_;

    case dbGroupObj:
      return group_tbl_;

    case dbAccessPointObj:
      return ap_tbl_;

    case dbGlobalConnectObj:
      return global_connect_tbl_;

    case dbGuideObj:
      return guide_tbl_;

    case dbNetTrackObj:
      return net_tracks_tbl_;

    case dbNetObj:
      return net_tbl_;

    case dbBTermObj:
      return bterm_tbl_;

    case dbITermObj:
      return iterm_tbl_;

    case dbBoxObj:
      return box_tbl_;

    case dbViaObj:
      return via_tbl_;

    case dbGCellGridObj:
      return gcell_grid_tbl_;

    case dbTrackGridObj:
      return track_grid_tbl_;

    case dbObstructionObj:
      return obstruction_tbl_;

    case dbBlockageObj:
      return blockage_tbl_;

    case dbWireObj:
      return wire_tbl_;

    case dbSWireObj:
      return swire_tbl_;

    case dbSBoxObj:
      return sbox_tbl_;

    case dbCapNodeObj:
      return cap_node_tbl_;

    case dbRSegObj:
      return r_seg_tbl_;

    case dbCCSegObj:
      return cc_seg_tbl_;

    case dbRowObj:
      return row_tbl_;

    case dbFillObj:
      return fill_tbl_;

    case dbRegionObj:
      return region_tbl_;

    case dbHierObj:
      return hier_tbl_;

    case dbBPinObj:
      return bpin_tbl_;

    case dbTechNonDefaultRuleObj:
      return non_default_rule_tbl_;

    case dbTechLayerRuleObj:
      return layer_rule_tbl_;

    case dbPropertyObj:
      return prop_tbl_;

    case dbDftObj:
      return dft_tbl_;

    default:
      break;
  }

  return getTable()->getObjectTable(type);
}

dbOStream& operator<<(dbOStream& stream, const _dbBTermGroup& obj)
{
  stream << obj.bterms;
  stream << obj.order;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbBTermGroup& obj)
{
  stream >> obj.bterms;
  stream >> obj.order;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbBTermTopLayerGrid& obj)
{
  stream << obj.layer;
  stream << obj.x_step;
  stream << obj.y_step;
  stream << obj.region;
  stream << obj.pin_width;
  stream << obj.pin_height;
  stream << obj.keepout;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbBTermTopLayerGrid& obj)
{
  stream >> obj.layer;
  stream >> obj.x_step;
  stream >> obj.y_step;
  stream >> obj.region;
  stream >> obj.pin_width;
  stream >> obj.pin_height;
  stream >> obj.keepout;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbBlock& block)
{
  std::list<dbBlockCallBackObj*>::const_iterator cbitr;
  for (cbitr = block.callbacks_.begin(); cbitr != block.callbacks_.end();
       ++cbitr) {
    (**cbitr)().inDbBlockStreamOutBefore(
        (dbBlock*) &block);  // client ECO initialization  - payam
  }
  dbOStreamScope scope(stream, "dbBlock");
  stream << block.def_units_;
  stream << block.dbu_per_micron_;
  stream << block.hier_delimiter_;
  stream << block.left_bus_delimiter_;
  stream << block.right_bus_delimiter_;
  stream << block.num_ext_corners_;
  stream << block.corners_per_block_;
  stream << block.corner_name_list_;
  stream << block.name_;
  stream << block.die_area_;
  stream << block.core_area_;
  stream << block.blocked_regions_for_pins_;
  stream << block.chip_;
  stream << block.bbox_;
  stream << block.parent_;
  stream << block.next_block_;
  stream << block.gcell_grid_;
  stream << block.parent_block_;
  stream << block.parent_inst_;
  stream << block.top_module_;
  stream << block.net_hash_;
  stream << block.inst_hash_;
  stream << block.module_hash_;
  stream << block.modinst_hash_;

  stream << block.powerdomain_hash_;
  stream << block.logicport_hash_;
  stream << block.powerswitch_hash_;
  stream << block.isolation_hash_;
  stream << block.levelshifter_hash_;
  stream << block.group_hash_;
  stream << block.inst_hdr_hash_;
  stream << block.bterm_hash_;
  stream << block.max_cap_node_id_;
  stream << block.max_rseg_id_;
  stream << block.max_cc_seg_id_;
  stream << block.children_;
  stream << block.component_mask_shift_;
  stream << block.currentCcAdjOrder_;

  stream << *block.bterm_tbl_;
  stream << *block.iterm_tbl_;
  stream << *block.net_tbl_;
  stream << *block.inst_hdr_tbl_;
  stream << *block.module_tbl_;
  stream << *block.inst_tbl_;
  stream << *block.scan_inst_tbl_;
  stream << *block.modinst_tbl_;
  stream << *block.modbterm_tbl_;
  stream << *block.busport_tbl_;
  stream << *block.moditerm_tbl_;
  stream << *block.modnet_tbl_;
  stream << *block.powerdomain_tbl_;
  stream << *block.logicport_tbl_;
  stream << *block.powerswitch_tbl_;
  stream << *block.isolation_tbl_;
  stream << *block.levelshifter_tbl_;
  stream << *block.group_tbl_;
  stream << *block.ap_tbl_;
  stream << *block.global_connect_tbl_;
  stream << *block.guide_tbl_;
  stream << *block.net_tracks_tbl_;
  stream << *block.box_tbl_;
  stream << *block.via_tbl_;
  stream << *block.gcell_grid_tbl_;
  stream << *block.track_grid_tbl_;
  stream << *block.obstruction_tbl_;
  stream << *block.blockage_tbl_;
  stream << *block.wire_tbl_;
  stream << *block.swire_tbl_;
  stream << *block.sbox_tbl_;
  stream << *block.row_tbl_;
  stream << *block.fill_tbl_;
  stream << *block.region_tbl_;
  stream << *block.hier_tbl_;
  stream << *block.bpin_tbl_;
  stream << *block.non_default_rule_tbl_;
  stream << *block.layer_rule_tbl_;
  stream << *block.prop_tbl_;

  stream << *block.name_cache_;
  stream << *block.r_val_tbl_;
  stream << *block.c_val_tbl_;
  stream << *block.cc_val_tbl_;
  stream << NamedTable("cap_node_tbl", block.cap_node_tbl_);
  stream << NamedTable("r_seg_tbl", block.r_seg_tbl_);
  stream << NamedTable("cc_seg_tbl", block.cc_seg_tbl_);
  stream << *block.ext_control_;
  stream << block.dft_;
  stream << *block.dft_tbl_;
  stream << block.min_routing_layer_;
  stream << block.max_routing_layer_;
  stream << block.min_layer_for_clock_;
  stream << block.max_layer_for_clock_;
  stream << block.bterm_groups_;
  stream << block.bterm_top_layer_grid_;
  stream << block.inst_scan_inst_map_;
  stream << block.unique_net_index_;
  stream << block.unique_inst_index_;

  //---------------------------------------------------------- stream out
  // properties
  // TOM
  dbObjectTable* table = block.getTable();
  dbId<_dbProperty> propList = table->getPropList(block.getOID());

  stream << propList;
  // TOM
  //----------------------------------------------------------

  for (cbitr = block.callbacks_.begin(); cbitr != block.callbacks_.end();
       ++cbitr) {
    (*cbitr)->inDbBlockStreamOutAfter((dbBlock*) &block);
  }
  return stream;
}

/**
 * @brief Rebuilds the name-to-ID hash maps for hierarchical objects within
 * their respective modules.
 *
 * This function is used during database loading (operator>>) to restore the
 * fast-lookup hashes that were stored in the ODB.
 *
 * @tparam T The public ODB object type (e.g., dbInst, dbModNet).
 * @tparam T_impl The internal implementation type (e.g., _dbInst, _dbModNet).
 * @param block The database block containing the objects.
 * @param table The internal database table storing the object implementations.
 * @param module_field Member pointer to the field storing the parent module ID
 *                     (e.g., &_dbInst::module_ or &_dbModInst::parent_).
 * @param hash_field Member pointer to the hash map member in _dbModule where
 *                   the object ID should be stored.
 */
template <typename T, typename T_impl>
static void rebuildModuleHash(
    _dbBlock& block,
    dbTable<T_impl>* table,
    dbId<_dbModule> T_impl::*module_field,
    std::unordered_map<std::string, dbId<T_impl>> _dbModule::*hash_field)
{
  dbSet<T> items((dbBlock*) &block, table);
  for (T* obj : items) {
    T_impl* _obj = (T_impl*) obj;
    dbId<_dbModule> mid = _obj->*module_field;
    if (mid == 0) {
      mid = block.top_module_;
    }
    _dbModule* module = block.module_tbl_->getPtr(mid);
    if (module && _obj->name_) {
      (module->*hash_field)[_obj->name_] = _obj->getId();
    }
  }
}

dbIStream& operator>>(dbIStream& stream, _dbBlock& block)
{
  _dbDatabase* db = block.getImpl()->getDatabase();

  stream >> block.def_units_;
  stream >> block.dbu_per_micron_;
  stream >> block.hier_delimiter_;
  stream >> block.left_bus_delimiter_;
  stream >> block.right_bus_delimiter_;
  stream >> block.num_ext_corners_;
  stream >> block.corners_per_block_;
  stream >> block.corner_name_list_;
  stream >> block.name_;
  if (db->isSchema(kSchemaDieAreaIsPolygon)) {
    stream >> block.die_area_;
  } else {
    Rect rect;
    stream >> rect;
    block.die_area_ = rect;
  }
  if (db->isSchema(kSchemaCoreAreaIsPolygon)) {
    stream >> block.core_area_;
  }
  if (db->isSchema(kSchemaDbBlockBlockedRegionsForPins)) {
    stream >> block.blocked_regions_for_pins_;
  }
  // In the older schema we can't set the tech here, we handle this later in
  // dbDatabase.
  dbId<_dbTech> old_db_tech;
  if (db->isSchema(kSchemaBlockTech) && !db->isSchema(kSchemaChipTech)) {
    stream >> old_db_tech;
  }
  stream >> block.chip_;
  stream >> block.bbox_;
  stream >> block.parent_;
  stream >> block.next_block_;
  stream >> block.gcell_grid_;
  stream >> block.parent_block_;
  stream >> block.parent_inst_;
  stream >> block.top_module_;
  stream >> block.net_hash_;
  stream >> block.inst_hash_;
  stream >> block.module_hash_;
  stream >> block.modinst_hash_;
  if (db->isSchema(kSchemaUpdateHierarchy)) {
    if (!db->isSchema(kSchemaDbRemoveHash)) {
      dbHashTable<_dbModBTerm> unused_modbterm_hash;
      dbHashTable<_dbModITerm> unused_moditerm_hash;
      dbHashTable<_dbModNet> unused_modnet_hash;
      dbHashTable<_dbBusPort> unused_busport_hash;
      stream >> unused_modbterm_hash;
      stream >> unused_moditerm_hash;
      stream >> unused_modnet_hash;
      stream >> unused_busport_hash;
    }
  }
  stream >> block.powerdomain_hash_;
  stream >> block.logicport_hash_;
  stream >> block.powerswitch_hash_;
  stream >> block.isolation_hash_;
  if (db->isSchema(kSchemaLevelShifter)) {
    stream >> block.levelshifter_hash_;
  }
  stream >> block.group_hash_;
  stream >> block.inst_hdr_hash_;
  stream >> block.bterm_hash_;
  stream >> block.max_cap_node_id_;
  stream >> block.max_rseg_id_;
  stream >> block.max_cc_seg_id_;
  if (!db->isSchema(kSchemaBlockExtModelIndex)) {
    int ignore_minExtModelIndex;
    int ignore_maxExtModelIndex;
    stream >> ignore_minExtModelIndex;
    stream >> ignore_maxExtModelIndex;
  }
  stream >> block.children_;
  if (db->isSchema(kSchemaBlockComponentMaskShift)) {
    stream >> block.component_mask_shift_;
  }
  stream >> block.currentCcAdjOrder_;
  stream >> *block.bterm_tbl_;
  stream >> *block.iterm_tbl_;
  stream >> *block.net_tbl_;
  stream >> *block.inst_hdr_tbl_;
  if (db->isSchema(kSchemaDbRemoveHash)) {
    stream >> *block.module_tbl_;
    stream >> *block.inst_tbl_;
  } else {
    stream >> *block.inst_tbl_;
    stream >> *block.module_tbl_;
  }
  if (db->isSchema(kSchemaDbRemoveHash)) {
    // Construct dbinst_hash_
    rebuildModuleHash<dbInst>(
        block, block.inst_tbl_, &_dbInst::module_, &_dbModule::dbinst_hash_);
  }
  if (db->isSchema(kSchemaBlockOwnsScanInsts)) {
    stream >> *block.scan_inst_tbl_;
  }
  stream >> *block.modinst_tbl_;
  if (db->isSchema(kSchemaDbRemoveHash)) {
    // Construct modinst_hash_
    rebuildModuleHash<dbModInst>(block,
                                 block.modinst_tbl_,
                                 &_dbModInst::parent_,
                                 &_dbModule::modinst_hash_);
  }
  if (db->isSchema(kSchemaUpdateHierarchy)) {
    stream >> *block.modbterm_tbl_;
    if (db->isSchema(kSchemaDbRemoveHash)) {
      // Construct modbterm_hash_
      rebuildModuleHash<dbModBTerm>(block,
                                    block.modbterm_tbl_,
                                    &_dbModBTerm::parent_,
                                    &_dbModule::modbterm_hash_);
    }
    if (db->isSchema(kSchemaDbRemoveHash)) {
      stream >> *block.busport_tbl_;
    }
    stream >> *block.moditerm_tbl_;
    stream >> *block.modnet_tbl_;
    if (db->isSchema(kSchemaDbRemoveHash)) {
      // Construct modnet_hash_
      rebuildModuleHash<dbModNet>(block,
                                  block.modnet_tbl_,
                                  &_dbModNet::parent_,
                                  &_dbModule::modnet_hash_);
    }
  }
  stream >> *block.powerdomain_tbl_;
  stream >> *block.logicport_tbl_;
  stream >> *block.powerswitch_tbl_;
  stream >> *block.isolation_tbl_;
  if (db->isSchema(kSchemaLevelShifter)) {
    stream >> *block.levelshifter_tbl_;
  }
  stream >> *block.group_tbl_;
  stream >> *block.ap_tbl_;
  if (db->isSchema(kSchemaAddGlobalConnect)) {
    stream >> *block.global_connect_tbl_;
  }
  stream >> *block.guide_tbl_;
  if (db->isSchema(kSchemaNetTracks)) {
    stream >> *block.net_tracks_tbl_;
  }
  stream >> *block.box_tbl_;
  stream >> *block.via_tbl_;
  stream >> *block.gcell_grid_tbl_;
  stream >> *block.track_grid_tbl_;
  stream >> *block.obstruction_tbl_;
  stream >> *block.blockage_tbl_;
  stream >> *block.wire_tbl_;
  stream >> *block.swire_tbl_;
  stream >> *block.sbox_tbl_;
  stream >> *block.row_tbl_;
  stream >> *block.fill_tbl_;
  stream >> *block.region_tbl_;
  stream >> *block.hier_tbl_;
  stream >> *block.bpin_tbl_;
  stream >> *block.non_default_rule_tbl_;
  stream >> *block.layer_rule_tbl_;
  stream >> *block.prop_tbl_;
  stream >> *block.name_cache_;
  stream >> *block.r_val_tbl_;
  stream >> *block.c_val_tbl_;
  stream >> *block.cc_val_tbl_;
  stream >> *block.cap_node_tbl_;  // DKF
  stream >> *block.r_seg_tbl_;     // DKF
  stream >> *block.cc_seg_tbl_;
  stream >> *block.ext_control_;
  if (db->isSchema(kSchemaAddScan)) {
    stream >> block.dft_;
    stream >> *block.dft_tbl_;
  }
  if (db->isSchema(kSchemaDbMarkerGroup)
      && db->isLessThanSchema(kSchemaChipMarkerCategories)) {
    _dbChip* chip = db->chip_tbl_->getPtr(block.chip_);
    stream >> *chip->marker_categories_tbl_;
    dbHashTable<_dbMarkerCategory> tmp_hash;
    stream >> tmp_hash;
  }
  if (db->isSchema(kSchemaDbBlockLayersRanges)) {
    stream >> block.min_routing_layer_;
    stream >> block.max_routing_layer_;
    stream >> block.min_layer_for_clock_;
    stream >> block.max_layer_for_clock_;
  }
  if (db->isSchema(kSchemaBlockPinGroups)) {
    stream >> block.bterm_groups_;
  }
  if (db->isSchema(kSchemaBtermTopLayerGrid)) {
    stream >> block.bterm_top_layer_grid_;
  }
  if (db->isSchema(kSchemaMapInstsToScanInsts)) {
    stream >> block.inst_scan_inst_map_;
  }
  if (db->isSchema(kSchemaUniqueIndices)) {
    stream >> block.unique_net_index_;
    stream >> block.unique_inst_index_;
  }

  //---------------------------------------------------------- stream in
  // properties
  // TOM
  dbObjectTable* table = block.getTable();
  dbId<_dbProperty> oldList = table->getPropList(block.getOID());
  dbId<_dbProperty> propList;
  stream >> propList;

  if (propList != 0) {
    table->setPropList(block.getOID(), propList);
  } else if (oldList != 0) {
    table->setPropList(block.getOID(), 0);
  }
  // TOM
  //-------------------------------------------------------------------------------

  if (db->isSchema(kSchemaBlockTech) && !db->isSchema(kSchemaChipTech)) {
    _dbChip* chip = db->chip_tbl_->getPtr(block.chip_);
    chip->tech_ = old_db_tech;
  }

  if (!db->isSchema(kSchemaCoreAreaIsPolygon)) {
    // Wait for rows to be available
    dbBlock* blk = (dbBlock*) (&block);
    block.core_area_ = blk->computeCoreArea();
  }

  return stream;
}

void _dbBlock::clearSystemBlockagesAndObstructions()
{
  dbSet<dbBlockage> blockages(this, blockage_tbl_);
  dbSet<dbObstruction> obstructions(this, obstruction_tbl_);

  for (auto blockage = blockages.begin(); blockage != blockages.end();) {
    if (!blockage->isSystemReserved()) {
      blockage++;
      continue;
    }
    blockage->setIsSystemReserved(false);
    blockage = dbBlockage::destroy(blockage);
  }

  for (auto obstruction = obstructions.begin();
       obstruction != obstructions.end();) {
    if (!obstruction->isSystemReserved()) {
      obstruction++;
      continue;
    }
    obstruction->setIsSystemReserved(false);
    obstruction = dbObstruction::destroy(obstruction);
  }
}

void _dbBlock::add_rect(const Rect& rect)
{
  _dbBox* box = box_tbl_->getPtr(bbox_);

  if (flags_.valid_bbox) {
    box->shape_.rect.merge(rect);
  }
}
void _dbBlock::add_oct(const Oct& oct)
{
  _dbBox* box = box_tbl_->getPtr(bbox_);

  if (flags_.valid_bbox) {
    box->shape_.rect.merge(oct);
  }
}

void _dbBlock::remove_rect(const Rect& rect)
{
  _dbBox* box = box_tbl_->getPtr(bbox_);

  if (flags_.valid_bbox) {
    flags_.valid_bbox = box->shape_.rect.inside(rect);
  }
}

bool _dbBlock::operator==(const _dbBlock& rhs) const
{
  if (flags_.valid_bbox != rhs.flags_.valid_bbox) {
    return false;
  }

  if (def_units_ != rhs.def_units_) {
    return false;
  }

  if (dbu_per_micron_ != rhs.dbu_per_micron_) {
    return false;
  }

  if (hier_delimiter_ != rhs.hier_delimiter_) {
    return false;
  }

  if (left_bus_delimiter_ != rhs.left_bus_delimiter_) {
    return false;
  }

  if (right_bus_delimiter_ != rhs.right_bus_delimiter_) {
    return false;
  }

  if (num_ext_corners_ != rhs.num_ext_corners_) {
    return false;
  }

  if (corners_per_block_ != rhs.corners_per_block_) {
    return false;
  }
  if (corner_name_list_ && rhs.corner_name_list_) {
    if (strcmp(corner_name_list_, rhs.corner_name_list_) != 0) {
      return false;
    }
  } else if (corner_name_list_ || rhs.corner_name_list_) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (die_area_ != rhs.die_area_) {
    return false;
  }

  if (core_area_ != rhs.core_area_) {
    return false;
  }

  if (chip_ != rhs.chip_) {
    return false;
  }

  if (bbox_ != rhs.bbox_) {
    return false;
  }

  if (parent_ != rhs.parent_) {
    return false;
  }

  if (next_block_ != rhs.next_block_) {
    return false;
  }

  if (gcell_grid_ != rhs.gcell_grid_) {
    return false;
  }

  if (parent_block_ != rhs.parent_block_) {
    return false;
  }

  if (parent_inst_ != rhs.parent_inst_) {
    return false;
  }

  if (top_module_ != rhs.top_module_) {
    return false;
  }

  if (net_hash_ != rhs.net_hash_) {
    return false;
  }

  if (inst_hash_ != rhs.inst_hash_) {
    return false;
  }

  if (module_hash_ != rhs.module_hash_) {
    return false;
  }

  if (modinst_hash_ != rhs.modinst_hash_) {
    return false;
  }

  if (powerdomain_hash_ != rhs.powerdomain_hash_) {
    return false;
  }

  if (logicport_hash_ != rhs.logicport_hash_) {
    return false;
  }

  if (powerswitch_hash_ != rhs.powerswitch_hash_) {
    return false;
  }

  if (isolation_hash_ != rhs.isolation_hash_) {
    return false;
  }

  if (levelshifter_hash_ != rhs.levelshifter_hash_) {
    return false;
  }

  if (group_hash_ != rhs.group_hash_) {
    return false;
  }

  if (inst_hdr_hash_ != rhs.inst_hdr_hash_) {
    return false;
  }

  if (bterm_hash_ != rhs.bterm_hash_) {
    return false;
  }

  if (max_cap_node_id_ != rhs.max_cap_node_id_) {
    return false;
  }

  if (max_rseg_id_ != rhs.max_rseg_id_) {
    return false;
  }

  if (max_cc_seg_id_ != rhs.max_cc_seg_id_) {
    return false;
  }

  if (children_ != rhs.children_) {
    return false;
  }

  if (component_mask_shift_ != rhs.component_mask_shift_) {
    return false;
  }

  if (currentCcAdjOrder_ != rhs.currentCcAdjOrder_) {
    return false;
  }

  if (*bterm_tbl_ != *rhs.bterm_tbl_) {
    return false;
  }

  if (*iterm_tbl_ != *rhs.iterm_tbl_) {
    return false;
  }

  if (*net_tbl_ != *rhs.net_tbl_) {
    return false;
  }

  if (*inst_hdr_tbl_ != *rhs.inst_hdr_tbl_) {
    return false;
  }

  if (*inst_tbl_ != *rhs.inst_tbl_) {
    return false;
  }

  if (*module_tbl_ != *rhs.module_tbl_) {
    return false;
  }

  if (*modinst_tbl_ != *rhs.modinst_tbl_) {
    return false;
  }

  if (*powerdomain_tbl_ != *rhs.powerdomain_tbl_) {
    return false;
  }

  if (*logicport_tbl_ != *rhs.logicport_tbl_) {
    return false;
  }

  if (*powerswitch_tbl_ != *rhs.powerswitch_tbl_) {
    return false;
  }

  if (*isolation_tbl_ != *rhs.isolation_tbl_) {
    return false;
  }

  if (*levelshifter_tbl_ != *rhs.levelshifter_tbl_) {
    {
      return false;
    }
  }

  if (*group_tbl_ != *rhs.group_tbl_) {
    return false;
  }

  if (*ap_tbl_ != *rhs.ap_tbl_) {
    return false;
  }

  if (*guide_tbl_ != *rhs.guide_tbl_) {
    return false;
  }

  if (*net_tracks_tbl_ != *rhs.net_tracks_tbl_) {
    return false;
  }

  if (*box_tbl_ != *rhs.box_tbl_) {
    return false;
  }

  if (*via_tbl_ != *rhs.via_tbl_) {
    return false;
  }

  if (*gcell_grid_tbl_ != *rhs.gcell_grid_tbl_) {
    return false;
  }

  if (*track_grid_tbl_ != *rhs.track_grid_tbl_) {
    return false;
  }

  if (*obstruction_tbl_ != *rhs.obstruction_tbl_) {
    return false;
  }

  if (*blockage_tbl_ != *rhs.blockage_tbl_) {
    return false;
  }

  if (*wire_tbl_ != *rhs.wire_tbl_) {
    return false;
  }

  if (*swire_tbl_ != *rhs.swire_tbl_) {
    return false;
  }

  if (*sbox_tbl_ != *rhs.sbox_tbl_) {
    return false;
  }

  if (*row_tbl_ != *rhs.row_tbl_) {
    return false;
  }

  if (*fill_tbl_ != *rhs.fill_tbl_) {
    return false;
  }

  if (*region_tbl_ != *rhs.region_tbl_) {
    return false;
  }

  if (*hier_tbl_ != *rhs.hier_tbl_) {
    return false;
  }

  if (*bpin_tbl_ != *rhs.bpin_tbl_) {
    return false;
  }

  if (*non_default_rule_tbl_ != *rhs.non_default_rule_tbl_) {
    return false;
  }

  if (*layer_rule_tbl_ != *rhs.layer_rule_tbl_) {
    return false;
  }

  if (*prop_tbl_ != *rhs.prop_tbl_) {
    return false;
  }

  if (*name_cache_ != *rhs.name_cache_) {
    return false;
  }

  if (*r_val_tbl_ != *rhs.r_val_tbl_) {
    return false;
  }

  if (*c_val_tbl_ != *rhs.c_val_tbl_) {
    return false;
  }

  if (*cc_val_tbl_ != *rhs.cc_val_tbl_) {
    return false;
  }

  if (*cap_node_tbl_ != *rhs.cap_node_tbl_) {
    return false;
  }

  if (*r_seg_tbl_ != *rhs.r_seg_tbl_) {
    return false;
  }

  if (*cc_seg_tbl_ != *rhs.cc_seg_tbl_) {
    return false;
  }

  if (dft_ != rhs.dft_) {
    return false;
  }

  if (*dft_tbl_ != *rhs.dft_tbl_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// dbBlock - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbBlock::getName()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->name_;
}

const char* dbBlock::getConstName()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->name_;
}

dbBox* dbBlock::getBBox()
{
  _dbBlock* block = (_dbBlock*) this;

  if (block->flags_.valid_bbox == 0) {
    block->ComputeBBox();
  }

  _dbBox* bbox = block->box_tbl_->getPtr(block->bbox_);
  return (dbBox*) bbox;
}

void _dbBlock::ComputeBBox()
{
  _dbBox* bbox = box_tbl_->getPtr(bbox_);
  bbox->shape_.rect.reset(INT_MAX, INT_MAX, INT_MIN, INT_MIN);

  for (dbInst* inst : dbSet<dbInst>(this, inst_tbl_)) {
    if (inst->isPlaced()) {
      _dbBox* box = (_dbBox*) inst->getBBox();
      bbox->shape_.rect.merge(box->shape_.rect);
    }
  }

  for (dbBTerm* bterm : dbSet<dbBTerm>(this, bterm_tbl_)) {
    for (dbBPin* bp : bterm->getBPins()) {
      if (bp->getPlacementStatus().isPlaced()) {
        for (dbBox* box : bp->getBoxes()) {
          Rect r = box->getBox();
          bbox->shape_.rect.merge(r);
        }
      }
    }
  }

  for (dbObstruction* obs : dbSet<dbObstruction>(this, obstruction_tbl_)) {
    _dbBox* box = (_dbBox*) obs->getBBox();
    bbox->shape_.rect.merge(box->shape_.rect);
  }

  for (dbSBox* box : dbSet<dbSBox>(this, sbox_tbl_)) {
    Rect rect = box->getBox();
    bbox->shape_.rect.merge(rect);
  }

  for (dbWire* wire : dbSet<dbWire>(this, wire_tbl_)) {
    const auto opt_bbox = wire->getBBox();
    if (opt_bbox) {
      bbox->shape_.rect.merge(opt_bbox.value());
    }
  }

  if (bbox->shape_.rect.xMin() == INT_MAX) {  // empty block
    bbox->shape_.rect.reset(0, 0, 0, 0);
  }

  flags_.valid_bbox = 1;
}

dbDatabase* dbBlock::getDataBase()
{
  dbDatabase* db = (dbDatabase*) getImpl()->getDatabase();
  return db;
}

dbChip* dbBlock::getChip()
{
  _dbBlock* block = (_dbBlock*) this;
  _dbDatabase* db = block->getDatabase();
  _dbChip* chip = db->chip_tbl_->getPtr(block->chip_);
  return (dbChip*) chip;
}

dbBlock* dbBlock::getParent()
{
  _dbBlock* block = (_dbBlock*) this;
  _dbDatabase* db = block->getDatabase();
  _dbChip* chip = db->chip_tbl_->getPtr(block->chip_);
  if (!block->parent_.isValid()) {
    return nullptr;
  }
  _dbBlock* parent = chip->block_tbl_->getPtr(block->parent_);
  return (dbBlock*) parent;
}

dbInst* dbBlock::getParentInst()
{
  _dbBlock* block = (_dbBlock*) this;

  if (block->parent_block_ == 0) {
    return nullptr;
  }

  _dbChip* chip = (_dbChip*) block->getOwner();
  _dbBlock* parent_block = chip->block_tbl_->getPtr(block->parent_block_);
  _dbInst* parent_inst = parent_block->inst_tbl_->getPtr(block->parent_inst_);
  return (dbInst*) parent_inst;
}

dbModule* dbBlock::getTopModule() const
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbModule*) block->module_tbl_->getPtr(block->top_module_);
}

dbSet<dbBlock> dbBlock::getChildren()
{
  _dbBlock* block = (_dbBlock*) this;
  _dbDatabase* db = getImpl()->getDatabase();
  _dbChip* chip = db->chip_tbl_->getPtr(block->chip_);
  return dbSet<dbBlock>(block, chip->block_itr_);
}

dbBlock* dbBlock::findChild(const char* name)
{
  for (dbBlock* child : getChildren()) {
    if (strcmp(child->getConstName(), name) == 0) {
      return child;
    }
  }

  return nullptr;
}

dbSet<dbBTerm> dbBlock::getBTerms()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbBTerm>(block, block->bterm_tbl_);
}

dbBTerm* dbBlock::findBTerm(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbBTerm*) block->bterm_hash_.find(name);
}

std::vector<dbBlock::dbBTermGroup> dbBlock::getBTermGroups()
{
  _dbBlock* block = (_dbBlock*) this;
  std::vector<dbBlock::dbBTermGroup> groups;
  for (const _dbBTermGroup& group : block->bterm_groups_) {
    dbBlock::dbBTermGroup bterm_group;
    for (const auto& bterm_id : group.bterms) {
      bterm_group.bterms.push_back(
          (dbBTerm*) block->bterm_tbl_->getPtr(bterm_id));
    }
    bterm_group.order = group.order;
    groups.push_back(std::move(bterm_group));
  }

  return groups;
}

void dbBlock::addBTermGroup(const std::vector<dbBTerm*>& bterms, bool order)
{
  _dbBlock* block = (_dbBlock*) this;
  _dbBTermGroup group;
  for (dbBTerm* bterm : bterms) {
    group.bterms.emplace_back(bterm->getId());
  }
  group.order = order;
  block->bterm_groups_.push_back(std::move(group));
}

void dbBlock::setBTermTopLayerGrid(
    const dbBlock::dbBTermTopLayerGrid& top_layer_grid)
{
  _dbBlock* block = (_dbBlock*) this;
  _dbBTermTopLayerGrid& top_grid = block->bterm_top_layer_grid_;

  utl::Logger* logger = block->getImpl()->getLogger();
  const odb::Rect& die_area = getDieArea();

  if (!die_area.contains(top_layer_grid.region.getEnclosingRect())) {
    logger->error(
        utl::ODB, 124, "Top layer grid region is out of the die area.");
  }

  if (top_layer_grid.x_step <= 0) {
    logger->error(
        utl::ODB,
        500,
        "The x_step for the top layer grid must be greater than zero.");
  }
  if (top_layer_grid.y_step <= 0) {
    logger->error(
        utl::ODB,
        501,
        "The y_step for the top layer grid must be greater than zero.");
  }
  if (top_layer_grid.pin_width <= 0) {
    logger->error(
        utl::ODB,
        502,
        "The pin width for the top layer grid must be greater than zero.");
  }
  if (top_layer_grid.pin_height <= 0) {
    logger->error(
        utl::ODB,
        503,
        "The pin height for the top layer grid must be greater than zero.");
  }
  if (top_layer_grid.keepout <= 0) {
    logger->error(
        utl::ODB,
        504,
        "The pin keepout for the top layer grid must be greater than zero.");
  }

  top_grid.layer = top_layer_grid.layer->getId();
  top_grid.x_step = top_layer_grid.x_step;
  top_grid.y_step = top_layer_grid.y_step;
  top_grid.region = top_layer_grid.region;
  top_grid.pin_width = top_layer_grid.pin_width;
  top_grid.pin_height = top_layer_grid.pin_height;
  top_grid.keepout = top_layer_grid.keepout;
}

std::optional<dbBlock::dbBTermTopLayerGrid> dbBlock::getBTermTopLayerGrid()
{
  _dbBlock* block = (_dbBlock*) this;

  dbBlock::dbBTermTopLayerGrid top_layer_grid;

  odb::dbTech* tech = getDb()->getTech();
  if (block->bterm_top_layer_grid_.region.getPoints().empty()) {
    return std::nullopt;
  }

  top_layer_grid.layer = odb::dbTechLayer::getTechLayer(
      tech, block->bterm_top_layer_grid_.layer);
  top_layer_grid.x_step = block->bterm_top_layer_grid_.x_step;
  top_layer_grid.y_step = block->bterm_top_layer_grid_.y_step;
  top_layer_grid.region = block->bterm_top_layer_grid_.region;
  top_layer_grid.pin_width = block->bterm_top_layer_grid_.pin_width;
  top_layer_grid.pin_height = block->bterm_top_layer_grid_.pin_height;
  top_layer_grid.keepout = block->bterm_top_layer_grid_.keepout;

  return top_layer_grid;
}

Polygon dbBlock::getBTermTopLayerGridRegion()
{
  _dbBlock* block = (_dbBlock*) this;

  const Polygon& region = block->bterm_top_layer_grid_.region;
  if (region.getPoints().empty()) {
    utl::Logger* logger = block->getImpl()->getLogger();
    logger->error(utl::ODB,
                  428,
                  "Cannot get top layer grid region. Pin placement grid on top "
                  "layer not created.");
  }

  return region;
}

Rect dbBlock::findConstraintRegion(const Direction2D& edge, int begin, int end)
{
  _dbBlock* block = (_dbBlock*) this;
  block->ensureConstraintRegion(edge, begin, end);
  Rect constraint_region;
  const Rect& die_bounds = getDieArea();
  if (edge == south) {
    constraint_region = Rect(begin, die_bounds.yMin(), end, die_bounds.yMin());
  } else if (edge == north) {
    constraint_region = Rect(begin, die_bounds.yMax(), end, die_bounds.yMax());
  } else if (edge == west) {
    constraint_region = Rect(die_bounds.xMin(), begin, die_bounds.xMin(), end);
  } else if (edge == east) {
    constraint_region = Rect(die_bounds.xMax(), begin, die_bounds.xMax(), end);
  }

  return constraint_region;
}

void dbBlock::addBTermConstraintByDirection(dbIoType direction,
                                            const Rect& constraint_region)
{
  for (dbBTerm* bterm : getBTerms()) {
    if (bterm->getIoType() == direction) {
      bterm->setConstraintRegion(constraint_region);
    }
  }
}

void dbBlock::addBTermsToConstraint(const std::vector<dbBTerm*>& bterms,
                                    const Rect& constraint_region)
{
  for (dbBTerm* bterm : bterms) {
    const auto& bterm_constraint = bterm->getConstraintRegion();
    if (bterm_constraint && bterm_constraint.value() != constraint_region) {
      getImpl()->getLogger()->error(
          utl::ODB,
          239,
          "Pin {} is assigned to multiple constraints.",
          bterm->getName());
    } else {
      bterm->setConstraintRegion(constraint_region);
    }
  }
}

dbSet<dbITerm> dbBlock::getITerms()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbITerm>(block, block->iterm_tbl_);
}

dbSet<dbInst> dbBlock::getInsts()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbInst>(block, block->inst_tbl_);
}

dbSet<dbModule> dbBlock::getModules()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbModule>(block, block->module_tbl_);
}

dbSet<dbModInst> dbBlock::getModInsts()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbModInst>(block, block->modinst_tbl_);
}

dbSet<dbModBTerm> dbBlock::getModBTerms()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbModBTerm>(block, block->modbterm_tbl_);
}

dbSet<dbModITerm> dbBlock::getModITerms()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbModITerm>(block, block->moditerm_tbl_);
}

dbSet<dbModNet> dbBlock::getModNets()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbModNet>(block, block->modnet_tbl_);
}

dbSet<dbPowerDomain> dbBlock::getPowerDomains()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbPowerDomain>(block, block->powerdomain_tbl_);
}

dbSet<dbLogicPort> dbBlock::getLogicPorts()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbLogicPort>(block, block->logicport_tbl_);
}

dbSet<dbPowerSwitch> dbBlock::getPowerSwitches()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbPowerSwitch>(block, block->powerswitch_tbl_);
}

dbSet<dbIsolation> dbBlock::getIsolations()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbIsolation>(block, block->isolation_tbl_);
}

dbSet<dbLevelShifter> dbBlock::getLevelShifters()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbLevelShifter>(block, block->levelshifter_tbl_);
}

dbSet<dbGroup> dbBlock::getGroups()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbGroup>(block, block->group_tbl_);
}

dbSet<dbAccessPoint> dbBlock::getAccessPoints()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbAccessPoint>(block, block->ap_tbl_);
}

dbSet<dbGlobalConnect> dbBlock::getGlobalConnects()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbGlobalConnect>(block, block->global_connect_tbl_);
}

std::vector<dbTechLayer*> dbBlock::getComponentMaskShift()
{
  _dbBlock* block = (_dbBlock*) this;
  _dbTech* tech = block->getTech();
  std::vector<dbTechLayer*> layers;
  for (const auto& layer_id : block->component_mask_shift_) {
    layers.push_back((dbTechLayer*) tech->layer_tbl_->getPtr(layer_id));
  }
  return layers;
}

void dbBlock::setComponentMaskShift(const std::vector<dbTechLayer*>& layers)
{
  _dbBlock* block = (_dbBlock*) this;
  for (auto* layer : layers) {
    block->component_mask_shift_.push_back(layer->getId());
  }
}

dbInst* dbBlock::findInst(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbInst*) block->inst_hash_.find(name);
}

dbModule* dbBlock::findModule(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  dbModule* module = (dbModule*) block->module_hash_.find(name);
  if (module != nullptr) {
    return module;
  }
  // Search in children blocks for uninstantiated modules
  dbSet<dbBlock> children = getChildren();
  for (auto* child : children) {
    module = child->findModule(name);
    if (module != nullptr) {
      return module;
    }
  }
  return nullptr;
}

dbPowerDomain* dbBlock::findPowerDomain(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbPowerDomain*) block->powerdomain_hash_.find(name);
}

dbLogicPort* dbBlock::findLogicPort(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbLogicPort*) block->logicport_hash_.find(name);
}

dbPowerSwitch* dbBlock::findPowerSwitch(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbPowerSwitch*) block->powerswitch_hash_.find(name);
}

dbIsolation* dbBlock::findIsolation(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbIsolation*) block->isolation_hash_.find(name);
}

dbLevelShifter* dbBlock::findLevelShifter(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbLevelShifter*) block->levelshifter_hash_.find(name);
}

dbModInst* dbBlock::findModInst(const char* path)
{
  char hier_delimiter[2] = {getHierarchyDelimiter(), '\0'};
  char* _path = strdup(path);
  dbModule* cur_mod = getTopModule();
  dbModInst* cur_inst = nullptr;
  char* token = strtok(_path, hier_delimiter);
  while (token != nullptr) {
    cur_inst = cur_mod->findModInst(token);
    if (cur_inst == nullptr) {
      break;
    }
    cur_mod = cur_inst->getMaster();
    token = strtok(nullptr, hier_delimiter);
  }
  free((void*) _path);
  return cur_inst;
}

dbGroup* dbBlock::findGroup(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbGroup*) block->group_hash_.find(name);
}

dbITerm* dbBlock::findITerm(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;

  std::string s(name);

  std::string::size_type idx = s.rfind(block->hier_delimiter_);

  if (idx == std::string::npos) {  // no delimiter
    return nullptr;
  }

  std::string instName = s.substr(0, idx);
  std::string termName = s.substr(idx + 1, s.size());

  dbInst* inst = findInst(instName.c_str());

  if (inst == nullptr) {
    return nullptr;
  }

  return inst->findITerm(termName.c_str());
}

dbSet<dbObstruction> dbBlock::getObstructions()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbObstruction>(block, block->obstruction_tbl_);
}

dbSet<dbBlockage> dbBlock::getBlockages()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbBlockage>(block, block->blockage_tbl_);
}

dbSet<dbNet> dbBlock::getNets()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbNet>(block, block->net_tbl_);
}

dbSet<dbCapNode> dbBlock::getCapNodes()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbCapNode>(block, block->cap_node_tbl_);
}

dbNet* dbBlock::findNet(const char* name) const
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbNet*) block->net_hash_.find(name);
}

dbModNet* dbBlock::findModNet(const char* hierarchical_name) const
{
  if (hierarchical_name == nullptr || hierarchical_name[0] == '\0') {
    return nullptr;
  }

  std::string path(hierarchical_name);
  std::stringstream ss(path);
  std::string token;
  std::vector<std::string> tokens;
  const char delimiter = getHierarchyDelimiter();

  while (std::getline(ss, token, delimiter)) {
    if (token.empty() == false) {
      tokens.push_back(token);
    }
  }

  if (tokens.empty()) {
    return nullptr;
  }

  dbModule* current_module = getTopModule();

  // Traverse the hierarchy through module instances.
  // The last token is the net name, so iterate up to the second to last token.
  for (size_t i = 0; i < tokens.size() - 1; i++) {
    dbModInst* mod_inst = current_module->findModInst(tokens[i].c_str());
    if (mod_inst == nullptr) {
      return nullptr;  // Invalid path
    }
    current_module = mod_inst->getMaster();
    if (current_module == nullptr) {
      return nullptr;
    }
  }

  // The last token is the ModNet name.
  return current_module->getModNet(tokens.back().c_str());
}

dbVia* dbBlock::findVia(const char* name)
{
  for (dbVia* via : getVias()) {
    if (strcmp(via->getConstName(), name) == 0) {
      return via;
    }
  }

  return nullptr;
}

dbSet<dbVia> dbBlock::getVias()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbVia>(block, block->via_tbl_);
}

dbSet<dbRow> dbBlock::getRows()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbRow>(block, block->row_tbl_);
}

dbSet<dbFill> dbBlock::getFills()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbFill>(block, block->fill_tbl_);
}

dbGCellGrid* dbBlock::getGCellGrid()
{
  _dbBlock* block = (_dbBlock*) this;

  if (block->gcell_grid_ == 0) {
    return nullptr;
  }

  return (dbGCellGrid*) block->gcell_grid_tbl_->getPtr(block->gcell_grid_);
}

dbSet<dbRegion> dbBlock::getRegions()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbRegion>(block, block->region_tbl_);
}

dbRegion* dbBlock::findRegion(const char* name)
{
  for (dbRegion* r : getRegions()) {
    if (r->getName() == name) {
      return r;
    }
  }

  return nullptr;
}

int dbBlock::getDefUnits()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->def_units_;
}

void dbBlock::setDefUnits(int units)
{
  _dbBlock* block = (_dbBlock*) this;
  block->def_units_ = units;
}

int dbBlock::getDbUnitsPerMicron()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->dbu_per_micron_;
}

double dbBlock::dbuToMicrons(int dbu)
{
  const double dbu_micron = getTech()->getDbUnitsPerMicron();
  return dbu / dbu_micron;
}

double dbBlock::dbuToMicrons(unsigned int dbu)
{
  const double dbu_micron = getTech()->getDbUnitsPerMicron();
  return dbu / dbu_micron;
}

double dbBlock::dbuToMicrons(int64_t dbu)
{
  const double dbu_micron = getTech()->getDbUnitsPerMicron();
  return dbu / dbu_micron;
}

double dbBlock::dbuToMicrons(double dbu)
{
  const double dbu_micron = getTech()->getDbUnitsPerMicron();
  return dbu / dbu_micron;
}

double dbBlock::dbuAreaToMicrons(const int64_t dbu_area)
{
  const double dbu_micron = getTech()->getDbUnitsPerMicron();
  return dbu_area / (dbu_micron * dbu_micron);
}

int dbBlock::micronsToDbu(const double microns)
{
  const int dbu_per_micron = getTech()->getDbUnitsPerMicron();
  double dbu = microns * dbu_per_micron;
  return static_cast<int>(std::round(dbu));
}

int64_t dbBlock::micronsAreaToDbu(const double micronsArea)
{
  const int dbu_per_micron = getTech()->getDbUnitsPerMicron();
  const int64_t dbu_per_square_micron
      = static_cast<int64_t>(dbu_per_micron) * dbu_per_micron;
  double dbuArea = micronsArea * dbu_per_square_micron;
  return static_cast<int64_t>(std::round(dbuArea));
}

char dbBlock::getHierarchyDelimiter() const
{
  _dbBlock* block = (_dbBlock*) this;
  return block->hier_delimiter_;
}

void dbBlock::setBusDelimiters(char left, char right)
{
  _dbBlock* block = (_dbBlock*) this;
  block->left_bus_delimiter_ = left;
  block->right_bus_delimiter_ = right;
}

void dbBlock::getBusDelimiters(char& left, char& right)
{
  _dbBlock* block = (_dbBlock*) this;
  left = block->left_bus_delimiter_;
  right = block->right_bus_delimiter_;
}

dbSet<dbTrackGrid> dbBlock::getTrackGrids()
{
  _dbBlock* block = (_dbBlock*) this;
  dbSet<dbTrackGrid> tracks(block, block->track_grid_tbl_);
  return tracks;
}

dbTrackGrid* dbBlock::findTrackGrid(dbTechLayer* layer)
{
  for (dbTrackGrid* g : getTrackGrids()) {
    if (g->getTechLayer() == layer) {
      return g;
    }
  }

  return nullptr;
}

void dbBlock::getMasters(std::vector<dbMaster*>& masters)
{
  _dbBlock* block = (_dbBlock*) this;
  for (dbInstHdr* hdr : dbSet<dbInstHdr>(block, block->inst_hdr_tbl_)) {
    masters.push_back(hdr->getMaster());
  }
}

void dbBlock::setCoreArea(const Rect& new_area)
{
  setCoreArea(Polygon(new_area));
}

void dbBlock::setCoreArea(const Polygon& new_area)
{
  _dbBlock* block = (_dbBlock*) this;

  block->core_area_ = new_area;
  for (auto callback : block->callbacks_) {
    callback->inDbBlockSetCoreArea(this);
  }
}

void dbBlock::setDieArea(const Rect& new_area)
{
  _dbBlock* block = (_dbBlock*) this;

  // Clear any existing system blockages
  block->clearSystemBlockagesAndObstructions();

  block->die_area_ = new_area;
  for (auto callback : block->callbacks_) {
    callback->inDbBlockSetDieArea(this);
  }
}

void dbBlock::setDieArea(const Polygon& new_area)
{
  _dbBlock* block = (_dbBlock*) this;
  block->die_area_ = new_area;
  for (auto callback : block->callbacks_) {
    callback->inDbBlockSetDieArea(this);
  }

  // Clear any existing system blockages
  block->clearSystemBlockagesAndObstructions();

  dbTech* tech = getTech();
  dbSet<dbTechLayer> tech_layers = tech->getLayers();
  // General flow here:
  // We have a polygon floorplan. We can represent this floorplan
  // as a bounding rectangle with obstructions where there is empty
  // space in the polygon.
  //
  // The following code runs a subtraction on the bounding box and
  // the polygon. This generates a list of polygons where there is
  // supposed to be empty space. These polygons are then decomposed
  // into rectangles. Which are then used to create obstructions and
  // placement blockages.

  Polygon bounding_rect = block->die_area_.getEnclosingRect();
  std::vector<Polygon> results = bounding_rect.difference(block->die_area_);
  for (odb::Polygon& blockage_area : results) {
    std::vector<Rect> blockages;
    decompose_polygon(blockage_area.getPoints(), blockages);
    for (odb::Rect& blockage_rect : blockages) {
      dbBlockage* db_blockage = dbBlockage::create(this,
                                                   blockage_rect.xMin(),
                                                   blockage_rect.yMin(),
                                                   blockage_rect.xMax(),
                                                   blockage_rect.yMax());
      db_blockage->setIsSystemReserved(true);

      // Create routing blockages
      for (dbTechLayer* tech_layer : tech_layers) {
        if (tech_layer->getType() == odb::dbTechLayerType::OVERLAP) {
          continue;
        }

        dbObstruction* obs = dbObstruction::create(this,
                                                   tech_layer,
                                                   blockage_rect.xMin(),
                                                   blockage_rect.yMin(),
                                                   blockage_rect.xMax(),
                                                   blockage_rect.yMax());
        obs->setIsSystemReserved(true);
      }
    }
  }
}

Rect dbBlock::getDieArea()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->die_area_.getEnclosingRect();
}

Polygon dbBlock::getDieAreaPolygon()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->die_area_;
}

Rect dbBlock::getCoreArea()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->core_area_.getEnclosingRect();
}

Polygon dbBlock::getCoreAreaPolygon()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->core_area_;
}

void dbBlock::addBlockedRegionForPins(const Rect& region)
{
  _dbBlock* block = (_dbBlock*) this;
  block->blocked_regions_for_pins_.push_back(region);
}

const std::vector<Rect>& dbBlock::getBlockedRegionsForPins()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->blocked_regions_for_pins_;
}

Polygon dbBlock::computeCoreArea()
{
  std::vector<odb::Rect> rows;
  rows.reserve(getRows().size());
  for (dbRow* row : getRows()) {
    if (row->getSite()->getClass() != odb::dbSiteClass::PAD) {
      rows.push_back(row->getBBox());
    }
  }

  if (!rows.empty()) {
    const auto polys = Polygon::merge(rows);

    if (polys.size() > 1) {
      odb::Rect area;
      area.mergeInit();
      for (const auto& row : rows) {
        area.merge(row);
      }
      return area;
    }

    return polys[0];
  }

  // Default to die area if there aren't any rows.
  return getDieAreaPolygon();
}

void dbBlock::setExtmi(void* ext)
{
  _dbBlock* block = (_dbBlock*) this;
  block->extmi_ = ext;
}

void* dbBlock::getExtmi()
{
  _dbBlock* block = (_dbBlock*) this;
  return (block->extmi_);
}

dbExtControl* dbBlock::getExtControl()
{
  _dbBlock* block = (_dbBlock*) this;
  return (block->ext_control_);
}

dbDft* dbBlock::getDft() const
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbDft*) block->dft_tbl_->getPtr(block->dft_);
}

int dbBlock::getMinRoutingLayer() const
{
  _dbBlock* block = (_dbBlock*) this;
  return block->min_routing_layer_;
}

void dbBlock::setMinRoutingLayer(const int min_routing_layer)
{
  _dbBlock* block = (_dbBlock*) this;
  block->min_routing_layer_ = min_routing_layer;
}

int dbBlock::getMaxRoutingLayer() const
{
  _dbBlock* block = (_dbBlock*) this;
  return block->max_routing_layer_;
}

void dbBlock::setMaxRoutingLayer(const int max_routing_layer)
{
  _dbBlock* block = (_dbBlock*) this;
  block->max_routing_layer_ = max_routing_layer;
}

int dbBlock::getMinLayerForClock() const
{
  _dbBlock* block = (_dbBlock*) this;
  return block->min_layer_for_clock_;
}

void dbBlock::setMinLayerForClock(const int min_layer_for_clock)
{
  _dbBlock* block = (_dbBlock*) this;
  block->min_layer_for_clock_ = min_layer_for_clock;
}

int dbBlock::getMaxLayerForClock() const
{
  _dbBlock* block = (_dbBlock*) this;
  return block->max_layer_for_clock_;
}

void dbBlock::setMaxLayerForClock(const int max_layer_for_clock)
{
  _dbBlock* block = (_dbBlock*) this;
  block->max_layer_for_clock_ = max_layer_for_clock;
}

int dbBlock::getGCellTileSize()
{
  _dbBlock* block = (_dbBlock*) this;

  // lambda function to get the average track spacing of a given layer
  auto getAverageTrackSpacing = [this](int layer_idx) -> int {
    dbTech* tech = getTech();
    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer_idx);
    odb::dbTrackGrid* track_grid = findTrackGrid(tech_layer);

    if (track_grid == nullptr) {
      getImpl()->getLogger()->error(
          utl::ODB,
          358,
          "Track grid for routing layer {} not found.",
          tech_layer->getName());
    }

    int track_spacing, track_init, num_tracks;
    track_grid->getAverageTrackSpacing(track_spacing, track_init, num_tracks);
    // for layers with multiple track patterns, ensure even track spacing
    return (track_spacing % 2 == 0) ? track_spacing : track_spacing - 1;
  };

  // Use the pitch of the fourth routing layer as the top option to compute the
  // gcell tile size.
  const int upper_layer_for_gcell_size = 4;
  const int pitches_in_tile = 15;

  if (block->max_routing_layer_ < upper_layer_for_gcell_size) {
    return getAverageTrackSpacing(block->max_routing_layer_) * pitches_in_tile;
  }

  // Use the middle track spacing between M2, M3 and M4
  std::vector<int> track_spacings = {getAverageTrackSpacing(2),
                                     getAverageTrackSpacing(3),
                                     getAverageTrackSpacing(4)};
  std::ranges::sort(track_spacings);

  return track_spacings[1] * pitches_in_tile;
}

void dbBlock::getExtCornerNames(std::list<std::string>& ecl)
{
  _dbBlock* block = (_dbBlock*) this;
  if (block->corner_name_list_) {
    ecl.emplace_back(block->corner_name_list_);
  } else {
    ecl.emplace_back();
  }
}

dbSet<dbCCSeg> dbBlock::getCCSegs()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbCCSeg>(block, block->cc_seg_tbl_);
}

dbSet<dbRSeg> dbBlock::getRSegs()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbRSeg>(block, block->r_seg_tbl_);
}

dbTechNonDefaultRule* dbBlock::findNonDefaultRule(const char* name)
{
  for (dbTechNonDefaultRule* r : getNonDefaultRules()) {
    if (strcmp(r->getConstName(), name) == 0) {
      return r;
    }
  }

  return nullptr;
}

dbSet<dbTechNonDefaultRule> dbBlock::getNonDefaultRules()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbTechNonDefaultRule>(block, block->non_default_rule_tbl_);
}

void dbBlock::copyExtDb(uint32_t fr,
                        uint32_t to,
                        uint32_t extDbCnt,
                        double resFactor,
                        double ccFactor,
                        double gndcFactor)
{
  _dbBlock* block = (_dbBlock*) this;
  uint32_t j;
  if (resFactor != 1.0) {
    for (j = 1; j < block->r_val_tbl_->size(); j += extDbCnt) {
      (*block->r_val_tbl_)[j + to] = (*block->r_val_tbl_)[j + fr] * resFactor;
    }
  } else {
    for (j = 1; j < block->r_val_tbl_->size(); j += extDbCnt) {
      (*block->r_val_tbl_)[j + to] = (*block->r_val_tbl_)[j + fr];
    }
  }
  if (ccFactor != 1.0) {
    for (j = 1; j < block->cc_val_tbl_->size(); j += extDbCnt) {
      (*block->cc_val_tbl_)[j + to] = (*block->cc_val_tbl_)[j + fr] * ccFactor;
    }
  } else {
    for (j = 1; j < block->cc_val_tbl_->size(); j += extDbCnt) {
      (*block->cc_val_tbl_)[j + to] = (*block->cc_val_tbl_)[j + fr];
    }
  }
  if (gndcFactor != 1.0) {
    for (j = 1; j < block->c_val_tbl_->size(); j += extDbCnt) {
      (*block->c_val_tbl_)[j + to] = (*block->c_val_tbl_)[j + fr] * gndcFactor;
    }
  } else {
    for (j = 1; j < block->c_val_tbl_->size(); j += extDbCnt) {
      (*block->c_val_tbl_)[j + to] = (*block->c_val_tbl_)[j + fr];
    }
  }
}

bool dbBlock::adjustCC(float adjFactor,
                       double ccThreshHold,
                       std::vector<dbNet*>& nets,
                       std::vector<dbNet*>& halonets)
{
  bool adjusted = false;
  _dbBlock* block = (_dbBlock*) this;
  std::vector<dbCCSeg*> adjustedCC;
  const uint32_t adjustOrder = block->currentCcAdjOrder_ + 1;
  for (dbNet* net : nets) {
    adjusted |= net->adjustCC(
        adjustOrder, adjFactor, ccThreshHold, adjustedCC, halonets);
  }
  for (dbCCSeg* ccs : adjustedCC) {
    ccs->setMark(false);
  }
  if (adjusted) {
    block->currentCcAdjOrder_ = adjustOrder;
  }
  return adjusted;
}

bool dbBlock::groundCC(float gndFactor)
{
  if (gndFactor == 0.0) {
    return false;
  }
  bool grounded = false;
  for (dbNet* net : getNets()) {
    grounded |= net->groundCC(gndFactor);
  }
  for (dbNet* net : getNets()) {
    net->destroyCCSegs();
  }
  return grounded;
}

void dbBlock::undoAdjustedCC(std::vector<dbNet*>& nets,
                             std::vector<dbNet*>& halonets)
{
  std::vector<dbCCSeg*> adjustedCC;
  for (dbNet* net : getNets()) {
    net->undoAdjustedCC(adjustedCC, halonets);
  }
  for (dbCCSeg* ccs : adjustedCC) {
    ccs->setMark(false);
  }
}

void dbBlock::adjustRC(double resFactor, double ccFactor, double gndcFactor)
{
  _dbBlock* block = (_dbBlock*) this;
  uint32_t j;
  if (resFactor != 1.0) {
    for (j = 1; j < block->r_val_tbl_->size(); j++) {
      (*block->r_val_tbl_)[j] *= resFactor;
    }
  }
  if (ccFactor != 1.0) {
    for (j = 1; j < block->cc_val_tbl_->size(); j++) {
      (*block->cc_val_tbl_)[j] *= ccFactor;
    }
  }
  if (gndcFactor != 1.0) {
    for (j = 1; j < block->c_val_tbl_->size(); j++) {
      (*block->c_val_tbl_)[j] *= gndcFactor;
    }
  }
}

void dbBlock::getExtCount(int& numOfNet,
                          int& numOfRSeg,
                          int& numOfCapNode,
                          int& numOfCCSeg)
{
  _dbBlock* block = (_dbBlock*) this;
  numOfNet = block->net_tbl_->size();
  numOfRSeg = block->r_seg_tbl_->size();
  numOfCapNode = block->cap_node_tbl_->size();
  numOfCCSeg = block->cc_seg_tbl_->size();
}
/*
void dbBlock::setMinExtMOdel(int n)
{
    _dbBlock * block = (_dbBlock *) this;
    block->_min_ext_model= n;
}
void dbBlock::setMaxExtMOdel(int n)
{
    _dbBlock * block = (_dbBlock *) this;
    block->_max_ext_model= n;
}
int dbBlock::getMaxExtMOdel()
{
    _dbBlock * block = (_dbBlock *) this;
    return block->_max_ext_model;
}
int dbBlock::getMinExtMOdel()
{
    _dbBlock * block = (_dbBlock *) this;
    return block->_min_ext_model;
}
*/
int dbBlock::getCornerCount()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->num_ext_corners_;
}

int dbBlock::getCornersPerBlock()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->corners_per_block_;
}

bool dbBlock::extCornersAreIndependent()
{
  bool independent = getExtControl()->_independentExtCorners;
  return independent;
}

int dbBlock::getExtDbCount()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->num_ext_dbs_;
}

void dbBlock::initParasiticsValueTables()
{
  _dbBlock* block = (_dbBlock*) this;
  if ((block->r_seg_tbl_->size() > 0) || (block->cap_node_tbl_->size() > 0)
      || (block->cc_seg_tbl_->size() > 0)) {
    for (dbNet* net : getNets()) {
      _dbNet* n = (_dbNet*) net;
      n->cap_nodes_ = 0;
      n->r_segs_ = 0;
    }
  }

  int ttttClear = 1;
  _dbDatabase* db = block->cap_node_tbl_->db_;
  if (ttttClear) {
    block->cap_node_tbl_->clear();
  } else {
    delete block->cap_node_tbl_;
    block->cap_node_tbl_ = new dbTable<_dbCapNode, 4096>(
        db, block, (GetObjTbl_t) &_dbBlock::getObjectTable, dbCapNodeObj);
  }
  block->max_cap_node_id_ = 0;

  if (ttttClear) {
    block->r_seg_tbl_->clear();
  } else {
    delete block->r_seg_tbl_;
    block->r_seg_tbl_ = new dbTable<_dbRSeg, 4096>(
        db, block, (GetObjTbl_t) &_dbBlock::getObjectTable, dbRSegObj);
  }
  block->max_rseg_id_ = 0;

  if (ttttClear) {
    block->cc_seg_tbl_->clear();
  } else {
    delete block->cc_seg_tbl_;
    block->cc_seg_tbl_ = new dbTable<_dbCCSeg, 4096>(
        db, block, (GetObjTbl_t) &_dbBlock::getObjectTable, dbCCSegObj);
  }
  block->max_cc_seg_id_ = 0;

  if (ttttClear) {
    block->cc_val_tbl_->clear();
  } else {
    delete block->cc_val_tbl_;
    block->cc_val_tbl_ = new dbPagedVector<float, 4096, 12>();
  }
  block->cc_val_tbl_->push_back(0.0);

  if (ttttClear) {
    block->r_val_tbl_->clear();
  } else {
    delete block->r_val_tbl_;
    block->r_val_tbl_ = new dbPagedVector<float, 4096, 12>();
  }
  block->r_val_tbl_->push_back(0.0);

  if (ttttClear) {
    block->c_val_tbl_->clear();
  } else {
    delete block->c_val_tbl_;
    block->c_val_tbl_ = new dbPagedVector<float, 4096, 12>();
  }
  block->c_val_tbl_->push_back(0.0);
}

void dbBlock::setCornersPerBlock(int cornersPerBlock)
{
  initParasiticsValueTables();
  _dbBlock* block = (_dbBlock*) this;
  block->corners_per_block_ = cornersPerBlock;
}

void dbBlock::setCornerCount(int cornersStoredCnt,
                             int extDbCnt,
                             const char* name_list)
{
  assert((cornersStoredCnt > 0) && (cornersStoredCnt <= 256));
  _dbBlock* block = (_dbBlock*) this;

  // TODO: Should this change be logged in the journal?
  //       Yes !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbBlock {}, setCornerCount cornerCnt {}, extDbCnt {}, "
             "name_list {}",
             block->getId(),
             cornersStoredCnt,
             extDbCnt,
             name_list);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbBlockObj);
    block->journal_->pushParam(block->getId());
    block->journal_->pushParam(_dbBlock::kCornerCount);
    block->journal_->pushParam(cornersStoredCnt);
    block->journal_->pushParam(extDbCnt);
    block->journal_->pushParam(name_list);
    block->journal_->endAction();
  }
  initParasiticsValueTables();

  block->num_ext_corners_ = cornersStoredCnt;
  block->corners_per_block_ = cornersStoredCnt;
  block->num_ext_dbs_ = extDbCnt;
  if (name_list != nullptr) {
    if (block->corner_name_list_) {
      free(block->corner_name_list_);
    }
    block->corner_name_list_ = strdup((char*) name_list);
  }
}
dbBlock* dbBlock::getExtCornerBlock(uint32_t corner)
{
  dbBlock* block = findExtCornerBlock(corner);
  if (!block) {
    block = this;
  }
  return block;
}

dbBlock* dbBlock::findExtCornerBlock(uint32_t corner)
{
  char cornerName[64];
  sprintf(cornerName, "extCornerBlock__%d", corner);
  return findChild(cornerName);
}

dbBlock* dbBlock::createExtCornerBlock(uint32_t corner)
{
  char cornerName[64];
  sprintf(cornerName, "extCornerBlock__%d", corner);
  dbBlock* extBlk = dbBlock::create(this, cornerName, '/');
  assert(extBlk);
  char name[64];
  for (dbNet* net : getNets()) {
    sprintf(name, "%d", net->getId());
    dbNet* xnet = dbNet::create(extBlk, name, true);
    if (xnet == nullptr) {
      getImpl()->getLogger()->error(
          utl::ODB, 8, "Cannot duplicate net {}", net->getConstName());
    }
    if (xnet->getId() != net->getId()) {
      getImpl()->getLogger()->warn(utl::ODB,
                                   9,
                                   "id mismatch ({},{}) for net {}",
                                   xnet->getId(),
                                   net->getId(),
                                   net->getConstName());
    }
    dbSigType ty = net->getSigType();
    if (ty.isSupply()) {
      xnet->setSpecial();
    }

    xnet->setSigType(ty);
  }
  extBlk->setCornersPerBlock(1);
  return extBlk;
}

char* dbBlock::getCornerNameList()
{
  _dbBlock* block = (_dbBlock*) this;

  return block->corner_name_list_;
}
void dbBlock::setCornerNameList(const char* name_list)
{
  _dbBlock* block = (_dbBlock*) this;

  if (block->corner_name_list_ != nullptr) {
    free(block->corner_name_list_);
  }

  block->corner_name_list_ = strdup(name_list);
}
std::string dbBlock::getExtCornerName(const int corner)
{
  _dbBlock* block = (_dbBlock*) this;
  if (block->num_ext_corners_ == 0) {
    return "";
  }
  assert((corner >= 0) && (corner < block->num_ext_corners_));

  if (block->corner_name_list_ == nullptr) {
    return "";
  }

  std::stringstream ss(block->corner_name_list_);
  std::string word;
  int ii = 0;

  while (ss >> word) {
    if (ii == corner) {
      return word;
    }
    ii++;
  }
  return "";
}

int dbBlock::getExtCornerIndex(const char* cornerName)
{
  _dbBlock* block = (_dbBlock*) this;

  if (block->corner_name_list_ == nullptr) {
    return -1;
  }

  std::stringstream ss(block->corner_name_list_);
  std::string word;
  int ii = 0;

  while (ss >> word) {
    if (cornerName == word) {
      return ii;
    }
    ii++;
  }
  return -1;
}

void dbBlock::setCornerCount(int cnt)
{
  setCornerCount(cnt, cnt, nullptr);
}

dbBlock* dbBlock::create(dbChip* chip_, const char* name_, char hier_delimiter_)
{
  _dbChip* chip = (_dbChip*) chip_;

  if (chip->top_ != 0) {
    return nullptr;
  }

  _dbBlock* top = chip->block_tbl_->create();
  top->initialize(chip, nullptr, name_, hier_delimiter_);
  chip->top_ = top->getOID();
  top->dbu_per_micron_ = chip_->getTech()->getDbUnitsPerMicron();
  return (dbBlock*) top;
}

dbBlock* dbBlock::create(dbBlock* parent_,
                         const char* name_,
                         char hier_delimiter)
{
  if (parent_->findChild(name_)) {
    return nullptr;
  }

  _dbBlock* parent = (_dbBlock*) parent_;
  _dbChip* chip = (_dbChip*) parent->getOwner();
  _dbBlock* child = chip->block_tbl_->create();
  child->initialize(chip, parent, name_, hier_delimiter);
  child->dbu_per_micron_ = parent_->getTech()->getDbUnitsPerMicron();
  return (dbBlock*) child;
}

dbBlock* dbBlock::getBlock(dbChip* chip_, uint32_t dbid_)
{
  _dbChip* chip = (_dbChip*) chip_;
  return (dbBlock*) chip->block_tbl_->getPtr(dbid_);
}

dbBlock* dbBlock::getBlock(dbBlock* block_, uint32_t dbid_)
{
  _dbChip* chip = (_dbChip*) block_->getImpl()->getOwner();
  return (dbBlock*) chip->block_tbl_->getPtr(dbid_);
}

void dbBlock::destroy(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbChip* chip = (_dbChip*) block->getOwner();
  // delete the children of this block
  for (const dbId<_dbBlock>& child_id : block->children_) {
    _dbBlock* child = chip->block_tbl_->getPtr(child_id);
    destroy((dbBlock*) child);
  }
  // Deleting top block
  if (block->parent_ == 0) {
    chip->top_ = 0;
  } else {
    // unlink this block from the parent
    _dbBlock* parent = chip->block_tbl_->getPtr(block->parent_);
    unlink_child_from_parent(block, parent);
  }

  dbProperty::destroyProperties(block);
  chip->block_tbl_->destroy(block);
}

void unlink_child_from_parent(_dbBlock* child, _dbBlock* parent)
{
  uint32_t id = child->getOID();

  auto& children = parent->children_;
  for (auto citr = children.begin(); citr != children.end(); ++citr) {
    if (*citr == id) {
      children.erase(citr);
      break;
    }
  }
}

dbSet<dbBlock>::iterator dbBlock::destroy(dbSet<dbBlock>::iterator& itr)
{
  dbBlock* bt = *itr;
  dbSet<dbBlock>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbBlockSearch* dbBlock::getSearchDb()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->search_db_;
}

void dbBlock::getWireUpdatedNets(std::vector<dbNet*>& result)
{
  int tot = 0;
  int upd = 0;
  int enc = 0;
  for (dbNet* net : getNets()) {
    tot++;
    _dbNet* n = (_dbNet*) net;

    if (n->flags_.wire_altered != 1) {
      continue;
    }
    upd++;
    enc++;

    result.push_back(net);
  }
  getImpl()->getLogger()->info(
      utl::ODB, 10, "tot = {}, upd = {}, enc = {}", tot, upd, enc);
}

void dbBlock::destroyCCs(std::vector<dbNet*>& nets)
{
  for (dbNet* net : nets) {
    net->destroyCCSegs();
  }
}

void dbBlock::destroyRSegs(std::vector<dbNet*>& nets)
{
  for (dbNet* net : nets) {
    net->destroyRSegs();
  }
}

void dbBlock::destroyCNs(std::vector<dbNet*>& nets, bool cleanExtid)
{
  for (dbNet* net : nets) {
    net->destroyCapNodes(cleanExtid);
  }
}

void dbBlock::destroyCornerParasitics(std::vector<dbNet*>& nets)
{
  std::vector<dbNet*> cnets;
  uint32_t jj;
  for (jj = 0; jj < nets.size(); jj++) {
    dbNet* net = dbNet::getNet(this, nets[jj]->getId());
    cnets.push_back(net);
  }
  destroyCCs(cnets);
  destroyRSegs(cnets);
  destroyCNs(cnets, true);
}

void dbBlock::destroyParasitics(std::vector<dbNet*>& nets)
{
  destroyCornerParasitics(nets);
  if (!extCornersAreIndependent()) {
    return;
  }
  int numcorners = getCornerCount();
  dbBlock* extBlock;
  for (int corner = 1; corner < numcorners; corner++) {
    extBlock = findExtCornerBlock(corner);
    extBlock->destroyCornerParasitics(nets);
  }
}

void dbBlock::getCcHaloNets(std::vector<dbNet*>& changedNets,
                            std::vector<dbNet*>& ccHaloNets)
{
  uint32_t jj;
  dbNet* ccNet;
  for (jj = 0; jj < changedNets.size(); jj++) {
    changedNets[jj]->setMark(true);
  }
  for (jj = 0; jj < changedNets.size(); jj++) {
    for (dbCapNode* capn : changedNets[jj]->getCapNodes()) {
      dbSet<dbCCSeg> ccSegs = capn->getCCSegs();
      for (auto ccitr = ccSegs.begin(); ccitr != ccSegs.end();) {
        dbCCSeg* cc = *ccitr;
        ++ccitr;
        dbCapNode* tcap = cc->getSourceCapNode();
        if (tcap != capn) {
          ccNet = tcap->getNet();
        } else {
          ccNet = cc->getTargetCapNode()->getNet();
        }
        if (ccNet->isMarked()) {
          continue;
        }
        ccNet->setMark(true);
        ccHaloNets.push_back(ccNet);
      }
    }
  }
  for (jj = 0; jj < changedNets.size(); jj++) {
    changedNets[jj]->setMark(false);
  }
  for (jj = 0; jj < ccHaloNets.size(); jj++) {
    ccHaloNets[jj]->setMark(false);
  }
}

//
// Utility to write db file
//

void dbBlock::writeDb(char* filename, int allNode)
{
  _dbBlock* block = (_dbBlock*) this;
  std::string dbname;
  if (allNode) {
    if (block->journal_) {
      dbname = fmt::format("{}.main.{}.db", filename, getpid());
    } else {
      dbname = fmt::format("{}.remote.{}.db", filename, getpid());
    }
  } else {
    dbname = fmt::format("{}.db", filename);
  }
  std::ofstream file(dbname, std::ios::binary);
  if (!file) {
    getImpl()->getLogger()->warn(
        utl::ODB, 19, "Can not open file {} to write!", dbname);
    return;
  }
  getDataBase()->write(file);
  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbBlock {}, writeDb",
             block->getId());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbBlockObj);
    block->journal_->pushParam(block->getId());
    block->journal_->pushParam(_dbBlock::kWriteDb);
    block->journal_->pushParam(filename);
    block->journal_->pushParam(allNode);
    block->journal_->endAction();
  }
}

void dbBlock::writeGuides(const char* filename) const
{
  std::ofstream guide_file;
  guide_file.open(filename);
  if (!guide_file) {
    getImpl()->getLogger()->error(
        utl::ODB, 307, "Guides file could not be opened.");
  }
  dbBlock* block = (dbBlock*) this;
  std::vector<dbNet*> nets;
  nets.reserve(block->getNets().size());
  for (auto net : block->getNets()) {
    if (!net->getGuides().empty()) {
      nets.push_back(net);
    }
  }
  std::ranges::sort(nets, [](odb::dbNet* net1, odb::dbNet* net2) {
    return strcmp(net1->getConstName(), net2->getConstName()) < 0;
  });
  for (auto net : nets) {
    guide_file << net->getName() << "\n";
    guide_file << "(\n";
    for (auto guide : net->getGuides()) {
      guide_file << guide->getBox().xMin() << " " << guide->getBox().yMin()
                 << " " << guide->getBox().xMax() << " "
                 << guide->getBox().yMax() << " "
                 << guide->getLayer()->getName() << "\n";
    }
    guide_file << ")\n";
  }
  guide_file.close();
}

void dbBlock::clearUserInstFlags()
{
  for (dbInst* inst : getInsts()) {
    inst->clearUserFlag2();
    inst->clearUserFlag1();
    inst->clearUserFlag3();
  }
}

std::map<dbTechLayer*, odb::dbTechVia*> dbBlock::getDefaultVias()
{
  odb::dbTech* tech = getTech();
  odb::dbSet<odb::dbTechVia> vias = tech->getVias();
  std::map<dbTechLayer*, odb::dbTechVia*> default_vias;

  for (odb::dbTechVia* via : vias) {
    odb::dbStringProperty* prop
        = odb::dbStringProperty::find(via, "OR_DEFAULT");

    if (prop == nullptr) {
      continue;
    }
    default_vias[via->getBottomLayer()] = via;
  }

  if (default_vias.empty()) {
    utl::Logger* logger = getImpl()->getLogger();
    debugPrint(
        logger, utl::ODB, "get_default_vias", 1, "No OR_DEFAULT vias defined.");
    for (odb::dbTechVia* via : vias) {
      dbTechLayer* tech_layer = via->getBottomLayer();
      if (tech_layer != nullptr && tech_layer->getRoutingLevel() != 0
          && default_vias.find(tech_layer) == default_vias.end()) {
        debugPrint(logger,
                   utl::ODB,
                   "get_default_vias",
                   1,
                   "Via for layers {} and {}: {}",
                   via->getBottomLayer()->getName(),
                   via->getTopLayer()->getName(),
                   via->getName());
        default_vias[tech_layer] = via;
        debugPrint(logger,
                   utl::ODB,
                   "get_default_vias",
                   1,
                   "Using via {} as default.",
                   via->getConstName());
      }
    }
  }

  return default_vias;
}

void dbBlock::destroyRoutes()
{
  dbBlock* block = this;
  for (odb::dbNet* db_net : block->getNets()) {
    if (!db_net->getSigType().isSupply() && !db_net->isSpecial()
        && db_net->getSWires().empty() && !db_net->isConnectedByAbutment()) {
      odb::dbWire* wire = db_net->getWire();
      if (wire != nullptr) {
        odb::dbWire::destroy(wire);
      }
    }
  }
}

void dbBlock::setDrivingItermsforNets()
{
  for (dbNet* net : getNets()) {
    if ((net->getSigType() == dbSigType::GROUND)
        || (net->getSigType() == dbSigType::POWER)) {
      continue;
    }

    net->setDrivingITerm(nullptr);

    for (dbITerm* tr : net->getITerms()) {
      if (tr->getIoType() == dbIoType::OUTPUT) {
        net->setDrivingITerm(tr);
        break;
      }
    }
  }
}

void dbBlock::preExttreeMergeRC(double max_cap, uint32_t corner)
{
  if (!getExtControl()->_exttreePreMerg) {
    return;
  }
  if (max_cap == 0.0) {
    max_cap = 10.0;
  }
  if (getExtControl()->_exttreeMaxcap >= max_cap) {
    return;
  }
  getExtControl()->_exttreeMaxcap = max_cap;
  for (dbNet* net : getNets()) {
    net->preExttreeMergeRC(max_cap, corner);
  }
}

bool dbBlock::designIsRouted(bool verbose)
{
  bool design_is_routed = true;
  for (dbNet* net : getNets()) {
    if (net->isSpecial()) {
      continue;
    }

    const int pin_count = net->getBTermCount() + net->getITerms().size();

    uint32_t wire_cnt = 0, via_cnt = 0;
    net->getWireCount(wire_cnt, via_cnt);
    bool has_wires = wire_cnt != 0 || via_cnt != 0;

    if (pin_count > 1 && !has_wires && !net->isConnectedByAbutment()) {
      if (verbose) {
        getImpl()->getLogger()->warn(
            utl::ODB, 232, "Net {} is not routed.", net->getName());
        design_is_routed = false;
      } else {
        return false;
      }
    }
  }
  return design_is_routed;
}

void dbBlock::destroyNetWires()
{
  for (dbNet* db_net : getNets()) {
    dbWire* wire = db_net->getWire();
    if (!db_net->isSpecial() && wire) {
      dbWire::destroy(wire);
    }
  }
}

int dbBlock::globalConnect(bool force, bool verbose)
{
  dbSet<dbGlobalConnect> gcs = getGlobalConnects();
  const std::vector<dbGlobalConnect*> connects(gcs.begin(), gcs.end());
  _dbBlock* dbblock = (_dbBlock*) this;
  return dbblock->globalConnect(connects, force, verbose);
}

int dbBlock::globalConnect(dbGlobalConnect* gc, bool force, bool verbose)
{
  _dbBlock* dbblock = (_dbBlock*) this;
  return dbblock->globalConnect({gc}, force, verbose);
}

void dbBlock::clearGlobalConnect()
{
  dbSet<dbGlobalConnect> gcs = getGlobalConnects();
  std::set<dbGlobalConnect*> connects(gcs.begin(), gcs.end());
  for (auto* connect : connects) {
    odb::dbGlobalConnect::destroy(connect);
  }
}

void dbBlock::reportGlobalConnect()
{
  _dbBlock* dbblock = (_dbBlock*) this;
  utl::Logger* logger = dbblock->getImpl()->getLogger();

  dbSet<dbGlobalConnect> gcs = getGlobalConnects();

  logger->report("Global connection rules: {}", gcs.size());
  for (auto* connect : gcs) {
    auto* region = connect->getRegion();

    std::string region_msg;
    if (region != nullptr) {
      region_msg = fmt::format(" in \"{}\"", region->getName());
    }
    logger->report("  Instances{}: \"{}\" with pins \"{}\" connect to \"{}\"",
                   region_msg,
                   connect->getInstPattern(),
                   connect->getPinPattern(),
                   connect->getNet()->getName());
  }
}

int dbBlock::addGlobalConnect(dbRegion* region,
                              const char* instPattern,
                              const char* pinPattern,
                              dbNet* net,
                              bool do_connect)
{
  _dbBlock* dbblock = (_dbBlock*) this;

  utl::Logger* logger = dbblock->getImpl()->getLogger();

  if (net == nullptr) {
    logger->error(utl::ODB, 381, "Invalid net specified.");
  }

  if (net->isDoNotTouch()) {
    logger->warn(utl::ODB,
                 382,
                 "{} is marked do not touch, which will cause the global "
                 "connect rule to be ignored.",
                 net->getName());
  }

  dbGlobalConnect* gc
      = odb::dbGlobalConnect::create(net, region, instPattern, pinPattern);

  if (gc != nullptr && do_connect) {
    return globalConnect(gc, false, false);
  }
  return 0;
}

int _dbBlock::globalConnect(const std::vector<dbGlobalConnect*>& connects,
                            bool force,
                            bool verbose)
{
  _dbBlock* dbblock = (_dbBlock*) this;
  utl::Logger* logger = dbblock->getImpl()->getLogger();

  if (connects.empty()) {
    logger->warn(utl::ODB, 378, "Global connections are not set up.");
    return 0;
  }

  // order rules so non-regions are handled first
  std::vector<_dbGlobalConnect*> non_region_rules;
  std::vector<_dbGlobalConnect*> region_rules;

  std::set<dbITerm*> connected_iterms;
  std::set<dbITerm*> skipped_iterms;
  // only search for instances once
  std::map<std::string, std::vector<dbInst*>> inst_map;
  std::set<dbInst*> donottouchinsts;
  for (dbGlobalConnect* connect : connects) {
    _dbGlobalConnect* gc = (_dbGlobalConnect*) connect;
    if (gc->region_ != 0) {
      region_rules.push_back(gc);
    } else {
      non_region_rules.push_back(gc);
    }
    const std::string inst_pattern = connect->getInstPattern();
    if (inst_map.find(inst_pattern) != inst_map.end()) {
      continue;
    }
    std::vector<dbInst*> insts = connect->getInsts();

    // remove insts marked do not touch
    std::set<dbInst*> remove_insts;
    for (dbInst* inst : insts) {
      if (inst->isDoNotTouch()) {
        remove_insts.insert(inst);
      }
    }
    auto [first, last] = std::ranges::remove_if(insts, [&](dbInst* inst) {
      return remove_insts.find(inst) != remove_insts.end();
    });
    insts.erase(first, last);

    inst_map[inst_pattern] = std::move(insts);

    for (dbInst* inst : remove_insts) {
      if (gc->needsModification(inst)) {
        donottouchinsts.insert(inst);
      }
    }
  }

  if (!donottouchinsts.empty()) {
    for (dbInst* inst : donottouchinsts) {
      logger->warn(utl::ODB,
                   383,
                   "{} is marked do not touch and will be skipped in global "
                   "connections.",
                   inst->getName());
    }
  }

  for (_dbGlobalConnect* connect : non_region_rules) {
    const auto& [connections, skipped]
        = connect->connect(inst_map[connect->inst_pattern_], force);
    connected_iterms.insert(connections.begin(), connections.end());
    skipped_iterms.insert(skipped.begin(), skipped.end());
  }
  for (_dbGlobalConnect* connect : region_rules) {
    const auto& [connections, skipped]
        = connect->connect(inst_map[connect->inst_pattern_], force);
    connected_iterms.insert(connections.begin(), connections.end());
    skipped_iterms.insert(skipped.begin(), skipped.end());
  }

  if (verbose) {
    logger->info(utl::ODB,
                 403,
                 "{} connections made, {} conflicts skipped{}",
                 connected_iterms.size(),
                 skipped_iterms.size(),
                 skipped_iterms.empty() ? "." : ", use -force to connect.");
  }

  return connected_iterms.size();
}

_dbTech* _dbBlock::getTech()
{
  dbBlock* block = (dbBlock*) this;
  return (_dbTech*) block->getTech();
}

dbTech* dbBlock::getTech()
{
  return getChip()->getTech();
}

dbSet<dbMarkerCategory> dbBlock::getMarkerCategories()
{
  return getChip()->getMarkerCategories();
}

dbMarkerCategory* dbBlock::findMarkerCategory(const char* name)
{
  return getChip()->findMarkerCategory(name);
}

void dbBlock::writeMarkerCategories(const std::string& file)
{
  std::ofstream report(file);

  if (!report) {
    _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
    utl::Logger* logger = obj->getLogger();

    logger->error(utl::ODB, 272, "Unable to open {} to write markers", file);
  }

  writeMarkerCategories(report);

  report.close();
}

void dbBlock::writeMarkerCategories(std::ofstream& report)
{
  std::set<_dbMarkerCategory*> groups;
  for (dbMarkerCategory* group : getMarkerCategories()) {
    groups.insert((_dbMarkerCategory*) group);
  }

  _dbMarkerCategory::writeJSON(report, groups);
}

void dbBlock::debugPrintContent(std::ostream& str_db)
{
  str_db << fmt::format("Debug: Data base tables for block at {}:\n",
                        getName());

  str_db << "Db nets (The Flat db view)\n";

  for (auto dbnet : getNets()) {
    str_db << fmt::format(
        "dbNet {} (id {})\n", dbnet->getName(), dbnet->getId());

    for (auto db_iterm : dbnet->getITerms()) {
      str_db << fmt::format(
          "\t-> dbIterm {} ({})\n", db_iterm->getId(), db_iterm->getName());
    }
    for (auto db_bterm : dbnet->getBTerms()) {
      str_db << fmt::format("\t-> dbBterm {}\n", db_bterm->getId());
    }
  }

  str_db << "Block ports\n";
  // got through the ports and their owner
  str_db << "\t\tBTerm Ports +++\n";
  for (auto bt : getBTerms()) {
    str_db << fmt::format("\t\tBterm ({}) {} Net {} ({})  Mod Net {} ({}) \n",
                          bt->getId(),
                          bt->getName().c_str(),
                          bt->getNet() ? bt->getNet()->getName().c_str() : "",
                          bt->getNet() ? bt->getNet()->getId() : 0,
                          bt->getModNet() ? bt->getModNet()->getName() : "",
                          bt->getModNet() ? bt->getModNet()->getId() : 0);
  }
  str_db << "\t\tBTerm Ports ---\n";

  str_db << "The hierarchical db view:\n";
  dbSet<dbModule> block_modules = getModules();
  str_db << fmt::format("Content size {} modules\n", block_modules.size());
  for (auto mi : block_modules) {
    dbModule* cur_obj = mi;
    if (cur_obj == getTopModule()) {
      str_db << "Top Module\n";
    }
    str_db << fmt::format("\tModule {} {}\n",
                          (cur_obj == getTopModule()) ? "(Top Module)" : "",
                          cur_obj->getName());
    // in case of top level, care as the bterms double up as pins
    if (cur_obj == getTopModule()) {
      for (auto bterm : getBTerms()) {
        str_db << fmt::format(
            "Top dbBTerm {} dbNet {} ({}) dbModNet {} ({})\n",
            bterm->getName(),
            bterm->getNet() ? bterm->getNet()->getName() : "",
            bterm->getNet() ? bterm->getNet()->getId() : -1,
            bterm->getModNet() ? bterm->getModNet()->getName() : "",
            bterm->getModNet() ? bterm->getModNet()->getId() : -1);
      }
    }
    // got through the module ports and their owner
    str_db << "\t\tModBTerm Ports +++\n";

    for (auto module_port : cur_obj->getModBTerms()) {
      str_db << fmt::format(
          "\t\tPort {} Net {} ({})\n",
          module_port->getName(),
          (module_port->getModNet()) ? (module_port->getModNet()->getName())
                                     : "No-modnet",
          (module_port->getModNet()) ? module_port->getModNet()->getId() : -1);

      str_db << fmt::format("\t\tPort parent {}\n\n",
                            module_port->getParent()->getName());
    }
    str_db << "\t\tModBTermPorts ---\n";

    str_db << "\t\tModule instances +++\n";
    for (auto module_inst : mi->getModInsts()) {
      str_db << fmt::format("\t\tMod inst {} ", module_inst->getName());
      dbModule* master = module_inst->getMaster();
      str_db << fmt::format("\t\tMaster {}\n\n",
                            module_inst->getMaster()->getName());
      dbBlock* owner = master->getOwner();
      if (owner != this) {
        str_db << "\t\t\tMaster owner in wrong block\n";
      }
      str_db << "\t\tConnections\n";
      for (dbModITerm* miterm_pin : module_inst->getModITerms()) {
        str_db << fmt::format(
            "\t\t\tModIterm : {} ({}) Mod Net {} ({}) \n",
            miterm_pin->getName(),
            miterm_pin->getId(),
            miterm_pin->getModNet() ? (miterm_pin->getModNet()->getName())
                                    : "No-net",
            miterm_pin->getModNet() ? miterm_pin->getModNet()->getId() : 0);
      }
    }
    str_db << "\t\tModule instances ---\n";
    str_db << "\t\tDb instances +++\n";
    for (dbInst* db_inst : cur_obj->getInsts()) {
      str_db << fmt::format("\t\tdb inst {}\n", db_inst->getName());
      str_db << "\t\tdb iterms:\n";
      for (dbITerm* iterm : db_inst->getITerms()) {
        dbMTerm* mterm = iterm->getMTerm();
        str_db << fmt::format(
            "\t\t\t\t iterm: {} ({}) Net: {} Mod net : {} ({})\n",
            mterm->getName(),
            iterm->getId(),
            iterm->getNet() ? iterm->getNet()->getName() : "unk-dbnet",
            iterm->getModNet() ? iterm->getModNet()->getName() : "unk-modnet",
            iterm->getModNet() ? iterm->getModNet()->getId() : -1);
      }
    }
    str_db << "\t\tDb instances ---\n";
    str_db << "\tModule nets (modnets) +++ \n";
    str_db << fmt::format("\t# mod nets {} in {}\n",
                          cur_obj->getModNets().size(),
                          cur_obj->getName());

    for (auto mod_net : cur_obj->getModNets()) {
      str_db << fmt::format(
          "\t\tNet: {} ({})\n", mod_net->getName(), mod_net->getId());
      str_db << "\t\tConnections -> modIterms/modbterms/bterms/iterms:\n";
      str_db << fmt::format("\t\t -> {} moditerms\n",
                            mod_net->getModITerms().size());
      for (dbModITerm* modi_term : mod_net->getModITerms()) {
        str_db << fmt::format("\t\t\t{}\n", modi_term->getName());
      }
      str_db << fmt::format("\t\t -> {} modbterms\n",
                            mod_net->getModBTerms().size());
      for (dbModBTerm* modb_term : mod_net->getModBTerms()) {
        str_db << fmt::format("\t\t\t{}\n", modb_term->getName());
      }
      str_db << fmt::format("\t\t -> {} iterms\n", mod_net->getITerms().size());
      for (dbITerm* db_iterm : mod_net->getITerms()) {
        str_db << fmt::format("\t\t\t{}\n", db_iterm->getName().c_str());
      }
      str_db << fmt::format("\t\t -> {} bterms\n", mod_net->getBTerms().size());
      for (dbBTerm* db_bterm : mod_net->getBTerms()) {
        str_db << fmt::format("\t\t\t{}\n", db_bterm->getName().c_str());
      }
    }
  }
}

void _dbBlock::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["corner_name"].add(corner_name_list_);
  info.children["blocked_regions_for_pins"].add(blocked_regions_for_pins_);

  info.children["net_hash"].add(net_hash_);
  info.children["inst_hash"].add(inst_hash_);
  info.children["module_hash"].add(module_hash_);
  info.children["modinst_hash"].add(modinst_hash_);
  info.children["powerdomain_hash"].add(powerdomain_hash_);
  info.children["logicport_hash"].add(logicport_hash_);
  info.children["powerswitch_hash"].add(powerswitch_hash_);
  info.children["isolation_hash"].add(isolation_hash_);
  info.children["levelshifter_hash"].add(levelshifter_hash_);
  info.children["group_hash"].add(group_hash_);
  info.children["inst_hdr_hash"].add(inst_hdr_hash_);
  info.children["bterm_hash"].add(bterm_hash_);

  info.children["children"].add(children_);
  info.children["component_mask_shift"].add(component_mask_shift_);

  bterm_tbl_->collectMemInfo(info.children["bterm"]);
  iterm_tbl_->collectMemInfo(info.children["iterm"]);
  net_tbl_->collectMemInfo(info.children["net"]);
  inst_hdr_tbl_->collectMemInfo(info.children["inst_hdr"]);
  inst_tbl_->collectMemInfo(info.children["inst"]);
  box_tbl_->collectMemInfo(info.children["box"]);
  via_tbl_->collectMemInfo(info.children["via"]);
  gcell_grid_tbl_->collectMemInfo(info.children["gcell_grid"]);
  track_grid_tbl_->collectMemInfo(info.children["track_grid"]);
  obstruction_tbl_->collectMemInfo(info.children["obstruction"]);
  blockage_tbl_->collectMemInfo(info.children["blockage"]);
  wire_tbl_->collectMemInfo(info.children["wire"]);
  swire_tbl_->collectMemInfo(info.children["swire"]);
  sbox_tbl_->collectMemInfo(info.children["sbox"]);
  row_tbl_->collectMemInfo(info.children["row"]);
  fill_tbl_->collectMemInfo(info.children["fill"]);
  region_tbl_->collectMemInfo(info.children["region"]);
  hier_tbl_->collectMemInfo(info.children["hier"]);
  bpin_tbl_->collectMemInfo(info.children["bpin"]);
  non_default_rule_tbl_->collectMemInfo(info.children["non_default_rule"]);
  layer_rule_tbl_->collectMemInfo(info.children["layer_rule"]);
  prop_tbl_->collectMemInfo(info.children["prop"]);
  module_tbl_->collectMemInfo(info.children["module"]);
  powerdomain_tbl_->collectMemInfo(info.children["powerdomain"]);
  logicport_tbl_->collectMemInfo(info.children["logicport"]);
  powerswitch_tbl_->collectMemInfo(info.children["powerswitch"]);
  isolation_tbl_->collectMemInfo(info.children["isolation"]);
  levelshifter_tbl_->collectMemInfo(info.children["levelshifter"]);
  modinst_tbl_->collectMemInfo(info.children["modinst"]);
  group_tbl_->collectMemInfo(info.children["group"]);
  ap_tbl_->collectMemInfo(info.children["ap"]);
  global_connect_tbl_->collectMemInfo(info.children["global_connect"]);
  guide_tbl_->collectMemInfo(info.children["guide"]);
  net_tracks_tbl_->collectMemInfo(info.children["net_tracks"]);
  dft_tbl_->collectMemInfo(info.children["dft"]);
  modbterm_tbl_->collectMemInfo(info.children["modbterm"]);
  moditerm_tbl_->collectMemInfo(info.children["moditerm"]);
  modnet_tbl_->collectMemInfo(info.children["modnet"]);
  busport_tbl_->collectMemInfo(info.children["busport"]);
  cap_node_tbl_->collectMemInfo(info.children["cap_node"]);
  r_seg_tbl_->collectMemInfo(info.children["r_seg"]);
  cc_seg_tbl_->collectMemInfo(info.children["cc_seg"]);

  name_cache_->collectMemInfo(info.children["name_cache"]);
  info.children["r_val"].add(*r_val_tbl_);
  info.children["c_val"].add(*c_val_tbl_);
  info.children["cc_val"].add(*cc_val_tbl_);

  info.children["module_name_id_map"].add(module_name_id_map_);
}

void _dbBlock::ensureConstraintRegion(const Direction2D& edge,
                                      int& begin,
                                      int& end)
{
  /// Ensure that the constraint region defined in the given edge is completely
  /// inside the die area.
  dbBlock* block = (dbBlock*) this;
  const int input_begin = begin;
  const int input_end = end;
  const Rect& die_bounds = block->getDieArea();
  if (edge == south || edge == north) {
    begin = std::max(begin, die_bounds.xMin());
    end = std::min(end, die_bounds.xMax());
  } else if (edge == west || edge == east) {
    begin = std::max(begin, die_bounds.yMin());
    end = std::min(end, die_bounds.yMax());
  }

  if (input_begin != begin || input_end != end) {
    utl::Logger* logger = getImpl()->getLogger();
    logger->warn(utl::ODB,
                 11,
                 "Region {}-{} on edge {} modified to {}-{} to respect the "
                 "die area limits.",
                 block->dbuToMicrons(input_begin),
                 block->dbuToMicrons(input_end),
                 edge,
                 block->dbuToMicrons(begin),
                 block->dbuToMicrons(end));
  }
}

std::string _dbBlock::makeNewName(
    dbModInst* parent,
    const char* base_name,
    const dbNameUniquifyType& uniquify,
    uint32_t& unique_index,
    const std::function<bool(const char*)>& exists)
{
  dbBlock* block = (dbBlock*) this;

  // Decide hierarchical name without unique index
  fmt::memory_buffer buf;
  if (parent) {
    fmt::format_to(std::back_inserter(buf),
                   "{}{}",
                   parent->getHierarchicalName(),
                   block->getHierarchyDelimiter());
  }
  buf.append(fmt::string_view(base_name));
  buf.push_back('\0');  // Null-terminate for find* functions

  // If uniquify is IF_NEEDED*, check for uniqueness before adding a suffix.
  bool uniquify_if_needed
      = (dbNameUniquifyType::IF_NEEDED == uniquify)
        || (dbNameUniquifyType::IF_NEEDED_WITH_UNDERSCORE == uniquify);
  if (uniquify_if_needed && !exists(buf.data())) {
    return std::string(buf.data());
  }

  // The name is not unique, or we must uniquify.
  // Prepare the prefix for the uniquifying loop.
  buf.resize(buf.size() - 1);  // Remove null terminator

  // Use underscore if specified
  if ((dbNameUniquifyType::IF_NEEDED_WITH_UNDERSCORE == uniquify
       || dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE == uniquify)) {
    buf.push_back('_');
  }

  const size_t prefix_size = buf.size();
  do {
    buf.resize(prefix_size);
    fmt::format_to(std::back_inserter(buf), "{}", unique_index++);
    buf.push_back('\0');  // Null-terminate
  } while (exists(buf.data()));
  return std::string(buf.data());  // Returns a new unique full name
}

////////////////////////////////////////////////////////////////
// TODO:
//----
// when making a unique net name search within the scope of the
// containing module only (parent scope module) which is passed in.
// This requires scoping nets in the module in hierarchical mode
// (as was done with dbInsts) and will require changing the
// method: (dbNetwork::name).
// Currently all nets are scoped within a dbBlock.
//

// If uniquify is IF_NEEDED, unique suffix will be added when necessary.
// This is added to cover the existing multiple use cases of making a
// new net name w/ and w/o unique suffix.
std::string dbBlock::makeNewNetName(dbModInst* parent,
                                    const char* base_name,
                                    const dbNameUniquifyType& uniquify)
{
  _dbBlock* block = reinterpret_cast<_dbBlock*>(this);
  auto exists = [this](const char* name) { return findNet(name) != nullptr; };
  return block->makeNewName(
      parent, base_name, uniquify, block->unique_net_index_, exists);
}

std::string dbBlock::makeNewModNetName(dbModule* parent,
                                       const char* base_name,
                                       const dbNameUniquifyType& uniquify,
                                       dbNet* corresponding_flat_net)
{
  auto exists = [parent, this, corresponding_flat_net](const char* full_name) {
    const char* base_name_ptr = getBaseName(full_name);
    if (parent->getModNet(base_name_ptr)
        || parent->findModBTerm(base_name_ptr)) {
      return true;
    }

    // Internal flat net collision check (uses full hierarchical name)
    // - Allow collision with the corresponding flat net (same logical net)
    // - Collision with other internal flat nets is not allowed
    dbNet* existing_net = findNet(full_name);
    if (existing_net != nullptr && existing_net != corresponding_flat_net) {
      if (existing_net->isInternalTo(parent)) {
        return true;  // Collision with a different internal flat net
      }
    }

    return false;
  };

  _dbBlock* block = reinterpret_cast<_dbBlock*>(this);
  return block->makeNewName(parent->getModInst(),
                            base_name,
                            uniquify,
                            block->unique_net_index_,
                            exists);
}

std::string dbBlock::makeNewInstName(dbModInst* parent,
                                     const char* base_name,
                                     const dbNameUniquifyType& uniquify)
{
  // NOTE: TODO: The scoping should be within
  // the dbModule scope for the instance, not the whole network.
  // dbInsts are already scoped within a dbModule
  // To get the dbModule for a dbInst used inst -> getModule
  // then search within that scope. That way the instance name
  // does not have to be some massive string like root/X/Y/U1.
  //
  _dbBlock* block = reinterpret_cast<_dbBlock*>(this);
  auto exists = [this](const char* name) {
    return findInst(name) != nullptr || findModInst(name) != nullptr;
  };
  return block->makeNewName(
      parent, base_name, uniquify, block->unique_inst_index_, exists);
}

const char* dbBlock::getBaseName(const char* full_name) const
{
  // If name contains the hierarchy delimiter, use the partial string
  // after the last occurrence of the hierarchy delimiter.
  // This prevents a very long term/net name creation when the name
  // begins with a back-slash as "\soc/module1/instance_a/.../clk_port"
  const char* last_hier_delimiter = strrchr(full_name, getHierarchyDelimiter());
  if (last_hier_delimiter != nullptr) {
    return last_hier_delimiter + 1;
  }
  return full_name;
}

dbModITerm* dbBlock::findModITerm(const char* hierarchical_name)
{
  if (hierarchical_name == nullptr) {
    return nullptr;
  }

  const char* last_delim = strrchr(hierarchical_name, getHierarchyDelimiter());
  if (last_delim == nullptr) {
    return nullptr;
  }

  std::string inst_path(hierarchical_name, last_delim - hierarchical_name);
  const char* term_name = last_delim + 1;

  dbModInst* inst = findModInst(inst_path.c_str());
  if (inst) {
    return inst->findModITerm(term_name);
  }
  return nullptr;
}

dbModBTerm* dbBlock::findModBTerm(const char* hierarchical_name)
{
  if (hierarchical_name == nullptr) {
    return nullptr;
  }

  const char* last_delim = strrchr(hierarchical_name, getHierarchyDelimiter());
  if (last_delim == nullptr) {
    // Top level port
    if (dbModule* top_module = getTopModule()) {
      return top_module->findModBTerm(hierarchical_name);
    }
    return nullptr;
  }

  std::string inst_path(hierarchical_name, last_delim - hierarchical_name);
  const char* term_name = last_delim + 1;

  dbModInst* inst = findModInst(inst_path.c_str());
  if (inst) {
    if (dbModule* master = inst->getMaster()) {
      return master->findModBTerm(term_name);
    }
  }
  return nullptr;
}

}  // namespace odb
