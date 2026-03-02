// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "extended_technology_mapping.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>

#include "aig/aig/aig.h"
#include "base/abc/abc.h"
#include "cut/abc_library_factory.h"
#include "cut/logic_cut.h"
#include "cut/logic_extractor.h"
#include "db_sta/dbSta.hh"
#include "lorina/common.hpp"
#include "lorina/diagnostics.hpp"
#include "map/scl/sclLib.h"
#include "misc/vec/vecStr.h"
#include "mockturtle/algorithms/emap.hpp"
#include "mockturtle/io/genlib_reader.hpp"
#include "mockturtle/networks/aig.hpp"
#include "mockturtle/networks/block.hpp"
#include "mockturtle/utils/name_utils.hpp"
#include "mockturtle/utils/tech_library.hpp"
#include "mockturtle/views/names_view.hpp"
#include "mockturtle/views/topo_view.hpp"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "ord/OpenRoad.hh"
#include "sta/ConcreteLibrary.hh"
#include "sta/FuncExpr.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Search.hh"
#include "utl/Logger.h"

namespace abc {
Vec_Str_t* Abc_SclProduceGenlibStr(SC_Lib* p,
                                   float Slew,
                                   float Gain,
                                   int nGatesMin,
                                   int fUseAll,
                                   int* pnCellCount);
}  // namespace abc

