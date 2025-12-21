// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbInsertBuffer.h"

#include "dbInst.h"
#include "dbModule.h"
#include "dbNet.h"
#include "utl/Logger.h"

namespace odb {

dbInsertBuffer::dbInsertBuffer(dbNet* net)
    : net_(net),
      block_(net ? net->getBlock() : nullptr),
      logger_(net ? net->getImpl()->getLogger() : nullptr)
{
}

void dbInsertBuffer::resetMembers()
{
  buf_input_iterm_ = nullptr;
  buf_output_iterm_ = nullptr;
  new_flat_net_ = nullptr;
  new_mod_net_ = nullptr;
  target_module_ = nullptr;
  buffer_master_ = nullptr;
  base_name_ = nullptr;
  uniquify_ = dbNameUniquifyType::ALWAYS;
  orig_drvr_pin_ = nullptr;
}

dbInst* dbInsertBuffer::insertBufferBeforeLoad(
    dbObject* load_input_term,
    const dbMaster* buffer_master,
    const Point* loc,
    const char* base_name,
    const dbNameUniquifyType& uniquify)
{
  return insertBufferSimple(load_input_term,
                            buffer_master,
                            loc,
                            base_name,
                            uniquify,
                            /* insertBefore */ true);
}

dbInst* dbInsertBuffer::insertBufferAfterDriver(
    dbObject* drvr_output_term,
    const dbMaster* buffer_master,
    const Point* loc,
    const char* base_name,
    const dbNameUniquifyType& uniquify)
{
  return insertBufferSimple(drvr_output_term,
                            buffer_master,
                            loc,
                            base_name,
                            uniquify,
                            /* insertBefore */ false);
}

dbInst* dbInsertBuffer::insertBufferSimple(dbObject* term_obj,
                                           const dbMaster* buffer_master,
                                           const Point* loc,
                                           const char* base_name,
                                           const dbNameUniquifyType& uniquify,
                                           bool insertBefore)
{
  if (term_obj == nullptr || buffer_master == nullptr || block_ == nullptr) {
    return nullptr;
  }

  // 1. Check terminal type and connectivity.
  dbModNet* orig_mod_net = nullptr;
  if (!validateTermAndGetModNet(term_obj, orig_mod_net)) {
    return nullptr;
  }

  // 3. Initialize members
  resetMembers();

  target_module_ = (term_obj->getObjectType() == dbITermObj)
                       ? static_cast<dbITerm*>(term_obj)->getInst()->getModule()
                       : nullptr;
  buffer_master_ = buffer_master;
  base_name_ = base_name;
  uniquify_ = uniquify;

  // Store original driver pin before buffer insertion modifies net structure
  orig_drvr_pin_ = net_->getFirstDriverTerm();

  // 4. Check buffer validity and create buffer instance
  dbInst* buffer_inst = checkAndCreateBuffer();
  if (buffer_inst == nullptr) {
    return nullptr;
  }

  // 5. Create new net for one side of the buffer
  std::set<dbObject*> terms;
  terms.insert(term_obj);

  new_flat_net_ = createNewFlatNet(terms);
  if (new_flat_net_ == nullptr) {
    dbInst::destroy(buffer_inst);
    return nullptr;
  }

  // 6. Rewire
  rewireBufferSimple(insertBefore, orig_mod_net, term_obj);

  // 7. Place the new buffer
  placeNewBuffer(buffer_inst, loc, term_obj);

  // 8. Set buffer attributes
  setBufferAttributes(buffer_inst);

  return buffer_inst;
}

dbInst* dbInsertBuffer::insertBufferBeforeLoads(
    std::set<dbObject*>& load_pins,
    const dbMaster* buffer_master,
    const Point* loc,
    const char* base_name,
    const dbNameUniquifyType& uniquify,
    bool loads_on_same_db_net)
{
  // jk: dbg
  static int i = 0;
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "insert_buffer BeforeLoads#{}",
             ++i);

  if (load_pins.empty() || buffer_master == nullptr || block_ == nullptr) {
    return nullptr;
  }

  resetMembers();

  buffer_master_ = buffer_master;
  base_name_ = base_name;
  uniquify_ = uniquify;

  // Store original driver pin before buffer insertion modifies net structure
  orig_drvr_pin_ = net_->getFirstDriverTerm();

  dlogBeforeLoadsParams(load_pins, loc, loads_on_same_db_net);

  // 1. Validate Load Pins & Find Lowest Common Ancestor (Hierarchy)
  std::set<dbNet*> other_dbnets;
  target_module_ = validateLoadPinsAndFindLCA(
      load_pins, other_dbnets, loads_on_same_db_net);
  if (target_module_ == nullptr) {
    return nullptr;  // Validation failed
  }

  // 2. Create Buffer Instance
  dbInst* buffer_inst = checkAndCreateBuffer();
  if (buffer_inst == nullptr) {
    logger_->error(
        utl::ODB, 1205, "BeforeLoads: Failed to create buffer instance.");
    return nullptr;
  }

  // 3. Create Buffer Net
  createNewFlatAndHierNets(load_pins);
  if (new_flat_net_ == nullptr) {
    dbInst::destroy(buffer_inst);
    return nullptr;
  }

  // 4. Rewire load pin connections
  rewireBufferLoadPins(load_pins);

  // 5. Place the Buffer
  placeBufferAtLocation(buffer_inst, loc, load_pins);

  // 6. Set buffer attributes
  setBufferAttributes(buffer_inst);

  dlogInsertBufferSuccess(buffer_inst);
  dlogSeparator();

  return buffer_inst;
}

bool dbInsertBuffer::validateTermAndGetModNet(dbObject* term_obj,
                                              dbModNet*& mod_net) const
{
  mod_net = nullptr;

  // 1. Validate term type and connectivity
  if (term_obj->getObjectType() == dbITermObj) {
    dbITerm* iterm = static_cast<dbITerm*>(term_obj);
    if (iterm->getNet() != net_) {
      return false;  // Not connected to this net
    }

    // Check dont_touch attribute
    if (checkDontTouch(iterm)) {
      return false;
    }

    mod_net = iterm->getModNet();
  } else if (term_obj->getObjectType() == dbBTermObj) {
    dbBTerm* bterm = static_cast<dbBTerm*>(term_obj);
    if (bterm->getNet() != net_) {
      return false;  // Not connected to this net
    }
    mod_net = bterm->getModNet();
  } else {
    return false;  // Invalid term type
  }

  return true;
}

