// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "upf/upf.h"

#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "utl/Logger.h"
#include "writer.h"

namespace upf {

bool create_power_domain(utl::Logger* logger,
                         odb::dbBlock* block,
                         const std::string& name)
{
  if (odb::dbPowerDomain::create(block, name.c_str()) == nullptr) {
    logger->warn(utl::UPF, 1, "Creation of {} power domain failed", name);
    return false;
  }

  return true;
}

bool update_power_domain(utl::Logger* logger,
                         odb::dbBlock* block,
                         const std::string& name,
                         const std::string& elements)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(name.c_str());
  if (pd != nullptr) {
    pd->addElement(elements);
  } else {
    logger->warn(utl::UPF,
                 2,
                 "Couldn't retrieve power domain {} while adding element {}",
                 name,
                 elements);
    return false;
  }
  return true;
}

bool create_logic_port(utl::Logger* logger,
                       odb::dbBlock* block,
                       const std::string& name,
                       const std::string& direction)
{
  if (odb::dbLogicPort::create(block, name.c_str(), direction) == nullptr) {
    logger->warn(utl::UPF, 3, "Creation of {} logic port failed", name);
    return false;
  }
  return true;
}

bool create_power_switch(utl::Logger* logger,
                         odb::dbBlock* block,
                         const std::string& name,
                         const std::string& power_domain)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(power_domain.c_str());
  if (pd == nullptr) {
    logger->warn(
        utl::UPF,
        4,
        "Couldn't retrieve power domain {} while creating power switch {}",
        power_domain,
        name);
    return false;
  }

  odb::dbPowerSwitch* ps = odb::dbPowerSwitch::create(block, name.c_str());
  if (ps == nullptr) {
    logger->warn(utl::UPF, 5, "Creation of {} power switch failed", name);
    return false;
  }

  ps->setPowerDomain(pd);
  pd->addPowerSwitch(ps);
  return true;
}

bool update_power_switch_control(utl::Logger* logger,
                                 odb::dbBlock* block,
                                 const std::string& name,
                                 const std::string& control_port,
                                 const std::string& control_net)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name.c_str());
  if (ps == nullptr) {
    logger->warn(
        utl::UPF,
        6,
        "Couldn't retrieve power switch {} while adding control port {}",
        name,
        control_port);
    return false;
  }

  ps->addControlPort(control_port, control_net);
  return true;
}

bool update_power_switch_acknowledge(utl::Logger* logger,
                                     odb::dbBlock* block,
                                     const std::string& name,
                                     const std::string& port,
                                     const std::string& net,
                                     const std::string& boolean)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name.c_str());
  if (ps == nullptr) {
    logger->warn(utl::UPF,
                 61,
                 "Couldn't retrieve power switch {} while adding acknowledge "
                 "port {}",
                 name,
                 port);
    return false;
  }

  ps->addAcknowledgePort(port, net, boolean);
  return true;
}

bool update_power_switch_on(utl::Logger* logger,
                            odb::dbBlock* block,
                            const std::string& name,
                            const std::string& on_state,
                            const std::string& port_name,
                            const std::string& boolean)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name.c_str());
  if (ps == nullptr) {
    logger->warn(utl::UPF,
                 7,
                 "Couldn't retrieve power switch {} while adding on state {}",
                 name,
                 on_state);
    return false;
  }
  ps->addOnState(on_state, port_name, boolean);
  return true;
}

bool update_power_switch_input(utl::Logger* logger,
                               odb::dbBlock* block,
                               const std::string& name,
                               const std::string& in_port,
                               const std::string& net)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name.c_str());
  if (ps == nullptr) {
    logger->warn(utl::UPF,
                 8,
                 "Couldn't retrieve power switch {} while adding input port {}",
                 name,
                 in_port);
    return false;
  }
  ps->addInSupplyPort(in_port, net);
  return true;
}

