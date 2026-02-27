#include "UvDRC/UvDRC.hh"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/MinMax.hh"
#include "sta/Sdc.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace uv_drc {

// LocEqual implementation
bool LocEqual::operator()(const odb::Point& loc1, const odb::Point& loc2) const
{
  return loc1.x() == loc2.x() && loc1.y() == loc2.y();
}

// LocHash implementation
size_t LocHash::operator()(const odb::Point& loc) const
{
  size_t seed = loc.x() * 31 + loc.y();
  seed = HashCombine(seed, std::hash<int>()(loc.x()));
  seed = HashCombine(seed, std::hash<int>()(loc.y()));
  return seed;
}

std::size_t LocHash::HashCombine(std::size_t seed, std::size_t value) const
{
  return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

void RCTreeNode::DebugPrint(utl::Logger* logger)
{
  logger->report("RCTreeNode DebugPrint:");
  DebugPrint(0, logger);
}

void RCTreeNode::DebugPrint(int indent, utl::Logger* logger)
{
  std::stringstream os;
  {
    std::string indent_str(indent, ' ');
    os << indent_str;
  }
  switch (type_) {
    case RCTreeNodeType::LOAD:
      os << "LOAD";
      break;
    case RCTreeNodeType::WIRE:
      os << "WIRE";
      break;
    case RCTreeNodeType::JUNCTION:
      os << "JUNCTION";
      break;
    case RCTreeNodeType::DRIVER:
      os << "DRIVER";
      break;
    default:
      os << "NONE";
      break;
  }
  os << ", (" << loc_.x() << ", " << loc_.y() << ")";
  logger->report(os.str());
  auto d_nodes = DownstreamNodes();
  for (const auto& d : d_nodes) {
    d->DebugPrint(indent + 2, logger);
  }
}

// 0: sol1 / sol2 cannot dominate each other
// 1: sol1 dominates sol2
// -1: sol2 dominates sol1
int RCTreeNode::IsDominated(BufferSolution& sol1, BufferSolution& sol2)
{
  bool sol1_better_in_all = true;
  bool sol2_better_in_all = true;

  if (sol1.cap > sol2.cap) {
    sol1_better_in_all = false;
  } else if (sol1.cap < sol2.cap) {
    sol2_better_in_all = false;
  }

  if (sol1.wire_slew > sol2.wire_slew) {
    sol1_better_in_all = false;
  } else if (sol1.wire_slew < sol2.wire_slew) {
    sol2_better_in_all = false;
  }

  if (sol1.buffer_locs.size() > sol2.buffer_locs.size()) {
    sol1_better_in_all = false;
  } else if (sol1.buffer_locs.size() < sol2.buffer_locs.size()) {
    sol2_better_in_all = false;
  }

  if (sol1.limit < sol2.limit) {
    sol1_better_in_all = false;
  } else if (sol1.limit > sol2.limit) {
    sol2_better_in_all = false;
  }

  if (sol1_better_in_all && !sol2_better_in_all) {
    return 1;
  }
  if (!sol1_better_in_all && sol2_better_in_all) {
    return -1;
  }
  return 0;
}

void RCTreeNode::AddSolutionAndEnsureDominance(
    std::vector<BufferSolution>& solutions,
    BufferSolution new_sol)
{
  // Check if the new solution is dominated by any existing solution
  for (auto it = solutions.begin(); it != solutions.end();) {
    int dom_result = IsDominated(new_sol, *it);
    if (dom_result == -1) {
      // New solution is dominated by an existing solution
      return;
    }

    if (dom_result == 1) {
      // New solution dominates an existing solution
      it = solutions.erase(it);
    } else {
      ++it;
    }
  }
  // If not dominated, add the new solution
  solutions.push_back(new_sol);
}

std::vector<BufferSolution> LoadNode::CalcBufferSolutions(
    const sta::Corner* corner,
    double unit_r,
    double unit_c,
    float max_cap,
    rsz::Resizer* resizer,
    sta::dbSta* sta,
    BufferCandidates& buffer_candidates,
    float r_drvr)
{
  if ((max_cap <= 0 || load_cap_ <= max_cap)
      && r_drvr * load_cap_ * UvDRCSlewBuffer::K_OPENROAD_SLEW_FACTOR
             <= slew_limit_) {
    return {BufferSolution{load_cap_, 0.0, slew_limit_, BufferLocMap{}}};
  }

  // Find a suitable buffer
  for (auto& buffer_cand : buffer_candidates) {
    if (buffer_cand.load_cap_limit >= load_cap_
        && buffer_cand.drive_resistance * load_cap_
                   * UvDRCSlewBuffer::K_OPENROAD_SLEW_FACTOR
               <= slew_limit_) {
      // TODO: we do not have a weight for different buffer sizes in this demo
      BufferSolution solution(buffer_cand.input_cap,
                              0.0,
                              buffer_cand.input_slew_limit,
                              BufferLocMap{{this, buffer_cand.cell}});
      return {solution};
    }
  }
  return {};  // No suitable solution.
}

std::vector<BufferSolution> WireNode::CalcBufferSolutions(
    const sta::Corner* corner,
    double unit_r,
    double unit_c,
    float max_cap,
    rsz::Resizer* resizer,
    sta::dbSta* sta,
    BufferCandidates& buffer_candidates,
    float r_drvr)
{
  // Aggregate downstream solutions
  std::vector<BufferSolution> downstream_solutions
      = downstream_->CalcBufferSolutions(corner,
                                         unit_r,
                                         unit_c,
                                         max_cap,
                                         resizer,
                                         sta,
                                         buffer_candidates,
                                         r_drvr);
  if (downstream_solutions.empty()) {  // No valid downstream solution
    return {};
  }

  auto distance = odb::Point::manhattanDistance(loc_, downstream_->Location());
  if (distance == 0) {  // no wire segment
    return downstream_solutions;
  }

  std::vector<BufferSolution> solutions{};
  auto distance_meter = resizer->dbuToMeters(distance);
  // Wire delay and slew calculation
  double wire_r = unit_r * distance_meter;
  double wire_c = unit_c * distance_meter;

  for (auto& sol : downstream_solutions) {
    double wire_slew = wire_r * (sol.cap + wire_c / 2)
                       * UvDRCSlewBuffer::K_OPENROAD_SLEW_FACTOR;
    // double wire_slew
    //     = wire_r * (sol.cap + wire_c) *
    //     UvDRCSlewBuffer::K_OPENROAD_SLEW_FACTOR;
    float total_cap = sol.cap + wire_c;
    float total_wire_slew = sol.wire_slew + wire_slew;
    if ((max_cap <= 0 || total_cap <= max_cap)
        && total_wire_slew
                   + r_drvr * total_cap
                         * UvDRCSlewBuffer::K_OPENROAD_SLEW_FACTOR
               <= sol.limit) {
      // Solution with no buffer
      BufferSolution new_sol{
          total_cap, total_wire_slew, sol.limit, sol.buffer_locs};
      AddSolutionAndEnsureDominance(solutions, new_sol);
    }

    // Buffered solution
    for (auto& buffer_cand : buffer_candidates) {
      if (buffer_cand.load_cap_limit >= total_cap
          && buffer_cand.drive_resistance * total_cap
                     * UvDRCSlewBuffer::K_OPENROAD_SLEW_FACTOR
                 <= sol.limit) {
        BufferSolution new_sol{buffer_cand.input_cap,
                               0.0,
                               buffer_cand.input_slew_limit,
                               sol.buffer_locs};
        new_sol.buffer_locs[this] = buffer_cand.cell;
        AddSolutionAndEnsureDominance(solutions, new_sol);
        break;
      }
    }
  }

  return solutions;
}

std::vector<BufferSolution> JuncNode::CalcBufferSolutions(
    const sta::Corner* corner,
    double unit_r,
    double unit_c,
    float max_cap,
    rsz::Resizer* resizer,
    sta::dbSta* sta,
    BufferCandidates& buffer_candidates,
    float r_drvr)
{
  auto child1_solutions = downstream1_->CalcBufferSolutions(
      corner, unit_r, unit_c, max_cap, resizer, sta, buffer_candidates, r_drvr);
  auto child2_solutions = downstream2_->CalcBufferSolutions(
      corner, unit_r, unit_c, max_cap, resizer, sta, buffer_candidates, r_drvr);

  if (child1_solutions.empty() || child2_solutions.empty()) {
    return {};
  }

  std::vector<BufferSolution> solutions{};
  // Combine child solutions
  for (auto& sol1 : child1_solutions) {
    for (auto& sol2 : child2_solutions) {
      float total_cap = sol1.cap + sol2.cap;
      bool choose_slew1
          = sol1.limit - sol1.wire_slew < sol2.limit - sol2.wire_slew;
      float total_wire_slew = choose_slew1 ? sol1.wire_slew : sol2.wire_slew;
      float limit = choose_slew1 ? sol1.limit : sol2.limit;
      if ((max_cap <= 0 || total_cap <= max_cap)
          && total_wire_slew
                     + r_drvr * total_cap
                           * UvDRCSlewBuffer::K_OPENROAD_SLEW_FACTOR
                 <= limit) {
        BufferLocMap combined_locs = sol1.buffer_locs;
        combined_locs.insert(sol2.buffer_locs.begin(), sol2.buffer_locs.end());
        BufferSolution new_sol{
            total_cap, total_wire_slew, limit, combined_locs};
        AddSolutionAndEnsureDominance(solutions, new_sol);
      }

      // buffered sol at JUNC's input
      for (auto& buffer_cand : buffer_candidates) {
        if (buffer_cand.load_cap_limit >= total_cap
            && buffer_cand.drive_resistance * total_cap
                       * UvDRCSlewBuffer::K_OPENROAD_SLEW_FACTOR
                   <= limit) {
          BufferLocMap combined_locs = sol1.buffer_locs;
          combined_locs.insert(sol2.buffer_locs.begin(),
                               sol2.buffer_locs.end());
          combined_locs[this] = buffer_cand.cell;
          BufferSolution new_sol{buffer_cand.input_cap,
                                 0.0,
                                 buffer_cand.input_slew_limit,
                                 combined_locs};
          AddSolutionAndEnsureDominance(solutions, new_sol);
          break;
        }
      }
    }
  }

  return solutions;
}

void UvDRCSlewBuffer::InitBufferCandidates()
{
  // TODO: In this demo, we only use the default slew/cap metrics from liberty
  // file, without considering its performance on given corner.

  if (!buffer_candidates_.empty()) {  // init done
    return;
  }

  for (auto b : resizer_->buffer_fast_sizes_) {
    sta::LibertyPort *in, *out;
    b->bufferPorts(in, out);
    float drive_resistance = out->driveResistance();

    float input_cap = in->capacitance();

    bool exists;

    float in_slew_limit = sta::INF;
    in->libertyLibrary()->defaultMaxSlew(in_slew_limit, exists);
    assert(in_slew_limit != sta::INF);

    float load_cap_limit;
    out->capacitanceLimit(sta::MinMax::max(), load_cap_limit, exists);
    assert(exists);

    BufferCandidate candidate{
        b, input_cap, drive_resistance, in_slew_limit, load_cap_limit};
    buffer_candidates_.push_back(candidate);
  }
  // Sort by input capacitance ascendingly
  std::sort(buffer_candidates_.begin(),
            buffer_candidates_.end(),
            [](const BufferCandidate& a, const BufferCandidate& b) {
              return a.input_cap < b.input_cap;
            });
}

std::tuple<LocVec, PinVec> UvDRCSlewBuffer::InitNetConnections(
    const sta::Pin* drvr_pin)
{
  auto network = resizer_->network();
  // auto sdc_network = network->sdcNetwork();
  auto db_network = resizer_->getDbNetwork();
  odb::dbNet* db_net = db_network->findFlatDbNet(drvr_pin);
  auto net = db_network->dbToSta(db_net);

  LocVec locs{};
  PinVec pins{};
  locs.push_back(db_network->location(drvr_pin));
  pins.push_back(drvr_pin);

  auto pin_iter = network->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    auto pin = pin_iter->next();
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    odb::dbModITerm* mod_iterm;
    db_network->staToDb(pin, iterm, bterm, mod_iterm);
    if ((iterm || bterm) && pin != drvr_pin) {
      odb::Point loc = db_network->location(pin);
      locs.push_back(loc);
      pins.push_back(pin);
    }
  }
  delete pin_iter;

  return {locs, pins};
}

