// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBlock.h"

#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "dbAccessPoint.h"
#include "dbArrayTable.h"
#include "dbArrayTable.hpp"
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
#include "dbHashTable.hpp"
#include "dbHier.h"
#include "dbITerm.h"
#include "dbITermItr.h"
#include "dbInst.h"
#include "dbInstHdr.h"
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
#include "dbTable.h"
#include "dbTable.hpp"
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
#include "odb/dbShape.h"
#include "odb/defout.h"
#include "odb/geom_boost.h"
#include "odb/lefout.h"
#include "odb/poly_decomp.h"
#include "utl/Logger.h"

namespace odb {

struct OldTransform
{
  int _orient;
  int _originX;
  int _originY;
  int _sizeX;
  int _sizeY;
};

dbIStream& operator>>(dbIStream& stream, OldTransform& t)
{
  stream >> t._orient;
  stream >> t._originX;
  stream >> t._originY;
  stream >> t._sizeX;
  stream >> t._sizeY;
  return stream;
}

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
  _flags._valid_bbox = 0;
  _flags._spare_bits = 0;
  _def_units = 100;
  _dbu_per_micron = 1000;
  _hier_delimiter = 0;
  _left_bus_delimiter = 0;
  _right_bus_delimiter = 0;
  _num_ext_corners = 0;
  _corners_per_block = 0;
  _corner_name_list = nullptr;
  _name = nullptr;
  _die_area = Rect(0, 0, 0, 0);
  _maxCapNodeId = 0;
  _maxRSegId = 0;
  _maxCCSegId = 0;
  _min_routing_layer = 1;
  _max_routing_layer = -1;
  _min_layer_for_clock = -1;
  _max_layer_for_clock = -2;

  _currentCcAdjOrder = 0;
  _bterm_tbl = new dbTable<_dbBTerm>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbBTermObj);

  _iterm_tbl = new dbTable<_dbITerm>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbITermObj, 1024, 10);

  _net_tbl = new dbTable<_dbNet>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbNetObj);

  _inst_hdr_tbl = new dbTable<_dbInstHdr>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbInstHdrObj);

  _inst_tbl = new dbTable<_dbInst>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbInstObj);

  _module_tbl = new dbTable<_dbModule>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbModuleObj);

  _modinst_tbl = new dbTable<_dbModInst>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbModInstObj);

  _modbterm_tbl = new dbTable<_dbModBTerm>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbModBTermObj);

  _moditerm_tbl = new dbTable<_dbModITerm>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbModITermObj);

  _modnet_tbl = new dbTable<_dbModNet>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbModNetObj);

  _busport_tbl = new dbTable<_dbBusPort>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbBusPortObj);

  _powerdomain_tbl = new dbTable<_dbPowerDomain>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbPowerDomainObj);

  _logicport_tbl = new dbTable<_dbLogicPort>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbLogicPortObj);

  _powerswitch_tbl = new dbTable<_dbPowerSwitch>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbPowerSwitchObj);

  _isolation_tbl = new dbTable<_dbIsolation>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbIsolationObj);

  _levelshifter_tbl = new dbTable<_dbLevelShifter>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbLevelShifterObj);

  _group_tbl = new dbTable<_dbGroup>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbGroupObj);

  ap_tbl_ = new dbTable<_dbAccessPoint>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbAccessPointObj);

  global_connect_tbl_ = new dbTable<_dbGlobalConnect>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbGlobalConnectObj);

  _guide_tbl = new dbTable<_dbGuide>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbGuideObj);

  _net_tracks_tbl = new dbTable<_dbNetTrack>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbNetTrackObj);

  _box_tbl = new dbTable<_dbBox>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbBoxObj, 1024, 10);

  _via_tbl = new dbTable<_dbVia>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbViaObj, 1024, 10);

  _gcell_grid_tbl = new dbTable<_dbGCellGrid>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbGCellGridObj);

  _track_grid_tbl = new dbTable<_dbTrackGrid>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbTrackGridObj);

  _obstruction_tbl = new dbTable<_dbObstruction>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbObstructionObj);

  _blockage_tbl = new dbTable<_dbBlockage>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbBlockageObj);

  _wire_tbl = new dbTable<_dbWire>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbWireObj);

  _swire_tbl = new dbTable<_dbSWire>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbSWireObj);

  _sbox_tbl = new dbTable<_dbSBox>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbSBoxObj);

  _row_tbl = new dbTable<_dbRow>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbRowObj);

  _fill_tbl = new dbTable<_dbFill>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbFillObj);

  _region_tbl = new dbTable<_dbRegion>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbRegionObj, 32, 5);

  _hier_tbl = new dbTable<_dbHier>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbHierObj, 16, 4);

  _bpin_tbl = new dbTable<_dbBPin>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbBPinObj);

  _non_default_rule_tbl = new dbTable<_dbTechNonDefaultRule>(
      db,
      this,
      (GetObjTbl_t) &_dbBlock::getObjectTable,
      dbTechNonDefaultRuleObj,
      16,
      4);

  _layer_rule_tbl
      = new dbTable<_dbTechLayerRule>(db,
                                      this,
                                      (GetObjTbl_t) &_dbBlock::getObjectTable,
                                      dbTechLayerRuleObj,
                                      16,
                                      4);

  _prop_tbl = new dbTable<_dbProperty>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbPropertyObj);

  _name_cache
      = new _dbNameCache(db, this, (GetObjTbl_t) &_dbBlock::getObjectTable);

  _r_val_tbl = new dbPagedVector<float, 4096, 12>();
  _r_val_tbl->push_back(0.0);

  _c_val_tbl = new dbPagedVector<float, 4096, 12>();
  _c_val_tbl->push_back(0.0);

  _cc_val_tbl = new dbPagedVector<float, 4096, 12>();
  _cc_val_tbl->push_back(0.0);

  _cap_node_tbl
      = new dbTable<_dbCapNode>(db,
                                this,
                                (GetObjTbl_t) &_dbBlock::getObjectTable,
                                dbCapNodeObj,
                                4096,
                                12);

  // We need to allocate the first cap-node (id == 1) to resolve a problem with
  // the extraction code (Hopefully this is temporary)
  _cap_node_tbl->create();

  _r_seg_tbl = new dbTable<_dbRSeg>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbRSegObj, 4096, 12);

  _cc_seg_tbl = new dbTable<_dbCCSeg>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbCCSegObj, 4096, 12);

  _extControl = new dbExtControl();

  _dft_tbl = new dbTable<_dbDft>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbDftObj, 4096, 12);
  _dbDft* dft_ptr = _dft_tbl->create();
  dft_ptr->initialize();
  _dft = dft_ptr->getId();

  _marker_categories_tbl = new dbTable<_dbMarkerCategory>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbMarkerCategoryObj);

  _net_hash.setTable(_net_tbl);
  _inst_hash.setTable(_inst_tbl);
  _module_hash.setTable(_module_tbl);
  _modinst_hash.setTable(_modinst_tbl);
  _powerdomain_hash.setTable(_powerdomain_tbl);
  _logicport_hash.setTable(_logicport_tbl);
  _powerswitch_hash.setTable(_powerswitch_tbl);
  _isolation_hash.setTable(_isolation_tbl);
  _levelshifter_hash.setTable(_levelshifter_tbl);
  _group_hash.setTable(_group_tbl);
  _inst_hdr_hash.setTable(_inst_hdr_tbl);
  _bterm_hash.setTable(_bterm_tbl);
  _marker_category_hash.setTable(_marker_categories_tbl);

  _net_bterm_itr = new dbNetBTermItr(_bterm_tbl);

  _net_iterm_itr = new dbNetITermItr(_iterm_tbl);

  _inst_iterm_itr = new dbInstITermItr(_iterm_tbl);

  _box_itr = new dbBoxItr(_box_tbl, nullptr, false);

  _swire_itr = new dbSWireItr(_swire_tbl);

  _sbox_itr = new dbSBoxItr(_sbox_tbl);

  _cap_node_itr = new dbCapNodeItr(_cap_node_tbl);

  _r_seg_itr = new dbRSegItr(_r_seg_tbl);

  _cc_seg_itr = new dbCCSegItr(_cc_seg_tbl);

  _region_inst_itr = new dbRegionInstItr(_inst_tbl);

  _module_inst_itr = new dbModuleInstItr(_inst_tbl);

  _module_modinst_itr = new dbModuleModInstItr(_modinst_tbl);

  _module_modinstmoditerm_itr = new dbModuleModInstModITermItr(_moditerm_tbl);

  _module_modbterm_itr = new dbModuleModBTermItr(_modbterm_tbl);

  _module_modnet_itr = new dbModuleModNetItr(_modnet_tbl);

  _module_modnet_modbterm_itr = new dbModuleModNetModBTermItr(_modbterm_tbl);
  _module_modnet_moditerm_itr = new dbModuleModNetModITermItr(_moditerm_tbl);
  _module_modnet_iterm_itr = new dbModuleModNetITermItr(_iterm_tbl);
  _module_modnet_bterm_itr = new dbModuleModNetBTermItr(_bterm_tbl);

  _region_group_itr = new dbRegionGroupItr(_group_tbl);

  _group_itr = new dbGroupItr(_group_tbl);

  _guide_itr = new dbGuideItr(_guide_tbl);

  _net_track_itr = new dbNetTrackItr(_net_tracks_tbl);

  _group_inst_itr = new dbGroupInstItr(_inst_tbl);

  _group_modinst_itr = new dbGroupModInstItr(_modinst_tbl);

  _group_power_net_itr = new dbGroupPowerNetItr(_net_tbl);

  _group_ground_net_itr = new dbGroupGroundNetItr(_net_tbl);

  _bpin_itr = new dbBPinItr(_bpin_tbl);

  _prop_itr = new dbPropertyItr(_prop_tbl);

  _num_ext_dbs = 1;
  _searchDb = nullptr;
  _extmi = nullptr;
  _journal = nullptr;
  _journal_pending = nullptr;
}

