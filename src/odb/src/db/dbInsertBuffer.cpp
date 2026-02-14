// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbInsertBuffer.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <string_view>

#include "dbInst.h"
#include "dbModule.h"
#include "dbNet.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

dbInsertBuffer::dbInsertBuffer(dbNet* net)
    : net_(net),
      block_(net ? net->getBlock() : nullptr),
      logger_(net ? net->getImpl()->getLogger() : nullptr)
{
  assert(net_ != nullptr);
  assert(block_ != nullptr);
  assert(logger_ != nullptr);
}

void dbInsertBuffer::resetMembers()
{
  buf_input_iterm_ = nullptr;
  buf_output_iterm_ = nullptr;
  new_flat_net_ = nullptr;
  new_mod_net_ = nullptr;
  target_module_ = nullptr;
  buffer_master_ = nullptr;
  new_buf_base_name_ = kDefaultBufBaseName;
  new_net_base_name_ = kDefaultNetBaseName;
  uniquify_ = dbNameUniquifyType::ALWAYS;
  orig_drvr_pin_ = nullptr;
  is_target_only_cache_.clear();
  module_reusable_nets_.clear();
}

dbInst* dbInsertBuffer::insertBufferBeforeLoad(
    dbObject* load_input_term,
    const dbMaster* buffer_master,
    const Point* loc,
    const char* new_buf_base_name,
    const char* new_net_base_name,
    const dbNameUniquifyType& uniquify)
{
  dlogInsertBufferStart(++insert_buffer_call_count_, "BeforeLoad");

  return insertBufferSimple(load_input_term,
                            buffer_master,
                            loc,
                            new_buf_base_name,
                            new_net_base_name,
                            uniquify,
                            /* insertBefore */ true);
}

dbInst* dbInsertBuffer::insertBufferAfterDriver(
    dbObject* drvr_output_term,
    const dbMaster* buffer_master,
    const Point* loc,
    const char* new_buf_base_name,
    const char* new_net_base_name,
    const dbNameUniquifyType& uniquify)
{
  dlogInsertBufferStart(++insert_buffer_call_count_, "AfterDriver");

  return insertBufferSimple(drvr_output_term,
                            buffer_master,
                            loc,
                            new_buf_base_name,
                            new_net_base_name,
                            uniquify,
                            /* insertBefore */ false);
}

dbInst* dbInsertBuffer::insertBufferSimple(dbObject* term_obj,
                                           const dbMaster* buffer_master,
                                           const Point* loc,
                                           const char* new_buf_base_name,
                                           const char* new_net_base_name,
                                           const dbNameUniquifyType& uniquify,
                                           bool insertBefore)
{
  validateArgumentsSimple(term_obj, buffer_master);
  resetMembers();

  target_module_ = (term_obj->getObjectType() == dbITermObj)
                       ? static_cast<dbITerm*>(term_obj)->getInst()->getModule()
                       : nullptr;
  buffer_master_ = buffer_master;
  new_buf_base_name_ = new_buf_base_name;
  new_net_base_name_ = new_net_base_name;
  uniquify_ = uniquify;

  // Store original driver pin before buffer insertion modifies net structure
  orig_drvr_pin_ = net_->getFirstDriverTerm();

  // 1. Check terminal type and connectivity.
  dbModNet* orig_mod_net = nullptr;
  if (!validateTermAndGetModNet(term_obj, orig_mod_net)) {
    return nullptr;
  }

  // 2. Check buffer validity and create buffer instance
  dbInst* buffer_inst = checkAndCreateBuffer();
  if (buffer_inst == nullptr) {
    return nullptr;
  }

  // 3. Create new net for one side of the buffer
  std::set<dbObject*> terms;
  terms.insert(term_obj);
  new_flat_net_ = createNewFlatNet(terms);

  // 4. Rewire
  rewireBufferSimple(insertBefore, orig_mod_net, term_obj);

  // 5. Place the new buffer
  if (loc) {
    placeBufferAtLocation(buffer_inst, *loc);
  } else {
    placeBufferAtPin(buffer_inst, term_obj);
  }

  // 6. Set buffer attributes
  setBufferAttributes(buffer_inst);

  checkSanity();

  return buffer_inst;
}

dbInst* dbInsertBuffer::insertBufferBeforeLoads(
    const std::set<dbObject*>& load_pins,
    const dbMaster* buffer_master,
    const Point* loc,
    const char* new_buf_base_name,
    const char* new_net_base_name,
    const dbNameUniquifyType& uniquify,
    bool loads_on_diff_nets)
{
  validateArgumentsBeforeLoads(load_pins, buffer_master);
  dlogInsertBufferStart(++insert_buffer_call_count_, "BeforeLoads");

  resetMembers();

  buffer_master_ = buffer_master;
  new_buf_base_name_ = new_buf_base_name;
  new_net_base_name_ = new_net_base_name;
  uniquify_ = uniquify;

  // Store original driver pin before buffer insertion modifies net structure
  orig_drvr_pin_ = net_->getFirstDriverTerm();

  dlogBeforeLoadsParams(load_pins, loc, loads_on_diff_nets);

  // 1. Validate Load Pins & Find Lowest Common Ancestor (Hierarchy)
  target_module_ = validateLoadPinsAndFindLCA(load_pins, loads_on_diff_nets);
  if (target_module_ == nullptr) {
    return nullptr;  // Validation failed
  }

  // 2. Create Buffer Instance
  dbInst* buffer_inst = checkAndCreateBuffer();
  if (buffer_inst == nullptr) {
    logger_->error(
        utl::ODB,
        1205,
        "InsertBufferBeforeLoads: Failed to create buffer instance.");
  }

  // 3. Create Buffer Net
  createNewFlatAndHierNets(load_pins);
  if (new_flat_net_ == nullptr) {
    dbInst::destroy(buffer_inst);
    return nullptr;
  }

  // 4. Rewire load pin connections
  populateReusableModNets(load_pins);
  rewireBufferLoadPins(load_pins);

  // 5. Place the Buffer
  if (loc) {
    placeBufferAtLocation(buffer_inst, *loc);
  } else {
    placeBufferAtCentroid(buffer_inst, orig_drvr_pin_, load_pins);
  }

  // 6. Set buffer attributes
  setBufferAttributes(buffer_inst);

  dlogInsertBufferSuccess(buffer_inst);
  dlogSeparator();

  checkSanity();

  return buffer_inst;
}

