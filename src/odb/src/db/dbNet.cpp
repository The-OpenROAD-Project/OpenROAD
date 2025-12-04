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

dbNet* createBufferNet(dbNet* net,
                       dbBTerm* bterm,
                       const char* suffix,
                       dbModNet* mod_net,
                       dbModule* parent_mod,
                       const dbNameUniquifyType& uniquify)
{
  dbBlock* block = net->getBlock();
  if (bterm == nullptr) {
    // If not connecting to a BTerm, just append the suffix.
    const std::string new_net_name_str = std::string(net->getName()) + suffix;
    return dbNet::create(block, new_net_name_str.c_str(), uniquify, parent_mod);
  }

  // If the connected term is a port, the new net should take the name of the
  // port to maintain connectivity.
  const char* port_name = bterm->getConstName();

  // If the original net name is the same as the port name, it should be
  // renamed to avoid conflict.
  if (std::string_view(block->getBaseName(net->getConstName())) == port_name) {
    const std::string new_orig_net_name = block->makeNewNetName(
        parent_mod ? parent_mod->getModInst() : nullptr, "net", uniquify);
    net->rename(new_orig_net_name.c_str());

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

// Helper to generate unique name for port punching. jk: inefficient
std::string makeUniqueHierName(dbModule* module,
                               const std::string& base_name,
                               const char* suffix)
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
int getModuleDepth(dbModule* mod)
{
  int depth = 0;
  while (mod != nullptr) {
    mod = mod->getModInst() ? mod->getModInst()->getParent() : nullptr;
    depth++;
  }
  return depth;
}

// Helper: Find LCA (Lowest Common Ancestor)
dbModule* findLCA(dbModule* m1, dbModule* m2)
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

// jk: TODO: move
// Create hierarchical pins/nets bottom-up from leaf to target_module.
// This corresponds to "Port Punching".
//
// load_pin: The leaf pin deep in hierarchy.
// target_module: The module where the buffer is placed (top of the connection
// chain). top_mod_iterm: Output parameter, returns the top-most ModITerm
// connected to target_module.
bool createHierarchicalConnection(dbITerm* load_pin,
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
    dbObject* child_obj = (dbObject*) load_pin;
    std::string base_name
        = block->getBaseName(load_pin->getNet()->getConstName());

    // We are connecting FROM the buffer (in target_module) TO the load (deep
    // down). So from the perspective of the hierarchical modules, these are
    // INPUT ports.

    // Helper lambda to recursively check if all loads connected to a ModNet
    // (directly or through child modules) are in load_pins
    std::function<bool(dbModNet*)> allLoadsAreTargets
        = [&](dbModNet* net) -> bool {
      if (net == nullptr) {
        return true;
      }

      // Check all directly connected ITerms
      for (dbITerm* iterm : net->getITerms()) {
        if (load_pins.find(iterm) == load_pins.end()) {
          return false;
        }
      }

      // Check all loads reachable through child modules (ModITerms)
      for (dbModITerm* miterm : net->getModITerms()) {
        if (dbModBTerm* child_bterm = miterm->getChildModBTerm()) {
          if (dbModNet* child_net = child_bterm->getModNet()) {
            if (allLoadsAreTargets(child_net) == false) {
              return false;
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
            1022,
            "Cannot create hierarchical connection: '{}' is not a descendant "
            "of '{}'.",
            load_pin->getInst()->getName(),
            target_module->getName());
        break;
      }

      // Check if there's an existing hierarchical connection to reuse.
      dbModNet* existing_mod_net = nullptr;
      if (child_obj->getObjectType() == dbITermObj) {
        existing_mod_net = (static_cast<dbITerm*>(child_obj))->getModNet();
      } else if (child_obj->getObjectType() == dbModITermObj) {
        existing_mod_net = (static_cast<dbModITerm*>(child_obj))->getModNet();
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
        if (allLoadsAreTargets(existing_mod_net) == false) {
          safe_to_reuse = false;
        }

        if (safe_to_reuse) {
          for (dbModITerm* miterm : existing_mod_net->getModITerms()) {
            if (miterm != child_obj) {
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
            child_obj = static_cast<dbObject*>(parent_iterm);
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

      // Check if there is ANY other ModNet in this module that we can reuse.
      // This happens when we have multiple loads in the same module (or down
      // the hierarchy) that are being buffered. The first load creates the
      // port, and subsequent loads should reuse it.
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
              if (child_obj->getObjectType() == dbITermObj) {
                (static_cast<dbITerm*>(child_obj))->connect(mod_net);
              } else if (child_obj->getObjectType() == dbModITermObj) {
                (static_cast<dbModITerm*>(child_obj))->connect(mod_net);
              }

              reused_path = true;

              // Move up
              child_obj = static_cast<dbObject*>(parent_moditerm);
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
      if (child_obj->getObjectType() == dbITermObj) {
        dbITerm* load_iterm = static_cast<dbITerm*>(child_obj);
        // load_iterm->disconnect();  // jk: not ok. STA assert fail.
        load_iterm->connect(mod_net);
      } else if (child_obj->getObjectType() == dbModITermObj) {
        (static_cast<dbModITerm*>(child_obj))->connect(mod_net);
      }

      // 4. Create Pin (ModITerm) on the instance of current module in the
      // parent
      dbModITerm* mod_iterm
          = dbModITerm::create(parent_mod_inst, unique_name.c_str(), mod_bterm);

      // Prepare for next iteration (moving up)
      child_obj = static_cast<dbObject*>(mod_iterm);
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

}  // anonymous namespace

template class dbTable<_dbNet>;

_dbNet::_dbNet(_dbDatabase* db, const _dbNet& n)
    : _flags(n._flags),
      _name(nullptr),
      _next_entry(n._next_entry),
      _iterms(n._iterms),
      _bterms(n._bterms),
      _wire(n._wire),
      _global_wire(n._global_wire),
      _swires(n._swires),
      _cap_nodes(n._cap_nodes),
      _r_segs(n._r_segs),
      _non_default_rule(n._non_default_rule),
      guides_(n.guides_),
      tracks_(n.tracks_),
      _groups(n._groups),
      _weight(n._weight),
      _xtalk(n._xtalk),
      _ccAdjustFactor(n._ccAdjustFactor),
      _ccAdjustOrder(n._ccAdjustOrder)

{
  if (n._name) {
    _name = safe_strdup(n._name);
  }
  _drivingIterm = -1;
}

_dbNet::_dbNet(_dbDatabase* db)
{
  _flags._sig_type = dbSigType::SIGNAL;
  _flags._wire_type = dbWireType::ROUTED;
  _flags._special = 0;
  _flags._wild_connect = 0;
  _flags._wire_ordered = 0;
  _flags._unused2 = 0;
  _flags._disconnected = 0;
  _flags._spef = 0;
  _flags._select = 0;
  _flags._mark = 0;
  _flags._mark_1 = 0;
  _flags._wire_altered = 0;
  _flags._extracted = 0;
  _flags._rc_graph = 0;
  _flags._unused = 0;
  _flags._set_io = 0;
  _flags._io = 0;
  _flags._dont_touch = 0;
  _flags._fixed_bump = 0;
  _flags._source = dbSourceType::NONE;
  _flags._rc_disconnected = 0;
  _flags._block_rule = 0;
  _flags._has_jumpers = 0;
  _name = nullptr;
  _gndc_calibration_factor = 1.0;
  _cc_calibration_factor = 1.0;
  _weight = 1;
  _xtalk = 0;
  _ccAdjustFactor = -1;
  _ccAdjustOrder = 0;
  _drivingIterm = -1;
}

_dbNet::~_dbNet()
{
  if (_name) {
    free((void*) _name);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbNet& net)
{
  uint* bit_field = (uint*) &net._flags;
  stream << *bit_field;
  stream << net._name;
  stream << net._gndc_calibration_factor;
  stream << net._cc_calibration_factor;
  stream << net._next_entry;
  stream << net._iterms;
  stream << net._bterms;
  stream << net._wire;
  stream << net._global_wire;
  stream << net._swires;
  stream << net._cap_nodes;
  stream << net._r_segs;
  stream << net._non_default_rule;
  stream << net._weight;
  stream << net._xtalk;
  stream << net._ccAdjustFactor;
  stream << net._ccAdjustOrder;
  stream << net._groups;
  stream << net.guides_;
  stream << net.tracks_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbNet& net)
{
  uint* bit_field = (uint*) &net._flags;
  stream >> *bit_field;
  stream >> net._name;
  stream >> net._gndc_calibration_factor;
  stream >> net._cc_calibration_factor;
  stream >> net._next_entry;
  stream >> net._iterms;
  stream >> net._bterms;
  stream >> net._wire;
  stream >> net._global_wire;
  stream >> net._swires;
  stream >> net._cap_nodes;
  stream >> net._r_segs;
  stream >> net._non_default_rule;
  stream >> net._weight;
  stream >> net._xtalk;
  stream >> net._ccAdjustFactor;
  stream >> net._ccAdjustOrder;
  stream >> net._groups;
  stream >> net.guides_;
  _dbDatabase* db = net.getImpl()->getDatabase();
  if (db->isSchema(db_schema_net_tracks)) {
    stream >> net.tracks_;
  }

  return stream;
}

bool _dbNet::operator<(const _dbNet& rhs) const
{
  return strcmp(_name, rhs._name) < 0;
}

bool _dbNet::operator==(const _dbNet& rhs) const
{
  if (_flags._sig_type != rhs._flags._sig_type) {
    return false;
  }

  if (_flags._wire_type != rhs._flags._wire_type) {
    return false;
  }

  if (_flags._special != rhs._flags._special) {
    return false;
  }

  if (_flags._wild_connect != rhs._flags._wild_connect) {
    return false;
  }

  if (_flags._wire_ordered != rhs._flags._wire_ordered) {
    return false;
  }

  if (_flags._disconnected != rhs._flags._disconnected) {
    return false;
  }

  if (_flags._spef != rhs._flags._spef) {
    return false;
  }

  if (_flags._select != rhs._flags._select) {
    return false;
  }

  if (_flags._mark != rhs._flags._mark) {
    return false;
  }

  if (_flags._mark_1 != rhs._flags._mark_1) {
    return false;
  }

  if (_flags._wire_altered != rhs._flags._wire_altered) {
    return false;
  }

  if (_flags._extracted != rhs._flags._extracted) {
    return false;
  }

  if (_flags._rc_graph != rhs._flags._rc_graph) {
    return false;
  }

  if (_flags._set_io != rhs._flags._set_io) {
    return false;
  }

  if (_flags._io != rhs._flags._io) {
    return false;
  }

  if (_flags._dont_touch != rhs._flags._dont_touch) {
    return false;
  }

  if (_flags._fixed_bump != rhs._flags._fixed_bump) {
    return false;
  }

  if (_flags._source != rhs._flags._source) {
    return false;
  }

  if (_flags._rc_disconnected != rhs._flags._rc_disconnected) {
    return false;
  }

  if (_flags._block_rule != rhs._flags._block_rule) {
    return false;
  }

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0) {
      return false;
    }
  } else if (_name || rhs._name) {
    return false;
  }

  if (_gndc_calibration_factor != rhs._gndc_calibration_factor) {
    return false;
  }
  if (_cc_calibration_factor != rhs._cc_calibration_factor) {
    return false;
  }

  if (_next_entry != rhs._next_entry) {
    return false;
  }

  if (_iterms != rhs._iterms) {
    return false;
  }

  if (_bterms != rhs._bterms) {
    return false;
  }

  if (_wire != rhs._wire) {
    return false;
  }

  if (_global_wire != rhs._global_wire) {
    return false;
  }

  if (_swires != rhs._swires) {
    return false;
  }

  if (_cap_nodes != rhs._cap_nodes) {
    return false;
  }

  if (_r_segs != rhs._r_segs) {
    return false;
  }

  if (_non_default_rule != rhs._non_default_rule) {
    return false;
  }

  if (_weight != rhs._weight) {
    return false;
  }

  if (_xtalk != rhs._xtalk) {
    return false;
  }

  if (_ccAdjustFactor != rhs._ccAdjustFactor) {
    return false;
  }

  if (_ccAdjustOrder != rhs._ccAdjustOrder) {
    return false;
  }

  if (_groups != rhs._groups) {
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
  return net->_name;
}

const char* dbNet::getConstName() const
{
  _dbNet* net = (_dbNet*) this;
  return net->_name;
}

void dbNet::printNetName(FILE* fp, bool idFlag, bool newLine)
{
  if (idFlag) {
    fprintf(fp, " %d", getId());
  }

  _dbNet* net = (_dbNet*) this;
  fprintf(fp, " %s", net->_name);

  if (newLine) {
    fprintf(fp, "\n");
  }
}

bool dbNet::rename(const char* name)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (block->_net_hash.hasMember(name)) {
    return false;
  }

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbNet({} {:p}) '{}', rename to '{}'",
               getId(),
               static_cast<void*>(this),
               getName(),
               name);
    block->_journal->updateField(this, _dbNet::NAME, net->_name, name);
  }

  block->_net_hash.remove(net);
  free((void*) net->_name);
  net->_name = safe_strdup(name);
  block->_net_hash.insert(net);

  return true;
}

void dbNet::swapNetNames(dbNet* source, bool ok_to_journal)
{
  _dbNet* dest_net = (_dbNet*) this;
  _dbNet* source_net = (_dbNet*) source;
  _dbBlock* block = (_dbBlock*) source_net->getOwner();

  char* dest_name_ptr = dest_net->_name;
  char* source_name_ptr = source_net->_name;

  // allow undo..
  if (block->_journal && ok_to_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: swap dbName (dbNet) between {} at id {} and {} at id {}",
               source->getName(),
               source->getId(),
               getName(),
               getId());
    block->_journal->beginAction(dbJournal::SWAP_OBJECT);
    // a name
    block->_journal->pushParam(dbNameObj);
    // the type of name swap
    block->_journal->pushParam(dbNetObj);
    // stash the source and dest in that order,
    // let undo reorder
    block->_journal->pushParam(source_net->getId());
    block->_journal->pushParam(dest_net->getId());
    block->_journal->endAction();
  }

  block->_net_hash.remove(dest_net);
  block->_net_hash.remove(source_net);

  // swap names without copy, just swap the pointers
  dest_net->_name = source_name_ptr;
  source_net->_name = dest_name_ptr;

  block->_net_hash.insert(dest_net);
  block->_net_hash.insert(source_net);
}

bool dbNet::isRCDisconnected()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._rc_disconnected == 1;
}

void dbNet::setRCDisconnected(bool value)
{
  _dbNet* net = (_dbNet*) this;
  net->_flags._rc_disconnected = value;
}

int dbNet::getWeight()
{
  _dbNet* net = (_dbNet*) this;
  return net->_weight;
}

void dbNet::setWeight(int weight)
{
  _dbNet* net = (_dbNet*) this;
  net->_weight = weight;
}

dbSourceType dbNet::getSourceType()
{
  _dbNet* net = (_dbNet*) this;
  dbSourceType t(net->_flags._source);
  return t;
}

void dbNet::setSourceType(dbSourceType type)
{
  _dbNet* net = (_dbNet*) this;
  net->_flags._source = type;
}

int dbNet::getXTalkClass()
{
  _dbNet* net = (_dbNet*) this;
  return net->_xtalk;
}

void dbNet::setXTalkClass(int value)
{
  _dbNet* net = (_dbNet*) this;
  net->_xtalk = value;
}

float dbNet::getCcAdjustFactor()
{
  _dbNet* net = (_dbNet*) this;
  return net->_ccAdjustFactor;
}

void dbNet::setCcAdjustFactor(float factor)
{
  _dbNet* net = (_dbNet*) this;
  net->_ccAdjustFactor = factor;
}

uint dbNet::getCcAdjustOrder()
{
  _dbNet* net = (_dbNet*) this;
  return net->_ccAdjustOrder;
}

void dbNet::setCcAdjustOrder(uint order)
{
  _dbNet* net = (_dbNet*) this;
  net->_ccAdjustOrder = order;
}

void dbNet::setDrivingITerm(int id)
{
  _dbNet* net = (_dbNet*) this;
  net->_drivingIterm = id;
}

int dbNet::getDrivingITermId() const
{
  _dbNet* net = (_dbNet*) this;
  return net->_drivingIterm;
}

dbITerm* dbNet::getDrivingITerm() const
{
  _dbNet* net = (_dbNet*) this;
  if (net->_drivingIterm <= 0) {
    return nullptr;
  }
  return dbITerm::getITerm(getBlock(), net->_drivingIterm);
}

bool dbNet::hasFixedBump()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._fixed_bump == 1;
}

void dbNet::setFixedBump(bool value)
{
  _dbNet* net = (_dbNet*) this;
  net->_flags._fixed_bump = value;
}

void dbNet::setWireType(dbWireType wire_type)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._wire_type = wire_type.getValue();

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setWireType: {}",
               getId(),
               wire_type.getValue());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

dbWireType dbNet::getWireType() const
{
  _dbNet* net = (_dbNet*) this;
  return dbWireType(net->_flags._wire_type);
}

dbSigType dbNet::getSigType() const
{
  _dbNet* net = (_dbNet*) this;
  return dbSigType(net->_flags._sig_type);
}

void dbNet::setSigType(dbSigType sig_type)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._sig_type = sig_type.getValue();

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setSigType: {}",
               getId(),
               sig_type.getValue());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

float dbNet::getGndcCalibFactor()
{
  _dbNet* net = (_dbNet*) this;
  return net->_gndc_calibration_factor;
}

void dbNet::setGndcCalibFactor(float gndcCalib)
{
  _dbNet* net = (_dbNet*) this;
  net->_gndc_calibration_factor = gndcCalib;
}

float dbNet::getRefCc()
{
  _dbNet* net = (_dbNet*) this;
  return net->_refCC;
}

void dbNet::setRefCc(float cap)
{
  _dbNet* net = (_dbNet*) this;
  net->_refCC = cap;
}

float dbNet::getCcCalibFactor()
{
  _dbNet* net = (_dbNet*) this;
  return net->_cc_calibration_factor;
}

void dbNet::setCcCalibFactor(float ccCalib)
{
  _dbNet* net = (_dbNet*) this;
  net->_cc_calibration_factor = ccCalib;
}

float dbNet::getDbCc()
{
  _dbNet* net = (_dbNet*) this;
  return net->_dbCC;
}

void dbNet::setDbCc(float cap)
{
  _dbNet* net = (_dbNet*) this;
  net->_dbCC = cap;
}

void dbNet::addDbCc(float cap)
{
  _dbNet* net = (_dbNet*) this;
  net->_dbCC += cap;
}

float dbNet::getCcMatchRatio()
{
  _dbNet* net = (_dbNet*) this;
  return net->_CcMatchRatio;
}

void dbNet::setCcMatchRatio(float ratio)
{
  _dbNet* net = (_dbNet*) this;
  net->_CcMatchRatio = ratio;
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
  if (net->_ccAdjustFactor > 0) {
    getImpl()->getLogger()->warn(
        utl::ODB,
        48,
        "Net {} {} had been CC adjusted by {}. Please unadjust first.",
        getId(),
        getConstName(),
        net->_ccAdjustFactor);
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
  net->_ccAdjustFactor = adjFactor;
  net->_ccAdjustOrder = adjOrder;
  return true;
}

void dbNet::undoAdjustedCC(std::vector<dbCCSeg*>& adjustedCC,
                           std::vector<dbNet*>& halonets)
{
  _dbNet* net = (_dbNet*) this;
  if (net->_ccAdjustFactor < 0) {
    return;
  }
  const uint adjOrder = net->_ccAdjustOrder;
  const float adjFactor = 1 / net->_ccAdjustFactor;
  for (dbCapNode* node : getCapNodes()) {
    node->adjustCC(adjOrder, adjFactor, adjustedCC, halonets);
  }
  net->_ccAdjustFactor = -1;
  net->_ccAdjustOrder = 0;
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
  return net->_flags._spef == 1;
}

void dbNet::setSpef(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._spef = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setSpef: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isSelect()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._select == 1;
}

void dbNet::setSelect(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._select = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setSelect: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
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
  return net->_flags._mark == 1;
}

void dbNet::setMark(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._mark = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setMark: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isMark_1ed()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._mark_1 == 1;
}

void dbNet::setMark_1(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._mark_1 = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setMark_1: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isWireOrdered()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._wire_ordered == 1;
}

void dbNet::setWireOrdered(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._wire_ordered = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setWireOrdered: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isDisconnected()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._disconnected == 1;
}

void dbNet::setDisconnected(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._disconnected = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setDisconnected: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

void dbNet::setWireAltered(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._wire_altered = (value == true) ? 1 : 0;
  if (value) {
    net->_flags._wire_ordered = 0;
  }

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setWireAltered: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isWireAltered()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._wire_altered == 1;
}

void dbNet::setExtracted(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._extracted = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setExtracted: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isExtracted()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._extracted == 1;
}

void dbNet::setRCgraph(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._rc_graph = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setRCgraph: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isRCgraph()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._rc_graph == 1;
}

dbBlock* dbNet::getBlock() const
{
  return (dbBlock*) getImpl()->getOwner();
}

dbSet<dbITerm> dbNet::getITerms() const
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbITerm>(net, block->_net_iterm_itr);
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
  return dbSet<dbBTerm>(net, block->_net_bterm_itr);
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

    if (iterm->getIoType() != dbIoType::OUTPUT) {
      continue;
    }

    return iterm;
  }

  for (dbBTerm* bterm : getBTerms()) {
    if (bterm->getSigType().isSupply()) {
      continue;
    }

    if (bterm->getIoType() == dbIoType::OUTPUT
        || bterm->getIoType() == dbIoType::FEEDTHRU) {
      continue;
    }

    return bterm;
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
  return dbSet<dbSWire>(net, block->_swire_itr);
}

dbSWire* dbNet::getFirstSWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->_swires == 0) {
    return nullptr;
  }

  return (dbSWire*) block->_swire_tbl->getPtr(net->_swires);
}