stt::Tree UvDRCSlewBuffer::MakeSteinerTree(const sta::Pin* drvr_pin,
                                           LocVec& locs,
                                           PinVec& pins)
{
  if (locs.size() < 2) {
    throw std::runtime_error(
        "At least two locations are required to build a Steiner Tree.");
  }

  std::vector<int> x;
  std::vector<int> y;

  bool is_placed = true;
  auto db_network = resizer_->getDbNetwork();

  for (std::size_t i = 0; i != locs.size(); i++) {
    const odb::Point& loc = locs[i];
    x.push_back(loc.x());
    y.push_back(loc.y());
    is_placed &= db_network->isPlaced(pins[i]);
  }

  if (!is_placed) {
    throw std::runtime_error(
        "Buffering for fixing slew can only be called after placement.");
  }

  return resizer_->getSteinerTreeBuilder()->makeSteinerTree(
      db_network->findFlatDbNet(drvr_pin), x, y, 0);
}

// From SteinerTree to RCTree structure
RCTreeNodePtr UvDRCSlewBuffer::BuildRCTree(const sta::Pin* drvr_pin,
                                           const sta::Corner* corner,
                                           stt::Tree& tree,
                                           LocVec& locs,
                                           PinVec& pins)
{
  if (locs.size() != tree.deg) {
    throw std::runtime_error(
        "Pins count does not match the Steiner Tree degree.");
  }
  std::size_t pin_count = pins.size();
  RCTreeNodePtr root = nullptr;
  std::unordered_map<odb::Point, RCTreeNodePtr, LocHash, LocEqual>
      loc_node_map{};

  std::vector<RCTreeNodePtr> nodes;
  std::vector<std::vector<std::size_t>> adjacents(tree.branchCount(),
                                                  std::vector<std::size_t>{});

  auto network = resizer_->network();

  for (std::size_t i = 0; i != tree.branchCount(); i++) {
    if (i == 0) {
      root = std::make_shared<DrivNode>(locs[i], drvr_pin);
      nodes.push_back(root);
      continue;  // No need to check adjs for root
    }
    RCTreeNodePtr node = nullptr;
    if (i < pin_count) {
      auto load_port = network->libertyPort(pins[i]);
      if (load_port) {
        float load_cap = resizer_->portCapacitance(load_port, corner);
        float slew_limit = resizer_->maxInputSlew(load_port, corner);
        node = std::make_shared<LoadNode>(
            locs[i], pins[i], load_cap, slew_limit);
      } else if (network->isTopLevelPort(pins[i])) {
        // For top-level port without liberty port info, use default values
        float load_cap = 0.0;
        float slew_limit = sta::INF;
        sta::Port* port = network->port(pins[i]);
        for (auto rf : rsz::RiseFall::range()) {
          float pin_cap, wire_cap;
          int fanout;
          bool has_pin_cap, has_wire_cap, has_fanout;
          resizer_->sdc_->portExtCap(port,
                                     rf,
                                     corner,
                                     sta::MinMax::max(),
                                     pin_cap,
                                     has_pin_cap,
                                     wire_cap,
                                     has_wire_cap,
                                     fanout,
                                     has_fanout);
          if (has_pin_cap) {
            load_cap = std::max(load_cap, pin_cap);
          }
        }
        node = std::make_shared<LoadNode>(
            locs[i], pins[i], load_cap, slew_limit);
      } else {
        throw std::runtime_error(
            "Load pin does not have liberty port information.");
      }
    } else {
      node = std::make_shared<JuncNode>(
          odb::Point{tree.branch[i].x, tree.branch[i].y});
    }
    nodes.push_back(node);
    adjacents[tree.branch[i].n].push_back(i);
  }

  BuildRCTreeHelper(nodes, adjacents, 0);
  // root->DebugPrint(resizer_->logger());
  return root;
}

