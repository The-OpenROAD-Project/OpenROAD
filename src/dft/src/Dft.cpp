// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "dft/Dft.hh"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "ClockDomain.hh"
#include "DftConfig.hh"
#include "KMeans.hh"
#include "OdbScanCellAdapter.hh"
#include "Opt.hh"
#include "ScanArchitect.hh"
#include "ScanArchitectConfig.hh"
#include "ScanCell.hh"
#include "ScanCellFactory.hh"
#include "ScanPin.hh"
#include "ScanReplace.hh"
#include "ScanStitch.hh"
#include "boost/property_tree/json_parser.hpp"
#include "boost/property_tree/ptree.hpp"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"

namespace {
constexpr char kDefaultPartition[] = "default";
}  // namespace

namespace dft {

namespace {

// Returns the first dbScanInst in a chain, or nullptr if the chain is empty.
odb::dbScanInst* firstScanInst(odb::dbScanChain* chain)
{
  for (odb::dbScanPartition* part : chain->getScanPartitions()) {
    for (odb::dbScanList* list : part->getScanLists()) {
      for (odb::dbScanInst* si : list->getScanInsts()) {
        return si;
      }
    }
  }
  return nullptr;
}

// Returns the single dbScanList for a chain (executeDftPlan creates exactly
// one partition with one list per chain).
odb::dbScanList* getScanList(odb::dbScanChain* chain)
{
  for (odb::dbScanPartition* part : chain->getScanPartitions()) {
    for (odb::dbScanList* list : part->getScanLists()) {
      return list;
    }
  }
  return nullptr;
}

// Rewires a scan chain to match the given ordered cell list:
//   chain.scan_in → cells[0].SI
//   cells[i].SO   → cells[i+1].SI  for i in [0, n-2]
//   cells[n-1].SO → chain.scan_out  (metadata only — see note below)
//
// Safe to call multiple times (idempotent): connect() disconnects from the
// old net before attaching to the new one.
//
// Note on scan_out: the chain-level scan_out BTerm/ITerm is typically a
// functional Q output that must remain on its original net.  We only update
// the chain metadata pointer (setScanOut) rather than physically moving nets.
void RestitchChain(odb::dbScanChain* chain,
                   const std::vector<OdbScanCellAdapter*>& cells,
                   odb::dbBlock* block,
                   utl::Logger* logger)
{
  if (cells.empty()) {
    return;
  }

  // Connect chain scan_in to the first cell's SI.
  odb::dbITerm* first_si = cells.front()->getScanInITerm();
  if (first_si != nullptr) {
    std::visit(
        [&](auto&& pin) {
          if (pin != nullptr) {
            odb::dbNet* net = pin->getNet();
            if (net != nullptr) {
              first_si->connect(net);
            }
          }
        },
        chain->getScanIn());
  }

  // Wire SO[i] → SI[i+1].
  for (size_t i = 0; i + 1 < cells.size(); i++) {
    odb::dbITerm* so_iterm = cells[i]->getScanOutITerm();
    odb::dbITerm* si_iterm = cells[i + 1]->getScanInITerm();
    if (so_iterm == nullptr || si_iterm == nullptr) {
      continue;
    }
    odb::dbNet* net = so_iterm->getNet();
    if (net == nullptr) {
      net = odb::dbNet::create(block, so_iterm->getName().c_str());
      if (net == nullptr) {
        logger->error(
            utl::DFT, 15, "Failed to create net for scan_opt restitching.");
      }
      net->setSigType(odb::dbSigType::SCAN);
      so_iterm->connect(net);
    }
    si_iterm->connect(net);
  }

  // Update chain scan_out metadata to the last cell's SO ITerm.
  odb::dbITerm* last_so = cells.back()->getScanOutITerm();
  if (last_so != nullptr) {
    chain->setScanOut(last_so);
  }
}

}  // namespace

Dft::Dft(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger)
    : db_(db),
      sta_(sta),
      logger_(logger),
      dft_config_(std::make_unique<DftConfig>())
{
}

Dft::~Dft() = default;

void Dft::reset()
{
  scan_replace_.reset();
  need_to_run_pre_dft_ = true;
}

void Dft::pre_dft()
{
  scan_replace_ = std::make_unique<ScanReplace>(db_, sta_, logger_);
  scan_replace_->collectScanCellAvailable();

  // This should always be at the end
  need_to_run_pre_dft_ = false;
}

void Dft::reportDftPlan(bool verbose)
{
  if (need_to_run_pre_dft_) {
    pre_dft();
  }

  std::vector<std::unique_ptr<ScanChain>> scan_chains = scanArchitect();

  logger_->report("***************************");
  logger_->report("Report DFT Plan");
  logger_->report("Number of chains: {:d}", scan_chains.size());
  logger_->report("Clock domain: {:s}",
                  ScanArchitectConfig::ClockMixingName(
                      dft_config_->getScanArchitectConfig().getClockMixing()));
  logger_->report("***************************\n");
  for (const auto& scan_chain : scan_chains) {
    scan_chain->report(logger_, verbose);
  }
  logger_->report("");
}

void Dft::scanReplace()
{
  if (need_to_run_pre_dft_) {
    pre_dft();
  }
  scan_replace_->scanReplace();
}

void Dft::writeToOdb()
{
  odb::dbBlock* db_block = db_->getChip()->getBlock();
  odb::dbDft* db_dft = db_block->getDft();

  db_dft->reset();

  for (const auto& chain : scan_chains_) {
    odb::dbScanChain* db_sc = odb::dbScanChain::create(db_dft);
    db_sc->setName(chain->getName());
    odb::dbScanPartition* db_part = odb::dbScanPartition::create(db_sc);
    db_part->setName(kDefaultPartition);
    odb::dbScanList* db_scanlist = odb::dbScanList::create(db_part);

    for (const auto& scan_cell : chain->getScanCells()) {
      std::string inst_name(scan_cell->getName());
      odb::dbInst* db_inst = db_block->findInst(inst_name.c_str());
      odb::dbScanInst* db_scaninst = db_scanlist->add(db_inst);
      db_scaninst->setBits(scan_cell->getBits());
      ScanLoad scan_enable = scan_cell->getScanEnable();
      std::visit([&](auto&& pin) { db_scaninst->setScanEnable(pin); },
                 scan_enable.getValue());
      auto scan_in_term = scan_cell->getScanIn().getValue();
      auto scan_out_term = scan_cell->getScanOut().getValue();
      db_scaninst->setAccessPins(
          {.scan_in = scan_in_term, .scan_out = scan_out_term});

      const ClockDomain& clock_domain = scan_cell->getClockDomain();
      db_scaninst->setScanClock(clock_domain.getClockName());
      switch (clock_domain.getClockEdge()) {
        case ClockEdge::Rising:
          db_scaninst->setClockEdge(odb::dbScanInst::ClockEdge::Rising);
          break;
        case ClockEdge::Falling:
          db_scaninst->setClockEdge(odb::dbScanInst::ClockEdge::Falling);
          break;
      }
    }

    std::optional<ScanDriver> sc_enable_driver = chain->getScanEnable();
    std::optional<ScanDriver> sc_in_driver = chain->getScanIn();
    std::optional<ScanLoad> sc_out_load = chain->getScanOut();

    if (sc_enable_driver.has_value()) {
      std::visit(
          [&](auto&& sc_enable_term) { db_sc->setScanEnable(sc_enable_term); },
          sc_enable_driver.value().getValue());
    }
    if (sc_in_driver.has_value()) {
      std::visit([&](auto&& sc_in_term) { db_sc->setScanIn(sc_in_term); },
                 sc_in_driver.value().getValue());
    }
    if (sc_out_load.has_value()) {
      std::visit([&](auto&& sc_out_term) { db_sc->setScanOut(sc_out_term); },
                 sc_out_load.value().getValue());
    }
  }

  db_dft->setScanInserted(true);
}

void Dft::executeDftPlan()
{
  if (need_to_run_pre_dft_) {
    pre_dft();
  }

  scan_chains_ = scanArchitect();

  ScanStitch stitch(db_, logger_, dft_config_->getScanStitchConfig());
  stitch.Stitch(scan_chains_);

  writeToOdb();
}

DftConfig* Dft::getMutableDftConfig()
{
  return dft_config_.get();
}

const DftConfig& Dft::getDftConfig() const
{
  return *dft_config_;
}

void Dft::reportDftConfig() const
{
  logger_->report("DFT Config:");
  dft_config_->report(logger_);
}

std::vector<std::unique_ptr<ScanChain>> Dft::scanArchitect()
{
  std::vector<std::unique_ptr<ScanCell>> scan_cells
      = CollectScanCells(db_, sta_, logger_);

  // Scan Architect
  std::unique_ptr<ScanCellsBucket> scan_cells_bucket
      = std::make_unique<ScanCellsBucket>(logger_);
  scan_cells_bucket->init(dft_config_->getScanArchitectConfig(), scan_cells);

  std::unique_ptr<ScanArchitect> scan_architect
      = ScanArchitect::ConstructScanScanArchitect(
          dft_config_->getScanArchitectConfig(),
          std::move(scan_cells_bucket),
          logger_);
  scan_architect->init();
  scan_architect->architect();

  return scan_architect->getScanChains();
}

namespace {

// Half-perimeter of the bounding box of a set of points.  O(n).
int64_t computeHPWL(const std::vector<odb::Point>& pts)
{
  if (pts.empty()) {
    return 0;
  }
  int xmin = pts[0].x(), xmax = pts[0].x();
  int ymin = pts[0].y(), ymax = pts[0].y();
  for (const auto& p : pts) {
    xmin = std::min(xmin, p.x());
    xmax = std::max(xmax, p.x());
    ymin = std::min(ymin, p.y());
    ymax = std::max(ymax, p.y());
  }
  return static_cast<int64_t>(xmax - xmin)
         + static_cast<int64_t>(ymax - ymin);
}

// Sum of pairwise Manhattan distances.  O(n²).
int64_t computeSumPairwise(const std::vector<odb::Point>& pts)
{
  int64_t sum = 0;
  const int n = static_cast<int>(pts.size());
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      sum += std::abs(static_cast<int64_t>(pts[i].x()) - pts[j].x())
             + std::abs(static_cast<int64_t>(pts[i].y()) - pts[j].y());
    }
  }
  return sum;
}