_dbBlock::~_dbBlock()
{
  if (_name) {
    free((void*) _name);
  }

  delete _bterm_tbl;
  delete _iterm_tbl;
  delete _net_tbl;
  delete _inst_hdr_tbl;
  delete _inst_tbl;
  delete _module_tbl;
  delete _modinst_tbl;
  delete _modbterm_tbl;
  delete _moditerm_tbl;
  delete _modnet_tbl;
  delete _busport_tbl;
  delete _powerdomain_tbl;
  delete _logicport_tbl;
  delete _powerswitch_tbl;
  delete _isolation_tbl;
  delete _levelshifter_tbl;
  delete _group_tbl;
  delete ap_tbl_;
  delete global_connect_tbl_;
  delete _guide_tbl;
  delete _net_tracks_tbl;
  delete _box_tbl;
  delete _via_tbl;
  delete _gcell_grid_tbl;
  delete _track_grid_tbl;
  delete _obstruction_tbl;
  delete _blockage_tbl;
  delete _wire_tbl;
  delete _swire_tbl;
  delete _sbox_tbl;
  delete _row_tbl;
  delete _fill_tbl;
  delete _region_tbl;
  delete _hier_tbl;
  delete _bpin_tbl;
  delete _non_default_rule_tbl;
  delete _layer_rule_tbl;
  delete _prop_tbl;
  delete _name_cache;
  delete _r_val_tbl;
  delete _c_val_tbl;
  delete _cc_val_tbl;
  delete _cap_node_tbl;
  delete _r_seg_tbl;
  delete _cc_seg_tbl;
  delete _extControl;
  delete _net_bterm_itr;
  delete _net_iterm_itr;
  delete _inst_iterm_itr;
  delete _box_itr;
  delete _swire_itr;
  delete _sbox_itr;
  delete _cap_node_itr;
  delete _r_seg_itr;
  delete _cc_seg_itr;
  delete _region_inst_itr;
  delete _module_inst_itr;
  delete _module_modinst_itr;
  delete _module_modinstmoditerm_itr;
  delete _module_modbterm_itr;
  delete _module_modnet_itr;
  delete _module_modnet_modbterm_itr;
  delete _module_modnet_moditerm_itr;
  delete _module_modnet_iterm_itr;
  delete _module_modnet_bterm_itr;
  delete _region_group_itr;
  delete _group_itr;
  delete _guide_itr;
  delete _net_track_itr;
  delete _group_inst_itr;
  delete _group_modinst_itr;
  delete _group_power_net_itr;
  delete _group_ground_net_itr;
  delete _bpin_itr;
  delete _prop_itr;
  delete _dft_tbl;
  delete _marker_categories_tbl;

  while (!_callbacks.empty()) {
    auto _cbitr = _callbacks.begin();
    (*_cbitr)->removeOwner();
  }
  delete _journal;
  delete _journal_pending;
}

void dbBlock::clear()
{
  _dbBlock* block = (_dbBlock*) this;
  _dbDatabase* db = block->getDatabase();
  _dbBlock* parent = (_dbBlock*) getParent();
  _dbChip* chip = (_dbChip*) getChip();
  _dbTech* tech = (_dbTech*) getTech();

  // save a copy of the name
  char* name = strdup(block->_name);
  ZALLOCATED(name);

  // save a copy of the delimiter
  char delimiter = block->_hier_delimiter;

  std::list<dbBlockCallBackObj*> callbacks;

  // save callbacks
  callbacks.swap(block->_callbacks);

  // unlink the child from the parent
  if (parent) {
    unlink_child_from_parent(block, parent);
  }

  // destroy the block contents
  block->~_dbBlock();

  // call in-place new to create new block
  new (block) _dbBlock(db);

  // initialize the
  block->initialize(chip, tech, parent, name, delimiter);

  // restore callbacks
  block->_callbacks.swap(callbacks);

  free((void*) name);

  if (block->_journal) {
    delete block->_journal;
    block->_journal = nullptr;
  }

  if (block->_journal_pending) {
    delete block->_journal_pending;
    block->_journal_pending = nullptr;
  }
}

void _dbBlock::initialize(_dbChip* chip,
                          _dbTech* tech,
                          _dbBlock* parent,
                          const char* name,
                          char delimiter)
{
  _name = strdup(name);
  ZALLOCATED(name);

  _dbBox* box = _box_tbl->create();
  box->_flags._owner_type = dbBoxOwner::BLOCK;
  box->_owner = getOID();
  box->_shape._rect.reset(INT_MAX, INT_MAX, INT_MIN, INT_MIN);
  _bbox = box->getOID();
  _chip = chip->getOID();
  _tech = tech->getOID();
  _hier_delimiter = delimiter;
  // create top module
  _dbModule* _top = (_dbModule*) dbModule::create((dbBlock*) this, name);
  _top_module = _top->getOID();
  if (parent) {
    _def_units = parent->_def_units;
    _dbu_per_micron = parent->_dbu_per_micron;
    _parent = parent->getOID();
    parent->_children.push_back(getOID());
    _num_ext_corners = parent->_num_ext_corners;
    _corners_per_block = parent->_corners_per_block;
  }
}

