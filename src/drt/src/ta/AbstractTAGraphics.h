// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include <memory>
#include <vector>

#include "FlexTA.h"
#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"

namespace gui {
class Painter;
}

namespace drt {
class AbstractTAGraphics
{
 public:
  // Show a message in the status bar
  virtual void status(const std::string& message) = 0;

  // Draw iroutes for one guide
  virtual void drawIrouteGuide(frNet* net,
                               odb::dbTechLayer* layer,
                               gui::Painter& painter)
      = 0;

  // From Renderer API
  virtual void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) = 0;

  // Update status and optionally pause
  virtual void endIter(int iter) = 0;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();
};

}  // namespace drt