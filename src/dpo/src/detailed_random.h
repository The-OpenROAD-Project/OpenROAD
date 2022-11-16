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

  void run(DetailedMgr* mgrPtr, std::string command);
  void run(DetailedMgr* mgrPtr, std::vector<std::string>& args);

 private:
  double go();

  double eval(std::vector<double>& costs, std::vector<std::string>& expr);
  double doOperation(double a, double b, char op);
  bool isOperator(char ch);
  bool isObjective(char ch);
  bool isNumber(char ch);

  void collectCandidates();

  // Standard stuff.
  DetailedMgr* mgrPtr_;

  Architecture* arch_;
  Network* network_;

  // Candidate cells.
  std::vector<Node*> candidates_;

  // For generating move lists.
  std::vector<DetailedGenerator*> generators_;

  // For evaluating objectives.
  std::vector<DetailedObjective*> objectives_;

  // Parameters controlling the moves.
  double movesPerCandidate_;

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
  bool generate(DetailedMgr* mgr, std::vector<Node*>& candiates) override;
  void stats() override;
  void init(DetailedMgr*) override {}

 private:
  DetailedMgr* mgr_;
  Architecture* arch_;
  Network* network_;
  RoutingParams* rt_;

  int attempts_;
  int moves_;
  int swaps_;
};

class DisplacementGenerator : public DetailedGenerator
{
 public:
  DisplacementGenerator();

 public:
  bool generate(DetailedMgr* mgr, std::vector<Node*>& candiates) override;
  void stats() override;
  void init(DetailedMgr*) override {}

 private:
  DetailedMgr* mgr_;
  Architecture* arch_;
  Network* network_;
  RoutingParams* rt_;

  int attempts_;
  int moves_;
  int swaps_;
};

}  // namespace dpo
