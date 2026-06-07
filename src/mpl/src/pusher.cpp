// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pusher.h"

#include <cstdlib>
#include <map>
#include <vector>

#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace mpl {
using utl::MPL;

Pusher::Pusher(utl::Logger* logger,
               Cluster* root,
               odb::dbBlock* block,
               const std::vector<odb::Rect>& io_blockages)
    : logger_(logger), root_(root), block_(block)
{
  core_ = block_->getCoreArea();
  setIOBlockages(io_blockages);
}

void Pusher::setIOBlockages(const std::vector<odb::Rect>& io_blockages)
{
  io_blockages_ = io_blockages;
}

void Pusher::fetchMacroClusters(Cluster* parent,
                                std::vector<Cluster*>& macro_clusters)
{
  for (auto& child : parent->getChildren()) {
    if (child->getClusterType() == HardMacroCluster) {
      macro_clusters.push_back(child.get());

      for (HardMacro* hard_macro : child->getHardMacros()) {
        hard_macros_.push_back(hard_macro);
      }

    } else if (child->getClusterType() == MixedCluster) {
      fetchMacroClusters(child.get(), macro_clusters);
    }
  }
}

void Pusher::pushMacrosToCoreBoundaries()
{
  // Case in which the design has nothing but macros.
  if (root_->getClusterType() == HardMacroCluster) {
    return;
  }

  if (designHasSingleCentralizedMacroArray()) {
    return;
  }

  std::vector<Cluster*> macro_clusters;
  fetchMacroClusters(root_, macro_clusters);

  for (Cluster* macro_cluster : macro_clusters) {
    if (macro_cluster->isFixedMacro()) {
      continue;
    }

    debugPrint(logger_,
               MPL,
               "boundary_push",
               1,
               "Macro Cluster {}",
               macro_cluster->getName());

    std::map<Boundary, int> boundaries_distance
        = getDistanceToCloseBoundaries(macro_cluster);

    if (logger_->debugCheck(MPL, "boundary_push", 1)) {
      logger_->report("Distance to Close Boundaries:");

      for (auto& [boundary, distance] : boundaries_distance) {
        logger_->report("{} {}", toString(boundary), distance);
      }
    }

    pushMacroClusterToCoreBoundaries(macro_cluster, boundaries_distance);
  }
}

bool Pusher::designHasSingleCentralizedMacroArray()
{
  int macro_cluster_count = 0;

  for (auto& child : root_->getChildren()) {
    switch (child->getClusterType()) {
      case MixedCluster:
        return false;
      case HardMacroCluster:
        ++macro_cluster_count;
        break;
      case StdCellCluster: {
        // Note: to check whether or not a std cell cluster is "tiny"
        // we use the area of its SoftMacro abstraction, because the
        // Cluster::getArea() will give us the actual std cell area
        // of the instances from that cluster.
        if (child->getSoftMacro()->getArea() != 0) {
          return false;
        }
      }
    }

    if (macro_cluster_count > 1) {
      return false;
    }
  }

  return true;
}

// We only group macros of the same size, so here we can use any HardMacro
// from the cluster to set the minimum distance from the respective
// boundary to trigger a push.
std::map<Boundary, int> Pusher::getDistanceToCloseBoundaries(
    Cluster* macro_cluster)
{
  std::map<Boundary, int> boundaries_distance;

  const odb::Rect cluster_box = macro_cluster->getBBox();

  HardMacro* hard_macro = macro_cluster->getHardMacros().front();

  Boundary hor_boundary_to_push;
  const int distance_to_left = std::abs(cluster_box.xMin() - core_.xMin());
  const int distance_to_right = std::abs(cluster_box.xMax() - core_.xMax());
  int smaller_hor_distance = 0;

  if (distance_to_left < distance_to_right) {
    hor_boundary_to_push = Boundary::L;
    smaller_hor_distance = distance_to_left;
  } else {
    hor_boundary_to_push = Boundary::R;
    smaller_hor_distance = distance_to_right;
  }

  const int hard_macro_width = hard_macro->getWidth();
  if (smaller_hor_distance < hard_macro_width) {
    boundaries_distance[hor_boundary_to_push] = smaller_hor_distance;
  }

  Boundary ver_boundary_to_push;
  const int distance_to_top = std::abs(cluster_box.yMax() - core_.yMax());
  const int distance_to_bottom = std::abs(cluster_box.yMin() - core_.yMin());
  int smaller_ver_distance = 0;

  if (distance_to_bottom < distance_to_top) {
    ver_boundary_to_push = Boundary::B;
    smaller_ver_distance = distance_to_bottom;
  } else {
    ver_boundary_to_push = Boundary::T;
    smaller_ver_distance = distance_to_top;
  }

  const int hard_macro_height = hard_macro->getHeight();
  if (smaller_ver_distance < hard_macro_height) {
    boundaries_distance[ver_boundary_to_push] = smaller_ver_distance;
  }

  return boundaries_distance;
}

