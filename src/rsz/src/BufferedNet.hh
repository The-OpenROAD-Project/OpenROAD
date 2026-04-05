// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

#include "odb/geom.h"
#include "spdlog/fmt/fmt.h"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace est {
class EstimateParasitics;
}

namespace rsz {

class Resizer;
class RepairSetup;

class BufferedNet;
using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using Requireds = std::array<sta::Required, sta::RiseFall::index_count>;

class FixedDelay
{
 public:
  FixedDelay();
  explicit FixedDelay(sta::Delay float_value, Resizer* resizer);
  sta::Delay toSeconds() const { return ((float) value_fs_) / second_; }

  // 100 seconds
  static const FixedDelay INF;
  // 0 seconds
  static const FixedDelay ZERO;

  bool operator<(const FixedDelay rhs) const
  {
    return value_fs_ < rhs.value_fs_;
  }
  bool operator>(const FixedDelay rhs) const
  {
    return value_fs_ > rhs.value_fs_;
  }
  bool operator<=(const FixedDelay rhs) const
  {
    return value_fs_ <= rhs.value_fs_;
  }
  bool operator>=(const FixedDelay rhs) const
  {
    return value_fs_ >= rhs.value_fs_;
  }
  FixedDelay operator+(const FixedDelay rhs) const
  {
    return fromFs(value_fs_ + rhs.value_fs_);
  }
  FixedDelay operator-(const FixedDelay rhs) const
  {
    return fromFs(value_fs_ - rhs.value_fs_);
  }
  FixedDelay operator-() const { return fromFs(-value_fs_); }

  static FixedDelay lerp(FixedDelay a, FixedDelay b, float t)
  {
    if (t == 1.0f) {
      return b;
    }

    return a + fromFs((float) (b.value_fs_ - a.value_fs_) * t);
  }

 private:
  static FixedDelay fromFs(int64_t v)
  {
    FixedDelay ret;
    ret.value_fs_ = v;
    return ret;
  }

  static constexpr double second_ = 1.0e15;

  // delay in femtoseconds
  int64_t value_fs_;
};

enum class BufferedNetType
{
  load,
  junction,
  wire,
  via,
  buffer
};

// The routing tree is represented as a binary tree with the sinks being the
// leaves of the tree, the junctions being the Steiner nodes and the root being
// the source of the net.
class BufferedNet
{
 public:
  // load
  BufferedNet(BufferedNetType type,
              const odb::Point& location,
              const sta::Pin* load_pin,
              const sta::Scene* corner,
              const Resizer* resizer);
  // wire
  BufferedNet(BufferedNetType type,
              const odb::Point& location,
              int layer,
              const BufferedNetPtr& ref,
              const sta::Scene* corner,
              const Resizer* resizer,
              const est::EstimateParasitics* estimate_parasitics);
  // via
  BufferedNet(BufferedNetType type,
              const odb::Point& location,
              int layer,
              int ref_layer,
              const BufferedNetPtr& ref,
              const sta::Scene* corner,
              const Resizer* resizer);
  // junc
  BufferedNet(BufferedNetType type,
              const odb::Point& location,
              const BufferedNetPtr& ref,
              const BufferedNetPtr& ref2,
              const Resizer* resizer);
  // buffer
  BufferedNet(BufferedNetType type,
              const odb::Point& location,
              sta::LibertyCell* buffer_cell,
              const BufferedNetPtr& ref,
              const sta::Scene* corner,
              const Resizer* resizer,
              const est::EstimateParasitics* estimate_parasitics);
  std::string to_string(const Resizer* resizer) const;
  void reportTree(const Resizer* resizer) const;
  void reportTree(int level, const Resizer* resizer) const;
  BufferedNetType type() const { return type_; }
  // junction steiner point location connecting ref/ref2
  // wire     wire is from loc to location(ref_)
  // buffer   buffer driver pin location
  // load     load pin location
  odb::Point location() const { return location_; }
  float cap() const { return cap_; }
  void setCapacitance(float cap);
  float fanout() const { return fanout_; }
  void setFanout(float fanout);
  float maxLoadSlew() const { return max_load_slew_; }
  void setMaxLoadSlew(float max_slew);
  // load
  const sta::Pin* loadPin() const { return load_pin_; }
  // wire
  int length() const;
  void wireRC(const sta::Scene* corner,
              const Resizer* resizer,
              const est::EstimateParasitics* estimate_parasitics,
              // Return values.
              double& res,
              double& cap);
  // via
  double viaResistance(const sta::Scene* corner,
                       const Resizer* resizer,
                       const est::EstimateParasitics* estimate_parasitics);
  // wire, via
  int layer() const { return layer_; }
  // via
  int refLayer() const { return ref_layer_; }
  // buffer
  sta::LibertyCell* bufferCell() const { return buffer_cell_; }
  // junction  left
  // buffer    wire
  // wire      end of wire
  BufferedNetPtr ref() const { return ref_; }
  // junction  right
  BufferedNetPtr ref2() const { return ref2_; }

