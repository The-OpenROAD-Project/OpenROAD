/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022-2023, The Regents of the University of California,
// Google LLC All rights reserved.
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

#include "upf/upf.h"

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"

namespace upf {

bool create_power_domain(utl::Logger* logger,
                         odb::dbBlock* block,
                         const char* name)
{
  if (odb::dbPowerDomain::create(block, name) == nullptr) {
    logger->warn(utl::UPF, 1, "Creation of '%s' power domain failed", name);
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
        utl::UPF,
        2,
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
    logger->warn(utl::UPF, 3, "Creation of '%s' logic port failed", name);
    return false;
  }
  return true;
}

bool create_power_switch(utl::Logger* logger,
                         odb::dbBlock* block,
                         const char* name,
                         const char* power_domain)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(power_domain);
  if (pd == nullptr) {
    logger->warn(
        utl::UPF,
        4,
        "Couldn't retrieve power domain '%s' while creating power switch '%s'",
        power_domain,
        name);
    return false;
  }

  odb::dbPowerSwitch* ps = odb::dbPowerSwitch::create(block, name);
  if (ps == nullptr) {
    logger->warn(utl::UPF, 5, "Creation of '%s' power switch failed", name);
    return false;
  }

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
        utl::UPF,
        6,
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
        utl::UPF,
        7,
        "Couldn't retrieve power switch '%s' while adding on state '%s'",
        name,
        on_state);
    return false;
  }
  ps->addOnState(on_state);
  return true;
}

bool update_power_switch_input(utl::Logger* logger,
                               odb::dbBlock* block,
                               const char* name,
                               const char* in_port)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name);
  if (ps == nullptr) {
    logger->warn(
        utl::UPF,
        8,
        "Couldn't retrieve power switch '%s' while adding input port '%s'",
        name,
        in_port);
    return false;
  }
  ps->addInSupplyPort(in_port);
  return true;
}

bool update_power_switch_output(utl::Logger* logger,
                                odb::dbBlock* block,
                                const char* name,
                                const char* out_port)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name);
  if (ps == nullptr) {
    logger->warn(
        utl::UPF,
        9,
        "Couldn't retrieve power switch '%s' while adding output port '%s'",
        name,
        out_port);
    return false;
  }
  ps->addOutSupplyPort(out_port);
  return true;
}

bool update_power_switch_cell(utl::Logger* logger,
                              odb::dbBlock* block,
                              const char* name,
                              odb::dbMaster* cell)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name);
  if (ps == nullptr) {
    logger->warn(utl::UPF,
                 10,
                 "Couldn't retrieve power switch '%s' while adding cell '%s'",
                 name,
                 cell->getName().c_str());
    return false;
  }
  ps->setLibCell(cell);
  return true;
}

bool update_power_switch_port_map(utl::Logger* logger,
                                  odb::dbBlock* block,
                                  const char* name,
                                  const char* model_port,
                                  const char* switch_port)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name);
  if (ps == nullptr) {
    logger->warn(
        utl::UPF,
        11,
        "Couldn't retrieve power switch '%s' while updating port mapping",
        name);
    return false;
  }
  ps->addPortMap(model_port, switch_port);
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
    logger->warn(utl::UPF,
                 12,
                 "Couldn't retrieve power domain '%s' while creating/updating "
                 "isolation '%s'",
                 power_domain,
                 name);
    return false;
  }

  odb::dbIsolation* iso = block->findIsolation(name);
  if (iso == nullptr && update) {
    logger->warn(
        utl::UPF, 13, "Couldn't update a non existing isolation %s", name);
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
    logger->warn(utl::UPF,
                 14,
                 "Couldn't retrieve power domain '%s' while updating "
                 "isolation '%s'",
                 power_domain,
                 strategy);
    return false;
  }

  odb::dbIsolation* iso = block->findIsolation(strategy);
  if (iso == nullptr) {
    logger->warn(utl::UPF, 15, "Couldn't find isolation %s", strategy);
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
    logger->warn(utl::UPF,
                 16,
                 "Couldn't retrieve power domain '%s' while updating its area ",
                 domain);
    return false;
  }

  pd->setArea(x1, y1, x2, y2);

  return true;
}