dbWire* dbNet::getWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->_wire == 0) {
    return nullptr;
  }

  return (dbWire*) block->_wire_tbl->getPtr(net->_wire);
}

dbWire* dbNet::getGlobalWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->_global_wire == 0) {
    return nullptr;
  }

  return (dbWire*) block->_wire_tbl->getPtr(net->_global_wire);
}

bool dbNet::setIOflag()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  const uint prev_flags = flagsToUInt(net);
  net->_flags._set_io = 1;
  net->_flags._io = 0;
  const uint n = getBTerms().size();

  if (n > 0) {
    net->_flags._io = 1;

    if (block->_journal) {
      debugPrint(getImpl()->getLogger(),
                 utl::ODB,
                 "DB_ECO",
                 1,
                 "ECO: net {}, setIOFlag",
                 getId());
      block->_journal->updateField(
          this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
    }

    return true;
  }

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setIOFlag",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }

  return false;
}

bool dbNet::isIO()
{
  _dbNet* net = (_dbNet*) this;

  if (net->_flags._set_io > 0) {
    return net->_flags._io == 1;
  }
  return setIOflag();
}

void dbNet::setDoNotTouch(bool v)
{
  _dbNet* net = (_dbNet*) this;
  net->_flags._dont_touch = v;
}

bool dbNet::isDoNotTouch() const
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._dont_touch == 1;
}

