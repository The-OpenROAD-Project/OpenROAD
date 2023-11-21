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

#include "dbBlock.h"

#include <errno.h>
#include <unistd.h>

#include <fstream>
#include <memory>
#include <set>
#include <string>

#include "db.h"
#include "dbAccessPoint.h"
#include "dbArrayTable.h"
#include "dbArrayTable.hpp"
#include "dbBPin.h"
#include "dbBPinItr.h"
#include "dbBTerm.h"
#include "dbBTermItr.h"
#include "dbBlockCallBackObj.h"
#include "dbBlockItr.h"
#include "dbBlockage.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbCCSeg.h"
#include "dbCCSegItr.h"
#include "dbCapNode.h"
#include "dbCapNodeItr.h"
#include "dbChip.h"
#include "dbDatabase.h"
#include "dbDiff.h"
#include "dbDiff.hpp"
#include "dbExtControl.h"
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
#include "dbLogicPort.h"
#include "dbModInst.h"
#include "dbModule.h"
#include "dbModuleInstItr.h"
#include "dbModuleModInstItr.h"
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
#include "dbShape.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechLayerRule.h"
#include "dbTechNonDefaultRule.h"
#include "dbTrackGrid.h"
#include "dbVia.h"
#include "dbWire.h"
#include "defout.h"
#include "lefout.h"
#include "parse.h"
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

_dbBlock::_dbBlock(_dbDatabase* db)
{
  _flags._valid_bbox = 0;
  _flags._spare_bits = 0;
  _def_units = 100;
  _dbu_per_micron = 1000;
  _hier_delimeter = 0;
  _left_bus_delimeter = 0;
  _right_bus_delimeter = 0;
  _num_ext_corners = 0;
  _corners_per_block = 0;
  _corner_name_list = 0;
  _name = 0;
  _maxCapNodeId = 0;
  _maxRSegId = 0;
  _maxCCSegId = 0;

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

  _powerdomain_tbl = new dbTable<_dbPowerDomain>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbPowerDomainObj);

  _logicport_tbl = new dbTable<_dbLogicPort>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbLogicPortObj);

  _powerswitch_tbl = new dbTable<_dbPowerSwitch>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbPowerSwitchObj);

  _isolation_tbl = new dbTable<_dbIsolation>(
      db, this, (GetObjTbl_t) &_dbBlock::getObjectTable, dbIsolationObj);

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

  _net_hash.setTable(_net_tbl);
  _inst_hash.setTable(_inst_tbl);
  _module_hash.setTable(_module_tbl);
  _modinst_hash.setTable(_modinst_tbl);
  _powerdomain_hash.setTable(_powerdomain_tbl);
  _logicport_hash.setTable(_logicport_tbl);
  _powerswitch_hash.setTable(_powerswitch_tbl);
  _isolation_hash.setTable(_isolation_tbl);
  _group_hash.setTable(_group_tbl);
  _inst_hdr_hash.setTable(_inst_hdr_tbl);
  _bterm_hash.setTable(_bterm_tbl);

  _net_bterm_itr = new dbNetBTermItr(_bterm_tbl);

  _net_iterm_itr = new dbNetITermItr(_iterm_tbl);

  _inst_iterm_itr = new dbInstITermItr(_iterm_tbl);

  _box_itr = new dbBoxItr(_box_tbl);

  _swire_itr = new dbSWireItr(_swire_tbl);

  _sbox_itr = new dbSBoxItr(_sbox_tbl);

  _cap_node_itr = new dbCapNodeItr(_cap_node_tbl);

  _r_seg_itr = new dbRSegItr(_r_seg_tbl);

  _cc_seg_itr = new dbCCSegItr(_cc_seg_tbl);

  _region_inst_itr = new dbRegionInstItr(_inst_tbl);

  _module_inst_itr = new dbModuleInstItr(_inst_tbl);

  _module_modinst_itr = new dbModuleModInstItr(_modinst_tbl);

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

_dbBlock::_dbBlock(_dbDatabase* db, const _dbBlock& block)
    : _flags(block._flags),
      _def_units(block._def_units),
      _dbu_per_micron(block._dbu_per_micron),
      _hier_delimeter(block._hier_delimeter),
      _left_bus_delimeter(block._left_bus_delimeter),
      _right_bus_delimeter(block._right_bus_delimeter),
      _num_ext_corners(block._num_ext_corners),
      _corners_per_block(block._corners_per_block),
      _corner_name_list(block._corner_name_list),
      _name(nullptr),
      _die_area(block._die_area),
      _tech(block._tech),
      _chip(block._chip),
      _bbox(block._bbox),
      _parent(block._parent),
      _next_block(block._next_block),
      _gcell_grid(block._gcell_grid),
      _parent_block(block._parent_block),
      _parent_inst(block._parent_inst),
      _top_module(block._top_module),
      _net_hash(block._net_hash),
      _inst_hash(block._inst_hash),
      _module_hash(block._module_hash),
      _modinst_hash(block._modinst_hash),
      _powerdomain_hash(block._powerdomain_hash),
      _logicport_hash(block._logicport_hash),
      _powerswitch_hash(block._powerswitch_hash),
      _isolation_hash(block._isolation_hash),
      _group_hash(block._group_hash),
      _inst_hdr_hash(block._inst_hdr_hash),
      _bterm_hash(block._bterm_hash),
      _maxCapNodeId(block._maxCapNodeId),
      _maxRSegId(block._maxRSegId),
      _maxCCSegId(block._maxCCSegId),
      _children(block._children),
      _component_mask_shift(block._component_mask_shift),
      _currentCcAdjOrder(block._currentCcAdjOrder)
{
  if (block._name) {
    _name = strdup(block._name);
    ZALLOCATED(_name);
  }

  _bterm_tbl = new dbTable<_dbBTerm>(db, this, *block._bterm_tbl);

  _iterm_tbl = new dbTable<_dbITerm>(db, this, *block._iterm_tbl);

  _net_tbl = new dbTable<_dbNet>(db, this, *block._net_tbl);

  _inst_hdr_tbl = new dbTable<_dbInstHdr>(db, this, *block._inst_hdr_tbl);

  _inst_tbl = new dbTable<_dbInst>(db, this, *block._inst_tbl);

  _module_tbl = new dbTable<_dbModule>(db, this, *block._module_tbl);

  _modinst_tbl = new dbTable<_dbModInst>(db, this, *block._modinst_tbl);

  _powerdomain_tbl
      = new dbTable<_dbPowerDomain>(db, this, *block._powerdomain_tbl);

  _logicport_tbl = new dbTable<_dbLogicPort>(db, this, *block._logicport_tbl);

  _powerswitch_tbl
      = new dbTable<_dbPowerSwitch>(db, this, *block._powerswitch_tbl);

  _isolation_tbl = new dbTable<_dbIsolation>(db, this, *block._isolation_tbl);

  _group_tbl = new dbTable<_dbGroup>(db, this, *block._group_tbl);

  ap_tbl_ = new dbTable<_dbAccessPoint>(db, this, *block.ap_tbl_);

  global_connect_tbl_
      = new dbTable<_dbGlobalConnect>(db, this, *block.global_connect_tbl_);

  _guide_tbl = new dbTable<_dbGuide>(db, this, *block._guide_tbl);

  _net_tracks_tbl = new dbTable<_dbNetTrack>(db, this, *block._net_tracks_tbl);

  _box_tbl = new dbTable<_dbBox>(db, this, *block._box_tbl);

  _via_tbl = new dbTable<_dbVia>(db, this, *block._via_tbl);

  _gcell_grid_tbl = new dbTable<_dbGCellGrid>(db, this, *block._gcell_grid_tbl);

  _track_grid_tbl = new dbTable<_dbTrackGrid>(db, this, *block._track_grid_tbl);

  _obstruction_tbl
      = new dbTable<_dbObstruction>(db, this, *block._obstruction_tbl);

  _blockage_tbl = new dbTable<_dbBlockage>(db, this, *block._blockage_tbl);

  _wire_tbl = new dbTable<_dbWire>(db, this, *block._wire_tbl);

  _swire_tbl = new dbTable<_dbSWire>(db, this, *block._swire_tbl);

  _sbox_tbl = new dbTable<_dbSBox>(db, this, *block._sbox_tbl);

  _row_tbl = new dbTable<_dbRow>(db, this, *block._row_tbl);

  _fill_tbl = new dbTable<_dbFill>(db, this, *block._fill_tbl);

  _region_tbl = new dbTable<_dbRegion>(db, this, *block._region_tbl);

  _hier_tbl = new dbTable<_dbHier>(db, this, *block._hier_tbl);

  _bpin_tbl = new dbTable<_dbBPin>(db, this, *block._bpin_tbl);

  _non_default_rule_tbl = new dbTable<_dbTechNonDefaultRule>(
      db, this, *block._non_default_rule_tbl);

  _layer_rule_tbl
      = new dbTable<_dbTechLayerRule>(db, this, *block._layer_rule_tbl);

  _prop_tbl = new dbTable<_dbProperty>(db, this, *block._prop_tbl);

  _name_cache = new _dbNameCache(db, this, *block._name_cache);

  _r_val_tbl = new dbPagedVector<float, 4096, 12>(*block._r_val_tbl);

  _c_val_tbl = new dbPagedVector<float, 4096, 12>(*block._c_val_tbl);

  _cc_val_tbl = new dbPagedVector<float, 4096, 12>(*block._cc_val_tbl);

  _cap_node_tbl = new dbTable<_dbCapNode>(db, this, *block._cap_node_tbl);

  _r_seg_tbl = new dbTable<_dbRSeg>(db, this, *block._r_seg_tbl);

  _cc_seg_tbl = new dbTable<_dbCCSeg>(db, this, *block._cc_seg_tbl);

  _extControl = new dbExtControl();

  _net_hash.setTable(_net_tbl);
  _inst_hash.setTable(_inst_tbl);
  _module_hash.setTable(_module_tbl);
  _modinst_hash.setTable(_modinst_tbl);
  _group_hash.setTable(_group_tbl);
  _inst_hdr_hash.setTable(_inst_hdr_tbl);
  _bterm_hash.setTable(_bterm_tbl);
  _powerdomain_hash.setTable(_powerdomain_tbl);
  _logicport_hash.setTable(_logicport_tbl);
  _powerswitch_hash.setTable(_powerswitch_tbl);
  _isolation_hash.setTable(_isolation_tbl);

  _net_bterm_itr = new dbNetBTermItr(_bterm_tbl);

  _net_iterm_itr = new dbNetITermItr(_iterm_tbl);

  _inst_iterm_itr = new dbInstITermItr(_iterm_tbl);

  _box_itr = new dbBoxItr(_box_tbl);

  _swire_itr = new dbSWireItr(_swire_tbl);

  _sbox_itr = new dbSBoxItr(_sbox_tbl);

  _cap_node_itr = new dbCapNodeItr(_cap_node_tbl);

  _r_seg_itr = new dbRSegItr(_r_seg_tbl);

  _cc_seg_itr = new dbCCSegItr(_cc_seg_tbl);

  _region_inst_itr = new dbRegionInstItr(_inst_tbl);

  _module_inst_itr = new dbModuleInstItr(_inst_tbl);

  _module_modinst_itr = new dbModuleModInstItr(_modinst_tbl);

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

  _num_ext_dbs = 0;

  // ??? Initialize search-db on copy?
  _searchDb = nullptr;

  // ??? callbacks
  // _callbacks = ???

  // ??? _ext?
  _extmi = block._extmi;
  _journal = nullptr;
  _journal_pending = nullptr;
}