bool dbInsertBuffer::validateTermAndGetModNet(const dbObject* term_obj,
                                              dbModNet*& mod_net) const
{
  mod_net = nullptr;

  // Check dont_touch flag of the target net
  if (net_->isDoNotTouch()) {
    return false;
  }

  // Validate term type and connectivity
  if (term_obj->getObjectType() == dbITermObj) {
    const dbITerm* iterm = static_cast<const dbITerm*>(term_obj);
    if (iterm->getNet() != net_) {
      return false;  // Not connected to this net
    }

    // Check dont_touch attribute
    if (checkDontTouch(iterm)) {
      return false;
    }

    mod_net = iterm->getModNet();
  } else if (term_obj->getObjectType() == dbBTermObj) {
    const dbBTerm* bterm = static_cast<const dbBTerm*>(term_obj);
    if (bterm->getNet() != net_) {
      return false;  // Not connected to this net
    }
    mod_net = bterm->getModNet();
  } else {
    logger_->error(
        utl::ODB,
        1217,
        "validateTermAndGetModNet: term_obj ({}) must be dbITerm or dbBTerm",
        term_obj->getTypeName());
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
  dbInst* buffer_inst = dbInst::create(block_,
                                       const_cast<dbMaster*>(buffer_master_),
                                       new_buf_base_name_,
                                       uniquify_,
                                       target_module_);

  buf_input_iterm_ = buffer_inst->getITerm(input_mterm);
  buf_output_iterm_ = buffer_inst->getITerm(output_mterm);

  return buffer_inst;
}

bool dbInsertBuffer::checkDontTouch(const dbITerm* iterm) const
{
  if (iterm == nullptr) {
    return false;
  }

  if (iterm->getInst()->isDoNotTouch()) {
    return true;
  }

  dbNet* net = iterm->getNet();
  if (net != nullptr && net->isDoNotTouch()) {
    return true;
  }

  return false;
}

void dbInsertBuffer::placeBufferAtLocation(dbInst* buffer_inst,
                                           const Point& loc,
                                           const char* reason)
{
  buffer_inst->setLocation(loc.getX(), loc.getY());
  buffer_inst->setPlacementStatus(dbPlacementStatus::PLACED);
  dlogPlacedBuffer(buffer_inst, loc, reason);
}

void dbInsertBuffer::placeBufferAtPin(dbInst* buffer_inst, const dbObject* term)
{
  int x = 0;
  int y = 0;
  if (getPinLocation(term, x, y)) {
    placeBufferAtLocation(buffer_inst, Point(x, y), "pin location");
  } else {
    buffer_inst->setPlacementStatus(dbPlacementStatus::UNPLACED);
    dlogUnplacedBuffer(buffer_inst, "pin location not available");
  }
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

bool dbInsertBuffer::getPinLocation(const dbObject* pin, int& x, int& y) const
{
  if (pin == nullptr) {
    return false;
  }

  if (pin->getObjectType() == dbITermObj) {
    const dbITerm* iterm = static_cast<const dbITerm*>(pin);
    dbInst* inst = iterm->getInst();

    // Check if instance is placed first
    if (!inst->isPlaced()) {
      return false;
    }

    if (iterm->getAvgXY(&x, &y)) {
      return true;
    }

    // Fallback to instance origin (only if placed, which is guaranteed above)
    Point origin = inst->getOrigin();
    x = origin.getX();
    y = origin.getY();
    return true;
  }

  if (pin->getObjectType() == dbBTermObj) {
    const dbBTerm* bterm = static_cast<const dbBTerm*>(pin);
    return bterm->getFirstPinLocation(x, y);
  }

  return false;
}

bool dbInsertBuffer::computeCentroid(const dbObject* drvr_pin,
                                     const std::set<dbObject*>& load_pins,
                                     Point& result) const
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

  // If no valid pin locations found (all unplaced), return false
  if (count == 0) {
    return false;
  }

  result
      = Point(static_cast<int>(sum_x / count), static_cast<int>(sum_y / count));
  return true;
}

dbNet* dbInsertBuffer::createNewFlatNet(
    const std::set<dbObject*>& connected_terms)
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

  std::string new_net_name = (new_net_base_name_) ? new_net_base_name_ : "net";
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
          new_net_name.c_str(),
          dbNameUniquifyType::ALWAYS);
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
  dbNet* new_net = dbNet::create(
      block_, new_net_name.c_str(), new_net_uniquify, target_module_);
  if (new_net == nullptr) {
    logger_->error(
        utl::ODB,
        1203,
        "Failed to create a new net '{}'. There may be a name collision.",
        new_net_name);
  }

  return new_net;
}