dbInst* dbInsertBuffer::checkAndCreateBuffer()
{
  dbMTerm* input_mterm = nullptr;
  dbMTerm* output_mterm = nullptr;
  for (dbMTerm* mterm : const_cast<dbMaster*>(buffer_master_)->getMTerms()) {
    if (mterm->getIoType() == dbIoType::INPUT) {
      if (input_mterm != nullptr) {
        logger_->warn(utl::ODB,
                      1207,
                      "Buffer master '{}' has more than one input.",
                      buffer_master_->getConstName());
        return nullptr;
      }
      input_mterm = mterm;
    } else if (mterm->getIoType() == dbIoType::OUTPUT) {
      if (output_mterm != nullptr) {
        logger_->warn(utl::ODB,
                      1208,
                      "Buffer master '{}' has more than one output.",
                      buffer_master_->getConstName());
        return nullptr;
      }
      output_mterm = mterm;
    }
  }
  if (input_mterm == nullptr || output_mterm == nullptr) {
    logger_->warn(utl::ODB,
                  1209,
                  "Buffer master '{}' is not a simple buffer.",
                  buffer_master_->getConstName());
    return nullptr;
  }

  // Create a new buffer instance
  const char* inst_base_name = (base_name_) ? base_name_ : "buf";
  dbInst* buffer_inst = dbInst::create(block_,
                                       const_cast<dbMaster*>(buffer_master_),
                                       inst_base_name,
                                       uniquify_,
                                       target_module_);

  buf_input_iterm_ = buffer_inst->findITerm(input_mterm->getConstName());
  buf_output_iterm_ = buffer_inst->findITerm(output_mterm->getConstName());

  return buffer_inst;
}

bool dbInsertBuffer::checkDontTouch(dbITerm* iterm) const
{
  if (net_->isDoNotTouch()) {
    return true;
  }

  if (iterm && iterm->getInst()->isDoNotTouch()) {
    return true;
  }

  return false;
}

void dbInsertBuffer::placeNewBuffer(dbInst* buffer_inst,
                                    const Point* loc,
                                    dbObject* term)
{
  int x = 0;
  int y = 0;
  if (loc) {
    x = loc->getX();
    y = loc->getY();
  } else {
    getPinLocation(term, x, y);
  }

  buffer_inst->setLocation(x, y);
  buffer_inst->setPlacementStatus(dbPlacementStatus::PLACED);
}

void dbInsertBuffer::rewireBufferSimple(bool insertBefore,
                                        dbModNet* orig_mod_net,
                                        dbObject* term)
{
  dbNet* in_net = insertBefore ? net_ : new_flat_net_;
  dbNet* out_net = insertBefore ? new_flat_net_ : net_;
  dbModNet* in_mod_net = insertBefore ? orig_mod_net : nullptr;
  dbModNet* out_mod_net = insertBefore ? nullptr : orig_mod_net;

  // Connect buffer input
  buf_input_iterm_->connect(in_net);
  if (in_mod_net) {
    buf_input_iterm_->connect(in_mod_net);
  }

  // Connect buffer output
  buf_output_iterm_->connect(out_net);
  if (out_mod_net) {
    buf_output_iterm_->connect(out_mod_net);
  }

  // Connect the original terminal (load or driver) to the new net
  if (term->getObjectType() == dbITermObj) {
    dbITerm* iterm = static_cast<dbITerm*>(term);
    iterm->disconnect();  // Disconnect both flat and mod nets
    iterm->connect(new_flat_net_);
  } else if (term->getObjectType() == dbBTermObj) {
    dbBTerm* bterm = static_cast<dbBTerm*>(term);
    bterm->disconnect();
    bterm->connect(new_flat_net_);
  }
}

bool dbInsertBuffer::getPinLocation(dbObject* pin, int& x, int& y) const
{
  if (pin == nullptr) {
    return false;
  }

  if (pin->getObjectType() == dbITermObj) {
    dbITerm* iterm = static_cast<dbITerm*>(pin);
    if (iterm->getAvgXY(&x, &y)) {
      return true;
    }

    // Fallback to instance origin
    dbInst* inst = iterm->getInst();
    Point origin = inst->getOrigin();
    x = origin.getX();
    y = origin.getY();
    return true;
  }

  if (pin->getObjectType() == dbBTermObj) {
    dbBTerm* bterm = static_cast<dbBTerm*>(pin);
    return bterm->getFirstPinLocation(x, y);
  }

  return false;
}

Point dbInsertBuffer::computeCentroid(
    dbObject* drvr_pin,
    const std::set<dbObject*>& load_pins) const
{
  uint64_t sum_x = 0;
  uint64_t sum_y = 0;
  int count = 0;

  // Include driver pin
  int x = 0, y = 0;
  if (getPinLocation(drvr_pin, x, y)) {
    sum_x += x;
    sum_y += y;
    count++;
  }

  // Include load pins
  for (dbObject* pin : load_pins) {
    if (getPinLocation(pin, x, y)) {
      sum_x += x;
      sum_y += y;
      count++;
    }
  }

  if (count == 0) {
    return Point(0, 0);
  }

  return Point(static_cast<int>(sum_x / count),
               static_cast<int>(sum_y / count));
}

