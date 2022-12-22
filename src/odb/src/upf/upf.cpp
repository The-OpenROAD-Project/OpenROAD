/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#include "odb/upf.h"

#include <tcl.h>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "utl/Logger.h"

namespace upf {

bool create_power_domain(utl::Logger* logger,
                         odb::dbBlock* block,
                         const char* name)
{
  if (odb::dbPowerDomain::create(block, name) == nullptr) {
    logger->warn(utl::ODB, 10001, "Creation of '%s' power domain failed", name);
    return false;
  }

  return true;
}

bool update_power_domain(utl::Logger* logger,
                         odb::dbBlock* block,
                         const char* name,
                         const char* elements)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(name);
  if (pd != nullptr) {
    pd->addElement(std::string(elements));
  } else {
    logger->warn(
        utl::ODB,
        10002,
        "Couldn't retrieve power domain '%s' while adding element '%s'",
        name,
        elements);
    return false;
  }
  return true;
}

bool create_logic_port(utl::Logger* logger,
                       odb::dbBlock* block,
                       const char* name,
                       const char* direction)
{
  if (odb::dbLogicPort::create(block, name, std::string(direction))
      == nullptr) {
    logger->warn(utl::ODB, 10003, "Creation of '%s' logic port failed", name);
    return false;
  }
  return true;
}

bool create_power_switch(utl::Logger* logger,
                         odb::dbBlock* block,
                         const char* name,
                         const char* power_domain,
                         const char* out_port,
                         const char* in_port)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(power_domain);
  if (pd == nullptr) {
    logger->warn(
        utl::ODB,
        10004,
        "Couldn't retrieve power domain '%s' while creating power switch '%s'",
        power_domain,
        name);
    return false;
  }

  odb::dbPowerSwitch* ps = odb::dbPowerSwitch::create(block, name);
  if (ps == nullptr) {
    logger->warn(utl::ODB, 10005, "Creation of '%s' power switch failed", name);
    return false;
  }

  ps->setInSupplyPort(std::string(in_port));
  ps->setOutSupplyPort(std::string(out_port));
  ps->setPowerDomain(pd);
  pd->addPowerSwitch(ps);
  return true;
}

bool update_power_switch_control(utl::Logger* logger,
                                 odb::dbBlock* block,
                                 const char* name,
                                 const char* control_port)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name);
  if (ps == nullptr) {
    logger->warn(
        utl::ODB,
        10006,
        "Couldn't retrieve power switch '%s' while adding control port '%s'",
        name,
        control_port);
    return false;
  }

  ps->addControlPort(std::string(control_port));
  return true;
}

bool update_power_switch_on(utl::Logger* logger,
                            odb::dbBlock* block,
                            const char* name,
                            const char* on_state)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name);
  if (ps == nullptr) {
    logger->warn(
        utl::ODB,
        10007,
        "Couldn't retrieve power switch '%s' while adding on state '%s'",
        name,
        on_state);
    return false;
  }
  ps->addOnState(on_state);
  return true;
}

bool set_isolation(utl::Logger* logger,
                   odb::dbBlock* block,
                   const char* name,
                   const char* power_domain,
                   bool update,
                   const char* applies_to,
                   const char* clamp_value,
                   const char* signal,
                   const char* sense,
                   const char* location)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(power_domain);
  if (pd == nullptr) {
    logger->warn(utl::ODB,
                 10008,
                 "Couldn't retrieve power domain '%s' while creating/updating "
                 "isolation '%s'",
                 power_domain,
                 name);
    return false;
  }

  odb::dbIsolation* iso = block->findIsolation(name);
  if (iso == nullptr && update) {
    logger->warn(
        utl::ODB, 10009, "Couldn't update a non existing isolation %s", name);
    return false;
  }

  if (iso == nullptr) {
    iso = odb::dbIsolation::create(block, name);
  }

  if (!update) {
    iso->setPowerDomain(pd);
    pd->addIsolation(iso);
  }

  if (strlen(applies_to) > 0) {
    iso->setAppliesTo(applies_to);
  }

  if (strlen(clamp_value) > 0) {
    iso->setClampValue(clamp_value);
  }

  if (strlen(signal) > 0) {
    iso->setIsolationSignal(signal);
  }

  if (strlen(sense) > 0) {
    iso->setIsolationSense(sense);
  }

  if (strlen(location) > 0) {
    iso->setLocation(location);
  }

  return true;
}

