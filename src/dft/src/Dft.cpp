// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "dft/Dft.hh"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
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
#include "rsz/Resizer.hh"
#include "utl/Logger.h"

namespace {
constexpr char kDefaultPartition[] = "default";
}  // namespace

namespace dft {

namespace {

// A scan chain that scan_opt can reorder: one partition holding one list.
struct OptChain
{
  odb::dbScanChain* chain;
  odb::dbScanList* list;
  // Ordered from scan-in to scan-out.
  std::vector<std::unique_ptr<OdbScanCellAdapter>> cells;
  // Set when rebalancing changed which cells the chain holds.
  bool members_changed = false;
};

// Returns the chain's only scan list, or nullptr if it has more than one
// partition or list.
odb::dbScanList* singleScanList(odb::dbScanChain* chain)
{
  odb::dbSet<odb::dbScanPartition> partitions = chain->getScanPartitions();
  if (partitions.size() != 1) {
    return nullptr;
  }
  odb::dbSet<odb::dbScanList> lists = (*partitions.begin())->getScanLists();
  if (lists.size() != 1) {
    return nullptr;
  }
  return *lists.begin();
}

std::optional<odb::Point> pinLocation(
    const std::variant<odb::dbBTerm*, odb::dbITerm*>& pin)
{
  const bool is_null
      = std::visit([](auto* term) { return term == nullptr; }, pin);
  if (is_null) {
    return std::nullopt;
  }
  int x = 0;
  int y = 0;
  if (!ScanPin(pin).getLocation(x, y)) {
    return std::nullopt;
  }
  return odb::Point(x, y);
}

int64_t manhattan(const odb::Point& a, const odb::Point& b)
{
  return std::abs(int64_t{a.x()} - b.x()) + std::abs(int64_t{a.y()} - b.y());
}

// Wire length of the chain from its scan-in pin through each cell's SO to
// the next cell's SI. The wire out of the last cell is not counted; scan_opt
// never moves it.
int64_t chainLength(
    const std::optional<odb::Point>& source,
    const std::vector<std::unique_ptr<OdbScanCellAdapter>>& cells)
{
  int64_t length = 0;
  if (source && !cells.empty()) {
    length += manhattan(*source, cells.front()->getScanInLocation());
  }
  for (size_t i = 0; i + 1 < cells.size(); i++) {
    length += manhattan(cells[i]->getScanOutLocation(),
                        cells[i + 1]->getScanInLocation());
  }
  return length;
}

// Reorders a chain's cells to reduce wire length. The last cell drives the
// chain's scan-out, which may be a functional port, so it stays last.
// Falling-edge cells stay ahead of rising-edge cells. Returns false, leaving
// the order alone, if the chain cannot be reordered under those rules.
bool reorderChain(std::vector<std::unique_ptr<OdbScanCellAdapter>>& cells,
                  const std::optional<odb::Point>& source,
                  utl::Logger* logger)
{
  if (cells.size() < 3) {
    return false;
  }
  using Edge = odb::dbScanInst::ClockEdge;
  std::unique_ptr<OdbScanCellAdapter> last = std::move(cells.back());
  cells.pop_back();

  std::vector<std::unique_ptr<ScanCell>> falling;
  std::vector<std::unique_ptr<ScanCell>> rising;
  for (auto& cell : cells) {
    if (cell->getScanInst()->getClockEdge() == Edge::Falling) {
      falling.push_back(std::move(cell));
    } else {
      rising.push_back(std::move(cell));
    }
  }
  const bool last_falling
      = last->getScanInst()->getClockEdge() == Edge::Falling;

  auto restore = [&](std::vector<std::unique_ptr<ScanCell>>& group) {
    for (auto& cell : group) {
      cells.emplace_back(static_cast<OdbScanCellAdapter*>(cell.release()));
    }
  };
  cells.clear();

  if (last_falling && !rising.empty()) {
    // A falling-edge cell after rising-edge ones; not a chain scan_opt built.
    restore(falling);
    restore(rising);
    cells.push_back(std::move(last));
    return false;
  }

  const odb::Point sink = last->getScanInLocation();
  OptimizeScanWirelength(
      falling,
      logger,
      source,
      rising.empty() ? std::optional<odb::Point>(sink) : std::nullopt);
  std::optional<odb::Point> rising_source = source;
  if (!falling.empty()) {
    rising_source = falling.back()->getScanOutLocation();
  }
  OptimizeScanWirelength(rising, logger, rising_source, sink);

  restore(falling);
  restore(rising);
  cells.push_back(std::move(last));
  return true;
}

// Rewires a chain to match the order of `cells`: the chain's scan-in drives
// cells[0].SI and cells[i].SO drives cells[i + 1].SI. The last cell's SO and
// the chain's scan-out are left as they are.
void restitchChain(
    odb::dbScanChain* chain,
    const std::vector<std::unique_ptr<OdbScanCellAdapter>>& cells,
    odb::dbBlock* block,
    utl::Logger* logger)
{
  if (cells.empty()) {
    return;
  }

  odb::dbITerm* first_si = cells.front()->getScanInITerm();
  if (first_si != nullptr) {
    std::visit(
        [&](auto* pin) {
          if (pin != nullptr && pin->getNet() != nullptr) {
            first_si->connect(pin->getNet());
          }
        },
        chain->getScanIn());
  }

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
}

// Rewrites the scan list so it runs from scan-in to scan-out in the order of
// `cells`.
void rewriteScanList(
    odb::dbScanList* list,
    const std::vector<std::unique_ptr<OdbScanCellAdapter>>& cells)
{
  list->clear();
  for (auto it = cells.rbegin(); it != cells.rend(); ++it) {
    (*it)->getScanInst()->insertAtFront(list);
  }
}

// Moves cells between chains of the same clock domain so each chain holds a
// spatially compact set. Each chain keeps its last cell, which drives its
// scan-out. Only chains whose cells all share one clock and edge take part.
void rebalanceChains(std::vector<OptChain>& chains, utl::Logger* logger)
{
  using DomainKey = std::pair<std::string, odb::dbScanInst::ClockEdge>;
  std::map<DomainKey, std::vector<OptChain*>> domains;
  for (OptChain& chain : chains) {
    const odb::dbScanInst* first = chain.cells.front()->getScanInst();
    const DomainKey key{first->getScanClock(), first->getClockEdge()};
    const bool uniform = std::ranges::all_of(chain.cells, [&](const auto& c) {
      const odb::dbScanInst* inst = c->getScanInst();
      return inst->getScanClock() == key.first
             && inst->getClockEdge() == key.second && c->isPlaced();
    });
    if (uniform && chain.cells.size() >= 2) {
      domains[key].push_back(&chain);
    }
  }

  for (auto& [key, members] : domains) {
    const int k = static_cast<int>(members.size());
    if (k < 2) {
      continue;
    }

    // Pool every cell but each chain's last one.
    std::vector<std::unique_ptr<OdbScanCellAdapter>> pool;
    std::vector<std::unique_ptr<OdbScanCellAdapter>> lasts;
    int64_t cap_bits = 0;
    for (OptChain* chain : members) {
      int64_t bits = 0;
      lasts.push_back(std::move(chain->cells.back()));
      chain->cells.pop_back();
      for (auto& cell : chain->cells) {
        bits += static_cast<int64_t>(cell->getBits());
        pool.push_back(std::move(cell));
      }
      chain->cells.clear();
      cap_bits = std::max(cap_bits, bits);
    }
    for (const auto& cell : pool) {
      cap_bits = std::max(cap_bits, static_cast<int64_t>(cell->getBits()));
    }

    std::vector<ScanCell*> cell_ptrs;
    cell_ptrs.reserve(pool.size());
    for (const auto& cell : pool) {
      cell_ptrs.push_back(cell.get());
    }
    const std::vector<int> assignment = KMeansClusters(cell_ptrs, k, cap_bits);

    // Give each cluster to the chain whose last cell is nearest its center.
    std::vector<int64_t> sum_x(k, 0);
    std::vector<int64_t> sum_y(k, 0);
    std::vector<int64_t> count(k, 0);
    for (size_t i = 0; i < pool.size(); i++) {
      const odb::Point p = pool[i]->getOrigin();
      sum_x[assignment[i]] += p.x();
      sum_y[assignment[i]] += p.y();
      count[assignment[i]]++;
    }
    std::vector<std::tuple<int64_t, int, int>> pairs;  // dist, cluster, chain
    for (int c = 0; c < k; c++) {
      const odb::Point center
          = count[c] ? odb::Point(sum_x[c] / count[c], sum_y[c] / count[c])
                     : lasts[c]->getOrigin();
      for (int m = 0; m < k; m++) {
        pairs.emplace_back(manhattan(center, lasts[m]->getOrigin()), c, m);
      }
    }
    std::ranges::sort(pairs);
    std::vector<int> chain_of_cluster(k, -1);
    std::vector<bool> chain_taken(k, false);
    for (const auto& [dist, c, m] : pairs) {
      if (chain_of_cluster[c] < 0 && !chain_taken[m]) {
        chain_of_cluster[c] = m;
        chain_taken[m] = true;
      }
    }

    for (size_t i = 0; i < pool.size(); i++) {
      members[chain_of_cluster[assignment[i]]]->cells.push_back(
          std::move(pool[i]));
    }
    for (int m = 0; m < k; m++) {
      members[m]->cells.push_back(std::move(lasts[m]));
      members[m]->members_changed = true;
    }

    logger->info(utl::DFT,
                 19,
                 "Rebalanced {} cells across {} chains (bit cap = {}).",
                 pool.size(),
                 k,
                 cap_bits);
  }
}

}  // namespace

Dft::Dft(odb::dbDatabase* db,
         sta::dbSta* sta,
         rsz::Resizer* resizer,
         utl::Logger* logger)
    : db_(db),
      sta_(sta),
      resizer_(resizer),
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
  scan_replace_ = std::make_unique<ScanReplace>(db_, sta_, resizer_, logger_);
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
    // add() inserts at the front; reverse so the list runs from scan-in to
    // scan-out, as the DEF reader stores it.
    odb::dbSet<odb::dbScanInst> scan_insts = db_scanlist->getScanInsts();
    if (scan_insts.reversible() && scan_insts.orderReversed()) {
      scan_insts.reverse();
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
  return static_cast<int64_t>(xmax - xmin) + static_cast<int64_t>(ymax - ymin);
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

void Dft::scanOpt(bool rebalance_chains)
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbDft* db_dft = block->getDft();

  if (db_dft->getScanChains().empty()) {
    logger_->warn(utl::DFT,
                  20,
                  "No scan chains found. Run execute_dft_plan or read a DEF "
                  "with SCANCHAINS first.");
    return;
  }

  std::vector<OptChain> chains;
  for (odb::dbScanChain* chain : db_dft->getScanChains()) {
    odb::dbScanList* list = singleScanList(chain);
    if (list == nullptr) {
      logger_->warn(utl::DFT,
                    22,
                    "Skipping scan chain {}: scan_opt only reorders chains "
                    "with one partition and one scan list.",
                    chain->getName());
      continue;
    }
    OptChain opt_chain{.chain = chain, .list = list, .cells = {}};
    bool placed = true;
    for (odb::dbScanInst* scan_inst : list->getScanInsts()) {
      opt_chain.cells.push_back(
          std::make_unique<OdbScanCellAdapter>(scan_inst, logger_));
      placed &= opt_chain.cells.back()->isPlaced();
    }
    if (!placed) {
      logger_->warn(utl::DFT,
                    23,
                    "Skipping scan chain {}: not all of its cells are placed.",
                    chain->getName());
      continue;
    }
    if (!opt_chain.cells.empty()) {
      chains.push_back(std::move(opt_chain));
    }
  }

  std::vector<int64_t> initial_length;
  initial_length.reserve(chains.size());
  for (const OptChain& chain : chains) {
    initial_length.push_back(
        chainLength(pinLocation(chain.chain->getScanIn()), chain.cells));
  }

  if (rebalance_chains) {
    rebalanceChains(chains, logger_);
  }

  int chains_changed = 0;
  int64_t total_initial = 0;
  int64_t total_final = 0;
  for (size_t i = 0; i < chains.size(); i++) {
    OptChain& chain = chains[i];
    const std::optional<odb::Point> source
        = pinLocation(chain.chain->getScanIn());

    std::vector<odb::dbScanInst*> before;
    before.reserve(chain.cells.size());
    for (const auto& cell : chain.cells) {
      before.push_back(cell->getScanInst());
    }

    reorderChain(chain.cells, source, logger_);

    bool changed = chain.members_changed;
    for (size_t j = 0; j < before.size() && !changed; j++) {
      changed = before[j] != chain.cells[j]->getScanInst();
    }
    if (changed) {
      restitchChain(chain.chain, chain.cells, block, logger_);
      rewriteScanList(chain.list, chain.cells);
      chains_changed++;
    }

    total_initial += initial_length[i];
    total_final += chainLength(source, chain.cells);
  }

  logger_->info(utl::DFT,
                16,
                "Reordered {} of {} scan chain(s); scan wire length {:.2f} um "
                "-> {:.2f} um.",
                chains_changed,
                chains.size(),
                block->dbuToMicrons(total_initial),
                block->dbuToMicrons(total_final));
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

    logger_->report(
        "{},{},{},{},{}", chain->getName(), pts.size(), hpwl, mst, pw);
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
