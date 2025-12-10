// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbNet.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "dbBPin.h"
#include "dbBTerm.h"
#include "dbBTermItr.h"
#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCCSegItr.h"
#include "dbCapNode.h"
#include "dbCapNodeItr.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbGroup.h"
#include "dbGuide.h"
#include "dbGuideItr.h"
#include "dbITerm.h"
#include "dbITermItr.h"
#include "dbInst.h"
#include "dbJournal.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbNetTrack.h"
#include "dbNetTrackItr.h"
#include "dbRSeg.h"
#include "dbRSegItr.h"
#include "dbSWire.h"
#include "dbSWireItr.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechNonDefaultRule.h"
#include "dbWire.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbExtControl.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/dbUtil.h"
#include "odb/geom.h"
#include "odb/odb.h"
#include "utl/Logger.h"

namespace odb {

namespace {

// Helper function to check terminal type and connectivity.
bool checkAndGetTerm(dbNet* net,
                     dbObject* term_obj,
                     dbITerm*& iterm,
                     dbBTerm*& bterm,
                     dbModNet*& mod_net)
{
  iterm = nullptr;
  bterm = nullptr;
  mod_net = nullptr;

  if (term_obj->getObjectType() == dbITermObj) {
    iterm = (dbITerm*) term_obj;
    if (iterm->getNet() != net) {
      return false;  // Not connected to this net
    }
    mod_net = iterm->getModNet();
  } else if (term_obj->getObjectType() == dbBTermObj) {
    bterm = (dbBTerm*) term_obj;
    if (bterm->getNet() != net) {
      return false;  // Not connected to this net
    }
    mod_net = bterm->getModNet();
  } else {
    return false;  // Invalid term type
  }
  return true;
}

// Helper function to check buffer master, create a buffer instance, and get
// iterms.
dbInst* checkAndCreateBuffer(dbBlock* block,
                             const dbMaster* buffer_master,
                             const char* base_name,
                             const dbNameUniquifyType& uniquify,
                             dbModule* parent_mod,
                             dbITerm*& buf_input_iterm,
                             dbITerm*& buf_output_iterm)
{
  dbMTerm* input_mterm = nullptr;
  dbMTerm* output_mterm = nullptr;
  for (dbMTerm* mterm : const_cast<dbMaster*>(buffer_master)->getMTerms()) {
    if (mterm->getIoType() == dbIoType::INPUT) {
      if (input_mterm != nullptr) {
        return nullptr;  // More than one input
      }
      input_mterm = mterm;
    } else if (mterm->getIoType() == dbIoType::OUTPUT) {
      if (output_mterm != nullptr) {
        return nullptr;  // More than one output
      }
      output_mterm = mterm;
    }
  }
  if (input_mterm == nullptr || output_mterm == nullptr) {
    // Not a simple buffer
    return nullptr;
  }

  const char* inst_base_name = (base_name) ? base_name : "buf";
  dbInst* buffer_inst = dbInst::create(block,
                                       const_cast<dbMaster*>(buffer_master),
                                       inst_base_name,
                                       uniquify,
                                       parent_mod);

  buf_input_iterm = buffer_inst->findITerm(input_mterm->getConstName());
  buf_output_iterm = buffer_inst->findITerm(output_mterm->getConstName());

  return buffer_inst;
}

// Helper function to check if the net or any of its connected instances
// are marked as "don't touch".
bool checkDontTouch(dbNet* net, dbITerm* drvr_iterm, dbITerm* load_iterm)
{
  if (net->isDoNotTouch()) {
    return true;
  }

  if (drvr_iterm && drvr_iterm->getInst()->isDoNotTouch()) {
    return true;
  }

  if (load_iterm && load_iterm->getInst()->isDoNotTouch()) {
    return true;
  }

  return false;
}

void placeNewBuffer(dbInst* buffer_inst,
                    const Point* loc,
                    dbITerm* term,
                    dbBTerm* bterm)
{
  int x = 0;
  int y = 0;
  if (loc) {
    x = loc->getX();
    y = loc->getY();
  } else {
    if (term) {
      Point origin = term->getInst()->getOrigin();
      x = origin.getX();
      y = origin.getY();
    } else {  // bterm
      auto bpins = bterm->getBPins();
      if (bpins.empty() == false) {
        dbBPin* first_bpin = *bpins.begin();
        Rect box = first_bpin->getBBox();
        x = box.xMin();
        y = box.yMin();
      }
    }
  }

  buffer_inst->setLocation(x, y);
  buffer_inst->setPlacementStatus(dbPlacementStatus::PLACED);
}

void rewireBuffer(bool insertBefore,
                  dbITerm* buf_input_iterm,
                  dbITerm* buf_output_iterm,
                  dbNet* orig_net,
                  dbModNet* orig_mod_net,
                  dbNet* new_net,
                  dbITerm* term_iterm,
                  dbBTerm* term_bterm)
{
  dbNet* in_net = insertBefore ? orig_net : new_net;
  dbNet* out_net = insertBefore ? new_net : orig_net;
  dbModNet* in_mod_net = insertBefore ? orig_mod_net : nullptr;
  dbModNet* out_mod_net = insertBefore ? nullptr : orig_mod_net;

  // Connect buffer input
  buf_input_iterm->connect(in_net);
  if (in_mod_net) {
    buf_input_iterm->connect(in_mod_net);
  }

  // Connect buffer output
  buf_output_iterm->connect(out_net);
  if (out_mod_net) {
    buf_output_iterm->connect(out_mod_net);
  }

  // Connect the original terminal (load or driver) to the new net
  if (term_iterm) {
    term_iterm->disconnect();
    term_iterm->connect(new_net);
  } else {  // term_bterm
    term_bterm->disconnect();
    term_bterm->connect(new_net);
  }
}

Point computeCentroid(const std::set<dbObject*>& pins)
{
  uint64_t sum_x = 0;
  uint64_t sum_y = 0;
  int count = 0;

  for (dbObject* term_obj : pins) {
    if (term_obj->getObjectType() != dbITermObj) {
      continue;
    }
    dbITerm* term = static_cast<dbITerm*>(term_obj);
    int x = 0, y = 0;
    if (term->getAvgXY(&x, &y)) {
      sum_x += x;
      sum_y += y;
      count++;
    } else {
      dbInst* inst = term->getInst();
      Point origin = inst->getOrigin();
      sum_x += origin.getX();
      sum_y += origin.getY();
      count++;
    }
  }

  if (count == 0) {
    return Point(0, 0);
  }

  return Point(static_cast<int>(sum_x / count),
               static_cast<int>(sum_y / count));
}

}  // anonymous namespace

dbNet* dbNet::createBufferNet(dbBTerm* bterm,
                              const char* suffix,
                              dbModNet* mod_net,
                              dbModule* parent_mod,
                              const dbNameUniquifyType& uniquify)
{
  dbBlock* block = getBlock();
  if (bterm == nullptr) {
    // If not connecting to a BTerm, just append the suffix.
    const std::string new_net_name_str = std::string(getName()) + suffix;
    return dbNet::create(block, new_net_name_str.c_str(), uniquify, parent_mod);
  }

  // If the connected term is a port, the new net should take the name of the
  // port to maintain connectivity.
  const char* port_name = bterm->getConstName();

  // If the original net name is the same as the port name, it should be
  // renamed to avoid conflict.
  if (std::string_view(block->getBaseName(getConstName())) == port_name) {
    const std::string new_orig_net_name = block->makeNewNetName(
        parent_mod ? parent_mod->getModInst() : nullptr, "net", uniquify);
    rename(new_orig_net_name.c_str());

    // Rename modnet if it has name conflict.
    if (mod_net && std::string_view(mod_net->getConstName()) == port_name) {
      mod_net->rename(new_orig_net_name.c_str());
    }
  }

  // The new net takes the name of the port.
  // The uniquify parameter is not strictly needed here if we assume port names
  // are unique, but we pass it for safety.
  return dbNet::create(
      block, port_name, dbNameUniquifyType::IF_NEEDED, parent_mod);
}

// Helper to generate unique name for port punching. jk: inefficient
std::string dbNet::makeUniqueHierName(dbModule* module,
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

// Helper: Calculate module depth
int dbNet::getModuleDepth(dbModule* mod) const
{
  int depth = 0;
  while (mod != nullptr) {
    mod = mod->getModInst() ? mod->getModInst()->getParent() : nullptr;
    depth++;
  }
  return depth;
}

// Helper: Find LCA (Lowest Common Ancestor)
dbModule* dbNet::findLCA(dbModule* m1, dbModule* m2) const
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

// jk: TODO: move
// Create hierarchical pins/nets bottom-up from leaf to target_module.
// This corresponds to "Port Punching".
//
// load_pin: The leaf pin deep in hierarchy.
// target_module: The module where the buffer is placed (top of the connection
// chain). top_mod_iterm: Output parameter, returns the top-most ModITerm
// connected to target_module.
bool dbNet::createHierarchicalConnection(dbITerm* load_pin,
                                         dbITerm* drvr_term,
                                         const std::set<dbObject*>& load_pins)
{
  dbModule* target_module = drvr_term->getInst()->getModule();
  dbBlock* block = load_pin->getBlock();
  dbModule* current_module = load_pin->getInst()->getModule();
  dbModITerm* top_mod_iterm = nullptr;

  if (current_module == target_module
      || block->getDb()->hasHierarchy() == false) {
    top_mod_iterm = nullptr;  // Already in same module, no hierarchy handling
  } else {
    dbObject* load_obj = (dbObject*) load_pin;
    std::string base_name
        = block->getBaseName(load_pin->getNet()->getConstName());

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
        block->getImpl()->getLogger()->error(
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
            debugPrint(getImpl()->getLogger(),
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
          trace_module
              = parent_pin->getParent()->getParent();  // ModITerm -> ModInst ->
                                                       // Module
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

template class dbTable<_dbNet>;

_dbNet::_dbNet(_dbDatabase* db, const _dbNet& n)
    : flags_(n.flags_),
      name_(nullptr),
      next_entry_(n.next_entry_),
      iterms_(n.iterms_),
      bterms_(n.bterms_),
      wire_(n.wire_),
      global_wire_(n.global_wire_),
      swires_(n.swires_),
      cap_nodes_(n.cap_nodes_),
      r_segs_(n.r_segs_),
      non_default_rule_(n.non_default_rule_),
      guides_(n.guides_),
      tracks_(n.tracks_),
      groups_(n.groups_),
      weight_(n.weight_),
      xtalk_(n.xtalk_),
      cc_adjust_factor_(n.cc_adjust_factor_),
      cc_adjust_order_(n.cc_adjust_order_)

{
  if (n.name_) {
    name_ = safe_strdup(n.name_);
  }
  driving_iterm_ = -1;
}

_dbNet::_dbNet(_dbDatabase* db)
{
  flags_.sig_type = dbSigType::SIGNAL;
  flags_.wire_type = dbWireType::ROUTED;
  flags_.special = 0;
  flags_.wild_connect = 0;
  flags_.wire_ordered = 0;
  flags_.unused2 = 0;
  flags_.disconnected = 0;
  flags_.spef = 0;
  flags_.select = 0;
  flags_.mark = 0;
  flags_.mark_1 = 0;
  flags_.wire_altered = 0;
  flags_.extracted = 0;
  flags_.rc_graph = 0;
  flags_.unused = 0;
  flags_.set_io = 0;
  flags_.io = 0;
  flags_.dont_touch = 0;
  flags_.fixed_bump = 0;
  flags_.source = dbSourceType::NONE;
  flags_.rc_disconnected = 0;
  flags_.block_rule = 0;
  flags_.has_jumpers = 0;
  name_ = nullptr;
  gndc_calibration_factor_ = 1.0;
  cc_calibration_factor_ = 1.0;
  weight_ = 1;
  xtalk_ = 0;
  cc_adjust_factor_ = -1;
  cc_adjust_order_ = 0;
  driving_iterm_ = -1;
}

_dbNet::~_dbNet()
{
  if (name_) {
    free((void*) name_);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbNet& net)
{
  uint* bit_field = (uint*) &net.flags_;
  stream << *bit_field;
  stream << net.name_;
  stream << net.gndc_calibration_factor_;
  stream << net.cc_calibration_factor_;
  stream << net.next_entry_;
  stream << net.iterms_;
  stream << net.bterms_;
  stream << net.wire_;
  stream << net.global_wire_;
  stream << net.swires_;
  stream << net.cap_nodes_;
  stream << net.r_segs_;
  stream << net.non_default_rule_;
  stream << net.weight_;
  stream << net.xtalk_;
  stream << net.cc_adjust_factor_;
  stream << net.cc_adjust_order_;
  stream << net.groups_;
  stream << net.guides_;
  stream << net.tracks_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbNet& net)
{
  uint* bit_field = (uint*) &net.flags_;
  stream >> *bit_field;
  stream >> net.name_;
  stream >> net.gndc_calibration_factor_;
  stream >> net.cc_calibration_factor_;
  stream >> net.next_entry_;
  stream >> net.iterms_;
  stream >> net.bterms_;
  stream >> net.wire_;
  stream >> net.global_wire_;
  stream >> net.swires_;
  stream >> net.cap_nodes_;
  stream >> net.r_segs_;
  stream >> net.non_default_rule_;
  stream >> net.weight_;
  stream >> net.xtalk_;
  stream >> net.cc_adjust_factor_;
  stream >> net.cc_adjust_order_;
  stream >> net.groups_;
  stream >> net.guides_;
  _dbDatabase* db = net.getImpl()->getDatabase();
  if (db->isSchema(db_schema_net_tracks)) {
    stream >> net.tracks_;
  }

  return stream;
}

bool _dbNet::operator<(const _dbNet& rhs) const
{
  return strcmp(name_, rhs.name_) < 0;
}

bool _dbNet::operator==(const _dbNet& rhs) const
{
  if (flags_.sig_type != rhs.flags_.sig_type) {
    return false;
  }

  if (flags_.wire_type != rhs.flags_.wire_type) {
    return false;
  }

  if (flags_.special != rhs.flags_.special) {
    return false;
  }

  if (flags_.wild_connect != rhs.flags_.wild_connect) {
    return false;
  }

  if (flags_.wire_ordered != rhs.flags_.wire_ordered) {
    return false;
  }

  if (flags_.disconnected != rhs.flags_.disconnected) {
    return false;
  }

  if (flags_.spef != rhs.flags_.spef) {
    return false;
  }

  if (flags_.select != rhs.flags_.select) {
    return false;
  }

  if (flags_.mark != rhs.flags_.mark) {
    return false;
  }

  if (flags_.mark_1 != rhs.flags_.mark_1) {
    return false;
  }

  if (flags_.wire_altered != rhs.flags_.wire_altered) {
    return false;
  }

  if (flags_.extracted != rhs.flags_.extracted) {
    return false;
  }

  if (flags_.rc_graph != rhs.flags_.rc_graph) {
    return false;
  }

  if (flags_.set_io != rhs.flags_.set_io) {
    return false;
  }

  if (flags_.io != rhs.flags_.io) {
    return false;
  }

  if (flags_.dont_touch != rhs.flags_.dont_touch) {
    return false;
  }

  if (flags_.fixed_bump != rhs.flags_.fixed_bump) {
    return false;
  }

  if (flags_.source != rhs.flags_.source) {
    return false;
  }

  if (flags_.rc_disconnected != rhs.flags_.rc_disconnected) {
    return false;
  }

  if (flags_.block_rule != rhs.flags_.block_rule) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (gndc_calibration_factor_ != rhs.gndc_calibration_factor_) {
    return false;
  }
  if (cc_calibration_factor_ != rhs.cc_calibration_factor_) {
    return false;
  }

  if (next_entry_ != rhs.next_entry_) {
    return false;
  }

  if (iterms_ != rhs.iterms_) {
    return false;
  }

  if (bterms_ != rhs.bterms_) {
    return false;
  }

  if (wire_ != rhs.wire_) {
    return false;
  }

  if (global_wire_ != rhs.global_wire_) {
    return false;
  }

  if (swires_ != rhs.swires_) {
    return false;
  }

  if (cap_nodes_ != rhs.cap_nodes_) {
    return false;
  }

  if (r_segs_ != rhs.r_segs_) {
    return false;
  }

  if (non_default_rule_ != rhs.non_default_rule_) {
    return false;
  }

  if (weight_ != rhs.weight_) {
    return false;
  }

  if (xtalk_ != rhs.xtalk_) {
    return false;
  }

  if (cc_adjust_factor_ != rhs.cc_adjust_factor_) {
    return false;
  }

  if (cc_adjust_order_ != rhs.cc_adjust_order_) {
    return false;
  }

  if (groups_ != rhs.groups_) {
    return false;
  }

  if (guides_ != rhs.guides_) {
    return false;
  }

  if (tracks_ != rhs.tracks_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// dbNet - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbNet::getName() const
{
  _dbNet* net = (_dbNet*) this;
  return net->name_;
}

const char* dbNet::getConstName() const
{
  _dbNet* net = (_dbNet*) this;
  return net->name_;
}

void dbNet::printNetName(FILE* fp, bool idFlag, bool newLine)
{
  if (idFlag) {
    fprintf(fp, " %d", getId());
  }

  _dbNet* net = (_dbNet*) this;
  fprintf(fp, " %s", net->name_);

  if (newLine) {
    fprintf(fp, "\n");
  }
}

bool dbNet::rename(const char* name)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (block->net_hash_.hasMember(name)) {
    return false;
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             1,
             "ECO: {}, rename to '{}'",
             net->getDebugName(),
             name);

  if (block->journal_) {
    block->journal_->updateField(this, _dbNet::kName, net->name_, name);
  }

  block->net_hash_.remove(net);
  free((void*) net->name_);
  net->name_ = safe_strdup(name);
  block->net_hash_.insert(net);

  return true;
}

void dbNet::swapNetNames(dbNet* source, bool ok_to_journal)
{
  _dbNet* dest_net = (_dbNet*) this;
  _dbNet* source_net = (_dbNet*) source;
  _dbBlock* block = (_dbBlock*) source_net->getOwner();

  char* dest_name_ptr = dest_net->name_;
  char* source_name_ptr = source_net->name_;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             1,
             "ECO: swap dbNet name between {} and {}",
             source->getDebugName(),
             dest_net->getDebugName());

  // allow undo..
  if (block->journal_ && ok_to_journal) {
    block->journal_->beginAction(dbJournal::kSwapObject);
    // a name
    block->journal_->pushParam(dbNameObj);
    // the type of name swap
    block->journal_->pushParam(dbNetObj);
    // stash the source and dest in that order,
    // let undo reorder
    block->journal_->pushParam(source_net->getId());
    block->journal_->pushParam(dest_net->getId());
    block->journal_->endAction();
  }

  block->net_hash_.remove(dest_net);
  block->net_hash_.remove(source_net);

  // swap names without copy, just swap the pointers
  dest_net->name_ = source_name_ptr;
  source_net->name_ = dest_name_ptr;

  block->net_hash_.insert(dest_net);
  block->net_hash_.insert(source_net);
}

bool dbNet::isRCDisconnected()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.rc_disconnected == 1;
}

void dbNet::setRCDisconnected(bool value)
{
  _dbNet* net = (_dbNet*) this;
  net->flags_.rc_disconnected = value;
}

int dbNet::getWeight()
{
  _dbNet* net = (_dbNet*) this;
  return net->weight_;
}

void dbNet::setWeight(int weight)
{
  _dbNet* net = (_dbNet*) this;
  net->weight_ = weight;
}

dbSourceType dbNet::getSourceType()
{
  _dbNet* net = (_dbNet*) this;
  dbSourceType t(net->flags_.source);
  return t;
}

void dbNet::setSourceType(dbSourceType type)
{
  _dbNet* net = (_dbNet*) this;
  net->flags_.source = type;
}

int dbNet::getXTalkClass()
{
  _dbNet* net = (_dbNet*) this;
  return net->xtalk_;
}

void dbNet::setXTalkClass(int value)
{
  _dbNet* net = (_dbNet*) this;
  net->xtalk_ = value;
}

float dbNet::getCcAdjustFactor()
{
  _dbNet* net = (_dbNet*) this;
  return net->cc_adjust_factor_;
}

void dbNet::setCcAdjustFactor(float factor)
{
  _dbNet* net = (_dbNet*) this;
  net->cc_adjust_factor_ = factor;
}

uint dbNet::getCcAdjustOrder()
{
  _dbNet* net = (_dbNet*) this;
  return net->cc_adjust_order_;
}

void dbNet::setCcAdjustOrder(uint order)
{
  _dbNet* net = (_dbNet*) this;
  net->cc_adjust_order_ = order;
}

void dbNet::setDrivingITerm(int id)
{
  _dbNet* net = (_dbNet*) this;
  net->driving_iterm_ = id;
}

int dbNet::getDrivingITermId() const
{
  _dbNet* net = (_dbNet*) this;
  return net->driving_iterm_;
}

dbITerm* dbNet::getDrivingITerm() const
{
  _dbNet* net = (_dbNet*) this;
  if (net->driving_iterm_ <= 0) {
    return nullptr;
  }
  return dbITerm::getITerm(getBlock(), net->driving_iterm_);
}

bool dbNet::hasFixedBump()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.fixed_bump == 1;
}

void dbNet::setFixedBump(bool value)
{
  _dbNet* net = (_dbNet*) this;
  net->flags_.fixed_bump = value;
}

void dbNet::setWireType(dbWireType wire_type)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->flags_.wire_type = wire_type.getValue();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setWireType: {}",
             net->getDebugName(),
             wire_type.getValue());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

dbWireType dbNet::getWireType() const
{
  _dbNet* net = (_dbNet*) this;
  return dbWireType(net->flags_.wire_type);
}

dbSigType dbNet::getSigType() const
{
  _dbNet* net = (_dbNet*) this;
  return dbSigType(net->flags_.sig_type);
}

void dbNet::setSigType(dbSigType sig_type)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->flags_.sig_type = sig_type.getValue();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setSigType: {}",
             net->getDebugName(),
             sig_type.getValue());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

float dbNet::getGndcCalibFactor()
{
  _dbNet* net = (_dbNet*) this;
  return net->gndc_calibration_factor_;
}

void dbNet::setGndcCalibFactor(float gndcCalib)
{
  _dbNet* net = (_dbNet*) this;
  net->gndc_calibration_factor_ = gndcCalib;
}

float dbNet::getRefCc()
{
  _dbNet* net = (_dbNet*) this;
  return net->ref_cc_;
}

void dbNet::setRefCc(float cap)
{
  _dbNet* net = (_dbNet*) this;
  net->ref_cc_ = cap;
}

float dbNet::getCcCalibFactor()
{
  _dbNet* net = (_dbNet*) this;
  return net->cc_calibration_factor_;
}

void dbNet::setCcCalibFactor(float ccCalib)
{
  _dbNet* net = (_dbNet*) this;
  net->cc_calibration_factor_ = ccCalib;
}

float dbNet::getDbCc()
{
  _dbNet* net = (_dbNet*) this;
  return net->db_cc_;
}

void dbNet::setDbCc(float cap)
{
  _dbNet* net = (_dbNet*) this;
  net->db_cc_ = cap;
}

void dbNet::addDbCc(float cap)
{
  _dbNet* net = (_dbNet*) this;
  net->db_cc_ += cap;
}

float dbNet::getCcMatchRatio()
{
  _dbNet* net = (_dbNet*) this;
  return net->cc_match_ratio_;
}

void dbNet::setCcMatchRatio(float ratio)
{
  _dbNet* net = (_dbNet*) this;
  net->cc_match_ratio_ = ratio;
}

void dbNet::calibrateCouplingCap(int corner)
{
  const float srcnetCcCalibFactor = getCcCalibFactor();
  std::vector<dbCCSeg*> ccSet;
  getSrcCCSegs(ccSet);
  for (dbCCSeg* cc : ccSet) {
    const float tgtnetCcCalibFactor = cc->getTargetNet()->getCcCalibFactor();
    const float factor = (srcnetCcCalibFactor + tgtnetCcCalibFactor) / 2;
    if (factor == 1.0) {
      continue;
    }
    if (corner < 0) {
      cc->adjustCapacitance(factor);
    } else {
      cc->adjustCapacitance(factor, corner);
    }
  }
}

void dbNet::calibrateCouplingCap()
{
  calibrateCouplingCap(-1);
}

uint dbNet::getRSegCount()
{
  return getRSegs().size();
}

uint dbNet::maxInternalCapNum()
{
  uint max_n = 0;
  for (dbCapNode* capn : getCapNodes()) {
    if (!capn->isInternal()) {
      continue;
    }

    const uint n = capn->getNode();
    if (max_n < n) {
      max_n = n;
    }
  }
  return max_n;
}
void dbNet::collapseInternalCapNum(FILE* mapFile)
{
  uint cnt = 1;
  for (dbCapNode* capn : getCapNodes()) {
    cnt++;
    if (capn->isInternal()) {
      if (mapFile) {
        fprintf(mapFile, "    %8d : %8d\n", capn->getNode(), cnt);
      }
      capn->setNode(cnt);
    }
  }
}

uint dbNet::getCapNodeCount()
{
  return getCapNodes().size();
}

uint dbNet::getCcCount()
{
  uint count = 0;
  for (dbCapNode* node : getCapNodes()) {
    count += node->getCCSegs().size();
  }
  return count;
}

bool dbNet::groundCC(const float gndFactor)
{
  bool grounded = false;

  for (dbCapNode* node : getCapNodes()) {
    grounded |= node->groundCC(gndFactor);
  }
  return grounded;
}

bool dbNet::adjustCC(uint adjOrder,
                     float adjFactor,
                     double ccThreshHold,
                     std::vector<dbCCSeg*>& adjustedCC,
                     std::vector<dbNet*>& halonets)
{
  _dbNet* net = (_dbNet*) this;
  if (net->cc_adjust_factor_ > 0) {
    getImpl()->getLogger()->warn(
        utl::ODB,
        48,
        "Net {} {} had been CC adjusted by {}. Please unadjust first.",
        getId(),
        getConstName(),
        net->cc_adjust_factor_);
    return false;
  }
  bool needAdjust = false;
  for (dbCapNode* node : getCapNodes()) {
    if (node->needAdjustCC(ccThreshHold)) {
      needAdjust = true;
    }
  }
  if (!needAdjust) {
    return false;
  }

  for (dbCapNode* node : getCapNodes()) {
    node->adjustCC(adjOrder, adjFactor, adjustedCC, halonets);
  }
  net->cc_adjust_factor_ = adjFactor;
  net->cc_adjust_order_ = adjOrder;
  return true;
}

void dbNet::undoAdjustedCC(std::vector<dbCCSeg*>& adjustedCC,
                           std::vector<dbNet*>& halonets)
{
  _dbNet* net = (_dbNet*) this;
  if (net->cc_adjust_factor_ < 0) {
    return;
  }
  const uint adjOrder = net->cc_adjust_order_;
  const float adjFactor = 1 / net->cc_adjust_factor_;
  for (dbCapNode* node : getCapNodes()) {
    node->adjustCC(adjOrder, adjFactor, adjustedCC, halonets);
  }
  net->cc_adjust_factor_ = -1;
  net->cc_adjust_order_ = 0;
}

void dbNet::adjustNetGndCap(uint corner, float factor)
{
  if (factor == 1.0) {
    return;
  }
  bool foreign = ((dbBlock*) getImpl()->getOwner())->getExtControl()->_foreign;
  if (foreign) {
    for (dbCapNode* node : getCapNodes()) {
      node->adjustCapacitance(factor, corner);
    }
  } else {
    for (dbRSeg* rc : getRSegs()) {
      rc->adjustCapacitance(factor, corner);
    }
  }
}
void dbNet::adjustNetGndCap(float factor)
{
  if (factor == 1.0) {
    return;
  }
  bool foreign = ((dbBlock*) getImpl()->getOwner())->getExtControl()->_foreign;
  if (foreign) {
    for (dbCapNode* node : getCapNodes()) {
      node->adjustCapacitance(factor);
    }
  } else {
    for (dbRSeg* rc : getRSegs()) {
      rc->adjustCapacitance(factor);
    }
  }
}

void dbNet::calibrateGndCap()
{
  adjustNetGndCap(getGndcCalibFactor());
}

void dbNet::calibrateCapacitance()
{
  calibrateGndCap();
  calibrateCouplingCap();
}
void dbNet::adjustNetRes(float factor, uint corner)
{
  if (factor == 1.0) {
    return;
  }
  for (dbRSeg* rc : getRSegs()) {
    rc->adjustResistance(factor, corner);
  }
}
void dbNet::adjustNetRes(float factor)
{
  if (factor == 1.0) {
    return;
  }
  for (dbRSeg* rc : getRSegs()) {
    rc->adjustResistance(factor);
  }
}

bool dbNet::isSpef()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.spef == 1;
}

void dbNet::setSpef(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->flags_.spef = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setSpef: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isSelect()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.select == 1;
}

void dbNet::setSelect(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->flags_.select = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setSelect: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isEnclosed(Rect* bbox)  // assuming no intersection
{
  dbWire* wire = getWire();
  dbWirePathItr pitr;
  dbWirePath path;
  dbWirePathShape pathShape;
  pitr.begin(wire);
  uint cnt = 0;
  while (pitr.getNextPath(path)) {
    if (path.point.getX() > bbox->xMax() || path.point.getX() < bbox->xMin()
        || path.point.getY() > bbox->yMax()
        || path.point.getY() < bbox->yMin()) {
      return false;
    }
    cnt++;
    if (cnt >= 4) {
      return true;
    }
    while (pitr.getNextShape(pathShape)) {
      if (pathShape.point.getX() > bbox->xMax()
          || pathShape.point.getX() < bbox->xMin()
          || pathShape.point.getY() > bbox->yMax()
          || pathShape.point.getY() < bbox->yMin()) {
        return false;
      }
      cnt++;
      if (cnt >= 4) {
        return true;
      }
    }
  }
  return true;
}

bool dbNet::isMarked()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.mark == 1;
}

void dbNet::setMark(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->flags_.mark = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setMark: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isMark_1ed()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.mark_1 == 1;
}

void dbNet::setMark_1(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->flags_.mark_1 = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setMark_1: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isWireOrdered()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.wire_ordered == 1;
}

void dbNet::setWireOrdered(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->flags_.wire_ordered = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setWireOrdered: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isDisconnected()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.disconnected == 1;
}

void dbNet::setDisconnected(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->flags_.disconnected = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setDisconnected: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

void dbNet::setWireAltered(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->flags_.wire_altered = (value == true) ? 1 : 0;
  if (value) {
    net->flags_.wire_ordered = 0;
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setWireAltered: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isWireAltered()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.wire_altered == 1;
}

void dbNet::setExtracted(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->flags_.extracted = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setExtracted: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isExtracted()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.extracted == 1;
}

void dbNet::setRCgraph(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->flags_.rc_graph = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setRCgraph: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isRCgraph()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.rc_graph == 1;
}

dbBlock* dbNet::getBlock() const
{
  return (dbBlock*) getImpl()->getOwner();
}

dbSet<dbITerm> dbNet::getITerms() const
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbITerm>(net, block->net_iterm_itr_);
}

dbITerm* dbNet::get1stITerm()
{
  dbSet<dbITerm> iterms = getITerms();

  dbITerm* it = nullptr;
  dbSet<dbITerm>::iterator iitr = iterms.begin();
  if (iitr != iterms.end()) {
    it = *iitr;
  }
  return it;
}

dbSet<dbBTerm> dbNet::getBTerms() const
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbBTerm>(net, block->net_bterm_itr_);
}

dbBTerm* dbNet::get1stBTerm()
{
  dbSet<dbBTerm> bterms = getBTerms();

  dbBTerm* bt = nullptr;
  dbSet<dbBTerm>::iterator bitr = bterms.begin();
  if (bitr != bterms.end()) {
    bt = *bitr;
  }
  return bt;
}

dbObject* dbNet::getFirstDriverTerm() const
{
  for (dbITerm* iterm : getITerms()) {
    if (iterm->getSigType().isSupply()) {
      continue;
    }

    if (iterm->isClocked()) {
      continue;
    }

    if (iterm->getIoType() == dbIoType::OUTPUT
        || iterm->getIoType() == dbIoType::INOUT) {
      return iterm;
    }
  }

  for (dbBTerm* bterm : getBTerms()) {
    if (bterm->getSigType().isSupply()) {
      continue;
    }

    if (bterm->getIoType() == dbIoType::INPUT
        || bterm->getIoType() == dbIoType::INOUT) {
      return bterm;
    }
  }

  return nullptr;
}

dbITerm* dbNet::getFirstOutput() const
{
  if (getDrivingITermId() > 0) {
    return dbITerm::getITerm((dbBlock*) getImpl()->getOwner(),
                             getDrivingITermId());
  }

  for (dbITerm* tr : getITerms()) {
    if (tr->getSigType().isSupply()) {
      continue;
    }

    if (tr->isClocked()) {
      continue;
    }

    if (tr->getIoType() != dbIoType::OUTPUT) {
      continue;
    }

    return tr;
  }

  return nullptr;
}

dbITerm* dbNet::get1stSignalInput(bool io)
{
  for (dbITerm* tr : getITerms()) {
    if (tr->getSigType().isSupply()) {
      continue;
    }

    if (tr->getIoType() != dbIoType::INPUT) {
      continue;
    }

    if (io && (tr->getIoType() != dbIoType::INOUT)) {
      continue;
    }

    return tr;
  }

  return nullptr;
}

dbSet<dbSWire> dbNet::getSWires()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbSWire>(net, block->swire_itr_);
}

dbSWire* dbNet::getFirstSWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->swires_ == 0) {
    return nullptr;
  }

  return (dbSWire*) block->swire_tbl_->getPtr(net->swires_);
}

