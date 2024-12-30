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

#include "dbJournal.h"

#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCapNode.h"
#include "dbITerm.h"
#include "dbInst.h"
#include "dbNet.h"
#include "dbRSeg.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace odb {

dbJournal::dbJournal(dbBlock* block)
    : _block(block),
      _logger(block->getImpl()->getLogger()),
      _log(_logger),
      _start_action(false),
      _action_idx(0),
      _cur_action(0)
{
}

void dbJournal::clear()
{
  _log.clear();
  _start_action = false;
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            bool prev_value,
                            bool new_value)
{
  beginAction(UPDATE_FIELD);
  _log.push(obj->getObjectType());
  _log.push(obj->getId());
  _log.push(field_id);
  _log.push(prev_value);
  _log.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            char prev_value,
                            char new_value)
{
  beginAction(UPDATE_FIELD);
  _log.push(obj->getObjectType());
  _log.push(obj->getId());
  _log.push(field_id);
  _log.push(prev_value);
  _log.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            unsigned char prev_value,
                            unsigned char new_value)
{
  beginAction(UPDATE_FIELD);
  _log.push(obj->getObjectType());
  _log.push(obj->getId());
  _log.push(field_id);
  _log.push(prev_value);
  _log.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            int prev_value,
                            int new_value)
{
  beginAction(UPDATE_FIELD);
  _log.push(obj->getObjectType());
  _log.push(obj->getId());
  _log.push(field_id);
  _log.push(prev_value);
  _log.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            unsigned int prev_value,
                            unsigned int new_value)
{
  beginAction(UPDATE_FIELD);
  _log.push(obj->getObjectType());
  _log.push(obj->getId());
  _log.push(field_id);
  _log.push(prev_value);
  _log.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            float prev_value,
                            float new_value)
{
  beginAction(UPDATE_FIELD);
  _log.push(obj->getObjectType());
  _log.push(obj->getId());
  _log.push(field_id);
  _log.push(prev_value);
  _log.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            double prev_value,
                            double new_value)
{
  beginAction(UPDATE_FIELD);
  _log.push(obj->getObjectType());
  _log.push(obj->getId());
  _log.push(field_id);
  _log.push(prev_value);
  _log.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            const char* prev_value,
                            const char* new_value)
{
  beginAction(UPDATE_FIELD);
  _log.push(obj->getObjectType());
  _log.push(obj->getId());
  _log.push(field_id);
  _log.push(prev_value);
  _log.push(new_value);
  endAction();
}

void dbJournal::beginAction(Action action)
{
  if (_start_action != false) {
    _logger->critical(
        utl::ODB, 398, "In journal, nested actions are not allowed.");
  }
  _start_action = true;
  _action_idx = _log.size();
  _log.push((unsigned char) action);
}

void dbJournal::pushParam(bool value)
{
  _log.push(value);
}

void dbJournal::pushParam(char value)
{
  _log.push(value);
}

void dbJournal::pushParam(unsigned char value)
{
  _log.push(value);
}

void dbJournal::pushParam(int value)
{
  _log.push(value);
}

void dbJournal::pushParam(unsigned int value)
{
  _log.push(value);
}

void dbJournal::pushParam(double value)
{
  _log.push(value);
}

void dbJournal::pushParam(float value)
{
  _log.push(value);
}

void dbJournal::pushParam(const char* value)
{
  _log.push(value);
}

void dbJournal::pushParam(const std::string& value)
{
  _log.push(value.c_str());
}

void dbJournal::endAction()
{
  _start_action = false;
  _log.push((unsigned char) END_ACTION);
  _log.push(_action_idx);  // This value allows log to be scanned backwards.
}

void dbJournal::redo()
{
  _log.begin();

  while (!_log.end()) {
    uint s = _log.idx();
    _log.pop(_cur_action);

    switch (_cur_action) {
      case CREATE_OBJECT:
        redo_createObject();
        break;

      case DELETE_OBJECT:
        redo_deleteObject();
        break;

      case CONNECT_OBJECT:
        redo_connectObject();
        break;

      case DISCONNECT_OBJECT:
        redo_disconnectObject();
        break;

      case SWAP_OBJECT:
        redo_swapObject();
        break;

      case UPDATE_FIELD:
        redo_updateField();
        break;

      case END_ACTION:
        _logger->critical(utl::ODB, 399, "In undo saw unexpected END_ACTION.");
        break;
    }

    unsigned char end_action;
    unsigned int action_idx;
    _log.pop(end_action);
    _log.pop(action_idx);
    if (end_action != END_ACTION || action_idx != s) {
      _logger->critical(utl::ODB, 419, "In undo, expected END_ACTION.");
    }
  }
}

