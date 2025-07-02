// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "odb/geom.h"
#include "spdlog/fmt/fmt.h"
#include "sta/Delay.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::Logger;

using odb::Point;

using sta::Corner;
using sta::DcalcAnalysisPt;
using sta::Delay;
using sta::LibertyCell;
using sta::Network;
using sta::Path;
using sta::Pin;
using sta::Required;
using sta::RiseFall;
using sta::StaState;
using sta::Unit;
using sta::Units;

class Resizer;
class RepairSetup;

class BufferedNet;
using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using Requireds = std::array<Required, RiseFall::index_count>;

class FixedDelay
{
 public:
  explicit FixedDelay(sta::Delay float_value)
  {
    value_fs_ = float_value * second_;
  }

  sta::Delay toSeconds() { return ((float) value_fs_) / second_; }

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
    FixedDelay ret(0);
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
              const Point& location,
              const Pin* load_pin,
              const Corner* corner,
              const Resizer* resizer);
  // wire
  BufferedNet(BufferedNetType type,
              const Point& location,
              int layer,
              const BufferedNetPtr& ref,
              const Corner* corner,
              const Resizer* resizer);
  // junc
  BufferedNet(BufferedNetType type,
              const Point& location,
              const BufferedNetPtr& ref,
              const BufferedNetPtr& ref2,
              const Resizer* resizer);
  // buffer
  BufferedNet(BufferedNetType type,
              const Point& location,
              LibertyCell* buffer_cell,
              const BufferedNetPtr& ref,
              const Corner* corner,
              const Resizer* resizer);
  std::string to_string(const Resizer* resizer) const;
  void reportTree(const Resizer* resizer) const;
  void reportTree(int level, const Resizer* resizer) const;
  BufferedNetType type() const { return type_; }
  // junction steiner point location connecting ref/ref2
  // wire     wire is from loc to location(ref_)
  // buffer   buffer driver pin location
  // load     load pin location
  Point location() const { return location_; }
  float cap() const { return cap_; }
  void setCapacitance(float cap);
  float fanout() const { return fanout_; }
  void setFanout(float fanout);
  float maxLoadSlew() const { return max_load_slew_; }
  void setMaxLoadSlew(float max_slew);
  // load
  const Pin* loadPin() const { return load_pin_; }
  // wire
  int length() const;
  // routing level
  int layer() const { return layer_; }
  void wireRC(const Corner* corner,
              const Resizer* resizer,
              // Return values.
              double& res,
              double& cap);
  // buffer
  LibertyCell* bufferCell() const { return buffer_cell_; }
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

 private:
  BufferedNetType type_;
  Point location_;
  // only used by load type
  const Pin* load_pin_{nullptr};
  // only used by buffer type
  LibertyCell* buffer_cell_{nullptr};
  // only used by wire type
  int layer_{null_layer};
  // only used by buffer, wire, and junc types
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
};

}  // namespace rsz