bool dbNet::isSpecial() const
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._special == 1;
}

void dbNet::setSpecial()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._special = 1;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setSpecial",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

void dbNet::clearSpecial()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._special = 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, clearSpecial",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
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
  return net->_flags._wild_connect == 1;
}

void dbNet::setWildConnected()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  // uint prev_flags = flagsToUInt(net);

  net->_flags._wild_connect = 1;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setWildConnected",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

void dbNet::clearWildConnected()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  // uint prev_flags = flagsToUInt(net);

  net->_flags._wild_connect = 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, clearWildConnected",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

dbSet<dbRSeg> dbNet::getRSegs()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbRSeg>(net, block->_r_seg_itr);
}

void dbNet::reverseRSegs()
{
  dbSet<dbRSeg> rSet = getRSegs();
  rSet.reverse();
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbNet {}, reverse rsegs sequence",
               getId());
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbNetObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(_dbNet::REVERSE_RSEG);
    block->_journal->endAction();
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
  uint pid = net->_r_segs;
  net->_r_segs = rid;
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbNet {}, set 1stRSegNode {}",
               getId(),
               rid);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbNetObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(_dbNet::HEAD_RSEG);
    block->_journal->pushParam(pid);
    block->_journal->pushParam(rid);
    block->_journal->endAction();
  }
}