std::string dbInsertBuffer::makeUniqueHierName(const dbModule* module,
                                               const std::string& base_name,
                                               const char* suffix) const
{
  std::string name = (suffix == nullptr) ? base_name : base_name + suffix;
  const char hier_delim = block_->getHierarchyDelimiter();
  const std::string hier_prefix
      = module->isTop() ? "" : (module->getHierarchicalName() + hier_delim);

  // Helper to check if a dbNet is an internal net of the 'module'.
  // Note that the new name must not conflict with any internal flat nets.
  auto hasInternalDbNet = [&](const std::string& net_base_name) {
    dbNet* net = block_->findNet((hier_prefix + net_base_name).c_str());
    return net != nullptr && net->isInternalTo(const_cast<dbModule*>(module));
  };

  // Ensure uniqueness against ModNets, ModBTerms, and internal dbNets
  int id = 0;
  std::string unique_name = name;
  while (module->getModNet(unique_name.c_str())
         || module->findModBTerm(unique_name.c_str())
         || hasInternalDbNet(unique_name)) {
    unique_name = fmt::format("{}_{}", name, id++);
  }

  return unique_name;
}

int dbInsertBuffer::getModuleDepth(const dbModule* mod) const
{
  int depth = 0;
  while (mod != nullptr) {
    mod = mod->getParentModule();
    depth++;
  }
  return depth;
}

dbModule* dbInsertBuffer::findLCA(dbModule* m1, dbModule* m2) const
{
  dbModule* top_module = block_->getTopModule();
  if (m1 == nullptr || m2 == nullptr) {
    return top_module;
  }

  if (m1->getOwner() != m2->getOwner()) {
    logger_->error(utl::ODB,
                   1212,
                   "findLCA: Two modules '{}' (block: {}) and '{}' (block: {}) "
                   "must be in the same block.",
                   m1->getName(),
                   m1->getOwner()->getName(),
                   m2->getName(),
                   m2->getOwner()->getName());
  }

  if (m1 == top_module || m2 == top_module) {
    return top_module;
  }

  int d1 = getModuleDepth(m1);
  int d2 = getModuleDepth(m2);

  // Bring m1 and m2 to the same depth
  while (d1 > d2) {
    m1 = m1->getParentModule();
    d1--;
  }
  while (d2 > d1) {
    m2 = m2->getParentModule();
    d2--;
  }

  // Move up until they meet
  while (m1 != m2) {
    m1 = m1->getParentModule();
    m2 = m2->getParentModule();
  }
  return m1;
}

std::optional<bool> dbInsertBuffer::getCachedReusability(dbModNet* net) const
{
  auto it = is_target_only_cache_.find(net);
  if (it != is_target_only_cache_.end()) {
    return it->second;
  }
  return std::nullopt;
}

bool dbInsertBuffer::checkAllLoadsAreTargets(
    dbModNet* start_net,
    const std::set<dbObject*>& load_pins) const
{
  if (start_net == nullptr) {
    return true;
  }

  // Check cache first
  if (std::optional<bool> result = getCachedReusability(start_net)) {
    return *result;
  }

  std::set<dbModNet*> visited;

  std::function<bool(dbModNet*)> worker = [&](dbModNet* net) -> bool {
    if (net == nullptr) {
      return true;
    }

    // Check cache
    if (std::optional<bool> result = getCachedReusability(net)) {
      return *result;
    }

    // Cycle detection
    if (visited.find(net) != visited.end()) {
      return true;
    }
    visited.insert(net);

    bool result = true;

    // 1. Check ITerms
    for (dbITerm* iterm : net->getITerms()) {
      if (load_pins.find(iterm) == load_pins.end()) {
        result = false;
        break;
      }
    }

    if (result) {
      // 2. ModITerms (down)
      for (dbModITerm* miterm : net->getModITerms()) {
        if (dbModBTerm* child_bterm = miterm->getChildModBTerm()) {
          if (child_bterm->getIoType() == dbIoType::INPUT
              || child_bterm->getIoType() == dbIoType::INOUT) {
            if (dbModNet* child_net = child_bterm->getModNet()) {
              if (!worker(child_net)) {
                result = false;
                break;
              }
            }
          }
        }
      }
    }

    if (result) {
      // 3. Upward
      for (dbModBTerm* modbterm : net->getModBTerms()) {
        if (modbterm->getIoType() == dbIoType::OUTPUT
            || modbterm->getIoType() == dbIoType::INOUT) {
          if (dbModITerm* parent_moditerm = modbterm->getParentModITerm()) {
            if (dbModNet* parent_net = parent_moditerm->getModNet()) {
              if (!worker(parent_net)) {
                result = false;
                break;
              }
            }
          }
        }
      }
    }

    // Cache result
    markModNetReusability(net, result);
    return result;
  };

  return worker(start_net);
}

void dbInsertBuffer::populateReusableModNets(
    const std::set<dbObject*>& load_pins)
{
  // Algorithm:
  // 1. Iterate through each leaf-level load pin.
  // 2. Trace the hierarchy upwards from the load pin towards the target (LCA)
  //    module.
  // 3. At each hierarchical boundary, verify if the current dbModNet drives
  //    ONLY target loads.
  // 4. If isolated, cache the net as reusable; if not, stop tracing up for this
  //    path.

  // Iterate through all target load pins
  for (dbObject* load_obj : load_pins) {
    if (load_obj->getObjectType() != dbITermObj) {
      continue;
    }

    // Initialize tracing from the leaf terminal's module
    dbObject* trace_obj = load_obj;
    dbModule* trace_module
        = static_cast<dbITerm*>(trace_obj)->getInst()->getModule();

    // Climb the hierarchy until the target common ancestor module is reached
    while (trace_module != target_module_) {
      // Find the hierarchical net connected to the current terminal
      dbModNet* modnet = getModNet(trace_obj);

      if (modnet == nullptr) {
        break;
      }

      // Check if this hierarchical net is isolated to target loads
      if (checkAllLoadsAreTargets(modnet, load_pins)) {
        // Cache the net as reusable for this module
        module_reusable_nets_[trace_module].insert(modnet);
      } else {
        // If this net drives non-target loads, any parent net driving this
        // will also be "dirty", so we stop traversing up this path.
        break;
      }

      // Find the input port (BTerm) of the current module to continue moving up
      dbModBTerm* modbterm = nullptr;
      for (dbModBTerm* mbterm : modnet->getModBTerms()) {
        if (mbterm->getIoType() == dbIoType::INPUT) {
          modbterm = mbterm;
          break;
        }
      }

      if (modbterm == nullptr) {
        break;
      }

      // Retrieve the pin on the instance of this module in the parent module
      dbModITerm* parent_pin = modbterm->getParentModITerm();
      if (parent_pin == nullptr) {
        break;
      }

      // Update trace module and object to continue upward traversal
      // dbModITerm -> dbModInst -> parent dbModule
      trace_module = parent_pin->getParent()->getParent();
      trace_obj = parent_pin;
    }
  }
}