dbNet* dbInsertBuffer::createNewFlatNet(std::set<dbObject*>& connected_terms)
{
  // Create a new net for buffering in the target module.
  //
  // Algorithm:
  // - The new net will drive the connected_terms, and the original net will be
  //   connected to the buffer terminal.
  // - If the original net name matches a connected BTerm, rename the original
  //   net to a unique name and assign the BTerm's name to the new net.
  //   This ensures that the net name matches the port name for Verilog
  //   compatibility.

  std::string new_net_name = "net";
  dbNameUniquifyType new_net_uniquify = uniquify_;

  // Check if the net name conflicts with any port name
  for (dbObject* obj : connected_terms) {
    if (obj->getObjectType() != dbBTermObj) {
      continue;
    }

    dbBTerm* bterm = static_cast<dbBTerm*>(obj);
    std::string_view bterm_name{bterm->getConstName()};
    if (bterm_name == block_->getBaseName(net_->getConstName())) {
      // The original net names uses the BTerm name.
      // Rename this net if its name is the same as a port name in loads_pins
      std::string new_orig_net_name = block_->makeNewNetName(
          target_module_ ? target_module_->getModInst() : nullptr,
          "net",
          uniquify_);
      net_->rename(new_orig_net_name.c_str());

      // Rename the mod net name connected to the load pin if it is the
      // same as the port name
      dbModNet* load_mnet = bterm->getModNet();
      if (load_mnet) {
        std::string_view mnet_name{load_mnet->getConstName()};
        if (mnet_name == bterm_name) {
          load_mnet->rename(new_orig_net_name.c_str());
        }
      }

      // New net name should be the port name
      new_net_name = bterm_name;
      new_net_uniquify = dbNameUniquifyType::IF_NEEDED;
      break;
    }
  }

  // Create a new net
  return dbNet::create(
      block_, new_net_name.c_str(), new_net_uniquify, target_module_);
}

std::string dbInsertBuffer::makeUniqueHierName(dbModule* module,
                                               const std::string& base_name,
                                               const char* suffix) const
{
  std::string name = base_name + "_" + suffix;
  int id = 0;
  std::string unique_name = name;
  // Ensure uniqueness against both ModNets and ModBTerms
  while (module->getModNet(unique_name.c_str())
         || module->findModBTerm(unique_name.c_str())) {
    unique_name = name + "_" + std::to_string(id++);
  }
  return unique_name;
}

int dbInsertBuffer::getModuleDepth(dbModule* mod) const
{
  int depth = 0;
  while (mod != nullptr) {
    mod = mod->getModInst() ? mod->getModInst()->getParent() : nullptr;
    depth++;
  }
  return depth;
}

dbModule* dbInsertBuffer::findLCA(dbModule* m1, dbModule* m2) const
{
  if (m1 == nullptr) {
    return m2;
  }
  if (m2 == nullptr) {
    return m1;
  }

  int d1 = getModuleDepth(m1);
  int d2 = getModuleDepth(m2);

  while (d1 > d2) {
    m1 = m1->getModInst() ? m1->getModInst()->getParent() : nullptr;
    d1--;
  }
  while (d2 > d1) {
    m2 = m2->getModInst() ? m2->getModInst()->getParent() : nullptr;
    d2--;
  }

  while (m1 != m2 && m1 != nullptr && m2 != nullptr) {
    m1 = m1->getModInst() ? m1->getModInst()->getParent() : nullptr;
    m2 = m2->getModInst() ? m2->getModInst()->getParent() : nullptr;
  }
  return m1;
}