bool update_power_switch_output(utl::Logger* logger,
                                odb::dbBlock* block,
                                const std::string& name,
                                const std::string& out_port,
                                const std::string& net)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name.c_str());
  if (ps == nullptr) {
    logger->warn(
        utl::UPF,
        9,
        "Couldn't retrieve power switch {} while adding output port {}",
        name,
        out_port);
    return false;
  }
  ps->setOutSupplyPort(out_port, net);
  return true;
}

bool update_power_switch_cell(utl::Logger* logger,
                              odb::dbBlock* block,
                              const std::string& name,
                              odb::dbMaster* cell)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name.c_str());
  if (ps == nullptr) {
    logger->warn(utl::UPF,
                 10,
                 "Couldn't retrieve power switch {} while adding cell {}",
                 name,
                 cell->getName().c_str());
    return false;
  }
  ps->setLibCell(cell);
  return true;
}

bool update_power_switch_port_map(utl::Logger* logger,
                                  odb::dbBlock* block,
                                  const std::string& name,
                                  const std::string& model_port,
                                  const std::string& switch_port)
{
  odb::dbPowerSwitch* ps = block->findPowerSwitch(name.c_str());
  if (ps == nullptr) {
    logger->warn(
        utl::UPF,
        11,
        "Couldn't retrieve power switch {} while updating port mapping",
        name);
    return false;
  }
  ps->addPortMap(model_port, switch_port);
  return true;
}

bool set_isolation(utl::Logger* logger,
                   odb::dbBlock* block,
                   const std::string& name,
                   const std::string& power_domain,
                   bool update,
                   const std::string& applies_to,
                   const std::string& clamp_value,
                   const std::string& signal,
                   const std::string& sense,
                   const std::string& location)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(power_domain.c_str());
  if (pd == nullptr) {
    logger->warn(utl::UPF,
                 12,
                 "Couldn't retrieve power domain {} while creating/updating "
                 "isolation {}",
                 power_domain,
                 name);
    return false;
  }

  odb::dbIsolation* iso = block->findIsolation(name.c_str());
  if (iso == nullptr && update) {
    logger->warn(
        utl::UPF, 13, "Couldn't update a non existing isolation {}", name);
    return false;
  }

  if (iso == nullptr) {
    iso = odb::dbIsolation::create(block, name.c_str());
  }

  if (!update) {
    iso->setPowerDomain(pd);
    pd->addIsolation(iso);
  }

  if (!applies_to.empty()) {
    iso->setAppliesTo(applies_to);
  }

  if (!clamp_value.empty()) {
    iso->setClampValue(clamp_value);
  }

  if (!signal.empty()) {
    iso->setIsolationSignal(signal);
  }

  if (!sense.empty()) {
    iso->setIsolationSense(sense);
  }

  if (!location.empty()) {
    iso->setLocation(location);
  }

  return true;
}

bool use_interface_cell(utl::Logger* logger,
                        odb::dbBlock* block,
                        const std::string& power_domain,
                        const std::string& strategy,
                        const std::string& cell)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(power_domain.c_str());
  if (pd == nullptr) {
    logger->warn(utl::UPF,
                 14,
                 "Couldn't retrieve power domain {} while updating "
                 "isolation {}",
                 power_domain,
                 strategy);
    return false;
  }

  odb::dbIsolation* iso = block->findIsolation(strategy.c_str());
  if (iso == nullptr) {
    logger->warn(utl::UPF, 15, "Couldn't find isolation {}", strategy);
    return false;
  }

  iso->addIsolationCell(cell);

  return true;
}