void dbInsertBuffer::advanceToParentModule(dbObject*& load_obj,
                                           dbModule*& current_module,
                                           dbModITerm*& top_mod_iterm,
                                           dbModITerm* next_mod_iterm)
{
  load_obj = static_cast<dbObject*>(next_mod_iterm);
  current_module = current_module->getModInst()->getParent();
  top_mod_iterm = next_mod_iterm;
}

bool dbInsertBuffer::tryReuseParentPath(dbObject*& load_obj,
                                        dbModule*& current_module,
                                        dbModITerm*& top_mod_iterm,
                                        const std::set<dbObject*>& load_pins)
{
  // Check if there's an existing hierarchical connection to reuse.
  dbModNet* existing_mod_net = getModNet(load_obj);

  if (!existing_mod_net) {
    return false;
  }

  if (checkAllLoadsAreTargets(existing_mod_net, load_pins)) {
    // Check if it goes up (connected to ModBTerm)
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

      // Add to cache BEFORE updating current_module
      module_reusable_nets_[current_module].insert(existing_mod_net);

      // Prepare for next iteration (moving up)
      advanceToParentModule(
          load_obj, current_module, top_mod_iterm, parent_iterm);
      return true;
    }
  }
  return false;
}

bool dbInsertBuffer::tryReuseModNetInModule(dbObject*& load_obj,
                                            dbModule*& current_module,
                                            dbModITerm*& top_mod_iterm)
{
  auto it = module_reusable_nets_.find(current_module);
  if (it == module_reusable_nets_.end()) {
    return false;
  }

  for (dbModNet* candidate_net : it->second) {
    // Check if this net has been tainted (marked as fan-in of the buffer)
    if (isMarkedAsNotReusable(candidate_net)) {
      continue;
    }

    dbModBTerm* modbterm = nullptr;
    for (dbModBTerm* bterm : candidate_net->getModBTerms()) {
      if (bterm->getIoType() == dbIoType::INPUT) {
        modbterm = bterm;
        break;
      }
    }

    if (modbterm) {
      dbModITerm* parent_moditerm = modbterm->getParentModITerm();
      if (parent_moditerm) {
        connect(load_obj, candidate_net);

        // Prepare for next iteration (moving up)
        advanceToParentModule(
            load_obj, current_module, top_mod_iterm, parent_moditerm);
        return true;
      }
    }
  }
  return false;
}

void dbInsertBuffer::createNewHierConnection(dbObject*& load_obj,
                                             dbModule*& current_module,
                                             dbModITerm*& top_mod_iterm,
                                             const std::string& base_name)
{
  // Name generation
  std::string unique_name = makeUniqueHierName(current_module, base_name);

  // Create Port (ModBTerm) on current module
  dbModBTerm* mod_bterm
      = dbModBTerm::create(current_module, unique_name.c_str());
  mod_bterm->setIoType(dbIoType::INPUT);

  // Create Net (ModNet) inside current module
  dbModNet* mod_net = dbModNet::create(current_module, unique_name.c_str());
  mod_bterm->connect(mod_net);

  // Register this new net as reusable for subsequent loads
  markModNetReusability(mod_net, true);
  module_reusable_nets_[current_module].insert(mod_net);

  // Connect lower level object (either leaf ITerm or previous ModITerm)
  connect(load_obj, mod_net);

  // Create Pin (ModITerm) on the instance of current module in the parent
  dbModInst* parent_mod_inst = current_module->getModInst();
  dbModITerm* mod_iterm
      = dbModITerm::create(parent_mod_inst, unique_name.c_str(), mod_bterm);

  // Prepare for next iteration (moving up)
  advanceToParentModule(load_obj, current_module, top_mod_iterm, mod_iterm);
}

void dbInsertBuffer::performFinalConnections(dbITerm* load_pin,
                                             dbITerm* drvr_term,
                                             dbModITerm* top_mod_iterm)
{
  // 4.1. Disconnect the load from the original net
  if (top_mod_iterm) {
    // If we established a hierarchical connection, we only want to disconnect
    // the flat net, preserving the ModNet connection we just made (or reused).
    load_pin->disconnectDbNet();
  } else {
    // If no hierarchical connection (same module), disconnect everything
    // as we will connect to a new ModNet (if applicable) in this module.
    load_pin->disconnect();
  }

  // 4.2. Connect load to the new ModNet
  // - IMPORTANT: dbSta prioritizes hier net connection over flat net
  //   connection. So hier net connection should be edited first.
  //   otherwise, STA caches may not be updated correctly.
  if (dbModNet* drvr_mod_net = drvr_term->getModNet()) {
    if (top_mod_iterm) {
      // Connect the top-most hierarchical pin to the buffer's output ModNet
      top_mod_iterm->disconnect();  // Disconnect both flat and hier nets
      top_mod_iterm->connect(drvr_mod_net);
    } else {
      // Load is in the same module as buffer, connect to ModNet if it exists
      load_pin->disconnect();
      load_pin->connect(drvr_mod_net);
    }
  }

  // 4.3. Connect load to the new flat net
  load_pin->connect(drvr_term->getNet());
}