void dbJournal::redo_createObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbNetObj: {
      std::string name;
      uint net_id;
      _log.pop(name);
      _log.pop(net_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: create dbNet {}",
                 name.c_str());
      dbNet::create(_block, name.c_str());
      break;
    }

    case dbBTermObj: {
      uint dbNet_id;
      std::string name;
      _log.pop(dbNet_id);
      _log.pop(name);

      dbNet* net = dbNet::getNet(_block, dbNet_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: create dbBTermObj {}, Net: {}",
                 name,
                 dbNet_id);
      dbBTerm::create(net, name.c_str());
      break;
    }

    case dbInstObj: {
      uint lib_id;
      uint master_id;
      uint inst_id;
      std::string name;
      _log.pop(lib_id);
      _log.pop(master_id);
      _log.pop(name);
      _log.pop(inst_id);
      dbLib* lib = dbLib::getLib(_block->getDb(), lib_id);
      dbMaster* master = dbMaster::getMaster(lib, master_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: create dbInstObj {}, master: {}, lib: {}",
                 name,
                 master_id,
                 lib_id);
      dbInst::create(_block, master, name.c_str());
      break;
    }

    case dbRSegObj: {
      uint net_id;
      int x;
      int y;
      uint path_dir;
      bool allocate_cap;
      _log.pop(net_id);
      _log.pop(x);
      _log.pop(y);
      _log.pop(path_dir);
      _log.pop(allocate_cap);
      dbNet* net = dbNet::getNet(_block, net_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: create dbRSeg, net {}, x {}, y {}, path_dir {}, "
                 "alloc_cap {}",
                 net_id,
                 x,
                 y,
                 path_dir,
                 allocate_cap);
      dbRSeg::create(net, x, y, path_dir, allocate_cap);
      break;
    }

    case dbCapNodeObj: {
      uint net_id;
      uint node;
      bool foreign;
      _log.pop(net_id);
      _log.pop(node);
      _log.pop(foreign);
      dbNet* net = dbNet::getNet(_block, net_id);
      debugPrint(
          _logger,
          utl::ODB,
          "DB_ECO",
          2,
          "REDO ECO: create dbCapNodeObj, net_id {}, node {}, foreign {}",
          net_id,
          node,
          foreign);
      dbCapNode::create(net, node, foreign);
      break;
    }

    case dbCCSegObj: {
      uint nodeA, nodeB;
      bool merge;
      _log.pop(nodeA);
      _log.pop(nodeB);
      _log.pop(merge);
      dbCapNode* a = dbCapNode::getCapNode(_block, nodeA);
      dbCapNode* b = dbCapNode::getCapNode(_block, nodeB);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 1,
                 "REDO: dbCCSeg::create, nodeA = {}, nodeB = {}, merge = {}",
                 a->getId(),
                 b->getId(),
                 merge);
      dbCCSeg::create(a, b, merge);
    }

    default:
      break;
  }
}

void dbJournal::redo_deleteObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbNetObj: {
      std::string name;
      uint net_id;
      uint flags;
      uint ndr_id;
      _log.pop(name);
      _log.pop(net_id);
      _log.pop(flags);
      _log.pop(ndr_id);
      dbNet* net = dbNet::getNet(_block, net_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: destroy dbNet, net_id {}",
                 net_id);
      dbNet::destroy(net);
      break;
    }

    case dbBTermObj: {
      uint bterm_id;
      _log.pop(bterm_id);
      dbBTerm* bterm = dbBTerm::getBTerm(_block, bterm_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: destroy dbBTerm, bterm_id {}",
                 bterm_id);
      dbBTerm::destroy(bterm);
      break;
    }
    case dbInstObj: {
      uint lib_id;
      uint master_id;
      uint inst_id;
      uint flags;
      std::string name;
      int x;
      int y;
      uint group_id;
      uint module_id;
      uint region_id;
      _log.pop(lib_id);
      _log.pop(master_id);
      _log.pop(name);
      _log.pop(inst_id);
      _log.pop(flags);
      _log.pop(x);
      _log.pop(y);
      _log.pop(group_id);
      _log.pop(module_id);
      _log.pop(region_id);
      dbInst* inst = dbInst::getInst(_block, inst_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: destroy dbInst, inst_id {}",
                 inst_id);
      dbInst::destroy(inst);
      break;
    }

    case dbRSegObj: {
      uint rseg_id;
      _log.pop(rseg_id);
      dbRSeg* rseg = dbRSeg::getRSeg(_block, rseg_id);
      uint net_id;
      _log.pop(net_id);
      if (net_id) {
        dbNet* net = dbNet::getNet(_block, net_id);
        debugPrint(_logger,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: destroy dbRSegObj, rseg_id {}, net_id {}",
                   rseg_id,
                   net_id);
        dbRSeg::destroy(rseg, net);
      } else {
        debugPrint(_logger,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: simple destroy dbRSegObj, rseg_id {}, net_id {}",
                   rseg_id,
                   net_id);
        dbRSeg::destroyS(rseg);
      }
      break;
    }

    case dbCapNodeObj: {
      uint node_id;
      _log.pop(node_id);
      dbCapNode* node = dbCapNode::getCapNode(_block, node_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: destroy dbCapNode, node_id {}",
                 node_id);
      dbCapNode::destroy(node);
      break;
    }

    case dbCCSegObj: {
      uint seg_id;
      _log.pop(seg_id);
      uint regular;
      _log.pop(regular);
      dbCCSeg* seg = dbCCSeg::getCCSeg(_block, seg_id);
      if (regular) {
        debugPrint(_logger,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: destroy dbCCSeg, inst_id {}",
                   seg_id);
        dbCCSeg::destroy(seg);
      } else {
        debugPrint(_logger,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: simple destroy dbCCSeg, inst_id {}",
                   seg_id);
        dbCCSeg::destroyS(seg);
      }
      break;
    }
    default:
      break;
  }
}