dbObjectTable* _dbBlock::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbInstHdrObj:
      return _inst_hdr_tbl;

    case dbInstObj:
      return _inst_tbl;

    case dbModuleObj:
      return _module_tbl;

    case dbModInstObj:
      return _modinst_tbl;

    case dbPowerDomainObj:
      return _powerdomain_tbl;

    case dbLogicPortObj:
      return _logicport_tbl;

    case dbPowerSwitchObj:
      return _powerswitch_tbl;

    case dbIsolationObj:
      return _isolation_tbl;

    case dbLevelShifterObj:
      return _levelshifter_tbl;

    case dbGroupObj:
      return _group_tbl;

    case dbAccessPointObj:
      return ap_tbl_;

    case dbGlobalConnectObj:
      return global_connect_tbl_;

    case dbGuideObj:
      return _guide_tbl;

    case dbNetTrackObj:
      return _net_tracks_tbl;

    case dbNetObj:
      return _net_tbl;

    case dbBTermObj:
      return _bterm_tbl;

    case dbITermObj:
      return _iterm_tbl;

    case dbBoxObj:
      return _box_tbl;

    case dbViaObj:
      return _via_tbl;

    case dbGCellGridObj:
      return _gcell_grid_tbl;

    case dbTrackGridObj:
      return _track_grid_tbl;

    case dbObstructionObj:
      return _obstruction_tbl;

    case dbBlockageObj:
      return _blockage_tbl;

    case dbWireObj:
      return _wire_tbl;

    case dbSWireObj:
      return _swire_tbl;

    case dbSBoxObj:
      return _sbox_tbl;

    case dbCapNodeObj:
      return _cap_node_tbl;

    case dbRSegObj:
      return _r_seg_tbl;

    case dbCCSegObj:
      return _cc_seg_tbl;

    case dbRowObj:
      return _row_tbl;

    case dbFillObj:
      return _fill_tbl;

    case dbRegionObj:
      return _region_tbl;

    case dbHierObj:
      return _hier_tbl;

    case dbBPinObj:
      return _bpin_tbl;

    case dbTechNonDefaultRuleObj:
      return _non_default_rule_tbl;

    case dbTechLayerRuleObj:
      return _layer_rule_tbl;

    case dbPropertyObj:
      return _prop_tbl;

    case dbDftObj:
      return _dft_tbl;

    case dbMarkerCategoryObj:
      return _marker_categories_tbl;

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
  for (cbitr = block._callbacks.begin(); cbitr != block._callbacks.end();
       ++cbitr) {
    (**cbitr)().inDbBlockStreamOutBefore(
        (dbBlock*) &block);  // client ECO initialization  - payam
  }
  _dbDatabase* db = block.getImpl()->getDatabase();
  dbOStreamScope scope(stream, "dbBlock");
  stream << block._def_units;
  stream << block._dbu_per_micron;
  stream << block._hier_delimiter;
  stream << block._left_bus_delimiter;
  stream << block._right_bus_delimiter;
  stream << block._num_ext_corners;
  stream << block._corners_per_block;
  stream << block._corner_name_list;
  stream << block._name;
  stream << block._die_area;
  if (db->isSchema(db_schema_dbblock_blocked_regions_for_pins)) {
    stream << block._blocked_regions_for_pins;
  }
  stream << block._tech;
  stream << block._chip;
  stream << block._bbox;
  stream << block._parent;
  stream << block._next_block;
  stream << block._gcell_grid;
  stream << block._parent_block;
  stream << block._parent_inst;
  stream << block._top_module;
  stream << block._net_hash;
  stream << block._inst_hash;
  stream << block._module_hash;
  stream << block._modinst_hash;

  stream << block._powerdomain_hash;
  stream << block._logicport_hash;
  stream << block._powerswitch_hash;
  stream << block._isolation_hash;
  stream << block._levelshifter_hash;
  stream << block._group_hash;
  stream << block._inst_hdr_hash;
  stream << block._bterm_hash;
  stream << block._maxCapNodeId;
  stream << block._maxRSegId;
  stream << block._maxCCSegId;
  stream << block._children;
  stream << block._component_mask_shift;
  stream << block._currentCcAdjOrder;

  stream << *block._bterm_tbl;
  stream << *block._iterm_tbl;
  stream << *block._net_tbl;
  stream << *block._inst_hdr_tbl;
  if (db->isSchema(db_schema_db_remove_hash)) {
    stream << *block._module_tbl;
    stream << *block._inst_tbl;
  } else {
    stream << *block._inst_tbl;
    stream << *block._module_tbl;
  }
  stream << *block._modinst_tbl;
  if (db->isSchema(db_schema_update_hierarchy)) {
    stream << *block._modbterm_tbl;
    if (db->isSchema(db_schema_db_remove_hash)) {
      stream << *block._busport_tbl;
    }
    stream << *block._moditerm_tbl;
    stream << *block._modnet_tbl;
  }
  stream << *block._powerdomain_tbl;
  stream << *block._logicport_tbl;
  stream << *block._powerswitch_tbl;
  stream << *block._isolation_tbl;
  stream << *block._levelshifter_tbl;
  stream << *block._group_tbl;
  stream << *block.ap_tbl_;
  stream << *block.global_connect_tbl_;
  stream << *block._guide_tbl;
  stream << *block._net_tracks_tbl;
  stream << *block._box_tbl;
  stream << *block._via_tbl;
  stream << *block._gcell_grid_tbl;
  stream << *block._track_grid_tbl;
  stream << *block._obstruction_tbl;
  stream << *block._blockage_tbl;
  stream << *block._wire_tbl;
  stream << *block._swire_tbl;
  stream << *block._sbox_tbl;
  stream << *block._row_tbl;
  stream << *block._fill_tbl;
  stream << *block._region_tbl;
  stream << *block._hier_tbl;
  stream << *block._bpin_tbl;
  stream << *block._non_default_rule_tbl;
  stream << *block._layer_rule_tbl;
  stream << *block._prop_tbl;

  stream << *block._name_cache;
  stream << *block._r_val_tbl;
  stream << *block._c_val_tbl;
  stream << *block._cc_val_tbl;
  stream << NamedTable("cap_node_tbl", block._cap_node_tbl);
  stream << NamedTable("r_seg_tbl", block._r_seg_tbl);
  stream << NamedTable("cc_seg_tbl", block._cc_seg_tbl);
  stream << *block._extControl;
  stream << block._dft;
  stream << *block._dft_tbl;
  stream << *block._marker_categories_tbl;
  stream << block._marker_category_hash;
  if (block.getDatabase()->isSchema(db_schema_dbblock_layers_ranges)) {
    stream << block._min_routing_layer;
    stream << block._max_routing_layer;
    stream << block._min_layer_for_clock;
    stream << block._max_layer_for_clock;
  }
  if (db->isSchema(db_schema_block_pin_groups)) {
    stream << block._bterm_groups;
  }
  if (db->isSchema(db_schema_bterm_top_layer_grid)) {
    stream << block._bterm_top_layer_grid;
  }

  //---------------------------------------------------------- stream out
  // properties
  // TOM
  dbObjectTable* table = block.getTable();
  dbId<_dbProperty> propList = table->getPropList(block.getOID());

  stream << propList;
  // TOM
  //----------------------------------------------------------

  for (cbitr = block._callbacks.begin(); cbitr != block._callbacks.end();
       ++cbitr) {
    (*cbitr)->inDbBlockStreamOutAfter((dbBlock*) &block);
  }
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbBlock& block)
{
  _dbDatabase* db = block.getImpl()->getDatabase();

  stream >> block._def_units;
  stream >> block._dbu_per_micron;
  stream >> block._hier_delimiter;
  stream >> block._left_bus_delimiter;
  stream >> block._right_bus_delimiter;
  stream >> block._num_ext_corners;
  stream >> block._corners_per_block;
  stream >> block._corner_name_list;
  stream >> block._name;
  if (db->isSchema(db_schema_die_area_is_polygon)) {
    stream >> block._die_area;
  } else {
    Rect rect;
    stream >> rect;
    block._die_area = rect;
  }
  if (db->isSchema(db_schema_dbblock_blocked_regions_for_pins)) {
    stream >> block._blocked_regions_for_pins;
  }
  // In the older schema we can't set the tech here, we handle this later in
  // dbDatabase.
  if (db->isSchema(db_schema_block_tech)) {
    stream >> block._tech;
  }
  stream >> block._chip;
  stream >> block._bbox;
  stream >> block._parent;
  stream >> block._next_block;
  stream >> block._gcell_grid;
  stream >> block._parent_block;
  stream >> block._parent_inst;
  stream >> block._top_module;
  stream >> block._net_hash;
  stream >> block._inst_hash;
  stream >> block._module_hash;
  stream >> block._modinst_hash;
  if (db->isSchema(db_schema_update_hierarchy)) {
    if (!db->isSchema(db_schema_db_remove_hash)) {
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
  stream >> block._powerdomain_hash;
  stream >> block._logicport_hash;
  stream >> block._powerswitch_hash;
  stream >> block._isolation_hash;
  if (db->isSchema(db_schema_level_shifter)) {
    stream >> block._levelshifter_hash;
  }
  stream >> block._group_hash;
  stream >> block._inst_hdr_hash;
  stream >> block._bterm_hash;
  stream >> block._maxCapNodeId;
  stream >> block._maxRSegId;
  stream >> block._maxCCSegId;
  if (!db->isSchema(db_schema_block_ext_model_index)) {
    int ignore_minExtModelIndex;
    int ignore_maxExtModelIndex;
    stream >> ignore_minExtModelIndex;
    stream >> ignore_maxExtModelIndex;
  }
  stream >> block._children;
  if (db->isSchema(db_schema_block_component_mask_shift)) {
    stream >> block._component_mask_shift;
  }
  stream >> block._currentCcAdjOrder;
  stream >> *block._bterm_tbl;
  stream >> *block._iterm_tbl;
  stream >> *block._net_tbl;
  stream >> *block._inst_hdr_tbl;
  if (db->isSchema(db_schema_db_remove_hash)) {
    stream >> *block._module_tbl;
    stream >> *block._inst_tbl;
  } else {
    stream >> *block._inst_tbl;
    stream >> *block._module_tbl;
  }
  stream >> *block._modinst_tbl;
  if (db->isSchema(db_schema_update_hierarchy)) {
    stream >> *block._modbterm_tbl;
    if (db->isSchema(db_schema_db_remove_hash)) {
      stream >> *block._busport_tbl;
    }
    stream >> *block._moditerm_tbl;
    stream >> *block._modnet_tbl;
  }
  stream >> *block._powerdomain_tbl;
  stream >> *block._logicport_tbl;
  stream >> *block._powerswitch_tbl;
  stream >> *block._isolation_tbl;
  if (db->isSchema(db_schema_level_shifter)) {
    stream >> *block._levelshifter_tbl;
  }
  stream >> *block._group_tbl;
  stream >> *block.ap_tbl_;
  if (db->isSchema(db_schema_add_global_connect)) {
    stream >> *block.global_connect_tbl_;
  }
  stream >> *block._guide_tbl;
  if (db->isSchema(db_schema_net_tracks)) {
    stream >> *block._net_tracks_tbl;
  }
  stream >> *block._box_tbl;
  stream >> *block._via_tbl;
  stream >> *block._gcell_grid_tbl;
  stream >> *block._track_grid_tbl;
  stream >> *block._obstruction_tbl;
  stream >> *block._blockage_tbl;
  stream >> *block._wire_tbl;
  stream >> *block._swire_tbl;
  stream >> *block._sbox_tbl;
  stream >> *block._row_tbl;
  stream >> *block._fill_tbl;
  stream >> *block._region_tbl;
  stream >> *block._hier_tbl;
  stream >> *block._bpin_tbl;
  stream >> *block._non_default_rule_tbl;
  stream >> *block._layer_rule_tbl;
  stream >> *block._prop_tbl;
  stream >> *block._name_cache;
  stream >> *block._r_val_tbl;
  stream >> *block._c_val_tbl;
  stream >> *block._cc_val_tbl;
  stream >> *block._cap_node_tbl;  // DKF
  stream >> *block._r_seg_tbl;     // DKF
  stream >> *block._cc_seg_tbl;
  stream >> *block._extControl;
  if (db->isSchema(db_schema_add_scan)) {
    stream >> block._dft;
    stream >> *block._dft_tbl;
  }
  if (db->isSchema(db_schema_dbmarkergroup)) {
    stream >> *block._marker_categories_tbl;
    stream >> block._marker_category_hash;
  }
  if (db->isSchema(db_schema_dbblock_layers_ranges)) {
    stream >> block._min_routing_layer;
    stream >> block._max_routing_layer;
    stream >> block._min_layer_for_clock;
    stream >> block._max_layer_for_clock;
  }
  if (db->isSchema(db_schema_block_pin_groups)) {
    stream >> block._bterm_groups;
  }
  if (db->isSchema(db_schema_bterm_top_layer_grid)) {
    stream >> block._bterm_top_layer_grid;
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

  return stream;
}

void _dbBlock::clearSystemBlockagesAndObstructions()
{
  dbSet<dbBlockage> blockages(this, _blockage_tbl);
  dbSet<dbObstruction> obstructions(this, _obstruction_tbl);

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
  _dbBox* box = _box_tbl->getPtr(_bbox);

  if (_flags._valid_bbox) {
    box->_shape._rect.merge(rect);
  }
}
void _dbBlock::add_oct(const Oct& oct)
{
  _dbBox* box = _box_tbl->getPtr(_bbox);

  if (_flags._valid_bbox) {
    box->_shape._rect.merge(oct);
  }
}

void _dbBlock::remove_rect(const Rect& rect)
{
  _dbBox* box = _box_tbl->getPtr(_bbox);

  if (_flags._valid_bbox) {
    _flags._valid_bbox = box->_shape._rect.inside(rect);
  }
}

bool _dbBlock::operator==(const _dbBlock& rhs) const
{
  if (_flags._valid_bbox != rhs._flags._valid_bbox) {
    return false;
  }

  if (_def_units != rhs._def_units) {
    return false;
  }

  if (_dbu_per_micron != rhs._dbu_per_micron) {
    return false;
  }

  if (_hier_delimiter != rhs._hier_delimiter) {
    return false;
  }

  if (_left_bus_delimiter != rhs._left_bus_delimiter) {
    return false;
  }

  if (_right_bus_delimiter != rhs._right_bus_delimiter) {
    return false;
  }

  if (_num_ext_corners != rhs._num_ext_corners) {
    return false;
  }

  if (_corners_per_block != rhs._corners_per_block) {
    return false;
  }
  if (_corner_name_list && rhs._corner_name_list) {
    if (strcmp(_corner_name_list, rhs._corner_name_list) != 0) {
      return false;
    }
  } else if (_corner_name_list || rhs._corner_name_list) {
    return false;
  }

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0) {
      return false;
    }
  } else if (_name || rhs._name) {
    return false;
  }

  if (_die_area != rhs._die_area) {
    return false;
  }

  if (_tech != rhs._tech) {
    return false;
  }

  if (_chip != rhs._chip) {
    return false;
  }

  if (_bbox != rhs._bbox) {
    return false;
  }

  if (_parent != rhs._parent) {
    return false;
  }

  if (_next_block != rhs._next_block) {
    return false;
  }

  if (_gcell_grid != rhs._gcell_grid) {
    return false;
  }

  if (_parent_block != rhs._parent_block) {
    return false;
  }

  if (_parent_inst != rhs._parent_inst) {
    return false;
  }

  if (_top_module != rhs._top_module) {
    return false;
  }

  if (_net_hash != rhs._net_hash) {
    return false;
  }

  if (_inst_hash != rhs._inst_hash) {
    return false;
  }

  if (_module_hash != rhs._module_hash) {
    return false;
  }

  if (_modinst_hash != rhs._modinst_hash) {
    return false;
  }

  if (_powerdomain_hash != rhs._powerdomain_hash) {
    return false;
  }

  if (_logicport_hash != rhs._logicport_hash) {
    return false;
  }

  if (_powerswitch_hash != rhs._powerswitch_hash) {
    return false;
  }

  if (_isolation_hash != rhs._isolation_hash) {
    return false;
  }

  if (_levelshifter_hash != rhs._levelshifter_hash) {
    return false;
  }

  if (_group_hash != rhs._group_hash) {
    return false;
  }

  if (_inst_hdr_hash != rhs._inst_hdr_hash) {
    return false;
  }

  if (_bterm_hash != rhs._bterm_hash) {
    return false;
  }

  if (_maxCapNodeId != rhs._maxCapNodeId) {
    return false;
  }

  if (_maxRSegId != rhs._maxRSegId) {
    return false;
  }

  if (_maxCCSegId != rhs._maxCCSegId) {
    return false;
  }

  if (_children != rhs._children) {
    return false;
  }

  if (_component_mask_shift != rhs._component_mask_shift) {
    return false;
  }

  if (_currentCcAdjOrder != rhs._currentCcAdjOrder) {
    return false;
  }

  if (*_bterm_tbl != *rhs._bterm_tbl) {
    return false;
  }

  if (*_iterm_tbl != *rhs._iterm_tbl) {
    return false;
  }

  if (*_net_tbl != *rhs._net_tbl) {
    return false;
  }

  if (*_inst_hdr_tbl != *rhs._inst_hdr_tbl) {
    return false;
  }

  if (*_inst_tbl != *rhs._inst_tbl) {
    return false;
  }

  if (*_module_tbl != *rhs._module_tbl) {
    return false;
  }

  if (*_modinst_tbl != *rhs._modinst_tbl) {
    return false;
  }

  if (*_powerdomain_tbl != *rhs._powerdomain_tbl) {
    return false;
  }

  if (*_logicport_tbl != *rhs._logicport_tbl) {
    return false;
  }

  if (*_powerswitch_tbl != *rhs._powerswitch_tbl) {
    return false;
  }

  if (*_isolation_tbl != *rhs._isolation_tbl) {
    return false;
  }

  if (*_levelshifter_tbl != *rhs._levelshifter_tbl) {
    {
      return false;
    }
  }

  if (*_group_tbl != *rhs._group_tbl) {
    return false;
  }

  if (*ap_tbl_ != *rhs.ap_tbl_) {
    return false;
  }

  if (*_guide_tbl != *rhs._guide_tbl) {
    return false;
  }

  if (*_net_tracks_tbl != *rhs._net_tracks_tbl) {
    return false;
  }

  if (*_box_tbl != *rhs._box_tbl) {
    return false;
  }

  if (*_via_tbl != *rhs._via_tbl) {
    return false;
  }

  if (*_gcell_grid_tbl != *rhs._gcell_grid_tbl) {
    return false;
  }

  if (*_track_grid_tbl != *rhs._track_grid_tbl) {
    return false;
  }

  if (*_obstruction_tbl != *rhs._obstruction_tbl) {
    return false;
  }

  if (*_blockage_tbl != *rhs._blockage_tbl) {
    return false;
  }

  if (*_wire_tbl != *rhs._wire_tbl) {
    return false;
  }

  if (*_swire_tbl != *rhs._swire_tbl) {
    return false;
  }

  if (*_sbox_tbl != *rhs._sbox_tbl) {
    return false;
  }

  if (*_row_tbl != *rhs._row_tbl) {
    return false;
  }

  if (*_fill_tbl != *rhs._fill_tbl) {
    return false;
  }

  if (*_region_tbl != *rhs._region_tbl) {
    return false;
  }

  if (*_hier_tbl != *rhs._hier_tbl) {
    return false;
  }

  if (*_bpin_tbl != *rhs._bpin_tbl) {
    return false;
  }

  if (*_non_default_rule_tbl != *rhs._non_default_rule_tbl) {
    return false;
  }

  if (*_layer_rule_tbl != *rhs._layer_rule_tbl) {
    return false;
  }

  if (*_prop_tbl != *rhs._prop_tbl) {
    return false;
  }

  if (*_name_cache != *rhs._name_cache) {
    return false;
  }

  if (*_r_val_tbl != *rhs._r_val_tbl) {
    return false;
  }

  if (*_c_val_tbl != *rhs._c_val_tbl) {
    return false;
  }

  if (*_cc_val_tbl != *rhs._cc_val_tbl) {
    return false;
  }

  if (*_cap_node_tbl != *rhs._cap_node_tbl) {
    return false;
  }

  if (*_r_seg_tbl != *rhs._r_seg_tbl) {
    return false;
  }

  if (*_cc_seg_tbl != *rhs._cc_seg_tbl) {
    return false;
  }

  if (_dft != rhs._dft) {
    return false;
  }

  if (*_dft_tbl != *rhs._dft_tbl) {
    return false;
  }

  if (*_marker_categories_tbl != *rhs._marker_categories_tbl) {
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
  return block->_name;
}

const char* dbBlock::getConstName()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_name;
}

dbBox* dbBlock::getBBox()
{
  _dbBlock* block = (_dbBlock*) this;

  if (block->_flags._valid_bbox == 0) {
    ComputeBBox();
  }

  _dbBox* bbox = block->_box_tbl->getPtr(block->_bbox);
  return (dbBox*) bbox;
}

void dbBlock::ComputeBBox()
{
  _dbBlock* block = (_dbBlock*) this;
  _dbBox* bbox = block->_box_tbl->getPtr(block->_bbox);
  bbox->_shape._rect.reset(INT_MAX, INT_MAX, INT_MIN, INT_MIN);

  for (dbInst* inst : getInsts()) {
    if (inst->isPlaced()) {
      _dbBox* box = (_dbBox*) inst->getBBox();
      bbox->_shape._rect.merge(box->_shape._rect);
    }
  }

  for (dbBTerm* bterm : getBTerms()) {
    for (dbBPin* bp : bterm->getBPins()) {
      if (bp->getPlacementStatus().isPlaced()) {
        for (dbBox* box : bp->getBoxes()) {
          Rect r = box->getBox();
          bbox->_shape._rect.merge(r);
        }
      }
    }
  }

  for (dbObstruction* obs : getObstructions()) {
    _dbBox* box = (_dbBox*) obs->getBBox();
    bbox->_shape._rect.merge(box->_shape._rect);
  }

  for (dbSBox* box : dbSet<dbSBox>(block, block->_sbox_tbl)) {
    Rect rect = box->getBox();
    bbox->_shape._rect.merge(rect);
  }

  for (dbWire* wire : dbSet<dbWire>(block, block->_wire_tbl)) {
    const auto opt_bbox = wire->getBBox();
    if (opt_bbox) {
      bbox->_shape._rect.merge(opt_bbox.value());
    }
  }

  if (bbox->_shape._rect.xMin() == INT_MAX) {  // empty block
    bbox->_shape._rect.reset(0, 0, 0, 0);
  }

  block->_flags._valid_bbox = 1;
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
  _dbChip* chip = db->_chip_tbl->getPtr(block->_chip);
  return (dbChip*) chip;
}

dbBlock* dbBlock::getParent()
{
  _dbBlock* block = (_dbBlock*) this;
  _dbDatabase* db = block->getDatabase();
  _dbChip* chip = db->_chip_tbl->getPtr(block->_chip);
  if (!block->_parent.isValid()) {
    return nullptr;
  }
  _dbBlock* parent = chip->_block_tbl->getPtr(block->_parent);
  return (dbBlock*) parent;
}

dbInst* dbBlock::getParentInst()
{
  _dbBlock* block = (_dbBlock*) this;

  if (block->_parent_block == 0) {
    return nullptr;
  }

  _dbChip* chip = (_dbChip*) block->getOwner();
  _dbBlock* parent_block = chip->_block_tbl->getPtr(block->_parent_block);
  _dbInst* parent_inst = parent_block->_inst_tbl->getPtr(block->_parent_inst);
  return (dbInst*) parent_inst;
}

dbModule* dbBlock::getTopModule()
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbModule*) block->_module_tbl->getPtr(block->_top_module);
}