bool dbInsertBuffer::stitchLoadToDriver(dbITerm* load_pin,
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
    dbNet* load_net = load_pin->getNet();
    std::string base_name = block_->getBaseName(
        load_net ? load_net->getConstName() : net_->getConstName());

    while (current_module != target_module && current_module != nullptr) {
      if (current_module->getModInst()
          == nullptr) {  // current_module is top module
        logger_->error(
            utl::ODB,
            1206,
            "Cannot create hierarchical connection: '{}' is not a descendant "
            "of '{}'.",
            load_pin->getInst()->getName(),
            target_module->getName());
        break;
      }

      // 1. Check if the current net is reusable (using cache)
      if (tryReuseParentPath(
              load_obj, current_module, top_mod_iterm, load_pins)) {
        continue;
      }

      // 2. Check if there is ANY other ModNet in this module that we can reuse.
      if (tryReuseModNetInModule(load_obj, current_module, top_mod_iterm)) {
        continue;
      }

      // 3. Create ports & modnet
      createNewHierConnection(
          load_obj, current_module, top_mod_iterm, base_name);
    }
  }

  // 4. Perform connections
  performFinalConnections(load_pin, drvr_term, top_mod_iterm);

  return top_mod_iterm != nullptr;
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
  dbModNet* curr_modnet = getModNet(driver_term);

  // 3. Traverse fanout from driver's modnet to find target module's modnet
  return curr_modnet->findInHierarchy(
      [&](dbModNet* modnet) {
        return modnets_in_target_module.contains(modnet);
      },
      dbHierSearchDir::FANOUT);
}

dbModNet* dbInsertBuffer::getModNet(const dbObject* obj) const
{
  if (obj->getObjectType() == dbITermObj) {
    return static_cast<const dbITerm*>(obj)->getModNet();
  }
  if (obj->getObjectType() == dbModITermObj) {
    return static_cast<const dbModITerm*>(obj)->getModNet();
  }
  if (obj->getObjectType() == dbBTermObj) {
    return static_cast<const dbBTerm*>(obj)->getModNet();
  }
  assert(false);
  return nullptr;
}

dbNet* dbInsertBuffer::getNet(const dbObject* obj) const
{
  if (obj->getObjectType() == dbITermObj) {
    return static_cast<const dbITerm*>(obj)->getNet();
  }
  if (obj->getObjectType() == dbBTermObj) {
    return static_cast<const dbBTerm*>(obj)->getNet();
  }
  return nullptr;
}

dbModule* dbInsertBuffer::getModule(const dbObject* obj) const
{
  if (obj->getObjectType() == dbITermObj) {
    return static_cast<const dbITerm*>(obj)->getInst()->getModule();
  }
  if (obj->getObjectType() == dbBTermObj) {
    return block_->getTopModule();
  }
  return nullptr;
}

std::string dbInsertBuffer::getName(const dbObject* obj) const
{
  if (obj->getObjectType() == dbITermObj) {
    return static_cast<const dbITerm*>(obj)->getMTerm()->getName();
  }
  if (obj->getObjectType() == dbBTermObj) {
    return static_cast<const dbBTerm*>(obj)->getName();
  }
  assert(false);
  return "";
}

void dbInsertBuffer::connect(dbObject* obj, dbNet* net)
{
  if (obj->getObjectType() == dbITermObj) {
    static_cast<dbITerm*>(obj)->connect(net);
  } else if (obj->getObjectType() == dbBTermObj) {
    static_cast<dbBTerm*>(obj)->connect(net);
  } else {
    assert(false);
  }
}

void dbInsertBuffer::connect(dbObject* obj, dbModNet* net)
{
  if (obj->getObjectType() == dbITermObj) {
    static_cast<dbITerm*>(obj)->connect(net);
  } else if (obj->getObjectType() == dbModITermObj) {
    static_cast<dbModITerm*>(obj)->connect(net);
  } else if (obj->getObjectType() == dbBTermObj) {
    static_cast<dbBTerm*>(obj)->connect(net);
  } else {
    assert(false);
  }
}

void dbInsertBuffer::ensureFlatNetConnection(dbObject* driver, dbObject* load)
{
  dbNet* driver_net = getNet(driver);
  dbNet* load_net = getNet(load);

  if (driver_net && load_net) {
    if (driver_net != load_net) {
      driver_net->mergeNet(load_net);
    }
  } else if (driver_net) {
    connect(load, driver_net);
  } else if (load_net) {
    connect(driver, load_net);
  } else {
    const char* base_name = "net";
    dbModNet* driver_modnet = getModNet(driver);
    dbModNet* load_modnet = getModNet(load);

    if (driver_modnet) {
      base_name = driver_modnet->getConstName();
    } else if (load_modnet) {
      base_name = load_modnet->getConstName();
    }

    dbNet* new_net = dbNet::create(block_, base_name, uniquify_);
    if (new_net) {
      connect(driver, new_net);
      connect(load, new_net);
    }
  }

  // Check if flat net name matches any of hierarchical net names
  dbNet* flat_net = getNet(driver);
  if (flat_net) {
    bool name_match = false;
    const char* flat_name = flat_net->getConstName();

    // Efficient check
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
      // Rename to the modnet name connected to the driver
      dbModNet* driver_mod_net = getModNet(driver);
      if (driver_mod_net) {
        flat_net->rename(driver_mod_net->getHierarchicalName().c_str());
      } else if (dbModNet* load_mod_net = getModNet(load)) {
        flat_net->rename(load_mod_net->getHierarchicalName().c_str());
      }
    }
  }
}