bool set_domain_area(utl::Logger* logger,
                     odb::dbBlock* block,
                     const std::string& domain,
                     const odb::Rect& area)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(domain.c_str());
  if (pd == nullptr) {
    logger->warn(utl::UPF,
                 16,
                 "Couldn't retrieve power domain {} while updating its area ",
                 domain);
    return false;
  }

  pd->setArea(area);

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
    if (current_path.starts_with(name)
        && name.length() > longest_prefix_length) {
      longest_prefix_length = name.length();
      longest_prefix = std::move(name);
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
  bool enable_is_inverted = (enable_func->op() == sta::FuncExpr::Op::not_);
  bool new_enable_sense = (enable_is_inverted) ? !sense : sense;

  switch (func->op()) {
    case sta::FuncExpr::Op::or_:
      invert_output = !clamp_val;
      invert_control = !new_enable_sense;
      break;
    case sta::FuncExpr::Op::and_:
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
                      "multiple power domain definitions for the same path {}",
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
          utl::UPF, 20, "Creation of {} region failed", domain->getName());
      return false;
    }
    region->setRegionType(odb::dbRegionType::EXCLUSIVE);
    // Specifying region area
    odb::Rect area;
    if (domain->getArea(area)) {
      odb::dbBox::create(
          region, area.xMin(), area.yMin(), area.xMax(), area.yMax());
    } else {
      logger->warn(utl::UPF,
                   21,
                   "No area specified for {} power domain",
                   domain->getName());
    }

    auto group = odb::dbGroup::create(region, domain->getName());
    if (!group) {
      logger->warn(utl::UPF,
                   22,
                   "Creation of {} group failed, duplicate group exists.",
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
    auto net = odb::dbNet::create(block, port->getName());
    if (!net) {
      logger->warn(utl::UPF,
                   23,
                   "Creation of '{}' dbNet from UPF Logic Port failed",
                   port->getName());
      success = false;
    }
    if (success && !odb::dbBTerm::create(net, port->getName())) {
      logger->warn(utl::UPF,
                   45,
                   "Creation of '{}' dbBTerm from UPF Logic Port failed",
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
      path = mod->getModInst()->getHierarchicalName();
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
      auto master_cell = network->dbToSta(master);
      auto lib_cell = network->libertyCell(master_cell);

      if (lib_cell && lib_cell->isInverter()
          && lib_cell->area() < smallest_area) {
        smallest_area = lib_cell->area();
        smallest_inverter = lib_cell;
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
                                    odb::dbMaster* inverter_m,
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
                 "Isolation {} defined, but no cells defined.",
                 iso->getName());
    return false;
  }

  // Search for the most appropriate isolation cell
  float smallest_area = std::numeric_limits<float>::max();
  float inverter_area = 0;
  if (inverter_m) {
    inverter_area = inverter_m->getArea();
  }

  for (auto&& iso_cell : iso_cells) {
    sta::Cell* master_cell = network->dbToSta(iso_cell);
    sta::LibertyCell* liberty_cell = network->libertyCell(master_cell);

    // Find enable & data pins for the isolation cell
    sta::LibertyPort* tmp_enable_lib_port = nullptr;
    sta::LibertyPort* tmp_out_lib_port = nullptr;
    odb::dbMTerm* tmp_enable_term = nullptr;
    odb::dbMTerm* tmp_data_term = nullptr;
    odb::dbMTerm* tmp_output_term = nullptr;

    for (auto&& term : iso_cell->getMTerms()) {
      sta::LibertyPort* lib_port
          = liberty_cell->findLibertyPort(term->getName().c_str());

      if (!lib_port) {
        continue;
      }

      if (lib_port->isolationCellData()) {
        tmp_data_term = term;
      }

      if (lib_port->isolationCellEnable()) {
        tmp_enable_term = term;
        tmp_enable_lib_port = lib_port;
      }

      if (term->getIoType() == odb::dbIoType::OUTPUT) {
        tmp_output_term = term;
        tmp_out_lib_port = lib_port;
      }
    }

    if (!tmp_output_term || !tmp_data_term || !tmp_enable_term) {
      // Isolation cell defined, but can't find one of output, data or enable
      // terms.
      continue;
    }

    // Determine if an inverter is needed for the control pin & output
    bool tmp_invert_output = false;
    bool tmp_invert_control = false;
    bool matched = check_isolation_match(tmp_out_lib_port->function(),
                                         tmp_enable_lib_port,
                                         isolation_sense,
                                         isolation_clamp_val,
                                         logger,
                                         tmp_invert_control,
                                         tmp_invert_output);
    if (!matched) {
      continue;
    }

    // Update the smallest_area
    float tmp_area = liberty_cell->area();
    if (tmp_invert_control || tmp_invert_output) {
      if (!inverter_m) {
        continue;
      }

      if (tmp_invert_control) {
        tmp_area += inverter_area;
      }
      if (tmp_invert_output) {
        tmp_area += inverter_area;
      }
    }

    if (tmp_area < smallest_area) {
      smallest_area = tmp_area;
      smallest_iso_m = iso_cell;
      enable_term = tmp_enable_term;
      data_term = tmp_data_term;
      output_term = tmp_output_term;
      invert_control = tmp_invert_control;
      invert_output = tmp_invert_output;
    }
  }

  if (smallest_area == std::numeric_limits<float>::max()) {
    logger->warn(utl::UPF,
                 25,
                 "Isolation {} cells defined, but can't find any in the lib.",
                 iso->getName());
    return false;
  }
  return true;
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

  auto isolation_inst = block->findInst(inst_name.c_str());
  if (isolation_inst) {
    return true;
  }

  isolation_inst
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

    if (target_group) {
      target_group->addInst(inv_inst);
    }
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

    if (target_group) {
      target_group->addInst(inv_inst);
    }
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
                         odb::dbITerm* target_iterm,
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
    logger->warn(
        utl::UPF, 71, "Isolation {} has no net connected", iso->getName());

    return false;
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
    odb::dbGroup* target_group = target_iterm->getInst()->getGroup();
    std::string net_out_name
        = net->getName() + "_" + target_iterm->getMTerm()->getName() + "_o";
    auto net_out = block->findNet(net_out_name.c_str());

    if (!net_out) {
      net_out = odb::dbNet::create(block, net_out_name.c_str());
    }

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

    target_iterm->disconnect();
    target_iterm->connect(net_out);

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
                   "Isolation {} has location {}, but only self|parent|fanout"
                   "supported, defaulting to self.",
                   iso->getName(),
                   iso->getLocation());
      target_group = pd->getGroup();
    }

    std::string net_out_name = net->getName() + "_o";
    auto net_out = block->findNet(net_out_name.c_str());

    if (!net_out) {
      net_out = odb::dbNet::create(block, net_out_name.c_str());
    }

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

    target_iterm->disconnect();
    target_iterm->connect(net_out);
  }

  return true;
}