dbWire* dbNet::getWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->wire_ == 0) {
    return nullptr;
  }

  return (dbWire*) block->wire_tbl_->getPtr(net->wire_);
}

dbWire* dbNet::getGlobalWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->global_wire_ == 0) {
    return nullptr;
  }

  return (dbWire*) block->wire_tbl_->getPtr(net->global_wire_);
}

bool dbNet::setIOflag()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  const uint prev_flags = flagsToUInt(net);
  net->flags_.set_io = 1;
  net->flags_.io = 0;
  const uint n = getBTerms().size();

  if (n > 0) {
    net->flags_.io = 1;
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             1,
             "ECO: {}, setIOFlag",
             net->getDebugName());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }

  return (n > 0);
}

bool dbNet::isIO()
{
  _dbNet* net = (_dbNet*) this;

  if (net->flags_.set_io > 0) {
    return net->flags_.io == 1;
  }
  return setIOflag();
}

void dbNet::setDoNotTouch(bool v)
{
  _dbNet* net = (_dbNet*) this;
  net->flags_.dont_touch = v;
}

bool dbNet::isDoNotTouch() const
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.dont_touch == 1;
}

bool dbNet::isSpecial() const
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.special == 1;
}

void dbNet::setSpecial()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->flags_.special = 1;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setSpecial",
             net->getDebugName());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

