// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <string>
#include <vector>
namespace dpl {
class Node;
}
namespace dpo {

class Architecture;
class DetailedMgr;
class Network;
using dpl::Node;

class DetailedMisParams
{
 public:
  enum Strategy
  {
    KDTree = 0,
    Binning = 1,
    Colour = 2,
  };

  double _maxDifferenceInMetric = 0.03;  // How much we allow the routine to
                                         // reintroduce overlap into placement
  unsigned _maxNumNodes = 15;  // Only consider this many number of nodes for
                               // B&B (<= MAX_BB_NODES)
  unsigned _maxPasses = 1;     // Maximum number of B&B passes
  double _sizeTol = 1.1;       // Tolerance for what is considered same-size
  unsigned _skipNetsLargerThanThis = 50;  // Skip nets larger than this amount.
  Strategy _strategy = Binning;           // The type of strategy to consider
  bool _useSameSize = true;  // If 'false', cells can swap with approximately
                             // same-size locations
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class DetailedMis
{
  // Flow-based solver for replacing nodes using matching.
  enum Objective
  {
    Hpwl,
    Disp
  };

 public:
  DetailedMis(Architecture* arch, Network* network);
  virtual ~DetailedMis();

  void run(DetailedMgr* mgrPtr, const std::string& command);
  void run(DetailedMgr* mgrPtr, std::vector<std::string>& args);

 private:
  struct Bucket;

  void place();
  void collectMovableCells();
  void colorCells();
  void buildGrid();
  void clearGrid();
  void populateGrid();
  bool gatherNeighbours(Node* ndi);
  void solveMatch();
  double getHpwl(const Node* ndi, double xi, double yi);
  double getDisp(const Node* ndi, double xi, double yi);

 public:
  /* DetailedMisParams _params; */

  DetailedMgr* mgrPtr_ = nullptr;

  Architecture* arch_;
  Network* network_;

  std::vector<Node*> candidates_;
  std::vector<bool> movable_;
  std::vector<int> colors_;
  std::vector<Node*> neighbours_;

  // Grid used for binning and locating cells.
  std::vector<std::vector<Bucket*>> grid_;
  int dimW_ = 0;
  int dimH_ = 0;
  double stepX_ = 0;
  double stepY_ = 0;
  std::map<Node*, Bucket*> cellToBinMap_;

  std::vector<int> timesUsed_;

  // Other.
  int skipEdgesLargerThanThis_ = 100;
  int maxProblemSize_ = 25;
  int traversal_ = 0;
  bool useSameSize_ = true;
  bool useSameColor_ = true;
  int maxTimesUsed_ = 2;
  Objective obj_ = DetailedMis::Hpwl;
};

}  // namespace dpo