bool use_interface_cell(utl::Logger* logger,
                        odb::dbBlock* block,
                        const char* power_domain,
                        const char* strategy,
                        const char* cell)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(power_domain);
  if (pd == nullptr) {
    logger->warn(utl::ODB,
                 11008,
                 "Couldn't retrieve power domain '%s' while updating "
                 "isolation '%s'",
                 power_domain,
                 strategy);
    return false;
  }

  odb::dbIsolation* iso = block->findIsolation(strategy);
  if (iso == nullptr) {
    logger->warn(
        utl::ODB, 11009, "Couldn't find a non existing isolation %s", strategy);
    return false;
  }

  std::string _cell(cell);
  iso->addIsolationCell(_cell);

  return true;
}

bool set_domain_area(utl::Logger* logger,
                     odb::dbBlock* block,
                     char* domain,
                     float x1,
                     float y1,
                     float x2,
                     float y2)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(domain);
  if (pd == nullptr) {
    logger->warn(utl::ODB,
                 10080,
                 "Couldn't retrieve power domain '%s' while updating its area ",
                 domain);
    return false;
  }

  pd->setArea(x1, y1, x2, y2);

  return true;
}

bool findLongestPrefix(
    utl::Logger* logger,
    odb::dbBlock* block,
    std::map<std::string, std::vector<odb::dbPowerDomain*>>& module_to_domain,
    std::map<std::string, std::vector<odb::dbPowerDomain*>>& path_to_domain,
    std::string& currentPath)
{
  /*

  path_to_domain is as follows:
  * -> PD_TOP
  aes_0 -> pd_1
  aes_1 -> pd_2


  we have the following modules
  aes_0 -> matches with pd_1
  aes_0/aa -> matches with pd_1
  aes_0/aa/a1 -> matches with pd_1
  aes_1  -> matches with pd_2
  aes_1/aa  -> matches with pd_2
  aes_1/aa/a1 -> matches with pd_2
  aes_2 -> matches with PD_TOP
  aes_2/aa -> matches with PD_TOP
  aes_2/aa/a1 -> matches with PD_TOP
  */

  std::string longestPrefix_str = "";
  int longestPrefix_int = 0;

  for (auto const& path : path_to_domain) {
    std::string name = path.first;
    if (currentPath.find(name) == 0 && name.length() > longestPrefix_int) {
      longestPrefix_int = name.length();
      longestPrefix_str = name;
    }
  }

  if (longestPrefix_int == 0) {
    // No specified paths match this current module path
    // Check if there's a wild card and assign it
    if (path_to_domain.find(".") != path_to_domain.end()) {
      module_to_domain[currentPath] = path_to_domain["."];
    }
  } else {
    module_to_domain[currentPath] = path_to_domain[longestPrefix_str];
  }

  return true;
}
bool populate_lookup(
    utl::Logger* logger,
    odb::dbBlock* block,
    std::map<std::string, std::vector<odb::dbPowerDomain*>>& module_to_domain,
    std::map<std::string, std::vector<odb::dbPowerDomain*>>& path_to_domain,
    odb::dbModule* current)
{
  auto child_modules = current->getChildren();
  for (auto&& child_mod_inst : child_modules) {
    std::string name = child_mod_inst->getHierarchicalName();
    findLongestPrefix(logger, block, module_to_domain, path_to_domain, name);
    odb::dbModule* child_mod = child_mod_inst->getMaster();
    populate_lookup(logger, block, module_to_domain, path_to_domain, child_mod);
  }

  return true;
}