void dbJournal::redo_connectObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbITermObj: {
      uint iterm_id;
      _log.pop(iterm_id);
      dbITerm* iterm = dbITerm::getITerm(_block, iterm_id);
      uint net_id;
      _log.pop(net_id);
      if (net_id != 0) {
        dbNet* net = dbNet::getNet(_block, net_id);
        debugPrint(_logger,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: connect dbITermObj, iterm_id {}, net_id {}",
                   iterm_id,
                   net_id);
        iterm->connect(net);
      }
      uint mod_net_id;
      _log.pop(mod_net_id);
      if (mod_net_id != 0) {
        dbModNet* mod_net = dbModNet::getModNet(_block, mod_net_id);
        debugPrint(_logger,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: connect dbITermObj, iterm_id {}, mod_net_id {}",
                   iterm_id,
                   mod_net_id);
        iterm->connect(mod_net);
      }
      break;
    }

    case dbBTermObj: {
      uint bterm_id;
      _log.pop(bterm_id);
      dbBTerm* bterm = dbBTerm::getBTerm(_block, bterm_id);
      uint net_id;
      _log.pop(net_id);
      dbNet* net = dbNet::getNet(_block, net_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: connect dbBTermObj, bterm_id {}, net_id {}",
                 bterm_id,
                 net_id);
      bterm->connect(net);
      break;
    }
    default:
      break;
  }
}

void dbJournal::redo_disconnectObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbITermObj: {
      uint iterm_id;
      _log.pop(iterm_id);
      dbITerm* iterm = dbITerm::getITerm(_block, iterm_id);
      uint net_id;
      _log.pop(net_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: disconnect dbITermObj, iterm_id {}",
                 iterm_id);
      // note: this will disconnect the modnet and the dbNet
      iterm->disconnect();
      break;
    }

    case dbBTermObj: {
      uint bterm_id;
      _log.pop(bterm_id);
      dbBTerm* bterm = dbBTerm::getBTerm(_block, bterm_id);
      uint net_id;
      _log.pop(net_id);
      bterm->disconnect();

      break;
    }

    default:
      break;
  }
}

void dbJournal::redo_swapObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbInstObj: {
      uint inst_id;
      _log.pop(inst_id);
      dbInst* inst = dbInst::getInst(_block, inst_id);

      uint prev_lib_id;
      _log.pop(prev_lib_id);

      uint prev_master_id;
      _log.pop(prev_master_id);

      uint lib_id;
      _log.pop(lib_id);
      dbLib* lib = dbLib::getLib(_block->getDb(), lib_id);

      uint master_id;
      _log.pop(master_id);
      dbMaster* master = dbMaster::getMaster(lib, master_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "ECO: swapMaster inst {}, prev lib/master: {}/{}, new "
                 "lib/master: {}/{}",
                 inst_id,
                 prev_lib_id,
                 prev_master_id,
                 lib_id,
                 master_id);
      inst->swapMaster(master);
      break;
    }

    default:
      break;
  }
}

void dbJournal::redo_updateField()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbBlockObj:
      redo_updateBlockField();
      break;

    case dbNetObj:
      redo_updateNetField();
      break;

    case dbInstObj:
      redo_updateInstField();
      break;

    case dbBTermObj:
      redo_updateBTermField();
      break;

    case dbITermObj:
      redo_updateITermField();
      break;

    case dbRSegObj:
      redo_updateRSegField();
      break;

    case dbCCSegObj:
      redo_updateCCSegField();
      break;

    case dbCapNodeObj:
      redo_updateCapNodeField();
      break;

    default:
      break;
  }
}

void dbJournal::redo_updateBlockField()
{
  uint block_id;
  _log.pop(block_id);
  int field;
  _log.pop(field);
  switch ((_dbBlock::Field) field) {
    case _dbBlock::CORNERCOUNT: {
      int cornerCount;
      _log.pop(cornerCount);
      int extDbCount;
      _log.pop(extDbCount);
      std::string name;
      _log.pop(name);
      debugPrint(
          _logger,
          utl::ODB,
          "DB_ECO",
          2,
          "REDO ECO: dbBlock {}, setCornerCount cornerCount {}, extDbCount "
          "{}, name_list {}",
          block_id,
          cornerCount,
          extDbCount,
          name.c_str());
      _block->setCornerCount(cornerCount, extDbCount, name.c_str());
      break;
    }

    case _dbBlock::WRITEDB: {
      std::string name;
      _log.pop(name);
      int allNode;
      _log.pop(allNode);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbBlock {}, writeDb, filename {}, allNode {}",
                 block_id,
                 name.c_str(),
                 allNode);
      _block->writeDb((char*) name.c_str(), allNode);
      break;
    }

    default:
      break;
  }
}

void dbJournal::redo_updateNetField()
{
  uint net_id;
  _log.pop(net_id);
  _dbNet* net = (_dbNet*) dbNet::getNet(_block, net_id);

  int field;
  _log.pop(field);

  switch ((_dbNet::Field) field) {
    case _dbNet::FLAGS: {
      uint prev_flags;
      _log.pop(prev_flags);
      uint* flags = (uint*) &net->_flags;
      _log.pop(*flags);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbNetObj {}, updateNetField: {} to {}",
                 net_id,
                 prev_flags,
                 *flags);
      break;
    }

    case _dbNet::NON_DEFAULT_RULE: {
      uint prev_rule;
      _log.pop(prev_rule);
      _log.pop(net->_non_default_rule.id());
      bool prev_block_rule;
      bool cur_block_rule;
      _log.pop(prev_block_rule);
      _log.pop(cur_block_rule);
      net->_flags._block_rule = cur_block_rule;
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbNetObj {}, updateNonDefaultRule: {} to {}",
                 net_id,
                 prev_rule,
                 net->_non_default_rule.id());
      break;
    }

    case _dbNet::TERM_EXTID: {
      int capId;
      _log.pop(capId);
      ((dbNet*) net)->setTermExtIds(capId);
      if (capId) {
        debugPrint(_logger,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: dbNetObj {} set term extId",
                   net_id);
      } else
        debugPrint(_logger,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: dbNetObj {} reset term extId",
                   net_id);
      break;
    }

    case _dbNet::HEAD_RSEG: {
      uint pid;
      uint rid;
      _log.pop(pid);
      _log.pop(rid);
      ((dbNet*) net)->set1stRSegId(rid);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbNetObj {}, set1stRSegId {} to {}",
                 net_id,
                 pid,
                 rid);
      break;
    }

    case _dbNet::REVERSE_RSEG: {
      dbSet<dbRSeg> rSet = ((dbNet*) net)->getRSegs();
      rSet.reverse();
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbNetObj {}, reverse rsegs sequence",
                 net_id);
      break;
    }

    case _dbNet::HEAD_CAPNODE: {
      uint pid;
      uint cid;
      _log.pop(pid);
      _log.pop(cid);
      ((dbNet*) net)->set1stCapNodeId(cid);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbNetObj {}, set1stRSegId {} to {}",
                 net_id,
                 pid,
                 cid);
      break;
    }

    default:
      break;
  }
}

