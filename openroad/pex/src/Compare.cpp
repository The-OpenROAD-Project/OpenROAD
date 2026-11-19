// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Compare.h"

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fsv/NetWindow.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"
#include "fsv/Stack.h"
#include "odb/db.h"

namespace pex {

namespace {

// The total and the couplings by net name that extraction left on a net.
struct Extracted
{
  double total = 0;
  std::map<std::string, double> couplings;
};

Extracted extracted(odb::dbNet* net)
{
  Extracted result;
  for (odb::dbCapNode* node : net->getCapNodes()) {
    result.total += node->getCapacitance(0);
    for (odb::dbCCSeg* cc : node->getCCSegs()) {
      uint32_t unused;
      odb::dbNet* other = cc->getTheOtherCapn(node, unused)->getNet();
      result.total += cc->getCapacitance(0);
      result.couplings[other->getName()] += cc->getCapacitance(0);
    }
  }
  return result;
}

}  // namespace

std::vector<odb::dbNet*> sampleNets(odb::dbBlock* block, const int count)
{
  std::vector<odb::dbNet*> extracted;
  for (odb::dbNet* net : block->getNets()) {
    if (net->isRCgraph()) {
      extracted.push_back(net);
    }
  }
  if (std::cmp_greater_equal(count, extracted.size())) {
    return extracted;
  }
  std::vector<odb::dbNet*> result;
  result.reserve(count);
  for (int i = 0; i < count; i++) {
    result.push_back(extracted[(i * extracted.size()) / count]);
  }
  return result;
}

std::vector<NetComparison> compareWithFsv(odb::dbBlock* block,
                                          const fsv::Stack& stack,
                                          const std::vector<odb::dbNet*>& nets,
                                          const CompareOptions& options)
{
  const fsv::LayerExtent extent = [&](const std::string_view layer) {
    const fsv::Stack::Layer l = stack.layer(layer);
    if (!l.valid()) {
      throw std::invalid_argument("the stack has no layer "
                                  + std::string(layer));
    }
    const fsv::Stack::ZExtent z = stack.zExtent(l);
    return std::make_pair(z.bottom, z.top);
  };
  fsv::SolverOptions solver_options;
  solver_options.threads = options.threads;
  fsv::Solver solver(solver_options);
  fsv::SolveOptions solve_options;
  solve_options.tol_total = options.tol;
  std::vector<NetComparison> result;
  for (odb::dbNet* net : nets) {
    if (!net->isRCgraph()) {
      throw std::invalid_argument("net " + net->getName()
                                  + " has no parasitics");
    }
    const fsv::NetWindow window = fsv::readNetWindow(
        block, {net}, extent, {.halo = options.halo, .margin = -1});
    const fsv::Scene scene(window.geometry, stack);
    const int master = window.targets.front();
    const fsv::CapRow row = solver.solve(scene, master, solve_options);
    const Extracted pex = extracted(net);
    NetComparison c;
    c.net = net->getName();
    c.fsv_total = row.total();
    c.fsv_sigma = row.sigma[master];
    c.pex_total = pex.total;
    for (int n = 0; n < window.geometry.netCount(); n++) {
      if (n == master || window.geometry.grounded(n)
          || row.coupling(n) < 0.05 * row.total()) {
        continue;
      }
      const std::string& name = window.geometry.netName(n);
      odb::dbNet* other = block->findNet(name.c_str());
      if (other == nullptr || other->getSigType().isSupply()) {
        continue;
      }
      const auto it = pex.couplings.find(name);
      c.couplings.push_back({
          .net = name,
          .fsv = row.coupling(n),
          .pex = it == pex.couplings.end() ? 0 : it->second,
      });
    }
    result.push_back(std::move(c));
  }
  return result;
}

}  // namespace pex
