///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <vector>

#include "detailed_generator.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class Architecture;
class DetailedMgr;
class DetailedObjective;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
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
  RoutingParams* rt_ = nullptr;

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
  RoutingParams* rt_ = nullptr;

  int attempts_ = 0;
  int moves_ = 0;
  int swaps_ = 0;
};

}  // namespace dpo