// Manhattan-distance MST length via Prim's algorithm.  O(n²).
// Lower bound on the optimal tour over the same point set.
int64_t computeMST(const std::vector<odb::Point>& pts)
{
  const int n = static_cast<int>(pts.size());
  if (n < 2) {
    return 0;
  }
  std::vector<int64_t> min_dist(n, std::numeric_limits<int64_t>::max());
  std::vector<bool> in_tree(n, false);
  min_dist[0] = 0;
  int64_t total = 0;

  for (int iter = 0; iter < n; iter++) {
    int u = -1;
    int64_t best = std::numeric_limits<int64_t>::max();
    for (int v = 0; v < n; v++) {
      if (!in_tree[v] && min_dist[v] < best) {
        best = min_dist[v];
        u = v;
      }
    }
    if (u == -1) {
      break;
    }
    in_tree[u] = true;
    total += min_dist[u];

    for (int v = 0; v < n; v++) {
      if (!in_tree[v]) {
        const int64_t d
            = std::abs(static_cast<int64_t>(pts[u].x()) - pts[v].x())
              + std::abs(static_cast<int64_t>(pts[u].y()) - pts[v].y());
        if (d < min_dist[v]) {
          min_dist[v] = d;
        }
      }
    }
  }
  return total;
}

// Collects cell origins from every scan inst on the chain.
std::vector<odb::Point> collectChainOrigins(odb::dbScanChain* chain)
{
  std::vector<odb::Point> pts;
  for (odb::dbScanPartition* part : chain->getScanPartitions()) {
    for (odb::dbScanList* list : part->getScanLists()) {
      for (odb::dbScanInst* si : list->getScanInsts()) {
        pts.push_back(si->getInst()->getOrigin());
      }
    }
  }
  return pts;
}

}  // namespace