_dbBlock::~_dbBlock()
{
  if (_name)
    free((void*) _name);

  delete _bterm_tbl;
  delete _iterm_tbl;
  delete _net_tbl;
  delete _inst_hdr_tbl;
  delete _inst_tbl;
  delete _module_tbl;
  delete _modinst_tbl;
  delete _powerdomain_tbl;
  delete _logicport_tbl;
  delete _powerswitch_tbl;
  delete _isolation_tbl;
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

  std::list<dbBlockCallBackObj*>::iterator _cbitr;
  while (_callbacks.begin() != _callbacks.end()) {
    _cbitr = _callbacks.begin();
    (*_cbitr)->removeOwner();
  }
  if (_journal)
    delete _journal;

  if (_journal_pending)
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

  // save a copy of the delimeter
  char delimeter = block->_hier_delimeter;

  std::list<dbBlockCallBackObj*> callbacks;

  // save callbacks
  callbacks.swap(block->_callbacks);

  // unlink the child from the parent
  if (parent)
    unlink_child_from_parent(block, parent);

  // destroy the block contents
  block->~_dbBlock();

  // call in-place new to create new block
  new (block) _dbBlock(db);

  // initialize the
  block->initialize(chip, tech, parent, name, delimeter);

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
                          char delimeter)
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
  _hier_delimeter = delimeter;
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
    default:
      break;
  }

  return getTable()->getObjectTable(type);
}

dbOStream& operator<<(dbOStream& stream, const _dbBlock& block)
{
  std::list<dbBlockCallBackObj*>::const_iterator cbitr;
  for (cbitr = block._callbacks.begin(); cbitr != block._callbacks.end();
       ++cbitr)
    (**cbitr)().inDbBlockStreamOutBefore(
        (dbBlock*) &block);  // client ECO initialization  - payam

  stream << block._def_units;
  stream << block._dbu_per_micron;
  stream << block._hier_delimeter;
  stream << block._left_bus_delimeter;
  stream << block._right_bus_delimeter;
  stream << block._num_ext_corners;
  stream << block._corners_per_block;
  stream << block._corner_name_list;
  stream << block._name;
  stream << block._die_area;
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
  stream << *block._inst_tbl;
  stream << *block._module_tbl;
  stream << *block._modinst_tbl;
  stream << *block._powerdomain_tbl;
  stream << *block._logicport_tbl;
  stream << *block._powerswitch_tbl;
  stream << *block._isolation_tbl;
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
  stream << *block._cap_node_tbl;  // DKF - 2/21/05
  stream << *block._r_seg_tbl;     // DKF - 2/21/05
  stream << *block._cc_seg_tbl;
  stream << *block._extControl;

  //---------------------------------------------------------- stream out
  // properties
  // TOM
  dbObjectTable* table = block.getTable();
  dbId<_dbProperty> propList = table->getPropList(block.getOID());

  stream << propList;
  // TOM
  //----------------------------------------------------------

  for (cbitr = block._callbacks.begin(); cbitr != block._callbacks.end();
       ++cbitr)
    (*cbitr)->inDbBlockStreamOutAfter((dbBlock*) &block);
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbBlock& block)
{
  _dbDatabase* db = block.getImpl()->getDatabase();

  stream >> block._def_units;
  stream >> block._dbu_per_micron;
  stream >> block._hier_delimeter;
  stream >> block._left_bus_delimeter;
  stream >> block._right_bus_delimeter;
  stream >> block._num_ext_corners;
  stream >> block._corners_per_block;
  stream >> block._corner_name_list;
  stream >> block._name;
  stream >> block._die_area;
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
  stream >> block._powerdomain_hash;
  stream >> block._logicport_hash;
  stream >> block._powerswitch_hash;
  stream >> block._isolation_hash;
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
  stream >> *block._inst_tbl;
  stream >> *block._module_tbl;
  stream >> *block._modinst_tbl;
  stream >> *block._powerdomain_tbl;
  stream >> *block._logicport_tbl;
  stream >> *block._powerswitch_tbl;
  stream >> *block._isolation_tbl;
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

  //---------------------------------------------------------- stream in
  // properties
  // TOM
  dbObjectTable* table = block.getTable();
  dbId<_dbProperty> oldList = table->getPropList(block.getOID());
  dbId<_dbProperty> propList;
  stream >> propList;

  if (propList != 0)
    table->setPropList(block.getOID(), propList);
  else if (oldList != 0)
    table->setPropList(block.getOID(), 0);
  // TOM
  //-------------------------------------------------------------------------------

  return stream;
}

void _dbBlock::add_rect(const Rect& rect)
{
  _dbBox* box = _box_tbl->getPtr(_bbox);

  if (_flags._valid_bbox)
    box->_shape._rect.merge(rect);
}
void _dbBlock::add_oct(const Oct& oct)
{
  _dbBox* box = _box_tbl->getPtr(_bbox);

  if (_flags._valid_bbox)
    box->_shape._rect.merge(oct);
}

void _dbBlock::remove_rect(const Rect& rect)
{
  _dbBox* box = _box_tbl->getPtr(_bbox);

  if (_flags._valid_bbox)
    _flags._valid_bbox = box->_shape._rect.inside(rect);
}

bool _dbBlock::operator==(const _dbBlock& rhs) const
{
  if (_flags._valid_bbox != rhs._flags._valid_bbox)
    return false;

  if (_def_units != rhs._def_units)
    return false;

  if (_dbu_per_micron != rhs._dbu_per_micron)
    return false;

  if (_hier_delimeter != rhs._hier_delimeter)
    return false;

  if (_left_bus_delimeter != rhs._left_bus_delimeter)
    return false;

  if (_right_bus_delimeter != rhs._right_bus_delimeter)
    return false;

  if (_num_ext_corners != rhs._num_ext_corners)
    return false;

  if (_corners_per_block != rhs._corners_per_block)
    return false;
  if (_corner_name_list && rhs._corner_name_list) {
    if (strcmp(_corner_name_list, rhs._corner_name_list) != 0)
      return false;
  } else if (_corner_name_list || rhs._corner_name_list)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_die_area != rhs._die_area)
    return false;

  if (_tech != rhs._tech)
    return false;

  if (_chip != rhs._chip)
    return false;

  if (_bbox != rhs._bbox)
    return false;

  if (_parent != rhs._parent)
    return false;

  if (_next_block != rhs._next_block)
    return false;

  if (_gcell_grid != rhs._gcell_grid)
    return false;

  if (_parent_block != rhs._parent_block)
    return false;

  if (_parent_inst != rhs._parent_inst)
    return false;

  if (_top_module != rhs._top_module)
    return false;

  if (_net_hash != rhs._net_hash)
    return false;

  if (_inst_hash != rhs._inst_hash)
    return false;

  if (_module_hash != rhs._module_hash)
    return false;

  if (_modinst_hash != rhs._modinst_hash)
    return false;

  if (_powerdomain_hash != rhs._powerdomain_hash)
    return false;

  if (_logicport_hash != rhs._logicport_hash)
    return false;

  if (_powerswitch_hash != rhs._powerswitch_hash)
    return false;

  if (_isolation_hash != rhs._isolation_hash)
    return false;

  if (_group_hash != rhs._group_hash)
    return false;

  if (_inst_hdr_hash != rhs._inst_hdr_hash)
    return false;

  if (_bterm_hash != rhs._bterm_hash)
    return false;

  if (_maxCapNodeId != rhs._maxCapNodeId)
    return false;

  if (_maxRSegId != rhs._maxRSegId)
    return false;

  if (_maxCCSegId != rhs._maxCCSegId)
    return false;

  if (_children != rhs._children)
    return false;

  if (_component_mask_shift != rhs._component_mask_shift)
    return false;

  if (_currentCcAdjOrder != rhs._currentCcAdjOrder)
    return false;

  if (*_bterm_tbl != *rhs._bterm_tbl)
    return false;

  if (*_iterm_tbl != *rhs._iterm_tbl)
    return false;

  if (*_net_tbl != *rhs._net_tbl)
    return false;

  if (*_inst_hdr_tbl != *rhs._inst_hdr_tbl)
    return false;

  if (*_inst_tbl != *rhs._inst_tbl)
    return false;

  if (*_module_tbl != *rhs._module_tbl)
    return false;

  if (*_modinst_tbl != *rhs._modinst_tbl)
    return false;

  if (*_powerdomain_tbl != *rhs._powerdomain_tbl)
    return false;

  if (*_logicport_tbl != *rhs._logicport_tbl)
    return false;

  if (*_powerswitch_tbl != *rhs._powerswitch_tbl)
    return false;

  if (*_isolation_tbl != *rhs._isolation_tbl)
    return false;

  if (*_group_tbl != *rhs._group_tbl)
    return false;

  if (*ap_tbl_ != *rhs.ap_tbl_)
    return false;

  if (*_guide_tbl != *rhs._guide_tbl)
    return false;

  if (*_net_tracks_tbl != *rhs._net_tracks_tbl)
    return false;

  if (*_box_tbl != *rhs._box_tbl)
    return false;

  if (*_via_tbl != *rhs._via_tbl)
    return false;

  if (*_gcell_grid_tbl != *rhs._gcell_grid_tbl)
    return false;

  if (*_track_grid_tbl != *rhs._track_grid_tbl)
    return false;

  if (*_obstruction_tbl != *rhs._obstruction_tbl)
    return false;

  if (*_blockage_tbl != *rhs._blockage_tbl)
    return false;

  if (*_wire_tbl != *rhs._wire_tbl)
    return false;

  if (*_swire_tbl != *rhs._swire_tbl)
    return false;

  if (*_sbox_tbl != *rhs._sbox_tbl)
    return false;

  if (*_row_tbl != *rhs._row_tbl)
    return false;

  if (*_fill_tbl != *rhs._fill_tbl)
    return false;

  if (*_region_tbl != *rhs._region_tbl)
    return false;

  if (*_hier_tbl != *rhs._hier_tbl)
    return false;

  if (*_bpin_tbl != *rhs._bpin_tbl)
    return false;

  if (*_non_default_rule_tbl != *rhs._non_default_rule_tbl)
    return false;

  if (*_layer_rule_tbl != *rhs._layer_rule_tbl)
    return false;

  if (*_prop_tbl != *rhs._prop_tbl)
    return false;

  if (*_name_cache != *rhs._name_cache)
    return false;

  if (*_r_val_tbl != *rhs._r_val_tbl)
    return false;

  if (*_c_val_tbl != *rhs._c_val_tbl)
    return false;

  if (*_cc_val_tbl != *rhs._cc_val_tbl)
    return false;

  if (*_cap_node_tbl != *rhs._cap_node_tbl)
    return false;

  if (*_r_seg_tbl != *rhs._r_seg_tbl)
    return false;

  if (*_cc_seg_tbl != *rhs._cc_seg_tbl)
    return false;

  return true;
}