bool dbInsertBuffer::createHierarchicalConnection(
    dbITerm* load_pin,
    dbITerm* drvr_term,
    const std::set<dbObject*>& load_pins)
{
  dbModule* target_module = drvr_term->getInst()->getModule();
  dbModule* current_module = load_pin->getInst()->getModule();
  dbModITerm* top_mod_iterm = nullptr;

  if (current_module == target_module
      || block_->getDb()->hasHierarchy() == false) {
    top_mod_iterm = nullptr;  // Already in same module, no hierarchy handling
  } else {
    dbObject* load_obj = (dbObject*) load_pin;
    std::string base_name
        = block_->getBaseName(load_pin->getNet()->getConstName());

    // We are connecting FROM the buffer (in target_module) TO the load (deep
    // down). So from the perspective of the hierarchical modules, these are
    // INPUT ports.

    // Visited set to prevent infinite loops in recursion
    std::set<dbModNet*> visited_modnets;

    // Helper lambda to recursively check if all loads connected to a ModNet
    // (directly or through child modules) are in load_pins
    std::function<bool(dbModNet*)> allLoadsAreTargets
        = [&](dbModNet* net) -> bool {
      if (net == nullptr) {
        return true;
      }

      // Cycle detection
      if (visited_modnets.find(net) != visited_modnets.end()) {
        return true;
      }
      visited_modnets.insert(net);

      // 1. Check all directly connected ITerms
      for (dbITerm* iterm : net->getITerms()) {
        if (load_pins.find(iterm) == load_pins.end()) {
          return false;
        }
      }

      // 2. Check all loads reachable through child modules (ModITerms)
      for (dbModITerm* miterm : net->getModITerms()) {
        if (dbModBTerm* child_bterm = miterm->getChildModBTerm()) {
          if (child_bterm->getIoType() == dbIoType::INPUT
              || child_bterm->getIoType() == dbIoType::INOUT) {
            if (dbModNet* child_net = child_bterm->getModNet()) {
              if (allLoadsAreTargets(child_net) == false) {
                return false;
              }
            }
          }
        }
      }

      // 3. Upward Traversal: Check loads reachable through parent module
      for (dbModBTerm* modbterm : net->getModBTerms()) {
        // If the net escapes to the parent (OUTPUT/INOUT), follow the path UP.
        if (modbterm->getIoType() == dbIoType::OUTPUT
            || modbterm->getIoType() == dbIoType::INOUT) {
          if (dbModITerm* parent_moditerm = modbterm->getParentModITerm()) {
            if (dbModNet* parent_net = parent_moditerm->getModNet()) {
              // Recursively check the parent net
              if (allLoadsAreTargets(parent_net) == false) {
                return false;  // Found a non-target load up in the hierarchy!
              }
            }
          }
        }
      }

      return true;
    };

    while (current_module != target_module && current_module != nullptr) {
      dbModInst* parent_mod_inst = current_module->getModInst();
      if (parent_mod_inst == nullptr) {  // current_module is top module
        // This should not happen if the hierarchy is structured correctly,
        // as it implies we've hit the top module before reaching the target.
        logger_->error(
            utl::ODB,
            1206,
            "Cannot create hierarchical connection: '{}' is not a descendant "
            "of '{}'.",
            load_pin->getInst()->getName(),
            target_module->getName());
        break;
      }

      // Check if there's an existing hierarchical connection to reuse.
      dbModNet* existing_mod_net = nullptr;
      if (load_obj->getObjectType() == dbITermObj) {
        existing_mod_net = (static_cast<dbITerm*>(load_obj))->getModNet();
      } else if (load_obj->getObjectType() == dbModITermObj) {
        existing_mod_net = (static_cast<dbModITerm*>(load_obj))->getModNet();
      }

      bool reused_path = false;

      if (existing_mod_net) {
        // Check if we can safely reuse this net.
        // We can reuse ONLY if:
        //   1. All connected ITerms (and loads through child modules) are in
        //   load_pins
        //   2. All connected ModITerms are the child_obj (the path we are
        //   tracing)
        bool safe_to_reuse = true;

        // Use recursive check for all loads (including through child modules)
        visited_modnets.clear();  // Should clear before allLoadsAreTargets call
        if (allLoadsAreTargets(existing_mod_net) == false) {
          safe_to_reuse = false;
        }

        if (safe_to_reuse) {
          // Check if all ModITerms are the load_obj (the path we are tracing)
          for (dbModITerm* miterm : existing_mod_net->getModITerms()) {
            if (miterm != load_obj) {
              safe_to_reuse = false;
              break;
            }
          }
        }

        if (safe_to_reuse) {
          // Check if this net is connected to a boundary port (ModBTerm)
          for (dbModBTerm* modbterm : existing_mod_net->getModBTerms()) {
            if (modbterm->getIoType() != dbIoType::INPUT) {
              continue;
            }

            dbModITerm* parent_iterm = modbterm->getParentModITerm();
            if (!parent_iterm) {
              continue;
            }

            // Found a reusable path. Move up the hierarchy.
            debugPrint(logger_,
                       utl::ODB,
                       "insert_buffer",
                       1,
                       "Reusing existing hierarchical pin '{}'",
                       parent_iterm->getHierarchicalName());
            load_obj = static_cast<dbObject*>(parent_iterm);
            current_module = parent_mod_inst->getParent();
            top_mod_iterm = parent_iterm;
            reused_path = true;
            break;  // Exit the for loop since we found a path
          }
        }
      }

      if (reused_path) {
        continue;  // Continue to the next level up
      }

      // jk: TODO: too complex, need to be optimized. (mark non-reusable modnet
      // checked already or newly punched port is always reusable)

      // Check if there is ANY other ModNet (e.g., modnet created for newly
      // punched port) in this module that we can reuse. This happens when we
      // have multiple loads in the same module (or down the hierarchy) that are
      // being buffered. The first load creates the port, and subsequent loads
      // should reuse it.
      //
      // Optimization: Instead of iterating all ModNets in current_module,
      // we iterate load_pins and trace up to find if any of them is connected
      // to a ModNet in current_module.
      for (dbObject* other_load_obj : load_pins) {
        if (other_load_obj == load_pin) {
          continue;
        }
        if (other_load_obj->getObjectType() != dbITermObj) {
          continue;
        }

        // Trace up from other_load_obj to see if it has a net in current_module
        dbObject* trace_obj = other_load_obj;
        dbModule* trace_module = nullptr;

        if (trace_obj->getObjectType() == dbITermObj) {
          trace_module
              = static_cast<dbITerm*>(trace_obj)->getInst()->getModule();
        }

        while (trace_module != current_module && trace_module != nullptr) {
          // Find the ModNet in trace_module connected to trace_obj
          dbModNet* net = nullptr;
          if (trace_obj->getObjectType() == dbITermObj) {
            net = static_cast<dbITerm*>(trace_obj)->getModNet();
          } else {
            net = static_cast<dbModITerm*>(trace_obj)->getModNet();
          }

          if (!net) {
            trace_obj = nullptr;
            break;
          }  // Broken path

          // Find ModBTerm to go up
          dbModBTerm* bterm = nullptr;
          for (auto* bt : net->getModBTerms()) {
            // Assuming Input for load
            if (bt->getIoType() == dbIoType::INPUT) {
              bterm = bt;
              break;
            }
          }
          if (!bterm) {
            trace_obj = nullptr;
            break;
          }

          dbModITerm* parent_pin = bterm->getParentModITerm();
          if (!parent_pin) {
            trace_obj = nullptr;
            break;
          }

          trace_obj = parent_pin;

          // ModITerm -> ModInst -> Module
          trace_module = parent_pin->getParent()->getParent();
        }

        if (trace_module == current_module && trace_obj != nullptr) {
          // trace_obj is now the object in current_module (ModITerm) that leads
          // to other_load_obj OR if other_load_obj was already in
          // current_module, trace_obj is other_load_obj.

          // Check the net connected to trace_obj in current_module
          dbModNet* mod_net = nullptr;
          if (trace_obj->getObjectType() == dbITermObj) {
            mod_net = static_cast<dbITerm*>(trace_obj)->getModNet();
          } else {
            mod_net = static_cast<dbModITerm*>(trace_obj)->getModNet();
          }

          if (!mod_net) {
            continue;
          }

          // Verify candidate_net
          // 1. Must have a connection to the parent (ModBTerm)
          if (mod_net->getModBTerms().empty()) {
            continue;
          }

          // 2. Must NOT have any connection to ITerms (or loads reachable
          //    through child modules) that are NOT in load_pins
          //    (This ensures we don't accidentally merge with unrelated logic)
          visited_modnets.clear();  // Should clear before allLoadsAreTargets
                                    // jk: enhance this
          if (allLoadsAreTargets(mod_net) == false) {
            continue;
          }

          // Found a candidate!
          // Use the first ModBTerm to go up.
          dbModBTerm* modbterm = nullptr;
          for (dbModBTerm* bterm : mod_net->getModBTerms()) {
            if (bterm->getIoType() == dbIoType::INPUT) {
              modbterm = bterm;
              break;
            }
          }

          if (modbterm) {
            dbModITerm* parent_moditerm = modbterm->getParentModITerm();

            if (parent_moditerm) {
              // Connect current child_obj to this reusable net
              if (load_obj->getObjectType() == dbITermObj) {
                (static_cast<dbITerm*>(load_obj))->connect(mod_net);
              } else if (load_obj->getObjectType() == dbModITermObj) {
                (static_cast<dbModITerm*>(load_obj))->connect(mod_net);
              }

              reused_path = true;

              // Move up
              load_obj = static_cast<dbObject*>(parent_moditerm);
              current_module = parent_mod_inst->getParent();
              top_mod_iterm = parent_moditerm;
              break;
            }
          }
        }
      }
      if (reused_path) {
        continue;
      }

      // Name generation
      const char* suffix = "i";
      std::string unique_name
          = makeUniqueHierName(current_module, base_name, suffix);

      // 1. Create Port (ModBTerm) on current module
      dbModBTerm* mod_bterm
          = dbModBTerm::create(current_module, unique_name.c_str());
      mod_bterm->setIoType(dbIoType::INPUT);

      // 2. Create Net (ModNet) inside current module
      dbModNet* mod_net = dbModNet::create(current_module, unique_name.c_str());
      mod_bterm->connect(mod_net);

      // 3. Connect lower level object (either leaf ITerm or previous ModITerm)
      if (load_obj->getObjectType() == dbITermObj) {
        dbITerm* load_iterm = static_cast<dbITerm*>(load_obj);
        // load_iterm->disconnect();  // jk: not ok. STA assert fail.
        load_iterm->connect(mod_net);
      } else if (load_obj->getObjectType() == dbModITermObj) {
        (static_cast<dbModITerm*>(load_obj))->connect(mod_net);
      }

      // 4. Create Pin (ModITerm) on the instance of current module in the
      // parent
      dbModITerm* mod_iterm
          = dbModITerm::create(parent_mod_inst, unique_name.c_str(), mod_bterm);

      // Prepare for next iteration (moving up)
      load_obj = static_cast<dbObject*>(mod_iterm);
      current_module = parent_mod_inst->getParent();
      top_mod_iterm = mod_iterm;
    }
  }

  // Perform connections
  // 1. Disconnect the load from the original net
  if (top_mod_iterm) {
    // If we established a hierarchical connection, we only want to disconnect
    // the flat net, preserving the ModNet connection we just made (or reused).
    load_pin->disconnectDbNet();
  } else {
    // If no hierarchical connection (same module), disconnect everything
    // as we will connect to a new ModNet (if applicable) in this module.
    load_pin->disconnect();
  }

  // 2. Connect load to the new ModNet
  // - IMPORTANT: dbSta prioritizes hier net connectino over flat net
  //   connection. So hier net connection should be edited first.
  //   otherwise, STA caches may not be updated correctly.
  if (dbModNet* drvr_mod_net = drvr_term->getModNet()) {
    if (top_mod_iterm) {
      // Connect the top-most hierarchical pin to the buffer's output ModNet
      top_mod_iterm->disconnect();  // jk: needed???
      top_mod_iterm->connect(drvr_mod_net);
    } else {
      // Load is in the same module as buffer, connect to ModNet if it exists
      load_pin->disconnect();  // jk: needed???
      load_pin->connect(drvr_mod_net);
    }
  }

  // 3. Connect load to the new flat net
  load_pin->connect(drvr_term->getNet());

  return top_mod_iterm != nullptr;
}