void dbInsertBuffer::connectSameModule(dbObject* driver,
                                       dbObject* load,
                                       dbModule* driver_mod)
{
  dbModNet* driver_net = getModNet(driver);
  dbModNet* load_net = getModNet(load);

  if (driver_net && load_net) {
    if (driver_net != load_net) {
      driver_net->mergeModNet(load_net);
    }
  } else if (driver_net) {
    connect(load, driver_net);
  } else if (load_net) {
    connect(driver, load_net);
  } else {
    const char* base_name = "net";
    dbNet* flat_driver_net = getNet(driver);
    dbNet* flat_load_net = getNet(load);

    if (flat_driver_net) {
      base_name = block_->getBaseName(flat_driver_net->getConstName());
    } else if (flat_load_net) {
      base_name = block_->getBaseName(flat_load_net->getConstName());
    }

    dbNet* flat_net = flat_driver_net ? flat_driver_net : flat_load_net;
    dbModNet* new_modnet
        = dbModNet::create(driver_mod, base_name, uniquify_, flat_net);
    connect(driver, new_modnet);
    connect(load, new_modnet);
  }
}

dbModNet* dbInsertBuffer::ensureModNet(dbObject* obj,
                                       dbModule* mod,
                                       dbNet* corresponding_flat_net,
                                       const char* suffix)
{
  dbModNet* mod_net = getModNet(obj);
  if (!mod_net) {
    mod_net = dbModNet::create(mod, "net", uniquify_, corresponding_flat_net);
    connect(obj, mod_net);
  }

  // Connect peer ITerms/BTerms on the same flat net in the same module scope.
  // This handles cases where obj is dbModITerm and we still need to connect
  // leaf ITerms to the modnet.
  connectPeerITerms(mod, mod_net, corresponding_flat_net);
  return mod_net;
}

void dbInsertBuffer::connectPeerITerms(dbModule* mod,
                                       dbModNet* mod_net,
                                       dbNet* corresponding_flat_net)
{
  if (corresponding_flat_net == nullptr) {
    return;
  }

  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             3,
             "connectPeerITerms: mod='{}' flat_net='{}' iterms={}",
             mod->getName(),
             corresponding_flat_net->getConstName(),
             corresponding_flat_net->getITerms().size());

  for (dbITerm* peer_iterm : corresponding_flat_net->getITerms()) {
    if (peer_iterm->getInst()->getModule() == mod) {
      if (peer_iterm->getModNet() == nullptr) {
        debugPrint(logger_,
                   utl::ODB,
                   "insert_buffer",
                   3,
                   "connectPeerITerms: connecting peer_iterm '{}'",
                   peer_iterm->getName());
        peer_iterm->connect(mod_net);
      }
    }
  }

  // Connect BTerms if this is the top module
  if (mod == block_->getTopModule()) {
    for (dbBTerm* peer_bterm : corresponding_flat_net->getBTerms()) {
      if (peer_bterm->getModNet() == nullptr) {
        peer_bterm->connect(mod_net);
      }
    }
  }
}

dbObject* dbInsertBuffer::traceUp(dbObject* current_obj,
                                  dbModule* current_mod,
                                  dbModule* target_mod,
                                  dbIoType io_type,
                                  const char* suffix,
                                  dbNet* corresponding_flat_net)
{
  while (current_mod != target_mod) {
    dbModNet* mod_net
        = ensureModNet(current_obj, current_mod, corresponding_flat_net);

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

      // To avoid dbModBTerm name collision
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
}

void dbInsertBuffer::connectDifferentModule(dbObject* driver,
                                            dbObject* load,
                                            dbModule* driver_mod,
                                            dbModule* load_mod)
{
  // Find the LCA of the driver and load modules.
  dbModule* lca = findLCA(driver_mod, load_mod);

  dbNet* driver_net = getNet(driver);
  dbNet* load_net = getNet(load);
  dbNet* corresponding_flat_net = (driver_net) ? driver_net : load_net;

  // Trace UP from driver to LCA
  dbObject* driver_lca_obj = traceUp(
      driver, driver_mod, lca, dbIoType::OUTPUT, "out", corresponding_flat_net);

  // Trace UP from load to LCA
  dbObject* load_lca_obj = traceUp(
      load, load_mod, lca, dbIoType::INPUT, "in", corresponding_flat_net);

  // Connect at LCA
  dbModNet* driver_modnet = getModNet(driver_lca_obj);
  dbModNet* load_modnet = getModNet(load_lca_obj);
  dbModNet* lca_modnet = nullptr;

  if (driver_modnet) {
    lca_modnet = driver_modnet;
  } else if (load_modnet) {
    lca_modnet = load_modnet;
  } else {
    dbNet* flat_net = getNet(driver);
    if (flat_net == nullptr) {
      flat_net = getNet(load);
    }
    lca_modnet = dbModNet::create(lca, "net", uniquify_, flat_net);
  }

  connect(driver_lca_obj, lca_modnet);
  connect(load_lca_obj, lca_modnet);

  connectPeerITerms(lca, lca_modnet, corresponding_flat_net);
}

void dbInsertBuffer::hierarchicalConnect(dbObject* driver, dbObject* load)
{
  dbModule* driver_mod = getModule(driver);
  dbModule* load_mod = getModule(load);

  if (driver_mod == nullptr || load_mod == nullptr) {
    logger_->error(
        utl::ODB, 1210, "hierarchicalConnect: Invalid driver or load object.");
    return;
  }

  // Simple case (driver and load are in the same module)
  // - Connect the modnet.
  if (driver_mod == load_mod) {
    connectSameModule(driver, load, driver_mod);
  } else {
    // Complex case (driver and load are in different modules)
    // - Find the LCA of the driver and load modules.
    // - Connect the modnet of the driver to the modnet of the load.
    connectDifferentModule(driver, load, driver_mod, load_mod);
  }

  ensureFlatNetConnection(driver, load);
}

dbModule* dbInsertBuffer::validateLoadPinsAndFindLCA(
    const std::set<dbObject*>& load_pins,
    bool loads_on_diff_nets) const
{
  std::set<dbNet*> other_dbnets;
  dbModule* target_module = nullptr;
  bool first = true;

  // Check dont_touch flag of the target net
  if (net_->isDoNotTouch()) {
    logger_->warn(utl::ODB,
                  1202,
                  "InsertBufferBeforeLoads: Target net '{}' is dont_touch. "
                  "Cannot insert a buffer.",
                  net_->getName());
    return nullptr;
  }

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

      // dont_touch check for term and net
      if (checkDontTouch(iterm)) {
        logger_->warn(utl::ODB,
                      1211,
                      "InsertBufferBeforeLoads: Load pin '{}' is dont_touch. "
                      "Cannot insert a buffer.",
                      load_name);
        return nullptr;
      }
    } else if (load_obj->getObjectType() == dbBTermObj) {
      dbBTerm* bterm = static_cast<dbBTerm*>(load_obj);
      load_net = bterm->getNet();
      load_name = bterm->getName();
      curr_mod = block_->getTopModule();

      // dont_touch check for net
      if (load_net && load_net->isDoNotTouch()) {
        logger_->warn(utl::ODB,
                      1201,
                      "InsertBufferBeforeLoads: Load pin '{}' is dont_touch. "
                      "Cannot insert a buffer.",
                      load_name);
        return nullptr;
      }
    } else {
      logger_->error(
          utl::ODB,
          1204,
          "InsertBufferBeforeLoads: Load object ({}) is not an ITerm or BTerm.",
          load_obj->getTypeName());
      return nullptr;
    }

    // 2. Common logic: connectivity check
    if (load_net != net_) {
      if (load_net) {
        other_dbnets.insert(load_net);
      }
      dlogDifferentDbNet(load_net);

      if (!loads_on_diff_nets) {
        logger_->error(utl::ODB,
                       1200,
                       "InsertBufferBeforeLoads: Load pin '{}' is "
                       "not connected to net '{}'.",
                       load_name,
                       net_->getName());
        return nullptr;
      }
    }

    // 3. Find LCA (Lowest Common Ancestor)
    if (first) {
      target_module = curr_mod;
      first = false;
    } else {
      target_module = findLCA(target_module, curr_mod);
    }
  }

  dlogLCAModule(target_module);
  dlogDumpNets(other_dbnets);

  return target_module;
}

