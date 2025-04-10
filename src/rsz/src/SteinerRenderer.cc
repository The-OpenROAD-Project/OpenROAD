// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "SteinerRenderer.h"

#include "SteinerTree.hh"

namespace rsz {

SteinerRenderer::SteinerRenderer()
{
  gui::Gui::get()->registerRenderer(this);
}

void SteinerRenderer::highlight(SteinerTree* tree)
{
  tree_ = tree;
}

void SteinerRenderer::drawObjects(gui::Painter& painter)
{
  if (tree_) {
    painter.setPen(gui::Painter::red, true);
    for (int i = 0; i < tree_->branchCount(); ++i) {
      Point pt1, pt2;
      int steiner_pt1, steiner_pt2;
      int wire_length;
      tree_->branch(i, pt1, steiner_pt1, pt2, steiner_pt2, wire_length);
      painter.drawLine(pt1, pt2);
    }
  }
}

}  // namespace rsz