static odb::dbPowerDomain* match_module_to_domain(
    std::map<std::string, odb::dbPowerDomain*>& module_to_domain,
    std::map<std::string, odb::dbPowerDomain*>& path_to_domain,
    const std::string& current_path)
{
  std::string longest_prefix;
  int longest_prefix_length = 0;

  for (auto const& path : path_to_domain) {
    std::string name = path.first;
    if (current_path.find(name) == 0 && name.length() > longest_prefix_length) {
      longest_prefix_length = name.length();
      longest_prefix = name;
    }
  }

  if (longest_prefix_length == 0) {
    module_to_domain[current_path] = path_to_domain["."];
  } else {
    module_to_domain[current_path] = path_to_domain[longest_prefix];
  }

  return module_to_domain[current_path];
}

static bool build_domain_hierarchy(
    std::map<std::string, odb::dbPowerDomain*>& module_to_domain,
    std::map<std::string, odb::dbPowerDomain*>& path_to_domain,
    odb::dbModule* current_module,
    odb::dbPowerDomain* current_domain)
{
  auto child_modules = current_module->getChildren();
  for (auto&& child_mod_inst : child_modules) {
    std::string name = child_mod_inst->getHierarchicalName();
    auto matched_domain
        = match_module_to_domain(module_to_domain, path_to_domain, name);
    if (!matched_domain->isTop()) {
      matched_domain->setParent(current_domain);
    }
    odb::dbModule* child_mod = child_mod_inst->getMaster();
    build_domain_hierarchy(
        module_to_domain, path_to_domain, child_mod, matched_domain);
  }

  return true;
}

static bool check_isolation_match(sta::FuncExpr* func,
                                  sta::LibertyPort* enable,
                                  bool sense,
                                  bool clamp_val,
                                  utl::Logger* logger,
                                  bool& invert_control,
                                  bool& invert_output)
{
  bool enable_is_left = (func->left() && func->left()->hasPort(enable));
  bool enable_is_right = (func->right() && func->right()->hasPort(enable));
  if (!enable_is_left && !enable_is_right) {
    logger->warn(utl::UPF, 17, "isolation cell has no enable port");
    return false;
  }

  sta::FuncExpr* enable_func = (enable_is_left) ? func->left() : func->right();
  bool enable_is_inverted
      = (enable_func->op() == sta::FuncExpr::Operator::op_not);
  bool new_enable_sense = (enable_is_inverted) ? !sense : sense;

  switch (func->op()) {
    case sta::FuncExpr::Operator::op_or:
      invert_output = !clamp_val;
      invert_control = !new_enable_sense;
      break;
    case sta::FuncExpr::Operator::op_and:
      invert_output = clamp_val;
      invert_control = new_enable_sense;
      break;
    default:
      logger->warn(utl::UPF, 18, "unknown isolation cell function");
      return false;
  }

  return true;
}

static bool associate_groups(
    utl::Logger* logger,
    odb::dbBlock* block,
    std::map<std::string, odb::dbPowerDomain*>& path_to_domain,
    odb::dbPowerDomain*& top_domain)
{
  auto pds = block->getPowerDomains();

  for (auto&& domain : pds) {
    bool is_top_domain = false;
    auto els = domain->getElements();

    // Handling the case where there are multiple
    // domains pointing to the same path, which shouldn't occur!!
    for (auto&& el : els) {
      if (path_to_domain.find(el) == path_to_domain.end()) {
        path_to_domain[el] = domain;
      } else {
        logger->error(utl::UPF,
                      19,
                      "multiple power domain definitions for the same path %s",
                      el.c_str());
      }
      if (el == ".") {
        domain->setTop(true);
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
          utl::UPF, 20, "Creation of '%s' region failed", domain->getName());
      return false;
    }
    region->setRegionType(odb::dbRegionType::EXCLUSIVE);
    // Specifying region area
    int x1, x2, y1, y2;
    if (domain->getArea(x1, y1, x2, y2)) {
      odb::dbBox::create(region, x1, y1, x2, y2);
    } else {
      logger->warn(utl::UPF,
                   21,
                   "No area specified for '%s' power domain",
                   domain->getName());
    }

    auto group = odb::dbGroup::create(region, domain->getName());
    if (!group) {
      logger->warn(utl::UPF,
                   22,
                   "Creation of '%s' group failed, duplicate group exists.",
                   domain->getName());
      return false;
    }
    group->setType(odb::dbGroupType::POWER_DOMAIN);
    domain->setGroup(group);
  }

  return true;
}