RCTreeNodePtr UvDRCSlewBuffer::BuildRCTreeHelper(
    std::vector<RCTreeNodePtr>& nodes,
    std::vector<std::vector<std::size_t>>& adjacents,
    std::size_t current_index)
{
  RCTreeNodePtr current_node = nodes[current_index];
  auto& children = adjacents[current_index];
  if (children.size() > 2) {
    throw std::runtime_error(
        "RCTreeNode cannot have more than two downstream nodes.");
  }
  if (children.empty()) {
    return current_node;
  }

  if (current_node->Type() == RCTreeNodeType::LOAD) {
    if (children.size() == 1) {
      auto new_node = std::make_shared<JuncNode>(current_node->Location());
      new_node->AddDownstreamNode(current_node);
      new_node->AddDownstreamNode(
          BuildRCTreeHelper(nodes, adjacents, children[0]));
      return new_node;
    }

    if (children.size() == 2) {
      auto new_node = std::make_shared<JuncNode>(current_node->Location());
      auto new_node2 = std::make_shared<JuncNode>(current_node->Location());
      new_node->AddDownstreamNode(current_node);
      new_node->AddDownstreamNode(new_node2);
      new_node2->AddDownstreamNode(
          BuildRCTreeHelper(nodes, adjacents, children[0]));
      new_node2->AddDownstreamNode(
          BuildRCTreeHelper(nodes, adjacents, children[1]));
      return new_node;
    }
  }

  if (current_node->Type() == RCTreeNodeType::DRIVER) {
    if (children.size() == 2) {
      auto new_node = std::make_shared<JuncNode>(current_node->Location());
      current_node->AddDownstreamNode(new_node);
      new_node->AddDownstreamNode(
          BuildRCTreeHelper(nodes, adjacents, children[0]));
      new_node->AddDownstreamNode(
          BuildRCTreeHelper(nodes, adjacents, children[1]));
      return current_node;
    }
    if (children.size() == 1) {
      current_node->AddDownstreamNode(
          BuildRCTreeHelper(nodes, adjacents, children[0]));
      return current_node;
    }
    throw std::runtime_error(
        "Driver node must have at least one downstream node.");
  }

  for (auto& n_index : children) {
    RCTreeNodePtr child_node = BuildRCTreeHelper(nodes, adjacents, n_index);
    current_node->AddDownstreamNode(child_node);
  }
  return current_node;
}