void dbNet::clearSpecial()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->flags_.special = 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, clearSpecial",
             net->getDebugName());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isConnected(const dbNet* other) const
{
  return (this == other);
}

bool dbNet::isConnected(const dbModNet* other) const
{
  if (other == nullptr) {
    return false;
  }
  dbNet* net = other->findRelatedNet();
  return isConnected(net);
}

bool dbNet::isConnectedByAbutment()
{
  if (getITermCount() > 2 || getBTermCount() > 0) {
    return false;
  }

  bool first_mterm = true;
  std::vector<Rect> first_pin_boxes;
  for (dbITerm* iterm : getITerms()) {
    dbMTerm* mterm = iterm->getMTerm();
    dbMaster* master = mterm->getMaster();
    if (!master->isBlock()) {
      return false;
    }

    dbInst* inst = iterm->getInst();
    if (inst->isPlaced()) {
      const dbTransform transform = inst->getTransform();

      for (dbMPin* mpin : mterm->getMPins()) {
        for (dbBox* box : mpin->getGeometry()) {
          dbTechLayer* tech_layer = box->getTechLayer();
          if (tech_layer->getType() != dbTechLayerType::ROUTING) {
            continue;
          }
          odb::Rect rect = box->getBox();
          transform.apply(rect);
          if (first_mterm) {
            first_pin_boxes.push_back(rect);
          } else {
            for (const Rect& first_pin_box : first_pin_boxes) {
              if (rect.intersects(first_pin_box)) {
                return true;
              }
            }
          }
        }
      }
    }
    first_mterm = false;
  }

  return false;
}