static bool instantiate_logic_ports(utl::Logger* logger, odb::dbBlock* block)
{
  bool success = true;
  auto lps = block->getLogicPorts();
  for (auto&& port : lps) {
    if (!odb::dbNet::create(block, port->getName())) {
      logger->warn(utl::UPF,
                   23,
                   "Creation of '{}' dbNet from UPF Logic Port failed",
                   port->getName());
      success = false;
    }
  }

  return success;
}

static bool add_insts_to_group(
    odb::dbBlock* block,
    odb::dbPowerDomain* top_domain,
    std::map<std::string, odb::dbPowerDomain*>& path_to_domain)
{
  // Create a reverse lookup table
  // Module Path (Including empty i.e top module) -> Power Domain
  // If nothing matches then there's no power domain matching
  std::map<std::string, odb::dbPowerDomain*> module_to_domain;
  odb::dbModule* top_module = block->getTopModule();

  build_domain_hierarchy(
      module_to_domain, path_to_domain, top_module, top_domain);

  // For each dbInst, add it to the dbGroup matching its module's powerdomain
  auto all_instances = block->getInsts();
  for (auto&& inst : all_instances) {
    bool found = false;
    std::string path = ".";
    auto mod = inst->getModule();
    if (!mod) {
      continue;
    }
    if (mod == block->getTopModule()) {
      found = (module_to_domain.find(".") != module_to_domain.end());
    } else {
      auto modInst = mod->getModInst();
      path = modInst->getHierarchicalName();
      found = (module_to_domain.find(path) != module_to_domain.end());
    }

    if (found) {
      odb::dbPowerDomain* domain = module_to_domain[path];
      // Verify that this domain is not the top domain (i.e has a parent)
      if (domain->getParent()) {
        domain->getGroup()->addInst(inst);
      }
    }
  }

  return true;
}

static bool find_smallest_inverter(sta::dbNetwork* network,
                                   odb::dbBlock* block,
                                   odb::dbMaster*& inverter_m,
                                   odb::dbMTerm*& input_m,
                                   odb::dbMTerm*& output_m)
{
  float smallest_area = std::numeric_limits<float>::max();
  sta::LibertyCell* smallest_inverter = nullptr;
  sta::LibertyPort *inverter_input = nullptr, *inverter_output = nullptr;

  bool found = false;

  auto libs = block->getDataBase()->getLibs();
  for (auto&& lib : libs) {
    auto masters = lib->getMasters();
    for (auto&& master : masters) {
      auto master_cell_ = network->dbToSta(master);
      auto libcell_ = network->libertyCell(master_cell_);

      if (libcell_ && libcell_->isInverter()
          && libcell_->area() < smallest_area) {
        smallest_area = libcell_->area();
        smallest_inverter = libcell_;
        smallest_inverter->bufferPorts(inverter_input, inverter_output);
        inverter_m = network->staToDb(smallest_inverter);
        input_m = network->staToDb(inverter_input);
        output_m = network->staToDb(inverter_output);
        found = true;
      }
    }
  }

  return found;
}

static bool find_smallest_isolation(sta::dbNetwork* network,
                                    utl::Logger* logger,
                                    odb::dbIsolation* iso,
                                    odb::dbMaster*& smallest_iso_m,
                                    odb::dbMTerm*& enable_term,
                                    odb::dbMTerm*& data_term,
                                    odb::dbMTerm*& output_term,
                                    bool& invert_output,
                                    bool& invert_control)

