// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "DplObserver.h"
#include "gui/gui.h"
#include "odb/db.h"

namespace dpl {

using odb::dbBlock;
using odb::dbInst;
using odb::Point;
using odb::Rect;

class Opendp;
class Node;

class Graphics : public gui::Renderer, public DplObserver
{
 public:
  Graphics(Opendp* dp, float min_displacement, const dbInst* debug_instance);
  ~Graphics() override = default;
  void startPlacement(dbBlock* block) override;
  void placeInstance(dbInst* instance) override;
  void binSearch(const Node* cell,
                 GridX xl,
                 GridY yl,
                 GridX xh,
                 GridY yh) override;
  void endPlacement() override;

  // From Renderer API
  void drawObjects(gui::Painter& painter) override;

  static bool guiActive();

 private:
  Opendp* dp_;
  const dbInst* debug_instance_;
  dbBlock* block_ = nullptr;
  float min_displacement_;  // in row height
  std::vector<Rect> searched_;
};

}  // namespace dpl