bool dbNet::isWildConnected()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.wild_connect == 1;
}

void dbNet::setWildConnected()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  // uint prev_flags = flagsToUInt(net);

  net->flags_.wild_connect = 1;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setWildConnected",
             net->getDebugName());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

void dbNet::clearWildConnected()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  // uint prev_flags = flagsToUInt(net);

  net->flags_.wild_connect = 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, clearWildConnected",
             net->getDebugName());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

dbSet<dbRSeg> dbNet::getRSegs()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbRSeg>(net, block->r_seg_itr_);
}

void dbNet::reverseRSegs()
{
  dbSet<dbRSeg> rSet = getRSegs();
  rSet.reverse();
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, reverse rsegs sequence",
             getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbNetObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(_dbNet::kReverseRSeg);
    block->journal_->endAction();
  }
}

dbRSeg* dbNet::findRSeg(uint srcn, uint tgtn)
{
  for (dbRSeg* rseg : getRSegs()) {
    if (rseg->getSourceNode() == srcn && rseg->getTargetNode() == tgtn) {
      return rseg;
    }
  }
  return nullptr;
}

void dbNet::set1stRSegId(uint rid)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint pid = net->r_segs_;
  net->r_segs_ = rid;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, set 1stRSegNode {}",
             getDebugName(),
             rid);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbNetObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(_dbNet::kHeadRSeg);
    block->journal_->pushParam(pid);
    block->journal_->pushParam(rid);
    block->journal_->endAction();
  }
}

uint dbNet::get1stRSegId()
{
  _dbNet* net = (_dbNet*) this;
  return net->r_segs_;
}

dbRSeg* dbNet::getZeroRSeg()
{
  _dbNet* net = (_dbNet*) this;
  if (net->r_segs_ == 0) {
    return nullptr;
  }
  dbRSeg* zrc = dbRSeg::getRSeg((dbBlock*) net->getOwner(), net->r_segs_);
  return zrc;
}

dbCapNode* dbNet::findCapNode(uint nodeId)
{
  for (dbCapNode* n : getCapNodes()) {
    if (n->getNode() == nodeId) {
      return n;
    }
  }

  return nullptr;
}

dbSet<dbCapNode> dbNet::getCapNodes()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbCapNode>(net, block->cap_node_itr_);
}

void dbNet::setTermExtIds(int capId)  // 1: capNodeId, 0: reset
{
  dbSet<dbCapNode> nodeSet = getCapNodes();
  dbSet<dbCapNode>::iterator rc_itr;
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {} {} term extId",
             (capId) ? "set" : "reset",
             getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbNetObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(_dbNet::kTermExtId);
    block->journal_->pushParam(capId);
    block->journal_->endAction();
  }

  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    dbCapNode* capNode = *rc_itr;
    if (capNode->isBTerm()) {
      uint nodeId = capNode->getNode();
      dbBTerm* bterm = dbBTerm::getBTerm((dbBlock*) block, nodeId);
      uint extId = capId ? capNode->getId() : 0;
      bterm->setExtId(extId);
      continue;
    }

    if (capNode->isITerm()) {
      uint nodeId = capNode->getNode();
      dbITerm* iterm = dbITerm::getITerm((dbBlock*) block, nodeId);
      uint extId = capId ? capNode->getId() : 0;
      iterm->setExtId(extId);
    }
  }
}
void dbNet::set1stCapNodeId(uint capn_id)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint pid = net->cap_nodes_;
  net->cap_nodes_ = capn_id;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {} set 1stCapNode {}",
             net->getDebugName(),
             capn_id);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbNetObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(_dbNet::kHeadCapNode);
    block->journal_->pushParam(pid);
    block->journal_->pushParam(capn_id);
    block->journal_->endAction();
  }
}

uint dbNet::get1stCapNodeId()
{
  _dbNet* net = (_dbNet*) this;
  return net->cap_nodes_;
}

void dbNet::reverseCCSegs()
{
  for (dbCapNode* node : getCapNodes()) {
    node->getCCSegs().reverse();
  }
}

void dbNet::getSrcCCSegs(std::vector<dbCCSeg*>& S)
{
  for (dbCapNode* node : getCapNodes()) {
    const uint cap_id = node->getImpl()->getOID();
    for (dbCCSeg* seg : node->getCCSegs()) {
      _dbCCSeg* seg_impl = (_dbCCSeg*) seg;
      if (seg_impl->cap_node_[0] == cap_id) {
        S.push_back(seg);
      }
    }
  }
}

void dbNet::getTgtCCSegs(std::vector<dbCCSeg*>& S)
{
  for (dbCapNode* node : getCapNodes()) {
    const uint cap_id = node->getImpl()->getOID();
    for (dbCCSeg* seg : node->getCCSegs()) {
      _dbCCSeg* seg_impl = (_dbCCSeg*) seg;
      if (seg_impl->cap_node_[1] == cap_id) {
        S.push_back(seg);
      }
    }
  }
}

void dbNet::destroyCapNodes(bool cleanExtid)
{
  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  dbSet<dbCapNode> cap_nodes = getCapNodes();
  dbSet<dbCapNode>::iterator itr;

  for (itr = cap_nodes.begin(); itr != cap_nodes.end();) {
    dbCapNode* cn = *itr;
    uint oid = cn->getNode();

    if (cleanExtid) {
      if (cn->isITerm()) {
        (dbITerm::getITerm(block, oid))->setExtId(0);

      } else if (cn->isBTerm()) {
        (dbBTerm::getBTerm(block, oid))->setExtId(0);
      }
    }

    itr = dbCapNode::destroy(itr);
  }
}

void dbNet::destroyRSegs()
{
  dbSet<dbRSeg> segs = getRSegs();
  dbSet<dbRSeg>::iterator sitr;

  for (sitr = segs.begin(); sitr != segs.end();) {
    sitr = dbRSeg::destroy(sitr);
  }

  dbRSeg* zrc = getZeroRSeg();
  if (zrc) {
    dbRSeg::destroy(zrc);
  }
}

void dbNet::destroyCCSegs()
{
  for (dbCapNode* n : getCapNodes()) {
    dbSet<dbCCSeg> ccSegs = n->getCCSegs();
    dbSet<dbCCSeg>::iterator ccitr;

    for (ccitr = ccSegs.begin();
         ccitr != ccSegs.end();)  // ++ccitr here after destroy(cc) would crash
    {
      dbCCSeg* cc = *ccitr;
      ++ccitr;
      dbCCSeg::destroy(cc);
    }
  }
}

void dbNet::getCouplingNets(const uint corner,
                            const double ccThreshold,
                            std::set<dbNet*>& cnets)
{
  std::vector<dbNet*> inets;
  std::vector<double> netccap;

  for (dbCapNode* n : getCapNodes()) {
    for (dbCCSeg* cc : n->getCCSegs()) {
      const double cccap = cc->getCapacitance(corner);
      dbNet* tnet = cc->getSourceCapNode()->getNet();
      if (tnet == this) {
        tnet = cc->getTargetCapNode()->getNet();
      }
      if (tnet->isMarked()) {
        for (uint ii = 0; ii < inets.size(); ii++) {
          if (inets[ii] == tnet) {
            netccap[ii] += cccap;
            break;
          }
        }
        continue;
      }
      netccap.push_back(cccap);
      inets.push_back(tnet);
      tnet->setMark(true);
    }
  }
  for (uint ii = 0; ii < inets.size(); ii++) {
    if (netccap[ii] >= ccThreshold && cnets.find(inets[ii]) == cnets.end()) {
      cnets.insert(inets[ii]);
    }
    inets[ii]->setMark(false);
  }
}

void dbNet::getGndTotalCap(double* gndcap, double* totalcap, double mcf)
{
  dbSigType type = getSigType();
  if (type.isSupply()) {
    return;
  }
  dbSet<dbRSeg> rSet = getRSegs();
  if (rSet.begin() == rSet.end()) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 52,
                                 "Net {}, {} has no extraction data",
                                 getId(),
                                 getConstName());
    return;
  }
  bool foreign = ((dbBlock*) getImpl()->getOwner())->getExtControl()->_foreign;
  bool first = true;
  if (foreign) {
    for (dbCapNode* node : getCapNodes()) {
      if (first) {
        node->getGndTotalCap(gndcap, totalcap, mcf);
      } else {
        node->addGndTotalCap(gndcap, totalcap, mcf);
      }
      first = false;
    }
  } else {
    for (dbRSeg* rc : rSet) {
      if (first) {
        rc->getGndTotalCap(gndcap, totalcap, mcf);
      } else {
        rc->addGndTotalCap(gndcap, totalcap, mcf);
      }
      first = false;
    }
  }
}

void dbNet::preExttreeMergeRC(double max_cap, uint corner)
{
  dbBlock* block = (dbBlock*) (getImpl()->getOwner());
  double totalcap[ADS_MAX_CORNER];
  dbCapNode* tgtNode;
  std::vector<dbRSeg*> mrsegs;
  dbSigType type = getSigType();
  if ((type == dbSigType::POWER) || (type == dbSigType::GROUND)) {
    return;
  }
  dbSet<dbRSeg> rSet = getRSegs();
  if (rSet.begin() == rSet.end()) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 53,
                                 "Net {}, {} has no extraction data",
                                 getId(),
                                 getConstName());
    return;
  }
  dbRSeg* prc = getZeroRSeg();
  bool firstRC = true;
  uint cnt = 1;
  prc->getGndTotalCap(nullptr, &totalcap[0], 1 /*mcf*/);
  for (dbRSeg* rc : rSet) {
    mrsegs.push_back(rc);
    if (firstRC && cnt != 1) {
      rc->getGndTotalCap(nullptr, &totalcap[0], 1 /*mcf*/);
    } else {
      rc->addGndTotalCap(nullptr, &totalcap[0], 1 /*mcf*/);
    }
    cnt++;
    firstRC = false;
    tgtNode = dbCapNode::getCapNode(block, rc->getTargetNode());
    if (rc->getSourceNode() == rc->getTargetNode()) {
      continue;
    }
    if (!tgtNode->isTreeNode() && totalcap[corner] <= max_cap
        && !tgtNode->isDangling()) {
      continue;
    }
    prc = rc;
    mrsegs.clear();
    firstRC = true;
  }
}

void dbNet::destroyParasitics()
{
  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  std::vector<dbNet*> nets;
  nets.push_back(this);
  block->destroyParasitics(nets);
}