{
  bool isolation_sense = (iso->getIsolationSense() == "high");
  bool isolation_clamp_val = (iso->getClampValue() == "1");
  auto iso_cells = iso->getIsolationCells();
  if (iso_cells.empty()) {
    logger->warn(utl::UPF,
                 24,
                 "Isolation %s defined, but no cells defined.",
                 iso->getName());
    return false;
  }

  // For now using the first defined isolaton cell

  sta::LibertyCell* smallest_iso_l = nullptr;
  float smallest_area = std::numeric_limits<float>::max();

  for (auto&& iso : iso_cells) {
    sta::Cell* masterCell = network->dbToSta(iso);
    sta::LibertyCell* libertyCell = network->libertyCell(masterCell);

    if (libertyCell->area() < smallest_area) {
      smallest_iso_l = libertyCell;
      smallest_iso_m = iso;
      smallest_area = libertyCell->area();
    }
  }

  if (smallest_iso_m == nullptr) {
    logger->warn(utl::UPF,
                 25,
                 "Isolation %s cells defined, but can't find any in the lib.",
                 iso->getName());
    return false;
  }

  auto cell_terms = smallest_iso_m->getMTerms();

  // Find enable & data pins for the isolation cell
  sta::LibertyPort* out_lib_port = nullptr;
  sta::LibertyPort* enable_lib_port = nullptr;
  for (auto&& term : cell_terms) {
    sta::LibertyPort* lib_port
        = smallest_iso_l->findLibertyPort(term->getName().c_str());

    if (!lib_port) {
      continue;
    }

    if (lib_port->isolationCellData()) {
      data_term = term;
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

  if (!output_term || !data_term || !enable_term || !out_lib_port) {
    logger->warn(utl::UPF,
                 26,
                 "Isolation %s cells defined, but can't find one of output, "
                 "data or enable terms.",
                 iso->getName());
    return false;
  }

  // Determine if an inverter is needed for the control pin & output
  auto func = out_lib_port->function();

  return check_isolation_match(func,
                               enable_lib_port,
                               isolation_sense,
                               isolation_clamp_val,
                               logger,
                               invert_control,
                               invert_output);
}

static bool insert_isolation_cell(odb::dbBlock* block,
                                  odb::dbInst* inst,
                                  odb::dbNet* input_net,
                                  odb::dbNet* control_net,
                                  odb::dbNet* output_net,
                                  odb::dbMTerm* enable_term,
                                  odb::dbMTerm* data_term,
                                  odb::dbMTerm* output_term,
                                  odb::dbMaster* smallest_iso_m,
                                  bool invert_output,
                                  bool invert_control,
                                  odb::dbMaster* inverter_m,
                                  odb::dbMTerm* input_m,
                                  odb::dbMTerm* output_m,
                                  odb::dbGroup* target_group)
{
  std::string inst_name = inst->getName() + "_" + input_net->getName() + "_"
                          + output_net->getName() + "_isolation";

  auto isolation_inst
      = odb::dbInst::create(block, smallest_iso_m, inst_name.c_str());

  if (target_group) {
    target_group->addInst(isolation_inst);
  }

  auto enable_iterm = isolation_inst->getITerm(enable_term);
  auto data_iterm = isolation_inst->getITerm(data_term);
  auto output_iterm = isolation_inst->getITerm(output_term);

  if (invert_output) {
    std::string inv_name = isolation_inst->getName() + "_inv_out";
    auto inv_inst = odb::dbInst::create(block, inverter_m, inv_name.c_str());

    // connect output of inverter to output net connected to the instance
    // outside of this power domain
    inv_inst->getITerm(output_m)->connect(output_net);

    auto inverted_out_net = odb::dbNet::create(block, inv_name.c_str());
    // connect new net to output of isolation and input of inverter
    inv_inst->getITerm(input_m)->connect(inverted_out_net);
    output_iterm->connect(inverted_out_net);

  } else {
    output_iterm->connect(output_net);
  }

  if (invert_control) {
    std::string inv_name = isolation_inst->getName() + "_inv_control";
    auto inv_inst = odb::dbInst::create(block, inverter_m, inv_name.c_str());

    // connect control net to inverter input
    inv_inst->getITerm(input_m)->connect(control_net);

    auto inverted_control_net = odb::dbNet::create(block, inv_name.c_str());

    // connect new net to output of inverter and input of isolation cell enable
    // port
    inv_inst->getITerm(output_m)->connect(inverted_control_net);
    enable_iterm->connect(inverted_control_net);

  } else {
    enable_iterm->connect(control_net);
  }

  // connect isolation terms to existing nets
  data_iterm->connect(input_net);

  return true;
}

static bool isolate_port(utl::Logger* logger,
                         odb::dbBlock* block,
                         odb::dbInst* inst,
                         odb::dbITerm* iterm,
                         odb::dbPowerDomain* pd,
                         odb::dbIsolation* iso,
                         odb::dbMTerm* enable_term,
                         odb::dbMTerm* data_term,
                         odb::dbMTerm* output_term,
                         odb::dbMaster* smallest_iso_m,
                         bool invert_output,
                         bool invert_control,
                         odb::dbMaster* inverter_m,
                         odb::dbMTerm* input_m,
                         odb::dbMTerm* output_m)
{
  auto net = iterm->getNet();

  if (!net) {
    return true;
  }

  auto connectedIterms = net->getITerms();

  if (connectedIterms.size() < 2) {
    return true;
  }

  // Find ITERMS that belong to instances outside of this power domain
  std::vector<odb::dbITerm*> external_iterms;
  for (auto&& connectedIterm : connectedIterms) {
    auto connectedInst = connectedIterm->getInst();

    if (!connectedInst->getGroup()
        || connectedInst->getGroup() != pd->getGroup()) {
      external_iterms.push_back(connectedIterm);
    }
  }

  if (external_iterms.empty()) {
    return true;
  }

  auto control_net = block->findNet(iso->getIsolationSignal().c_str());
  if (!control_net) {
    logger->warn(utl::UPF,
                 27,
                 "Isolation {} has nonexisting control net {}",
                 iso->getName(),
                 iso->getIsolationSignal());

    return false;
  }

  if (iso->getLocation() == "fanout") {
    for (auto&& external_iterm : external_iterms) {
      odb::dbGroup* target_group = external_iterm->getInst()->getGroup();
      std::string net_out_name
          = net->getName() + "_" + external_iterm->getMTerm()->getName() + "_o";
      auto net_out = odb::dbNet::create(block, net_out_name.c_str());

      insert_isolation_cell(block,
                            inst,
                            net,
                            control_net,
                            net_out,
                            enable_term,
                            data_term,
                            output_term,
                            smallest_iso_m,
                            invert_output,
                            invert_control,
                            inverter_m,
                            input_m,
                            output_m,
                            target_group);

      external_iterm->disconnect();
      external_iterm->connect(net_out);
    }
  } else {
    odb::dbGroup* target_group = nullptr;

    if (iso->getLocation() == "parent") {
      auto ppd = pd->getParent();
      // if the parent domain is the top
      // domain, don't add to any group
      if (ppd && ppd->getGroup()) {
        target_group = ppd->getGroup();
      }
    } else if (iso->getLocation() == "self") {
      target_group = pd->getGroup();
    } else {
      logger->warn(utl::UPF,
                   28,
                   "Isolation %s has location %s, but only self|parent|fanout"
                   "supported, defaulting to self.",
                   iso->getName(),
                   iso->getLocation());
      target_group = pd->getGroup();
    }

    std::string net_out_name = net->getName() + "_o";
    auto net_out = odb::dbNet::create(block, net_out_name.c_str());

    insert_isolation_cell(block,
                          inst,
                          net,
                          control_net,
                          net_out,
                          enable_term,
                          data_term,
                          output_term,
                          smallest_iso_m,
                          invert_output,
                          invert_control,
                          inverter_m,
                          input_m,
                          output_m,
                          target_group);

    for (auto&& external_iterm : external_iterms) {
      external_iterm->disconnect();
      external_iterm->connect(net_out);
    }
  }

  return true;
}

bool eval_upf(sta::dbNetwork* network, utl::Logger* logger, odb::dbBlock* block)
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
  std::map<std::string, odb::dbPowerDomain*> path_to_domain;
  odb::dbPowerDomain* top_domain = nullptr;

  // For each power domain
  // 1. Create a dbGroup associated to it
  // 2. Create a dbRegion associated to it
  // 3. map defined path to the power domain
  if (!associate_groups(logger, block, path_to_domain, top_domain)) {
    return false;
  }

  if (top_domain == nullptr) {
    // A TOP DOMAIN should always exist
    logger->error(utl::UPF, 29, "No TOP DOMAIN found, aborting");
    return false;
  }

  // Associate each instance with its power domain group
  add_insts_to_group(block, top_domain, path_to_domain);

  // find the smallest possible inverter in advance
  odb::dbMaster* inverter_m = nullptr;
  odb::dbMTerm *input_m = nullptr, *output_m = nullptr;
  bool inverter_found
      = find_smallest_inverter(network, block, inverter_m, input_m, output_m);

  // PowerDomains have a hierarchy and when the isolation's
  // location is parent, it should be placed in the parent's power domain
  // For each domain, determine for each net, if there's an input to another
  // domain, create two nets that split 'data' into 'data_i' + 'data_o' 'data_i'
  // goes into the isolation cell from the original cell 'data_o' got outside of
  // the isolation cell into the end cell then connect isolation signal to the
  // enable (or reversed depending on iso sense + cell type) place the isolation
  // cell into either first or second domain?? (Not sure exactly)

  for (auto&& domain : pds) {
    if (domain == top_domain) {
      continue;
    }

    // For now we're only using the first isolation
    // TODO: determine what needs to be done in case of multiple strategies
    // for same domain
    auto isos = domain->getIsolations();

    if (isos.empty()) {
      continue;
    }

    if (isos.size() > 1) {
      logger->warn(
          utl::UPF,
          30,
          "Multiple isolation strategies defined for the same power domain %s.",
          domain->getName());
    }

    odb::dbIsolation* iso = isos[0];

    odb::dbMTerm* enable_term = nullptr;
    odb::dbMTerm* data_term = nullptr;
    odb::dbMTerm* output_term = nullptr;
    odb::dbMaster* smallest_iso_m = nullptr;
    bool invert_output, invert_control;

    if (!find_smallest_isolation(network,
                                 logger,
                                 iso,
                                 smallest_iso_m,
                                 enable_term,
                                 data_term,
                                 output_term,
                                 invert_output,
                                 invert_control)) {
      continue;
    }

    if ((invert_output || invert_control) && !inverter_found) {
      logger->warn(utl::UPF, 31, "can't find any inverters");
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
        if (iterm->getIoType() != odb::dbIoType::OUTPUT) {
          continue;
        }

        isolate_port(logger,
                     block,
                     inst,
                     iterm,
                     domain,
                     iso,
                     enable_term,
                     data_term,
                     output_term,
                     smallest_iso_m,
                     invert_output,
                     invert_control,
                     inverter_m,
                     input_m,
                     output_m);
      }
    }
  }

  return true;
}

