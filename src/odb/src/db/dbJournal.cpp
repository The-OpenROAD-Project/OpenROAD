// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbJournal.h"

#include <cassert>
#include <cstdint>
#include <cstdio>

#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCapNode.h"
#include "dbITerm.h"
#include "dbInst.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbNet.h"
#include "dbRSeg.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "utl/Logger.h"

namespace odb {

dbJournal::dbJournal(dbBlock* block)
    : block_(block), logger_(block->getImpl()->getLogger()), log_(logger_)
{
}

void dbJournal::clear()
{
  log_.clear();
  start_action_ = false;
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            bool prev_value,
                            bool new_value)
{
  beginAction(kUpdateField);
  log_.push(obj->getObjectType());
  log_.push(obj->getId());
  log_.push(field_id);
  log_.push(prev_value);
  log_.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            char prev_value,
                            char new_value)
{
  beginAction(kUpdateField);
  log_.push(obj->getObjectType());
  log_.push(obj->getId());
  log_.push(field_id);
  log_.push(prev_value);
  log_.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            unsigned char prev_value,
                            unsigned char new_value)
{
  beginAction(kUpdateField);
  log_.push(obj->getObjectType());
  log_.push(obj->getId());
  log_.push(field_id);
  log_.push(prev_value);
  log_.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            int prev_value,
                            int new_value)
{
  beginAction(kUpdateField);
  log_.push(obj->getObjectType());
  log_.push(obj->getId());
  log_.push(field_id);
  log_.push(prev_value);
  log_.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            unsigned int prev_value,
                            unsigned int new_value)
{
  beginAction(kUpdateField);
  log_.push(obj->getObjectType());
  log_.push(obj->getId());
  log_.push(field_id);
  log_.push(prev_value);
  log_.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            float prev_value,
                            float new_value)
{
  beginAction(kUpdateField);
  log_.push(obj->getObjectType());
  log_.push(obj->getId());
  log_.push(field_id);
  log_.push(prev_value);
  log_.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            double prev_value,
                            double new_value)
{
  beginAction(kUpdateField);
  log_.push(obj->getObjectType());
  log_.push(obj->getId());
  log_.push(field_id);
  log_.push(prev_value);
  log_.push(new_value);
  endAction();
}

void dbJournal::updateField(dbObject* obj,
                            int field_id,
                            const char* prev_value,
                            const char* new_value)
{
  beginAction(kUpdateField);
  log_.push(obj->getObjectType());
  log_.push(obj->getId());
  log_.push(field_id);
  log_.push(prev_value);
  log_.push(new_value);
  endAction();
}

void dbJournal::beginAction(Action action)
{
  if (start_action_ != false) {
    logger_->critical(
        utl::ODB, 398, "In journal, nested actions are not allowed.");
  }
  start_action_ = true;
  action_idx_ = log_.size();
  log_.push((unsigned char) action);
}

void dbJournal::pushParam(bool value)
{
  log_.push(value);
}

void dbJournal::pushParam(char value)
{
  log_.push(value);
}

void dbJournal::pushParam(unsigned char value)
{
  log_.push(value);
}

void dbJournal::pushParam(int value)
{
  log_.push(value);
}

void dbJournal::pushParam(unsigned int value)
{
  log_.push(value);
}

void dbJournal::pushParam(double value)
{
  log_.push(value);
}

void dbJournal::pushParam(float value)
{
  log_.push(value);
}

void dbJournal::pushParam(const char* value)
{
  log_.push(value);
}

void dbJournal::pushParam(const std::string& value)
{
  log_.push(value.c_str());
}

void dbJournal::endAction()
{
  start_action_ = false;
  log_.push((unsigned char) kEndAction);
  log_.push(action_idx_);  // This value allows log to be scanned backwards.
}

void dbJournal::redo()
{
  log_.begin();

  while (!log_.end()) {
    uint32_t s = log_.idx();
    unsigned char action;
    log_.pop(action);
    cur_action_ = static_cast<Action>(action);

    switch (cur_action_) {
      case kCreateObject:
        redo_createObject();
        break;

      case kDeleteObject:
        redo_deleteObject();
        break;

      case kConnectObject:
        redo_connectObject();
        break;

      case kDisconnectObject:
        redo_disconnectObject();
        break;

      case kSwapObject:
        redo_swapObject();
        break;

      case kUpdateField:
        redo_updateField();
        break;

      case kEndAction:
        logger_->critical(utl::ODB, 399, "In redo saw unexpected kEndAction.");
        break;
    }

    unsigned char end_action;
    unsigned int action_idx;
    log_.pop(end_action);
    log_.pop(action_idx);
    if (end_action != kEndAction || action_idx != s) {
      logger_->critical(
          utl::ODB, 419, "In redo, didn't see the expected kEndAction.");
    }
  }
}

void dbJournal::redo_createObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbNetObj: {
      std::string name;
      uint32_t net_id;
      log_.pop(name);
      log_.pop(net_id);
      dbNet* net = dbNet::create(block_, name.c_str());
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: create {}",
                 net->getDebugName());
      break;
    }

    case dbBTermObj: {
      uint32_t dbNet_id;
      std::string name;
      log_.pop(dbNet_id);
      log_.pop(name);

      dbNet* net = dbNet::getNet(block_, dbNet_id);
      dbBTerm* bterm = dbBTerm::create(net, name.c_str());
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: create {} on {}",
                 bterm->getDebugName(),
                 net->getDebugName());
      break;
    }

    case dbInstObj: {
      uint32_t lib_id;
      uint32_t master_id;
      uint32_t inst_id;
      std::string name;
      log_.pop(lib_id);
      log_.pop(master_id);
      log_.pop(name);
      log_.pop(inst_id);
      dbLib* lib = dbLib::getLib(block_->getDb(), lib_id);
      dbMaster* master = dbMaster::getMaster(lib, master_id);
      dbInst* inst = dbInst::create(block_, master, name.c_str());
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: create {} master '{}' lib '{}'",
                 inst->getDebugName(),
                 master->getName(),
                 lib->getName());
      break;
    }

    case dbRSegObj: {
      uint32_t net_id;
      int x;
      int y;
      uint32_t path_dir;
      bool allocate_cap;
      log_.pop(net_id);
      log_.pop(x);
      log_.pop(y);
      log_.pop(path_dir);
      log_.pop(allocate_cap);
      dbNet* net = dbNet::getNet(block_, net_id);
      debugPrint(logger_,
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
      uint32_t net_id;
      uint32_t node;
      bool foreign;
      log_.pop(net_id);
      log_.pop(node);
      log_.pop(foreign);
      dbNet* net = dbNet::getNet(block_, net_id);
      debugPrint(
          logger_,
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
      uint32_t nodeA, nodeB;
      bool merge;
      log_.pop(nodeA);
      log_.pop(nodeB);
      log_.pop(merge);
      dbCapNode* a = dbCapNode::getCapNode(block_, nodeA);
      dbCapNode* b = dbCapNode::getCapNode(block_, nodeB);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 1,
                 "REDO: dbCCSeg::create, nodeA = {}, nodeB = {}, merge = {}",
                 a->getId(),
                 b->getId(),
                 merge);
      dbCCSeg::create(a, b, merge);
      break;
    }

    case dbModuleObj: {
      std::string name;
      uint32_t obj_id;
      log_.pop(name);
      log_.pop(obj_id);
      (void) obj_id;
      dbModule::create(block_, name.c_str());
      break;
    }

    case dbModITermObj: {
      std::string name;
      uint32_t obj_id;
      uint32_t modbterm_obj_id;
      uint32_t parent_obj_id;
      log_.pop(name);
      log_.pop(obj_id);
      log_.pop(modbterm_obj_id);
      log_.pop(parent_obj_id);
      dbModInst* parent_mod_inst = dbModInst::getModInst(block_, parent_obj_id);
      dbModBTerm* mod_bterm = nullptr;
      if (modbterm_obj_id != 0) {
        mod_bterm = dbModBTerm::getModBTerm(block_, modbterm_obj_id);
      }
      dbModITerm* mod_iterm
          = dbModITerm::create(parent_mod_inst, name.c_str(), mod_bterm);
      (void) mod_iterm;
      break;
    }

    case dbModBTermObj: {
      std::string name;
      uint32_t obj_id;
      uint32_t parent_obj_id;
      log_.pop(name);
      log_.pop(obj_id);
      log_.pop(parent_obj_id);
      dbModule* parent_module = dbModule::getModule(block_, parent_obj_id);
      dbModBTerm::create(parent_module, name.c_str());
      break;
    }

    case dbModInstObj: {
      std::string name;
      uint32_t obj_id;
      uint32_t parent_obj_id;
      uint32_t master_obj_id;
      log_.pop(name);
      log_.pop(obj_id);
      log_.pop(parent_obj_id);
      log_.pop(master_obj_id);
      dbModule* parent_module = dbModule::getModule(block_, parent_obj_id);
      dbModule* master_module = dbModule::getModule(block_, master_obj_id);
      dbModInst::create(parent_module, master_module, name.c_str());
      break;
    }

    case dbModNetObj: {
      std::string name;
      uint32_t obj_id;
      uint32_t parent_obj_id;
      log_.pop(name);
      log_.pop(obj_id);
      log_.pop(parent_obj_id);
      dbModule* parent_module = dbModule::getModule(block_, parent_obj_id);
      dbModNet::create(parent_module, name.c_str());
      break;
    }
    default: {
      logger_->critical(
          utl::ODB, 1107, "Unknown type of action for redo_createObject");
      break;
    }
  }
}