void dbJournal::redo_updateInstField()
{
  uint inst_id;
  _log.pop(inst_id);
  _dbInst* inst = (_dbInst*) dbInst::getInst(_block, inst_id);

  int field;
  _log.pop(field);

  switch ((_dbInst::Field) field) {
    case _dbInst::FLAGS: {
      uint prev_flags;
      _log.pop(prev_flags);
      uint* flags = (uint*) &inst->_flags;
      _log.pop(*flags);

      // Changing the orientation flag requires updating the cached bbox
      _dbInstFlags* a = (_dbInstFlags*) flags;
      _dbInstFlags* b = (_dbInstFlags*) &prev_flags;
      if (a->_orient != b->_orient) {
        _dbInst::setInstBBox(inst);
      }

      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbInst {}, updateInstField: {} to {}",
                 inst_id,
                 prev_flags,
                 *flags);
      break;
    }

    case _dbInst::ORIGIN: {
      int prev_x;
      _log.pop(prev_x);
      int prev_y;
      _log.pop(prev_y);
      int current_x;
      _log.pop(current_x);
      int current_y;
      _log.pop(current_y);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbInst {}, origin: {},{} to {},{}",
                 inst_id,
                 prev_x,
                 prev_y,
                 inst->_x,
                 inst->_y);
      ((dbInst*) inst)->setOrigin(current_x, current_y);
      break;
    }

    default:
      break;
  }
}

void dbJournal::redo_updateBTermField()
{
  uint bterm_id;
  _log.pop(bterm_id);
  _dbBTerm* bterm = (_dbBTerm*) dbBTerm::getBTerm(_block, bterm_id);

  int field;
  _log.pop(field);

  switch ((_dbBTerm::Field) field) {
    case _dbBTerm::FLAGS: {
      uint prev_flags;
      _log.pop(prev_flags);
      uint* flags = (uint*) &bterm->_flags;
      _log.pop(*flags);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbBTerm {}, updateBTermField: {} to {}",
                 bterm_id,
                 prev_flags,
                 *flags);
      break;
    }
  }
}
void dbJournal::redo_updateITermField()
{
  uint iterm_id;
  _log.pop(iterm_id);
  _dbITerm* iterm = (_dbITerm*) dbITerm::getITerm(_block, iterm_id);

  int field;
  _log.pop(field);

  switch ((_dbITerm::Field) field) {
    case _dbITerm::FLAGS: {
      uint prev_flags;
      _log.pop(prev_flags);
      uint* flags = (uint*) &iterm->_flags;
      _log.pop(*flags);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbITerm {}, updateITermField: {} to {}",
                 iterm_id,
                 prev_flags,
                 *flags);
      break;
    }
  }
}

void dbJournal::redo_updateRSegField()
{
  uint rseg_id;
  _log.pop(rseg_id);
  _dbRSeg* rseg = (_dbRSeg*) dbRSeg::getRSeg(_block, rseg_id);

  int field;
  _log.pop(field);

  switch ((_dbRSeg::Field) field) {
    case _dbRSeg::FLAGS: {
      uint prev_flags;
      _log.pop(prev_flags);
      uint* flags = (uint*) &rseg->_flags;
      _log.pop(*flags);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbRSeg {}, updateRSegField: {} to {}",
                 rseg_id,
                 prev_flags,
                 *flags);
      break;
    }

    case _dbRSeg::SOURCE: {
      uint prev_source;
      _log.pop(prev_source);
      _log.pop(rseg->_source);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbRSeg {}, updateSource: {} to {}",
                 rseg_id,
                 prev_source,
                 rseg->_source);
      break;
    }

    case _dbRSeg::TARGET: {
      uint prev_target;
      _log.pop(prev_target);
      _log.pop(rseg->_target);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbRSeg {}, updateTarget: {} to {}",
                 rseg_id,
                 prev_target,
                 rseg->_target);
      break;
    }

    case _dbRSeg::RESISTANCE: {
      float prev_r;
      float r;
      int cnr;
      _log.pop(prev_r);
      _log.pop(r);
      _log.pop(cnr);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbRSeg {}, updateResistance: {} to {},{}",
                 rseg_id,
                 prev_r,
                 r,
                 cnr);
      ((dbRSeg*) rseg)->setResistance(r, cnr);
      break;
    }

    case _dbRSeg::CAPACITANCE: {
      float prev_c;
      float c;
      int cnr;
      _log.pop(prev_c);
      _log.pop(c);
      _log.pop(cnr);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbRSeg {}, updateCapacitance: {} to {},{}",
                 rseg_id,
                 prev_c,
                 c,
                 cnr);
      ((dbRSeg*) rseg)->setCapacitance(c, cnr);
      break;
    }

    case _dbRSeg::ADDRSEGCAPACITANCE: {
      uint oseg_id;
      _log.pop(oseg_id);
      _dbRSeg* other = (_dbRSeg*) dbRSeg::getRSeg(_block, oseg_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbRSeg {}, other dbRSeg {}, addCcCapacitance",
                 rseg_id,
                 oseg_id);
      ((dbRSeg*) rseg)->addRSegCapacitance((dbRSeg*) other);
      break;
    }

    case _dbRSeg::ADDRSEGRESISTANCE: {
      uint oseg_id;
      _log.pop(oseg_id);
      _dbRSeg* other = (_dbRSeg*) dbRSeg::getRSeg(_block, oseg_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbRSeg {}, other dbRSeg {}, addCcCapacitance",
                 rseg_id,
                 oseg_id);
      ((dbRSeg*) rseg)->addRSegResistance((dbRSeg*) other);
      break;
    }

    case _dbRSeg::COORDINATES: {
      int prev_x;
      int prev_y;
      int x;
      int y;
      _log.pop(prev_x);
      _log.pop(x);
      _log.pop(prev_y);
      _log.pop(y);
      debugPrint(
          _logger,
          utl::ODB,
          "DB_ECO",
          2,
          "REDO ECO: dbRSeg {}, updateCoordinates x: {} to {}, y: {} to {}",
          rseg_id,
          prev_x,
          x,
          prev_y,
          y);
      ((dbRSeg*) rseg)->setCoords(x, y);
      break;
    }
  }
}

