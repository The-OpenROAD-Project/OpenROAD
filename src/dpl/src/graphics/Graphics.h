// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "DplObserver.h"
#include "dpl/Opendp.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace dpl {

class Opendp;
class Node;

class Graphics : public gui::Renderer, public DplObserver
{
 public:
  Graphics(Opendp* dp,
           float min_displacement,
           const odb::dbInst* debug_instance);
  ~Graphics() override = default;
  void startPlacement(odb::dbBlock* block) override;
  void placeInstance(odb::dbInst* instance) override;
  void binSearch(const Node* cell,
                 GridX xl,
                 GridY yl,
                 GridX xh,
                 GridY yh) override;
  void redrawAndPause() override;

  // From Renderer API
  void drawObjects(gui::Painter& painter) override;

  static bool guiActive();

 private:
  Opendp* dp_;
  const odb::dbInst* debug_instance_;
  odb::dbBlock* block_ = nullptr;
  float min_displacement_;  // in row height
  std::vector<odb::Rect> searched_;
};

}  // namespace dpl