void dbJournal::redo_deleteObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbNetObj: {
      std::string name;
      uint32_t net_id;
      uint32_t flags;
      uint32_t ndr_id;
      log_.pop(name);
      log_.pop(net_id);
      log_.pop(flags);
      log_.pop(ndr_id);
      dbNet* net = dbNet::getNet(block_, net_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: destroy {}",
                 net->getDebugName());
      dbNet::destroy(net);
      break;
    }

    case dbBTermObj: {
      uint32_t bterm_id;
      log_.pop(bterm_id);
      dbBTerm* bterm = dbBTerm::getBTerm(block_, bterm_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: destroy {}",
                 bterm->getDebugName());
      dbBTerm::destroy(bterm);
      break;
    }
    case dbInstObj: {
      uint32_t lib_id;
      uint32_t master_id;
      uint32_t inst_id;
      uint32_t flags;
      std::string name;
      int x;
      int y;
      uint32_t group_id;
      uint32_t module_id;
      uint32_t region_id;
      log_.pop(lib_id);
      log_.pop(master_id);
      log_.pop(name);
      log_.pop(inst_id);
      log_.pop(flags);
      log_.pop(x);
      log_.pop(y);
      log_.pop(group_id);
      log_.pop(module_id);
      log_.pop(region_id);
      dbInst* inst = dbInst::getInst(block_, inst_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: destroy {}",
                 inst->getDebugName());
      dbInst::destroy(inst);
      break;
    }

    case dbRSegObj: {
      uint32_t rseg_id;
      log_.pop(rseg_id);
      dbRSeg* rseg = dbRSeg::getRSeg(block_, rseg_id);
      uint32_t net_id;
      log_.pop(net_id);
      if (net_id) {
        dbNet* net = dbNet::getNet(block_, net_id);
        debugPrint(logger_,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: destroy dbRSegObj, rseg_id {}, net_id {}",
                   rseg_id,
                   net_id);
        dbRSeg::destroy(rseg, net);
      } else {
        debugPrint(logger_,
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
      uint32_t node_id;
      log_.pop(node_id);
      dbCapNode* node = dbCapNode::getCapNode(block_, node_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: destroy dbCapNode, node_id {}",
                 node_id);
      dbCapNode::destroy(node);
      break;
    }

    case dbCCSegObj: {
      uint32_t seg_id;
      log_.pop(seg_id);
      uint32_t regular;
      log_.pop(regular);
      dbCCSeg* seg = dbCCSeg::getCCSeg(block_, seg_id);
      if (regular) {
        debugPrint(logger_,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: destroy dbCCSeg, inst_id {}",
                   seg_id);
        dbCCSeg::destroy(seg);
      } else {
        debugPrint(logger_,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: simple destroy dbCCSeg, inst_id {}",
                   seg_id);
        dbCCSeg::destroyS(seg);
      }
      break;
    }

    case dbModuleObj: {
      uint32_t module_id;
      std::string name;
      log_.pop(name);
      log_.pop(module_id);
      (void) name;
      auto module = dbModule::getModule(block_, module_id);
      dbModule::destroy(module);
      break;
    }

    case dbModITermObj: {
      uint32_t obj_id;
      uint32_t modbterm_id;
      std::string name;
      log_.pop(name);
      log_.pop(obj_id);
      log_.pop(modbterm_id);
      (void) name;
      (void) modbterm_id;
      auto moditerm = dbModITerm::getModITerm(block_, obj_id);
      dbModITerm::destroy(moditerm);
      break;
    }

    case dbModBTermObj: {
      uint32_t obj_id;
      std::string name;
      log_.pop(name);
      log_.pop(obj_id);
      (void) name;
      auto modbterm = dbModBTerm::getModBTerm(block_, obj_id);
      dbModBTerm::destroy(modbterm);
      break;
    }

    case dbModInstObj: {
      uint32_t obj_id;
      uint32_t parent_id;
      uint32_t master_id;
      std::string name;
      log_.pop(name);
      log_.pop(obj_id);
      log_.pop(parent_id);
      log_.pop(master_id);
      (void) name;
      auto modinst = dbModInst::getModInst(block_, obj_id);
      dbModInst::destroy(modinst);
      break;
    }

    case dbModNetObj: {
      std::string name;
      uint32_t obj_id;
      uint32_t parent_id;
      log_.pop(name);
      log_.pop(obj_id);
      log_.pop(parent_id);
      (void) name;
      auto modnet = dbModNet::getModNet(block_, obj_id);
      dbModNet::destroy(modnet);
      break;
    }
    default: {
      logger_->critical(
          utl::ODB, 1108, "Unknown type of action for redo_deleteObject");
      break;
    }
  }
}

