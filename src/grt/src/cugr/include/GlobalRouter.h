#pragma once
#include "GRNet.h"
#include "GridGraph.h"
#include "global.h"

namespace grt {

class CUGR
{
 public:
  CUGR(const Design* design, const Parameters& params);
  void route();
  void write(std::string guide_file = "");

 private:
  const Parameters& parameters;
  GridGraph gridGraph;
  std::vector<GRNet> nets;

  int areaOfPinPatches;
  int areaOfWirePatches;

  void sortNetIndices(std::vector<int>& netIndices) const;
  void getGuides(const GRNet& net,
                 std::vector<std::pair<int, grt::BoxT<int>>>& guides);
  void printStatistics() const;
};

}  // namespace grt