dbSet<dbBlock> dbBlock::getChildren()
{
  _dbBlock* block = (_dbBlock*) this;
  _dbDatabase* db = getImpl()->getDatabase();
  _dbChip* chip = db->_chip_tbl->getPtr(block->_chip);
  return dbSet<dbBlock>(block, chip->_block_itr);
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
  return dbSet<dbBTerm>(block, block->_bterm_tbl);
}

dbBTerm* dbBlock::findBTerm(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbBTerm*) block->_bterm_hash.find(name);
}

std::vector<dbBlock::dbBTermGroup> dbBlock::getBTermGroups()
{
  _dbBlock* block = (_dbBlock*) this;
  std::vector<dbBlock::dbBTermGroup> groups;
  for (const _dbBTermGroup& group : block->_bterm_groups) {
    dbBlock::dbBTermGroup bterm_group;
    for (const auto& bterm_id : group.bterms) {
      bterm_group.bterms.push_back(
          (dbBTerm*) block->_bterm_tbl->getPtr(bterm_id));
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
  block->_bterm_groups.push_back(std::move(group));
}

void dbBlock::setBTermTopLayerGrid(
    const dbBlock::dbBTermTopLayerGrid& top_layer_grid)
{
  _dbBlock* block = (_dbBlock*) this;
  _dbBTermTopLayerGrid& top_grid = block->_bterm_top_layer_grid;

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
  if (block->_bterm_top_layer_grid.region.getPoints().empty()) {
    return std::nullopt;
  }

  top_layer_grid.layer = odb::dbTechLayer::getTechLayer(
      tech, block->_bterm_top_layer_grid.layer);
  top_layer_grid.x_step = block->_bterm_top_layer_grid.x_step;
  top_layer_grid.y_step = block->_bterm_top_layer_grid.y_step;
  top_layer_grid.region = block->_bterm_top_layer_grid.region;
  top_layer_grid.pin_width = block->_bterm_top_layer_grid.pin_width;
  top_layer_grid.pin_height = block->_bterm_top_layer_grid.pin_height;
  top_layer_grid.keepout = block->_bterm_top_layer_grid.keepout;

  return top_layer_grid;
}

Polygon dbBlock::getBTermTopLayerGridRegion()
{
  _dbBlock* block = (_dbBlock*) this;

  const Polygon& region = block->_bterm_top_layer_grid.region;
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
  return dbSet<dbITerm>(block, block->_iterm_tbl);
}

dbSet<dbInst> dbBlock::getInsts()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbInst>(block, block->_inst_tbl);
}

dbSet<dbModule> dbBlock::getModules()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbModule>(block, block->_module_tbl);
}