void dbJournal::redo_connectObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbITermObj: {
      uint32_t iterm_id;
      log_.pop(iterm_id);
      dbITerm* iterm = dbITerm::getITerm(block_, iterm_id);
      uint32_t net_id;
      log_.pop(net_id);
      if (net_id != 0) {
        dbNet* net = dbNet::getNet(block_, net_id);
        debugPrint(logger_,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: connect {} to {}",
                   iterm->getDebugName(),
                   net->getDebugName());
        iterm->connect(net);
      }
      uint32_t mod_net_id;
      log_.pop(mod_net_id);
      if (mod_net_id != 0) {
        dbModNet* mod_net = dbModNet::getModNet(block_, mod_net_id);
        debugPrint(logger_,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: connect {} to {}",
                   iterm->getDebugName(),
                   mod_net->getDebugName());
        iterm->connect(mod_net);
      }
      break;
    }

    case dbBTermObj: {
      uint32_t bterm_id;
      log_.pop(bterm_id);
      dbBTerm* bterm = dbBTerm::getBTerm(block_, bterm_id);
      uint32_t net_id;
      log_.pop(net_id);
      dbNet* net = dbNet::getNet(block_, net_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: connect {} to {}",
                 bterm->getDebugName(),
                 net->getDebugName());
      bterm->connect(net);
      break;
    }

    case dbModBTermObj: {
      uint32_t modbterm_id;
      uint32_t modnet_id;
      log_.pop(modbterm_id);
      log_.pop(modnet_id);
      dbModBTerm* modbterm = dbModBTerm::getModBTerm(block_, modbterm_id);
      if (modnet_id != 0) {
        dbModNet* modnet = dbModNet::getModNet(block_, modnet_id);
        modbterm->connect(modnet);
      }
      break;
    }

    case dbModITermObj: {
      uint32_t moditerm_id;
      uint32_t modnet_id;
      log_.pop(moditerm_id);
      log_.pop(modnet_id);
      dbModITerm* moditerm = dbModITerm::getModITerm(block_, moditerm_id);
      if (modnet_id != 0) {
        dbModNet* modnet = dbModNet::getModNet(block_, modnet_id);
        moditerm->connect(modnet);
      }
      break;
    }

    default: {
      logger_->critical(
          utl::ODB, 1110, "Unknown type of object for redo_connectObject");
      break;
    }
  }
}

void dbJournal::redo_disconnectObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbITermObj: {
      uint32_t iterm_id;
      log_.pop(iterm_id);
      dbITerm* iterm = dbITerm::getITerm(block_, iterm_id);
      uint32_t net_id;
      uint32_t mnet_id;
      log_.pop(net_id);
      log_.pop(mnet_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: disconnect {}",
                 iterm->getDebugName());
      if (net_id != 0 && mnet_id != 0) {
        // note: this will disconnect the modnet and the dbNet
        iterm->disconnect();
      } else if (net_id != 0) {
        iterm->disconnectDbNet();
      } else if (mnet_id != 0) {
        iterm->disconnectDbModNet();
      }
      break;
    }

    case dbBTermObj: {
      uint32_t bterm_id;
      log_.pop(bterm_id);
      dbBTerm* bterm = dbBTerm::getBTerm(block_, bterm_id);
      uint32_t net_id;
      log_.pop(net_id);
      if (net_id != 0U) {
        bterm->disconnectDbNet();
      }
      uint32_t mnet_id;
      log_.pop(mnet_id);
      if (mnet_id != 0U) {
        bterm->disconnectDbModNet();
      }
      break;
    }

    case dbModBTermObj: {
      uint32_t modbterm_id;
      uint32_t modnet_id;
      log_.pop(modbterm_id);
      log_.pop(modnet_id);
      (void) modnet_id;
      auto modbterm = dbModBTerm::getModBTerm(block_, modbterm_id);
      modbterm->disconnect();
      break;
    }

    case dbModITermObj: {
      uint32_t moditerm_id;
      uint32_t modnet_id;
      log_.pop(moditerm_id);
      log_.pop(modnet_id);
      (void) modnet_id;
      auto moditerm = dbModITerm::getModITerm(block_, moditerm_id);
      moditerm->disconnect();
      break;
    }

    default: {
      logger_->critical(
          utl::ODB, 1109, "Unknown type of action for redo_disconnectObject");
      break;
    }
  }
}

void dbJournal::redo_swapObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbInstObj: {
      uint32_t inst_id;
      log_.pop(inst_id);
      dbInst* inst = dbInst::getInst(block_, inst_id);

      uint32_t prev_lib_id;
      log_.pop(prev_lib_id);

      uint32_t prev_master_id;
      log_.pop(prev_master_id);

      uint32_t lib_id;
      log_.pop(lib_id);
      dbLib* lib = dbLib::getLib(block_->getDb(), lib_id);

      uint32_t master_id;
      log_.pop(master_id);
      dbMaster* master = dbMaster::getMaster(lib, master_id);
      debugPrint(
          logger_,
          utl::ODB,
          "DB_ECO",
          2,
          "ECO: swapMaster {}, prev lib/master: {}/{}, new lib/master: {}/{}",
          inst->getDebugName(),
          inst->getName(),
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

    case dbModNetObj:
      redo_updateModNetField();
      break;

    default:
      break;
  }
}

void dbJournal::redo_updateBlockField()
{
  uint32_t block_id;
  log_.pop(block_id);
  int field;
  log_.pop(field);
  switch ((_dbBlock::Field) field) {
    case _dbBlock::kCornerCount: {
      int cornerCount;
      log_.pop(cornerCount);
      int extDbCount;
      log_.pop(extDbCount);
      std::string name;
      log_.pop(name);
      debugPrint(
          logger_,
          utl::ODB,
          "DB_ECO",
          2,
          "REDO ECO: dbBlock {}, setCornerCount cornerCount {}, extDbCount "
          "{}, name_list {}",
          block_id,
          cornerCount,
          extDbCount,
          name);
      block_->setCornerCount(cornerCount, extDbCount, name.c_str());
      break;
    }

    case _dbBlock::kWriteDb: {
      std::string name;
      log_.pop(name);
      int allNode;
      log_.pop(allNode);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbBlock {}, writeDb, filename {}, allNode {}",
                 block_id,
                 name,
                 allNode);
      block_->writeDb((char*) name.c_str(), allNode);
      break;
    }

    default:
      break;
  }
}

