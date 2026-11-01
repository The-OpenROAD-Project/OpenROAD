// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "fsv/Stack.h"
#include "pex/Model.h"
#include "pex/Patterns.h"

namespace pex {

// How a piece's cover on one side maps onto the model's contexts for its
// layer. A cover the model tabulated has its own context. One it did not
// (a layer past the builder's depth that its far factors did not pick)
// takes the tabulated covers on that side that bracket it in distance,
// weighted linearly in the logarithm of the distance; one past the
// farthest tabulated takes the farthest alone, and one nearer than the
// nearest takes the nearest.
class CoverMap
{
 public:
  // The model's contexts for `layer`, a conductor of the stack. Throws
  // std::invalid_argument for a handle that is not a conductor.
  CoverMap(const fsv::Stack& stack, const Model& model, Layer layer);

  // One context a cover resolves to: the name a lateral key holds, and
  // its weight.
  struct Part
  {
    std::string key;
    double weight;
  };

  // The contexts a cover resolves to: one, or two whose weights sum to
  // 1. No cover (a handle that names no layer) is the "" context alone,
  // as is a cover on a side where the model tabulates none, or a layer
  // beside the wire rather than over or under it, which covers nothing.
  // A cover is a conductor: a via handle throws std::invalid_argument, as
  // it would mean the caller took a via's shape for a covering wire.
  std::vector<Part> resolve(Layer cover) const;

  // The cover whose crossing table a cover takes: itself when the model
  // has one, else the nearest tabulated cover on that side (in the
  // logarithm of the distance) that has one; "" when none. Throws for a
  // via handle, as resolve does.
  std::string crossingCover(Layer cover) const;

  // The distance between the layer and a cover, face to face; negative
  // for a layer beside the wire.
  double distance(Layer cover) const;

 private:
  struct Tabulated
  {
    std::string name;
    double distance;
    bool crossing;  // whether the model has a crossing table for it
  };
  // Where a cover sits: its side's tabulated covers and its distance, or
  // nullptr for a layer beside the wire.
  const std::vector<Tabulated>* side(Layer cover, double& distance) const;

  const fsv::Stack& stack_;
  Layer layer_;
  std::vector<Tabulated> below_;  // ascending by distance
  std::vector<Tabulated> above_;
};

}  // namespace pex