dbSet<dbModInst> dbBlock::getModInsts()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbModInst>(block, block->_modinst_tbl);
}

dbSet<dbModBTerm> dbBlock::getModBTerms()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbModBTerm>(block, block->_modbterm_tbl);
}

dbSet<dbModITerm> dbBlock::getModITerms()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbModITerm>(block, block->_moditerm_tbl);
}

dbSet<dbModNet> dbBlock::getModNets()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbModNet>(block, block->_modnet_tbl);
}

dbSet<dbPowerDomain> dbBlock::getPowerDomains()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbPowerDomain>(block, block->_powerdomain_tbl);
}

dbSet<dbLogicPort> dbBlock::getLogicPorts()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbLogicPort>(block, block->_logicport_tbl);
}

dbSet<dbPowerSwitch> dbBlock::getPowerSwitches()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbPowerSwitch>(block, block->_powerswitch_tbl);
}

dbSet<dbIsolation> dbBlock::getIsolations()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbIsolation>(block, block->_isolation_tbl);
}

dbSet<dbLevelShifter> dbBlock::getLevelShifters()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbLevelShifter>(block, block->_levelshifter_tbl);
}

dbSet<dbGroup> dbBlock::getGroups()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbGroup>(block, block->_group_tbl);
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
  for (const auto& layer_id : block->_component_mask_shift) {
    layers.push_back((dbTechLayer*) tech->_layer_tbl->getPtr(layer_id));
  }
  return layers;
}

void dbBlock::setComponentMaskShift(const std::vector<dbTechLayer*>& layers)
{
  _dbBlock* block = (_dbBlock*) this;
  for (auto* layer : layers) {
    block->_component_mask_shift.push_back(layer->getId());
  }
}

dbInst* dbBlock::findInst(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbInst*) block->_inst_hash.find(name);
}

dbModule* dbBlock::findModule(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbModule*) block->_module_hash.find(name);
}

dbPowerDomain* dbBlock::findPowerDomain(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbPowerDomain*) block->_powerdomain_hash.find(name);
}

dbLogicPort* dbBlock::findLogicPort(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbLogicPort*) block->_logicport_hash.find(name);
}

dbPowerSwitch* dbBlock::findPowerSwitch(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbPowerSwitch*) block->_powerswitch_hash.find(name);
}

dbIsolation* dbBlock::findIsolation(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbIsolation*) block->_isolation_hash.find(name);
}

dbLevelShifter* dbBlock::findLevelShifter(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbLevelShifter*) block->_levelshifter_hash.find(name);
}

dbModInst* dbBlock::findModInst(const char* path)
{
  char* _path = strdup(path);
  dbModule* cur_mod = getTopModule();
  dbModInst* cur_inst = nullptr;
  char* token = strtok(_path, "/");
  while (token != nullptr) {
    cur_inst = cur_mod->findModInst(token);
    if (cur_inst == nullptr) {
      break;
    }
    cur_mod = cur_inst->getMaster();
    token = strtok(nullptr, "/");
  }
  free((void*) _path);
  return cur_inst;
}

dbGroup* dbBlock::findGroup(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbGroup*) block->_group_hash.find(name);
}

dbITerm* dbBlock::findITerm(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;

  std::string s(name);

  std::string::size_type idx = s.rfind(block->_hier_delimiter);

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
  return dbSet<dbObstruction>(block, block->_obstruction_tbl);
}