void dbJournal::redo_updateCapNodeField()
{
  uint node_id;
  _log.pop(node_id);
  _dbCapNode* node = (_dbCapNode*) dbCapNode::getCapNode(_block, node_id);

  int field;
  _log.pop(field);

  switch ((_dbCapNode::Fields) field) {
    case _dbCapNode::FLAGS: {
      uint prev_flags;
      _log.pop(prev_flags);
      uint* flags = (uint*) &node->_flags;
      _log.pop(*flags);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCapNode {}, updateFlags: {} to {}",
                 node_id,
                 prev_flags,
                 *flags);
      break;
    }

    case _dbCapNode::NODE_NUM: {
      uint prev_num;
      _log.pop(prev_num);
      _log.pop(node->_node_num);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCapNode {}, updateNodeNum: {} to {}",
                 node_id,
                 prev_num,
                 node->_node_num);
      break;
    }

    case _dbCapNode::CAPACITANCE: {
      float prev_c;
      float c;
      int cnr;
      _log.pop(prev_c);
      _log.pop(c);
      _log.pop(cnr);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCapNode {}, updateCapacitance: {} to {},{}",
                 node_id,
                 prev_c,
                 c,
                 cnr);
      ((dbCapNode*) node)->setCapacitance(c, cnr);
      break;
    }
    case _dbCapNode::ADDCAPNCAPACITANCE: {
      uint oseg_id;
      _log.pop(oseg_id);
      _dbCapNode* other = (_dbCapNode*) dbCapNode::getCapNode(_block, oseg_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCapNode {}, other dbCapNode {}, addCcCapacitance",
                 node_id,
                 oseg_id);
      ((dbCapNode*) node)->addCapnCapacitance((dbCapNode*) other);
      break;
    }
    case _dbCapNode::SETNET: {
      uint onet_id;
      _log.pop(onet_id);
      uint nnet_id;
      _log.pop(nnet_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCapNode {}, set net {}",
                 node_id,
                 nnet_id);
      ((dbCapNode*) node)->setNet(nnet_id);
      break;
    }
    case _dbCapNode::SETNEXT: {
      uint onext;
      _log.pop(onext);
      uint nnext;
      _log.pop(nnext);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCapNode {}, set next {}",
                 node_id,
                 nnext);
      ((dbCapNode*) node)->setNext(nnext);
      break;
    }
  }
}