  // repairNet
  int maxLoadWireLength() const;

  // Rebuffer
  const sta::RiseFallBoth* slackTransition() const
  {
    return slack_transitions_;
  };
  void setSlackTransition(const sta::RiseFallBoth* transitions);

  FixedDelay slack() const { return slack_; };
  void setSlack(FixedDelay slack);

  FixedDelay delay() const { return delay_; }
  void setDelay(FixedDelay delay);

  FixedDelay arrivalDelay() const { return arrival_delay_; }
  void setArrivalDelay(FixedDelay delay);

  // Downstream buffer count.
  int bufferCount() const;

  // Downstream number of loads.
  // This is distinct from fanout because fanout is seeded from
  // `resizer->portFanoutLoad(load_port)`
  int loadCount() const;

  float area() const { return area_; }

  static constexpr int null_layer = -1;

  struct Metrics
  {
    int max_load_wl;
    FixedDelay slack = FixedDelay::ZERO;
    float cap;
    float max_load_slew;
    float fanout;

    Metrics withMaxLoadWl(int max_load_wl)
    {
      Metrics ret = *this;
      ret.max_load_wl = max_load_wl;
      return ret;
    }

    Metrics withSlack(FixedDelay slack)
    {
      Metrics ret = *this;
      ret.slack = slack;
      return ret;
    }

    Metrics withCap(float cap)
    {
      Metrics ret = *this;
      ret.cap = cap;
      return ret;
    }
  };

  Metrics metrics() const
  {
    return Metrics{
        maxLoadWireLength(), slack(), cap(), maxLoadSlew(), fanout()};
  }

  bool fitsEnvelope(Metrics target);
  const sta::Scene* corner() { return corner_; }

 private:
  BufferedNetType type_;
  odb::Point location_;
  // only used by load type
  const sta::Pin* load_pin_{nullptr};
  // only used by buffer type
  sta::LibertyCell* buffer_cell_{nullptr};
  // only used by wire and via type
  int layer_{null_layer};
  // only used by via type
  int ref_layer_{null_layer};
  // only used by buffer, wire, via, and junc types
  BufferedNetPtr ref_;
  // only used by junc type
  BufferedNetPtr ref2_;

  // Capacitance looking downstream from here.
  float cap_{0.0};
  float fanout_{1};
  float max_load_slew_{sta::INF};

  // Rebuffer annotations

  // Area of buffers on the buffer tree looking downstream from here
  float area_{0};

  // Transition we need to consider for buffer delays
  const sta::RiseFallBoth* slack_transitions_ = nullptr;

  // Slack considering the buffer/wire delays downstream of here
  FixedDelay slack_ = FixedDelay::ZERO;

  // Computed delay of the buffer/wire
  FixedDelay delay_ = FixedDelay::ZERO;

  // Delay from driver pin to here
  FixedDelay arrival_delay_ = FixedDelay::ZERO;

  const sta::Scene* corner_ = nullptr;
};

// Template magic to make it easier to write algorithms descending
// over the buffer tree in the form of lambdas; it allows recursive
// lambda calling and it keeps track of the level number which is important
// for good debug prints
//
// https://stackoverflow.com/questions/2067988/how-to-make-a-recursive-lambda
template <class F>
struct visitor
{
  F f;
  int level = 0;
  explicit visitor(F&& f) : f(std::forward<F>(f)) {}
  template <class... Args>
  decltype(auto) operator()(Args&&... args)
  {
    level++;
    decltype(auto) ret = f(*this, level, std::forward<Args>(args)...);
    level--;
    return ret;
  }

  // delete the copy constructor
  visitor(const visitor&) = delete;
  visitor& operator=(const visitor&) = delete;
};

template <typename F, class... Args>
static decltype(auto) visitTree(F&& f, Args&&... args)
{
  visitor<std::decay_t<F>> v{std::forward<F>(f)};
  return v(std::forward<Args>(args)...);
}

}  // namespace rsz
