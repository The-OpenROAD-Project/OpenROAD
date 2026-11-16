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
  // Set when rebalancing already reordered the cells.
  bool reordered = false;
};

// Returns the chain's only scan list, or nullptr if it has more than one
// partition or list.
odb::dbScanList* singleScanList(odb::dbScanChain* chain)
{
  const odb::dbSet<odb::dbScanPartition> partitions
      = chain->getScanPartitions();
  if (partitions.size() != 1) {
    return nullptr;
  }
  const odb::dbSet<odb::dbScanList> lists
      = (*partitions.begin())->getScanLists();
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
// the order alone, if the chain cannot be reordered under those rules or the
// new order is not shorter than a current order that already follows them.
bool reorderChain(std::vector<std::unique_ptr<OdbScanCellAdapter>>& cells,
                  const std::optional<odb::Point>& source,
                  utl::Logger* logger)
{
  if (cells.size() < 3) {
    return false;
  }
  using Edge = odb::dbScanInst::ClockEdge;
  const auto is_falling = [](const std::unique_ptr<OdbScanCellAdapter>& cell) {
    return cell->getScanInst()->getClockEdge() == Edge::Falling;
  };
  // A falling-edge last cell after rising-edge ones is not a chain scan_opt
  // can order; leave it exactly as it is.
  if (is_falling(cells.back())
      && !std::all_of(cells.begin(), cells.end() - 1, is_falling)) {
    return false;
  }

  // The current order can be kept only if no falling-edge cell follows a
  // rising-edge one.
  const bool order_valid
      = std::is_partitioned(cells.begin(), cells.end() - 1, is_falling);
  const int64_t old_length = chainLength(source, cells);
  std::vector<const OdbScanCellAdapter*> old_order;
  for (const auto& cell : cells) {
    old_order.push_back(cell.get());
  }

  std::unique_ptr<OdbScanCellAdapter> last = std::move(cells.back());
  cells.pop_back();

  std::vector<std::unique_ptr<ScanCell>> falling;
  std::vector<std::unique_ptr<ScanCell>> rising;
  for (auto& cell : cells) {
    if (is_falling(cell)) {
      falling.push_back(std::move(cell));
    } else {
      rising.push_back(std::move(cell));
    }
  }
  const auto restore = [&](std::vector<std::unique_ptr<ScanCell>>& group) {
    for (auto& cell : group) {
      cells.emplace_back(static_cast<OdbScanCellAdapter*>(cell.release()));
    }
  };
  cells.clear();

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

  if (order_valid && chainLength(source, cells) >= old_length) {
    std::map<const OdbScanCellAdapter*, size_t> old_index;
    for (size_t i = 0; i < old_order.size(); i++) {
      old_index[old_order[i]] = i;
    }
    std::ranges::sort(cells, [&](const auto& a, const auto& b) {
      return old_index.at(a.get()) < old_index.at(b.get());
    });
    return false;
  }
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

const char* edgeName(const odb::dbScanInst::ClockEdge edge)
{
  return edge == odb::dbScanInst::ClockEdge::Falling ? "falling" : "rising";
}

// Moves cells between chains of the same clock domain so each chain holds a
// spatially compact set. Each chain keeps its last cell, which drives its
// scan-out, and no chain ends up with more bits than the longest one had.
// Only chains whose cells all share one clock and edge take part. A domain
// keeps its chains when rebalancing would lengthen its scan wiring. Every
// chain in a domain that was considered comes back reordered.
void rebalanceChains(std::vector<OptChain>& chains,
                     odb::dbBlock* block,
                     utl::Logger* logger)
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

  for (const auto& [key, members] : domains) {
    const int k = static_cast<int>(members.size());
    if (k < 2) {
      continue;
    }

    // Pool every cell but each chain's last one. Nothing moves until the
    // new assignment is known to fit.
    std::vector<ScanCell*> pool;
    std::vector<std::pair<int, size_t>> pool_origin;  // chain, cell index
    std::vector<int64_t> last_bits(k);
    int64_t max_chain_bits = 0;
    int64_t max_last_bits = 0;
    for (int m = 0; m < k; m++) {
      const auto& cells = members[m]->cells;
      int64_t bits = 0;
      for (const auto& cell : cells) {
        bits += static_cast<int64_t>(cell->getBits());
      }
      max_chain_bits = std::max(max_chain_bits, bits);
      last_bits[m] = static_cast<int64_t>(cells.back()->getBits());
      max_last_bits = std::max(max_last_bits, last_bits[m]);
      for (size_t i = 0; i + 1 < cells.size(); i++) {
        pool.push_back(cells[i].get());
        pool_origin.emplace_back(m, i);
      }
    }
    // Leave room for the largest last cell. k-means treats the cap as a
    // target, so the chain matching below checks the real limit.
    int64_t cap_bits = max_chain_bits - max_last_bits;
    for (const ScanCell* cell : pool) {
      cap_bits = std::max(cap_bits, static_cast<int64_t>(cell->getBits()));
    }
    const std::vector<int> assignment = KMeansClusters(pool, k, cap_bits);

    std::vector<int64_t> sum_x(k, 0);
    std::vector<int64_t> sum_y(k, 0);
    std::vector<int64_t> count(k, 0);
    std::vector<int64_t> cluster_bits(k, 0);
    for (size_t i = 0; i < pool.size(); i++) {
      const odb::Point p = pool[i]->getOrigin();
      sum_x[assignment[i]] += p.x();
      sum_y[assignment[i]] += p.y();
      count[assignment[i]]++;
      cluster_bits[assignment[i]] += static_cast<int64_t>(pool[i]->getBits());
    }

    // Give each cluster to the nearest chain (by last cell) that it fits:
    // the cluster plus the chain's own last cell may not exceed the longest
    // chain today.
    std::vector<std::tuple<int64_t, int, int>> pairs;  // dist, cluster, chain
    for (int c = 0; c < k; c++) {
      const odb::Point center
          = count[c] ? odb::Point(sum_x[c] / count[c], sum_y[c] / count[c])
                     : members[c]->cells.back()->getOrigin();
      for (int m = 0; m < k; m++) {
        if (cluster_bits[c] + last_bits[m] <= max_chain_bits) {
          pairs.emplace_back(
              manhattan(center, members[m]->cells.back()->getOrigin()), c, m);
        }
      }
    }
    std::ranges::sort(pairs);
    std::vector<int> chain_of_cluster(k, -1);
    std::vector<bool> chain_taken(k, false);
    int matched = 0;
    for (const auto& [dist, c, m] : pairs) {
      if (chain_of_cluster[c] < 0 && !chain_taken[m]) {
        chain_of_cluster[c] = m;
        chain_taken[m] = true;
        matched++;
      }
    }
    if (matched < k) {
      logger->info(utl::DFT,
                   24,
                   "Kept {} chains on clock {} ({} edge) as they are; "
                   "rebalancing would make a chain longer than {} bits.",
                   k,
                   key.first,
                   edgeName(key.second),
                   max_chain_bits);
      continue;
    }

    // Reorder both the current chains and the rebalanced ones, and keep the
    // rebalanced chains only if their scan wiring is shorter. k-means ignores
    // the fixed scan-in and scan-out ends, so it can lose.
    using Cells = std::vector<std::unique_ptr<OdbScanCellAdapter>>;
    const auto adapter = [&](const std::unique_ptr<OdbScanCellAdapter>& cell) {
      return std::make_unique<OdbScanCellAdapter>(cell->getScanInst(), logger);
    };
    std::vector<Cells> kept(k);
    std::vector<Cells> moved(k);
    for (int m = 0; m < k; m++) {
      for (const auto& cell : members[m]->cells) {
        kept[m].push_back(adapter(cell));
      }
    }
    for (size_t i = 0; i < pool.size(); i++) {
      const auto& [m, idx] = pool_origin[i];
      moved[chain_of_cluster[assignment[i]]].push_back(
          adapter(members[m]->cells[idx]));
    }
    int64_t kept_length = 0;
    int64_t moved_length = 0;
    for (int m = 0; m < k; m++) {
      moved[m].push_back(adapter(members[m]->cells.back()));
      const std::optional<odb::Point> source
          = pinLocation(members[m]->chain->getScanIn());
      reorderChain(kept[m], source, logger);
      reorderChain(moved[m], source, logger);
      kept_length += chainLength(source, kept[m]);
      moved_length += chainLength(source, moved[m]);
    }

    const bool rebalance = moved_length < kept_length;
    for (int m = 0; m < k; m++) {
      members[m]->cells = std::move(rebalance ? moved[m] : kept[m]);
      members[m]->members_changed = rebalance;
      members[m]->reordered = true;
    }
    if (!rebalance) {
      logger->info(utl::DFT,
                   25,
                   "Kept {} chains on clock {} ({} edge) as they are; "
                   "rebalancing does not shorten their scan wiring ({:.2f} um "
                   "vs {:.2f} um).",
                   k,
                   key.first,
                   edgeName(key.second),
                   block->dbuToMicrons(kept_length),
                   block->dbuToMicrons(moved_length));
      continue;
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

void Dft::scanOpt(const bool rebalance_chains)
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
    rebalanceChains(chains, block, logger_);
  }

  int chains_changed = 0;
  int64_t total_initial = 0;
  int64_t total_final = 0;
  for (size_t i = 0; i < chains.size(); i++) {
    OptChain& chain = chains[i];
    const std::optional<odb::Point> source
        = pinLocation(chain.chain->getScanIn());

    // Compare against the order in odb; rebalancing may already have
    // reordered the cells.
    std::vector<odb::dbScanInst*> before;
    for (odb::dbScanInst* scan_inst : chain.list->getScanInsts()) {
      before.push_back(scan_inst);
    }

    if (!chain.reordered) {
      reorderChain(chain.cells, source, logger_);
    }

    bool changed = chain.members_changed || before.size() != chain.cells.size();
    for (size_t j = 0; j < before.size() && !changed; j++) {
      changed = before[j] != chain.cells[j]->getScanInst();
    }
    if (changed) {
      restitchChain(chain.chain, chain.cells, block, logger_);
      rewriteScanList(chain.list, chain.cells);
      chains_changed++;
    }

    const int64_t final_length = chainLength(source, chain.cells);
    const std::string chain_name = chain.chain->getName();
    logger_->metric(fmt::format("dft__chain_twl__init__chain:{}", chain_name),
                    block->dbuToMicrons(initial_length[i]));
    logger_->metric(
        fmt::format("dft__chain_twl__post_opt__chain:{}", chain_name),
        block->dbuToMicrons(final_length));
    total_initial += initial_length[i];
    total_final += final_length;
  }
  logger_->metric("dft__chain_twl__init", block->dbuToMicrons(total_initial));
  logger_->metric("dft__chain_twl__post_opt", block->dbuToMicrons(total_final));

  logger_->info(utl::DFT,
                16,
                "Reordered {} of {} scan chain(s); scan wire length {:.2f} um "
                "-> {:.2f} um.",
                chains_changed,
                chains.size(),
                block->dbuToMicrons(total_initial),
                block->dbuToMicrons(total_final));
}

void Dft::reportChainMetrics() const
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