void dbJournal::redo_updateNetField()
{
  uint32_t net_id;
  log_.pop(net_id);
  _dbNet* net = (_dbNet*) dbNet::getNet(block_, net_id);

  int field;
  log_.pop(field);

  switch ((_dbNet::Field) field) {
    case _dbNet::kFlags: {
      uint32_t prev_flags;
      log_.pop(prev_flags);
      uint32_t* flags = (uint32_t*) &net->flags_;
      log_.pop(*flags);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbNetObj {}, updateNetField: {} to {}",
                 net_id,
                 prev_flags,
                 *flags);
      break;
    }

    case _dbNet::kNonDefaultRule: {
      uint32_t prev_rule;
      log_.pop(prev_rule);
      unsigned int id;
      log_.pop(id);
      net->non_default_rule_ = id;
      bool prev_block_rule;
      bool cur_block_rule;
      log_.pop(prev_block_rule);
      log_.pop(cur_block_rule);
      net->flags_.block_rule = cur_block_rule;
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbNetObj {}, updateNonDefaultRule: {} to {}",
                 net_id,
                 prev_rule,
                 net->non_default_rule_.id());
      break;
    }

    case _dbNet::kTermExtId: {
      int capId;
      log_.pop(capId);
      ((dbNet*) net)->setTermExtIds(capId);
      if (capId) {
        debugPrint(logger_,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: dbNetObj {} set term extId",
                   net_id);
      } else {
        debugPrint(logger_,
                   utl::ODB,
                   "DB_ECO",
                   2,
                   "REDO ECO: dbNetObj {} reset term extId",
                   net_id);
      }
      break;
    }

    case _dbNet::kHeadRSeg: {
      uint32_t pid;
      uint32_t rid;
      log_.pop(pid);
      log_.pop(rid);
      ((dbNet*) net)->set1stRSegId(rid);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbNetObj {}, set1stRSegId {} to {}",
                 net_id,
                 pid,
                 rid);
      break;
    }

    case _dbNet::kReverseRSeg: {
      dbSet<dbRSeg> rSet = ((dbNet*) net)->getRSegs();
      rSet.reverse();
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbNetObj {}, reverse rsegs sequence",
                 net_id);
      break;
    }

    case _dbNet::kHeadCapNode: {
      uint32_t pid;
      uint32_t cid;
      log_.pop(pid);
      log_.pop(cid);
      ((dbNet*) net)->set1stCapNodeId(cid);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbNetObj {}, set1stRSegId {} to {}",
                 net_id,
                 pid,
                 cid);
      break;
    }

    case _dbNet::kName: {
      std::string prev_name;
      log_.pop(prev_name);
      std::string new_name;
      log_.pop(new_name);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbNetObj {}, updateName from {} to {}",
                 net_id,
                 prev_name,
                 new_name);
      ((dbNet*) net)->rename(new_name.c_str());
      break;
    }

    default:
      break;
  }
}

void dbJournal::redo_updateModNetField()
{
  uint32_t modnet_id;
  log_.pop(modnet_id);
  _dbModNet* modnet = (_dbModNet*) dbModNet::getModNet(block_, modnet_id);

  int field;
  log_.pop(field);

  switch ((_dbModNet::Field) field) {
    case _dbModNet::kName: {
      std::string prev_name;
      log_.pop(prev_name);
      std::string new_name;
      log_.pop(new_name);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbModNetObj {}, updateName from {} to {}",
                 modnet_id,
                 prev_name,
                 new_name);
      ((dbModNet*) modnet)->rename(new_name.c_str());
      break;
    }

    default:
      break;
  }
}

void dbJournal::redo_updateInstField()
{
  uint32_t inst_id;
  log_.pop(inst_id);
  _dbInst* inst = (_dbInst*) dbInst::getInst(block_, inst_id);

  int field;
  log_.pop(field);

  switch ((_dbInst::Field) field) {
    case _dbInst::kFlags: {
      uint32_t prev_flags;
      log_.pop(prev_flags);
      uint32_t* flags = (uint32_t*) &inst->flags_;
      log_.pop(*flags);

      // Changing the orientation flag requires updating the cached bbox
      _dbInstFlags* a = (_dbInstFlags*) flags;
      _dbInstFlags* b = (_dbInstFlags*) &prev_flags;
      if (a->orient != b->orient) {
        _dbInst::setInstBBox(inst);
      }

      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbInst {}, updateInstField: {} to {}",
                 inst_id,
                 prev_flags,
                 *flags);
      break;
    }

    case _dbInst::kOrigin: {
      int prev_x;
      log_.pop(prev_x);
      int prev_y;
      log_.pop(prev_y);
      int current_x;
      log_.pop(current_x);
      int current_y;
      log_.pop(current_y);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbInst {}, origin: {},{} to {},{}",
                 inst_id,
                 prev_x,
                 prev_y,
                 inst->x_,
                 inst->y_);
      ((dbInst*) inst)->setOrigin(current_x, current_y);
      break;
    }

    case _dbInst::kName: {
      std::string prev_name;
      log_.pop(prev_name);
      std::string new_name;
      log_.pop(new_name);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbInstObj {}, updateName from {} to {}",
                 inst_id,
                 prev_name,
                 new_name);
      ((dbInst*) inst)->rename(new_name.c_str());
      break;
    }

    default:
      break;
  }
}