dbModNet* dbInsertBuffer::getFirstModNetInFaninOfLoads(
    const std::set<dbObject*>& load_pins,
    const std::set<dbModNet*>& modnets_in_target_module) const
{
  for (dbObject* load : load_pins) {
    dbModNet* curr_mod_net = nullptr;

    // 1. Get the modnet of the load
    if (load->getObjectType() == dbITermObj) {
      dbITerm* iterm = static_cast<dbITerm*>(load);
      curr_mod_net = iterm->getModNet();
    } else if (load->getObjectType() == dbBTermObj) {
      curr_mod_net = static_cast<dbBTerm*>(load)->getModNet();
    } else {
      continue;
    }

    if (curr_mod_net == nullptr) {
      continue;
    }

    // 2. Find the modnet in the target module by fanin traversal
    while (curr_mod_net) {
      // Found the modnet in the target module
      if (modnets_in_target_module.find(curr_mod_net)
          != modnets_in_target_module.end()) {
        return curr_mod_net;
      }

      // Go up to the next modnet in the fanin
      curr_mod_net = curr_mod_net->getNextModNetInFanin();
    }
  }

  return nullptr;
}

dbModNet* dbInsertBuffer::getFirstDriverModNetInTargetModule(
    const std::set<dbModNet*>& modnets_in_target_module) const
{
  // 1. Find the driver terminal of this flat net
  dbObject* driver_term = net_->getFirstDriverTerm();
  if (driver_term == nullptr) {
    return nullptr;
  }

  // 2. Get the driver's modnet
  dbModNet* curr_modnet = nullptr;
  if (driver_term->getObjectType() == dbITermObj) {
    curr_modnet = static_cast<dbITerm*>(driver_term)->getModNet();
  } else if (driver_term->getObjectType() == dbBTermObj) {
    curr_modnet = static_cast<dbBTerm*>(driver_term)->getModNet();
  }

  // 3. Traverse fanout from driver's modnet to find target module's modnet
  while (curr_modnet) {
    // Found the first modnet in the target module
    if (modnets_in_target_module.contains(curr_modnet)) {
      return curr_modnet;
    }
    curr_modnet = curr_modnet->getNextModNetInFanout();
  }

  return nullptr;
}

