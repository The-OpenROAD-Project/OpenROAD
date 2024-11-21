// Copyright 2019-2023 The Regents of the University of California, Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include "AbstractPathRenderer.h"
#include "db_sta/dbSta.hh"
#include "gui/gui.h"

namespace sta {

class PathExpanded;

class PathRenderer : public gui::Renderer, public AbstractPathRenderer
{
 public:
  explicit PathRenderer(dbSta* sta);
  ~PathRenderer() override;

  void highlight(PathRef* path) override;

  void drawObjects(gui::Painter& /* painter */) override;

 private:
  void highlightInst(const Pin* pin, gui::Painter& painter);

  dbSta* sta_;
  // Expanded path is owned by PathRenderer.
  std::unique_ptr<PathExpanded> path_;
  static const gui::Painter::Color signal_color;
  static const gui::Painter::Color clock_color;
};

}  // namespace sta