dbSet<dbBlockage> dbBlock::getBlockages()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbBlockage>(block, block->_blockage_tbl);
}

dbSet<dbNet> dbBlock::getNets()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbNet>(block, block->_net_tbl);
}

dbSet<dbCapNode> dbBlock::getCapNodes()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbCapNode>(block, block->_cap_node_tbl);
}

dbNet* dbBlock::findNet(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbNet*) block->_net_hash.find(name);
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
  return dbSet<dbVia>(block, block->_via_tbl);
}

dbSet<dbRow> dbBlock::getRows()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbRow>(block, block->_row_tbl);
}

dbSet<dbFill> dbBlock::getFills()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbFill>(block, block->_fill_tbl);
}

dbGCellGrid* dbBlock::getGCellGrid()
{
  _dbBlock* block = (_dbBlock*) this;

  if (block->_gcell_grid == 0) {
    return nullptr;
  }

  return (dbGCellGrid*) block->_gcell_grid_tbl->getPtr(block->_gcell_grid);
}

dbSet<dbRegion> dbBlock::getRegions()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbRegion>(block, block->_region_tbl);
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
  return block->_def_units;
}

void dbBlock::setDefUnits(int units)
{
  _dbBlock* block = (_dbBlock*) this;
  block->_def_units = units;
}

int dbBlock::getDbUnitsPerMicron()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_dbu_per_micron;
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

char dbBlock::getHierarchyDelimiter()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_hier_delimiter;
}

void dbBlock::setBusDelimiters(char left, char right)
{
  _dbBlock* block = (_dbBlock*) this;
  block->_left_bus_delimiter = left;
  block->_right_bus_delimiter = right;
}

void dbBlock::getBusDelimiters(char& left, char& right)
{
  _dbBlock* block = (_dbBlock*) this;
  left = block->_left_bus_delimiter;
  right = block->_right_bus_delimiter;
}

dbSet<dbTrackGrid> dbBlock::getTrackGrids()
{
  _dbBlock* block = (_dbBlock*) this;
  dbSet<dbTrackGrid> tracks(block, block->_track_grid_tbl);
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
  for (dbInstHdr* hdr : dbSet<dbInstHdr>(block, block->_inst_hdr_tbl)) {
    masters.push_back(hdr->getMaster());
  }
}

void dbBlock::setDieArea(const Rect& new_area)
{
  _dbBlock* block = (_dbBlock*) this;

  // Clear any existing system blockages
  block->clearSystemBlockagesAndObstructions();

  block->_die_area = new_area;
  for (auto callback : block->_callbacks) {
    callback->inDbBlockSetDieArea(this);
  }
}

void dbBlock::setDieArea(const Polygon& new_area)
{
  _dbBlock* block = (_dbBlock*) this;
  block->_die_area = new_area;
  for (auto callback : block->_callbacks) {
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

  Polygon bounding_rect = block->_die_area.getEnclosingRect();
  std::vector<Polygon> results = bounding_rect.difference(block->_die_area);
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
  return block->_die_area.getEnclosingRect();
}

Polygon dbBlock::getDieAreaPolygon()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_die_area;
}

void dbBlock::addBlockedRegionForPins(const Rect& region)
{
  _dbBlock* block = (_dbBlock*) this;
  block->_blocked_regions_for_pins.push_back(region);
}

const std::vector<Rect>& dbBlock::getBlockedRegionsForPins()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_blocked_regions_for_pins;
}

Rect dbBlock::getCoreArea()
{
  Rect rect;
  rect.mergeInit();

  for (dbRow* row : getRows()) {
    if (row->getSite()->getClass() != odb::dbSiteClass::PAD) {
      rect.merge(row->getBBox());
    }
  }

  if (!rect.isInverted()) {
    return rect;
  }

  // Default to die area if there aren't any rows.
  return getDieArea();
}

void dbBlock::setExtmi(void* ext)
{
  _dbBlock* block = (_dbBlock*) this;
  block->_extmi = ext;
}

void* dbBlock::getExtmi()
{
  _dbBlock* block = (_dbBlock*) this;
  return (block->_extmi);
}

dbExtControl* dbBlock::getExtControl()
{
  _dbBlock* block = (_dbBlock*) this;
  return (block->_extControl);
}

dbDft* dbBlock::getDft() const
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbDft*) block->_dft_tbl->getPtr(block->_dft);
}

int dbBlock::getMinRoutingLayer() const
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_min_routing_layer;
}

void dbBlock::setMinRoutingLayer(const int min_routing_layer)
{
  _dbBlock* block = (_dbBlock*) this;
  block->_min_routing_layer = min_routing_layer;
}

int dbBlock::getMaxRoutingLayer() const
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_max_routing_layer;
}

void dbBlock::setMaxRoutingLayer(const int max_routing_layer)
{
  _dbBlock* block = (_dbBlock*) this;
  block->_max_routing_layer = max_routing_layer;
}

int dbBlock::getMinLayerForClock() const
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_min_layer_for_clock;
}

void dbBlock::setMinLayerForClock(const int min_layer_for_clock)
{
  _dbBlock* block = (_dbBlock*) this;
  block->_min_layer_for_clock = min_layer_for_clock;
}

int dbBlock::getMaxLayerForClock() const
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_max_layer_for_clock;
}

void dbBlock::setMaxLayerForClock(const int max_layer_for_clock)
{
  _dbBlock* block = (_dbBlock*) this;
  block->_max_layer_for_clock = max_layer_for_clock;
}

void dbBlock::getExtCornerNames(std::list<std::string>& ecl)
{
  _dbBlock* block = (_dbBlock*) this;
  if (block->_corner_name_list) {
    ecl.push_back(block->_corner_name_list);
  } else {
    ecl.push_back("");
  }
}

dbSet<dbCCSeg> dbBlock::getCCSegs()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbCCSeg>(block, block->_cc_seg_tbl);
}

dbSet<dbRSeg> dbBlock::getRSegs()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbRSeg>(block, block->_r_seg_tbl);
}

dbTechNonDefaultRule* dbBlock::findNonDefaultRule(const char* name)
{
  for (dbTechNonDefaultRule* r : getNonDefaultRules()) {
    if (strcmp(r->getConstName(), name) == 0) {
      return (dbTechNonDefaultRule*) r;
    }
  }

  return nullptr;
}

dbSet<dbTechNonDefaultRule> dbBlock::getNonDefaultRules()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbTechNonDefaultRule>(block, block->_non_default_rule_tbl);
}