// TODO: IMPL
void UvDRCSlewBuffer::PrepareBufferSlots(RCTreeNodePtr root,
                                         const sta::Corner* corner)
{
  auto max_wire_length = resizer_->metersToDbu(resizer_->findMaxWireLength());
  max_wire_length = std::min(
      std::min(max_wire_length, user_max_wire_length_),
      std::min(MaxLengthForSlew(buffer_candidates_.front().cell, corner),
               MaxLengthForCap(buffer_candidates_.front().cell, corner)));

  auto buffer_step = max_wire_length / 2;

  // resizer_->logger()->report("Preparing buffer slots with buffer step: {}",
  //                            buffer_step);

  // Prepare buffer slots per max_wire_length
  auto d_nodes = root->DownstreamNodes();
  for (auto& d : d_nodes) {
    PrepareBufferSlotsHelper(root, d, buffer_step);
  }
  // root->DebugPrint(resizer_->logger());
}

// TODO: IMPL
void UvDRCSlewBuffer::PrepareBufferSlotsHelper(RCTreeNodePtr u,
                                               RCTreeNodePtr d,
                                               int buffer_step)
{
  auto u_loc = u->Location();
  auto d_loc = d->Location();
  auto distance = odb::Point::manhattanDistance(u_loc, d_loc);
  if (distance > buffer_step) {
    // TODO: insert wire nodes
    // NOTE: here we divide the wire into segments of same length,
    // another way is to prepare a slot per buffer_step.
    int n = distance / buffer_step;
    int delta_x = (u_loc.x() - d_loc.x()) / n;
    int delta_y = (u_loc.y() - d_loc.y()) / n;
    RCTreeNodePtr prevWireNode = nullptr;
    for (int i = 1; i <= n; i++) {
      odb::Point wire_node_loc{d_loc.x() + i * delta_x,
                               d_loc.y() + i * delta_y};
      RCTreeNodePtr wireNode = std::make_shared<WireNode>(wire_node_loc);
      if (prevWireNode != nullptr) {
        wireNode->AddDownstreamNode(prevWireNode);
      } else {
        wireNode->AddDownstreamNode(d);
      }
      prevWireNode = wireNode;
    }
    u->RemoveDownstreamNode(d);
    u->AddDownstreamNode(prevWireNode);
  } else if (u->Type() == RCTreeNodeType::JUNCTION
             && distance
                    > 0) {  // We still need a Buffer slot at junctions' outputs
    RCTreeNodePtr wireNode = std::make_shared<WireNode>(u_loc);
    wireNode->AddDownstreamNode(d);
    u->RemoveDownstreamNode(d);
    u->AddDownstreamNode(wireNode);
  }

  auto next_d_nodes = d->DownstreamNodes();
  for (auto& next_d_node : next_d_nodes) {
    PrepareBufferSlotsHelper(d, next_d_node, buffer_step);
  }
}