bool create_or_update_level_shifter(utl::Logger* logger,
                                    odb::dbBlock* block,
                                    const char* name,
                                    const char* domain,
                                    const char* source,
                                    const char* sink,
                                    const char* use_functional_equivalence,
                                    const char* applies_to,
                                    const char* applies_to_boundary,
                                    const char* rule,
                                    const char* threshold,
                                    const char* no_shift,
                                    const char* force_shift,
                                    const char* location,
                                    const char* input_supply,
                                    const char* output_supply,
                                    const char* internal_supply,
                                    const char* name_prefix,
                                    const char* name_suffix,
                                    bool update)
{
  odb::dbLevelShifter* ls = nullptr;
  odb::dbPowerDomain* pd = block->findPowerDomain(domain);

  if (update) {
    ls = block->findLevelShifter(name);
    if (ls == nullptr) {
      logger->warn(
          utl::UPF, 32, "Couldn't find level shifter {} to update", name);
      return false;
    }
  }

  if (ls == nullptr) {
    if (pd == nullptr) {
      logger->warn(utl::UPF,
                   38,
                   "Couldn't find power domain {} for level shifter {}",
                   domain,
                   name);
      return false;
    }
    ls = odb::dbLevelShifter::create(block, name, pd);
    if (ls == nullptr) {
      logger->warn(utl::UPF, 44, "Couldn't create level shifter {}", name);
      return false;
    }
  }

  if (strlen(source) > 0) {
    ls->setSource(source);
  }

  if (strlen(sink) > 0) {
    ls->setSink(sink);
  }

  if (strlen(applies_to) > 0) {
    ls->setAppliesTo(applies_to);
  }

  if (strlen(applies_to_boundary) > 0) {
    ls->setAppliesToBoundary(applies_to_boundary);
  }

  if (strlen(rule) > 0) {
    ls->setRule(rule);
  }

  if (strlen(location) > 0) {
    ls->setLocation(location);
  }

  if (strlen(input_supply) > 0) {
    ls->setInputSupply(input_supply);
  }

  if (strlen(output_supply) > 0) {
    ls->setOutputSupply(output_supply);
  }

  if (strlen(internal_supply) > 0) {
    ls->setInternalSupply(internal_supply);
  }

  if (strlen(name_prefix) > 0) {
    ls->setNamePrefix(name_prefix);
  }

  if (strlen(name_suffix) > 0) {
    ls->setNameSuffix(name_suffix);
  }

  if (strlen(threshold) > 0) {
    ls->setThreshold(std::stof(threshold));
  }

  if (strlen(no_shift) > 0) {
    ls->setNoShift(std::stoi(no_shift));
  }

  if (strlen(force_shift) > 0) {
    ls->setForceShift(std::stoi(force_shift));
  }

  ls->setUseFunctionalEquivalence(
      strcasecmp(use_functional_equivalence, "FALSE") ? true : false);

  return true;
}