void dbBlock::copyExtDb(uint fr,
                        uint to,
                        uint extDbCnt,
                        double resFactor,
                        double ccFactor,
                        double gndcFactor)
{
  _dbBlock* block = (_dbBlock*) this;
  uint j;
  if (resFactor != 1.0) {
    for (j = 1; j < block->_r_val_tbl->size(); j += extDbCnt) {
      (*block->_r_val_tbl)[j + to] = (*block->_r_val_tbl)[j + fr] * resFactor;
    }
  } else {
    for (j = 1; j < block->_r_val_tbl->size(); j += extDbCnt) {
      (*block->_r_val_tbl)[j + to] = (*block->_r_val_tbl)[j + fr];
    }
  }
  if (ccFactor != 1.0) {
    for (j = 1; j < block->_cc_val_tbl->size(); j += extDbCnt) {
      (*block->_cc_val_tbl)[j + to] = (*block->_cc_val_tbl)[j + fr] * ccFactor;
    }
  } else {
    for (j = 1; j < block->_cc_val_tbl->size(); j += extDbCnt) {
      (*block->_cc_val_tbl)[j + to] = (*block->_cc_val_tbl)[j + fr];
    }
  }
  if (gndcFactor != 1.0) {
    for (j = 1; j < block->_c_val_tbl->size(); j += extDbCnt) {
      (*block->_c_val_tbl)[j + to] = (*block->_c_val_tbl)[j + fr] * gndcFactor;
    }
  } else {
    for (j = 1; j < block->_c_val_tbl->size(); j += extDbCnt) {
      (*block->_c_val_tbl)[j + to] = (*block->_c_val_tbl)[j + fr];
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
  const uint adjustOrder = block->_currentCcAdjOrder + 1;
  for (dbNet* net : nets) {
    adjusted |= net->adjustCC(
        adjustOrder, adjFactor, ccThreshHold, adjustedCC, halonets);
  }
  for (dbCCSeg* ccs : adjustedCC) {
    ccs->setMark(false);
  }
  if (adjusted) {
    block->_currentCcAdjOrder = adjustOrder;
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
  uint j;
  if (resFactor != 1.0) {
    for (j = 1; j < block->_r_val_tbl->size(); j++) {
      (*block->_r_val_tbl)[j] *= resFactor;
    }
  }
  if (ccFactor != 1.0) {
    for (j = 1; j < block->_cc_val_tbl->size(); j++) {
      (*block->_cc_val_tbl)[j] *= ccFactor;
    }
  }
  if (gndcFactor != 1.0) {
    for (j = 1; j < block->_c_val_tbl->size(); j++) {
      (*block->_c_val_tbl)[j] *= gndcFactor;
    }
  }
}

void dbBlock::getExtCount(int& numOfNet,
                          int& numOfRSeg,
                          int& numOfCapNode,
                          int& numOfCCSeg)
{
  _dbBlock* block = (_dbBlock*) this;
  numOfNet = block->_net_tbl->size();
  numOfRSeg = block->_r_seg_tbl->size();
  numOfCapNode = block->_cap_node_tbl->size();
  numOfCCSeg = block->_cc_seg_tbl->size();
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
  return block->_num_ext_corners;
}

int dbBlock::getCornersPerBlock()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_corners_per_block;
}

bool dbBlock::extCornersAreIndependent()
{
  bool independent = getExtControl()->_independentExtCorners;
  return independent;
}

int dbBlock::getExtDbCount()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_num_ext_dbs;
}

void dbBlock::initParasiticsValueTables()
{
  _dbBlock* block = (_dbBlock*) this;
  if ((block->_r_seg_tbl->size() > 0) || (block->_cap_node_tbl->size() > 0)
      || (block->_cc_seg_tbl->size() > 0)) {
    for (dbNet* net : getNets()) {
      _dbNet* n = (_dbNet*) net;
      n->_cap_nodes = 0;
      n->_r_segs = 0;
    }
  }

  int ttttClear = 1;
  _dbDatabase* db = block->_cap_node_tbl->_db;
  if (ttttClear) {
    block->_cap_node_tbl->clear();
  } else {
    delete block->_cap_node_tbl;
    block->_cap_node_tbl
        = new dbTable<_dbCapNode>(db,
                                  block,
                                  (GetObjTbl_t) &_dbBlock::getObjectTable,
                                  dbCapNodeObj,
                                  4096,
                                  12);
  }
  block->_maxCapNodeId = 0;

  if (ttttClear) {
    block->_r_seg_tbl->clear();
  } else {
    delete block->_r_seg_tbl;
    block->_r_seg_tbl
        = new dbTable<_dbRSeg>(db,
                               block,
                               (GetObjTbl_t) &_dbBlock::getObjectTable,
                               dbRSegObj,
                               4096,
                               12);
  }
  block->_maxRSegId = 0;

  if (ttttClear) {
    block->_cc_seg_tbl->clear();
  } else {
    delete block->_cc_seg_tbl;
    block->_cc_seg_tbl
        = new dbTable<_dbCCSeg>(db,
                                block,
                                (GetObjTbl_t) &_dbBlock::getObjectTable,
                                dbCCSegObj,
                                4096,
                                12);
  }
  block->_maxCCSegId = 0;

  if (ttttClear) {
    block->_cc_val_tbl->clear();
  } else {
    delete block->_cc_val_tbl;
    block->_cc_val_tbl = new dbPagedVector<float, 4096, 12>();
  }
  block->_cc_val_tbl->push_back(0.0);

  if (ttttClear) {
    block->_r_val_tbl->clear();
  } else {
    delete block->_r_val_tbl;
    block->_r_val_tbl = new dbPagedVector<float, 4096, 12>();
  }
  block->_r_val_tbl->push_back(0.0);

  if (ttttClear) {
    block->_c_val_tbl->clear();
  } else {
    delete block->_c_val_tbl;
    block->_c_val_tbl = new dbPagedVector<float, 4096, 12>();
  }
  block->_c_val_tbl->push_back(0.0);
}

void dbBlock::setCornersPerBlock(int cornersPerBlock)
{
  initParasiticsValueTables();
  _dbBlock* block = (_dbBlock*) this;
  block->_corners_per_block = cornersPerBlock;
}

void dbBlock::setCornerCount(int cornersStoredCnt,
                             int extDbCnt,
                             const char* name_list)
{
  ZASSERT((cornersStoredCnt > 0) && (cornersStoredCnt <= 256));
  _dbBlock* block = (_dbBlock*) this;

  // TODO: Should this change be logged in the journal?
  //       Yes !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbBlock {}, setCornerCount cornerCnt {}, extDbCnt {}, "
               "name_list {}",
               block->getId(),
               cornersStoredCnt,
               extDbCnt,
               name_list);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbBlockObj);
    block->_journal->pushParam(block->getId());
    block->_journal->pushParam(_dbBlock::CORNERCOUNT);
    block->_journal->pushParam(cornersStoredCnt);
    block->_journal->pushParam(extDbCnt);
    block->_journal->pushParam(name_list);
    block->_journal->endAction();
  }
  initParasiticsValueTables();

  block->_num_ext_corners = cornersStoredCnt;
  block->_corners_per_block = cornersStoredCnt;
  block->_num_ext_dbs = extDbCnt;
  if (name_list != nullptr) {
    if (block->_corner_name_list) {
      free(block->_corner_name_list);
    }
    block->_corner_name_list = strdup((char*) name_list);
  }
}
dbBlock* dbBlock::getExtCornerBlock(uint corner)
{
  dbBlock* block = findExtCornerBlock(corner);
  if (!block) {
    block = this;
  }
  return block;
}

dbBlock* dbBlock::findExtCornerBlock(uint corner)
{
  char cornerName[64];
  sprintf(cornerName, "extCornerBlock__%d", corner);
  return findChild(cornerName);
}

dbBlock* dbBlock::createExtCornerBlock(uint corner)
{
  char cornerName[64];
  sprintf(cornerName, "extCornerBlock__%d", corner);
  dbBlock* extBlk = dbBlock::create(this, cornerName, nullptr, '/');
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

  return block->_corner_name_list;
}
void dbBlock::setCornerNameList(const char* name_list)
{
  _dbBlock* block = (_dbBlock*) this;

  if (block->_corner_name_list != nullptr) {
    free(block->_corner_name_list);
  }

  block->_corner_name_list = strdup(name_list);
}
void dbBlock::getExtCornerName(int corner, char* cName)
{
  cName[0] = '\0';
  _dbBlock* block = (_dbBlock*) this;
  if (block->_num_ext_corners == 0) {
    return;
  }
  ZASSERT((corner >= 0) && (corner < block->_num_ext_corners));

  if (block->_corner_name_list == nullptr) {
    return;
  }

  char buff[1024];
  strcpy(buff, block->_corner_name_list);

  int ii = 0;
  char* word = strtok(buff, " ");
  while (word != nullptr) {
    if (ii == corner) {
      strcpy(cName, word);
      return;
    }

    word = strtok(nullptr, " ");
    ii++;
  }
}
int dbBlock::getExtCornerIndex(const char* cornerName)
{
  _dbBlock* block = (_dbBlock*) this;

  if (block->_corner_name_list == nullptr) {
    return -1;
  }

  char buff[1024];
  strcpy(buff, block->_corner_name_list);

  uint ii = 0;
  char* word = strtok(buff, " ");
  while (word != nullptr) {
    if (strcmp(cornerName, word) == 0) {
      return ii;
    }

    word = strtok(nullptr, " ");
    ii++;
  }
  return -1;
}
void dbBlock::setCornerCount(int cnt)
{
  setCornerCount(cnt, cnt, nullptr);
}

dbBlock* dbBlock::create(dbChip* chip_,
                         const char* name_,
                         dbTech* tech_,
                         char hier_delimiter_)
{
  _dbChip* chip = (_dbChip*) chip_;

  if (chip->_top != 0) {
    return nullptr;
  }

  if (!tech_) {
    tech_ = chip_->getDb()->getTech();
  }

  _dbBlock* top = chip->_block_tbl->create();
  _dbTech* tech = (_dbTech*) tech_;
  top->initialize(chip, tech, nullptr, name_, hier_delimiter_);
  chip->_top = top->getOID();
  top->_dbu_per_micron = tech->_dbu_per_micron;
  return (dbBlock*) top;
}

dbBlock* dbBlock::create(dbBlock* parent_,
                         const char* name_,
                         dbTech* tech_,
                         char hier_delimiter)
{
  if (parent_->findChild(name_)) {
    return nullptr;
  }

  if (!tech_) {
    tech_ = parent_->getTech();
  }

  _dbBlock* parent = (_dbBlock*) parent_;
  _dbChip* chip = (_dbChip*) parent->getOwner();
  _dbBlock* child = chip->_block_tbl->create();
  _dbTech* tech = (_dbTech*) tech_;
  child->initialize(chip, tech, parent, name_, hier_delimiter);
  child->_dbu_per_micron = tech->_dbu_per_micron;
  return (dbBlock*) child;
}

dbBlock* dbBlock::getBlock(dbChip* chip_, uint dbid_)
{
  _dbChip* chip = (_dbChip*) chip_;
  return (dbBlock*) chip->_block_tbl->getPtr(dbid_);
}

dbBlock* dbBlock::getBlock(dbBlock* block_, uint dbid_)
{
  _dbChip* chip = (_dbChip*) block_->getImpl()->getOwner();
  return (dbBlock*) chip->_block_tbl->getPtr(dbid_);
}