void dbJournal::redo_updateCCSegField()
{
  uint seg_id;
  _log.pop(seg_id);
  _dbCCSeg* seg = (_dbCCSeg*) dbCCSeg::getCCSeg(_block, seg_id);

  int field;
  _log.pop(field);

  switch ((_dbCCSeg::Fields) field) {
    case _dbCCSeg::FLAGS: {
      uint prev_flags;
      _log.pop(prev_flags);
      uint* flags = (uint*) &seg->_flags;
      _log.pop(*flags);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCCSeg {}, updateFlags: {} to {}",
                 seg_id,
                 prev_flags,
                 *flags);
      break;
    }

    case _dbCCSeg::CAPACITANCE: {
      float prev_c;
      float c;
      int cnr;
      _log.pop(prev_c);
      _log.pop(c);
      _log.pop(cnr);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCCSeg {}, updateCapacitance: {} to {},{}",
                 seg_id,
                 prev_c,
                 c,
                 cnr);
      ((dbCCSeg*) seg)->setCapacitance(c, cnr);
      break;
    }

    case _dbCCSeg::ADDCCCAPACITANCE: {
      uint oseg_id;
      _log.pop(oseg_id);
      _dbCCSeg* other = (_dbCCSeg*) dbCCSeg::getCCSeg(_block, oseg_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCCSeg {}, other dbCCSeg {}, addCcCapacitance",
                 seg_id,
                 oseg_id);
      ((dbCCSeg*) seg)->addCcCapacitance((dbCCSeg*) other);
      break;
    }

    case _dbCCSeg::SWAPCAPNODE: {
      uint ocap_id;
      _log.pop(ocap_id);
      _dbCapNode* orig = (_dbCapNode*) dbCapNode::getCapNode(_block, ocap_id);
      uint ncap_id;
      _log.pop(ncap_id);
      _dbCapNode* newn = (_dbCapNode*) dbCapNode::getCapNode(_block, ncap_id);
      debugPrint(
          _logger,
          utl::ODB,
          "DB_ECO",
          2,
          "REDO ECO: dbCCSeg {}, origCapNode {}, newCapNode {}, swapCapnode",
          seg->getOID(),
          ocap_id,
          ncap_id);
      ((dbCCSeg*) seg)->swapCapnode((dbCapNode*) orig, (dbCapNode*) newn);
      break;
    }
    case _dbCCSeg::LINKCCSEG: {
      uint cap_id;
      _log.pop(cap_id);
      uint cseq;
      _log.pop(cseq);
      dbCapNode* capn = (dbCapNode*) dbCapNode::getCapNode(_block, cap_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCCSeg {}}, Link_cc_seg, capNode {}, cseq {}",
                 seg->getOID(),
                 cap_id,
                 cseq);
      ((dbCCSeg*) seg)->Link_cc_seg(capn, cseq);
      break;
    }
    case _dbCCSeg::UNLINKCCSEG: {
      uint cap_id;
      _log.pop(cap_id);
      dbCapNode* capn = (dbCapNode*) dbCapNode::getCapNode(_block, cap_id);
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCCSeg {}, unLink_cc_seg, capNode {}",
                 seg->getOID(),
                 cap_id);
      ((dbCCSeg*) seg)->unLink_cc_seg(capn);
      break;
    }
    case _dbCCSeg::SETALLCCCAP: {
      uint cornerCnt = _block->getCornerCount();
      double ttcap[ADS_MAX_CORNER];
      char ccCaps[400];
      ccCaps[0] = '\0';
      int pos = 0;
      for (uint ii = 0; ii < cornerCnt; ii++) {
        _log.pop(ttcap[ii]);
        pos += sprintf(&ccCaps[pos], "%f ", ttcap[ii]);
      }
      debugPrint(_logger,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCCSeg {}, setAllCcCap, caps: {}",
                 seg_id,
                 &ccCaps[0]);
      ((dbCCSeg*) seg)->setAllCcCap(&ttcap[0]);
      break;
    }
  }
}

dbObjectType dbJournal::popObjectType()
{
  int obj_type_value;
  _log.pop(obj_type_value);
  return static_cast<dbObjectType>(obj_type_value);
}

//
// WORK-IN-PROGRESS undo does not yet work.
//
void dbJournal::undo()
{
  if (_log.empty()) {
    return;
  }

  _log.moveToEnd();
  _log.moveBackOneInt();

  for (;;) {
    uint action_idx;
    _log.pop(action_idx);
    _log.set(action_idx);
    _log.pop(_cur_action);

    switch (_cur_action) {
      case CREATE_OBJECT:
        undo_createObject();
        break;

      case DELETE_OBJECT:
        undo_deleteObject();
        break;

      case CONNECT_OBJECT:
        undo_connectObject();
        break;

      case DISCONNECT_OBJECT:
        undo_disconnectObject();
        break;

      case SWAP_OBJECT:
        undo_swapObject();
        break;

      case UPDATE_FIELD:
        undo_updateField();
        break;

      case END_ACTION:
        break;
    }

    if (action_idx == 0) {
      break;
    }

    _log.set(action_idx);
    _log.moveBackOneInt();
  }
}

void dbJournal::undo_createObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbGuideObj: {
      uint guide_id;
      _log.pop(guide_id);
      dbGuide* guide = dbGuide::getGuide(_block, guide_id);
      dbGuide::destroy(guide);
      break;
    }
    case dbInstObj: {
      uint lib_id;
      uint master_id;
      uint inst_id;
      std::string name;
      _log.pop(lib_id);
      _log.pop(master_id);
      _log.pop(name);
      _log.pop(inst_id);
      dbInst* inst = dbInst::getInst(_block, inst_id);
      dbInst::destroy(inst);
      break;
    }

    case dbNetObj: {
      std::string name;
      uint net_id;
      _log.pop(name);
      _log.pop(net_id);
      dbNet* net = dbNet::getNet(_block, net_id);
      dbNet::destroy(net);
      break;
    }

    case dbModBTermObj: {
      uint modbterm_id;
      _log.pop(modbterm_id);
      dbModBTerm* modbterm = dbModBTerm::getModBTerm(_block, modbterm_id);
      dbModBTerm::destroy(modbterm);
      break;
    }

    case dbModITermObj: {
      uint moditerm_id;
      _log.pop(moditerm_id);
      dbModITerm* moditerm = dbModITerm::getModITerm(_block, moditerm_id);
      dbModITerm::destroy(moditerm);
      break;
    }

    default: {
      _logger->critical(utl::ODB,
                        441,
                        "No undo_createObject support for type {}",
                        dbObject::getTypeName(obj_type));
      break;
    }
  }
}