double dbNet::getTotalCouplingCap(uint corner)
{
  double cap = 0.0;
  for (dbCapNode* n : getCapNodes()) {
    for (dbCCSeg* cc : n->getCCSegs()) {
      cap += cc->getCapacitance(corner);
    }
  }

  return cap;
}

double dbNet::getTotalCapacitance(uint corner, bool cc)
{
  double cap = 0.0;
  double cap1 = 0.0;
  bool foreign = ((dbBlock*) getImpl()->getOwner())->getExtControl()->_foreign;

  if (foreign) {
    for (dbCapNode* node : getCapNodes()) {
      cap1 = node->getCapacitance(corner);
      cap += cap1;
    }
  } else {
    for (dbRSeg* rc : getRSegs()) {
      cap1 = rc->getCapacitance(corner);
      cap += cap1;
    }
  }
  if (cc) {
    cap += getTotalCouplingCap(corner);
  }
  return cap;
}

double dbNet::getTotalResistance(uint corner)
{
  double cap = 0.0;

  for (dbRSeg* rc : getRSegs()) {
    cap += rc->getResistance(corner);
  }
  return cap;
}

void dbNet::setNonDefaultRule(dbTechNonDefaultRule* rule)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_rule = net->non_default_rule_;
  bool prev_block_rule = net->flags_.block_rule;

  if (rule == nullptr) {
    net->non_default_rule_ = 0U;
    net->flags_.block_rule = 0;
  } else {
    net->non_default_rule_ = rule->getImpl()->getOID();
    net->flags_.block_rule = rule->isBlockRule();
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: {}, setNonDefaultRule: ",
             getDebugName(),
             (rule) ? rule->getImpl()->getOID() : 0);

  if (block->journal_) {
    // block->_journal->updateField(this, _dbNet::NON_DEFAULT_RULE, prev_rule,
    // net->_non_default_rule );
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(rule->getObjectType());
    block->journal_->pushParam(rule->getId());
    block->journal_->pushParam(_dbNet::kNonDefaultRule);
    block->journal_->pushParam(prev_rule);
    block->journal_->pushParam((uint) net->non_default_rule_);
    block->journal_->pushParam(prev_block_rule);
    block->journal_->pushParam((bool) net->flags_.block_rule);
    block->journal_->endAction();
  }
}

dbTechNonDefaultRule* dbNet::getNonDefaultRule()
{
  _dbNet* net = (_dbNet*) this;

  if (net->non_default_rule_ == 0) {
    return nullptr;
  }

  dbDatabase* db = (dbDatabase*) net->getDatabase();

  if (net->flags_.block_rule) {
    _dbBlock* block = (_dbBlock*) net->getOwner();
    return (dbTechNonDefaultRule*) block->non_default_rule_tbl_->getPtr(
        net->non_default_rule_);
  }

  _dbTech* tech = (_dbTech*) db->getTech();
  return (dbTechNonDefaultRule*) tech->non_default_rule_tbl_->getPtr(
      net->non_default_rule_);
}

void dbNet::getSignalWireCount(uint& wireCnt, uint& viaCnt)
{
  dbWirePath path;
  dbWirePathShape pshape;
  dbWire* wire = getWire();
  if (wire == nullptr) {
    return;
  }
  dbWirePathItr pitr;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    while (pitr.getNextShape(pshape)) {
      if (pshape.shape.isVia()) {
        viaCnt++;
      } else {
        wireCnt++;
      }
    }
  }
}
void dbNet::getNetStats(uint& wireCnt,
                        uint& viaCnt,
                        uint& len,
                        uint& layerCnt,
                        uint* levelTable)
{
  len = 0;
  wireCnt = 0;
  viaCnt = 0;
  layerCnt = 0;
  dbWirePath path;
  dbWirePathShape pshape;
  dbWire* wire = getWire();
  if (wire == nullptr) {
    return;
  }
  dbWirePathItr pitr;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    while (pitr.getNextShape(pshape)) {
      if (pshape.shape.isVia()) {
        viaCnt++;
        continue;
      }
      wireCnt++;

      uint level = pshape.shape.getTechLayer()->getRoutingLevel();
      if (levelTable) {
        levelTable[level]++;
      }
      len += std::max(pshape.shape.xMax() - pshape.shape.xMin(),
                      pshape.shape.yMax() - pshape.shape.yMin());
    }
  }
}
void dbNet::getPowerWireCount(uint& wireCnt, uint& viaCnt)
{
  for (dbSWire* swire : getSWires()) {
    for (dbSBox* s : swire->getWires()) {
      if (s->isVia()) {
        viaCnt++;
      } else {
        wireCnt++;
      }
    }
  }
}

void dbNet::getWireCount(uint& wireCnt, uint& viaCnt)
{
  if (getSigType() == dbSigType::POWER || getSigType() == dbSigType::GROUND) {
    getPowerWireCount(wireCnt, viaCnt);
  } else {
    getSignalWireCount(wireCnt, viaCnt);
  }
}

uint dbNet::getITermCount()
{
  return getITerms().size();
}

uint dbNet::getBTermCount()
{
  return getBTerms().size();
}

uint dbNet::getTermCount()
{
  return getITermCount() + getBTermCount();
}

Rect dbNet::getTermBBox()
{
  Rect net_box;
  net_box.mergeInit();

  for (dbITerm* iterm : getITerms()) {
    int x, y;
    if (iterm->getAvgXY(&x, &y)) {
      Rect iterm_rect(x, y, x, y);
      net_box.merge(iterm_rect);
    } else {
      // This clause is sort of worthless because getAvgXY prints
      // a warning when it fails.
      dbInst* inst = iterm->getInst();
      dbBox* inst_box = inst->getBBox();
      int center_x = (inst_box->xMin() + inst_box->xMax()) / 2;
      int center_y = (inst_box->yMin() + inst_box->yMax()) / 2;
      Rect inst_center(center_x, center_y, center_x, center_y);
      net_box.merge(inst_center);
    }
  }

  for (dbBTerm* bterm : getBTerms()) {
    for (dbBPin* bpin : bterm->getBPins()) {
      dbPlacementStatus status = bpin->getPlacementStatus();
      if (status.isPlaced()) {
        Rect pin_bbox = bpin->getBBox();
        int center_x = (pin_bbox.xMin() + pin_bbox.xMax()) / 2;
        int center_y = (pin_bbox.yMin() + pin_bbox.yMax()) / 2;
        Rect pin_center(center_x, center_y, center_x, center_y);
        net_box.merge(pin_center);
      }
    }
  }
  return net_box;
}

void dbNet::destroySWires()
{
  _dbNet* net = (_dbNet*) this;

  dbSet<dbSWire> swires = getSWires();

  for (auto sitr = swires.begin(); sitr != swires.end();) {
    sitr = dbSWire::destroy(sitr);
  }

  net->swires_ = 0;
}

dbNet* dbNet::create(dbBlock* block_, const char* name_, bool skipExistingCheck)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (!skipExistingCheck && block->net_hash_.hasMember(name_)) {
    return nullptr;
  }

  _dbNet* net = block->net_tbl_->create();
  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kCreateObject);
    block->journal_->pushParam(dbNetObj);
    block->journal_->pushParam(name_);
    block->journal_->pushParam(net->getOID());
    block->journal_->endAction();
  }

  net->name_ = safe_strdup(name_);
  block->net_hash_.insert(net);

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             1,
             "ECO: create {}",
             net->getDebugName());

  for (auto cb : block->callbacks_) {
    cb->inDbNetCreate((dbNet*) net);
  }

  return (dbNet*) net;
}

dbNet* dbNet::create(dbBlock* block,
                     const char* name,
                     const dbNameUniquifyType& uniquify,
                     dbModule* parent_module)
{
  std::string net_name = block->makeNewNetName(
      parent_module ? parent_module->getModInst() : nullptr, name, uniquify);
  return create(block, net_name.c_str());
}

void dbNet::destroy(dbNet* net_)
{
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  dbBlock* dbblock = (dbBlock*) block;

  if (net->flags_.dont_touch) {
    net->getLogger()->error(
        utl::ODB, 364, "Attempt to destroy dont_touch net {}", net->name_);
  }

  dbSet<dbITerm> iterms = net_->getITerms();
  dbSet<dbITerm>::iterator iitr = iterms.begin();

  while (iitr != iterms.end()) {
    dbITerm* iterm = *iitr;
    ++iitr;
    iterm->disconnect();
  }

  dbSet<dbBTerm> bterms = net_->getBTerms();
  for (auto bitr = bterms.begin(); bitr != bterms.end();) {
    bitr = dbBTerm::destroy(bitr);
  }

  dbSet<dbSWire> swires = net_->getSWires();
  for (auto sitr = swires.begin(); sitr != swires.end();) {
    sitr = dbSWire::destroy(sitr);
  }

  if (net->wire_ != 0) {
    dbWire* wire = (dbWire*) block->wire_tbl_->getPtr(net->wire_);
    dbWire::destroy(wire);
  }

  for (const dbId<_dbGroup>& _group_id : net->groups_) {
    dbGroup* group = (dbGroup*) block->group_tbl_->getPtr(_group_id);
    group->removeNet(net_);
  }

  dbSet<dbGuide> guides = net_->getGuides();
  for (auto gitr = guides.begin(); gitr != guides.end();) {
    gitr = dbGuide::destroy(gitr);
  }

  dbSet<dbGlobalConnect> connects = dbblock->getGlobalConnects();
  for (auto gitr = connects.begin(); gitr != connects.end();) {
    if (gitr->getNet()->getId() == net_->getId()) {
      gitr = dbGlobalConnect::destroy(gitr);
    } else {
      gitr++;
    }
  }

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             1,
             "ECO: delete {}",
             net->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbNetObj);
    block->journal_->pushParam(net_->getName());
    block->journal_->pushParam(net->getOID());
    uint* flags = (uint*) &net->flags_;
    block->journal_->pushParam(*flags);
    block->journal_->pushParam(net->non_default_rule_);
    block->journal_->endAction();
  }

  for (auto cb : block->callbacks_) {
    cb->inDbNetDestroy(net_);
  }

  dbProperty::destroyProperties(net);
  block->net_hash_.remove(net);
  block->net_tbl_->destroy(net);
}

dbSet<dbNet>::iterator dbNet::destroy(dbSet<dbNet>::iterator& itr)
{
  dbNet* bt = *itr;
  dbSet<dbNet>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbNet* dbNet::getNet(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbNet*) block->net_tbl_->getPtr(dbid_);
}

dbNet* dbNet::getValidNet(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  if (!block->net_tbl_->validId(dbid_)) {
    return nullptr;
  }
  return (dbNet*) block->net_tbl_->getPtr(dbid_);
}

bool dbNet::canMergeNet(dbNet* in_net)
{
  if (isDoNotTouch() || in_net->isDoNotTouch()) {
    return false;
  }

  for (dbITerm* iterm : in_net->getITerms()) {
    if (iterm->getInst()->isDoNotTouch()) {
      return false;
    }
  }

  return true;
}

void dbNet::mergeNet(dbNet* in_net)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  for (auto callback : block->callbacks_) {
    callback->inDbNetPreMerge(this, in_net);
  }

  // 1. Connect all terminals of in_net to this net.

  // in_net->getITerms() returns a terminal iterator, and iterm->connect() can
  // invalidate the iterator by disconnecting a dbITerm.
  // Calling iterm->connect() during iteration with the iterator is not safe.
  // Thus create another vector for safe iterms iteration.
  auto iterms_set = in_net->getITerms();
  std::vector<dbITerm*> iterms(iterms_set.begin(), iterms_set.end());
  for (dbITerm* iterm : iterms) {
    iterm->connect(this);
  }

  // Create vector for safe iteration.
  auto bterms_set = in_net->getBTerms();
  std::vector<dbBTerm*> bterms(bterms_set.begin(), bterms_set.end());
  for (dbBTerm* bterm : bterms) {
    bterm->connect(this);
  }

  // 2. Destroy in_net
  destroy(in_net);
}

