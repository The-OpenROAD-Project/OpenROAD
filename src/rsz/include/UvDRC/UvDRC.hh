#ifndef UV_DRC_HH
#define UV_DRC_HH

#include <odb/geom.h>
#include <stt/SteinerTreeBuilder.h>

#include <cstddef>
#include <db_sta/dbSta.hh>
#include <memory>
#include <rsz/Resizer.hh>
#include <sta/Delay.hh>
#include <sta/Network.hh>
#include <sta/Path.hh>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "sta/Corner.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace uv_drc {

class LocEqual
{
 public:
  bool operator()(const odb::Point& loc1, const odb::Point& loc2) const;
};

class LocHash
{
 public:
  size_t operator()(const odb::Point& loc) const;

 private:
  std::size_t HashCombine(std::size_t seed, std::size_t value) const;
};

using LocVec = std::vector<odb::Point>;
using PinVec = std::vector<const sta::Pin*>;

enum class UvDRCSlewModel
{
  Alpert,
  OpenROAD
};

struct BufferCandidate
{
  sta::LibertyCell* cell;
  float input_cap;
  float drive_resistance;
  float input_slew_limit;
  float load_cap_limit;
};
using BufferCandidates = std::vector<BufferCandidate>;

struct BufferSolution;
class RCTreeNode;
using RCTreeNodePtr = std::shared_ptr<RCTreeNode>;

using BufferLocMap = std::unordered_map<RCTreeNode*, sta::LibertyCell*>;

struct BufferSolution
{
  float cap;
  float wire_slew;
  float limit;
  BufferLocMap buffer_locs;
};

enum class RCTreeNodeType
{
  LOAD,
  WIRE,
  JUNCTION,
  DRIVER,
  NONE
};

class RCTreeNode
{
 public:
  RCTreeNode(odb::Point loc, RCTreeNodeType type) : loc_(loc), type_(type) {}
  virtual ~RCTreeNode() = default;
  RCTreeNodeType Type() { return type_; }
  odb::Point Location() const { return loc_; }
  virtual const sta::Pin* pin() const { return nullptr; }
  virtual const sta::Pin* LoadPin() const { return nullptr; }
  virtual void AddDownstreamNode(RCTreeNodePtr ptr) = 0;
  virtual void RemoveDownstreamNode(RCTreeNodePtr ptr) = 0;
  virtual std::vector<RCTreeNodePtr> DownstreamNodes() = 0;
  virtual std::size_t DownstreamNodeCount() = 0;
  virtual std::vector<BufferSolution> CalcBufferSolutions(
      const sta::Corner* corner,
      double unit_r,
      double unit_c,
      float max_cap,
      rsz::Resizer* resizer,
      sta::dbSta* sta,
      BufferCandidates& buffer_candidates,
      float r_drvr)
      = 0;
  void AddSolutionAndEnsureDominance(std::vector<BufferSolution>& solutions,
                                     BufferSolution new_sol);

  sta::PinSeq MakeBuffer(BufferSolution& sol,
                         rsz::Resizer* resizer,
                         sta::Network* network)
  {
    sta::PinSeq load_pins;
    for (auto d : DownstreamNodes()) {
      auto d_pins = d->MakeBuffer(sol, resizer, network);
      load_pins.insert(load_pins.end(), d_pins.begin(), d_pins.end());
    }
    if (LoadPin() != nullptr) {
      load_pins.push_back(LoadPin());
    }
    if (auto itr = sol.buffer_locs.find(this); itr != sol.buffer_locs.end()) {
      sta::LibertyCell* buffer_cell = itr->second;
      auto buffer = resizer->insertBufferBeforeLoads(
          nullptr, &load_pins, buffer_cell, &loc_, "UvDRCSlew");
      if (!buffer) {
        throw std::runtime_error("Failed to insert buffer.");
      }
      sta::LibertyPort* buffer_input_port;
      sta::LibertyPort* buffer_output_port;
      buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);
      sta::PinSeq new_load_pins{};
      new_load_pins.push_back(network->findPin(buffer, buffer_input_port));
      std::swap(load_pins, new_load_pins);
    }
    return load_pins;
  }
  void DebugPrint(utl::Logger* logger);

 protected:
  void DebugPrint(int indent, utl::Logger* logger);
  // 0: sol1 / sol2 cannot dominate each other
  // 1: sol1 dominates sol2
  // -1: sol2 dominates sol1
  int IsDominated(BufferSolution& sol1, BufferSolution& sol2);

 protected:
  odb::Point loc_;
  RCTreeNodeType type_;
};