void dbJournal::redo_updateBTermField()
{
  uint32_t bterm_id;
  log_.pop(bterm_id);
  _dbBTerm* bterm = (_dbBTerm*) dbBTerm::getBTerm(block_, bterm_id);

  int field;
  log_.pop(field);

  switch ((_dbBTerm::Field) field) {
    case _dbBTerm::kFlags: {
      uint32_t prev_flags;
      log_.pop(prev_flags);
      uint32_t* flags = (uint32_t*) &bterm->flags_;
      log_.pop(*flags);
      debugPrint(logger_,
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
  uint32_t iterm_id;
  log_.pop(iterm_id);
  _dbITerm* iterm = (_dbITerm*) dbITerm::getITerm(block_, iterm_id);

  int field;
  log_.pop(field);

  switch ((_dbITerm::Field) field) {
    case _dbITerm::kFlags: {
      uint32_t prev_flags;
      log_.pop(prev_flags);
      uint32_t* flags = (uint32_t*) &iterm->flags_;
      log_.pop(*flags);
      debugPrint(logger_,
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
  uint32_t rseg_id;
  log_.pop(rseg_id);
  _dbRSeg* rseg = (_dbRSeg*) dbRSeg::getRSeg(block_, rseg_id);

  int field;
  log_.pop(field);

  switch ((_dbRSeg::Field) field) {
    case _dbRSeg::kFlags: {
      uint32_t prev_flags;
      log_.pop(prev_flags);
      uint32_t* flags = (uint32_t*) &rseg->flags_;
      log_.pop(*flags);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbRSeg {}, updateRSegField: {} to {}",
                 rseg_id,
                 prev_flags,
                 *flags);
      break;
    }

    case _dbRSeg::kSource: {
      uint32_t prev_source;
      log_.pop(prev_source);
      log_.pop(rseg->source_);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbRSeg {}, updateSource: {} to {}",
                 rseg_id,
                 prev_source,
                 rseg->source_);
      break;
    }

    case _dbRSeg::kTarget: {
      uint32_t prev_target;
      log_.pop(prev_target);
      log_.pop(rseg->target_);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbRSeg {}, updateTarget: {} to {}",
                 rseg_id,
                 prev_target,
                 rseg->target_);
      break;
    }

    case _dbRSeg::kResistance: {
      float prev_r;
      float r;
      int cnr;
      log_.pop(prev_r);
      log_.pop(r);
      log_.pop(cnr);
      debugPrint(logger_,
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

    case _dbRSeg::kCapacitance: {
      float prev_c;
      float c;
      int cnr;
      log_.pop(prev_c);
      log_.pop(c);
      log_.pop(cnr);
      debugPrint(logger_,
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

    case _dbRSeg::kAddRSegCapacitance: {
      uint32_t oseg_id;
      log_.pop(oseg_id);
      _dbRSeg* other = (_dbRSeg*) dbRSeg::getRSeg(block_, oseg_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbRSeg {}, other dbRSeg {}, addCcCapacitance",
                 rseg_id,
                 oseg_id);
      ((dbRSeg*) rseg)->addRSegCapacitance((dbRSeg*) other);
      break;
    }

    case _dbRSeg::kAddRSegResistance: {
      uint32_t oseg_id;
      log_.pop(oseg_id);
      _dbRSeg* other = (_dbRSeg*) dbRSeg::getRSeg(block_, oseg_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbRSeg {}, other dbRSeg {}, addCcCapacitance",
                 rseg_id,
                 oseg_id);
      ((dbRSeg*) rseg)->addRSegResistance((dbRSeg*) other);
      break;
    }

    case _dbRSeg::kCoordinates: {
      int prev_x;
      int prev_y;
      int x;
      int y;
      log_.pop(prev_x);
      log_.pop(x);
      log_.pop(prev_y);
      log_.pop(y);
      debugPrint(
          logger_,
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
  uint32_t node_id;
  log_.pop(node_id);
  _dbCapNode* node = (_dbCapNode*) dbCapNode::getCapNode(block_, node_id);

  int field;
  log_.pop(field);

  switch ((_dbCapNode::Fields) field) {
    case _dbCapNode::kFlags: {
      uint32_t prev_flags;
      log_.pop(prev_flags);
      uint32_t* flags = (uint32_t*) &node->flags_;
      log_.pop(*flags);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCapNode {}, updateFlags: {} to {}",
                 node_id,
                 prev_flags,
                 *flags);
      break;
    }

    case _dbCapNode::kNodeNum: {
      uint32_t prev_num;
      log_.pop(prev_num);
      log_.pop(node->node_num_);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCapNode {}, updateNodeNum: {} to {}",
                 node_id,
                 prev_num,
                 node->node_num_);
      break;
    }

    case _dbCapNode::kCapacitance: {
      float prev_c;
      float c;
      int cnr;
      log_.pop(prev_c);
      log_.pop(c);
      log_.pop(cnr);
      debugPrint(logger_,
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
    case _dbCapNode::kAddCapnCapacitance: {
      uint32_t oseg_id;
      log_.pop(oseg_id);
      _dbCapNode* other = (_dbCapNode*) dbCapNode::getCapNode(block_, oseg_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCapNode {}, other dbCapNode {}, addCcCapacitance",
                 node_id,
                 oseg_id);
      ((dbCapNode*) node)->addCapnCapacitance((dbCapNode*) other);
      break;
    }
    case _dbCapNode::kSetNet: {
      uint32_t onet_id;
      log_.pop(onet_id);
      uint32_t nnet_id;
      log_.pop(nnet_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCapNode {}, set net {}",
                 node_id,
                 nnet_id);
      ((dbCapNode*) node)->setNet(nnet_id);
      break;
    }
    case _dbCapNode::kSetNext: {
      uint32_t onext;
      log_.pop(onext);
      uint32_t nnext;
      log_.pop(nnext);
      debugPrint(logger_,
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
  uint32_t seg_id;
  log_.pop(seg_id);
  _dbCCSeg* seg = (_dbCCSeg*) dbCCSeg::getCCSeg(block_, seg_id);

  int field;
  log_.pop(field);

  switch ((_dbCCSeg::Fields) field) {
    case _dbCCSeg::kFlags: {
      uint32_t prev_flags;
      log_.pop(prev_flags);
      uint32_t* flags = (uint32_t*) &seg->flags_;
      log_.pop(*flags);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCCSeg {}, updateFlags: {} to {}",
                 seg_id,
                 prev_flags,
                 *flags);
      break;
    }

    case _dbCCSeg::kCapacitance: {
      float prev_c;
      float c;
      int cnr;
      log_.pop(prev_c);
      log_.pop(c);
      log_.pop(cnr);
      debugPrint(logger_,
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

    case _dbCCSeg::kAddCcCapacitance: {
      uint32_t oseg_id;
      log_.pop(oseg_id);
      _dbCCSeg* other = (_dbCCSeg*) dbCCSeg::getCCSeg(block_, oseg_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCCSeg {}, other dbCCSeg {}, addCcCapacitance",
                 seg_id,
                 oseg_id);
      ((dbCCSeg*) seg)->addCcCapacitance((dbCCSeg*) other);
      break;
    }

    case _dbCCSeg::kSwapCapNode: {
      uint32_t ocap_id;
      log_.pop(ocap_id);
      _dbCapNode* orig = (_dbCapNode*) dbCapNode::getCapNode(block_, ocap_id);
      uint32_t ncap_id;
      log_.pop(ncap_id);
      _dbCapNode* newn = (_dbCapNode*) dbCapNode::getCapNode(block_, ncap_id);
      debugPrint(
          logger_,
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
    case _dbCCSeg::kLinkCcSeg: {
      uint32_t cap_id;
      log_.pop(cap_id);
      uint32_t cseq;
      log_.pop(cseq);
      dbCapNode* capn = dbCapNode::getCapNode(block_, cap_id);
      debugPrint(logger_,
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
    case _dbCCSeg::kUnlinkCcSeg: {
      uint32_t cap_id;
      log_.pop(cap_id);
      dbCapNode* capn = dbCapNode::getCapNode(block_, cap_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 2,
                 "REDO ECO: dbCCSeg {}, unLink_cc_seg, capNode {}",
                 seg->getOID(),
                 cap_id);
      ((dbCCSeg*) seg)->unLink_cc_seg(capn);
      break;
    }
    case _dbCCSeg::kSetAllCcCap: {
      uint32_t cornerCnt = block_->getCornerCount();
      double ttcap[ADS_MAX_CORNER];
      char ccCaps[400];
      ccCaps[0] = '\0';
      int pos = 0;
      for (uint32_t ii = 0; ii < cornerCnt; ii++) {
        log_.pop(ttcap[ii]);
        pos += sprintf(&ccCaps[pos], "%f ", ttcap[ii]);
      }
      debugPrint(logger_,
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
  log_.pop(obj_type_value);
  return static_cast<dbObjectType>(obj_type_value);
}

//
// WORK-IN-PROGRESS undo is incomplete
//
void dbJournal::undo()
{
  debugPrint(logger_,
             utl::ODB,
             "DB_ECO",
             4,
             "UNDO ECO: Starting undo of size {} >>>",
             log_.size());
  if (log_.empty()) {
    debugPrint(logger_,
               utl::ODB,
               "DB_ECO",
               4,
               "UNDO ECO: Exited early since log is empty >>>");
    return;
  }

  log_.moveToEnd();
  log_.moveBackOneInt();

  for (;;) {
    debugPrint(
        logger_, utl::ODB, "DB_ECO", 4, "UNDO ECO: Log index {}", log_.idx());
    uint32_t action_idx;
    log_.pop(action_idx);
    debugPrint(logger_,
               utl::ODB,
               "DB_ECO",
               4,
               "UNDO ECO: Undoing action at index {}",
               action_idx);
    log_.set(action_idx);
    unsigned char action;
    log_.pop(action);
    cur_action_ = static_cast<Action>(action);

    switch (cur_action_) {
      case kCreateObject:
        undo_createObject();
        break;

      case kDeleteObject:
        undo_deleteObject();
        break;

      case kConnectObject:
        undo_connectObject();
        break;

      case kDisconnectObject:
        undo_disconnectObject();
        break;

      case kSwapObject:
        undo_swapObject();
        break;

      case kUpdateField:
        undo_updateField();
        break;

      case kEndAction:
        break;
    }

    if (action_idx == 0) {
      break;
    }

    log_.set(action_idx);
    log_.moveBackOneInt();
  }
  debugPrint(logger_, utl::ODB, "DB_ECO", 4, "UNDO ECO: Finished undo >>>");
}

void dbJournal::undo_createObject()
{
  auto obj_type = popObjectType();

  switch (obj_type) {
    case dbGuideObj: {
      uint32_t guide_id;
      log_.pop(guide_id);
      dbGuide* guide = dbGuide::getGuide(block_, guide_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: create dbGuide at id {}, in layer {} box {}",
                 guide_id,
                 guide->getLayer()->getName(),
                 guide->getBox());
      dbGuide::destroy(guide);
      break;
    }

    case dbInstObj: {
      uint32_t lib_id;
      uint32_t master_id;
      uint32_t inst_id;
      std::string name;
      log_.pop(lib_id);
      log_.pop(master_id);
      log_.pop(name);
      log_.pop(inst_id);
      dbInst* inst = dbInst::getInst(block_, inst_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: create {}",
                 inst->getDebugName());
      dbInst::destroy(inst);
      break;
    }

    case dbNetObj: {
      std::string name;
      uint32_t net_id;
      log_.pop(name);
      log_.pop(net_id);
      dbNet* net = dbNet::getNet(block_, net_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: create {}",
                 net->getDebugName());
      dbNet::destroy(net);
      break;
    }

    case dbModuleObj: {
      std::string name;
      uint32_t obj_id;
      log_.pop(name);
      log_.pop(obj_id);
      (void) name;
      dbModule* mod = dbModule::getModule(block_, obj_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: create {}",
                 mod->getDebugName());
      dbModule::destroy(mod);
      break;
    }

    case dbModInstObj: {
      uint32_t obj_id;
      std::string name;
      uint32_t parent_id;
      uint32_t master_id;

      log_.pop(name);
      log_.pop(obj_id);
      log_.pop(parent_id);
      log_.pop(master_id);

      (void) name;
      dbModInst* mod_inst = dbModInst::getModInst(block_, obj_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: create {}",
                 mod_inst->getDebugName());
      dbModInst::destroy(mod_inst);
      break;
    }

    case dbModNetObj: {
      std::string name;
      uint32_t mod_net_id;
      uint32_t parent_id;
      log_.pop(name);
      log_.pop(mod_net_id);
      log_.pop(parent_id);
      dbModNet* mod_net = dbModNet::getModNet(block_, mod_net_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: create {}",
                 mod_net->getDebugName());
      dbModNet::destroy(mod_net);
      break;
    }

    case dbModBTermObj: {
      std::string name;
      uint32_t modbterm_id;
      uint32_t parent_id;
      log_.pop(name);
      log_.pop(modbterm_id);
      log_.pop(parent_id);
      (void) parent_id;
      dbModBTerm* modbterm = dbModBTerm::getModBTerm(block_, modbterm_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: create {}",
                 modbterm->getDebugName());
      dbModBTerm::destroy(modbterm);
      break;
    }

    case dbModITermObj: {
      std::string name;
      uint32_t moditerm_id;
      uint32_t modbterm_id;
      uint32_t mod_inst_parent_id;
      log_.pop(name);
      log_.pop(moditerm_id);
      log_.pop(modbterm_id);
      log_.pop(mod_inst_parent_id);
      dbModITerm* moditerm = dbModITerm::getModITerm(block_, moditerm_id);
      (void) name;
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: create {}",
                 moditerm->getDebugName());
      dbModITerm::destroy(moditerm);
      break;
    }

    default: {
      logger_->critical(utl::ODB,
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
      uint32_t net_id;
      int x_min;
      int y_min;
      int x_max;
      int y_max;
      uint32_t layer_id;
      uint32_t via_layer_id;
      bool is_congested;
      log_.pop(net_id);
      log_.pop(x_min);
      log_.pop(y_min);
      log_.pop(x_max);
      log_.pop(y_max);
      log_.pop(layer_id);
      log_.pop(via_layer_id);
      log_.pop(is_congested);
      auto net = dbNet::getNet(block_, net_id);
      auto layer = dbTechLayer::getTechLayer(block_->getTech(), layer_id);
      auto via_layer
          = dbTechLayer::getTechLayer(block_->getTech(), via_layer_id);
      auto guide = dbGuide::create(
          net, layer, via_layer, {x_min, y_min, x_max, y_max}, is_congested);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: delete dbGuide at new id {}, in layer {} box {}",
                 guide->getId(),
                 layer->getName(),
                 guide->getBox());
      break;
    }

    case dbInstObj: {
      uint32_t lib_id;
      uint32_t master_id;
      uint32_t inst_id;
      std::string name;
      int x;
      int y;
      uint32_t group_id;
      uint32_t module_id;
      uint32_t region_id;
      log_.pop(lib_id);
      log_.pop(master_id);
      log_.pop(name);
      log_.pop(inst_id);
      dbLib* lib = dbLib::getLib(block_->getDb(), lib_id);
      dbMaster* master = dbMaster::getMaster(lib, master_id);
      auto inst = dbInst::create(block_, master, name.c_str());
      _dbInst* impl = (_dbInst*) inst;
      uint32_t* flags = (uint32_t*) &impl->flags_;
      log_.pop(*flags);
      log_.pop(x);
      log_.pop(y);
      log_.pop(group_id);
      log_.pop(module_id);
      log_.pop(region_id);
      inst->setOrigin(x, y);
      if (group_id != 0) {
        auto group = dbGroup::getGroup(block_, group_id);
        group->addInst(inst);
      }
      if (module_id != 0) {
        auto module = dbModule::getModule(block_, module_id);
        module->addInst(inst);
      }
      if (region_id != 0) {
        auto region = dbRegion::getRegion(block_, region_id);
        region->addInst(inst);
      }
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: delete {} dbMaster({})",
                 inst->getDebugName(),
                 master_id);
      break;
    }

    case dbNetObj: {
      std::string name;
      uint32_t net_id;
      uint32_t ndr_id;
      log_.pop(name);
      log_.pop(net_id);
      auto net = dbNet::create(block_, name.c_str());
      _dbNet* impl = (_dbNet*) net;
      uint32_t* flags = (uint32_t*) &impl->flags_;
      log_.pop(*flags);
      log_.pop(ndr_id);
      _dbNet* net_impl = (_dbNet*) net;
      net_impl->non_default_rule_ = ndr_id;
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: delete {}",
                 net->getDebugName());
      break;
    }

    case dbModuleObj: {
      std::string name;
      uint32_t obj_id;
      log_.pop(name);
      log_.pop(obj_id);
      dbModule* module = dbModule::create(block_, name.c_str());
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: delete {}",
                 module->getDebugName());
      break;
    }

    case dbModInstObj: {
      std::string name;
      uint32_t obj_id;
      uint32_t master_module_id;
      uint32_t parent_module_id;
      uint32_t group_id;
      log_.pop(name);
      log_.pop(obj_id);
      log_.pop(parent_module_id);
      log_.pop(master_module_id);
      log_.pop(group_id);
      dbModule* parent_module = dbModule::getModule(block_, parent_module_id);
      dbModule* master_module = dbModule::getModule(block_, master_module_id);
      dbModInst* mod_inst
          = dbModInst::create(parent_module, master_module, name.c_str());
      if (group_id != 0) {
        auto group = dbGroup::getGroup(block_, group_id);
        group->addModInst(mod_inst);
      }
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: delete {}",
                 mod_inst->getDebugName());
      break;
    }

    case dbModNetObj: {
      std::string name;
      uint32_t module_id;
      uint32_t obj_id;
      log_.pop(name);
      log_.pop(obj_id);
      log_.pop(module_id);
      // get the parent module
      dbModule* module = dbModule::getModule(block_, module_id);
      auto net = dbModNet::create(module, name.c_str());
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: delete {}",
                 net->getDebugName());
      break;
    }

    case dbModBTermObj: {
      std::string name;
      uint32_t module_id;
      uint32_t obj_id;
      log_.pop(name);
      log_.pop(obj_id);
      log_.pop(module_id);
      // get the parent module
      dbModule* parent_module = dbModule::getModule(block_, module_id);
      auto mod_bterm = dbModBTerm::create(parent_module, name.c_str());
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: delete {}",
                 mod_bterm->getDebugName());
      break;
    }

    case dbModITermObj: {
      std::string name;
      uint32_t obj_id;
      uint32_t modinst_id;
      uint32_t modbterm_id;
      log_.pop(name);
      log_.pop(obj_id);
      log_.pop(modbterm_id);
      log_.pop(modinst_id);
      // get the parent module
      dbModInst* mod_inst = dbModInst::getModInst(block_, modinst_id);
      dbModBTerm* mod_bterm = nullptr;
      if (modbterm_id != 0U) {
        mod_bterm = dbModBTerm::getModBTerm(block_, modbterm_id);
      }
      auto mod_iterm = dbModITerm::create(mod_inst, name.c_str(), mod_bterm);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: delete {}",
                 mod_iterm->getDebugName());
      break;
    }

    default: {
      logger_->critical(utl::ODB,
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
      uint32_t mod_iterm_id;
      log_.pop(mod_iterm_id);
      dbModITerm* mod_iterm = dbModITerm::getModITerm(block_, mod_iterm_id);
      uint32_t net_id;
      log_.pop(net_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: connect {}",
                 mod_iterm->getDebugName());
      mod_iterm->disconnect();
      break;
    }

    case dbModBTermObj: {
      uint32_t modbterm_id;
      log_.pop(modbterm_id);
      dbModBTerm* modbterm = dbModBTerm::getModBTerm(block_, modbterm_id);
      uint32_t net_id;
      log_.pop(net_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: connect {}",
                 modbterm->getDebugName());
      modbterm->disconnect();
      break;
    }

    case dbITermObj: {
      uint32_t iterm_id;
      log_.pop(iterm_id);
      dbITerm* iterm = dbITerm::getITerm(block_, iterm_id);
      uint32_t net_id;
      uint32_t mnet_id;
      log_.pop(net_id);   // the db net
      log_.pop(mnet_id);  // the modnet

      dbNet* net = nullptr;
      if (net_id != 0) {
        net = dbNet::getNet(block_, net_id);
      }
      dbModNet* mod_net = nullptr;
      if (mnet_id != 0) {
        mod_net = dbModNet::getModNet(block_, mnet_id);
      }
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: connect {} to {} and {}",
                 iterm->getDebugName(),
                 (net) ? net->getDebugName() : "dbNet(NULL)",
                 (mod_net) ? mod_net->getDebugName() : "dbModNet(NULL)");

      //
      // we signal which net to disconnect
      // by the id, if non zero. if zero
      // do not disconnect. For example if we are just
      // disconnecting the dbNet then in the journalling for
      // the caller we set the mnet_id to zero.
      //
      assert(net_id != 0 || mnet_id != 0);
      if (net_id != 0) {
        iterm->disconnectDbNet();
      } else if (mnet_id != 0) {
        iterm->disconnectDbModNet();
      }
      break;
    }

    case dbBTermObj: {
      uint32_t bterm_id;
      log_.pop(bterm_id);
      dbBTerm* bterm = dbBTerm::getBTerm(block_, bterm_id);
      uint32_t net_id;
      uint32_t mnet_id;
      log_.pop(net_id);   // the db net
      log_.pop(mnet_id);  // the modnet
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: connect {} dbNet({}) dbModNet({})",
                 bterm->getDebugName(),
                 net_id,
                 mnet_id);
      assert(net_id != 0 || mnet_id != 0);
      if (net_id != 0) {
        bterm->disconnectDbNet();
      } else if (mnet_id != 0) {
        bterm->disconnectDbModNet();
      }
      break;
    }

    default: {
      logger_->critical(utl::ODB,
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
      uint32_t iterm_id;
      log_.pop(iterm_id);
      dbITerm* iterm = dbITerm::getITerm(block_, iterm_id);
      uint32_t net_id = 0U;
      log_.pop(net_id);
      dbNet* net = nullptr;
      if (net_id != 0) {
        net = dbNet::getNet(block_, net_id);
        iterm->connect(net);
      }
      uint32_t mnet_id = 0U;
      log_.pop(mnet_id);
      dbModNet* mod_net = nullptr;
      if (mnet_id != 0) {
        mod_net = dbModNet::getModNet(block_, mnet_id);
        iterm->connect(mod_net);
      }
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: disconnect {} from {} and {}",
                 iterm->getDebugName(),
                 (net) ? net->getDebugName() : "dbNet(NULL)",
                 (mod_net) ? mod_net->getDebugName() : "dbModNet(NULL)");
      break;
    }

    case dbBTermObj: {
      uint32_t bterm_id;
      log_.pop(bterm_id);
      dbBTerm* bterm = dbBTerm::getBTerm(block_, bterm_id);
      uint32_t net_id;
      log_.pop(net_id);
      if (net_id != 0) {
        dbNet* net = dbNet::getNet(block_, net_id);
        bterm->connect(net);
      }
      uint32_t mnet_id;
      log_.pop(mnet_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: disconnect {}",
                 bterm->getDebugName());
      if (mnet_id != 0) {
        dbModNet* mnet = dbModNet::getModNet(block_, mnet_id);
        bterm->connect(mnet);
      }
      break;
    }

    case dbModBTermObj: {
      uint32_t mod_bterm_id;
      uint32_t mod_net_id;
      log_.pop(mod_bterm_id);
      log_.pop(mod_net_id);
      dbModBTerm* mod_bterm = dbModBTerm::getModBTerm(block_, mod_bterm_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: disconnect {}",
                 mod_bterm->getDebugName());
      if (mod_net_id != 0U) {
        dbModNet* mnet = dbModNet::getModNet(block_, mod_net_id);
        mod_bterm->connect(mnet);
      }
      break;
    }

    case dbModITermObj: {
      uint32_t mod_iterm_id;
      uint32_t mod_net_id;
      log_.pop(mod_iterm_id);
      log_.pop(mod_net_id);
      dbModITerm* mod_iterm = dbModITerm::getModITerm(block_, mod_iterm_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: disconnect {}",
                 mod_iterm->getDebugName());
      if (mod_net_id != 0U) {
        dbModNet* mnet = dbModNet::getModNet(block_, mod_net_id);
        mod_iterm->connect(mnet);
      }

      break;
    }
    default: {
      logger_->critical(utl::ODB,
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
          uint32_t source_net_id;
          uint32_t dest_net_id;
          log_.pop(source_net_id);
          log_.pop(dest_net_id);
          // note because we are undoing the source is the prior dest
          //(we are move dest name back to source name).
          dbNet* source_net = dbNet::getNet(block_, dest_net_id);
          dbNet* dest_net = dbNet::getNet(block_, source_net_id);
          debugPrint(logger_,
                     utl::ODB,
                     "DB_ECO",
                     3,
                     "UNDO ECO: swap dbNet name between {} and {}",
                     source_net->getDebugName(),
                     dest_net->getDebugName());
          source_net->swapNetNames(dest_net, false);
          break;
        }
        default: {
          logger_->critical(
              utl::ODB,
              467,
              "No undo_swapObject support for type {} and subtype {}",
              dbObject::getTypeName(obj_type),
              dbObject::getTypeName(sub_obj_type));
        }
      }
      break;
    }

    case dbInstObj: {
      uint32_t inst_id;
      log_.pop(inst_id);

      uint32_t prev_lib_id;
      log_.pop(prev_lib_id);

      uint32_t prev_master_id;
      log_.pop(prev_master_id);

      uint32_t lib_id;
      log_.pop(lib_id);

      uint32_t master_id;
      log_.pop(master_id);

      dbInst* inst = dbInst::getInst(block_, inst_id);
      dbLib* lib = dbLib::getLib(block_->getDb(), prev_lib_id);
      dbMaster* master = dbMaster::getMaster(lib, prev_master_id);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: swap {}",
                 inst->getDebugName());
      inst->swapMaster(master);
      break;
    }

    default: {
      logger_->critical(utl::ODB,
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

    case dbModNetObj:
      undo_updateModNetField();
      break;

    default: {
      logger_->critical(utl::ODB,
                        445,
                        "No undo_updateField support for type {}",
                        dbObject::getTypeName(obj_type));
      break;
    }
  }
}

void dbJournal::undo_updateNetField()
{
  uint32_t net_id;
  log_.pop(net_id);
  _dbNet* net = (_dbNet*) dbNet::getNet(block_, net_id);

  int field;
  log_.pop(field);

  switch ((_dbNet::Field) field) {
    case _dbNet::kFlags: {
      uint32_t* flags = (uint32_t*) &net->flags_;
      log_.pop(*flags);
      uint32_t new_flags;
      log_.pop(new_flags);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: dbNet {}, updateFlags",
                 net_id);
      break;
    }

    case _dbNet::kName: {
      std::string prev_name;
      log_.pop(prev_name);
      std::string new_name;
      log_.pop(new_name);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: dbNet({} {:p}), updateName from '{}' to '{}'",
                 net_id,
                 static_cast<void*>(net),
                 new_name,
                 prev_name);
      ((dbNet*) net)->rename(prev_name.c_str());
      break;
    }

    default: {
      logger_->critical(
          utl::ODB, 408, "No undo_updateNetField support for field {}", field);
      break;
    }
  }
}

void dbJournal::undo_updateModNetField()
{
  uint32_t modnet_id;
  log_.pop(modnet_id);
  _dbModNet* modnet = (_dbModNet*) dbModNet::getModNet(block_, modnet_id);

  int field;
  log_.pop(field);

  switch ((_dbModNet::Field) field) {
    case _dbModNet::kName: {
      std::string prev_name;
      log_.pop(prev_name);
      std::string new_name;
      log_.pop(new_name);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: dbModNet({} {:p}), updateName from '{}' to '{}'",
                 modnet_id,
                 static_cast<void*>(modnet),
                 new_name,
                 prev_name);
      ((dbModNet*) modnet)->rename(prev_name.c_str());
      break;
    }

    default:
      logger_->critical(utl::ODB,
                        424,
                        "No undo_updateModNetField support for field {}",
                        field);
      break;
  }
}

void dbJournal::undo_updateInstField()
{
  uint32_t inst_id;
  log_.pop(inst_id);
  _dbInst* inst = (_dbInst*) dbInst::getInst(block_, inst_id);

  int field;
  log_.pop(field);

  switch ((_dbInst::Field) field) {
    case _dbInst::kFlags: {
      uint32_t* flags = (uint32_t*) &inst->flags_;
      log_.pop(*flags);
      uint32_t new_flags;
      log_.pop(new_flags);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: dbInst {}, updateFlags",
                 inst_id);

      // Changing the orientation flag requires updating the cached bbox
      _dbInstFlags* a = (_dbInstFlags*) flags;
      _dbInstFlags* b = (_dbInstFlags*) &new_flags;
      if (a->orient != b->orient) {
        _dbInst::setInstBBox(inst);
      }
      break;
    }

    case _dbInst::kOrigin: {
      int prev_x;
      log_.pop(prev_x);
      int prev_y;
      log_.pop(prev_y);
      int current_x;
      log_.pop(current_x);
      int current_y;
      log_.pop(current_y);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: dbInst {}, updateOrigin",
                 inst_id);
      ((dbInst*) inst)->setOrigin(prev_x, prev_y);
      break;
    }

    case _dbInst::kName: {
      std::string prev_name;
      log_.pop(prev_name);
      std::string new_name;
      log_.pop(new_name);
      debugPrint(logger_,
                 utl::ODB,
                 "DB_ECO",
                 3,
                 "UNDO ECO: dbInst({} {:p}), updateName from '{}' to '{}'",
                 inst_id,
                 static_cast<void*>(inst),
                 new_name,
                 prev_name);
      ((dbInst*) inst)->rename(prev_name.c_str());
      break;
    }

    default: {
      logger_->critical(
          utl::ODB, 409, "No undo_updateInstField support for field {}", field);
      break;
    }
  }
}

void dbJournal::undo_updateITermField()
{
  uint32_t iterm_id;
  log_.pop(iterm_id);
  //_dbITerm * iterm = (_dbITerm *) dbITerm::getITerm(_block, iterm_id );

  int field;
  log_.pop(field);

  switch ((_dbITerm::Field) field) {
    default: {
      logger_->critical(utl::ODB,
                        410,
                        "No undo_updateITermField support for field {}",
                        field);
      break;
    }
  }
}

void dbJournal::undo_updateRSegField()
{
  uint32_t rseg_id;
  log_.pop(rseg_id);
  //_dbRSeg * rseg = (_dbRSeg *) dbRSeg::getRSeg(_block, rseg_id );

  int field;
  log_.pop(field);

  switch ((_dbRSeg::Field) field) {
    default: {
      logger_->critical(
          utl::ODB, 411, "No undo_updateRSegField support for field {}", field);
    } break;
  }
}

void dbJournal::undo_updateCapNodeField()
{
  uint32_t node_id;
  log_.pop(node_id);
  //_dbCapNode * node = (_dbCapNode *) dbCapNode::getCapNode(_block, node_id
  //);

  int field;
  log_.pop(field);

  switch ((_dbCapNode::Fields) field) {
    default: {
      logger_->critical(utl::ODB,
                        412,
                        "No undo_updateCapNodeField support for field {}",
                        field);
      break;
    }
  }
}

void dbJournal::undo_updateCCSegField()
{
  uint32_t node_id;
  log_.pop(node_id);
  //_dbCCSeg * node = (_dbCCSeg *) dbCCSeg::getCCSeg(_block, node_id );

  int field;
  log_.pop(field);

  switch ((_dbCCSeg::Fields) field) {
    default: {
      logger_->critical(utl::ODB,
                        413,
                        "No undo_updateCCSegField support for field {}",
                        field);

      break;
    }
  }
}

void dbJournal::append(dbJournal* other)
{
  log_.append(other->log_);
}

dbOStream& operator<<(dbOStream& stream, const dbJournal& journal)
{
  stream << journal.log_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, dbJournal& journal)
{
  stream >> journal.log_;
  return stream;
}

}  // namespace odb