void dbNet::markNets(std::vector<dbNet*>& nets, dbBlock* block, bool mk)
{
  if (nets.empty()) {
    for (dbNet* net : block->getNets()) {
      net->setMark(mk);
    }
  } else {
    for (dbNet* net : nets) {
      net->setMark(mk);
    }
  }
}

dbSet<dbGuide> dbNet::getGuides() const
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbGuide>(net, block->guide_itr_);
}

void dbNet::clearGuides()
{
  dbSet<dbGuide> guides = getGuides();
  dbSet<dbGuide>::iterator itr = guides.begin();
  while (itr != guides.end()) {
    itr = dbGuide::destroy(itr);
  }
}

dbSet<dbNetTrack> dbNet::getTracks() const
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbNetTrack>(net, block->net_track_itr_);
}

void dbNet::clearTracks()
{
  dbSet<dbNetTrack> tracks = getTracks();
  dbSet<dbNetTrack>::iterator itr = tracks.begin();
  while (itr != tracks.end()) {
    itr = dbNetTrack::destroy(itr);
  }
}

bool dbNet::hasJumpers()
{
  bool has_jumpers = false;
  _dbNet* net = (_dbNet*) this;
  _dbDatabase* db = net->getImpl()->getDatabase();
  if (db->isSchema(db_schema_has_jumpers)) {
    has_jumpers = net->flags_.has_jumpers == 1;
  }
  return has_jumpers;
}

void dbNet::setJumpers(bool has_jumpers)
{
  _dbNet* net = (_dbNet*) this;
  _dbDatabase* db = net->getImpl()->getDatabase();
  if (db->isSchema(db_schema_has_jumpers)) {
    net->flags_.has_jumpers = has_jumpers ? 1 : 0;
  }
}

void dbNet::checkSanity() const
{
  // Check net itself
  std::vector<std::string> drvr_info_list;
  dbUtil::findBTermDrivers(this, drvr_info_list);
  dbUtil::findITermDrivers(this, drvr_info_list);
  dbUtil::checkNetSanity(this, drvr_info_list);

  // Check the consistency with the related dbModNet
  checkSanityModNetConsistency();
}

dbModInst* dbNet::findMainParentModInst() const
{
  dbBlock* block = getBlock();
  const char delim = block->getHierarchyDelimiter();
  const std::string net_name = getName();
  const size_t last_delim_pos = net_name.find_last_of(delim);

  if (last_delim_pos != std::string::npos) {
    const std::string net_parent_hier_name = net_name.substr(0, last_delim_pos);
    return block->findModInst(net_parent_hier_name.c_str());
  }

  return nullptr;
}

dbModule* dbNet::findMainParentModule() const
{
  dbModInst* parent_mod_inst = findMainParentModInst();
  if (parent_mod_inst) {
    return parent_mod_inst->getMaster();
  }

  return getBlock()->getTopModule();
}

bool dbNet::findRelatedModNets(std::set<dbModNet*>& modnet_set) const
{
  modnet_set.clear();

  std::vector<dbModNet*> nets_to_visit;

  // Helper to add a modnet to the result set and the visit queue if it's new.
  auto visitIfNew = [&](dbModNet* modnet) {
    if (modnet && modnet_set.insert(modnet).second) {
      nets_to_visit.push_back(modnet);
    }
  };

  // Find initial set of modnets from the current dbNet.
  for (dbITerm* iterm : getITerms()) {
    visitIfNew(iterm->getModNet());
  }
  for (dbBTerm* bterm : getBTerms()) {
    visitIfNew(bterm->getModNet());
  }

  // Perform a DFS traversal to find all connected modnets.
  while (!nets_to_visit.empty()) {
    dbModNet* current_mod_net = nets_to_visit.back();
    nets_to_visit.pop_back();

    for (dbModITerm* mod_iterm : current_mod_net->getModITerms()) {
      if (dbModBTerm* mod_bterm = mod_iterm->getChildModBTerm()) {
        visitIfNew(mod_bterm->getModNet());
      }
    }

    for (dbModBTerm* mod_bterm : current_mod_net->getModBTerms()) {
      if (dbModITerm* mod_iterm = mod_bterm->getParentModITerm()) {
        visitIfNew(mod_iterm->getModNet());
      }
    }
  }

  return !modnet_set.empty();
}

void dbNet::dump(bool show_modnets) const
{
  utl::Logger* logger = getImpl()->getLogger();
  logger->report("--------------------------------------------------");
  logger->report("dbNet: {} (id={})", getName(), getId());
  logger->report(
      "  Parent Block: {} (id={})", getBlock()->getName(), getBlock()->getId());
  logger->report("  SigType: {}", getSigType().getString());
  logger->report("  WireType: {}", getWireType().getString());
  if (isSpecial()) {
    logger->report("  Special: true");
  }
  if (isDoNotTouch()) {
    logger->report("  DoNotTouch: true");
  }

  logger->report("  ITerms ({}):", getITerms().size());
  for (dbITerm* term : getITerms()) {
    logger->report("    - {} ({}, {}, id={})",
                   term->getName(),
                   term->getSigType().getString(),
                   term->getIoType().getString(),
                   term->getId());
  }

  logger->report("  BTerms ({}):", getBTerms().size());
  for (dbBTerm* term : getBTerms()) {
    logger->report("    - {} ({}, {}, id={})",
                   term->getName(),
                   term->getSigType().getString(),
                   term->getIoType().getString(),
                   term->getId());
  }
  logger->report("--------------------------------------------------");

  if (show_modnets) {
    std::set<dbModNet*> modnets;
    findRelatedModNets(modnets);
    for (dbModNet* modnet : modnets) {
      modnet->dump();
    }
  }
}

void _dbNet::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["name"].add(name_);
  info.children_["groups"].add(groups_);
}

bool dbNet::isDeeperThan(const dbNet* net) const
{
  std::string this_name = getName();
  std::string other_name = net->getName();

  char delim = getBlock()->getHierarchyDelimiter();
  size_t this_depth = std::count(this_name.begin(), this_name.end(), delim);
  size_t other_depth = std::count(other_name.begin(), other_name.end(), delim);

  return (other_depth < this_depth);
}

dbModNet* dbNet::findModNetInHighestHier() const
{
  std::set<dbModNet*> modnets;
  if (findRelatedModNets(modnets) == false) {
    return nullptr;
  }

  dbModNet* highest = nullptr;
  size_t min_delimiters = (size_t) -1;
  char delim = getBlock()->getHierarchyDelimiter();

  // Network::highestConnectedNet(Net *net) compares level of hierarchy and
  // hierarchical net name as a tie breaker.
  // For consistency, this API also uses the hierarchical net name as a tie
  // breaker.
  for (dbModNet* modnet : modnets) {
    std::string name = modnet->getHierarchicalName();
    size_t num_delimiters = std::count(name.begin(), name.end(), delim);
    if (highest == nullptr || num_delimiters < min_delimiters
        || (num_delimiters == min_delimiters
            && name < highest->getHierarchicalName())) {  // name = tie breaker
      min_delimiters = num_delimiters;
      highest = modnet;
    }
  }

  return highest;
}

void dbNet::renameWithModNetInHighestHier()
{
  dbModNet* highest_mod_net = findModNetInHighestHier();
  if (highest_mod_net) {
    rename(highest_mod_net->getHierarchicalName().c_str());
  }
}

bool dbNet::isInternalTo(dbModule* module) const
{
  // If it's connected to any top-level ports (BTerms), it's not internal.
  if (!getBTerms().empty()) {
    return false;
  }

  // Check all instance terminals (ITerms) it's connected to.
  for (dbITerm* iterm : getITerms()) {
    if (iterm->getInst()->getModule() != module) {
      return false;
    }
  }

  return true;
}

void dbNet::checkSanityModNetConsistency() const
{
  utl::Logger* logger = getImpl()->getLogger();

  // 1. Find all related dbModNets with this dbNet.
  std::set<dbModNet*> related_modnets;
  findRelatedModNets(related_modnets);
  if (related_modnets.empty()) {
    return;
  }

  // 2. Find all ITerms and BTerms connected with this dbNet.
  std::set<dbITerm*> flat_iterms;
  for (dbITerm* iterm : getITerms()) {
    flat_iterms.insert(iterm);
  }

  std::set<dbBTerm*> flat_bterms;
  for (dbBTerm* bterm : getBTerms()) {
    flat_bterms.insert(bterm);
  }

  // 3. Find all ITerms and BTerms connected with all the related dbModNets.
  std::set<dbITerm*> hier_iterms;
  std::set<dbBTerm*> hier_bterms;
  for (dbModNet* modnet : related_modnets) {
    for (dbITerm* iterm : modnet->getITerms()) {
      hier_iterms.insert(iterm);
    }
    for (dbBTerm* bterm : modnet->getBTerms()) {
      hier_bterms.insert(bterm);
    }
  }

  // 4. If found any inconsistency, report the difference.

  // 4.1. Compare ITerms
  std::vector<dbITerm*> iterms_in_flat_only;
  std::set_difference(flat_iterms.begin(),
                      flat_iterms.end(),
                      hier_iterms.begin(),
                      hier_iterms.end(),
                      std::back_inserter(iterms_in_flat_only));

  if (iterms_in_flat_only.empty() == false) {
    logger->warn(utl::ODB,
                 484,
                 "SanityCheck: dbNet '{}' has ITerms not present in its "
                 "related dbModNets.",
                 getName());
    for (dbITerm* iterm : iterms_in_flat_only) {
      logger->warn(utl::ODB, 485, "  - ITerm: {}", iterm->getName());
    }
  }

  std::vector<dbITerm*> iterms_in_hier_only;
  std::set_difference(hier_iterms.begin(),
                      hier_iterms.end(),
                      flat_iterms.begin(),
                      flat_iterms.end(),
                      std::back_inserter(iterms_in_hier_only));

  if (iterms_in_hier_only.empty() == false) {
    logger->warn(utl::ODB,
                 488,
                 "SanityCheck: dbNet '{}' is missing ITerms that are present "
                 "in its related dbModNets.",
                 getName());
    for (dbITerm* iterm : iterms_in_hier_only) {
      logger->warn(utl::ODB,
                   489,
                   "  - ITerm: {} (in hier, not in flat)",
                   iterm->getName());
    }
  }

  // 4.2. Compare BTerms
  std::vector<dbBTerm*> bterms_in_flat_only;
  std::set_difference(flat_bterms.begin(),
                      flat_bterms.end(),
                      hier_bterms.begin(),
                      hier_bterms.end(),
                      std::back_inserter(bterms_in_flat_only));

  if (bterms_in_flat_only.empty() == false) {
    logger->warn(utl::ODB,
                 486,
                 "SanityCheck: dbNet '{}' has BTerms not present in its "
                 "related dbModNets.",
                 getName());
    for (dbBTerm* bterm : bterms_in_flat_only) {
      logger->warn(utl::ODB, 487, "  - BTerm: {}", bterm->getName());
    }
  }

  std::vector<dbBTerm*> bterms_in_hier_only;
  std::set_difference(hier_bterms.begin(),
                      hier_bterms.end(),
                      flat_bterms.begin(),
                      flat_bterms.end(),
                      std::back_inserter(bterms_in_hier_only));

  if (bterms_in_hier_only.empty() == false) {
    logger->warn(utl::ODB,
                 490,
                 "SanityCheck: dbNet '{}' is missing BTerms that are present "
                 "in its related dbModNets.",
                 getName());
    for (dbBTerm* bterm : bterms_in_hier_only) {
      logger->warn(utl::ODB,
                   491,
                   "  - BTerm: {} (in hier, not in flat)",
                   bterm->getName());
    }
  }
}