uint dbNet::get1stRSegId()
{
  _dbNet* net = (_dbNet*) this;
  return net->_r_segs;
}

dbRSeg* dbNet::getZeroRSeg()
{
  _dbNet* net = (_dbNet*) this;
  if (net->_r_segs == 0) {
    return nullptr;
  }
  dbRSeg* zrc = dbRSeg::getRSeg((dbBlock*) net->getOwner(), net->_r_segs);
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
  return dbSet<dbCapNode>(net, block->_cap_node_itr);
}

void dbNet::setTermExtIds(int capId)  // 1: capNodeId, 0: reset
{
  dbSet<dbCapNode> nodeSet = getCapNodes();
  dbSet<dbCapNode>::iterator rc_itr;
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();

  if (block->_journal) {
    if (capId) {
      debugPrint(getImpl()->getLogger(),
                 utl::ODB,
                 "DB_ECO",
                 1,
                 "ECO: set net {} term extId",
                 getId());
    } else {
      debugPrint(getImpl()->getLogger(),
                 utl::ODB,
                 "DB_ECO",
                 1,
                 "ECO: reset net {} term extId",
                 getId());
    }
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbNetObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(_dbNet::TERM_EXTID);
    block->_journal->pushParam(capId);
    block->_journal->endAction();
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
void dbNet::set1stCapNodeId(uint cid)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint pid = net->_cap_nodes;
  net->_cap_nodes = cid;
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbNet {}, set 1stCapNode {}",
               getId(),
               cid);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbNetObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(_dbNet::HEAD_CAPNODE);
    block->_journal->pushParam(pid);
    block->_journal->pushParam(cid);
    block->_journal->endAction();
  }
}

