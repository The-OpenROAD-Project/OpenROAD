// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "aig/aig/aig.h"
#include "base/abc/abc.h"
#include "cut/abc_library_factory.h"
#include "cut/logic_cut.h"
#include "cut/logic_extractor.h"
#include "extended_technology_mapping.h"
#include "map/mio/mio.h"
#include "mockturtle/algorithms/emap.hpp"
#include "mockturtle/io/genlib_reader.hpp"
#include "mockturtle/networks/aig.hpp"
#include "mockturtle/networks/block.hpp"
#include "mockturtle/utils/name_utils.hpp"
#include "mockturtle/utils/tech_library.hpp"
#include "mockturtle/views/cell_view.hpp"
#include "mockturtle/views/names_view.hpp"
#include "mockturtle/views/topo_view.hpp"
#include "ord/OpenRoad.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Search.hh"
#include "utl/Logger.h"

namespace abc {
Vec_Str_t* Abc_SclProduceGenlibStrSimple(SC_Lib* p);
}  // namespace abc

namespace rmp {

namespace {

std::tuple<mockturtle::names_view<mockturtle::aig_network>, cut::LogicCut>
extract_logic_to_mockturtle(sta::dbSta* sta,
                            sta::Corner* corner,
                            rsz::Resizer* resizer,
                            mockturtle::tech_library<9u>& tech_lib,
                            utl::Logger* logger)
{
  using abc::Abc_Ntk_t;
  using abc::Abc_NtkDelete;
  using abc::Aig_Man_t;
  using abc::Aig_Obj_t;
  using abc::Mio_Library_t;

  using aig_ntk = mockturtle::names_view<mockturtle::aig_network>;

  sta::dbNetwork* network = sta->getDbNetwork();

  // Disable incremental timing.
  sta->graphDelayCalc()->delaysInvalid();
  sta->search()->arrivalsInvalid();
  sta->search()->endpointsInvalid();

  cut::LogicExtractorFactory logic_extractor(sta, logger);
  for (sta::Vertex* endpoint : *sta->endpoints()) {
    logic_extractor.AppendEndpoint(endpoint);
  }

  cut::LogicCut cut = logic_extractor.BuildLogicCut(tech_lib);

  aig_ntk ntk = mockturtle::names_view(
      cut.BuildMappedMockturtleNetwork(tech_lib, network, logger));

  return {ntk, cut};
}

using BlockNtk
    = mockturtle::names_view<mockturtle::cell_view<mockturtle::block_network>>;

// Mapping info for one cell instance
struct CellMapping
{
  std::string master_name;              // dbMaster / Liberty cell name
  std::vector<std::string> input_pins;  // in fanin order (from gate::pins)
};

std::vector<odb::dbMTerm*> getSignalOutputs(odb::dbMaster* master)
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

CellMapping map_cell_from_standard_cell(const BlockNtk& ntk,
                                        const BlockNtk::node& n,
                                        utl::Logger* logger)
{
  CellMapping m;

  const auto& sc = ntk.get_cell(n);  // cell_view API
  m.master_name = sc.name;           // must match Liberty/LEF cell name

  if (sc.gates.empty()) {
    throw std::runtime_error(
        std::format("Standard cell {} has no gates (node {})",
                    sc.name,
                    ntk.node_to_index(n)));
  }

  const auto& first_gate = sc.gates.front();
  for (const auto& p : first_gate.pins) {
    m.input_pins.push_back(p.name);
  }

  return m;
}

void import_mockturtle_mapped_network(sta::dbSta* sta,
                                      const BlockNtk& ntk,
                                      const odb::dbSet<odb::dbLib>& libs,
                                      cut::LogicCut& cut,
                                      utl::Logger* logger)
{
  auto topo_ntk = mockturtle::topo_view(ntk);

  odb::dbChip* chip = sta->db()->getChip();
  odb::dbBlock* block = chip->getBlock();

  // Delete nets that only belong to the cut set.
  sta::NetSet nets_to_be_deleted(sta->getDbNetwork());
  std::unordered_set<sta::Net*> primary_input_or_output_nets;

  for (sta::Net* net : cut.primary_inputs()) {
    primary_input_or_output_nets.insert(net);
  }
  for (sta::Net* net : cut.primary_outputs()) {
    primary_input_or_output_nets.insert(net);
  }

  for (const sta::Instance* instance : cut.cut_instances()) {
    auto pin_iterator = std::unique_ptr<sta::InstancePinIterator>(
        sta->getDbNetwork()->pinIterator(instance));
    while (pin_iterator->hasNext()) {
      sta::Pin* pin = pin_iterator->next();
      sta::Net* connected_net = sta->getDbNetwork()->net(pin);
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
    sta->getDbNetwork()->deleteInstance(const_cast<sta::Instance*>(instance));
  }

  for (const sta::Net* net : nets_to_be_deleted) {
    sta->getDbNetwork()->deleteNet(const_cast<sta::Net*>(net));
  }

  // Add mapped network into design
  const auto num_nodes = topo_ntk.size();
  std::vector<std::vector<odb::dbNet*>> node_out_nets(num_nodes);

  // Const nets (for constant fanins)
  odb::dbNet* const0_net = nullptr;
  odb::dbNet* const1_net = nullptr;

  auto ensure_const_net = [&](bool value) -> odb::dbNet* {
    const char* net_name = value ? "CONST1" : "CONST0";
    odb::dbNet*& cache = value ? const1_net : const0_net;

    if (cache) {
      return cache;
    }

    odb::dbNet* net = block->findNet(net_name);
    if (!net) {
      net = odb::dbNet::create(block, net_name);
    }
    if (!net) {
      throw std::runtime_error(
          std::format("Failed to create or find const net {}", net_name));
    }

    cache = net;
    return net;
  };

  auto node_index = [&](const BlockNtk::node& n) -> auto {
    return topo_ntk.node_to_index(n);
  };

  // Primary inputs: nets + BTerms
  {
    topo_ntk.foreach_pi([&](const BlockNtk::node& n) {
      const auto idx = node_index(n);
      std::string name = ntk.get_name(topo_ntk.make_signal(n));

      odb::dbNet* net = block->findNet(name.c_str());
      if (!net) {
        throw std::runtime_error(std::format("Failed to find net {}", name));
      }

      node_out_nets[idx].resize(1);
      node_out_nets[idx][0] = net;
    });
  }

  // Gates: create instances + output nets (no inputs connected yet)
  topo_ntk.foreach_gate([&](const BlockNtk::node& n) {
    const auto idx = node_index(n);

    CellMapping mapping = map_cell_from_standard_cell(topo_ntk, n, logger);
    odb::dbMaster* master;
    for (const auto& lib : libs) {
      master = lib->findMaster(mapping.master_name.c_str());
      if (master) {
        break;
      }
    }
    if (!master) {
      throw std::runtime_error(
          std::format("Cannot find master {}", mapping.master_name));
    }

    std::string inst_name = std::format("n_{}", idx);
    odb::dbInst* inst = odb::dbInst::create(block, master, inst_name.c_str());
    if (!inst) {
      throw std::runtime_error(
          std::format("Failed to create dbInst {} for master {}",
                      inst_name,
                      mapping.master_name));
    }

    auto out_mterms = getSignalOutputs(master);
    const uint32_t num_cell_outputs = static_cast<uint32_t>(out_mterms.size());
    const uint32_t num_node_outputs = topo_ntk.num_outputs(n);

    if (num_node_outputs == 0) {
      // Shouldn't happen for mapped logic; skip
      return;
    }

    if (num_cell_outputs < num_node_outputs) {
      throw std::runtime_error(std::format(
          "Cell {} has only {} signal outputs but node {} has {} outputs",
          mapping.master_name,
          num_cell_outputs,
          idx,
          num_node_outputs));
    }

    node_out_nets[idx].resize(num_node_outputs);

    for (uint32_t out_pin_idx = 0; out_pin_idx < num_node_outputs;
         ++out_pin_idx) {
      odb::dbMTerm* o_mterm = out_mterms[out_pin_idx];
      const std::string pin_name = o_mterm->getName();

      odb::dbITerm* o_iterm = inst->findITerm(pin_name.c_str());
      if (!o_iterm) {
        throw std::runtime_error(std::format(
            "Instance {} has no output ITerm {}", inst_name, pin_name));
      }

      std::string net_name;
      topo_ntk.foreach_po([&](auto const& f, auto index) {
        if (topo_ntk.get_node(f) == n
            && topo_ntk.get_output_pin(f) == out_pin_idx) {
          net_name = ntk.get_output_name(index);
        }
      });

      if (net_name.empty()) {
        net_name = std::format("n_{}_o_{}", idx, out_pin_idx);
      }

      odb::dbNet* net = block->findNet(net_name.c_str());
      if (!net) {
        net = odb::dbNet::create(block, net_name.c_str());
      }
      if (!net) {
        throw std::runtime_error(
            std::format("Failed to create net {}", net_name));
      }

      o_iterm->connect(net);
      node_out_nets[idx][out_pin_idx] = net;
    }
  });

  topo_ntk.foreach_gate([&](const BlockNtk::node& n) {
    const auto idx = node_index(n);

    CellMapping mapping = map_cell_from_standard_cell(topo_ntk, n, logger);
    std::string inst_name = std::format("n_{}", idx);
    odb::dbInst* inst = block->findInst(inst_name.c_str());
    if (!inst) {
      throw std::runtime_error(std::format("Instance {} not found", inst_name));
    }

    uint32_t fanin_idx = 0;
    topo_ntk.foreach_fanin(n, [&](const BlockNtk::signal& f) {
      if (fanin_idx >= mapping.input_pins.size()) {
        throw std::runtime_error(std::format(
            "Not enough input pins in cell {} (node {}), fanins={} inputs={}",
            mapping.master_name,
            idx,
            fanin_idx,
            mapping.input_pins.size()));
      }

      odb::dbNet* src_net = nullptr;
      auto src_node = topo_ntk.get_node(f);

      if (topo_ntk.is_constant(src_node)) {
        // constant node; complemented = 1
        const bool value = topo_ntk.is_complemented(f);
        src_net = ensure_const_net(value);
      } else {
        const auto src_idx = node_index(src_node);
        uint32_t out_pin_idx = 0;
        if (topo_ntk.is_multioutput(src_node)) {
          out_pin_idx = topo_ntk.get_output_pin(f);
        }

        if (src_idx >= node_out_nets.size()
            || out_pin_idx >= node_out_nets[src_idx].size()
            || node_out_nets[src_idx][out_pin_idx] == nullptr) {
          throw std::runtime_error(
              std::format("Missing driver net for fanin of {} (node {})",
                          mapping.master_name,
                          idx));
        }
        src_net = node_out_nets[src_idx][out_pin_idx];
      }

      const std::string& pin_name = mapping.input_pins[fanin_idx++];
      odb::dbITerm* it = inst->findITerm(pin_name.c_str());
      if (!it) {
        throw std::runtime_error(std::format(
            "Master {} had no input ITerm {}", mapping.master_name, pin_name));
      }
      it->connect(src_net);
    });
  });
}

}  // anonymous namespace

void extended_technology_mapping(sta::dbSta* sta,
                                 odb::dbDatabase* db,
                                 sta::Corner* corner,
                                 bool map_multioutput,
                                 bool area_oriented_mapping,
                                 bool verbose,
                                 rsz::Resizer* resizer,
                                 utl::Logger* logger)
{
  // Configure emap parameters
  mockturtle::emap_params ps;

  ps.map_multioutput = map_multioutput;
  ps.verbose = verbose;

  if (area_oriented_mapping) {
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
    factory.SetCorner(corner);
    auto abc_library = factory.BuildScl();
    auto genlib_vec = abc::Abc_SclProduceGenlibStrSimple(abc_library.get());
    // ABC ends the file with '.end', but mockturtle doesn't like that
    for (int i = 0; i < sizeof(".end\n\0"); i++) {
      Vec_StrPop(genlib_vec);
    }
    Vec_StrPush(genlib_vec, '\0');
    auto genlib_str = Vec_StrArray(genlib_vec);
    std::istringstream genlib(genlib_str);

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
      = extract_logic_to_mockturtle(sta, corner, resizer, tech_lib, logger);

  // Extended technology mapping statistics
  mockturtle::emap_stats st;

  // Run emap
  auto mapped_ntk
      = mockturtle::names_view(mockturtle::emap(ntk, tech_lib, ps, &st));

  // Restore priamry IO names
  mockturtle::restore_pio_names_by_order(ntk, mapped_ntk);

  // Print statitstics
  logger->report(
      "Extended technology mapping stats:\n\tarea: {}\n\tdelay: {}\n\tpower: "
      "{}\n\tinverters: {}\n\tmultioutput gates: {}\n",
      st.area,
      st.delay,
      st.power,
      st.inverters,
      st.multioutput_gates);

  mapped_ntk.report_cells_usage();
  mapped_ntk.report_stats();

  // Import mapped network back to OpenROAD
  auto libs = db->getLibs();

  import_mockturtle_mapped_network(sta, mapped_ntk, libs, cut, logger);

  db->triggerPostReadDb();
}

}  // namespace rmp