void dbNet::dumpConnectivity(int level) const
{
  utl::Logger* logger = getImpl()->getLogger();
  logger->report("--------------------------------------------------");
  logger->report("Connectivity for dbNet: {} (id={})", getName(), getId());

  std::set<const dbObject*> visited;
  _dbNet::dumpNetConnectivity(this, level, 1, visited, logger);

  std::set<dbModNet*> modnets;
  if (findRelatedModNets(modnets)) {
    for (dbModNet* modnet : modnets) {
      _dbNet::dumpModNetConnectivity(modnet, level, 1, visited, logger);
    }
  }

  logger->report("--------------------------------------------------");
}

void _dbNet::dumpConnectivityRecursive(const dbObject* obj,
                                       int max_level,
                                       int level,
                                       std::set<const dbObject*>& visited,
                                       utl::Logger* logger)
{
  if (level > max_level || obj == nullptr) {
    return;
  }

  std::string details;
  if (obj->getObjectType() == dbITermObj) {
    auto iterm = static_cast<const dbITerm*>(obj);
    details = fmt::format(" (master: {}, io: {})",
                          iterm->getInst()->getMaster()->getName(),
                          iterm->getIoType().getString());
  } else if (obj->getObjectType() == dbBTermObj) {
    auto bterm = static_cast<const dbBTerm*>(obj);
    details = fmt::format(" (io: {})", bterm->getIoType().getString());
  } else if (obj->getObjectType() == dbModITermObj) {
    auto moditerm = static_cast<const dbModITerm*>(obj);
    if (auto modbterm = moditerm->getChildModBTerm()) {
      details = fmt::format(" (module: {}, io: {})",
                            moditerm->getParent()->getMaster()->getName(),
                            modbterm->getIoType().getString());
    } else {
      details = fmt::format(" (module: {})",
                            moditerm->getParent()->getMaster()->getName());
    }
  } else if (obj->getObjectType() == dbModBTermObj) {
    auto modbterm = static_cast<const dbModBTerm*>(obj);
    details = fmt::format(" (io: {})", modbterm->getIoType().getString());
  }

  if (visited.count(obj)) {
    logger->report("{:>{}}-> {} {} (id={}){}",
                   "",
                   level * 2,
                   obj->getTypeName(),
                   obj->getName(),
                   obj->getId(),
                   details,
                   " [visited]");
    return;
  }

  logger->report("{:>{}}- {} {} (id={}){}",
                 "",
                 level * 2,
                 obj->getTypeName(),
                 obj->getName(),
                 obj->getId(),
                 details);
  visited.insert(obj);

  switch (obj->getObjectType()) {
    case dbNetObj:
      dumpNetConnectivity(static_cast<const dbNet*>(obj),
                          max_level,
                          level + 1,
                          visited,
                          logger);
      break;
    case dbModNetObj:
      dumpModNetConnectivity(static_cast<const odb::dbModNet*>(obj),
                             max_level,
                             level + 1,
                             visited,
                             logger);
      break;
    case dbITermObj: {
      auto iterm = static_cast<const dbITerm*>(obj);
      _dbNet::dumpConnectivityRecursive(
          iterm->getNet(), max_level, level + 1, visited, logger);
      _dbNet::dumpConnectivityRecursive(
          iterm->getModNet(), max_level, level + 1, visited, logger);
      _dbNet::dumpConnectivityRecursive(
          iterm->getInst(), max_level, level + 1, visited, logger);
      break;
    }
    case dbBTermObj: {
      auto bterm = static_cast<const dbBTerm*>(obj);
      _dbNet::dumpConnectivityRecursive(
          bterm->getNet(), max_level, level + 1, visited, logger);
      _dbNet::dumpConnectivityRecursive(
          bterm->getModNet(), max_level, level + 1, visited, logger);
      break;
    }
    case dbModITermObj: {
      auto moditerm = static_cast<const dbModITerm*>(obj);
      _dbNet::dumpConnectivityRecursive(
          moditerm->getModNet(), max_level, level + 1, visited, logger);
      _dbNet::dumpConnectivityRecursive(
          moditerm->getParent(), max_level, level + 1, visited, logger);
      break;
    }
    case dbModBTermObj: {
      auto modbterm = static_cast<const dbModBTerm*>(obj);
      _dbNet::dumpConnectivityRecursive(
          modbterm->getModNet(), max_level, level + 1, visited, logger);
      break;
    }
    case dbInstObj: {
      auto inst = static_cast<const dbInst*>(obj);
      for (auto iterm : inst->getITerms()) {
        _dbNet::dumpConnectivityRecursive(
            iterm, max_level, level + 1, visited, logger);
      }
      break;
    }
    default:
      // Not an object type we traverse from, do nothing.
      break;
  }
}

void _dbNet::dumpNetConnectivity(const dbNet* net,
                                 int max_level,
                                 int level,
                                 std::set<const dbObject*>& visited,
                                 utl::Logger* logger)
{
  std::vector<const dbObject*> inputs;
  std::vector<const dbObject*> outputs;
  std::vector<const dbObject*> others;

  for (auto term : net->getITerms()) {
    if (term->getIoType() == dbIoType::INPUT) {
      inputs.push_back(term);
    } else if (term->getIoType() == dbIoType::OUTPUT) {
      outputs.push_back(term);
    } else {
      others.push_back(term);
    }
  }
  for (auto term : net->getBTerms()) {
    if (term->getIoType() == dbIoType::INPUT) {
      inputs.push_back(term);
    } else if (term->getIoType() == dbIoType::OUTPUT) {
      outputs.push_back(term);
    } else {
      others.push_back(term);
    }
  }

  for (auto term : inputs) {
    _dbNet::dumpConnectivityRecursive(term, max_level, level, visited, logger);
  }
  for (auto term : others) {
    _dbNet::dumpConnectivityRecursive(term, max_level, level, visited, logger);
  }
  for (auto term : outputs) {
    _dbNet::dumpConnectivityRecursive(term, max_level, level, visited, logger);
  }
}

void _dbNet::dumpModNetConnectivity(const dbModNet* modnet,
                                    int max_level,
                                    int level,
                                    std::set<const dbObject*>& visited,
                                    utl::Logger* logger)
{
  std::vector<const dbObject*> inputs;
  std::vector<const dbObject*> outputs;
  std::vector<const dbObject*> others;

  auto classifyTerm = [&](const auto* term) {
    dbIoType io_type;
    if constexpr (std::is_same_v<std::decay_t<decltype(*term)>, dbITerm>
                  || std::is_same_v<std::decay_t<decltype(*term)>, dbBTerm>) {
      io_type = term->getIoType();
    } else if constexpr (std::is_same_v<std::decay_t<decltype(*term)>,
                                        dbModITerm>) {
      if (auto bterm = term->getChildModBTerm()) {
        io_type = bterm->getIoType();
      } else {
        others.push_back(term);
        return;
      }
    } else if constexpr (std::is_same_v<std::decay_t<decltype(*term)>,
                                        dbModBTerm>) {
      io_type = term->getIoType();
    }

    if (io_type == dbIoType::INPUT) {
      inputs.push_back(term);
    } else if (io_type == dbIoType::OUTPUT) {
      outputs.push_back(term);
    } else {
      others.push_back(term);
    }
  };

  for (auto term : modnet->getITerms()) {
    classifyTerm(term);
  }
  for (auto term : modnet->getBTerms()) {
    classifyTerm(term);
  }
  for (auto term : modnet->getModITerms()) {
    classifyTerm(term);
  }
  for (auto term : modnet->getModBTerms()) {
    classifyTerm(term);
  }

  for (auto term : inputs) {
    _dbNet::dumpConnectivityRecursive(term, max_level, level, visited, logger);
  }
  for (auto term : others) {
    _dbNet::dumpConnectivityRecursive(term, max_level, level, visited, logger);
  }
  for (auto term : outputs) {
    _dbNet::dumpConnectivityRecursive(term, max_level, level, visited, logger);
  }
}

dbInst* dbNet::insertBufferBeforeLoad(dbObject* load_input_term,
                                      const dbMaster* buffer_master,
                                      const Point* loc,
                                      const char* base_name,
                                      const dbNameUniquifyType& uniquify)
{
  return insertBufferCommon(load_input_term,
                            buffer_master,
                            loc,
                            base_name,
                            uniquify,
                            /* insertBefore */ true);
}

dbInst* dbNet::insertBufferAfterDriver(dbObject* drvr_output_term,
                                       const dbMaster* buffer_master,
                                       const Point* loc,
                                       const char* base_name,
                                       const dbNameUniquifyType& uniquify)
{
  return insertBufferCommon(drvr_output_term,
                            buffer_master,
                            loc,
                            base_name,
                            uniquify,
                            /* insertBefore */ false);
}

dbInst* dbNet::insertBufferCommon(dbObject* term_obj,
                                  const dbMaster* buffer_master,
                                  const Point* loc,
                                  const char* base_name,
                                  const dbNameUniquifyType& uniquify,
                                  bool insertBefore)
{
  if (term_obj == nullptr || buffer_master == nullptr) {
    return nullptr;
  }

  dbBlock* block = getBlock();
  if (block == nullptr) {
    return nullptr;
  }

  // 1. Check terminal type and connectivity.
  dbITerm* term_iterm = nullptr;
  dbBTerm* term_bterm = nullptr;
  dbModNet* orig_mod_net = nullptr;
  if (!checkAndGetTerm(this, term_obj, term_iterm, term_bterm, orig_mod_net)) {
    return nullptr;
  }

  // 2. Check if dont_touch attribute is present
  if (checkDontTouch(this, term_iterm, nullptr)) {
    return nullptr;
  }

  // 3. Check buffer validity and create buffer instance
  dbITerm* buf_input_iterm = nullptr;
  dbITerm* buf_output_iterm = nullptr;
  dbModule* parent_mod
      = (term_iterm) ? term_iterm->getInst()->getModule() : nullptr;
  dbInst* buffer_inst = checkAndCreateBuffer(block,
                                             buffer_master,
                                             base_name,
                                             uniquify,
                                             parent_mod,
                                             buf_input_iterm,
                                             buf_output_iterm);
  if (buffer_inst == nullptr) {
    return nullptr;
  }

  // 4. Create new net for one side of the buffer
  const char* suffix = insertBefore ? "_load" : "_drvr";
  dbNet* new_net
      = createBufferNet(term_bterm, suffix, orig_mod_net, parent_mod, uniquify);
  if (new_net == nullptr) {
    dbInst::destroy(buffer_inst);
    return nullptr;
  }

  // 5. Rewire
  rewireBuffer(insertBefore,
               buf_input_iterm,
               buf_output_iterm,
               this,
               orig_mod_net,
               new_net,
               term_iterm,
               term_bterm);

  // 6. Place the new buffer
  placeNewBuffer(buffer_inst, loc, term_iterm, term_bterm);

  // 7. Propagate NDR
  if (dbTechNonDefaultRule* rule = getNonDefaultRule()) {
    new_net->setNonDefaultRule(rule);
  }

  return buffer_inst;
}