void Pusher::pushMacroClusterToCoreBoundaries(
    Cluster* macro_cluster,
    const std::map<Boundary, int>& boundaries_distance)
{
  if (boundaries_distance.empty()) {
    return;
  }

  std::vector<HardMacro*> hard_macros = macro_cluster->getHardMacros();
  // Check based on the shape of the macro cluster to avoid iterating each
  // of its HardMacros.
  odb::Rect cluster_box = macro_cluster->getBBox();

  for (const auto& [boundary, distance] : boundaries_distance) {
    if (distance == 0) {
      continue;
    }

    moveMacroClusterBox(cluster_box, boundary, distance);

    debugPrint(logger_,
               MPL,
               "boundary_push",
               1,
               "Moved {} in the direction of {}.",
               macro_cluster->getName(),
               toString(boundary));

    if (overlapsWithHardMacro(cluster_box, macro_cluster->getId())
        || overlapsWithIOBlockage(cluster_box)) {
      // Move back to original position.
      moveMacroClusterBox(cluster_box, boundary, -distance);
    } else {
      // Commit movement to hard macros if there are no overlaps with the
      // cluster
      for (HardMacro* hard_macro : hard_macros) {
        moveHardMacro(hard_macro, boundary, distance);
      }
    }
  }
}

void Pusher::moveMacroClusterBox(odb::Rect& cluster_box,
                                 const Boundary boundary,
                                 const int distance)
{
  switch (boundary) {
    case (Boundary::L): {
      cluster_box.moveDelta(-distance, 0);
      break;
    }
    case (Boundary::R): {
      cluster_box.moveDelta(distance, 0);
      break;
    }
    case (Boundary::T): {
      cluster_box.moveDelta(0, distance);
      break;
    }
    case (Boundary::B): {
      cluster_box.moveDelta(0, -distance);
      break;
    }
  }
}

void Pusher::moveHardMacro(HardMacro* hard_macro,
                           const Boundary boundary,
                           const int distance)
{
  switch (boundary) {
    case (Boundary::L): {
      hard_macro->setX(hard_macro->getX() - distance);
      break;
    }
    case (Boundary::R): {
      hard_macro->setX(hard_macro->getX() + distance);
      break;
    }
    case (Boundary::T): {
      hard_macro->setY(hard_macro->getY() + distance);
      break;
    }
    case (Boundary::B): {
      hard_macro->setY(hard_macro->getY() - distance);
      break;
    }
  }
}

bool Pusher::overlapsWithHardMacro(const odb::Rect& cluster_box, int cluster_id)
{
  for (const HardMacro* hard_macro : hard_macros_) {
    if (hard_macro->getCluster()->getId() == cluster_id) {
      continue;
    }

    if (cluster_box.overlaps(hard_macro->getBBox())) {
      debugPrint(logger_,
                 MPL,
                 "boundary_push",
                 1,
                 "\tFound overlap with HardMacro {}. Push will be reverted.",
                 hard_macro->getName());
      return true;
    }
  }

  return false;
}

bool Pusher::overlapsWithIOBlockage(const odb::Rect& cluster_box) const
{
  for (const odb::Rect& io_blockage : io_blockages_) {
    if (cluster_box.overlaps(io_blockage)) {
      debugPrint(logger_,
                 MPL,
                 "boundary_push",
                 1,
                 "\tFound overlap with IO blockage {}. Push will be reverted.",
                 io_blockage);
      return true;
    }
  }

  return false;
}

}  // namespace mpl