// Max length
int UvDRCSlewBuffer::MaxLengthForSlew(sta::LibertyCell* buffer_cell,
                                      const sta::Corner* corner)
{
  switch (model_) {
    case UvDRCSlewModel::Alpert: {
      return MaxLengthForSlewAlpert(buffer_cell, corner);
    }
    case UvDRCSlewModel::OpenROAD: {
      return MaxLengthForSlewOpenROAD(buffer_cell, corner);
    }
    default:
      throw std::runtime_error("Unsupported UvDRCSlewModel!");
  }
}

// TODO: Impl
int UvDRCSlewBuffer::MaxLengthForSlewAlpert(sta::LibertyCell* buffer_cell,
                                            const sta::Corner* corner)
{
  throw std::invalid_argument("Alpert Model is not implemented yet!");
}

int UvDRCSlewBuffer::MaxLengthForSlewOpenROAD(sta::LibertyCell* buffer_cell,
                                              const sta::Corner* corner)
{
  // OpenROAD's RC-based slew model
  // NOTE: the original version in Rebuffer uses Lumped RC model
  // Here I modify it to use distributed RC model (a wire's Cap would be
  // distributed to both ends)

  // In this demo, we assume unit r,c are same on H & V directions
  // TODO: Consider different unit r,c on H & V directions for better estimation
  sta::LibertyPort *in, *out;
  buffer_cell->bufferPorts(in, out);

  auto max_slew = resizer_->maxInputSlew(in, corner);
  if (max_slew == sta::INF) {
    return std::numeric_limits<int>::max();
  }
  static constexpr float k_slew_margin = 0.2f;  // 20%
  max_slew *= (1 - k_slew_margin);              // * 0.8

  double unit_r, unit_c;
  resizer_->estimate_parasitics_->wireSignalRC(corner, unit_r, unit_c);

  double r_drvr = out->driveResistance();
  double in_cap = in->capacitance();
  // Solve quadratic equation to get max length
  // 0.5 r c l^2 + (r * C_Load + R_drvr * c) l + R_drvr * C_Load = Max_Slew /
  // Delay_Slew_Factor
  const double a = unit_r * unit_c / 2;
  const double b = unit_c * r_drvr + unit_r * in_cap;
  const double c = r_drvr * in_cap - max_slew / K_OPENROAD_SLEW_FACTOR;
  const double d = b * b - 4 * a * c;
  if (d < 0) {
    resizer_->logger()->warn(
        utl::RSZ,
        2012,
        "cannot determine wire length limit implied by load "
        "slew on cell {} in corner {}",
        buffer_cell->name(),
        corner->name());
    return std::numeric_limits<int>::max();
  }

  const double max_length_meters = (-b + std::sqrt(d)) / (2 * a);

  if (max_length_meters > 1) {
    // no limit
    return std::numeric_limits<int>::max();
  }

  if (max_length_meters <= 0) {
    resizer_->logger()->warn(
        utl::RSZ,
        2013,
        "cannot determine wire length limit implied by load "
        "slew on cell {} in corner {}",
        buffer_cell->name(),
        corner->name());
    return std::numeric_limits<int>::max();
  }

  int max_length_dbu = resizer_->metersToDbu(max_length_meters);

  if (max_length_dbu == 0) {
    resizer_->logger()->warn(
        utl::RSZ,
        2014,
        "cannot determine wire length limit implied by load "
        "slew on cell {} in corner {}",
        buffer_cell->name(),
        corner->name());
    return std::numeric_limits<int>::max();
  }

  return max_length_dbu;
}