void Dft::scanOpt()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbDft* db_dft = block->getDft();

  // ---------------------------------------------------------------------------
  // Spatial pre-clustering: reassign scan cells between chains of the same
  // clock domain so that each chain holds a spatially compact set of cells.
  // This runs before per-chain optimization; it updates both dbScanList
  // membership and SI→SO nets via RestitchChain.
  // ---------------------------------------------------------------------------
  {
    // Group chains by (clock_name, clock_edge).  Only chains with the same
    // domain can share cells.
    using DomainKey = std::pair<std::string, odb::dbScanInst::ClockEdge>;
    std::map<DomainKey, std::vector<odb::dbScanChain*>> domain_chains;
    for (odb::dbScanChain* chain : db_dft->getScanChains()) {
      odb::dbScanInst* si = firstScanInst(chain);
      if (si == nullptr) {
        continue;
      }
      DomainKey key{std::string(si->getScanClock()), si->getClockEdge()};
      domain_chains[key].push_back(chain);
    }

    for (auto& [key, chains] : domain_chains) {
      const int k = static_cast<int>(chains.size());
      if (k < 2) {
        continue;
      }

      // Collect all scan insts and adapters across chains in this domain.
      // The adapter provides isPlaced() and getOrigin() for k-means.
      // The dbScanInst* is needed for insertAtFront reassignment.
      std::vector<std::unique_ptr<OdbScanCellAdapter>> adapters;
      std::vector<odb::dbScanInst*> scan_insts;
      for (odb::dbScanChain* chain : chains) {
        for (odb::dbScanPartition* part : chain->getScanPartitions()) {
          for (odb::dbScanList* list : part->getScanLists()) {
            for (odb::dbScanInst* si : list->getScanInsts()) {
              adapters.push_back(
                  std::make_unique<OdbScanCellAdapter>(si, logger_));
              scan_insts.push_back(si);
            }
          }
        }
      }

      // Skip this domain if any cell is unplaced.
      const bool all_placed
          = std::all_of(adapters.begin(), adapters.end(), [](const auto& a) {
              return a->isPlaced();
            });
      if (!all_placed) {
        continue;
      }

      // Run k-means.
      std::vector<ScanCell*> cell_ptrs;
      cell_ptrs.reserve(adapters.size());
      for (const auto& a : adapters) {
        cell_ptrs.push_back(a.get());
      }

      // Re-derive the architect's effective per-chain bit cap from the
      // existing chain composition.  ScanArchitect tightens the user's
      // max_length to ceil(total_bits / num_chains); the realised maximum
      // chain bit count after construction equals that cap.  Stage A
      // must not exceed it, otherwise the architect's max_length contract
      // is silently broken.
      int64_t cap_bits = 0;
      for (odb::dbScanChain* chain : chains) {
        int64_t chain_bits = 0;
        for (odb::dbScanPartition* part : chain->getScanPartitions()) {
          for (odb::dbScanList* list : part->getScanLists()) {
            for (odb::dbScanInst* si : list->getScanInsts()) {
              chain_bits += static_cast<int64_t>(si->getBits());
            }
          }
        }
        cap_bits = std::max(cap_bits, chain_bits);
      }
      // Sanity: a single oversized cell must always fit somewhere.
      for (const auto& a : adapters) {
        cap_bits = std::max(cap_bits, static_cast<int64_t>(a->getBits()));
      }

      const std::vector<int> assignments
          = KMeansClusters(cell_ptrs, k, cap_bits);

      // Partition scan_insts and adapters by cluster index.
      std::vector<std::vector<odb::dbScanInst*>> cluster_insts(k);
      std::vector<std::vector<OdbScanCellAdapter*>> cluster_adapters(k);
      for (size_t i = 0; i < scan_insts.size(); i++) {
        cluster_insts[assignments[i]].push_back(scan_insts[i]);
        cluster_adapters[assignments[i]].push_back(adapters[i].get());
      }

      // Reassign: for each chain c, clear its scan list, repopulate with the
      // cells assigned to cluster c, then restitch nets.
      //
      // insertAtFront prepends, so inserting in natural order produces a list
      // traversed in reverse.  The per-chain loop below calls std::reverse on
      // collected cells (matching the existing executeDftPlan convention), so
      // the cells arrive at OptimizeScanWirelength in natural order.
      for (int c = 0; c < k; c++) {
        odb::dbScanList* list = getScanList(chains[c]);
        if (list == nullptr) {
          continue;
        }
        list->clear();
        for (odb::dbScanInst* si : cluster_insts[c]) {
          si->insertAtFront(list);
        }
        // Rewire SI→SO nets for the new cell ordering in this chain.
        RestitchChain(chains[c], cluster_adapters[c], block, logger_);
      }

      logger_->info(utl::DFT,
                    19,
                    "K-means spatial pre-clustering: reassigned {} cells "
                    "across {} chains (bit cap = {}).",
                    adapters.size(),
                    k,
                    cap_bits);
    }
  }

  int chains_optimized = 0;
  for (odb::dbScanChain* chain : db_dft->getScanChains()) {
    // Collect all scan instances from this chain.
    std::vector<std::unique_ptr<ScanCell>> cells;
    for (odb::dbScanPartition* part : chain->getScanPartitions()) {
      for (odb::dbScanList* list : part->getScanLists()) {
        for (odb::dbScanInst* si : list->getScanInsts()) {
          cells.push_back(
              std::make_unique<OdbScanCellAdapter>(si, logger_));
        }
      }
    }
    // dbScanList::add() uses insertAtFront, so iteration order from odb is
    // reversed relative to how executeDftPlan inserted them.  Reverse to
    // recover the original chain order before optimising.
    std::reverse(cells.begin(), cells.end());

    if (cells.size() < 2) {
      continue;
    }

    // Record the original order to detect if anything changed.
    std::vector<std::string> original_order;
    original_order.reserve(cells.size());
    for (const auto& c : cells) {
      original_order.push_back(std::string(c->getName()));
    }

    // Run the wirelength optimizer.
    OptimizeScanWirelength(cells, logger_);

    // Check if the order actually changed.
    bool order_changed = false;
    for (size_t i = 0; i < cells.size(); i++) {
      if (std::string(cells[i]->getName()) != original_order[i]) {
        order_changed = true;
        break;
      }
    }

    if (!order_changed) {
      continue;
    }

    // Restitch: rewire SI→SO nets to match the optimised cell order.
    std::vector<OdbScanCellAdapter*> ordered_adapters;
    ordered_adapters.reserve(cells.size());
    for (const auto& c : cells) {
      ordered_adapters.push_back(static_cast<OdbScanCellAdapter*>(c.get()));
    }
    RestitchChain(chain, ordered_adapters, block, logger_);

    chains_optimized++;
  }

  logger_->info(
      utl::DFT, 16, "Optimized {} scan chain(s).", chains_optimized);
}