bool add_level_shifter_element(utl::Logger* logger,
                               odb::dbBlock* block,
                               const char* level_shifter_name,
                               const char* element)
{
  odb::dbLevelShifter* ls = block->findLevelShifter(level_shifter_name);
  if (ls == nullptr) {
    logger->warn(utl::UPF,
                 39,
                 "Couldn't find level shifter {} to add element {}",
                 level_shifter_name,
                 element);
    return false;
  }

  std::string _element(element);
  ls->addElement(_element);

  return true;
}

bool exclude_level_shifter_element(utl::Logger* logger,
                                   odb::dbBlock* block,
                                   const char* level_shifter_name,
                                   const char* exclude_element)
{
  odb::dbLevelShifter* ls = block->findLevelShifter(level_shifter_name);
  if (ls == nullptr) {
    logger->warn(utl::UPF,
                 41,
                 "Couldn't find level shifter {} to exclude element {}",
                 level_shifter_name,
                 exclude_element);
    return false;
  }

  ls->addExcludeElement(exclude_element);

  return true;
}

bool handle_level_shifter_instance(utl::Logger* logger,
                                   odb::dbBlock* block,
                                   const char* level_shifter_name,
                                   const char* instance_name,
                                   const char* port_name)
{
  odb::dbLevelShifter* ls = block->findLevelShifter(level_shifter_name);
  if (ls == nullptr) {
    logger->warn(utl::UPF,
                 42,
                 "Couldn't find level shifter {} to add instance {}",
                 level_shifter_name,
                 instance_name);
    return false;
  }

  ls->addInstance(instance_name, port_name);

  return true;
}

}  // namespace upf
