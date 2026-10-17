// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "packingAdapter.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "object.h"
#include "odb/geom.h"
#include "packing.h"
#include "shapes.h"

namespace mpl {

namespace {

int packedCount(const int sequence_pair_macros, const size_t size)
{
  if (sequence_pair_macros < 0
      || sequence_pair_macros > static_cast<int>(size)) {
    throw std::invalid_argument(
        "toPackingProblem: " + std::to_string(sequence_pair_macros)
        + " sequence-pair macros of " + std::to_string(size));
  }
  return sequence_pair_macros > 0 ? sequence_pair_macros
                                  : static_cast<int>(size);
}

PackingBlock fixedBlock(const std::string& name,
                        const int x,
                        const int y,
                        const int width,
                        const int height)
{
  PackingBlock block;
  block.name = name;
  block.shapes.emplace_back(width, height);
  block.fixed_x = x;
  block.fixed_y = y;
  return block;
}

}  // namespace

PackingProblem toPackingProblem(const odb::Rect& outline,
                                const std::vector<SoftMacro>& macros,
                                const int sequence_pair_macros,
                                std::vector<int>* macro_of_block)
{
  PackingProblem problem{outline.dx(), outline.dy(), {}};
  const int count = packedCount(sequence_pair_macros, macros.size());
  for (int i = 0; i < count; i++) {
    const SoftMacro& macro = macros[i];
    if (static_cast<int64_t>(macro.getWidth()) * macro.getHeight() == 0) {
      continue;  // takes no room
    }
    if (macro.isFixed()) {
      if (macro_of_block) {
        macro_of_block->push_back(i);
      }
      problem.blocks.push_back(fixedBlock(macro.getName(),
                                          macro.getX(),
                                          macro.getY(),
                                          macro.getWidth(),
                                          macro.getHeight()));
      continue;
    }
    PackingBlock block;
    block.name = macro.getName();
    const IntervalList& widths = macro.getWidthIntervals();
    const IntervalList& heights = macro.getHeightIntervals();
    if (widths.size() != heights.size()) {
      throw std::logic_error("toPackingProblem: " + macro.getName() + " has "
                             + std::to_string(widths.size()) + " widths and "
                             + std::to_string(heights.size()) + " heights");
    }
    bool rigid = true;
    for (size_t k = 0; k < widths.size(); k++) {
      rigid
          &= widths[k].min == widths[k].max && heights[k].min == heights[k].max;
    }
    if (widths.empty()) {
      // No shape curve: the annealer never resizes it.
      block.shapes.emplace_back(macro.getWidth(), macro.getHeight());
    } else if (rigid) {
      for (size_t k = 0; k < widths.size(); k++) {
        block.shapes.emplace_back(widths[k].min, heights[k].min);
      }
    } else {
      // SoftMacro::resizeRandomly: piece k keeps the area
      // min width * max height, and height = floor(area / width).
      for (size_t k = 0; k < widths.size(); k++) {
        block.soft_shapes.push_back(
            {widths[k], widths[k].min * static_cast<int64_t>(heights[k].max)});
      }
    }
    if (macro_of_block) {
      macro_of_block->push_back(i);
    }
    problem.blocks.push_back(block);
  }
  return problem;
}

PackingProblem toPackingProblem(const odb::Rect& outline,
                                const std::vector<HardMacro>& macros,
                                const int sequence_pair_macros,
                                std::vector<int>* macro_of_block)
{
  PackingProblem problem{outline.dx(), outline.dy(), {}};
  const int count = packedCount(sequence_pair_macros, macros.size());
  for (int i = 0; i < count; i++) {
    const HardMacro& macro = macros[i];
    if (macro.getArea() == 0) {
      continue;
    }
    if (macro.isFixed()) {
      if (macro_of_block) {
        macro_of_block->push_back(i);
      }
      problem.blocks.push_back(fixedBlock(macro.getName(),
                                          macro.getX(),
                                          macro.getY(),
                                          macro.getWidth(),
                                          macro.getHeight()));
      continue;
    }
    PackingBlock block;
    block.name = macro.getName();
    block.shapes.emplace_back(macro.getWidth(), macro.getHeight());
    if (macro_of_block) {
      macro_of_block->push_back(i);
    }
    problem.blocks.push_back(block);
  }
  return problem;
}

}  // namespace mpl