uint dbNet::get1stCapNodeId()
{
  _dbNet* net = (_dbNet*) this;
  return net->_cap_nodes;
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
      if (seg_impl->_cap_node[0] == cap_id) {
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
      if (seg_impl->_cap_node[1] == cap_id) {
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
  uint prev_rule = net->_non_default_rule;
  bool prev_block_rule = net->_flags._block_rule;

  if (rule == nullptr) {
    net->_non_default_rule = 0U;
    net->_flags._block_rule = 0;
  } else {
    net->_non_default_rule = rule->getImpl()->getOID();
    net->_flags._block_rule = rule->isBlockRule();
  }

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setNonDefaultRule: ",
               getId(),
               (rule) ? rule->getImpl()->getOID() : 0);
    // block->_journal->updateField(this, _dbNet::NON_DEFAULT_RULE, prev_rule,
    // net->_non_default_rule );
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(rule->getObjectType());
    block->_journal->pushParam(rule->getId());
    block->_journal->pushParam(_dbNet::NON_DEFAULT_RULE);
    block->_journal->pushParam(prev_rule);
    block->_journal->pushParam((uint) net->_non_default_rule);
    block->_journal->pushParam(prev_block_rule);
    block->_journal->pushParam((bool) net->_flags._block_rule);
    block->_journal->endAction();
  }
}

