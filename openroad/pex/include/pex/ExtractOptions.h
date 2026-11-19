// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The settings of an extraction.

#pragma once

namespace pex {

struct EvaluatorOptions
{
  // A box on another layer covers a piece when it overlaps at least this
  // fraction of the wire's width.
  double cover_overlap = 0.5;
  // Layers each way whose parallel wires beside a piece count.
  int diagonal_depth = 2;
};

struct ExtractOptions
{
  // A run of wire shapes joins into one resistor up to this, ohms; 0 makes
  // every shape its own.
  double max_res = 50;
  // A net pair's coupling goes to ground on both nets when its total is
  // below coupling_threshold fF and below coupling_ratio of each net's
  // total capacitance; either at 0 keeps every pair. A pair's coupling is
  // part of each net's total, so a ratio above 1 never keeps one.
  double coupling_threshold = 0.1;
  double coupling_ratio = 0.03;
  // Every net pair's coupling goes to ground on both nets, whatever the
  // thresholds: each net keeps its total, with no coupling elements.
  bool coupling_to_ground = false;
  // Worker threads; 0 runs everything on the calling thread.
  int threads = 0;
  EvaluatorOptions evaluator;
};

}  // namespace pex
