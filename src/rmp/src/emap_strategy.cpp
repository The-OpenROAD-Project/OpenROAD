// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "emap_strategy.h"

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
EmapStrategy::ExtractLogicToMockturtle(sta::dbSta* sta,
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

std::vector<odb::dbMTerm*> EmapStrategy::GetSignalOutputs(odb::dbMaster* master)
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
  std::stable_sort(outs.begin(), outs.end(), by_name);

  return outs;
}

EmapStrategy::CellMapping EmapStrategy::MapCellFromStdCell(
    const BlockNtk& ntk,
    const BlockNtk::node& n,
    utl::Logger* logger)
{
  CellMapping m;

  const auto& sc = ntk.get_cell(n);  // cell_view API
  m.master_name = sc.name;           // must match Liberty/LEF cell name

  if (sc.gates.empty()) {
    logger->error(utl::RMP,
                  86,
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
EmapStrategy::FindLibertyCellByMasterName(sta::Sta* sta,
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

std::optional<EmapStrategy::TieMaster> EmapStrategy::FindTieMaster(
    sta::dbSta* sta,
    bool value)
{
  for (const auto& lib : sta->db()->getLibs()) {
    for (auto* master : lib->getMasters()) {
      auto lib_cell = FindLibertyCellByMasterName(sta, master->getName());
      if (!lib_cell) {
        continue;
      }
      auto* port_it = lib_cell.value()->portIterator();
      while (port_it->hasNext()) {
        auto* port = port_it->next()->libertyPort();
        if (!port->direction()->isOutput()) {
          continue;
        }

        auto* func = port->function();
        if (!func) {
          continue;
        }

        auto* mterm = master->findMTerm(port->name().c_str());
        if (!mterm || mterm->getIoType() != odb::dbIoType::OUTPUT) {
          continue;
        }

        if ((value && func->op() == sta::FuncExpr::Op::one)
            || (!value && func->op() == sta::FuncExpr::Op::zero)) {
          return TieMaster{master, port->name()};
        }
      }
    }
  }

  return std::nullopt;
}

odb::dbNet* EmapStrategy::EnsureConstNet(bool value,
                                         odb::dbBlock* block,
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
    logger->error(utl::RMP, 87, "Failed to create const net");
  }

  auto tie = FindTieMaster(sta, value);
  if (!tie) {
    logger->error(
        utl::RMP,
        88,
        "No supply net and no tie cell found; cannot create constant driver");
  }

  std::string inst_name = value ? "u_const1" : "u_const0";
  odb::dbInst* inst = block->findInst(inst_name.c_str());
  if (!inst) {
    inst = odb::dbInst::create(block, tie->master, inst_name.c_str());
  }
  if (!inst) {
    logger->error(utl::RMP, 89, "Failed to create tie inst");
  }

  auto* out_it = inst->findITerm(tie->out_pin.c_str());
  if (!out_it) {
    logger->error(utl::RMP, 90, "Tie output iterm not found");
  }
  out_it->connect(net);

  if (value) {
    net1_cache_ = net;
  } else {
    net0_cache_ = net;
  }

  return net;
}

void EmapStrategy::ImportMockturtleMappedNetwork(sta::dbSta* sta,
                                                 const BlockNtk& ntk,
                                                 cut::LogicCut& cut,
                                                 utl::Logger* logger)
{
  auto topo_ntk = mockturtle::topo_view(ntk);

  odb::dbChip* chip = sta->db()->getChip();
  odb::dbBlock* block = chip->getBlock();
  sta::dbNetwork* db_network = sta->getDbNetwork();

  // Delete nets that only belong to the cut set.
  std::unordered_set<sta::Net*> primary_input_or_output_nets;
  primary_input_or_output_nets.insert(cut.primary_inputs().begin(),
                                      cut.primary_inputs().end());
  primary_input_or_output_nets.insert(cut.primary_outputs().begin(),
                                      cut.primary_outputs().end());

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
        db_network->deleteNet(connected_net);
      }
    }
    db_network->deleteInstance(const_cast<sta::Instance*>(instance));
  }

  // Add mapped network into design
  const auto num_nodes = topo_ntk.size();
  std::vector<std::vector<odb::dbNet*>> node_out_nets(num_nodes);

  auto node_output_signal = [&](const BlockNtk::node& n, uint32_t k) {
    auto s = topo_ntk.make_signal(n);  // output pin 0
    return mockturtle::signal<decltype(topo_ntk)>{
        s.index, s.complement, static_cast<uint64_t>(s.output) + k};
  };

  // Primary inputs
  topo_ntk.foreach_pi([&](const BlockNtk::node& n) {
    const auto idx = topo_ntk.node_to_index(n);

    const std::string name = ntk.get_name(topo_ntk.make_signal(n));
    if (name.empty()) {
      logger->error(utl::RMP, 71, "PI node {} has empty name", idx);
    }

    odb::dbNet* net = block->findNet(name.c_str());
    if (!net) {
      logger->error(utl::RMP, 72, "Failed to find PI net {}", name);
    }

    node_out_nets[idx].push_back(net);
  });

  // Gates: create instances + output nets (no inputs connected yet)
  topo_ntk.foreach_gate([&](const BlockNtk::node& n) {
    const auto idx = topo_ntk.node_to_index(n);

    CellMapping mapping = MapCellFromStdCell(topo_ntk, n, logger);
    odb::dbMaster* master = nullptr;
    for (const auto& lib : sta->db()->getLibs()) {
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

    const auto out_mterms = GetSignalOutputs(master);
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

  // Connect gate inputs to driver nets
  topo_ntk.foreach_gate([&](const BlockNtk::node& n) {
    const auto idx = topo_ntk.node_to_index(n);

    CellMapping mapping = MapCellFromStdCell(topo_ntk, n, logger);

    const std::string inst_name = fmt::format("n_{}", idx);
    odb::dbInst* inst = block->findInst(inst_name.c_str());
    if (!inst) {
      logger->error(utl::RMP, 79, "Instance {} not found", inst_name);
    }

    topo_ntk.foreach_fanin(
        n, [&](const BlockNtk::signal& f, uint32_t fanin_idx) {
          if (fanin_idx >= mapping.input_pins.size()) {
            logger->error(utl::RMP,
                          80,
                          "Not enough input pins in cell {} (node {}), "
                          "fanins={} inputs={}",
                          mapping.master_name,
                          idx,
                          fanin_idx,
                          mapping.input_pins.size());
          }

          odb::dbNet* src_net
              = GetDriverNet(topo_ntk, block, sta, logger, node_out_nets, f);
          const std::string& pin_name = mapping.input_pins[fanin_idx];
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
  std::unordered_map<std::string, odb::dbNet*> boundary_po_dbnets;
  for (sta::Net* sta_net : cut.primary_outputs()) {
    odb::dbNet* db_net = db_network->staToDb(sta_net);
    std::string net_name = db_network->name(sta_net);
    if (!db_net) {
      logger->error(
          utl::RMP, 83, "cut primary output net {} has no dbNet", net_name);
    }
    boundary_po_dbnets.emplace(net_name, db_net);
  }

  // Connect each mapped PO driver to corresponding boundary net by merging.
  topo_ntk.foreach_po([&](const BlockNtk::signal& f, uint32_t po_index) {
    std::string po_name = topo_ntk.get_output_name(po_index);
    auto it = boundary_po_dbnets.find(po_name);
    if (it == boundary_po_dbnets.end()) {
      logger->error(utl::RMP, 84, "Missing boundary net for PO {}", po_name);
    }
    odb::dbNet* boundary_net = it->second;

    odb::dbNet* driver_net
        = GetDriverNet(topo_ntk, block, sta, logger, node_out_nets, f);
    if (driver_net && boundary_net && driver_net != boundary_net) {
      driver_net->mergeNet(boundary_net);
    }
  });
}

template <typename Ntk>
odb::dbNet* EmapStrategy::GetDriverNet(
    mockturtle::topo_view<Ntk, false>& topo_ntk,
    odb::dbBlock* block,
    sta::dbSta* sta,
    utl::Logger* logger,
    std::vector<std::vector<odb::dbNet*>>& node_out_nets,
    const BlockNtk::signal& f)
{
  auto src_node = topo_ntk.get_node(f);

  if (topo_ntk.is_constant(src_node)) {
    auto get_constant_value = [topo_ntk, logger](const BlockNtk::node& src_node,
                                                 const BlockNtk::signal& f) {
      auto gate = topo_ntk.get_cell(src_node).gates[topo_ntk.get_output_pin(f)];
      bool value;
      if (gate.expression == "CONST1") {
        value = true;
      } else if (gate.expression == "CONST0") {
        value = false;
      } else {
        logger->error(
            utl::RMP, 78, "Unknown constant expression: {}", gate.expression);
      }
      return value ^ topo_ntk.is_complemented(f);
    };

    bool value = get_constant_value(src_node, f);
    return EnsureConstNet(value, block, sta, logger);
  }
  const uint32_t src_idx = topo_ntk.node_to_index(src_node);
  const uint32_t out_pin_idx
      = topo_ntk.is_multioutput(src_node) ? topo_ntk.get_output_pin(f) : 0;

  if (src_idx >= node_out_nets.size()
      || out_pin_idx >= node_out_nets[src_idx].size()
      || node_out_nets[src_idx][out_pin_idx] == nullptr) {
    logger->error(utl::RMP,
                  85,
                  "Missing driver net for src_idx={}, out_pin_idx={}",
                  src_idx,
                  out_pin_idx);
  }
  return node_out_nets[src_idx][out_pin_idx];
}

static void FilterDriverResistance(std::vector<mockturtle::gate>& gates,
                                   sta::dbSta* sta,
                                   utl::Logger* logger,
                                   double min_drive_resistance)
{
  sta::dbNetwork* const network = sta->getDbNetwork();
  std::erase_if(
      gates, [network, logger, min_drive_resistance](const auto& gate) {
        sta::LibertyCell* const cell = network->findLibertyCell(gate.name);
        if (!cell) {
          logger->info(utl::RMP, 2221, "Should have found cell {}", gate.name);
          return false;
        }
        sta::LibertyCellPortIterator port_iter(cell);
        float drive_resistance_sum = 0.0;
        size_t ports = 0;
        while (port_iter.hasNext()) {
          sta::LibertyPort* port = port_iter.next();
          drive_resistance_sum += port->driveResistance();
          ++ports;
        }
        if (ports == 0) {
          return false;
        }
        const float drive_resistance = drive_resistance_sum / ports;
        if (drive_resistance >= min_drive_resistance) {
          logger->info(utl::RMP,
                       2281,
                       "Accepting cell {} resistance={}, threshold={}",
                       gate.name,
                       drive_resistance,
                       min_drive_resistance);
        } else {
          logger->info(utl::RMP,
                       2261,
                       "Rejecting cell {} resistance={}, threshold={}",
                       gate.name,
                       drive_resistance,
                       min_drive_resistance);
        }
        return drive_resistance < min_drive_resistance;
      });

  // Fixup gate ids for mockturtle
  for (size_t i = 0; i < gates.size(); ++i) {
    gates[i].id = i;
  }
}

static void FilterDriverResistanceMax(std::vector<mockturtle::gate>& gates,
                                      sta::dbSta* sta,
                                      utl::Logger* logger,
                                      double max_drive_resistance)
{
  sta::dbNetwork* const network = sta->getDbNetwork();
  std::erase_if(
      gates, [network, logger, max_drive_resistance](const auto& gate) {
        sta::LibertyCell* const cell = network->findLibertyCell(gate.name);
        if (!cell) {
          logger->info(utl::RMP, 2231, "Should have found cell {}", gate.name);
          return false;
        }
        sta::LibertyCellPortIterator port_iter(cell);
        float drive_resistance_sum = 0.0;
        size_t ports = 0;
        while (port_iter.hasNext()) {
          sta::LibertyPort* port = port_iter.next();
          drive_resistance_sum += port->driveResistance();
          ++ports;
        }
        if (ports == 0) {
          return false;
        }
        const float drive_resistance = drive_resistance_sum / ports;
        if (drive_resistance <= max_drive_resistance) {
          logger->info(utl::RMP,
                       2241,
                       "Accepting cell {} resistance={}, threshold={}",
                       gate.name,
                       drive_resistance,
                       max_drive_resistance);
        } else {
          logger->info(utl::RMP,
                       2251,
                       "Rejecting cell {} resistance={}, threshold={}",
                       gate.name,
                       drive_resistance,
                       max_drive_resistance);
        }
        return drive_resistance >= max_drive_resistance;
      });

  // Fixup gate ids for mockturtle
  for (size_t i = 0; i < gates.size(); ++i) {
    gates[i].id = i;
  }
}

void EmapStrategy::OptimizeDesign(sta::dbSta* sta,
                                  utl::UniqueName& name_generator,
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

  ps.area_oriented_mapping = true;
  ps.create_po_buffers = create_po_buffers_;
  ps.insert_buffers = insert_buffers_;
  ps.map_multioutput = map_multioutput_;
  ps.verbose = verbose_;

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
  if (min_drive_resistance_ != 0) {
    FilterDriverResistance(gates, sta, logger, min_drive_resistance_);
  } else if (max_drive_resistance_ != 0) {
    FilterDriverResistanceMax(gates, sta, logger, max_drive_resistance_);
  }

  mockturtle::tech_library<9u> tech_lib(gates);

  // Create mockturtle AIG network
  auto [ntk, cut]
      = ExtractLogicToMockturtle(sta, scene_, resizer, tech_lib, logger);

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

  // Clear const network cache
  net0_cache_ = nullptr;
  net1_cache_ = nullptr;

  ImportMockturtleMappedNetwork(sta, mapped_ntk, cut, logger);

  sta->db()->triggerPostReadDb();
}

}  // namespace rmp