// sense is 0 if low and 1 if high
// clamp_val is 0 for low and 1 for high
// Returns -1 if can't find enable port
// Returns 0 if an inverter is not needed
// Returns 1 if an inverter is needed in control signal
// Returns 2 if an inverter is needed in output
// Returns 3 if an inverter is needed in both
int check_isolation_match(sta::FuncExpr* func,
                          sta::LibertyPort* enable,
                          bool sense,
                          bool clamp_val)
{
  bool enable_is_left = (func->left() && func->left()->hasPort(enable));
  bool enable_is_right = (func->right() && func->right()->hasPort(enable));
  if (!enable_is_left && !enable_is_right)
    return -1;
  sta::FuncExpr* enable_func = (enable_is_left) ? func->left() : func->right();
  bool enable_is_inverted
      = (enable_func->op() == sta::FuncExpr::Operator::op_not);
  bool new_enable_sense = (enable_is_inverted) ? !sense : sense;

  bool control_inv_needed = false;
  bool output_inv_needed = false;

  switch (func->op()) {
    case sta::FuncExpr::Operator::op_or:
      output_inv_needed = !clamp_val;
      control_inv_needed = !new_enable_sense;
      break;
    case sta::FuncExpr::Operator::op_and:
      output_inv_needed = clamp_val;
      control_inv_needed = new_enable_sense;
      break;
  }

  if (!control_inv_needed && !output_inv_needed)
    return 0;
  if (control_inv_needed && output_inv_needed)
    return 3;
  if (control_inv_needed)
    return 1;
  return 2;
}

bool associate_groups(
    utl::Logger* logger,
    odb::dbBlock* block,
    std::map<std::string, std::vector<odb::dbPowerDomain*>>& path_to_domain,
    odb::dbPowerDomain*& top_domain)
{
  auto pds = block->getPowerDomains();
  const int dbu = block->getDb()->getTech()->getLefUnits();

  for (auto&& domain : pds) {
    bool is_top_domain = false;
    auto els = domain->getElements();

    for (auto&& el :
         els) {  // Handling the case where there are multiple
                 // domains pointing to the same path, which shouldn't occur!!
      if (path_to_domain.find(el) == path_to_domain.end()) {
        std::vector<odb::dbPowerDomain*> dms = {domain};
        path_to_domain[el] = dms;
      } else {
        path_to_domain[el].push_back(domain);
      }
      if (el == ".") {
        top_domain = domain;
        is_top_domain = true;
      }
    }

    if (is_top_domain) {
      continue;
    }

    // Create region + group for this domain
    auto region = odb::dbRegion::create(block, domain->getName());
    if (!region) {
      logger->warn(
          utl::ODB, 10011, "Creation of '%s' region failed", domain->getName());
      return false;
    }

    // Specifying region area
    float _x1, _x2, _y1, _y2;
    if (domain->getArea(_x1, _y1, _x2, _y2)) {
      odb::dbBox::create(region,
                         std::round(_x1 * dbu),
                         std::round(_y1 * dbu),
                         std::round(_x2 * dbu),
                         std::round(_y2 * dbu));
    } else {
      logger->warn(utl::ODB,
                   120011,
                   "No area specified for '%s' power domain",
                   domain->getName());
    }

    auto group = odb::dbGroup::create(region, domain->getName());
    if (!group) {
      logger->warn(
          utl::ODB, 10012, "Creation of '%s' group failed", domain->getName());
      return false;
    }
    group->setType(odb::dbGroupType::POWER_DOMAIN);
    domain->setGroup(group);
  }

  return true;
}

bool instantiate_logic_ports(utl::Logger* logger, odb::dbBlock* block)
{
  auto lps = block->getLogicPorts();
  for (auto&& port : lps) {
    if (!odb::dbNet::create(block, port->getName())) {
      logger->warn(utl::ODB,
                   10010,
                   "Creation of '%s' dbNet from UPF Logic Port failed",
                   port->getName());
      return false;
    }
  }

  return true;
}

bool build_domain_hierarchy(utl::Logger* logger,
                            odb::dbBlock* block,
                            odb::dbPowerDomain* top_domain,
                            odb::dbSet<odb::dbPowerDomain>& pds)
{
  // Currently only support TOP DOMAIN + children domains
  // TODO: NEXT: Support nested domains
  for (auto&& domain : pds) {
    if (domain != top_domain) {
      domain->setParent(top_domain);
    }
  }

  return true;
}

