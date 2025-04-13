// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "router.h"

#include <algorithm>
#include <vector>

namespace dpo {

void RoutingParams::postProcess()
{
}

double RoutingParams::get_spacing(int layer,
                                  double xmin1,
                                  double xmax1,
                                  double ymin1,
                                  double ymax1,
                                  double xmin2,
                                  double xmax2,
                                  double ymin2,
                                  double ymax2)
{
  const double ww = std::max(std::min(ymax1 - ymin1, xmax1 - xmin1),
                             std::min(ymax2 - ymin2, xmax2 - xmin2));

  // Parallel run-length in the Y-dir.  Will be zero if the objects are above or
  // below each other.
  const double py
      = std::max(0.0, std::min(ymax1, ymax2) - std::max(ymin1, ymin2));

  // Parallel run-length in the X-dir.  Will be zero if the objects are left or
  // right of each other.
  const double px
      = std::max(0.0, std::min(xmax1, xmax2) - std::max(xmin1, xmin2));

  return get_spacing(layer, ww, std::max(px, py));
}

double RoutingParams::get_spacing(int layer, double width, double parallel)
{
  const std::vector<double>& w = spacingTableWidth_[layer];
  const std::vector<double>& p = spacingTableLength_[layer];

  if (w.empty() || p.empty()) {
    // This means no spacing table is present.  So, return the minimum wire
    // spacing for the layer...
    return wire_spacing_[layer];
  }

  int i = (int) w.size() - 1;
  while (i > 0 && width <= w[i]) {
    i--;
  }
  int j = (int) p.size() - 1;
  while (j > 0 && parallel <= p[j]) {
    j--;
  }

  return spacingTable_[layer][i][j];
}

double RoutingParams::get_maximum_spacing(int layer)
{
  const std::vector<double>& w = spacingTableWidth_[layer];
  const std::vector<double>& p = spacingTableLength_[layer];

  if (w.empty() || p.empty()) {
    // This means no spacing table is present.  So, return the minimum wire
    // spacing for the layer...
    return wire_spacing_[layer];
  }

  const int i = (int) w.size() - 1;
  const int j = (int) p.size() - 1;

  return spacingTable_[layer][i][j];
}

}  // namespace dpo