// Returns all connected iterms that are not in the same power domain
static std::vector<std::pair<odb::dbITerm*, odb::dbPowerDomain*>>
get_connected_terms(odb::dbBlock* block, odb::dbITerm* iterm)
{
  std::vector<std::pair<odb::dbITerm*, odb::dbPowerDomain*>> external_iterms;
  auto net = iterm->getNet();

  if (!net) {
    return external_iterms;
  }

  auto connected_iterms = net->getITerms();

  if (connected_iterms.size() < 2) {
    return external_iterms;
  }

  for (auto&& connected_iterm : connected_iterms) {
    if (connected_iterm == iterm) {
      continue;
    }

    auto connected_inst = connected_iterm->getInst();
    // TODO: if connected_inst is isolation cell or level shifting cell
    // then ignore it

    if (connected_inst->getGroup() != iterm->getInst()->getGroup()) {
      odb::dbPowerDomain* connected_domain = nullptr;
      if (connected_inst->getGroup()) {
        connected_domain
            = block->findPowerDomain(connected_inst->getGroup()->getName());
      }
      external_iterms.emplace_back(connected_iterm, connected_domain);
    }
  }

  return external_iterms;
}

static bool isolate_connection(odb::dbITerm* src_term,
                               odb::dbITerm* target_term,
                               odb::dbPowerDomain* domain,
                               odb::dbBlock* block,
                               utl::Logger* logger,
                               sta::dbNetwork* network)
{
  odb::dbInst* src_inst = src_term->getInst();

  auto isos = domain->getIsolations();
  if (isos.empty()) {
    return false;
  }
  if (isos.size() > 1) {
    logger->warn(
        utl::UPF,
        30,
        "Multiple isolation strategies defined for the same power domain {}.",
        domain->getName());
  }

  odb::dbIsolation* iso = isos[0];
  if (!iso->appliesTo(src_term->getIoType())) {
    return false;
  }

  // find the smallest possible inverter in advance
  odb::dbMaster* inverter_m = nullptr;
  odb::dbMTerm *input_m = nullptr, *output_m = nullptr;
  bool inverter_found
      = find_smallest_inverter(network, block, inverter_m, input_m, output_m);

  if (!inverter_found) {
    logger->warn(utl::UPF, 31, "can't find any inverters");
  }

  odb::dbMTerm* enable_term = nullptr;
  odb::dbMTerm* data_term = nullptr;
  odb::dbMTerm* output_term = nullptr;
  odb::dbMaster* smallest_iso_m = nullptr;
  bool invert_output, invert_control;

  if (!find_smallest_isolation(network,
                               logger,
                               iso,
                               inverter_m,
                               smallest_iso_m,
                               enable_term,
                               data_term,
                               output_term,
                               invert_output,
                               invert_control)) {
    return false;
  }

  return isolate_port(logger,
                      block,
                      src_inst,
                      src_term,
                      target_term,
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

static odb::dbLevelShifter* find_shift_strategy(odb::dbBlock* block,
                                                odb::dbPowerDomain* domain,
                                                odb::dbITerm* iterm)
{
  odb::dbInst* inst = iterm->getInst();
  odb::dbLevelShifter* found_strategy = nullptr;
  bool no_shift = false;
  auto shifters = domain->getLevelShifters();

  for (auto&& shifter : shifters) {
    auto els = shifter->getElements();
    bool found = els.empty();
    bool excluded = false;
    auto excluded_els = shifter->getExcludeElements();

    // look for the instance name in the elements list
    for (auto&& el : els) {
      if (el == inst->getName() || el == ".") {
        found = true;
        break;
      }
    }

    // look for the instance name in the excluded elements list
    for (auto&& el : excluded_els) {
      if (el == inst->getName() || el == ".") {
        excluded = true;
        break;
      }
    }

    // if instance is not in the excluded list and is in the elements list
    // and force shift is true, then we have to shift
    if (shifter->isForceShift() && (found && !excluded)) {
      return shifter;
    }

    // if instance is found in a strategy and no_shift is true, then we cannot
    // shift we wait until in the end, in the case a strategy with force shift
    // appears
    if (found && shifter->isNoShift()) {
      no_shift = true;
    }

    // if stategy.no_shift is false and no_shift is false and instance is found
    // then we have found a strategy, but we delay until in the end
    if ((found && !excluded) && !shifter->isNoShift() && !no_shift) {
      found_strategy = shifter;
    }
  }

  return (no_shift) ? nullptr : found_strategy;
}

static bool validate_shifting_strategy(odb::dbBlock* block,
                                       utl::Logger* logger,
                                       odb::dbLevelShifter* strategy,
                                       odb::dbITerm* src_term,
                                       odb::dbPowerDomain* src_domain,
                                       odb::dbITerm* target_term,
                                       odb::dbPowerDomain* target_domain)
{
  float src_voltage = src_domain->getVoltage();
  float target_voltage = target_domain->getVoltage();

  // In this case there's missing information about one of the domain's voltage
  if (src_voltage == 0 || target_voltage == 0) {
    logger->warn(utl::UPF,
                 53,
                 "Missing voltage information for one of the domains {} {}",
                 src_domain->getName(),
                 target_domain->getName());
    return false;
  }
  bool low_to_high = (src_voltage < target_voltage);
  bool high_to_low = (src_voltage > target_voltage);

  // in case of input port, we have to invert the low_to_high and high_to_low
  if (src_term->getIoType() == odb::dbIoType::INPUT) {
    low_to_high = !low_to_high;
    high_to_low = !high_to_low;
  }

  float difference = std::abs(src_voltage - target_voltage);

  if (low_to_high && strategy->getRule() != "low_to_high"
      && strategy->getRule() != "both") {
    return false;
  }

  if (high_to_low && strategy->getRule() != "high_to_low"
      && strategy->getRule() != "both") {
    return false;
  }

  if (difference < strategy->getThreshold()) {
    return false;
  }

  if (src_term->getIoType() == odb::dbIoType::INPUT
      && strategy->getAppliesTo() != "inputs"
      && strategy->getAppliesTo() != "both") {
    return false;
  }

  if (src_term->getIoType() == odb::dbIoType::OUTPUT
      && strategy->getAppliesTo() != "outputs"
      && strategy->getAppliesTo() != "both") {
    return false;
  }

  return true;
}

static bool insert_level_shifter(utl::Logger* logger,
                                 odb::dbBlock* block,
                                 odb::dbInst* inst,
                                 odb::dbITerm* iterm,
                                 odb::dbITerm* target_iterm,
                                 odb::dbPowerDomain* pd,
                                 odb::dbPowerDomain* target_domain,
                                 odb::dbLevelShifter* shift)
{
  auto net = iterm->getNet();

  // find dbMaster and create dbInst from shift.cell_name
  odb::dbMaster* shifter_master
      = block->getDataBase()->findMaster(shift->getCellName().c_str());
  if (!shifter_master) {
    logger->warn(utl::UPF,
                 54,
                 "Can't find master {} for level shifter {}",
                 shift->getCellName(),
                 shift->getName());
    return false;
  }

  // create dbInst
  std::string inst_name = iterm->getName('_') + "_" + target_iterm->getName('_')
                          + "_level_shifter";

  odb::dbInst* shifter_inst
      = odb::dbInst::create(block, shifter_master, inst_name.c_str());
  odb::dbITerm* input_term
      = shifter_inst->findITerm(shift->getCellInput().c_str());
  odb::dbITerm* output_term
      = shifter_inst->findITerm(shift->getCellOutput().c_str());

  if (!input_term || !output_term) {
    logger->warn(utl::UPF,
                 55,
                 "Can't find input or output term for level shifter {} based "
                 "on strategy specified input {} output {}",
                 shift->getName(),
                 shift->getCellInput(),
                 shift->getCellOutput());
    return false;
  }

  odb::dbGroup* target_group = nullptr;

  if (shift->getLocation() == "parent") {
    auto ppd = pd->getParent();
    // if the parent domain is the top
    // domain, don't add to any group
    if (ppd && ppd->getGroup()) {
      target_group = ppd->getGroup();
    }
  } else if (shift->getLocation() == "self") {
    target_group = pd->getGroup();
  } else if (shift->getLocation() == "fanout") {
    target_group = target_domain->getGroup();
  } else {
    logger->warn(utl::UPF,
                 35,
                 "Level Shifting strategy {} has location {}, but only "
                 "self|parent|fanout"
                 "supported, defaulting to self.",
                 shift->getName(),
                 shift->getLocation());
    target_group = pd->getGroup();
  }

  // add the level shifter to the target group
  target_group->addInst(shifter_inst);

  std::string net_out_name = inst_name + "_out_net";
  auto net_out = block->findNet(net_out_name.c_str());

  if (!net_out) {
    net_out = odb::dbNet::create(block, net_out_name.c_str());
  }

  input_term->connect(net);
  output_term->connect(net_out);
  if (iterm->getIoType() == odb::dbIoType::INPUT) {
    // if the terminal in question is of type input
    // we take disconnect the iterm
    // and connect it to the output of the level shifter
    iterm->disconnect();
    iterm->connect(net_out);
  } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
    // if the terminal in question is of type output
    // we take disconnect the target iterm
    // and connect it to the output of the level shifter
    target_iterm->disconnect();
    target_iterm->connect(net_out);
  } else {
    logger->warn(
        utl::UPF,
        56,
        "Level Shifting strategy {} has unknown io type, but only input|output"
        "supported.",
        shift->getName());
    return false;
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
  if (pds.empty()) {  // No power domains defined
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

  // get all cell names for level shifter
  // TODO: should be replaced later by querying lib to find if inst is level
  // shifter
  std::unordered_map<std::string, bool> level_shifter_cells;
  auto shifters = block->getLevelShifters();
  for (auto&& shifter : shifters) {
    level_shifter_cells[shifter->getCellName()] = true;
  }

  for (auto&& domain : pds) {
    if (domain == top_domain) {
      continue;
    }

    odb::dbGroup* curr_group = domain->getGroup();
    auto insts = curr_group->getInsts();

    for (auto&& inst : insts) {
      if (level_shifter_cells[inst->getMaster()->getName()]) {
        continue;
      }
      auto iterms = inst->getITerms();
      for (auto&& iterm : iterms) {
        // get all connected iterms as well
        auto connected_iterms = get_connected_terms(block, iterm);
        for (auto&& connected_iterm : connected_iterms) {
          odb::dbITerm* target_iterm = connected_iterm.first;
          odb::dbPowerDomain* target_domain = connected_iterm.second;

          // if target instance is a level shifter then skip
          if (level_shifter_cells
                  [target_iterm->getInst()->getMaster()->getName()]) {
            continue;
          }

          // if both domains have same voltage then isolate
          if (!target_domain
              || domain->getVoltage() == target_domain->getVoltage()) {
            isolate_connection(
                iterm, target_iterm, domain, block, logger, network);
            continue;
          }

          odb::dbLevelShifter* strategy
              = find_shift_strategy(block, domain, iterm);

          if (!strategy) {
            continue;
          }

          // check if strategy could be insterted between the two ports
          bool should_shift = validate_shifting_strategy(block,
                                                         logger,
                                                         strategy,
                                                         iterm,
                                                         domain,
                                                         target_iterm,
                                                         target_domain);

          if (!should_shift) {
            continue;
          }

          // insert level shifter between the two ports
          insert_level_shifter(logger,
                               block,
                               inst,
                               iterm,
                               target_iterm,
                               domain,
                               target_domain,
                               strategy);
        }
      }
    }
  }

  return true;
}

bool create_or_update_level_shifter(
    utl::Logger* logger,
    odb::dbBlock* block,
    const std::string& name,
    const std::string& domain,
    const std::string& source,
    const std::string& sink,
    const std::string& use_functional_equivalence,
    const std::string& applies_to,
    const std::string& applies_to_boundary,
    const std::string& rule,
    const std::string& threshold,
    const std::string& no_shift,
    const std::string& force_shift,
    const std::string& location,
    const std::string& input_supply,
    const std::string& output_supply,
    const std::string& internal_supply,
    const std::string& name_prefix,
    const std::string& name_suffix,
    bool update)
{
  odb::dbLevelShifter* ls = nullptr;
  odb::dbPowerDomain* pd = block->findPowerDomain(domain.c_str());

  if (update) {
    ls = block->findLevelShifter(name.c_str());
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
    ls = odb::dbLevelShifter::create(block, name.c_str(), pd);
    if (ls == nullptr) {
      logger->warn(utl::UPF, 44, "Couldn't create level shifter {}", name);
      return false;
    }
  }

  if (!source.empty()) {
    ls->setSource(source);
  }

  if (!sink.empty()) {
    ls->setSink(sink);
  }

  if (!applies_to.empty()) {
    ls->setAppliesTo(applies_to);
  }

  if (!applies_to_boundary.empty()) {
    ls->setAppliesToBoundary(applies_to_boundary);
  }

  if (!rule.empty()) {
    ls->setRule(rule);
  }

  if (!location.empty()) {
    ls->setLocation(location);
  }

  if (!input_supply.empty()) {
    ls->setInputSupply(input_supply);
  }

  if (!output_supply.empty()) {
    ls->setOutputSupply(output_supply);
  }

  if (!internal_supply.empty()) {
    ls->setInternalSupply(internal_supply);
  }

  if (!name_prefix.empty()) {
    ls->setNamePrefix(name_prefix);
  }

  if (!name_suffix.empty()) {
    ls->setNameSuffix(name_suffix);
  }

  if (!threshold.empty()) {
    ls->setThreshold(std::stof(threshold));
  }

  if (!no_shift.empty()) {
    ls->setNoShift(std::stoi(no_shift));
  }

  if (!force_shift.empty()) {
    ls->setForceShift(std::stoi(force_shift));
  }

  std::string upper_case_func_eqv = use_functional_equivalence;
  for (auto& c : upper_case_func_eqv) {
    c = toupper(c);
  }
  ls->setUseFunctionalEquivalence(upper_case_func_eqv == "TRUE" ? true : false);

  return true;
}

bool add_level_shifter_element(utl::Logger* logger,
                               odb::dbBlock* block,
                               const std::string& level_shifter_name,
                               const std::string& element)
{
  odb::dbLevelShifter* ls = block->findLevelShifter(level_shifter_name.c_str());
  if (ls == nullptr) {
    logger->warn(utl::UPF,
                 39,
                 "Couldn't find level shifter {} to add element {}",
                 level_shifter_name,
                 element);
    return false;
  }

  ls->addElement(element);

  return true;
}

bool exclude_level_shifter_element(utl::Logger* logger,
                                   odb::dbBlock* block,
                                   const std::string& level_shifter_name,
                                   const std::string& exclude_element)
{
  odb::dbLevelShifter* ls = block->findLevelShifter(level_shifter_name.c_str());
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
                                   const std::string& level_shifter_name,
                                   const std::string& instance_name,
                                   const std::string& port_name)
{
  odb::dbLevelShifter* ls = block->findLevelShifter(level_shifter_name.c_str());
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

bool set_domain_voltage(utl::Logger* logger,
                        odb::dbBlock* block,
                        const std::string& domain,
                        float voltage)
{
  odb::dbPowerDomain* pd = block->findPowerDomain(domain.c_str());
  if (pd == nullptr) {
    logger->warn(utl::UPF,
                 59,
                 "Couldn't find power domain {} to set voltage {}",
                 domain,
                 voltage);
    return false;
  }

  pd->setVoltage(voltage);

  return true;
}

bool set_level_shifter_cell(utl::Logger* logger,
                            odb::dbBlock* block,
                            const std::string& shifter,
                            const std::string& cell,
                            const std::string& input,
                            const std::string& ouput)

{
  odb::dbLevelShifter* ls = block->findLevelShifter(shifter.c_str());
  if (ls == nullptr) {
    logger->warn(utl::UPF,
                 60,
                 "Couldn't find level shifter {} to set cell {}",
                 shifter,
                 cell);
    return false;
  }

  ls->setCellName(cell);
  ls->setCellInput(input);
  ls->setCellOutput(ouput);

  return true;
}

void write_upf(utl::Logger* logger,
               odb::dbBlock* block,
               const std::string& file)
{
  upf::UPFWriter writer(block, logger);
  writer.write(file);
}

}  // namespace upf
