// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace sta {
class TimingArc;
}

namespace rsz {

using AnalysisPinKey
    = std::tuple<std::string, std::string, std::string, std::string, bool>;

// Captures one observed pin's electrical and path timing state in SI units.
struct AnalysisPinState
{
  std::string pin;
  std::string instance;
  std::string master;
  std::string net;
  std::string role;
  std::string scene;
  std::string rf;
  std::string startpoint;
  std::string launch_clock;
  std::optional<double> arrival;
  std::optional<double> required;
  std::optional<double> slack;
  std::optional<double> slew;
  std::optional<double> input_cap;
  std::optional<double> pin_cap;
  std::optional<double> wire_cap;
  std::optional<double> fanout;
  std::optional<double> drive_resistance;
  std::optional<double> drive_resistance_max;
  std::string sense{"max"};
  bool clock{false};

  AnalysisPinKey key() const { return {pin, scene, rf, sense, clock}; }
};

// Captures the selected Liberty arc and its measured cell delay.
struct AnalysisArcState
{
  std::string from;
  std::string to;
  std::string master;
  std::string scene;
  std::string from_rf;
  std::string to_rf;
  std::string role;
  std::optional<double> delay;
  const sta::TimingArc* arc{nullptr};
  std::string condition{};
  std::string mode_name{};
  std::string mode_value{};
  bool default_condition{false};
};

// Identifies one pin attached to an observed net.
struct AnalysisConnection
{
  std::string net;
  std::string pin;
  std::string instance;
  std::string direction;
};

// Captures one point of a reported timing path.
struct AnalysisPathPoint
{
  std::string pin{};
  std::string master{};
  std::string net{};
  std::string rf{};
  std::string role{};
  std::optional<double> arrival{};
  std::optional<double> slew{};
  std::optional<double> delay{};
};

// Captures one path's endpoints, slack, and ordered timing points.
struct AnalysisPathState
{
  std::string kind;
  std::string scene;
  std::string rf;
  std::string startpoint;
  std::string end_pin;
  std::string launch_clock;
  std::optional<double> slack;
  std::vector<AnalysisPathPoint> points;
  std::string sense{"max"};
  std::optional<double> required;
};

// Holds the timing and topology observed before or after one ECO move.
struct AnalysisSnapshot
{
  std::map<std::string, double> metrics;
  std::string target_instance;
  std::string target_master;
  std::vector<AnalysisPinState> pins;
  std::vector<AnalysisArcState> arcs;
  std::vector<AnalysisConnection> connections;
  std::vector<AnalysisPathState> paths;
};

// Captures one stage of the estimator window selected for a move.
struct AnalysisWindowStage
{
  std::string driver;
  std::string input;
  std::string master;
  std::string scene;
  std::string from_rf;
  std::string to_rf;
  bool target{false};
  double input_slew{0.0};
  double load_cap{0.0};
  double model_delay{0.0};
  double cell_delay{0.0};
  double wire_delay{0.0};
  double output_slew{0.0};
  std::string role{};
  std::string condition{};
  std::string mode_name{};
  std::string mode_value{};
};

}  // namespace rsz