void _dbBlock::differences(dbDiff& diff,
                           const char* field,
                           const _dbBlock& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._valid_bbox);
  DIFF_FIELD(_def_units);
  DIFF_FIELD(_dbu_per_micron);
  DIFF_FIELD(_hier_delimeter);
  DIFF_FIELD(_left_bus_delimeter);
  DIFF_FIELD(_right_bus_delimeter);
  DIFF_FIELD(_num_ext_corners);
  DIFF_FIELD(_corners_per_block);
  DIFF_FIELD(_name);
  DIFF_FIELD(_corner_name_list);
  DIFF_FIELD(_die_area);
  DIFF_FIELD(_tech);
  DIFF_FIELD(_chip);
  DIFF_FIELD(_bbox);
  DIFF_FIELD(_parent);
  DIFF_FIELD(_next_block);
  DIFF_OBJECT(_gcell_grid, _gcell_grid_tbl, rhs._gcell_grid_tbl);
  DIFF_FIELD(_parent_block);
  DIFF_FIELD(_parent_inst);
  DIFF_FIELD(_top_module);

  if (!diff.deepDiff()) {
    DIFF_HASH_TABLE(_net_hash);
    DIFF_HASH_TABLE(_inst_hash);
    DIFF_HASH_TABLE(_module_hash);
    DIFF_HASH_TABLE(_modinst_hash);
    DIFF_HASH_TABLE(_powerdomain_hash);
    DIFF_HASH_TABLE(_logicport_hash);
    DIFF_HASH_TABLE(_powerswitch_hash);
    DIFF_HASH_TABLE(_isolation_hash);
    DIFF_HASH_TABLE(_group_hash);
    DIFF_HASH_TABLE(_inst_hdr_hash);
    DIFF_HASH_TABLE(_bterm_hash);
  }

  DIFF_FIELD(_maxCapNodeId);
  DIFF_FIELD(_maxRSegId);
  DIFF_FIELD(_maxCCSegId);
  DIFF_VECTOR(_children);
  DIFF_VECTOR(_component_mask_shift);
  DIFF_FIELD(_currentCcAdjOrder);
  DIFF_TABLE(_bterm_tbl);
  DIFF_TABLE_NO_DEEP(_iterm_tbl);
  DIFF_TABLE(_net_tbl);
  DIFF_TABLE_NO_DEEP(_inst_hdr_tbl);
  DIFF_TABLE(_inst_tbl);
  DIFF_TABLE(_module_tbl);
  DIFF_TABLE(_modinst_tbl);
  DIFF_TABLE(_powerdomain_tbl);
  DIFF_TABLE(_logicport_tbl);
  DIFF_TABLE(_powerswitch_tbl);
  DIFF_TABLE(_isolation_tbl);
  DIFF_TABLE(_group_tbl);
  DIFF_TABLE(ap_tbl_);
  DIFF_TABLE(global_connect_tbl_);
  DIFF_TABLE(_guide_tbl);
  DIFF_TABLE(_net_tracks_tbl);
  DIFF_TABLE_NO_DEEP(_box_tbl);
  DIFF_TABLE(_via_tbl);
  DIFF_TABLE_NO_DEEP(_gcell_grid_tbl);
  DIFF_TABLE(_track_grid_tbl);
  DIFF_TABLE(_obstruction_tbl);
  DIFF_TABLE(_blockage_tbl);
  DIFF_TABLE_NO_DEEP(_wire_tbl);
  DIFF_TABLE_NO_DEEP(_swire_tbl);
  DIFF_TABLE_NO_DEEP(_sbox_tbl);
  DIFF_TABLE(_row_tbl);
  DIFF_TABLE(_fill_tbl);
  DIFF_TABLE(_region_tbl);
  DIFF_TABLE_NO_DEEP(_hier_tbl);
  DIFF_TABLE_NO_DEEP(_bpin_tbl);
  DIFF_TABLE(_non_default_rule_tbl);
  DIFF_TABLE(_layer_rule_tbl);
  DIFF_TABLE_NO_DEEP(_prop_tbl);
  DIFF_NAME_CACHE(_name_cache);

  if (*_r_val_tbl != *rhs._r_val_tbl)
    _r_val_tbl->differences(diff, "_r_val_tbl", *rhs._r_val_tbl);

  if (*_c_val_tbl != *rhs._c_val_tbl)
    _c_val_tbl->differences(diff, "_c_val_tbl", *rhs._c_val_tbl);

  if (*_cc_val_tbl != *rhs._cc_val_tbl)
    _cc_val_tbl->differences(diff, "_c_val_tbl", *rhs._cc_val_tbl);

  DIFF_TABLE_NO_DEEP(_cap_node_tbl);
  DIFF_TABLE_NO_DEEP(_r_seg_tbl);
  DIFF_TABLE_NO_DEEP(_cc_seg_tbl);
  DIFF_END
}

void _dbBlock::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._valid_bbox);
  DIFF_OUT_FIELD(_def_units);
  DIFF_OUT_FIELD(_dbu_per_micron);
  DIFF_OUT_FIELD(_hier_delimeter);
  DIFF_OUT_FIELD(_left_bus_delimeter);
  DIFF_OUT_FIELD(_right_bus_delimeter);
  DIFF_OUT_FIELD(_num_ext_corners);
  DIFF_OUT_FIELD(_corners_per_block);
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_corner_name_list);
  DIFF_OUT_FIELD(_die_area);
  DIFF_OUT_FIELD(_tech);
  DIFF_OUT_FIELD(_chip);
  DIFF_OUT_FIELD(_bbox);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_next_block);
  DIFF_OUT_OBJECT(_gcell_grid, _gcell_grid_tbl);
  DIFF_OUT_FIELD(_parent_block);
  DIFF_OUT_FIELD(_parent_inst);
  DIFF_OUT_FIELD(_top_module);

  if (!diff.deepDiff()) {
    DIFF_OUT_HASH_TABLE(_net_hash);
    DIFF_OUT_HASH_TABLE(_inst_hash);
    DIFF_OUT_HASH_TABLE(_module_hash);
    DIFF_OUT_HASH_TABLE(_modinst_hash);
    DIFF_OUT_HASH_TABLE(_powerdomain_hash);
    DIFF_OUT_HASH_TABLE(_logicport_hash);
    DIFF_OUT_HASH_TABLE(_powerswitch_hash);
    DIFF_OUT_HASH_TABLE(_isolation_hash);
    DIFF_OUT_HASH_TABLE(_group_hash);
    DIFF_OUT_HASH_TABLE(_inst_hdr_hash);
    DIFF_OUT_HASH_TABLE(_bterm_hash);
  }

  DIFF_OUT_FIELD(_maxCapNodeId);
  DIFF_OUT_FIELD(_maxRSegId);
  DIFF_OUT_FIELD(_maxCCSegId);
  DIFF_OUT_VECTOR(_children);
  DIFF_OUT_VECTOR(_component_mask_shift);
  DIFF_OUT_FIELD(_currentCcAdjOrder);
  DIFF_OUT_TABLE(_bterm_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_iterm_tbl);
  DIFF_OUT_TABLE(_net_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_inst_hdr_tbl);
  DIFF_OUT_TABLE(_inst_tbl);
  DIFF_OUT_TABLE(_module_tbl);
  DIFF_OUT_TABLE(_modinst_tbl);
  DIFF_OUT_TABLE(_powerdomain_tbl);
  DIFF_OUT_TABLE(_logicport_tbl);
  DIFF_OUT_TABLE(_powerswitch_tbl);
  DIFF_OUT_TABLE(_isolation_tbl);
  DIFF_OUT_TABLE(_group_tbl);
  DIFF_OUT_TABLE(ap_tbl_);
  DIFF_OUT_TABLE(global_connect_tbl_);
  DIFF_OUT_TABLE(_guide_tbl);
  DIFF_OUT_TABLE(_net_tracks_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_box_tbl);
  DIFF_OUT_TABLE(_via_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_gcell_grid_tbl);
  DIFF_OUT_TABLE(_track_grid_tbl);
  DIFF_OUT_TABLE(_obstruction_tbl);
  DIFF_OUT_TABLE(_blockage_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_wire_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_swire_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_sbox_tbl);
  DIFF_OUT_TABLE(_row_tbl);
  DIFF_OUT_TABLE(_fill_tbl);
  DIFF_OUT_TABLE(_region_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_hier_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_bpin_tbl);
  DIFF_OUT_TABLE(_non_default_rule_tbl);
  DIFF_OUT_TABLE(_layer_rule_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_prop_tbl);
  DIFF_OUT_NAME_CACHE(_name_cache);

  _r_val_tbl->out(diff, side, "_r_val_tbl");
  _c_val_tbl->out(diff, side, "_c_val_tbl");
  _cc_val_tbl->out(diff, side, "_c_val_tbl");

  DIFF_OUT_TABLE_NO_DEEP(_cap_node_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_r_seg_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_cc_seg_tbl);
  DIFF_END
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

  if (block->_flags._valid_bbox == 0)
    ComputeBBox();

  _dbBox* bbox = block->_box_tbl->getPtr(block->_bbox);
  return (dbBox*) bbox;
}