void dbInsertBuffer::hierarchicalConnect(dbITerm* driver, dbITerm* load)
{
  dbModule* driver_mod = driver->getInst()->getModule();
  dbModule* load_mod = load->getInst()->getModule();

  auto getModNet = [](dbObject* obj) -> dbModNet* {
    if (obj->getObjectType() == dbITermObj) {
      return static_cast<dbITerm*>(obj)->getModNet();
    }
    if (obj->getObjectType() == dbModITermObj) {
      return static_cast<dbModITerm*>(obj)->getModNet();
    }
    return nullptr;
  };

  auto ensureFlatNetConnection = [&](dbITerm* driver, dbITerm* load) {
    dbNet* driver_net = driver->getNet();
    dbNet* load_net = load->getNet();
    if (driver_net && load_net) {
      if (driver_net != load_net) {
        driver_net->mergeNet(load_net);
      }
    } else if (driver_net) {
      load->connect(driver_net);
    } else if (load_net) {
      driver->connect(load_net);
    } else {
      std::string base_name = driver->getMTerm()->getName();
      dbNet* new_net = dbNet::create(block_, base_name.c_str());
      if (!new_net) {
        // Fallback for name collision
        new_net = dbNet::create(block_, (base_name + "_conn").c_str());
      }
      if (new_net) {
        driver->connect(new_net);
        load->connect(new_net);
      }
    }

    // Check if flat net name matches any of hierarchical net names
    dbNet* flat_net = driver->getNet();
    if (flat_net) {
      bool name_match = false;
      const char* flat_name = flat_net->getConstName();

      // efficient check
      if (dbModNet* mod_net = block_->findModNet(flat_name)) {
        for (dbITerm* iterm : flat_net->getITerms()) {
          if (iterm->getModNet() == mod_net) {
            name_match = true;
            break;
          }
        }
        if (!name_match) {
          for (dbBTerm* bterm : flat_net->getBTerms()) {
            if (bterm->getModNet() == mod_net) {
              name_match = true;
              break;
            }
          }
        }
      }

      if (!name_match) {
        // Rename to something standard. try driver's mod net first
        dbModNet* driver_mod_net = getModNet(driver);
        if (driver_mod_net) {
          flat_net->rename(driver_mod_net->getHierarchicalName().c_str());
        } else if (dbModNet* load_mod_net = getModNet(load)) {
          flat_net->rename(load_mod_net->getHierarchicalName().c_str());
        }
      }
    }
  };

  // Simple case (driver and load are in the same module)
  // - Connect the modnet.
  if (driver_mod == load_mod) {
    dbModNet* driver_net = driver->getModNet();
    dbModNet* load_net = load->getModNet();

    if (driver_net && load_net) {
      if (driver_net != load_net) {
        driver_net->mergeModNet(load_net);
      }
    } else if (driver_net) {
      load->connect(driver_net);
    } else if (load_net) {
      driver->connect(load_net);
    } else {
      std::string base_name = driver->getMTerm()->getName();
      std::string name = makeUniqueHierName(driver_mod, base_name, "conn");
      dbModNet* new_net = dbModNet::create(driver_mod, name.c_str());
      driver->connect(new_net);
      load->connect(new_net);
    }
    ensureFlatNetConnection(driver, load);
    return;
  }

  // Complex case (driver and load are in different modules)
  // - Find the LCA of the driver and load modules.
  // - Connect the modnet of the driver to the modnet of the load.
  dbModule* lca = findLCA(driver_mod, load_mod);

  auto ensureModNet
      = [&](dbObject* obj, dbModule* mod, const char* suffix) -> dbModNet* {
    dbModNet* mod_net = getModNet(obj);
    if (!mod_net) {
      std::string base_name;
      if (obj->getObjectType() == dbITermObj) {
        base_name = static_cast<dbITerm*>(obj)->getMTerm()->getName();
      } else {
        base_name
            = static_cast<dbModITerm*>(obj)->getChildModBTerm()->getName();
      }
      std::string name = makeUniqueHierName(mod, base_name, suffix);
      mod_net = dbModNet::create(mod, name.c_str());
      if (obj->getObjectType() == dbITermObj) {
        dbITerm* iterm = static_cast<dbITerm*>(obj);
        iterm->connect(mod_net);
        // Connect other ITerms on the same flat net within the same module
        if (dbNet* flat_net = iterm->getNet()) {
          for (dbITerm* peer_iterm : flat_net->getITerms()) {
            if (peer_iterm->getInst()->getModule() == mod
                && peer_iterm != iterm) {
              peer_iterm->connect(mod_net);
            }
          }
        }
      } else {
        static_cast<dbModITerm*>(obj)->connect(mod_net);
      }
    }
    return mod_net;
  };

  auto traceUp = [&](dbObject* current_obj,
                     dbModule* current_mod,
                     dbModule* target_mod,
                     dbIoType io_type,
                     const char* suffix) -> dbObject* {
    while (current_mod != target_mod) {
      dbModNet* mod_net = ensureModNet(current_obj, current_mod, "net");

      dbModBTerm* port = nullptr;
      for (dbModBTerm* bterm : mod_net->getModBTerms()) {
        if (bterm->getIoType() == io_type) {
          port = bterm;
          break;
        }
      }
      if (!port) {
        std::string port_name = mod_net->getName();
        port = dbModBTerm::create(mod_net->getParent(), port_name.c_str());
        if (!port) {
          std::string unique_port_name
              = makeUniqueHierName(current_mod, port_name, suffix);
          port = dbModBTerm::create(mod_net->getParent(),
                                    unique_port_name.c_str());
        }
        port->setIoType(io_type);
        port->connect(mod_net);
        if (dbModInst* mod_inst = current_mod->getModInst()) {
          dbModITerm::create(mod_inst, port->getName(), port);
        }
      }

      current_obj = port->getParentModITerm();
      current_mod = current_mod->getModInst()->getParent();
    }
    return current_obj;
  };

  // Trace UP from driver to LCA
  dbObject* driver_lca_obj
      = traceUp(driver, driver_mod, lca, dbIoType::OUTPUT, "out");

  // Trace UP from load to LCA
  dbObject* load_lca_obj = traceUp(load, load_mod, lca, dbIoType::INPUT, "in");

  // Connect at LCA
  dbModNet* net1 = getModNet(driver_lca_obj);
  dbModNet* net2 = getModNet(load_lca_obj);
  dbModNet* lca_net = nullptr;

  if (net1) {
    lca_net = net1;
  } else if (net2) {
    lca_net = net2;
  } else {
    // Determine base name for new net
    std::string base_name;
    if (driver_lca_obj->getObjectType() == dbModITermObj) {
      base_name = static_cast<dbModITerm*>(driver_lca_obj)
                      ->getChildModBTerm()
                      ->getName();
    } else {  // driver_lca_obj is dbITerm
      base_name = static_cast<dbITerm*>(driver_lca_obj)->getMTerm()->getName();
    }
    std::string name = makeUniqueHierName(lca, base_name, "conn");
    lca_net = dbModNet::create(lca, name.c_str());
  }

  if (driver_lca_obj->getObjectType() == dbITermObj) {
    static_cast<dbITerm*>(driver_lca_obj)->connect(lca_net);
  } else {
    static_cast<dbModITerm*>(driver_lca_obj)->connect(lca_net);
  }

  if (load_lca_obj->getObjectType() == dbITermObj) {
    static_cast<dbITerm*>(load_lca_obj)->connect(lca_net);
  } else {
    static_cast<dbModITerm*>(load_lca_obj)->connect(lca_net);
  }

  // Check and create flat net connection if needed
  ensureFlatNetConnection(driver, load);
}

