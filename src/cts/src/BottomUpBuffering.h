// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace cts {

struct BufferingPoint
{
  double x = 0;
  double y = 0;
};

struct BufferingChoice
{
  std::string master;
  double input_cap = 0;
};

struct BufferingNode
{
  BufferingPoint point;
  bool sink = false;
  double input_cap = 0;
  std::string master;
  std::string fixed_master;
  std::vector<size_t> children;
};

struct BufferingLoad
{
  double pin_cap = 0;
  double wire_cap = 0;
  double max_wire_length = 0;
  size_t fanout = 0;

  double capacitance() const { return pin_cap + wire_cap; }
};

// Geometry is supplied top-down. Cell selection and additional stages are
// decided strictly in postorder, after every child's input cap is known.
class BottomUpBuffering
{
 public:
  using Selector = std::function<std::optional<BufferingChoice>(
      const BufferingLoad&,
      const std::string& fixed_master)>;
  using Legalizer = std::function<BufferingPoint(BufferingPoint)>;

  BottomUpBuffering(double cap_per_dbu,
                    double max_wire_length,
                    Selector selector,
                    Legalizer legalizer = {});

  // Throws on an invalid tree or an infeasible electrical load. No partially
  // buffered result is returned. The caller's input is left unchanged.
  std::vector<BufferingNode> run(std::vector<BufferingNode> nodes, size_t root);
  BufferingLoad load(const std::vector<BufferingNode>& nodes,
                     size_t driver) const;

 private:
  void buffer(size_t index, unsigned depth);
  size_t addStage(BufferingPoint point, std::vector<size_t> children);
  void split(size_t index, unsigned depth);

  double cap_per_dbu_;
  double max_wire_length_;
  Selector selector_;
  Legalizer legalizer_;
  size_t node_limit_ = 0;
  std::vector<BufferingNode> nodes_;
};

}  // namespace cts