namespace rmp {

std::tuple<mockturtle::names_view<mockturtle::aig_network>, cut::LogicCut>
ExtendedTechnologyMapping::extractLogicToMockturtle(
    sta::dbSta* sta,
    sta::Scene* scene,
    rsz::Resizer* resizer,
    mockturtle::tech_library<9u>& tech_lib,
    utl::Logger* logger)
{
  using abc::Abc_Ntk_t;
  using abc::Abc_NtkDelete;
  using abc::Aig_Man_t;
  using abc::Aig_Obj_t;

  using aig_ntk = mockturtle::names_view<mockturtle::aig_network>;

  sta::dbNetwork* network = sta->getDbNetwork();

  // Disable incremental timing.
  sta->graphDelayCalc()->delaysInvalid();
  sta->search()->arrivalsInvalid();
  sta->search()->endpointsInvalid();

  cut::LogicExtractorFactory logic_extractor(sta, logger);
  for (sta::Vertex* endpoint : sta->endpoints()) {
    logic_extractor.AppendEndpoint(endpoint);
  }

  cut::LogicCut cut = logic_extractor.BuildLogicCut(tech_lib);

  aig_ntk ntk = mockturtle::names_view(
      cut.BuildMappedMockturtleNetwork(tech_lib, network, logger));

  return {ntk, cut};
}

std::vector<odb::dbMTerm*> ExtendedTechnologyMapping::getSignalOutputs(
    odb::dbMaster* master)
{
  std::vector<odb::dbMTerm*> outs;

  for (auto* mterm : master->getMTerms()) {
    auto io = mterm->getIoType();
    auto sig = mterm->getSigType();

    if (sig == odb::dbSigType::POWER || sig == odb::dbSigType::GROUND) {
      continue;
    }
    if (io == odb::dbIoType::INPUT) {
      continue;
    }
    // Accept OUTPUT, INOUT, FEEDTHRU here as "outputs"
    outs.push_back(mterm);
  }

  auto by_name = [](odb::dbMTerm* a, odb::dbMTerm* b) {
    return std::strcmp(a->getName().c_str(), b->getName().c_str()) < 0;
  };
  std::sort(outs.begin(), outs.end(), by_name);

  return outs;
}

ExtendedTechnologyMapping::CellMapping
ExtendedTechnologyMapping::mapCellFromStdCell(const BlockNtk& ntk,
                                              const BlockNtk::node& n,
                                              utl::Logger* logger)
{
  CellMapping m;

  const auto& sc = ntk.get_cell(n);  // cell_view API
  m.master_name = sc.name;           // must match Liberty/LEF cell name

  if (sc.gates.empty()) {
    logger->error(utl::RMP,
                  66,
                  "Standard cell {} has no gates (node {})",
                  sc.name,
                  ntk.node_to_index(n));
  }

  const auto& first_gate = sc.gates.front();
  for (const auto& p : first_gate.pins) {
    m.input_pins.push_back(p.name);
  }

  return m;
}

std::optional<const sta::LibertyCell*>
ExtendedTechnologyMapping::findLibertyCellByMasterName(
    sta::Sta* sta,
    const std::string& cell_name)
{
  auto* libs = sta->network()->libertyLibraryIterator();
  while (libs->hasNext()) {
    auto lib = libs->next();
    if (auto* cell = lib->findCell(cell_name.c_str())) {
      return cell->libertyCell();
    }
  }
  return std::nullopt;
}

std::optional<ExtendedTechnologyMapping::TieMaster>
ExtendedTechnologyMapping::findTieMaster(const odb::dbSet<odb::dbLib>& libs,
                                         sta::dbSta* sta,
                                         bool value)
{
  std::vector<TieMaster> candidates;

  for (const auto& lib : libs) {
    for (auto* master : lib->getMasters()) {
      int sig_in = 0, sig_out = 0;
      std::string out_name;

      for (auto* mt : master->getMTerms()) {
        auto sig = mt->getSigType();
        if (sig == odb::dbSigType::POWER || sig == odb::dbSigType::GROUND) {
          continue;
        }
        switch (mt->getIoType().getValue()) {
          case odb::dbIoType::INPUT:
            sig_in++;
            break;
          case odb::dbIoType::OUTPUT:
            sig_out++;
            out_name = mt->getName();
            break;
          default:
            // ignore INOUT/FEEDTHRU
            break;
        }
      }

      if (sig_in != 0 || sig_out != 1) {
        continue;
      }

      // likely a constant cell
      TieMaster tm = {.master = master, .out_pin = out_name};

      auto lib_cell = findLibertyCellByMasterName(sta, master->getName());
      if (!lib_cell.has_value()) {
        continue;
      }
      auto* port_it = lib_cell.value()->portIterator();
      while (port_it->hasNext()) {
        auto* port = port_it->next()->libertyPort();
        if (!port->direction()->isOutput()) {
          continue;
        }
        auto* func = port->function();
        if ((value && func->op() == sta::FuncExpr::Op::one)
            || (!value && func->op() == sta::FuncExpr::Op::zero)) {
          return tm;
        }
      }
    }
  }

  return std::nullopt;
}

odb::dbNet* ExtendedTechnologyMapping::ensureConstNet(
    bool value,
    odb::dbBlock* block,
    const odb::dbSet<odb::dbLib>& libs,
    sta::dbSta* sta,
    utl::Logger* logger)
{
  odb::dbNet*& net = value ? net1_cache_ : net0_cache_;

  if (net) {
    return net;
  }

  const char* name = value ? "CONST1" : "CONST0";

  net = block->findNet(name);
  if (!net) {
    net = odb::dbNet::create(block, name);
  }
  if (!net) {
    logger->error(utl::RMP, 67, "Failed to create const net");
  }

  auto tie = findTieMaster(libs, sta, value);
  if (!tie) {
    logger->error(
        utl::RMP,
        68,
        "No supply net and no tie cell found; cannot create constant driver");
  }

  std::string inst_name = value ? "u_const1" : "u_const0";
  odb::dbInst* inst = block->findInst(inst_name.c_str());
  if (!inst) {
    inst = odb::dbInst::create(block, tie->master, inst_name.c_str());
  }
  if (!inst) {
    logger->error(utl::RMP, 69, "Failed to create tie inst");
  }

  auto* out_it = inst->findITerm(tie->out_pin.c_str());
  if (!out_it) {
    logger->error(utl::RMP, 70, "Tie output iterm not found");
  }
  out_it->connect(net);

  if (value) {
    net1_cache_ = net;
  } else {
    net0_cache_ = net;
  }

  return net;
}

void ExtendedTechnologyMapping::importMockturtleMappedNetwork(
    sta::dbSta* sta,
    const BlockNtk& ntk,
    const odb::dbSet<odb::dbLib>& libs,
    cut::LogicCut& cut,
    utl::Logger* logger)
{
  auto topo_ntk = mockturtle::topo_view(ntk);

  odb::dbChip* chip = sta->db()->getChip();
  odb::dbBlock* block = chip->getBlock();
  sta::dbNetwork* db_network = sta->getDbNetwork();

  // Delete nets that only belong to the cut set.
  sta::NetSet nets_to_be_deleted(db_network);
  std::unordered_set<sta::Net*> primary_input_or_output_nets;

  for (sta::Net* net : cut.primary_inputs()) {
    primary_input_or_output_nets.insert(net);
  }
  for (sta::Net* net : cut.primary_outputs()) {
    primary_input_or_output_nets.insert(net);
  }

  for (const sta::Instance* instance : cut.cut_instances()) {
    auto pin_iterator = std::unique_ptr<sta::InstancePinIterator>(
        db_network->pinIterator(instance));
    while (pin_iterator->hasNext()) {
      sta::Pin* pin = pin_iterator->next();
      sta::Net* connected_net = db_network->net(pin);
      if (connected_net == nullptr) {
        // This net is not connected to anything, so we cannot delete it.
        // This can happen if you have an unconnected output port.
        // For example one of Sky130's tie cell has both high and low outputs
        // and only one is connected to a net.
        continue;
      }
      // If pin isn't a primary input or output add to deleted list. The only
      // way this can happen is if a net is only used within the cutset, and
      // in that case we want to delete it.
      if (!primary_input_or_output_nets.contains(connected_net)) {
        nets_to_be_deleted.insert(connected_net);
      }
    }
  }

  for (const sta::Instance* instance : cut.cut_instances()) {
    db_network->deleteInstance(const_cast<sta::Instance*>(instance));
  }
  for (const sta::Net* net : nets_to_be_deleted) {
    db_network->deleteNet(const_cast<sta::Net*>(net));
  }

  // Add mapped network into design
  const auto num_nodes = topo_ntk.size();
  std::vector<std::vector<odb::dbNet*>> node_out_nets(num_nodes);

  auto node_index = [&](const BlockNtk::node& n) -> uint32_t {
    return topo_ntk.node_to_index(n);
  };

  auto node_output_signal = [&](const BlockNtk::node& n, uint32_t k) {
    auto s = topo_ntk.make_signal(n);  // output pin 0
    for (uint32_t i = 0; i < k; ++i) {
      s = topo_ntk.next_output_pin(s);
    }
    return s;
  };

  // Primary inputs
  topo_ntk.foreach_pi([&](const BlockNtk::node& n) {
    const auto idx = node_index(n);

    const std::string name = ntk.get_name(topo_ntk.make_signal(n));
    if (name.empty()) {
      logger->error(utl::RMP, 71, "PI node {} has empty name", idx);
    }

    odb::dbNet* net = block->findNet(name.c_str());
    if (!net) {
      logger->error(utl::RMP, 72, "Failed to find PI net {}", name);
    }

    node_out_nets[idx].resize(1);
    node_out_nets[idx][0] = net;
  });

  // Gates: create instances + output nets (no inputs connected yet)
  topo_ntk.foreach_gate([&](const BlockNtk::node& n) {
    const auto idx = node_index(n);

    CellMapping mapping = mapCellFromStdCell(topo_ntk, n, logger);
    odb::dbMaster* master = nullptr;
    for (const auto& lib : libs) {
      master = lib->findMaster(mapping.master_name.c_str());
      if (master) {
        break;
      }
    }
    if (!master) {
      logger->error(utl::RMP, 73, "Cannot find master {}", mapping.master_name);
    }

    const std::string inst_name = fmt::format("n_{}", idx);
    odb::dbInst* inst = odb::dbInst::create(block, master, inst_name.c_str());
    if (!inst) {
      logger->error(utl::RMP,
                    74,
                    "Failed to create dbInst {} for master {}",
                    inst_name,
                    mapping.master_name);
    }

    const auto out_mterms = getSignalOutputs(master);
    const uint32_t num_cell_outputs = static_cast<uint32_t>(out_mterms.size());
    const uint32_t num_node_outputs = topo_ntk.num_outputs(n);

    if (num_node_outputs == 0) {
      // Shouldn't happen for mapped logic; skip
      return;
    }

    if (num_cell_outputs < num_node_outputs) {
      logger->error(
          utl::RMP,
          75,
          "Cell {} has only {} signal outputs but node {} has {} outputs",
          mapping.master_name,
          num_cell_outputs,
          idx,
          num_node_outputs);
    }

    node_out_nets[idx].resize(num_node_outputs);

    for (uint32_t out_pin_idx = 0; out_pin_idx < num_node_outputs;
         ++out_pin_idx) {
      odb::dbMTerm* o_mterm = out_mterms[out_pin_idx];
      const std::string pin_name = o_mterm->getName();

      odb::dbITerm* o_iterm = inst->findITerm(pin_name.c_str());
      if (!o_iterm) {
        logger->error(utl::RMP,
                      76,
                      "Instance {} has no output ITerm {}",
                      inst_name,
                      pin_name);
      }

      std::string net_name;
      {
        auto out_sig = node_output_signal(n, out_pin_idx);
        if (ntk.has_name(out_sig)) {
          net_name = ntk.get_name(out_sig);
        }
      }
      if (net_name.empty()) {
        net_name = fmt::format("n_{}_o_{}", idx, out_pin_idx);
      }

      odb::dbNet* net = block->findNet(net_name.c_str());
      if (!net) {
        net = odb::dbNet::create(block, net_name.c_str());
      }
      if (!net) {
        logger->error(utl::RMP, 77, "Failed to create/find net {}", net_name);
      }

      o_iterm->connect(net);
      node_out_nets[idx][out_pin_idx] = net;
    }
  });

  auto get_constant_value = [topo_ntk, logger](const BlockNtk::node &src_node, const BlockNtk::signal &f) {
    auto gate = topo_ntk.get_cell(src_node).gates[topo_ntk.get_output_pin(f)];
    bool value;
    if (gate.expression == "CONST1") {
      value = true;
    } else if (gate.expression == "CONST0") {
      value = false;
    } else {
      logger->error(utl::RMP, 78, "Unknown constant expression: {}", gate.expression);
    }
    return value ^ topo_ntk.is_complemented(f);
  };

  // Connect gate inputs to driver nets
  topo_ntk.foreach_gate([&](const BlockNtk::node& n) {
    const auto idx = node_index(n);

    CellMapping mapping = mapCellFromStdCell(topo_ntk, n, logger);

    const std::string inst_name = fmt::format("n_{}", idx);
    odb::dbInst* inst = block->findInst(inst_name.c_str());
    if (!inst) {
      logger->error(utl::RMP, 79, "Instance {} not found", inst_name);
    }

    uint32_t fanin_idx = 0;
    topo_ntk.foreach_fanin(n, [&](const BlockNtk::signal& f) {
      if (fanin_idx >= mapping.input_pins.size()) {
        logger->error(
            utl::RMP,
            80,
            "Not enough input pins in cell {} (node {}), fanins={} inputs={}",
            mapping.master_name,
            idx,
            fanin_idx,
            mapping.input_pins.size());
      }

      odb::dbNet* src_net = nullptr;
      auto src_node = topo_ntk.get_node(f);

      if (topo_ntk.is_constant(src_node)) {
        bool value = get_constant_value(src_node, f);
        src_net = ensureConstNet(value, block, libs, sta, logger);
      } else {
        const auto src_idx = node_index(src_node);
        uint32_t out_pin_idx = 0;
        if (topo_ntk.is_multioutput(src_node)) {
          out_pin_idx = topo_ntk.get_output_pin(f);
        }

        if (src_idx >= node_out_nets.size()
            || out_pin_idx >= node_out_nets[src_idx].size()
            || node_out_nets[src_idx][out_pin_idx] == nullptr) {
          logger->error(utl::RMP,
                        81,
                        "Missing driver net for fanin of {} (node {})",
                        mapping.master_name,
                        idx);
        }
        src_net = node_out_nets[src_idx][out_pin_idx];
      }

      const std::string& pin_name = mapping.input_pins[fanin_idx++];
      odb::dbITerm* it = inst->findITerm(pin_name.c_str());
      if (!it) {
        logger->error(utl::RMP,
                      82,
                      "Master {} had no input ITerm {}",
                      mapping.master_name,
                      pin_name);
      }
      it->connect(src_net);
    });
  });

  // Connect mapped POs to the existing cut boundary nets
  std::vector<odb::dbNet*> boundary_po_dbnets;
  boundary_po_dbnets.reserve(cut.primary_outputs().size());
  for (sta::Net* sta_net : cut.primary_outputs()) {
    odb::dbNet* db_net = db_network->staToDb(sta_net);
    if (!db_net) {
      logger->error(utl::RMP,
                    83,
                    "cut primary output net {} has no dbNet",
                    db_network->name(sta_net));
    }
    boundary_po_dbnets.push_back(db_net);
  }

  // Connect each mapped PO driver to corresponding boundary net by merging.
  topo_ntk.foreach_po([&](const BlockNtk::signal& f, uint32_t po_index) {
    if (po_index >= boundary_po_dbnets.size()) {
      logger->error(
          utl::RMP,
          84,
          "Mapped network has more POs ({}) than cut.primary_outputs ({})",
          po_index + 1,
          boundary_po_dbnets.size());
    }

    odb::dbNet* boundary_net = boundary_po_dbnets[po_index];

    odb::dbNet* driver_net = nullptr;
    auto src_node = topo_ntk.get_node(f);
    uint32_t src_idx = 0;
    uint32_t out_pin_idx = 0;

    if (topo_ntk.is_constant(src_node)) {
      bool value = get_constant_value(src_node, f);
      driver_net = ensureConstNet(value, block, libs, sta, logger);
    } else {
      src_idx = node_index(src_node);
      if (topo_ntk.is_multioutput(src_node)) {
        out_pin_idx = topo_ntk.get_output_pin(f);
      }

      if (src_idx >= node_out_nets.size()
          || out_pin_idx >= node_out_nets[src_idx].size()
          || node_out_nets[src_idx][out_pin_idx] == nullptr) {
        logger->error(
            utl::RMP, 85, "Missing driver net for PO index {}", po_index);
      }
      driver_net = node_out_nets[src_idx][out_pin_idx];
    }

    if (driver_net && boundary_net && driver_net != boundary_net) {
      boundary_net->mergeNet(driver_net);
      node_out_nets[src_idx][out_pin_idx] = boundary_net;
    }
  });
}

void ExtendedTechnologyMapping::map(sta::dbSta* sta,
                                    odb::dbDatabase* db,
                                    rsz::Resizer* resizer,
                                    utl::Logger* logger)
{
  sta->ensureGraph();
  sta->ensureLevelized();
  sta->searchPreamble();
  for (auto mode : sta->modes()) {
    sta->ensureClkNetwork(mode);
  }

  // Configure emap parameters
  mockturtle::emap_params ps;

  ps.map_multioutput = map_multioutput_;
  ps.verbose = verbose_;

  if (area_oriented_mapping_) {
    ps.area_oriented_mapping = true;
  } else {
    ps.area_oriented_mapping = false;
    ps.relax_required = 0.0;
  }

  // Read genlib
  std::vector<mockturtle::gate> gates;
  {
    cut::AbcLibraryFactory factory(logger);
    factory.AddDbSta(sta);
    factory.AddResizer(resizer);
    factory.SetScene(scene_);
    auto abc_library = factory.BuildScl();
    auto lib = abc_library.get();
    int cell_count = 0;
    auto* genlib_vec = abc::Abc_SclProduceGenlibStr(
        lib, abc::Abc_SclComputeAverageSlew(lib), 200.0f, 0, true, &cell_count);
    // ABC ends the file with '.end', but mockturtle doesn't like that
    for (int i = 0; i < sizeof(".end\n\0"); i++) {
      abc::Vec_StrPop(genlib_vec);
    }
    abc::Vec_StrPush(genlib_vec, '\0');
    auto* genlib_str = abc::Vec_StrArray(genlib_vec);
    std::istringstream genlib(genlib_str);
    abc::Vec_StrFree(genlib_vec);

    class diagnostic_consumer : public lorina::diagnostic_consumer
    {
     public:
      diagnostic_consumer(utl::Logger* logger) : logger_(logger) {}

      void handle_diagnostic(lorina::diagnostic_level level,
                             const std::string& message) const override
      {
        switch (level) {
          case lorina::diagnostic_level::ignore:
            break;
          case lorina::diagnostic_level::note:
          case lorina::diagnostic_level::remark:
            logger_->info(utl::RMP, 424, "{}", message);
            break;
          case lorina::diagnostic_level::warning:
            logger_->warn(utl::RMP, 427, "{}", message);
            break;
          case lorina::diagnostic_level::error:
            logger_->error(utl::RMP, 430, "{}", message);
            break;
          case lorina::diagnostic_level::fatal:
            logger_->critical(utl::RMP, 433, "{}", message);
            break;
        }
      }

     private:
      utl::Logger* logger_;
    };

    diagnostic_consumer diag_consumer(logger);
    lorina::diagnostic_engine diag_engine(&diag_consumer);

    auto code = lorina::read_genlib(
        genlib, mockturtle::genlib_reader(gates), &diag_engine);

    if (code != lorina::return_code::success) {
      logger->report("Error reading genlib file");
      return;
    }
  }

  mockturtle::tech_library<9u> tech_lib(gates);

  // Create mockturtle AIG network
  auto [ntk, cut]
      = extractLogicToMockturtle(sta, scene_, resizer, tech_lib, logger);

  // Extended technology mapping statistics
  mockturtle::emap_stats st;

  // Run emap
  auto mapped_ntk
      = mockturtle::names_view(mockturtle::emap(ntk, tech_lib, ps, &st));

  // Restore priamry IO names
  mockturtle::restore_pio_names_by_order(ntk, mapped_ntk);

  // Print statitstics
  logger->report(
      "Extended technology mapping stats:\n\tarea: {:.3f}\n\tdelay: "
      "{:.3f}\n\tpower: {:.3f}\n\tinverters: {}\n\tmultioutput gates: {}\n",
      st.area,
      st.delay,
      st.power,
      st.inverters,
      st.multioutput_gates);

  mapped_ntk.report_cells_usage();
  mapped_ntk.report_stats();

  // Import mapped network back to OpenROAD
  auto libs = db->getLibs();

  // Clear const network cache
  net0_cache_ = nullptr;
  net1_cache_ = nullptr;

  importMockturtleMappedNetwork(sta, mapped_ntk, libs, cut, logger);

  db->triggerPostReadDb();
}

}  // namespace rmp
