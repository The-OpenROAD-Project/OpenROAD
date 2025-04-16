// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "detailed_generator.h"

namespace dpo {

class Architecture;
class DetailedMgr;
class DetailedObjective;
class Network;

class DetailedRandom
{
 public:
  enum DrcMode
  {
    DrcMode_NoPenalty = 0,
    DrcMode_NormalPenalty,
    DrcMode_Eliminate,
    DrcMode_Unknown
  };
  enum MoveMode
  {
    MoveMode_Median = 0,
    MoveMode_CellDensity1,
    MoveMode_RandomWindow,
    MoveMode_Unknown
  };
  enum MoveSource
  {
    MoveSource_All = 0,
    MoveSource_Wirelength,
    MoveSource_DrcViolators,
    MoveSource_Unknown
  };

 public:
  DetailedRandom(Architecture* arch, Network* network);

  void run(DetailedMgr* mgrPtr, const std::string& command);
  void run(DetailedMgr* mgrPtr, std::vector<std::string>& args);

 private:
  double go();

  double eval(const std::vector<double>& costs,
              const std::vector<std::string>& expr) const;
  double doOperation(double a, double b, char op) const;
  bool isOperator(char ch) const;
  bool isObjective(char ch) const;
  bool isNumber(char ch) const;

  void collectCandidates();

  // Standard stuff.
  DetailedMgr* mgrPtr_ = nullptr;

  Architecture* arch_;
  Network* network_;

  // Candidate cells.
  std::vector<Node*> candidates_;

  // For generating move lists.
  std::vector<DetailedGenerator*> generators_;

  // For evaluating objectives.
  std::vector<DetailedObjective*> objectives_;

  // Parameters controlling the moves.
  double movesPerCandidate_ = 3;

  // For costing.
  std::vector<double> initCost_;
  std::vector<double> currCost_;
  std::vector<double> nextCost_;
  std::vector<double> deltaCost_;

  // For obj evaluation.
  std::vector<std::string> expr_;
};

class RandomGenerator : public DetailedGenerator
{
 public:
  RandomGenerator();

 public:
  bool generate(DetailedMgr* mgr, std::vector<Node*>& candidates) override;
  void stats() override;
  void init(DetailedMgr*) override {}

 private:
  DetailedMgr* mgr_ = nullptr;
  Architecture* arch_ = nullptr;
  Network* network_ = nullptr;

  int attempts_ = 0;
  int moves_ = 0;
  int swaps_ = 0;
};

class DisplacementGenerator : public DetailedGenerator
{
 public:
  DisplacementGenerator();

 public:
  bool generate(DetailedMgr* mgr, std::vector<Node*>& candidates) override;
  void stats() override;
  void init(DetailedMgr*) override {}

 private:
  DetailedMgr* mgr_ = nullptr;
  Architecture* arch_ = nullptr;
  Network* network_ = nullptr;

  int attempts_ = 0;
  int moves_ = 0;
  int swaps_ = 0;
};

}  // namespace dpo
