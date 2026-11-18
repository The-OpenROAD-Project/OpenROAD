// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "odb/PtrSetMap.h"

namespace odb {
class dbITerm;
class dbNet;
}  // namespace odb

namespace pad {

class RDLSegment;

class RDLNet
{
 public:
  RDLNet(odb::dbNet* net);

  odb::dbNet* getNet() const { return net_; }

  void addSegment(odb::dbITerm* iterm,
                  const std::vector<odb::dbITerm*>& terminals);
  const std::vector<std::unique_ptr<RDLSegment>>& getSegments() const
  {
    return segments_;
  }
  void finalizeSegments();

  void updateRoute(RDLSegment* segment);
  const odb::PtrMap<odb::dbITerm, odb::PtrSet<odb::dbITerm>>& getRoutedPairs()
      const
  {
    return routed_pairs_;
  }

  bool isNonCoverRouted(odb::dbITerm* iterm) const;
  bool isRouted(odb::dbITerm* source, odb::dbITerm* dest) const;

  bool isRouted() const;
  bool isFailed() const;

 private:
  odb::dbNet* net_;

  std::vector<std::unique_ptr<RDLSegment>> segments_;
  odb::PtrMap<odb::dbITerm, odb::PtrSet<odb::dbITerm>> routed_pairs_;
  odb::PtrSet<odb::dbITerm> routed_noncover_terminals_;

  bool isRouted(odb::dbITerm* source,
                odb::dbITerm* dest,
                odb::PtrSet<odb::dbITerm>& visited) const;
};

}  // namespace pad