void Dft::reportChainMetrics()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbDft* db_dft = block->getDft();

  // Header (CSV-friendly for sweep collectors).
  logger_->report("chain_name,num_cells,hpwl,mst,sum_pairwise");

  int64_t total_hpwl = 0;
  int64_t total_mst = 0;
  int64_t total_pairwise = 0;
  int total_cells = 0;
  int chain_count = 0;

  for (odb::dbScanChain* chain : db_dft->getScanChains()) {
    const std::vector<odb::Point> pts = collectChainOrigins(chain);
    if (pts.empty()) {
      continue;
    }
    const int64_t hpwl = computeHPWL(pts);
    const int64_t mst = computeMST(pts);
    const int64_t pw = computeSumPairwise(pts);

    total_hpwl += hpwl;
    total_mst += mst;
    total_pairwise += pw;
    total_cells += static_cast<int>(pts.size());
    chain_count++;

    logger_->report("{},{},{},{},{}",
                    chain->getName(),
                    pts.size(),
                    hpwl,
                    mst,
                    pw);
  }

  logger_->info(utl::DFT,
                21,
                "Chain metrics: chains={}, cells={}, sum_hpwl={}, "
                "sum_mst={}, sum_pairwise={}",
                chain_count,
                total_cells,
                total_hpwl,
                total_mst,
                total_pairwise);
}

}  // namespace dft
