/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024, The Regents of the University of California
// Copyright (c) 2024, Minjae Kim (kmj0824@postech.ac.kr)
// All rights reserved.
//
// BSD 3-Clause License
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
//
// This code refers to the below contents.
// 1. [Abacus] PAPER:
//    Fast Legalization of Standard Cell Circuits with Minimal Movement
// 2. [DREAMPlace] LINK:
//    https://github.com/limbo018/DREAMPlace/tree/master/dreamplace/ops/abacus_legalize
///////////////////////////////////////////////////////////////////////////////

#ifndef OPENROAD_ABACUSLEGALIZER_H
#define OPENROAD_ABACUSLEGALIZER_H

#include <vector>

#include "odb/db.h"
namespace dpl {
/**
 * This legalizer is for comparing with OpenDP legalizer
 * \Refer:
 * [Abacus] Fast Legalization of Standard Cell Circuits with Minimal Movement
 * [DREAMPlace]
 * https://github.com/limbo018/DREAMPlace/tree/master/dreamplace/ops/abacus_legalize
 * This is the simple and fast legalizer
 * */
using namespace std;
using namespace odb;
class AbacusLegalizer
{
  using InstsInRow = std::vector<odb::dbInst*>;
  using Rows = std::vector<InstsInRow>;
  struct AbacusCluster
  {
    std::vector<odb::dbInst*> instSet;
    AbacusCluster* predecessor;
    AbacusCluster* successor;

    double e;  // weight of displacement in the objective
    double q;  // x = q/e
    double w;  // cluster width
    double x;  // optimal location (cluster's left edge coordinate)
  };

 public:
  /**
   * @brief
   * Algorithm1 of Abacus
   * */
  void runAbacus(odb::dbBlock* block);

 private:
  /**
   * @brief Algorithm2 of Abacus
   * */
  uint placeRow(InstsInRow* instsInRow, bool trial);
  void addCell(AbacusCluster* cluster, odb::dbInst* inst);
  void addCluster(AbacusCluster* predecessor, AbacusCluster* cluster);
  void collapse(AbacusLegalizer::AbacusCluster& cluster,
                vector<AbacusCluster>& abacusClusters);

  void initRow();

  /**
   * \brief
   * Cost evaluation for algorithm1.
   * This will evaluate the HPWL.
   * */
  uint getCost();

  /**
   * @brief
   * This function returns the index of row for the cell
   * */
  int getRowIdx(dbInst* cell);
  InstsInRow* getAboveRow(InstsInRow* rowTmp);
  InstsInRow* getBelowRow(InstsInRow* row);

  odb::dbBlock* targetBlock_ = nullptr;
  std::map<odb::dbInst*, InstsInRow*> cellToRowMap_;
  std::map<InstsInRow*, int> rowToRowIdxMap_;
  Rows rows_;
};
}  // namespace dpl
#endif  // OPENROAD_ABACUSLEGALIZER_H