void dbBlock::destroy(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbChip* chip = (_dbChip*) block->getOwner();
  // delete the children of this block
  for (const dbId<_dbBlock>& child_id : block->_children) {
    _dbBlock* child = chip->_block_tbl->getPtr(child_id);
    destroy((dbBlock*) child);
  }
  // Deleting top block
  if (block->_parent == 0) {
    chip->_top = 0;
  } else {
    // unlink this block from the parent
    _dbBlock* parent = chip->_block_tbl->getPtr(block->_parent);
    unlink_child_from_parent(block, parent);
  }

  dbProperty::destroyProperties(block);
  chip->_block_tbl->destroy(block);
}

void unlink_child_from_parent(_dbBlock* child, _dbBlock* parent)
{
  uint id = child->getOID();

  auto& children = parent->_children;
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
  return block->_searchDb;
}

void dbBlock::getWireUpdatedNets(std::vector<dbNet*>& result)
{
  int tot = 0;
  int upd = 0;
  int enc = 0;
  for (dbNet* net : getNets()) {
    tot++;
    _dbNet* n = (_dbNet*) net;

    if (n->_flags._wire_altered != 1) {
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
  uint jj;
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
  uint jj;
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
    if (block->_journal) {
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
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbBlock {}, writeDb",
               block->getId());
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbBlockObj);
    block->_journal->pushParam(block->getId());
    block->_journal->pushParam(_dbBlock::WRITEDB);
    block->_journal->pushParam(filename);
    block->_journal->pushParam(allNode);
    block->_journal->endAction();
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
  std::sort(nets.begin(), nets.end(), [](odb::dbNet* net1, odb::dbNet* net2) {
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

void dbBlock::setDrivingItermsforNets()
{
  for (dbNet* net : getNets()) {
    if ((net->getSigType() == dbSigType::GROUND)
        || (net->getSigType() == dbSigType::POWER)) {
      continue;
    }

    net->setDrivingITerm(0);

    for (dbITerm* tr : net->getITerms()) {
      if (tr->getIoType() == dbIoType::OUTPUT) {
        net->setDrivingITerm(tr->getId());
        break;
      }
    }
  }
}

void dbBlock::preExttreeMergeRC(double max_cap, uint corner)
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

    odb::uint wire_cnt = 0, via_cnt = 0;
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

int dbBlock::globalConnect()
{
  dbSet<dbGlobalConnect> gcs = getGlobalConnects();
  const std::vector<dbGlobalConnect*> connects(gcs.begin(), gcs.end());
  _dbBlock* dbblock = (_dbBlock*) this;
  return dbblock->globalConnect(connects);
}

int dbBlock::globalConnect(dbGlobalConnect* gc)
{
  _dbBlock* dbblock = (_dbBlock*) this;
  return dbblock->globalConnect({gc});
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
    return globalConnect(gc);
  }
  return 0;
}

int _dbBlock::globalConnect(const std::vector<dbGlobalConnect*>& connects)
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
    insts.erase(std::remove_if(insts.begin(),
                               insts.end(),
                               [&](dbInst* inst) {
                                 return remove_insts.find(inst)
                                        != remove_insts.end();
                               }),
                insts.end());

    inst_map[inst_pattern] = insts;

    _dbGlobalConnect* connect_rule = (_dbGlobalConnect*) connect;
    for (dbInst* inst : remove_insts) {
      if (connect_rule->needsModification(inst)) {
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
    const auto connections = connect->connect(inst_map[connect->inst_pattern_]);
    connected_iterms.insert(connections.begin(), connections.end());
  }
  for (_dbGlobalConnect* connect : region_rules) {
    const auto connections = connect->connect(inst_map[connect->inst_pattern_]);
    connected_iterms.insert(connections.begin(), connections.end());
  }

  return connected_iterms.size();
}

_dbTech* _dbBlock::getTech()
{
  _dbDatabase* db = getDatabase();
  return db->_tech_tbl->getPtr(_tech);
}

dbTech* dbBlock::getTech()
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbTech*) block->getTech();
}

dbSet<dbMarkerCategory> dbBlock::getMarkerCategories()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbMarkerCategory>(block, block->_marker_categories_tbl);
}

dbMarkerCategory* dbBlock::findMarkerCategory(const char* name)
{
  _dbBlock* block = (_dbBlock*) this;
  return (dbMarkerCategory*) block->_marker_category_hash.find(name);
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
                          ((dbModule*) cur_obj)->getName());
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

  info.children_["name"].add(_name);
  info.children_["corner_name"].add(_corner_name_list);
  info.children_["blocked_regions_for_pins"].add(_blocked_regions_for_pins);

  info.children_["net_hash"].add(_net_hash);
  info.children_["inst_hash"].add(_inst_hash);
  info.children_["module_hash"].add(_module_hash);
  info.children_["modinst_hash"].add(_modinst_hash);
  info.children_["powerdomain_hash"].add(_powerdomain_hash);
  info.children_["logicport_hash"].add(_logicport_hash);
  info.children_["powerswitch_hash"].add(_powerswitch_hash);
  info.children_["isolation_hash"].add(_isolation_hash);
  info.children_["marker_category_hash"].add(_marker_category_hash);
  info.children_["levelshifter_hash"].add(_levelshifter_hash);
  info.children_["group_hash"].add(_group_hash);
  info.children_["inst_hdr_hash"].add(_inst_hdr_hash);
  info.children_["bterm_hash"].add(_bterm_hash);

  info.children_["children"].add(_children);
  info.children_["component_mask_shift"].add(_component_mask_shift);

  _bterm_tbl->collectMemInfo(info.children_["bterm"]);
  _iterm_tbl->collectMemInfo(info.children_["iterm"]);
  _net_tbl->collectMemInfo(info.children_["net"]);
  _inst_hdr_tbl->collectMemInfo(info.children_["inst_hdr"]);
  _inst_tbl->collectMemInfo(info.children_["inst"]);
  _box_tbl->collectMemInfo(info.children_["box"]);
  _via_tbl->collectMemInfo(info.children_["via"]);
  _gcell_grid_tbl->collectMemInfo(info.children_["gcell_grid"]);
  _track_grid_tbl->collectMemInfo(info.children_["track_grid"]);
  _obstruction_tbl->collectMemInfo(info.children_["obstruction"]);
  _blockage_tbl->collectMemInfo(info.children_["blockage"]);
  _wire_tbl->collectMemInfo(info.children_["wire"]);
  _swire_tbl->collectMemInfo(info.children_["swire"]);
  _sbox_tbl->collectMemInfo(info.children_["sbox"]);
  _row_tbl->collectMemInfo(info.children_["row"]);
  _fill_tbl->collectMemInfo(info.children_["fill"]);
  _region_tbl->collectMemInfo(info.children_["region"]);
  _hier_tbl->collectMemInfo(info.children_["hier"]);
  _bpin_tbl->collectMemInfo(info.children_["bpin"]);
  _non_default_rule_tbl->collectMemInfo(info.children_["non_default_rule"]);
  _layer_rule_tbl->collectMemInfo(info.children_["layer_rule"]);
  _prop_tbl->collectMemInfo(info.children_["prop"]);
  _module_tbl->collectMemInfo(info.children_["module"]);
  _powerdomain_tbl->collectMemInfo(info.children_["powerdomain"]);
  _logicport_tbl->collectMemInfo(info.children_["logicport"]);
  _powerswitch_tbl->collectMemInfo(info.children_["powerswitch"]);
  _isolation_tbl->collectMemInfo(info.children_["isolation"]);
  _levelshifter_tbl->collectMemInfo(info.children_["levelshifter"]);
  _modinst_tbl->collectMemInfo(info.children_["modinst"]);
  _group_tbl->collectMemInfo(info.children_["group"]);
  ap_tbl_->collectMemInfo(info.children_["ap"]);
  global_connect_tbl_->collectMemInfo(info.children_["global_connect"]);
  _guide_tbl->collectMemInfo(info.children_["guide"]);
  _net_tracks_tbl->collectMemInfo(info.children_["net_tracks"]);
  _dft_tbl->collectMemInfo(info.children_["dft"]);
  _marker_categories_tbl->collectMemInfo(info.children_["marker_categories"]);
  _modbterm_tbl->collectMemInfo(info.children_["modbterm"]);
  _moditerm_tbl->collectMemInfo(info.children_["moditerm"]);
  _modnet_tbl->collectMemInfo(info.children_["modnet"]);
  _busport_tbl->collectMemInfo(info.children_["busport"]);
  _cap_node_tbl->collectMemInfo(info.children_["cap_node"]);
  _r_seg_tbl->collectMemInfo(info.children_["r_seg"]);
  _cc_seg_tbl->collectMemInfo(info.children_["cc_seg"]);

  _name_cache->collectMemInfo(info.children_["name_cache"]);
  info.children_["r_val"].add(*_r_val_tbl);
  info.children_["c_val"].add(*_c_val_tbl);
  info.children_["cc_val"].add(*_cc_val_tbl);

  info.children_["module_name_id_map"].add(_module_name_id_map);
}

}  // namespace odb