dbInst* dbNet::insertBufferBeforeLoads(std::set<dbObject*>& load_pins,
                                       const dbMaster* buffer_master,
                                       const Point* loc,
                                       const char* base_name,
                                       const dbNameUniquifyType& uniquify,
                                       bool loads_on_same_db_net)
{
  // jk: dbg
  static int i = 0;
  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads#{}",
             ++i);

  if (load_pins.empty() || buffer_master == nullptr) {
    return nullptr;
  }

  dbBlock* block = getBlock();
  if (block == nullptr) {
    return nullptr;
  }

  debugPrint(
      getImpl()->getLogger(),
      utl::ODB,
      "insert_buffer",
      1,
      "BeforeLoads: Try inserting a buffer on dbNet={}, load_pins_size={}, "
      "buffer_master={}, loc=({}, {}), base_name={}, uniquify={}, "
      "loads_on_same_db_net={}",
      getName(),
      load_pins.size(),
      buffer_master->getName(),
      loc ? loc->getX() : -1,
      loc ? loc->getY() : -1,
      base_name ? base_name : "buf",
      static_cast<int>(uniquify),
      loads_on_same_db_net);

  // 1. Validate Load Pins & Find Lowest Common Ancestor (Hierarchy)
  //    Also check for DontTouch attributes.
  dbModule* target_module = nullptr;
  bool first = true;

  std::set<dbNet*> other_dbnets;

  for (dbObject* load_obj : load_pins) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "insert_buffer",
               1,
               "BeforeLoads:   target load pin: {}",
               load_obj->getName());

    if (load_obj == nullptr) {
      continue;
    }

    dbModule* curr_mod = nullptr;
    if (load_obj->getObjectType() == dbITermObj) {
      dbITerm* load = static_cast<dbITerm*>(load_obj);

      // Check connectivity
      if (load->getNet() != this) {
        other_dbnets.insert(load->getNet());
        debugPrint(getImpl()->getLogger(),
                   utl::ODB,
                   "insert_buffer",
                   1,
                   "BeforeLoads:   * is on different dbNet '{}'",
                   load->getNet()->getName());

        if (loads_on_same_db_net) {
          getImpl()->getLogger()->error(utl::ODB,
                                        1200,
                                        "BeforeLoads: Load pin {} is "
                                        "not connected to net {}",
                                        load->getName(),
                                        getName());
          return nullptr;
        }
      }

      // Check dont_touch on the instance or the load pin's net
      if (checkDontTouch(this, nullptr, load)) {
        getImpl()->getLogger()->warn(utl::ODB,
                                     1201,
                                     "BeforeLoads: Load pin {} or "
                                     "net is dont_touch.",
                                     load->getName());
        return nullptr;
      }
      curr_mod = load->getInst()->getModule();
    } else if (load_obj->getObjectType() == dbBTermObj) {
      dbBTerm* load = static_cast<dbBTerm*>(load_obj);

      // Check connectivity
      if (load->getNet() != this) {
        other_dbnets.insert(load->getNet());
        debugPrint(getImpl()->getLogger(),
                   utl::ODB,
                   "insert_buffer",
                   1,
                   "BeforeLoads:   * is on different dbNet '{}'",
                   load->getNet()->getName());

        if (loads_on_same_db_net) {
          getImpl()->getLogger()->error(utl::ODB,
                                        1202,
                                        "BeforeLoads: Load pin {} is "
                                        "not connected to net {}",
                                        load->getName(),
                                        getName());
          return nullptr;
        }
      }

      // Check dont_touch on the net
      if (checkDontTouch(this, nullptr, nullptr)) {
        getImpl()->getLogger()->warn(utl::ODB,
                                     1203,
                                     "BeforeLoads: Load pin {} or "
                                     "net is dont_touch.",
                                     load->getName());
        return nullptr;
      }
      curr_mod = block->getTopModule();
    } else {
      getImpl()->getLogger()->error(
          utl::ODB, 1204, "BeforeLoads: Load object is not an ITerm or BTerm.");
      return nullptr;
    }

    if (first) {
      target_module = curr_mod;
      first = false;
    } else {
      target_module = findLCA(target_module, curr_mod);
    }
  }

  if (getImpl()->getLogger()->debugCheck(utl::ODB, "insert_buffer", 1)) {
    dbModInst* target_mod_inst
        = target_module ? target_module->getModInst() : nullptr;
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "insert_buffer",
               1,
               "BeforeLoads: LCA module: {} '{}'",
               target_module ? target_module->getName() : "null",
               target_mod_inst ? target_mod_inst->getHierarchicalName()
                               : "<null_inst>");
  }

  // Print net connectivities including both this flat and related hier nets
  if (getImpl()->getLogger()->debugCheck(utl::ODB, "insert_buffer", 2)) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "insert_buffer",
               1,
               "[Dump this dbNet]");
    dump(true);

    int other_dbnet_idx = 0;
    for (dbNet* other_dbnet : other_dbnets) {
      debugPrint(getImpl()->getLogger(),
                 utl::ODB,
                 "insert_buffer",
                 1,
                 "[Dump other dbNet {}]",
                 other_dbnet_idx++);
      other_dbnet->dump(true);
    }
  }

  if (target_module == nullptr) {
    // Fallback to top module if something went wrong or loads are in top
    target_module = block->getTopModule();
  }

  // 2. Create the Buffer Instance in the Target Hierarchy
  dbITerm* buf_input_iterm = nullptr;
  dbITerm* buf_output_iterm = nullptr;
  dbInst* buffer_inst = checkAndCreateBuffer(block,
                                             buffer_master,
                                             base_name,
                                             uniquify,
                                             target_module,  // Place in LCA
                                             buf_input_iterm,
                                             buf_output_iterm);
  if (buffer_inst == nullptr) {
    getImpl()->getLogger()->error(
        utl::ODB, 1205, "BeforeLoads: Failed to create buffer instance.");
    return nullptr;
  }

  // 3. Create the New Net (Buffer Output Net)
  //    The new net resides in the same hierarchy as the buffer.
  dbNet* new_flat_net = dbNet::create(block, "net", uniquify, target_module);

  // Check if we need a dbModNet (Logical Net)
  // dbModNet is required only if:
  //   1. Any load is a BTerm and buffer hierarchy is not top.
  //   2. Any load is an ITerm in a different hierarchy (Requires Port Punching,
  //      resulting in dbModITerm connection).
  bool needs_mod_net = false;
  if (getDb()->hasHierarchy()) {
    for (dbObject* load_obj : load_pins) {
      if (load_obj->getObjectType() == dbBTermObj
          && target_module != block->getTopModule()) {
        // A BTerm load requires a ModNet if the buffer is not at the top
        // level.
        needs_mod_net = true;
        break;
      }

      if (load_obj->getObjectType() == dbITermObj) {
        dbITerm* load = static_cast<dbITerm*>(load_obj);
        // If load module is different from buffer module, hierarchy crossing
        // occurs.
        if (load->getInst()->getModule() != target_module) {
          needs_mod_net = true;
          break;
        }
      }
    }
  }

  dbModNet* new_mod_net = nullptr;
  if (needs_mod_net) {
    // Create MOD net for logical connection in the target module only when
    // necessary
    const char* base_name = block->getBaseName(new_flat_net->getConstName());
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "insert_buffer",
               1,
               "BeforeLoads: Creating new hierarchical net "
               "(dbModNet) with base name '{}' in module '{}'",
               base_name,
               target_module->getName());
    new_mod_net = dbModNet::create(target_module, base_name);
  }

  // 4. Rewire Connections
  //
  //    Concept:
  //    [Driver] --(orig_net)--> [Buffer/A]
  //                             [Buffer/Y] --(new_net)--> [Target Loads]

  // 4.1. Connect Buffer Input to the Original Net
  //     Note: We only handle flat connectivity for the buffer input here.
  //     If the buffer is inside a sub-module, we assume the original net
  //     already traverses there (since we picked LCA).
  buf_input_iterm->connect(this);
  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Connected buffer input '{}' to original "
             "flat net '{}'",
             buf_input_iterm->getName(),
             this->getName());

  // Also connect to ModNet if it exists in this module
  std::set<dbModNet*> related_modnets;
  findRelatedModNets(related_modnets);
  dbModNet* orig_mod_net = nullptr;
  std::set<dbModNet*> modnets_in_target_module;
  for (dbModNet* modnet : related_modnets) {
    if (modnet->getParent() == target_module) {
      // There can be multiple modnets in the target module
      modnets_in_target_module.insert(modnet);
    }
  }

  if (modnets_in_target_module.size() > 1) {
    // There are multiple modnets in the target module.
    // Select the first modnet that the driver is connected to.
    //
    // Algorithm:
    // 1. Find the driver terminal of this flat net.
    // 2. Fanout traversal through modnets from the driver.
    // 3. Select the first modnet that is included in the
    //    modnets_in_target_module.
    orig_mod_net = getFirstDriverModNetInTargetModule(modnets_in_target_module);
  } else if (modnets_in_target_module.size() == 1) {
    orig_mod_net = *modnets_in_target_module.begin();
  }

  if (orig_mod_net) {
    buf_input_iterm->connect(orig_mod_net);
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "insert_buffer",
               1,
               "BeforeLoads: Connected buffer input '{}' to "
               "original hierarchical net '{}'",
               buf_input_iterm->getName(),
               orig_mod_net->getHierarchicalName());
  }

  // 4.2. Connect Buffer Output to the New Net (Flat & Hier)
  buf_output_iterm->connect(new_flat_net);
  if (new_mod_net) {
    buf_output_iterm->connect(new_mod_net);
  }
  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Connected buffer output '{}' to new "
             "flat net '{}'{}",
             buf_output_iterm->getName(),
             new_flat_net->getName(),
             new_mod_net ? fmt::format(" and new hierarchical net '{}'",
                                       new_mod_net->getHierarchicalName())
                         : "");

  // 4.3. Move Target Loads to the New Net
  int num_loads = load_pins.size();
  int load_idx = 0;
  for (dbObject* load_obj : load_pins) {
    if (load_obj->getObjectType() == dbITermObj) {
      dbITerm* load = static_cast<dbITerm*>(load_obj);
      // 4.3.1. Port Punching (Hierarchical Connection)
      // - If the load is in a deeper hierarchy than the buffer (target_module),
      //   we create hierarchical pins/nets to maintain logical consistency.
      // - We do this BEFORE disconnect to allow reusing existing hierarchical
      //   ports/nets.
      debugPrint(getImpl()->getLogger(),
                 utl::ODB,
                 "insert_buffer",
                 1,
                 "BeforeLoads: [load {}/{}] Creating hierarchical "
                 "connection "
                 "for load '{}'",
                 load_idx + 1,
                 num_loads,
                 load->getName());
      this->createHierarchicalConnection(load, buf_output_iterm, load_pins);
    } else {  // load_obj is a BTerm
      assert(load_obj->getObjectType() == dbBTermObj);
      dbBTerm* load = static_cast<dbBTerm*>(load_obj);
      load->disconnect();
      load->connect(new_flat_net, new_mod_net);
      debugPrint(getImpl()->getLogger(),
                 utl::ODB,
                 "insert_buffer",
                 1,
                 "BeforeLoads: [load {}/{}] Moved BTerm load '{}' "
                 "to new flat net '{}'{}",
                 load_idx + 1,
                 num_loads,
                 load->getName(),
                 new_flat_net->getName(),
                 new_mod_net ? fmt::format(" and new hierarchical net '{}'",
                                           new_mod_net->getHierarchicalName())
                             : "");
    }

    // For next iteration
    load_idx++;
  }

  // 5. Place the Buffer
  Point placement_loc;
  if (loc) {
    placement_loc = *loc;
  } else {
    placement_loc = computeCentroid(load_pins);
  }
  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Placing buffer '{}' at ({}, {})",
             buffer_inst->getName(),
             placement_loc.getX(),
             placement_loc.getY());
  placeNewBuffer(buffer_inst, &placement_loc, nullptr, nullptr);

  // 6. Propagate NDR if exists
  if (dbTechNonDefaultRule* rule = getNonDefaultRule()) {
    new_flat_net->setNonDefaultRule(rule);
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Successfully inserted a new buffer '{}'",
             buffer_inst->getName());
  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "insert_buffer",
             1,
             "-------------------------------------------------------");

  return buffer_inst;
}

// jk: move to proper location
dbModNet* dbNet::getFirstModNetInFaninOfLoads(
    const std::set<dbObject*>& load_pins,
    const std::set<dbModNet*>& modnets_in_target_module)
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

dbModNet* dbNet::getFirstDriverModNetInTargetModule(
    const std::set<dbModNet*>& modnets_in_target_module)
{
  // 1. Find the driver terminal of this flat net
  dbObject* driver_term = getFirstDriverTerm();
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

}  // namespace odb