void dbInsertBuffer::createNewFlatAndHierNets(
    const std::set<dbObject*>& load_pins)
{
  // Create a new flat net
  std::set<dbObject*> connected_terms;
  connected_terms.insert(load_pins.begin(), load_pins.end());
  new_flat_net_ = createNewFlatNet(connected_terms);

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
    new_mod_net_ = dbModNet::create(target_module_,
                                    base_name,
                                    dbNameUniquifyType::IF_NEEDED,
                                    new_flat_net_);
  }
}

void dbInsertBuffer::rewireBufferLoadPins(const std::set<dbObject*>& load_pins)
{
  // 1.1. Connect Buffer Input to the Original Net
  buf_input_iterm_->connect(net_);
  dlogConnectedBufferInputFlat();

  // 1.2. Also connect to ModNet
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
      // 1.2.1. Connect to the existing modnet
      buf_input_iterm_->connect(orig_mod_net);
      dlogConnectedBufferInputHier(orig_mod_net);
    } else {
      // 1.2.2. Connect to a new modnet if driver & buf hiers are different
      dbObject* drvr = net_->getFirstDriverTerm();
      if (drvr) {
        dbModule* drvr_mod = nullptr;
        if (drvr->getObjectType() == dbITermObj) {
          drvr_mod = static_cast<dbITerm*>(drvr)->getInst()->getModule();
        } else if (drvr->getObjectType() == dbBTermObj) {
          drvr_mod = block_->getTopModule();
        }

        if (drvr_mod != target_module_) {
          hierarchicalConnect(drvr, buf_input_iterm_);
          dlogConnectedBufferInputViaHierConnect(drvr);
        }
      }
    }

    // Mark all nets in the fanin cone of the buffer input as non-reusable
    // to prevent creating loops when connecting the buffer output.
    if (dbModNet* input_mod_net = buf_input_iterm_->getModNet()) {
      markFaninModNetsNotReusable(input_mod_net);
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
      stitchLoadToDriver(load, buf_output_iterm_, load_pins);
    } else {
      assert(load_obj->getObjectType() == dbBTermObj);
      assert(target_module_ == block_->getTopModule());
      dbBTerm* load = static_cast<dbBTerm*>(load_obj);
      load->disconnect();  // Disconnect both flat and hier nets
      load->connect(new_flat_net_, new_mod_net_);
      dlogMovedBTermLoad(load_idx, num_loads, load);
    }
    load_idx++;
  }
}

bool dbInsertBuffer::isMarkedAsNotReusable(dbModNet* net) const
{
  std::optional<bool> result = getCachedReusability(net);
  return result && *result == false;
}

void dbInsertBuffer::markModNetReusability(dbModNet* net,
                                           bool is_reusable) const
{
  is_target_only_cache_[net] = is_reusable;
}

void dbInsertBuffer::checkSanity() const
{
  if (logger_->debugCheck(utl::ODB, "insert_buffer_check_sanity", 1)) {
    net_->checkSanity();
    new_flat_net_->checkSanity();
  }
}