class LoadNode : public RCTreeNode
{
 public:
  LoadNode(odb::Point loc,
           const sta::Pin* pin,
           float load_cap,
           float slew_limit)
      : RCTreeNode(loc, RCTreeNodeType::LOAD),
        pin_(pin),
        load_cap_(load_cap),
        slew_limit_(slew_limit)
  {
  }
  ~LoadNode() override = default;
  void AddDownstreamNode(RCTreeNodePtr ptr) override
  {
    throw std::runtime_error("LoadNode cannot have downstream nodes.");
  }
  void RemoveDownstreamNode(RCTreeNodePtr ptr) override
  {
    throw std::runtime_error("LoadNode cannot have downstream nodes.");
  }
  std::vector<RCTreeNodePtr> DownstreamNodes() override { return {}; }
  std::size_t DownstreamNodeCount() override { return 0; }
  std::vector<BufferSolution> CalcBufferSolutions(
      const sta::Corner* corner,
      double unit_r,
      double unit_c,
      float max_cap,
      rsz::Resizer* resizer,
      sta::dbSta* sta,
      BufferCandidates& buffer_candidates,
      float r_drvr) override;
  const sta::Pin* LoadPin() const override { return pin_; }

 protected:
  const sta::Pin* pin_;
  float load_cap_;
  float slew_limit_;
};

class WireNode : public RCTreeNode
{
 public:
  WireNode(odb::Point loc, RCTreeNodeType type = RCTreeNodeType::WIRE)
      : RCTreeNode(loc, type)
  {
  }
  ~WireNode() override = default;
  void AddDownstreamNode(RCTreeNodePtr ptr) override
  {
    if (downstream_ != nullptr) {
      throw std::runtime_error("Wire node can only have one downstream node.");
    }
    downstream_ = ptr;
  }
  std::vector<RCTreeNodePtr> DownstreamNodes() override
  {
    return {downstream_};
  }
  void RemoveDownstreamNode(RCTreeNodePtr ptr) override
  {
    if (downstream_ != ptr) {
      throw std::runtime_error(
          "Remove non-matching downstream node from Wire node.");
    }
    downstream_ = nullptr;
  }
  std::size_t DownstreamNodeCount() override
  {
    return downstream_ == nullptr ? 0 : 1;
  }
  std::vector<BufferSolution> CalcBufferSolutions(
      const sta::Corner* corner,
      double unit_r,
      double unit_c,
      float max_cap,
      rsz::Resizer* resizer,
      sta::dbSta* sta,
      BufferCandidates& buffer_candidates,
      float r_drvr) override;

 protected:
  RCTreeNodePtr downstream_;
};

class DrivNode : public WireNode
{
 public:
  DrivNode(odb::Point loc, const sta::Pin* pin)
      : WireNode(loc, RCTreeNodeType::DRIVER), pin_(pin)
  {
  }
  ~DrivNode() override = default;
  void AddDownstreamNode(RCTreeNodePtr ptr) override
  {
    if (downstream_ != nullptr) {
      throw std::runtime_error(
          "Driver node can only have one downstream node.");
    }
    downstream_ = ptr;
  }
  std::vector<RCTreeNodePtr> DownstreamNodes() override
  {
    return {downstream_};
  }
  void RemoveDownstreamNode(RCTreeNodePtr ptr) override
  {
    if (downstream_ != ptr) {
      throw std::runtime_error(
          "Remove non-matching downstream node from Driver node.");
    }
    downstream_ = nullptr;
  }
  std::size_t DownstreamNodeCount() override
  {
    return downstream_ == nullptr ? 0 : 1;
  }

 protected:
  const sta::Pin* pin_;
};