bool add_insts_to_group(
    utl::Logger* logger,
    odb::dbBlock* block,
    odb::dbPowerDomain* top_domain,
    std::map<std::string, std::vector<odb::dbPowerDomain*>>& path_to_domain)
{
  // Create a reverse lookup table
  // Module Path (Including empty i.e top module) -> Power Domain
  // If nothing matches then there's no power domain matching
  std::map<std::string, std::vector<odb::dbPowerDomain*>> module_to_domain;
  odb::dbModule* top_module = block->getTopModule();

  if (path_to_domain.find(".") != path_to_domain.end()) {
    module_to_domain[""] = path_to_domain["."];
  }
  populate_lookup(logger, block, module_to_domain, path_to_domain, top_module);

  // For each dbInst, add it to the dbGroup matching its modules powerdomain
  auto all_instances = block->getInsts();
  for (auto&& inst : all_instances) {
    bool found = false;
    std::string path = "";
    auto mod = inst->getModule();
    if (!mod)
      continue;
    if (mod == block->getTopModule()) {
      found = (module_to_domain.find("") != module_to_domain.end());
    } else {
      auto modInst = mod->getModInst();
      path = modInst->getHierarchicalName();
      found = (module_to_domain.find(path) != module_to_domain.end());
    }

    if (found) {
      for (auto&& domain : module_to_domain[path]) {
        if (domain != top_domain) {  // if it belongs to top domain, no group
                                     // exists for this instance
          domain->getGroup()->addInst(inst);
        }
      }
    }
  }

  return true;
}

bool find_smallest_inverter(utl::Logger* logger,
                            odb::dbBlock* block,
                            odb::dbMaster*& inverter_m,
                            odb::dbMTerm*& input_m,
                            odb::dbMTerm*& output_m)
{
  auto network_ = ord::OpenRoad::openRoad()->getDbNetwork();
  float smallest_area = MAXFLOAT;
  sta::LibertyCell* smallest_inverter = nullptr;
  sta::LibertyPort *inverter_input = nullptr, *inverter_output = nullptr;
  sta::LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();

  bool found = false;

  auto libs = block->getDataBase()->getLibs();
  for (auto&& lib : libs) {
    auto masters = lib->getMasters();
    for (auto&& master : masters) {
      auto master_cell_ = network_->dbToSta(master);
      auto libcell_ = network_->libertyCell(master_cell_);

      if (libcell_ && libcell_->isInverter()
          && libcell_->area() < smallest_area) {
        smallest_area = libcell_->area();
        smallest_inverter = libcell_;
        smallest_inverter->bufferPorts(inverter_input, inverter_output);
        inverter_m = network_->staToDb(smallest_inverter);
        input_m = network_->staToDb(inverter_input);
        output_m = network_->staToDb(inverter_output);
        found = true;
      }
    }
  }

  return found;
}