int UvDRCSlewBuffer::MaxLengthForCap(sta::LibertyCell* buffer_cell,
                                     const sta::Corner* corner)
{
  sta::LibertyPort *in, *out;
  buffer_cell->bufferPorts(in, out);

  bool cap_limit_exists;
  float cap_limit;
  out->capacitanceLimit(sta::MinMax::max(), cap_limit, cap_limit_exists);

  static constexpr float k_cap_margin = 0.2f;  // 20%
  if (cap_limit_exists) {
    double wire_res, wire_cap;
    resizer_->estimate_parasitics_->wireSignalRC(corner, wire_res, wire_cap);
    double max_length
        = (cap_limit * (1 - k_cap_margin) - in->capacitance()) / wire_cap;
    return std::max(0, resizer_->metersToDbu(max_length));
  }

  return std::numeric_limits<int>::max();
}

std::size_t UvDRCSlewBuffer::Run(const sta::Pin* drvr_pin,
                                 const sta::Corner* corner,
                                 float max_cap)
{
  InitBufferCandidates();
  RCTreeNodePtr root = nullptr;
  {
    auto [locs, loc_map] = InitNetConnections(drvr_pin);
    auto steiner_tree = MakeSteinerTree(drvr_pin, locs, loc_map);

    // steiner_tree.printTree(resizer_->logger());

    root = BuildRCTree(drvr_pin, corner, steiner_tree, locs, loc_map);

    PrepareBufferSlots(root, corner);

    // root->DebugPrint(resizer_->logger());
  }

  double unit_r, unit_c;
  resizer_->estimate_parasitics_->wireSignalRC(corner, unit_r, unit_c);

  float r_drvr = resizer_->driveResistance(drvr_pin);
  for (auto& cand : buffer_candidates_) {
    r_drvr = std::max(r_drvr, cand.drive_resistance);
  }

  auto solutions = root->CalcBufferSolutions(corner,
                                             unit_r,
                                             unit_c,
                                             max_cap,
                                             resizer_,
                                             resizer_->sta_,
                                             buffer_candidates_,
                                             r_drvr);
  if (solutions.empty()) {
    throw std::runtime_error("No buffer solutions found for the root node.");
  }
  std::sort(solutions.begin(),
            solutions.end(),
            [](BufferSolution& a, BufferSolution& b) {
              return a.buffer_locs.size() < b.buffer_locs.size();
            });
  auto& best_solution = solutions.front();
  // resizer_->logger()->report(
  //     "Best buffer solution for the root node: area = {}",
  //     best_solution.buffer_locs.size());
  // for (auto& p : best_solution.buffer_locs) {
  //   resizer_->logger()->report("  Buffer at ({}, {}) with cell {}",
  //                              p.first->Location().x(),
  //                              p.first->Location().y(),
  //                              p.second->name());
  // }
  if (best_solution.buffer_locs.size() > 0) {
    root->MakeBuffer(best_solution, resizer_, resizer_->sta_->network());
  }
  return best_solution.buffer_locs.size();
}

}  // namespace uv_drc