class JuncNode : public RCTreeNode
{
 public:
  JuncNode(odb::Point loc) : RCTreeNode(loc, RCTreeNodeType::JUNCTION) {}
  ~JuncNode() override = default;
  void AddDownstreamNode(RCTreeNodePtr ptr) override
  {
    if (downstream1_ == nullptr) {
      downstream1_ = ptr;
    } else if (downstream2_ == nullptr) {
      downstream2_ = ptr;
    } else {
      throw std::runtime_error("Junc node can only have two downstream nodes.");
    }
  }
  std::vector<RCTreeNodePtr> DownstreamNodes() override
  {
    return {downstream1_, downstream2_};
  }
  void RemoveDownstreamNode(RCTreeNodePtr ptr) override
  {
    if (downstream1_ == ptr) {
      downstream1_ = nullptr;
    } else if (downstream2_ == ptr) {
      downstream2_ = nullptr;
    } else {
      throw std::runtime_error(
          "Remove non-matching downstream node from Junc node.");
    }
  }
  std::size_t DownstreamNodeCount() override
  {
    std::size_t count = 0;
    if (downstream1_ != nullptr) {
      count++;
    }
    if (downstream2_ != nullptr) {
      count++;
    }
    return count;
  }
  std::vector<BufferSolution> CalcBufferSolutions(
      const sta::Corner* corner,
      double unit_r,
      double unit_c,
      float max_cap,
      rsz::Resizer* resizer,
      sta::dbSta* sta,
      BufferCandidates& buffer_candidates,
      float r_drvr) override;

 private:
  RCTreeNodePtr downstream1_;
  RCTreeNodePtr downstream2_;
};

// UvDRCSlewBuffer targets to fix slew violations by inserting buffers.
class UvDRCSlewBuffer
{
 public:
  UvDRCSlewBuffer(rsz::Resizer* resizer,
                  UvDRCSlewModel model = UvDRCSlewModel::OpenROAD)
      : resizer_(resizer), model_(model)
  {
  }
  ~UvDRCSlewBuffer() = default;
  std::size_t Run(const sta::Pin* drvr_pin,
                  const sta::Corner* corner,
                  float max_cap);

  static constexpr double K_OPENROAD_SLEW_FACTOR = 1.39;  // From OpenROAD
 private:
  void InitBufferCandidates();
  void SetMaxWireLength(int max_length_dbu)
  {
    user_max_wire_length_ = max_length_dbu;
  }

  std::tuple<LocVec, PinVec> InitNetConnections(const sta::Pin* drvr_pin);
  stt::Tree MakeSteinerTree(const sta::Pin* drvr_pin,
                            LocVec& locs,
                            PinVec& pins);
  RCTreeNodePtr BuildRCTree(const sta::Pin* drvr_pin,
                            const sta::Corner* corner,
                            stt::Tree& tree,
                            LocVec& locs,
                            PinVec& pins);
  RCTreeNodePtr BuildRCTreeHelper(
      std::vector<RCTreeNodePtr>& nodes,
      std::vector<std::vector<std::size_t>>& adjacents,
      std::size_t current_index);
  void PrepareBufferSlots(RCTreeNodePtr root, const sta::Corner* corner);
  void PrepareBufferSlotsHelper(RCTreeNodePtr u,
                                RCTreeNodePtr d,
                                int buffer_step);

  int MaxLengthForSlew(sta::LibertyCell* buffer_cell,
                       const sta::Corner* corner);
  int MaxLengthForSlewAlpert(sta::LibertyCell* buffer_cell,
                             const sta::Corner* corner);
  int MaxLengthForSlewOpenROAD(sta::LibertyCell* buffer_cell,
                               const sta::Corner* corner);
  int MaxLengthForCap(sta::LibertyCell* buffer_cell, const sta::Corner* corner);

 private:
  rsz::Resizer* resizer_;
  // const sta::Pin* drvr_pin_;
  // const sta::Corner* corner_;
  // int max_cap_;
  UvDRCSlewModel model_;

  BufferCandidates buffer_candidates_;
  // RCTreeNodePtr root_ = nullptr;
  // TODO: DELETE THIS
  int user_max_wire_length_ = std::numeric_limits<int>::max();  // in DBU

  // float slew_margin_ = 0.0;
  // float cap_margin_ = 0.0;
};

}  // namespace uv_drc

#endif