bool eval_upf(utl::Logger* logger, odb::dbBlock* block)
{
  // TODO: NEXT: Lock any further UPF reads
  // Remove all regions and recreate everything

  // Create dbNet for each logic port defined in UPF
  instantiate_logic_ports(logger, block);

  auto pds = block->getPowerDomains();
  if (pds.size() == 0) {  // No power domains defined
    return true;
  }

  // lookup table 'defined path' -> 'power domain'
  std::map<std::string, std::vector<odb::dbPowerDomain*>> path_to_domain;
  odb::dbPowerDomain* top_domain = nullptr;

  // For each power domain
  // 1. Create a dbGroup associated to it
  // 2. Create a dbRegion associated to it
  // 3. map defined path to the power domain
  if (associate_groups(logger, block, path_to_domain, top_domain) == false) {
    return false;
  }

  if (top_domain == nullptr) {
    // A TOP DOMAIN should always exist
    logger->warn(utl::ODB, 10022, "No TOP DOMAIN found, aborting");
    return false;
  }

  // Creating hierarchy of power domains
  build_domain_hierarchy(logger, block, top_domain, pds);

  // Associate each instance with its power domain group
  add_insts_to_group(logger, block, top_domain, path_to_domain);

  // PowerDomains have a hierarchy and when the isolation's
  // location is parent, it should be placed in the parent's power domain
  // For each domain, determine for each net, if there's an input to another
  // domain, create two nets that split 'data' into 'data_i' + 'data_o' 'data_i'
  // goes into the isolation cell from the original cell 'data_o' got outside of
  // the isolation cell into the end cell then connect isolation signal to the
  // enable (or reversed depending on iso sense + cell type) place the isolation
  // cell into either first or second domain?? (Not sure exactly)

  auto network_ = ord::OpenRoad::openRoad()->getDbNetwork();
  for (auto&& domain : pds) {
    if (domain == top_domain)
      continue;

    // For now we're only using the first isolation
    // TODO: determine what needs to be done in case of multiple strategies
    // for same domain
    auto isos = domain->getIsolations();

    if (isos.size() < 1)
      continue;

    odb::dbIsolation* iso = isos[0];
    bool isolation_sense = (iso->getIsolationSense() == "high");
    bool isolation_clamp_val = (iso->getClampValue() == "1");
    auto iso_cells = iso->getIsolationCells();
    if (iso_cells.size() < 1) {
      logger->warn(utl::ODB,
                   13022,
                   "Isolation %s defined, but no cells defined.",
                   iso->getName());
      continue;
    }

    // For now using the first defined isolaton cell
    odb::dbMaster* smallest_iso_m = nullptr;
    sta::LibertyCell* smallest_iso_l = nullptr;
    float smallest_area = MAXFLOAT;

    for (auto&& iso : iso_cells) {
      sta::Cell* masterCell = network_->dbToSta(iso);
      sta::LibertyCell* libertyCell = network_->libertyCell(masterCell);

      if (libertyCell->area() < smallest_area) {
        smallest_iso_l = libertyCell;
        smallest_iso_m = iso;
        smallest_area = libertyCell->area();
      }
    }

    if (smallest_iso_m == nullptr) {
      logger->warn(utl::ODB,
                   130122,
                   "Isolation %s cells defined, but can't find any in the lib.",
                   iso->getName());
      continue;
    }

    auto cell_terms = smallest_iso_m->getMTerms();

    // Find enable & data pins for the isolation cell
    odb::dbMTerm* enable_term = nullptr;
    odb::dbMTerm* data_term = nullptr;
    odb::dbMTerm* output_term = nullptr;
    sta::LibertyPort* out_lib_port = nullptr;
    sta::LibertyPort* enable_lib_port = nullptr;
    sta::LibertyPort* data_lib_port = nullptr;
    for (auto&& term : cell_terms) {
      sta::LibertyPort* lib_port
          = smallest_iso_l->findLibertyPort(term->getName().c_str());

      if (!lib_port) {
        continue;
      }

      if (lib_port->isolationCellData()) {
        data_term = term;
        data_lib_port = lib_port;
      }

      if (lib_port->isolationCellEnable()) {
        enable_term = term;
        enable_lib_port = lib_port;
      }

      if (term->getIoType() == odb::dbIoType::OUTPUT) {
        output_term = term;
        out_lib_port = lib_port;
      }
    }

    if (!output_term || !data_term || !enable_term) {
      logger->warn(utl::ODB,
                   130152,
                   "Isolation %s cells defined, but can't find one of output, "
                   "data or enable terms.",
                   iso->getName());
      continue;
    }

    // Determine if an inverter is needed for the control pin & output
    auto func = out_lib_port->function();
    int inv_needed = check_isolation_match(
        func, enable_lib_port, isolation_sense, isolation_clamp_val);

    // find the smallest possible inverter
    odb::dbMaster* inverter_m = nullptr;
    odb::dbMTerm *input_m = nullptr, *output_m = nullptr;

    if (inv_needed > 0
        && find_smallest_inverter(logger, block, inverter_m, input_m, output_m)
               == false) {
      logger->warn(utl::ODB, 130482, "can't find any inverters");
      continue;
    }

    // Iterate through all pd's instances and determine if any signal needs
    // isolation If one is found, create dbInst from the isolation cell +
    // connect control signal of isolation to dbInst
    // connect previous net to data input
    // connect output from dbInst to

    odb::dbGroup* curr_group = domain->getGroup();
    auto insts = curr_group->getInsts();

    for (auto&& inst : insts) {
      auto iterms = inst->getITerms();
      for (auto&& iterm : iterms) {
        if (iterm->getIoType() != odb::dbIoType::OUTPUT)
          continue;

        auto net = iterm->getNet();

        if (!net) {
          continue;
        }

        auto connectedIterms = net->getITerms();

        if (connectedIterms.size() < 2)
          continue;

        // Find ITERMS that belong to instances outside of this power domain
        std::vector<odb::dbITerm*> external_iterms;
        for (auto&& connectedIterm : connectedIterms) {
          auto connectedInst = connectedIterm->getInst();

          if (!connectedInst->getGroup()
              || connectedInst->getGroup()->getName()
                     != curr_group->getName()) {
            external_iterms.push_back(connectedIterm);
          }
        }

        if (external_iterms.size() < 1)
          continue;
        // Create isolaton cell if there exists connected instances outside of
        // the power domain Add it to its specified location
        std::string inst_name = inst->getName() + "_isolation_cell";
        auto isolation_inst
            = odb::dbInst::create(block, smallest_iso_m, inst_name.c_str());

        if (iso->getLocation() == "self") {
          curr_group->addInst(isolation_inst);
        } else if (iso->getLocation() == "parent") {
          auto ppd = domain->getParent();
          if (ppd && ppd->getGroup()) {  // if the parent domain is the top
                                         // domain, don't add to any group
            ppd->getGroup()->addInst(isolation_inst);
          }
        } else {
          logger->warn(utl::ODB,
                       15022,
                       "Isolation %s has location %s, but only self|parent "
                       "supported, defaulting to self.",
                       iso->getName(),
                       iso->getLocation());
          curr_group->addInst(isolation_inst);
        }

        auto enable_iterm = isolation_inst->getITerm(enable_term);
        auto data_iterm = isolation_inst->getITerm(data_term);
        auto output_iterm = isolation_inst->getITerm(output_term);
        // Reuse original net to connect to data port of isolation cell
        data_iterm->connect(net);
        // Create new net to connect from isolation output to iterms outside the
        // domain
        std::string net_out_name = net->getName() + "_o";
        auto net_out = odb::dbNet::create(block, net_out_name.c_str());
        output_iterm->connect(net_out);

        // Add inverter to output
        if (inv_needed == 2 || inv_needed == 3) {
          std::string inv_name = inst->getName() + "_isolation_inv_out";
          auto inv_inst
              = odb::dbInst::create(block, inverter_m, inv_name.c_str());
          inv_inst->getITerm(input_m)->connect(net_out);
          // overwrite output net to external domain
          net_out_name = net->getName() + "_inv";
          net_out = odb::dbNet::create(block, net_out_name.c_str());

          inv_inst->getITerm(output_m)->connect(net_out);
        }

        for (auto&& external_iterm : external_iterms) {
          external_iterm->disconnect();
          external_iterm->connect(net_out);
        }

        // Find control net and connect it to control port of isolation cell
        auto control_net = block->findNet(iso->getIsolationSignal().c_str());
        if (!control_net) {
          logger->warn(utl::ODB,
                       15122,
                       "Isolation %s has non=existing control net %s, "
                       "isolation cell has no control signal",
                       iso->getName(),
                       iso->getIsolationSignal());
        } else {
          // Add inverter to control signal if needed
          if (inv_needed == 1 || inv_needed == 3) {
            std::string inv_name = inst->getName() + "_isolation_inv_control";

            auto inv_inst
                = odb::dbInst::create(block, inverter_m, inv_name.c_str());
            inv_inst->getITerm(input_m)->connect(control_net);
            // overwrite output net to external domain
            std::string control_net_out_name
                = control_net->getName() + "_inv_" + inst->getName();

            control_net
                = odb::dbNet::create(block, control_net_out_name.c_str());
            inv_inst->getITerm(output_m)->connect(control_net);
          }
          enable_iterm->connect(control_net);
        }
      }
    }
  }

  return true;
}

}  // namespace upf