dbModule* dbInsertBuffer::validateLoadPinsAndFindLCA(
    std::set<dbObject*>& load_pins,
    std::set<dbNet*>& other_dbnets,
    bool loads_on_same_db_net) const
{
  dbModule* target_module = nullptr;
  bool first = true;

  for (dbObject* load_obj : load_pins) {
    dlogTargetLoadPin(load_obj);

    if (load_obj == nullptr) {
      continue;
    }

    dbModule* curr_mod = nullptr;
    dbNet* load_net = nullptr;
    std::string load_name;

    // 1. Extract type-specific values
    if (load_obj->getObjectType() == dbITermObj) {
      dbITerm* iterm = static_cast<dbITerm*>(load_obj);
      load_net = iterm->getNet();
      load_name = iterm->getName();
      curr_mod = iterm->getInst()->getModule();

      // dont_touch check
      if (checkDontTouch(iterm)) {
        logger_->warn(utl::ODB,
                      1201,
                      "BeforeLoads: Load pin {} or "
                      "net is dont_touch.",
                      load_name);
        return nullptr;
      }
    } else if (load_obj->getObjectType() == dbBTermObj) {
      dbBTerm* bterm = static_cast<dbBTerm*>(load_obj);
      load_net = bterm->getNet();
      load_name = bterm->getName();
      curr_mod = block_->getTopModule();
      // iterm_for_check remains nullptr for BTerm
    } else {
      logger_->error(
          utl::ODB, 1204, "BeforeLoads: Load object is not an ITerm or BTerm.");
      return nullptr;
    }

    // 2. Common logic: connectivity check
    if (load_net != net_) {
      other_dbnets.insert(load_net);
      dlogDifferentDbNet(load_net->getName());

      if (loads_on_same_db_net) {
        logger_->error(utl::ODB,
                       1200,
                       "BeforeLoads: Load pin {} is "
                       "not connected to net {}",
                       load_name,
                       net_->getName());
        return nullptr;
      }
    }

    // 4. Find LCA (Lowest Common Ancestor)
    if (first) {
      target_module = curr_mod;
      first = false;
    } else {
      target_module = findLCA(target_module, curr_mod);
    }
  }

  dlogLCAModule(target_module);
  dlogDumpNets(other_dbnets);

  if (target_module == nullptr) {
    target_module = block_->getTopModule();
  }

  return target_module;
}

void dbInsertBuffer::createNewFlatAndHierNets(std::set<dbObject*>& load_pins)
{
  // Create a new flat net
  std::set<dbObject*> connected_terms;
  connected_terms.insert(load_pins.begin(), load_pins.end());
  new_flat_net_ = createNewFlatNet(connected_terms);
  if (new_flat_net_ == nullptr) {
    return;
  }

  // Check if we need to create a mod net
  // - If the new buffer module and the terminal module are different, new mod
  //   net is needed.
  bool needs_mod_net = false;
  if (net_->getDb()->hasHierarchy()) {
    for (dbObject* load_obj : load_pins) {
      if (load_obj->getObjectType() == dbBTermObj
          && target_module_ != block_->getTopModule()) {
        needs_mod_net = true;
        break;
      }

      if (load_obj->getObjectType() == dbITermObj) {
        dbITerm* load = static_cast<dbITerm*>(load_obj);
        if (load->getInst()->getModule() != target_module_) {
          needs_mod_net = true;
          break;
        }
      }
    }
  }

  // Create a new mod net if needed
  new_mod_net_ = nullptr;
  if (needs_mod_net) {
    const char* base_name = block_->getBaseName(new_flat_net_->getConstName());
    dlogCreatingNewHierNet(base_name);
    new_mod_net_ = dbModNet::create(target_module_, base_name);
  }
}

void dbInsertBuffer::rewireBufferLoadPins(std::set<dbObject*>& load_pins)
{
  // 1. Connect Buffer Input to the Original Net
  buf_input_iterm_->connect(net_);
  dlogConnectedBufferInputFlat();

  // Also connect to ModNet if it exists in this module
  if (block_->getDb()->hasHierarchy()) {
    std::set<dbModNet*> related_modnets;
    net_->findRelatedModNets(related_modnets);
    dbModNet* orig_mod_net = nullptr;
    std::set<dbModNet*> modnets_in_target_module;
    for (dbModNet* modnet : related_modnets) {
      if (modnet->getParent() == target_module_) {
        modnets_in_target_module.insert(modnet);
      }
    }

    if (modnets_in_target_module.size() > 1) {
      orig_mod_net
          = getFirstDriverModNetInTargetModule(modnets_in_target_module);
    } else if (modnets_in_target_module.size() == 1) {
      orig_mod_net = *modnets_in_target_module.begin();
    }

    if (orig_mod_net) {
      buf_input_iterm_->connect(orig_mod_net);
      dlogConnectedBufferInputHier(orig_mod_net);
    } else {
      dbObject* drvr = net_->getFirstDriverTerm();
      if (drvr && drvr->getObjectType() == dbITermObj) {
        dbITerm* drvr_iterm = static_cast<dbITerm*>(drvr);
        if (drvr_iterm->getInst()->getModule() != target_module_) {
          hierarchicalConnect(drvr_iterm, buf_input_iterm_);
          dlogConnectedBufferInputViaHierConnect(drvr_iterm);
        }
      }
    }
  }

  // 2. Connect Buffer Output to the New Net (Flat & Hier)
  buf_output_iterm_->connect(new_flat_net_);
  if (new_mod_net_) {
    buf_output_iterm_->connect(new_mod_net_);
  }
  dlogConnectedBufferOutput();

  // 3. Move Target Loads to the New Net
  int num_loads = load_pins.size();
  int load_idx = 0;
  for (dbObject* load_obj : load_pins) {
    if (load_obj->getObjectType() == dbITermObj) {
      dbITerm* load = static_cast<dbITerm*>(load_obj);
      dlogCreatingHierConn(load_idx, num_loads, load);
      createHierarchicalConnection(load, buf_output_iterm_, load_pins);
    } else {
      assert(load_obj->getObjectType() == dbBTermObj);
      dbBTerm* load = static_cast<dbBTerm*>(load_obj);
      load->disconnect();
      load->connect(new_flat_net_, new_mod_net_);
      dlogMovedBTermLoad(load_idx, num_loads, load);
    }
    load_idx++;
  }
}