void dbInsertBuffer::markFaninModNetsNotReusable(dbModNet* net)
{
  if (!net) {
    return;
  }

  // If already marked as false, stop (already visited).
  if (isMarkedAsNotReusable(net)) {
    return;
  }
  markModNetReusability(net, false);

  // 1. Child Instances (Down hierarchy, but Upstream signal flow)
  // Look for Child OUTPUT ports driving this net.
  for (dbModITerm* miterm : net->getModITerms()) {
    dbModBTerm* child_bterm = miterm->getChildModBTerm();
    if (child_bterm
        && (child_bterm->getIoType() == dbIoType::OUTPUT
            || child_bterm->getIoType() == dbIoType::INOUT)) {
      if (dbModNet* child_net = child_bterm->getModNet()) {
        markFaninModNetsNotReusable(child_net);
      }
    }
  }

  // 2. Parent Module (Up hierarchy, but Upstream signal flow)
  // Look for Parent INPUT ports driving this net.
  for (dbModBTerm* mbterm : net->getModBTerms()) {
    if (mbterm->getIoType() == dbIoType::INPUT
        || mbterm->getIoType() == dbIoType::INOUT) {
      if (dbModITerm* parent_miterm = mbterm->getParentModITerm()) {
        if (dbModNet* parent_net = parent_miterm->getModNet()) {
          markFaninModNetsNotReusable(parent_net);
        }
      }
    }
  }
}

void dbInsertBuffer::placeBufferAtCentroid(dbInst* buffer_inst,
                                           const dbObject* drvr_pin,
                                           const std::set<dbObject*>& load_pins)
{
  Point placement_loc;
  if (computeCentroid(drvr_pin, load_pins, placement_loc)) {
    placeBufferAtLocation(buffer_inst, placement_loc, "centroid");
  } else {
    buffer_inst->setPlacementStatus(dbPlacementStatus::UNPLACED);
    dlogUnplacedBuffer(buffer_inst,
                       "centroid computation failed (no placed pins)");
  }
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
                                           bool loads_on_diff_nets) const
{
  debugPrint(
      logger_,
      utl::ODB,
      "insert_buffer",
      1,
      "BeforeLoads: Try inserting a buffer on dbNet={}, load_pins_size={}, "
      "buffer_master={}, loc=({}, {}), new_buf_base_name={}, "
      "new_net_base_name={}, uniquify={}, loads_on_diff_nets={}",
      net_->getName(),
      load_pins.size(),
      buffer_master_->getName(),
      loc ? loc->getX() : -1,
      loc ? loc->getY() : -1,
      new_buf_base_name_,
      new_net_base_name_,
      static_cast<int>(uniquify_),
      loads_on_diff_nets);
}

void dbInsertBuffer::dlogTargetLoadPin(const dbObject* load_obj) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads:   target load pin: {}",
             load_obj->getName());
}

void dbInsertBuffer::dlogDifferentDbNet(const dbNet* net) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads:   * is on different dbNet '{}'",
             net ? net->getName() : "<null>");
}

void dbInsertBuffer::dlogLCAModule(const dbModule* target_module) const
{
  if (logger_->debugCheck(utl::ODB, "insert_buffer", 1)) {
    dbModInst* target_mod_inst
        = target_module ? target_module->getModInst() : nullptr;
    debugPrint(logger_,
               utl::ODB,
               "insert_buffer",
               1,
               "BeforeLoads: LCA module: {} '{}'",
               target_module ? target_module->getName() : "null_module",
               target_mod_inst ? target_mod_inst->getHierarchicalName()
                               : "<top_or_null_mod_inst>");
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

void dbInsertBuffer::dlogConnectedBufferInputHier(
    const dbModNet* orig_mod_net) const
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
    const dbObject* drvr_term) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Connected buffer input '{}' to "
             "original hierarchical net '{}' via hierarchicalConnect",
             buf_input_iterm_->getName(),
             drvr_term->getName());
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
                                          const dbITerm* load) const
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
                                        const dbBTerm* load) const
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

void dbInsertBuffer::dlogPlacedBuffer(const dbInst* buffer_inst,
                                      const Point& loc,
                                      const char* reason) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "Placed the new buffer '{}' at ({}, {}) using {}",
             buffer_inst->getName(),
             loc.getX(),
             loc.getY(),
             reason);
}

void dbInsertBuffer::dlogUnplacedBuffer(const dbInst* buffer_inst,
                                        const char* reason) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "Buffer '{}' set to UNPLACED: {}",
             buffer_inst->getName(),
             reason);
}

void dbInsertBuffer::dlogInsertBufferSuccess(const dbInst* buffer_inst) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "BeforeLoads: Successfully inserted a new buffer '{}'",
             buffer_inst->getName());
}

void dbInsertBuffer::validateArgumentsSimple(
    const dbObject* term_obj,
    const dbMaster* buffer_master) const
{
  if (term_obj == nullptr) {
    logger_->error(
        utl::ODB, 1213, "insertBufferSimple: term_obj must not be null.");
  }

  if (buffer_master == nullptr) {
    logger_->error(
        utl::ODB, 1214, "insertBufferSimple: buffer_master must not be null.");
  }
}

void dbInsertBuffer::validateArgumentsBeforeLoads(
    const std::set<dbObject*>& load_pins,
    const dbMaster* buffer_master) const
{
  if (load_pins.empty()) {
    logger_->error(utl::ODB,
                   1215,
                   "insertBufferBeforeLoads: load_pins must not be empty.");
  }

  if (buffer_master == nullptr) {
    logger_->error(utl::ODB,
                   1216,
                   "insertBufferBeforeLoads: buffer_master must not be null.");
  }
}

void dbInsertBuffer::dlogSeparator() const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "-------------------------------------------------------");
}

void dbInsertBuffer::dlogInsertBufferStart(int count, const char* mode) const
{
  debugPrint(logger_,
             utl::ODB,
             "insert_buffer",
             1,
             "insert_buffer {}#{}",
             mode,
             count);
}

}  // namespace odb
