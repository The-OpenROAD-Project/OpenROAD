///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <map>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "leidenInterface.h"
#include "odb/db.h"
#include "sta/Liberty.hh"

namespace utl {
class Logger;
}

namespace sta {
class dbNetwork;
}

namespace mpl2 {

class GraphForLeidenAlgorithm;
class HyperGraphForLeidenAlgorithm;

class leidenClustering
{
 public:
  ~leidenClustering();
  leidenClustering(odb::dbDatabase* db,
                   odb::dbBlock* block,
                   utl::Logger* logger);

  /**
   * @brief Executes the Leiden clustering algorithm.
   *
   * This function initiates and runs the Leiden clustering algorithm, which is
   * used for detecting communities in large networks. The algorithm optimizes
   * modularity to find the best partitioning of the
   * network.
   */
  void run();

 private:
  /**
   * @brief Pointer to the database.
   */
  odb::dbDatabase* db_;

  /**
   * @brief Pointer to the database block.
   */
  odb::dbBlock* block_;

  /**
   * @brief Logger for logging messages.
   */
  utl::Logger* logger_;

  /**
   * @brief Graph representation for the Leiden algorithm.
   */
  GraphForLeidenAlgorithm* graph_;

  /**
   * @brief Hypergraph representation for the Leiden algorithm.
   */
  HyperGraphForLeidenAlgorithm* hypergraph_;

  /**
   * @brief Initializes the clustering algorithm.
   */
  void init();

  /**
   * @brief Creates the hypergraph representation.
   */
  void createHypergraph();

  /**
   * @brief Creates the graph representation.
   */
  void createGraph();

  /**
   * @brief Runs the Leiden clustering algorithm.
   */
  void runLeidenClustering();

  /**
   * @brief Checks if a master should be ignored.
   *
   * @param master The master to check.
   * @return True if the master should be ignored, false otherwise.
   */
  bool isIgnoredMaster(odb::dbMaster* master);
};

}  // namespace mpl2