void dbJournal::undo_deleteObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbGuideObj: {
      uint net_id;
      int x_min;
      int y_min;
      int x_max;
      int y_max;
      uint layer_id;
      uint via_layer_id;
      bool is_congested;
      _log.pop(net_id);
      _log.pop(x_min);
      _log.pop(y_min);
      _log.pop(x_max);
      _log.pop(y_max);
      _log.pop(layer_id);
      _log.pop(via_layer_id);
      _log.pop(is_congested);
      auto net = dbNet::getNet(_block, net_id);
      auto layer = dbTechLayer::getTechLayer(_block->getTech(), layer_id);
      auto via_layer
          = dbTechLayer::getTechLayer(_block->getTech(), via_layer_id);
      dbGuide::create(
          net, layer, via_layer, {x_min, y_min, x_max, y_max}, is_congested);
      break;
    }
    case dbInstObj: {
      uint lib_id;
      uint master_id;
      uint inst_id;
      std::string name;
      int x;
      int y;
      uint group_id;
      uint module_id;
      uint region_id;
      _log.pop(lib_id);
      _log.pop(master_id);
      _log.pop(name);
      _log.pop(inst_id);
      dbLib* lib = dbLib::getLib(_block->getDb(), lib_id);
      dbMaster* master = dbMaster::getMaster(lib, master_id);
      auto inst = dbInst::create(_block, master, name.c_str());
      _dbInst* impl = (_dbInst*) inst;
      uint* flags = (uint*) &impl->_flags;
      _log.pop(*flags);
      _log.pop(x);
      _log.pop(y);
      _log.pop(group_id);
      _log.pop(module_id);
      _log.pop(region_id);
      inst->setLocation(x, y);
      if (group_id != 0) {
        auto group = dbGroup::getGroup(_block, group_id);
        group->addInst(inst);
      }
      if (module_id != 0) {
        auto module = dbModule::getModule(_block, module_id);
        module->addInst(inst);
      }
      if (region_id != 0) {
        auto region = dbRegion::getRegion(_block, region_id);
        region->addInst(inst);
      }
      break;
    }
    case dbNetObj: {
      std::string name;
      uint net_id;
      uint ndr_id;
      _log.pop(name);
      _log.pop(net_id);
      auto net = dbNet::create(_block, name.c_str());
      _dbNet* impl = (_dbNet*) net;
      uint* flags = (uint*) &impl->_flags;
      _log.pop(*flags);
      _log.pop(ndr_id);
      _dbNet* net_impl = (_dbNet*) net;
      net_impl->_non_default_rule = ndr_id;
      break;
    }
    default: {
      _logger->critical(utl::ODB,
                        417,
                        "No undo_deleteObject support for type {}",
                        dbObject::getTypeName(obj_type));
      break;
    }
  }
}

void dbJournal::undo_connectObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbModITermObj: {
      uint moditerm_id;
      _log.pop(moditerm_id);
      dbModITerm* moditerm = dbModITerm::getModITerm(_block, moditerm_id);
      uint net_id;
      _log.pop(net_id);
      moditerm->disconnect();
      break;
    }

    case dbModBTermObj: {
      uint modbterm_id;
      _log.pop(modbterm_id);
      dbModBTerm* modbterm = dbModBTerm::getModBTerm(_block, modbterm_id);
      uint net_id;
      _log.pop(net_id);
      modbterm->disconnect();
      break;
    }

    case dbITermObj: {
      uint iterm_id;
      _log.pop(iterm_id);
      dbITerm* iterm = dbITerm::getITerm(_block, iterm_id);
      uint net_id;
      _log.pop(net_id);
      // disconnects everything: modnet and dbnet
      iterm->disconnect();
      break;
    }

    case dbBTermObj: {
      uint bterm_id;
      _log.pop(bterm_id);
      dbBTerm* bterm = dbBTerm::getBTerm(_block, bterm_id);
      uint net_id;
      _log.pop(net_id);
      bterm->disconnect();
      break;
    }

    default: {
      _logger->critical(utl::ODB,
                        442,
                        "No undo_connectObject support for type {}",
                        dbObject::getTypeName(obj_type));
      break;
    }
  }
}

void dbJournal::undo_disconnectObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbITermObj: {
      uint iterm_id;
      _log.pop(iterm_id);
      dbITerm* iterm = dbITerm::getITerm(_block, iterm_id);
      uint net_id;
      _log.pop(net_id);
      if (net_id != 0) {
        dbNet* net = dbNet::getNet(_block, net_id);
        iterm->connect(net);
      }
      uint mnet_id;
      _log.pop(mnet_id);
      if (mnet_id != 0) {
        dbModNet* mod_net = dbModNet::getModNet(_block, mnet_id);
        iterm->connect(mod_net);
      }

      break;
    }

    case dbBTermObj: {
      uint bterm_id;
      _log.pop(bterm_id);
      dbBTerm* bterm = dbBTerm::getBTerm(_block, bterm_id);
      uint net_id;
      _log.pop(net_id);
      dbNet* net = dbNet::getNet(_block, net_id);
      bterm->connect(net);
      uint mnet_id;
      _log.pop(mnet_id);
      if (mnet_id != 0) {
        dbModNet* mnet = dbModNet::getModNet(_block, mnet_id);
        bterm->connect(mnet);
      }
      break;
    }
    default: {
      _logger->critical(utl::ODB,
                        443,
                        "No undo_disconnectObject support for type {}",
                        dbObject::getTypeName(obj_type));
      break;
    }
  }
}