void dbBlock::ComputeBBox()
{
  _dbBlock* block = (_dbBlock*) this;
  _dbBox* bbox = block->_box_tbl->getPtr(block->_bbox);
  bbox->_shape._rect.reset(INT_MAX, INT_MAX, INT_MIN, INT_MIN);

  dbSet<dbInst> insts = getInsts();
  dbSet<dbInst>::iterator iitr;

  for (iitr = insts.begin(); iitr != insts.end(); ++iitr) {
    dbInst* inst = *iitr;
    if (inst->isPlaced()) {
      _dbBox* box = (_dbBox*) inst->getBBox();
      bbox->_shape._rect.merge(box->_shape._rect);
    }
  }

  dbSet<dbBTerm> bterms = getBTerms();
  dbSet<dbBTerm>::iterator bitr;

  for (bitr = bterms.begin(); bitr != bterms.end(); ++bitr) {
    dbBTerm* bterm = *bitr;
    dbSet<dbBPin> bpins = bterm->getBPins();
    dbSet<dbBPin>::iterator pitr;

    for (pitr = bpins.begin(); pitr != bpins.end(); ++pitr) {
      dbBPin* bp = *pitr;
      if (bp->getPlacementStatus().isPlaced()) {
        for (dbBox* box : bp->getBoxes()) {
          Rect r = box->getBox();
          bbox->_shape._rect.merge(r);
        }
      }
    }
  }

  dbSet<dbObstruction> obstructions = getObstructions();
  dbSet<dbObstruction>::iterator oitr;

  for (oitr = obstructions.begin(); oitr != obstructions.end(); ++oitr) {
    dbObstruction* obs = *oitr;
    _dbBox* box = (_dbBox*) obs->getBBox();
    bbox->_shape._rect.merge(box->_shape._rect);
  }

  dbSet<dbSBox> sboxes(block, block->_sbox_tbl);
  dbSet<dbSBox>::iterator sitr;

  for (sitr = sboxes.begin(); sitr != sboxes.end(); ++sitr) {
    dbSBox* box = (dbSBox*) *sitr;
    Rect rect = box->getBox();
    bbox->_shape._rect.merge(rect);
  }

  dbSet<dbWire> wires(block, block->_wire_tbl);
  dbSet<dbWire>::iterator witr;

  for (witr = wires.begin(); witr != wires.end(); ++witr) {
    dbWire* wire = *witr;
    Rect r;
    if (wire->getBBox(r)) {
      bbox->_shape._rect.merge(r);
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

  if (block->_parent_block == 0)
    return nullptr;

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

dbBlock* dbBlock::findChild(const char* name_)
{
  dbSet<dbBlock> children = getChildren();
  dbSet<dbBlock>::iterator itr;

  for (itr = children.begin(); itr != children.end(); ++itr) {
    _dbBlock* child = (_dbBlock*) *itr;
    if (strcmp(child->_name, name_) == 0)
      return (dbBlock*) child;
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

dbModInst* dbBlock::findModInst(const char* path)
{
  char* _path = strdup(path);
  dbModule* cur_mod = getTopModule();
  dbModInst* cur_inst = nullptr;
  char* token = strtok(_path, "/");
  while (token != nullptr) {
    cur_inst = cur_mod->findModInst(token);
    if (cur_inst == nullptr)
      break;
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

  std::string::size_type idx = s.rfind(block->_hier_delimeter);

  if (idx == std::string::npos)  // no delimeter
    return nullptr;

  std::string instName = s.substr(0, idx);
  std::string termName = s.substr(idx + 1, s.size());

  dbInst* inst = findInst(instName.c_str());

  if (inst == nullptr)
    return nullptr;

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

bool dbBlock::findSomeMaster(const char* names, std::vector<dbMaster*>& masters)
{
  if (!names || names[0] == '\0')
    return false;

  dbLib* lib = getChip()->getDb()->findLib("lib");
  dbMaster* master;
  auto parser = std::make_unique<Ath__parser>(getImpl()->getLogger());
  parser->mkWords(names, nullptr);
  // uint noid;
  char* masterName;
  for (int ii = 0; ii < parser->getWordCnt(); ii++) {
    masterName = parser->get(ii);
    master = lib->findMaster(masterName);
    /*
    if (!master)
    {

        //noid = masterName[0]=='N' ? atoi(&masterName[1]) :
    atoi(&masterName[0]); master = dbNet::getValidNet(this, noid);
    }
    */
    if (master)
      masters.push_back(master);
    else
      getImpl()->getLogger()->warn(
          utl::ODB, 5, "Can not find master {}", masterName);
  }
  return masters.size() ? true : false;
}
bool dbBlock::findSomeNet(const char* names, std::vector<dbNet*>& nets)
{
  if (!names || names[0] == '\0')
    return false;
  _dbBlock* block = (_dbBlock*) this;
  dbNet* net;
  auto parser = std::make_unique<Ath__parser>(getImpl()->getLogger());
  parser->mkWords(names, nullptr);
  uint noid;
  char* netName;
  for (int ii = 0; ii < parser->getWordCnt(); ii++) {
    netName = parser->get(ii);
    net = (dbNet*) block->_net_hash.find(netName);
    if (!net) {
      noid = netName[0] == 'N' ? atoi(&netName[1]) : atoi(&netName[0]);
      net = dbNet::getValidNet(this, noid);
    }
    if (net)
      nets.push_back(net);
    else
      getImpl()->getLogger()->warn(utl::ODB, 6, "Can not find net {}", netName);
  }
  return nets.size() ? true : false;
}

bool dbBlock::findSomeInst(const char* names, std::vector<dbInst*>& insts)
{
  if (!names || names[0] == '\0')
    return false;
  _dbBlock* block = (_dbBlock*) this;
  dbInst* inst;
  auto parser = std::make_unique<Ath__parser>(getImpl()->getLogger());
  parser->mkWords(names, nullptr);
  uint ioid;
  char* instName;
  for (int ii = 0; ii < parser->getWordCnt(); ii++) {
    instName = parser->get(ii);
    inst = (dbInst*) block->_inst_hash.find(instName);
    if (!inst) {
      ioid = instName[0] == 'I' ? atoi(&instName[1]) : atoi(&instName[0]);
      inst = dbInst::getValidInst(this, ioid);
    }
    if (inst)
      insts.push_back(inst);
    else
      getImpl()->getLogger()->warn(
          utl::ODB, 7, "Can not find inst {}", instName);
  }
  return insts.size() ? true : false;
}

dbVia* dbBlock::findVia(const char* name)
{
  dbSet<dbVia> vias = getVias();
  dbSet<dbVia>::iterator itr;

  for (itr = vias.begin(); itr != vias.end(); ++itr) {
    _dbVia* via = (_dbVia*) *itr;

    if (strcmp(via->_name, name) == 0)
      return (dbVia*) via;
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

  if (block->_gcell_grid == 0)
    return nullptr;

  return (dbGCellGrid*) block->_gcell_grid_tbl->getPtr(block->_gcell_grid);
}

dbSet<dbRegion> dbBlock::getRegions()
{
  _dbBlock* block = (_dbBlock*) this;
  return dbSet<dbRegion>(block, block->_region_tbl);
}

dbRegion* dbBlock::findRegion(const char* name)
{
  dbSet<dbRegion> regions = getRegions();
  dbSet<dbRegion>::iterator itr;

  for (itr = regions.begin(); itr != regions.end(); ++itr) {
    _dbRegion* r = (_dbRegion*) *itr;

    if (strcmp(r->_name, name) == 0)
      return (dbRegion*) r;
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

char dbBlock::getHierarchyDelimeter()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_hier_delimeter;
}

void dbBlock::setBusDelimeters(char left, char right)
{
  _dbBlock* block = (_dbBlock*) this;
  block->_left_bus_delimeter = left;
  block->_right_bus_delimeter = right;
}

void dbBlock::getBusDelimeters(char& left, char& right)
{
  _dbBlock* block = (_dbBlock*) this;
  left = block->_left_bus_delimeter;
  right = block->_right_bus_delimeter;
}

dbSet<dbTrackGrid> dbBlock::getTrackGrids()
{
  _dbBlock* block = (_dbBlock*) this;
  dbSet<dbTrackGrid> tracks(block, block->_track_grid_tbl);
  return tracks;
}

dbTrackGrid* dbBlock::findTrackGrid(dbTechLayer* layer)
{
  dbSet<dbTrackGrid> tracks = getTrackGrids();
  dbSet<dbTrackGrid>::iterator itr;

  for (itr = tracks.begin(); itr != tracks.end(); ++itr) {
    dbTrackGrid* g = *itr;

    if (g->getTechLayer() == layer)
      return g;
  }

  return nullptr;
}

void dbBlock::getMasters(std::vector<dbMaster*>& masters)
{
  _dbBlock* block = (_dbBlock*) this;
  dbSet<dbInstHdr> inst_hdrs(block, block->_inst_hdr_tbl);
  dbSet<dbInstHdr>::iterator itr;

  for (itr = inst_hdrs.begin(); itr != inst_hdrs.end(); ++itr) {
    dbInstHdr* hdr = *itr;
    masters.push_back(hdr->getMaster());
  }
}

void dbBlock::setDieArea(const Rect& new_area)
{
  _dbBlock* block = (_dbBlock*) this;
  block->_die_area = new_area;
  for (auto callback : block->_callbacks) {
    callback->inDbBlockSetDieArea(this);
  }
}

Rect dbBlock::getDieArea()
{
  _dbBlock* block = (_dbBlock*) this;
  return block->_die_area;
}

Rect dbBlock::getCoreArea()
{
  // filter rows to remove those with site class PAD
  std::vector<odb::dbRow*> rows;
  for (dbRow* row : getRows()) {
    if (row->getSite()->getClass() != odb::dbSiteClass::PAD) {
      rows.push_back(row);
    }
  }
  if (!rows.empty()) {
    Rect rect;
    rect.mergeInit();

    for (dbRow* row : rows) {
      rect.merge(row->getBBox());
    }
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

void dbBlock::getExtCornerNames(std::list<std::string>& ecl)
{
  _dbBlock* block = (_dbBlock*) this;
  if (block->_corner_name_list)
    ecl.push_back(block->_corner_name_list);
  else
    ecl.push_back("");
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
  //_dbBlock * block = (_dbBlock *) this;
  //_dbTech * tech = (_dbTech *) getDb()->getTech();

  dbSet<dbTechNonDefaultRule> rules = getNonDefaultRules();
  dbSet<dbTechNonDefaultRule>::iterator itr;

  for (itr = rules.begin(); itr != rules.end(); ++itr) {
    _dbTechNonDefaultRule* r = (_dbTechNonDefaultRule*) *itr;

    if (strcmp(r->_name, name) == 0)
      return (dbTechNonDefaultRule*) r;
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
    for (j = 1; j < block->_r_val_tbl->size(); j += extDbCnt)
      (*block->_r_val_tbl)[j + to] = (*block->_r_val_tbl)[j + fr] * resFactor;
  } else {
    for (j = 1; j < block->_r_val_tbl->size(); j += extDbCnt)
      (*block->_r_val_tbl)[j + to] = (*block->_r_val_tbl)[j + fr];
  }
  if (ccFactor != 1.0) {
    for (j = 1; j < block->_cc_val_tbl->size(); j += extDbCnt)
      (*block->_cc_val_tbl)[j + to] = (*block->_cc_val_tbl)[j + fr] * ccFactor;
  } else {
    for (j = 1; j < block->_cc_val_tbl->size(); j += extDbCnt)
      (*block->_cc_val_tbl)[j + to] = (*block->_cc_val_tbl)[j + fr];
  }
  if (gndcFactor != 1.0) {
    for (j = 1; j < block->_c_val_tbl->size(); j += extDbCnt)
      (*block->_c_val_tbl)[j + to] = (*block->_c_val_tbl)[j + fr] * gndcFactor;
  } else {
    for (j = 1; j < block->_c_val_tbl->size(); j += extDbCnt)
      (*block->_c_val_tbl)[j + to] = (*block->_c_val_tbl)[j + fr];
  }
}

bool dbBlock::adjustCC(float adjFactor,
                       double ccThreshHold,
                       std::vector<dbNet*>& nets,
                       std::vector<dbNet*>& halonets)
{
  bool adjusted = false;
  dbNet* net;
  std::vector<dbCCSeg*> adjustedCC;
  std::vector<dbNet*>::iterator itr;
  uint adjustOrder = ((_dbBlock*) this)->_currentCcAdjOrder + 1;
  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    net = *itr;
    adjusted |= net->adjustCC(
        adjustOrder, adjFactor, ccThreshHold, adjustedCC, halonets);
  }
  std::vector<dbCCSeg*>::iterator ccitr;
  dbCCSeg* ccs;
  for (ccitr = adjustedCC.begin(); ccitr != adjustedCC.end(); ++ccitr) {
    ccs = *ccitr;
    ccs->setMark(false);
  }
  if (adjusted)
    ((_dbBlock*) this)->_currentCcAdjOrder = adjustOrder;
  return adjusted;
}

bool dbBlock::groundCC(float gndFactor)
{
  if (gndFactor == 0.0)
    return false;
  bool grounded = false;
  dbNet* net;
  dbSet<dbNet> nets = getNets();
  dbSet<dbNet>::iterator nitr;
  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    net = *nitr;
    grounded |= net->groundCC(gndFactor);
  }
  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    net = *nitr;
    dbNet* net = *nitr;
    net->destroyCCSegs();
  }
  return grounded;
}

void dbBlock::undoAdjustedCC(std::vector<dbNet*>& nets,
                             std::vector<dbNet*>& halonets)
{
  dbNet* net;
  std::vector<dbCCSeg*> adjustedCC;
  std::vector<dbNet*>::iterator itr;
  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    net = *itr;
    net->undoAdjustedCC(adjustedCC, halonets);
  }
  std::vector<dbCCSeg*>::iterator ccitr;
  dbCCSeg* ccs;
  for (ccitr = adjustedCC.begin(); ccitr != adjustedCC.end(); ++ccitr) {
    ccs = *ccitr;
    ccs->setMark(false);
  }
}

void dbBlock::adjustRC(double resFactor, double ccFactor, double gndcFactor)
{
  _dbBlock* block = (_dbBlock*) this;
  uint j;
  if (resFactor != 1.0) {
    for (j = 1; j < block->_r_val_tbl->size(); j++)
      (*block->_r_val_tbl)[j] *= resFactor;
  }
  if (ccFactor != 1.0) {
    for (j = 1; j < block->_cc_val_tbl->size(); j++)
      (*block->_cc_val_tbl)[j] *= ccFactor;
  }
  if (gndcFactor != 1.0) {
    for (j = 1; j < block->_c_val_tbl->size(); j++)
      (*block->_c_val_tbl)[j] *= gndcFactor;
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
    dbSet<dbNet> nets = getNets();
    dbSet<dbNet>::iterator nitr;

    for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
      _dbNet* n = (_dbNet*) *nitr;
      n->_cap_nodes = 0;  // DKF
      n->_r_segs = 0;     // DKF
    }
  }

  int ttttClear = 1;
  _dbDatabase* db = block->_cap_node_tbl->_db;
  if (ttttClear)
    block->_cap_node_tbl->clear();
  else {
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

  if (ttttClear)
    block->_r_seg_tbl->clear();
  else {
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

  if (ttttClear)
    block->_cc_seg_tbl->clear();
  else {
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

  if (ttttClear)
    block->_cc_val_tbl->clear();
  else {
    delete block->_cc_val_tbl;
    block->_cc_val_tbl = new dbPagedVector<float, 4096, 12>();
  }
  block->_cc_val_tbl->push_back(0.0);

  if (ttttClear)
    block->_r_val_tbl->clear();
  else {
    delete block->_r_val_tbl;
    block->_r_val_tbl = new dbPagedVector<float, 4096, 12>();
  }
  block->_r_val_tbl->push_back(0.0);

  if (ttttClear)
    block->_c_val_tbl->clear();
  else {
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
    if (block->_corner_name_list)
      free(block->_corner_name_list);
    block->_corner_name_list = strdup((char*) name_list);
  }
}
dbBlock* dbBlock::getExtCornerBlock(uint corner)
{
  dbBlock* block = findExtCornerBlock(corner);
  if (!block)
    block = this;
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
  dbSet<dbNet> nets = getNets();
  dbSet<dbNet>::iterator nitr;
  dbNet* net;
  char name[64];
  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    net = *nitr;
    sprintf(name, "%d", net->getId());
    dbNet* xnet = dbNet::create(extBlk, name, true);
    if (xnet == nullptr) {
      getImpl()->getLogger()->error(
          utl::ODB, 8, "Cannot duplicate net {}", net->getConstName());
    }
    if (xnet->getId() != net->getId())
      getImpl()->getLogger()->warn(utl::ODB,
                                   9,
                                   "id mismatch ({},{}) for net {}",
                                   xnet->getId(),
                                   net->getId(),
                                   net->getConstName());
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

  if (block->_corner_name_list != nullptr)
    free(block->_corner_name_list);

  block->_corner_name_list = strdup(name_list);
}
void dbBlock::getExtCornerName(int corner, char* cName)
{
  cName[0] = '\0';
  _dbBlock* block = (_dbBlock*) this;
  if (block->_num_ext_corners == 0)
    return;
  ZASSERT((corner >= 0) && (corner < block->_num_ext_corners));

  if (block->_corner_name_list == nullptr)
    return;

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
  return;
}
int dbBlock::getExtCornerIndex(const char* cornerName)
{
  _dbBlock* block = (_dbBlock*) this;

  if (block->_corner_name_list == nullptr)
    return -1;

  char buff[1024];
  strcpy(buff, block->_corner_name_list);

  uint ii = 0;
  char* word = strtok(buff, " ");
  while (word != nullptr) {
    if (strcmp(cornerName, word) == 0)
      return ii;

    word = strtok(nullptr, " ");
    ii++;
  }
  return -1;
}
void dbBlock::setCornerCount(int cnt)
{
  setCornerCount(cnt, cnt, nullptr);
}

void dbBlock::copyViaTable(dbBlock* dst_, dbBlock* src_)
{
  _dbBlock* dst = (_dbBlock*) dst_;
  _dbBlock* src = (_dbBlock*) src_;
  delete dst->_via_tbl;
  dst->_via_tbl = new dbTable<_dbVia>(dst->getDatabase(), dst, *src->_via_tbl);
}

dbBlock* dbBlock::create(dbChip* chip_,
                         const char* name_,
                         dbTech* tech_,
                         char hier_delimeter_)
{
  _dbChip* chip = (_dbChip*) chip_;

  if (chip->_top != 0)
    return nullptr;

  if (!tech_) {
    tech_ = chip_->getDb()->getTech();
  }

  _dbBlock* top = chip->_block_tbl->create();
  _dbTech* tech = (_dbTech*) tech_;
  top->initialize(chip, tech, nullptr, name_, hier_delimeter_);
  chip->_top = top->getOID();
  top->_dbu_per_micron = tech->_dbu_per_micron;
  return (dbBlock*) top;
}

dbBlock* dbBlock::create(dbBlock* parent_,
                         const char* name_,
                         dbTech* tech_,
                         char hier_delimeter)
{
  if (parent_->findChild(name_))
    return nullptr;

  if (!tech_) {
    tech_ = parent_->getTech();
  }

  _dbBlock* parent = (_dbBlock*) parent_;
  _dbChip* chip = (_dbChip*) parent->getOwner();
  _dbBlock* child = chip->_block_tbl->create();
  _dbTech* tech = (_dbTech*) tech_;
  child->initialize(chip, tech, parent, name_, hier_delimeter);
  child->_dbu_per_micron = tech->_dbu_per_micron;
  return (dbBlock*) child;
}

dbBlock* dbBlock::duplicate(dbBlock* child_, const char* name_)
{
  _dbBlock* child = (_dbBlock*) child_;

  // must be a child block
  if (child->_parent == 0)
    return nullptr;

  _dbBlock* parent = (_dbBlock*) child_->getParent();
  _dbChip* chip = (_dbChip*) child->getOwner();

  // make a copy
  _dbBlock* dup = chip->_block_tbl->duplicate(child);

  // link child-to-parent
  parent->_children.push_back(dup->getOID());
  dup->_parent = parent->getOID();

  if (name_ && dup->_name) {
    free((void*) dup->_name);
    dup->_name = strdup(name_);
    ZALLOCATED(dup->_name);
  }

  return (dbBlock*) dup;
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
  for (dbId<_dbBlock> child_id : block->_children) {
    _dbBlock* child = chip->_block_tbl->getPtr(child_id);
    destroy((dbBlock*) child);
  }
  // Deleting top block
  if (block->_parent == 0)
    chip->_top = 0;
  else {
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

  dbVector<dbId<_dbBlock>>::iterator citr;

  for (citr = parent->_children.begin(); citr != parent->_children.end();
       ++citr) {
    if (*citr == id) {
      parent->_children.erase(citr);
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
  dbSet<dbNet> nets = getNets();
  dbSet<dbNet>::iterator nitr;

  int tot = 0;
  int upd = 0;
  int enc = 0;
  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    tot++;
    dbNet* net = *nitr;

    _dbNet* n = (_dbNet*) *nitr;

    if (n->_flags._wire_altered != 1)
      continue;
    upd++;
    enc++;

    result.push_back(net);
  }
  getImpl()->getLogger()->info(
      utl::ODB, 10, "tot = {}, upd = {}, enc = {}", tot, upd, enc);
}

void dbBlock::destroyCCs(std::vector<dbNet*>& nets)
{
  std::vector<dbNet*>::iterator itr;

  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    dbNet* net = *itr;
    net->destroyCCSegs();
  }
}

void dbBlock::destroyRSegs(std::vector<dbNet*>& nets)
{
  std::vector<dbNet*>::iterator itr;

  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    dbNet* net = *itr;
    net->destroyRSegs();
  }
}

void dbBlock::destroyCNs(std::vector<dbNet*>& nets, bool cleanExtid)
{
  std::vector<dbNet*>::iterator itr;

  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    dbNet* net = *itr;
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
  if (!extCornersAreIndependent())
    return;
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
  for (jj = 0; jj < changedNets.size(); jj++)
    changedNets[jj]->setMark(true);
  for (jj = 0; jj < changedNets.size(); jj++) {
    dbSet<dbCapNode> capNodes = changedNets[jj]->getCapNodes();
    dbSet<dbCapNode>::iterator citr;
    for (citr = capNodes.begin(); citr != capNodes.end(); ++citr) {
      dbCapNode* capn = *citr;
      dbSet<dbCCSeg> ccSegs = capn->getCCSegs();
      dbSet<dbCCSeg>::iterator ccitr;
      for (ccitr = ccSegs.begin(); ccitr != ccSegs.end();) {
        dbCCSeg* cc = *ccitr;
        ++ccitr;
        dbCapNode* tcap = cc->getSourceCapNode();
        if (tcap != capn)
          ccNet = tcap->getNet();
        else
          ccNet = cc->getTargetCapNode()->getNet();
        if (ccNet->isMarked())
          continue;
        ccNet->setMark(true);
        ccHaloNets.push_back(ccNet);
      }
    }
  }
  for (jj = 0; jj < changedNets.size(); jj++)
    changedNets[jj]->setMark(false);
  for (jj = 0; jj < ccHaloNets.size(); jj++)
    ccHaloNets[jj]->setMark(false);
}

void dbBlock::restoreOldCornerParasitics(dbBlock* pblock,
                                         std::vector<dbNet*>& nets,
                                         bool coupled_rc,
                                         std::vector<dbNet*>& ccHaloNets,
                                         std::vector<uint>& capnn,
                                         std::vector<uint>& rsegn)
{
  // destroyParasitics(nets);  ** discard new parasitics
  uint jj;
  dbNet* net;
  dbCCSeg* ccSeg;
  dbCapNode* capnd;
  dbCapNode* otherCapnode;
  dbNet* otherNet;
  uint otherid;
  std::vector<dbNet*>::iterator itr;

  for (jj = 0; jj < nets.size(); jj++) {
    net = dbNet::getNet(this, nets[jj]->getId());
    net->set1stCapNodeId(capnn[jj]);
    // have extId of terms becoming per corner ??
    if (pblock == this)
      net->setTermExtIds(1);
    net->set1stRSegId(rsegn[jj]);
  }
  for (itr = nets.begin(); itr != nets.end(); ++itr)
    (*itr)->setMark(true);
  for (itr = ccHaloNets.begin(); itr != ccHaloNets.end(); ++itr)
    (*itr)->setMark_1(true);
  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    net = dbNet::getNet(this, (*itr)->getId());
    dbSet<dbCapNode> nodeSet = net->getCapNodes();
    dbSet<dbCapNode>::iterator rc_itr;
    for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
      capnd = *rc_itr;
      dbSet<dbCCSeg> ccSegs = capnd->getCCSegs();
      dbSet<dbCCSeg>::iterator ccitr;
      for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ccitr++) {
        ccSeg = *ccitr;
        otherCapnode = ccSeg->getTheOtherCapn(capnd, otherid);
        otherNet = dbNet::getNet(pblock, otherCapnode->getNet()->getId());
        if (otherNet->isMarked())
          continue;
        if (otherNet->isMark_1ed() || !coupled_rc)  // link_cc_seg
          ccSeg->Link_cc_seg(otherCapnode, otherid);
        else {
          getImpl()->getLogger()->warn(
              utl::ODB,
              11,
              "net {} {} capNode {} ccseg {} has otherCapNode {} not from "
              "changed or halo nets",
              net->getId(),
              (char*) net->getConstName(),
              capnd->getId(),
              ccSeg->getId(),
              otherCapnode->getId());
          getImpl()->getLogger()->error(
              utl::ODB,
              12,
              "   the other capNode is from net {} {}",
              otherNet->getId(),
              otherNet->getConstName());
        }
      }
    }
  }
  for (itr = nets.begin(); itr != nets.end(); ++itr)
    (*itr)->setMark(false);
  for (itr = ccHaloNets.begin(); itr != ccHaloNets.end(); ++itr)
    (*itr)->setMark_1(false);
}

void dbBlock::restoreOldParasitics(std::vector<dbNet*>& nets,
                                   bool coupled_rc,
                                   std::vector<dbNet*>& ccHaloNets,
                                   std::vector<uint>* capnn,
                                   std::vector<uint>* rsegn)
{
  restoreOldCornerParasitics(
      this, nets, coupled_rc, ccHaloNets, capnn[0], rsegn[0]);
  if (!extCornersAreIndependent())
    return;
  int numcorners = getCornerCount();
  dbBlock* extBlock;
  for (int corner = 1; corner < numcorners; corner++) {
    extBlock = findExtCornerBlock(corner);
    extBlock->restoreOldCornerParasitics(
        this, nets, coupled_rc, ccHaloNets, capnn[corner], rsegn[corner]);
  }
}

void dbBlock::destroyOldCornerParasitics(std::vector<dbNet*>& nets,
                                         std::vector<uint>& capnn,
                                         std::vector<uint>& rsegn)
{
  std::vector<dbNet*> cnets;
  std::vector<uint> ncapnn;
  std::vector<uint> nrsegn;
  uint jj;
  for (jj = 0; jj < nets.size(); jj++) {
    dbNet* net = dbNet::getNet(this, nets[jj]->getId());
    cnets.push_back(net);
    ncapnn.push_back(net->get1stCapNodeId());
    net->set1stCapNodeId(capnn[jj]);
    nrsegn.push_back(net->get1stRSegId());
    net->set1stRSegId(rsegn[jj]);
  }
  // destroyParasitics(nets);
  destroyCCs(cnets);
  destroyRSegs(cnets);
  destroyCNs(cnets, false);  // don't touch ext_id's of terms
  for (jj = 0; jj < cnets.size(); jj++) {
    dbNet* net = cnets[jj];
    net->set1stCapNodeId(ncapnn[jj]);
    net->set1stRSegId(nrsegn[jj]);
  }
}

void dbBlock::destroyOldParasitics(std::vector<dbNet*>& nets,
                                   std::vector<uint>* capnn,
                                   std::vector<uint>* rsegn)
{
  destroyOldCornerParasitics(nets, capnn[0], rsegn[0]);
  if (!extCornersAreIndependent())
    return;
  int numcorners = getCornerCount();
  dbBlock* extBlock;
  for (int corner = 1; corner < numcorners; corner++) {
    extBlock = findExtCornerBlock(corner);
    extBlock->destroyOldCornerParasitics(nets, capnn[corner], rsegn[corner]);
  }
}

void dbBlock::replaceOldParasitics(std::vector<dbNet*>& nets,
                                   std::vector<uint>& capnn,
                                   std::vector<uint>& rsegn)
{
  dbNet* net;
  _dbBlock* block = (_dbBlock*) this;

  uint jj;

  dbJournal* tmpj = block->_journal;
  block->_journal = nullptr;
  for (jj = 0; jj < nets.size(); jj++) {
    net = nets[jj];
    capnn.push_back(net->get1stCapNodeId());
    net->set1stCapNodeId(0);
    rsegn.push_back(net->get1stRSegId());
    net->set1stRSegId(0);
    net->createZeroRc(getExtControl()->_foreign);
  }
  block->_journal = tmpj;
}

void dbBlock::restoreOldParasitics(std::vector<dbNet*>& nets,
                                   std::vector<uint>& capnn,
                                   std::vector<uint>& rsegn)
{
  uint jj;
  dbNet* net;
  _dbBlock* block = (_dbBlock*) this;

  dbJournal* tmpj = block->_journal;
  block->_journal = nullptr;
  destroyParasitics(nets);
  for (jj = 0; jj < nets.size(); jj++) {
    net = nets[jj];
    net->set1stCapNodeId(capnn[jj]);
    net->set1stRSegId(rsegn[jj]);
  }
  block->_journal = tmpj;
}

void dbBlock::keepOldCornerParasitics(dbBlock* pblock,
                                      std::vector<dbNet*>& nets,
                                      bool coupled_rc,
                                      std::vector<dbNet*>& ccHaloNets,
                                      std::vector<uint>& capnn,
                                      std::vector<uint>& rsegn)
{
  dbNet* net;
  dbNet* onet;
  dbCapNode* capnd;
  dbCapNode* other;
  dbCCSeg* ccSeg;
  uint cid;

  std::vector<dbNet*>::iterator itr;

  for (itr = nets.begin(); itr != nets.end(); ++itr)
    (*itr)->setMark(true);
  for (itr = ccHaloNets.begin(); itr != ccHaloNets.end(); ++itr)
    (*itr)->setMark_1(true);
  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    net = dbNet::getNet(this, (*itr)->getId());
    dbSet<dbCapNode> nodeSet = net->getCapNodes();
    dbSet<dbCapNode>::iterator rc_itr;
    for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
      capnd = *rc_itr;
      dbSet<dbCCSeg> ccSegs = capnd->getCCSegs();
      dbSet<dbCCSeg>::iterator ccitr;
      for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ccitr++) {
        ccSeg = *ccitr;
        other = ccSeg->getTheOtherCapn(capnd, cid);
        onet = dbNet::getNet(pblock, other->getNet()->getId());
        if (onet->isMarked())
          continue;
        if (onet->isMark_1ed() || !coupled_rc)
          ccSeg->unLink_cc_seg(other);
        else
          getImpl()->getLogger()->error(
              utl::ODB,
              13,
              "ccseg {} has other capn {} not from changed or halo nets",
              ccSeg->getId(),
              other->getId());
      }
    }
  }
  for (itr = nets.begin(); itr != nets.end(); ++itr)
    (*itr)->setMark(false);
  for (itr = ccHaloNets.begin(); itr != ccHaloNets.end(); ++itr)
    (*itr)->setMark_1(false);
  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    net = dbNet::getNet(this, (*itr)->getId());
    // have extId of terms becoming per corner ??
    if (pblock == this)
      net->setTermExtIds(0);
    capnn.push_back(net->get1stCapNodeId());
    net->set1stCapNodeId(0);
    rsegn.push_back(net->get1stRSegId());
    net->set1stRSegId(0);
  }
}

void dbBlock::keepOldParasitics(std::vector<dbNet*>& nets,
                                bool coupled_rc,
                                std::vector<dbNet*>& ccHaloNets,
                                std::vector<uint>* capnn,
                                std::vector<uint>* rsegn)
{
  keepOldCornerParasitics(
      this, nets, coupled_rc, ccHaloNets, capnn[0], rsegn[0]);
  if (!extCornersAreIndependent())
    return;
  int numcorners = getCornerCount();
  dbBlock* extBlock;
  for (int corner = 1; corner < numcorners; corner++) {
    extBlock = findExtCornerBlock(corner);
    extBlock->keepOldCornerParasitics(
        this, nets, coupled_rc, ccHaloNets, capnn[corner], rsegn[corner]);
  }
}

#if 0
//
// Utility to create a net comprising a single SWire and two BTerms
// Returns pointer to net (nullptr iff not successful)
//
dbNet *
dbBlock::createNetSingleSWire(const char *innm, int x1, int y1, int x2, int y2, uint rlevel)
{
  if (!innm)
    return nullptr;

  dbTech *intech = ((dbDatabase *) (getChip())->getOwner())->getTech();
  dbTechLayer *inly = nullptr;
  if (!intech || ((inly = intech->findRoutingLayer(rlevel)) == nullptr))
    return nullptr;
  
  if (x2 < x1)
    std::swap(x1,x2);
  if (y2 < y1)
    std::swap(y1, y2);

  dbNet *nwnet = dbNet::create(this,innm);
  if (!nwnet)
    return nullptr;

  nwnet->setSigType(dbSigType::SIGNAL);

  dbSWire *nwsw = dbSWire::create(nwnet, dbWireType::ROUTED);
  if (!nwsw)
    return nullptr;
  dbSBox  *nwsbx = dbSBox::create(nwsw, inly, x1, y1, x2, y2, dbWireShapeType::NONE);
  if (!nwsbx)
    return nullptr;

  std::pair<dbBTerm *, dbBTerm *> cktrms = nwnet->createTerms4SingleNet(x1, y1, x2, y2, inly);
  if ((cktrms.first == nullptr) || (cktrms.second == nullptr))
    return nullptr;

  return nwnet;
}

//
// Utility to create a net comprising a single Wire and two BTerms
// Requires creating a suitable non-default rule for the wire if none exists.
// Returns pointer to net (nullptr iff not successful)
//
dbNet *
dbBlock::createNetSingleWire(const char *innm, int x1, int y1, int x2, int y2, uint rlevel, bool skipBterms, dbTechLayerDir indir)
{
        static int opendx = -1;

	if (!innm)
		return nullptr;
	
	dbTech *intech = ((dbDatabase *) (getChip())->getOwner())->getTech();
	dbTechLayer *inly = nullptr;
	if (!intech || ((inly = intech->findRoutingLayer(rlevel)) == nullptr))
		return nullptr;
	
	if (x2 < x1)
		std::swap(x1,x2);
	if (y2 < y1)
		std::swap(y1, y2);
	
	dbNet *nwnet = dbNet::create(this,innm);
	if (!nwnet)
		return nullptr;
	
	nwnet->setSigType(dbSigType::SIGNAL);

	std::pair<dbBTerm *, dbBTerm *> blutrms;
	if (! skipBterms ) {
		blutrms = nwnet->createTerms4SingleNet(x1, y1, x2, y2, inly);
		
		if ((blutrms.first == nullptr) || (blutrms.second == nullptr))
			return nullptr;
	}

	dbWireEncoder ncdr;
	ncdr.begin(dbWire::create(nwnet));
	
	int fwidth;
	if (indir == dbTechLayerDir::NONE)
	  fwidth = std::min(x2 - x1, y2 - y1);
	else
	  fwidth = (indir == dbTechLayerDir::VERTICAL) ? x2 - x1 : y2 - y1;

	uint hwidth = fwidth/2;

	if (inly->getWidth()==fwidth) {
		ncdr.newPath(inly, dbWireType::ROUTED);
	}
	else {
		dbSet<dbTechNonDefaultRule> nd_rules = intech->getNonDefaultRules();
		dbSet<dbTechNonDefaultRule>::iterator nditr;
		dbTechLayerRule *tst_rule;
		dbTechNonDefaultRule  *wdth_rule = nullptr;
		for (nditr = nd_rules.begin(); nditr != nd_rules.end(); ++nditr)
		{
			tst_rule = (*nditr)->getLayerRule(inly);
			if (tst_rule && (tst_rule->getWidth() == fwidth))
			{
				wdth_rule = (*nditr);
				break;
			}
		}
		
		char  rule_name[14];
		dbTechLayer *curly;
		if (!wdth_rule)
		{
			// Find first open slot, opendx static so only search once
			if (opendx == -1) 
			{
				for (opendx = 1; opendx <= 300000; ++opendx)
				{
					snprintf(rule_name, 14, "ADS_ND_%d", opendx);
					if ((wdth_rule = dbTechNonDefaultRule::create(intech, rule_name)) != nullptr)
						break;
				}
			}
			else
			{
				snprintf(rule_name, 14, "ADS_ND_%d", ++opendx);
				assert(wdth_rule = dbTechNonDefaultRule::create(intech, rule_name));
			}
			
			if (!wdth_rule)
			{
        
				getImpl()->getLogger()->warn(utl::ODB, 14, "Failed to generate non-default rule for single wire net {}", innm);
				return nullptr;
			}
			
			dbTechLayerRule *curly_rule;
			int i;
			for (i = 1; i <= 12; i++)   // Twelve routing layers??
			{
				if ((curly = intech->findRoutingLayer(i)) != nullptr)
				{
					curly_rule = dbTechLayerRule::create(wdth_rule, curly);
					curly_rule->setWidth(MAX(fwidth,curly->getWidth()));
					curly_rule->setSpacing(curly->getSpacing());
				}
			}
			
			dbTechVia  *curly_via;
			dbSet<dbTechVia> all_vias = intech->getVias();
			dbSet<dbTechVia>::iterator viter;
			std::string  nd_via_name("");
			for (viter = all_vias.begin(); viter != all_vias.end(); ++viter)
			{
				if (((*viter)->getNonDefaultRule() == nullptr) && ((*viter)->isDefault()))
				{
					nd_via_name = std::string(rule_name) + std::string("_") + std::string((*viter)->getName().c_str());
					curly_via = dbTechVia::clone(wdth_rule, (*viter), nd_via_name.c_str());
				}
			}
		}
		
		ncdr.newPath(inly, dbWireType::ROUTED, wdth_rule->getLayerRule(inly));
	}
	if (((x2-x1) == fwidth) || (indir == dbTechLayerDir::VERTICAL)) {
		if ((y2-y1) == fwidth)
			ncdr.addPoint(x1+hwidth, y1+hwidth);
		else
			ncdr.addPoint(x1+hwidth, y1, 0);
		//ncdr.addPoint(x1+hwidth, y2);
	}
	else 
		ncdr.addPoint(x1, y1+hwidth, 0);
	
	if (! skipBterms )
		ncdr.addBTerm(blutrms.first);
	
	if (((x2-x1) == fwidth) || (indir == dbTechLayerDir::VERTICAL)) {
		if ((y2-y1) == fwidth)
			ncdr.addPoint(x1+hwidth, y2-hwidth+1);
		else
			ncdr.addPoint(x1+hwidth, y2, 0);
		//ncdr.addPoint(x1+hwidth, y2);
	}
	else
		ncdr.addPoint(x2, y1+hwidth, 0);
	
	if (! skipBterms )
		ncdr.addBTerm(blutrms.second);

	ncdr.end();
	
	return nwnet;
}
#endif

//
// Utility to write db file
//

void dbBlock::writeDb(char* filename, int allNode)
{
  _dbBlock* block = (_dbBlock*) this;
  std::string dbname;
  if (allNode) {
    if (block->_journal)
      dbname = fmt::format("{}.main.{}.db", filename, getpid());
    else
      dbname = fmt::format("{}.remote.{}.db", filename, getpid());
  } else
    dbname = fmt::format("{}.db", filename);
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
    if (!net->getGuides().empty())
      nets.push_back(net);
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

bool dbBlock::differences(dbBlock* block1,
                          dbBlock* block2,
                          FILE* out,
                          int indent)
{
  _dbBlock* b1 = (_dbBlock*) block1;
  _dbBlock* b2 = (_dbBlock*) block2;

  dbDiff diff(out);
  diff.setDeepDiff(true);
  diff.setIndentPerLevel(indent);
  b1->differences(diff, nullptr, *b2);
  return diff.hasDifferences();
}

uint dbBlock::levelize(std::vector<dbInst*>& startingInsts,
                       std::vector<dbInst*>& instsToBeLeveled)
{
  if (startingInsts.size() <= 0)
    return 0;

  std::vector<dbInst*>::iterator itr;
  for (itr = startingInsts.begin(); itr != startingInsts.end(); ++itr) {
    dbInst* inst = *itr;
    int l = inst->getLevel();
    if (l == 0)
      continue;
    uint level = 0;
    if (l < 0)
      level = -l;
    else
      level = l;
    dbSet<dbITerm> iterms = inst->getITerms();
    dbSet<dbITerm>::iterator iitr;
    for (iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
      dbITerm* iterm = *iitr;
      if ((iterm->getSigType() == dbSigType::GROUND)
          || (iterm->getSigType() == dbSigType::POWER))
        continue;

      if ((iterm->getIoType() == dbIoType::INPUT)
          || (iterm->getIoType() == dbIoType::INOUT))
        continue;

      dbNet* net = iterm->getNet();
      if (net != nullptr) {
        net->setLevelAtFanout(level + 1, false, instsToBeLeveled);
      }
    }
  }
  return instsToBeLeveled.size();
}
uint dbBlock::levelizeFromPrimaryInputs()
{
  dbSet<dbBTerm> bterms = getBTerms();
  dbSet<dbBTerm>::iterator bitr;

  std::vector<dbInst*> instsToBeLeveled;

  uint level = 1;
  for (bitr = bterms.begin(); bitr != bterms.end(); ++bitr) {
    dbBTerm* bterm = *bitr;

    dbNet* net = bterm->getNet();

    if (net != nullptr) {
      if ((net->getSigType() == dbSigType::GROUND)
          || (net->getSigType() == dbSigType::POWER))
        continue;

      net->setLevelAtFanout(level, true, instsToBeLeveled);
    }
  }
  if (instsToBeLeveled.size() <= 0)
    return 0;

  while (1) {
    std::vector<dbInst*> startingInsts = instsToBeLeveled;
    instsToBeLeveled.clear();

    uint cnt = levelize(startingInsts, instsToBeLeveled);
    if (cnt == 0)
      break;
  }
  return 0;
}
uint dbBlock::levelizeFromSequential()
{
  std::vector<dbInst*> instsToBeLeveled;

  dbSet<dbInst> insts = getInsts();
  dbSet<dbInst>::iterator iitr;

  for (iitr = insts.begin(); iitr != insts.end(); ++iitr) {
    dbInst* inst = *iitr;
    if (!inst->getMaster()->isSequential())
      continue;
    inst->setLevel(1, false);
    instsToBeLeveled.push_back(inst);
  }
  if (instsToBeLeveled.size() <= 0)
    return 0;

  while (1) {
    std::vector<dbInst*> startingInsts = instsToBeLeveled;
    instsToBeLeveled.clear();

    uint cnt = levelize(startingInsts, instsToBeLeveled);
    if (cnt == 0)
      break;
  }
  return 0;
}
int dbBlock::markBackwardsUser2(dbInst* firstInst,
                                bool mark,
                                std::vector<dbInst*>& resultTable)
{
  std::vector<dbInst*> instsToBeMarked;

  if (firstInst == nullptr) {
    dbSet<dbInst> insts = getInsts();
    dbSet<dbInst>::iterator iitr;

    for (iitr = insts.begin(); iitr != insts.end(); ++iitr) {
      dbInst* inst = *iitr;
      if (!inst->getMaster()->isSequential())
        continue;

      instsToBeMarked.push_back(inst);
    }
  } else {
    instsToBeMarked.push_back(firstInst);
  }
  if (instsToBeMarked.size() <= 0)
    return 0;

  while (1) {
    std::vector<dbInst*> startingInsts = instsToBeMarked;
    instsToBeMarked.clear();

    int cnt
        = markBackwardsUser2(startingInsts, instsToBeMarked, mark, resultTable);
    if (cnt == 0)
      break;
    if (!mark && (cnt < 0))
      return -1;
  }
  return 0;
}
/*
int dbBlock::markBackwardsUser2(std::vector<dbInst *> & startingInsts,
std::vector<dbInst *> & instsToMark, bool mark, std::vector<dbInst *> &
resultTable)
{
        if (startingInsts.size()<=0)
                return 0;

        std::vector<dbInst *>::iterator itr;
        for (itr= startingInsts.begin(); itr != startingInsts.end(); ++itr)
        {
                dbInst *inst= *itr;
                if (mark) {
                        inst->setUserFlag2();
                        //resultTable.push_back(inst);
                }
                else if (inst->getUserFlag2())
                        return -1;

                dbSet<dbITerm> iterms= inst->getITerms();
                dbSet<dbITerm>::iterator iitr;
                for (iitr= iterms.begin(); iitr != iterms.end(); ++iitr)
                {
                        dbITerm *iterm= *iitr;
                        if ((iterm->getSigType() == dbSigType::GROUND)||
(iterm->getSigType() == dbSigType::POWER)) continue; if
(!iterm->isInputSignal()) continue; if (iterm->isClocked()) continue;

                        dbNet *inputNet= iterm->getNet();

                        if (inputNet==nullptr)
                                continue;

                        if ((inputNet->getSigType()==dbSigType::GROUND)||
(inputNet->getSigType()==dbSigType::POWER)) continue;

                        dbITerm* out= inputNet->getFirstOutput();

                        if (out==nullptr)
                                continue;

                        dbInst *faninInst= out->getInst();
                        if
(faninInst->getMaster()->getType()!=dbMasterType::CORE) continue;

                        if (mark) {
                                if (! faninInst->getUserFlag2()) {
                                        faninInst->setUserFlag2();
                                        resultTable.push_back(faninInst);
                                }
                        }
                        else if (faninInst->getUserFlag2())
                                return -1;

                        if (! faninInst->getMaster()->isSequential())
                                instsToMark.push_back(faninInst);
                }
        }
        return instsToMark.size();
}
*/
int dbBlock::markBackwardsUser2(std::vector<dbInst*>& startingInsts,
                                std::vector<dbInst*>& instsToMark,
                                bool mark,
                                std::vector<dbInst*>& resultTable)
{
  if (startingInsts.size() <= 0)
    return 0;

  std::vector<dbInst*>::iterator itr;
  for (itr = startingInsts.begin(); itr != startingInsts.end(); ++itr) {
    dbInst* inst = *itr;

    if (inst->getMaster()->isSequential())
      continue;

    if (mark) {
      ;  // inst->setUserFlag2();
      // resultTable.push_back(inst);
    } else if (inst->getUserFlag2())
      return -1;

    dbMaster* master = inst->getMaster();
    for (uint ii = 0; ii < (uint) master->getMTermCount(); ii++) {
      dbITerm* iterm = inst->getITerm(ii);
      if (!iterm->isInputSignal())
        continue;
      if (iterm->isClocked())
        continue;

      dbNet* inputNet = iterm->getNet();

      if (inputNet == nullptr)
        continue;

      if ((inputNet->getSigType() == dbSigType::GROUND)
          || (inputNet->getSigType() == dbSigType::POWER))
        continue;

      dbITerm* out = inputNet->getFirstOutput();

      if (out == nullptr)
        continue;

      dbInst* faninInst = out->getInst();
      if (faninInst->getMaster()->getType() != dbMasterType::CORE) {
        faninInst->setUserFlag2();
        continue;
      }

      if (mark) {
        if (!faninInst->getUserFlag2()) {
          faninInst->setUserFlag2();
          resultTable.push_back(faninInst);
          if (!faninInst->getMaster()->isSequential())
            instsToMark.push_back(faninInst);
        }
      } else if (faninInst->getUserFlag2()) {
        return -1;
      }
    }
  }
  return instsToMark.size();
}

int dbBlock::markBackwardsUser2(dbNet* net,
                                bool mark,
                                std::vector<dbInst*>& resultTable)
{
  std::vector<dbInst*> instsToBeMarked;

  int n = markBackwardsUser2(net, instsToBeMarked, mark, resultTable);

  if (n == 0)
    return 0;
  if (!mark && (n < 0))
    return -1;

  while (1) {
    std::vector<dbInst*> startingInsts = instsToBeMarked;
    instsToBeMarked.clear();

    int cnt
        = markBackwardsUser2(startingInsts, instsToBeMarked, mark, resultTable);
    if (cnt == 0)
      break;
    if (!mark && (cnt < 0))
      return -1;
  }
  return 0;
}

int dbBlock::markBackwardsUser2(dbNet* net,
                                std::vector<dbInst*>& instsToMark,
                                bool mark,
                                std::vector<dbInst*>& resultTable)
{
  if (net == nullptr)
    return 0;

  dbITerm* out = net->getFirstOutput();
  if (out == nullptr)
    return 0;

  dbInst* faninInst = out->getInst();
  if (mark) {
    if (!faninInst->getUserFlag2()) {
      faninInst->setUserFlag2();
      resultTable.push_back(faninInst);
    }
  } else if (faninInst->getUserFlag2())
    return -1;

  if (!faninInst->getMaster()->isSequential())
    instsToMark.push_back(faninInst);

  return instsToMark.size();
}

void dbBlock::clearUserInstFlags()
{
  dbSet<dbInst> insts = getInsts();
  dbSet<dbInst>::iterator itr;
  for (itr = insts.begin(); itr != insts.end(); ++itr) {
    dbInst* inst = *itr;

    inst->clearUserFlag2();
    inst->clearUserFlag1();
    inst->clearUserFlag3();
  }
}
void dbBlock::setDrivingItermsforNets()
{
  dbSet<dbNet> nets = getNets();
  dbSet<dbNet>::iterator nitr;

  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    dbNet* net = *nitr;
    if ((net->getSigType() == dbSigType::GROUND)
        || (net->getSigType() == dbSigType::POWER))
      continue;

    net->setDrivingITerm(0);
    dbSet<dbITerm> iterms = net->getITerms();
    dbSet<dbITerm>::iterator iitr;

    for (iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
      dbITerm* tr = *iitr;

      if (tr->getIoType() == dbIoType::OUTPUT) {
        net->setDrivingITerm(tr->getId());
        break;
      }
    }
  }
}

void dbBlock::preExttreeMergeRC(double max_cap, uint corner)
{
  if (!getExtControl()->_exttreePreMerg)
    return;
  if (max_cap == 0.0)
    max_cap = 10.0;
  if (getExtControl()->_exttreeMaxcap >= max_cap)
    return;
  getExtControl()->_exttreeMaxcap = max_cap;
  dbSet<dbNet> bnets = getNets();
  dbSet<dbNet>::iterator net_itr;
  dbNet* net;
  for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
    net = *net_itr;
    net->preExttreeMergeRC(max_cap, corner);
  }
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

  if (do_connect) {
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
    donottouchinsts.insert(remove_insts.begin(), remove_insts.end());
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

}  // namespace odb