dbTechNonDefaultRule* dbNet::getNonDefaultRule()
{
  _dbNet* net = (_dbNet*) this;

  if (net->_non_default_rule == 0) {
    return nullptr;
  }

  dbDatabase* db = (dbDatabase*) net->getDatabase();

  if (net->_flags._block_rule) {
    _dbBlock* block = (_dbBlock*) net->getOwner();
    return (dbTechNonDefaultRule*) block->_non_default_rule_tbl->getPtr(
        net->_non_default_rule);
  }

  _dbTech* tech = (_dbTech*) db->getTech();
  return (dbTechNonDefaultRule*) tech->_non_default_rule_tbl->getPtr(
      net->_non_default_rule);
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

  net->_swires = 0;
}

dbNet* dbNet::create(dbBlock* block_, const char* name_, bool skipExistingCheck)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (!skipExistingCheck && block->_net_hash.hasMember(name_)) {
    return nullptr;
  }

  _dbNet* net = block->_net_tbl->create();
  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: create dbNet({}, {:p}) '{}'",
               net->getId(),
               static_cast<void*>(net),
               name_);
    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbNetObj);
    block->_journal->pushParam(name_);
    block->_journal->pushParam(net->getOID());
    block->_journal->endAction();
  }

  net->_name = safe_strdup(name_);
  block->_net_hash.insert(net);

  for (auto cb : block->_callbacks) {
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

  if (net->_flags._dont_touch) {
    net->getLogger()->error(
        utl::ODB, 364, "Attempt to destroy dont_touch net {}", net->_name);
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

  if (net->_wire != 0) {
    dbWire* wire = (dbWire*) block->_wire_tbl->getPtr(net->_wire);
    dbWire::destroy(wire);
  }

  for (const dbId<_dbGroup>& _group_id : net->_groups) {
    dbGroup* group = (dbGroup*) block->_group_tbl->getPtr(_group_id);
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

  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: delete dbNet({}, {:p}) '{}'",
               net->getId(),
               static_cast<void*>(net),
               net->_name);
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbNetObj);
    block->_journal->pushParam(net_->getName());
    block->_journal->pushParam(net->getOID());
    uint* flags = (uint*) &net->_flags;
    block->_journal->pushParam(*flags);
    block->_journal->pushParam(net->_non_default_rule);
    block->_journal->endAction();
  }

  for (auto cb : block->_callbacks) {
    cb->inDbNetDestroy(net_);
  }

  dbProperty::destroyProperties(net);
  block->_net_hash.remove(net);
  block->_net_tbl->destroy(net);
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
  return (dbNet*) block->_net_tbl->getPtr(dbid_);
}