void dbJournal::undo_swapObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbNameObj: {
      // name swapping undo
      auto sub_obj_type = popObjectType();
      switch (sub_obj_type) {
        // net name swap
        case dbNetObj: {
          // assume source and destination
          uint source_net_id;
          uint dest_net_id;
          _log.pop(source_net_id);
          _log.pop(dest_net_id);
          // note because we are undoing the source is the prior dest
          //(we are move dest name back to source name).
          dbNet* source_net = dbNet::getNet(_block, dest_net_id);
          dbNet* dest_net = dbNet::getNet(_block, source_net_id);
          // don't allow undo to be undone, turn off journaling doing swap
          source_net->swapNetNames(dest_net, false);
          break;
        }
        default: {
          _logger->critical(utl::ODB,
                            467,
                            "No undo_swapObject Name support for type {}",
                            dbObject::getTypeName(sub_obj_type));
        }
      }
    }
    case dbInstObj: {
      uint inst_id;
      _log.pop(inst_id);

      uint prev_lib_id;
      _log.pop(prev_lib_id);

      uint prev_master_id;
      _log.pop(prev_master_id);

      uint lib_id;
      _log.pop(lib_id);

      uint master_id;
      _log.pop(master_id);

      dbInst* inst = dbInst::getInst(_block, inst_id);
      dbLib* lib = dbLib::getLib(_block->getDb(), prev_lib_id);
      dbMaster* master = dbMaster::getMaster(lib, prev_master_id);
      inst->swapMaster(master);
      break;
    }

    default: {
      _logger->critical(utl::ODB,
                        444,
                        "No undo_swapObject support for type {}",
                        dbObject::getTypeName(obj_type));
      break;
    }
  }
}

void dbJournal::undo_updateField()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbNetObj:
      undo_updateNetField();
      break;

    case dbInstObj:
      undo_updateInstField();
      break;

    case dbITermObj:
      undo_updateITermField();
      break;

    case dbRSegObj:
      undo_updateRSegField();
      break;

    case dbCapNodeObj:
      undo_updateCapNodeField();
      break;

    default: {
      _logger->critical(utl::ODB,
                        445,
                        "No undo_updateField support for type {}",
                        dbObject::getTypeName(obj_type));
      break;
    }
  }
}

void dbJournal::undo_updateNetField()
{
  uint net_id;
  _log.pop(net_id);
  _dbNet* net = (_dbNet*) dbNet::getNet(_block, net_id);

  int field;
  _log.pop(field);

  switch ((_dbNet::Field) field) {
    case _dbNet::FLAGS: {
      uint* flags = (uint*) &net->_flags;
      _log.pop(*flags);
      uint new_flags;
      _log.pop(new_flags);
      break;
    }
    default: {
      _logger->critical(
          utl::ODB, 408, "No undo_updateNetField support for field {}", field);
      break;
    }
  }
}

void dbJournal::undo_updateInstField()
{
  uint inst_id;
  _log.pop(inst_id);
  _dbInst* inst = (_dbInst*) dbInst::getInst(_block, inst_id);

  int field;
  _log.pop(field);

  switch ((_dbInst::Field) field) {
    case _dbInst::FLAGS: {
      uint* flags = (uint*) &inst->_flags;
      _log.pop(*flags);
      uint new_flags;
      _log.pop(new_flags);

      // Changing the orientation flag requires updating the cached bbox
      _dbInstFlags* a = (_dbInstFlags*) flags;
      _dbInstFlags* b = (_dbInstFlags*) &new_flags;
      if (a->_orient != b->_orient) {
        _dbInst::setInstBBox(inst);
      }
      break;
    }

    case _dbInst::ORIGIN: {
      int prev_x;
      _log.pop(prev_x);
      int prev_y;
      _log.pop(prev_y);
      int current_x;
      _log.pop(current_x);
      int current_y;
      _log.pop(current_y);
      ((dbInst*) inst)->setOrigin(prev_x, prev_y);
      break;
    }
    default: {
      _logger->critical(
          utl::ODB, 409, "No undo_updateInstField support for field {}", field);
      break;
    }
  }
}

void dbJournal::undo_updateITermField()
{
  uint iterm_id;
  _log.pop(iterm_id);
  //_dbITerm * iterm = (_dbITerm *) dbITerm::getITerm(_block, iterm_id );

  int field;
  _log.pop(field);

  switch ((_dbITerm::Field) field) {
    default: {
      _logger->critical(utl::ODB,
                        410,
                        "No undo_updateITermField support for field {}",
                        field);
      break;
    }
  }
}

void dbJournal::undo_updateRSegField()
{
  uint rseg_id;
  _log.pop(rseg_id);
  //_dbRSeg * rseg = (_dbRSeg *) dbRSeg::getRSeg(_block, rseg_id );

  int field;
  _log.pop(field);

  switch ((_dbRSeg::Field) field) {
    default: {
      _logger->critical(
          utl::ODB, 411, "No undo_updateRSegField support for field {}", field);
    } break;
  }
}

void dbJournal::undo_updateCapNodeField()
{
  uint node_id;
  _log.pop(node_id);
  //_dbCapNode * node = (_dbCapNode *) dbCapNode::getCapNode(_block, node_id
  //);

  int field;
  _log.pop(field);

  switch ((_dbCapNode::Fields) field) {
    default: {
      _logger->critical(utl::ODB,
                        412,
                        "No undo_updateCapNodeField support for field {}",
                        field);
      break;
    }
  }
}

void dbJournal::undo_updateCCSegField()
{
  uint node_id;
  _log.pop(node_id);
  //_dbCCSeg * node = (_dbCCSeg *) dbCCSeg::getCCSeg(_block, node_id );

  int field;
  _log.pop(field);

  switch ((_dbCCSeg::Fields) field) {
    default: {
      _logger->critical(utl::ODB,
                        413,
                        "No undo_updateCCSegField support for field {}",
                        field);

      break;
    }
  }
}

dbOStream& operator<<(dbOStream& stream, const dbJournal& journal)
{
  stream << journal._log;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, dbJournal& journal)
{
  stream >> journal._log;
  return stream;
}

}  // namespace odb