void dbInsertBuffer::placeBufferAtLocation(dbInst* buffer_inst,
                                           const Point* loc,
                                           std::set<dbObject*>& load_pins)
{
  Point placement_loc;
  if (loc) {
    placement_loc = *loc;
  } else {
    placement_loc = computeCentroid(orig_drvr_pin_, load_pins);
  }
  dlogPlacingBuffer(buffer_inst, placement_loc);
  placeNewBuffer(buffer_inst, &placement_loc, nullptr);
}

void dbInsertBuffer::setBufferAttributes(dbInst* buffer_inst)
{
  new_flat_net_->setSigType(net_->getSigType());
  buffer_inst->setSourceType(dbSourceType::TIMING);

  if (dbTechNonDefaultRule* ndr = net_->getNonDefaultRule()) {
    new_flat_net_->setNonDefaultRule(ndr);
  }
}

void dbInsertBuffer::dlogBeforeLoadsParams(const std::set<dbObject*>& load_pins,
                                           const Point* loc,
                                           bool loads_on_same_db_net) const
{
  debugPrint(
      logger_,
      utl::ODB,
      "insert_buffer",
      1,
      "BeforeLoads: Try inserting a buffer on dbNet={}, load_pins_size={}, "
      "buffer_master={}, loc=({}, {}), base_name={}, uniquify={}, "
      "loads_on_same_db_net={}",
      net_->getName(),
      load_pins.size(),
      buffer_master_->getName(),
      loc ? loc->getX() : -1,
      loc ? loc->getY() : -1,
      base_name_ ? base_name_ : "buf",
      static_cast<int>(uniquify_),
      loads_on_same_db_net);
}

void dbInsertBuffer::dlogTargetLoadPin(dbObject* load_obj) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads:   target load pin: {}",
             load_obj->getName());
}

void dbInsertBuffer::dlogDifferentDbNet(const std::string& net_name) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads:   * is on different dbNet '{}'",
             net_name);
}

void dbInsertBuffer::dlogLCAModule(dbModule* target_module) const
{
  if (logger_->debugCheck(utl::ODB, "insert_buffer", 1)) {
    dbModInst* target_mod_inst
        = target_module ? target_module->getModInst() : nullptr;
    debugPrint(logger_,
               utl::ODB,
               "insert_buffer",
               1,
               "BeforeLoads: LCA module: {} '{}'",
               target_module ? target_module->getName() : "null",
               target_mod_inst ? target_mod_inst->getHierarchicalName()
                               : "<null_inst>");
  }
}

void dbInsertBuffer::dlogDumpNets(const std::set<dbNet*>& other_dbnets) const
{
  if (logger_->debugCheck(utl::ODB, "insert_buffer", 2)) {
    debugPrint(logger_, utl::ODB, "insert_buffer", 2, "[Dump this dbNet]");
    net_->dump(true);

    int other_dbnet_idx = 0;
    for (dbNet* other_dbnet : other_dbnets) {
      debugPrint(logger_,
                 utl::ODB,
                 "insert_buffer",
                 2,
                 "[Dump other dbNet {}]",
                 other_dbnet_idx++);
      other_dbnet->dump(true);
    }
  }
}

void dbInsertBuffer::dlogCreatingNewHierNet(const char* base_name) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Creating new hierarchical net "
             "(dbModNet) with base name '{}' in module '{}'",
             base_name,
             target_module_->getName());
}

void dbInsertBuffer::dlogConnectedBufferInputFlat() const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Connected buffer input '{}' to original "
             "flat net '{}'",
             buf_input_iterm_->getName(),
             net_->getName());
}

void dbInsertBuffer::dlogConnectedBufferInputHier(dbModNet* orig_mod_net) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Connected buffer input '{}' to "
             "original hierarchical net '{}'",
             buf_input_iterm_->getName(),
             orig_mod_net->getHierarchicalName());
}

void dbInsertBuffer::dlogConnectedBufferInputViaHierConnect(
    dbITerm* drvr_iterm) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Connected buffer input '{}' to "
             "original hierarchical net '{}' via hierarchicalConnect",
             buf_input_iterm_->getName(),
             drvr_iterm->getName());
}

void dbInsertBuffer::dlogConnectedBufferOutput() const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Connected buffer output '{}' to new "
             "flat net '{}'{}",
             buf_output_iterm_->getName(),
             new_flat_net_->getName(),
             new_mod_net_ ? fmt::format(" and new hierarchical net '{}'",
                                        new_mod_net_->getHierarchicalName())
                          : "");
}

void dbInsertBuffer::dlogCreatingHierConn(int load_idx,
                                          int num_loads,
                                          dbITerm* load) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: [load {}/{}] Creating hierarchical "
             "connection for load '{}'",
             load_idx + 1,
             num_loads,
             load->getName());
}

void dbInsertBuffer::dlogMovedBTermLoad(int load_idx,
                                        int num_loads,
                                        dbBTerm* load) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: [load {}/{}] Moved BTerm load '{}' "
             "to new flat net '{}'{}",
             load_idx + 1,
             num_loads,
             load->getName(),
             new_flat_net_->getName(),
             new_mod_net_ ? fmt::format(" and new hierarchical net '{}'",
                                        new_mod_net_->getHierarchicalName())
                          : "");
}

void dbInsertBuffer::dlogPlacingBuffer(dbInst* buffer_inst,
                                       const Point& loc) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Placing buffer '{}' at ({}, {})",
             buffer_inst->getName(),
             loc.getX(),
             loc.getY());
}

void dbInsertBuffer::dlogInsertBufferSuccess(dbInst* buffer_inst) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Successfully inserted a new buffer '{}'",
             buffer_inst->getName());
}

void dbInsertBuffer::dlogSeparator() const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "-------------------------------------------------------");
}

}  // namespace odb