dbNet* dbNet::getValidNet(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  if (!block->_net_tbl->validId(dbid_)) {
    return nullptr;
  }
  return (dbNet*) block->_net_tbl->getPtr(dbid_);
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

  for (auto callback : block->_callbacks) {
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
  return dbSet<dbGuide>(net, block->_guide_itr);
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
  return dbSet<dbNetTrack>(net, block->_net_track_itr);
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
    has_jumpers = net->_flags._has_jumpers == 1;
  }
  return has_jumpers;
}

void dbNet::setJumpers(bool has_jumpers)
{
  _dbNet* net = (_dbNet*) this;
  _dbDatabase* db = net->getImpl()->getDatabase();
  if (db->isSchema(db_schema_has_jumpers)) {
    net->_flags._has_jumpers = has_jumpers ? 1 : 0;
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

  info.children_["name"].add(_name);
  info.children_["groups"].add(_groups);
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

  for (dbModNet* modnet : modnets) {
    std::string name = modnet->getHierarchicalName();
    size_t num_delimiters = std::count(name.begin(), name.end(), delim);
    if (highest == nullptr || num_delimiters < min_delimiters) {
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
  dbNet* new_net = createBufferNet(
      this, term_bterm, suffix, orig_mod_net, parent_mod, uniquify);
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
      if (loads_on_same_db_net && load->getNet() != this) {
        getImpl()->getLogger()->error(utl::ODB,
                                      1020,
                                      "BeforeLoads: Load pin {} is "
                                      "not connected to net {}",
                                      load->getName(),
                                      getName());
        return nullptr;
      }

      // Check dont_touch on the instance or the load pin's net
      if (checkDontTouch(this, nullptr, load)) {
        getImpl()->getLogger()->warn(utl::ODB,
                                     1017,
                                     "BeforeLoads: Load pin {} or "
                                     "net is dont_touch.",
                                     load->getName());
        return nullptr;
      }
      curr_mod = load->getInst()->getModule();
    } else if (load_obj->getObjectType() == dbBTermObj) {
      dbBTerm* load = static_cast<dbBTerm*>(load_obj);

      // Check connectivity
      if (loads_on_same_db_net && load->getNet() != this) {
        getImpl()->getLogger()->error(utl::ODB,
                                      999,
                                      "BeforeLoads: Load pin {} is "
                                      "not connected to net {}",
                                      load->getName(),
                                      getName());
        return nullptr;
      }

      // Check dont_touch on the net
      if (checkDontTouch(this, nullptr, nullptr)) {
        getImpl()->getLogger()->warn(utl::ODB,
                                     1021,
                                     "BeforeLoads: Load pin {} or "
                                     "net is dont_touch.",
                                     load->getName());
        return nullptr;
      }
      curr_mod = block->getTopModule();
    } else {
      getImpl()->getLogger()->error(
          utl::ODB, 1019, "BeforeLoads: Load object is not an ITerm or BTerm.");
      return nullptr;
    }

    if (first) {
      target_module = curr_mod;
      first = false;
    } else {
      target_module = findLCA(target_module, curr_mod);
    }
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: LCA module: {}",
             target_module ? target_module->getName() : "null");

  // Print net connectivities including both this flat and related hier nets
  if (getImpl()->getLogger()->debugCheck(utl::ODB, "insert_buffer", 2)) {
    dump(true);
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
        utl::ODB, 1018, "BeforeLoads: Failed to create buffer instance.");
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
  for (dbModNet* modnet : related_modnets) {
    if (modnet->getParent() == target_module) {
      orig_mod_net = modnet;
      break;
    }
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
      createHierarchicalConnection(load, buf_output_iterm, load_pins);
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

  return buffer_inst;
}

}  // namespace odb
