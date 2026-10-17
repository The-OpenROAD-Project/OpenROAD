// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <QColor>
#include <QFont>

#include "gui/gui.h"

namespace odb {
class dbTechLayer;
class dbNet;
class dbInst;
class dbSite;
}  // namespace odb

namespace gui {

// Adds the Qt-typed half of the display options to the Qt-free web::Options
// predicates.  Implemented by DisplayControls and consumed by the Qt painters
// (GuiPainter, RenderThread), which hold a QtOptions* rather than going
// through web::Painter::getOptions().
class QtOptions : public web::Options
{
 public:
  virtual QColor background() = 0;
  virtual QColor color(const odb::dbTechLayer* layer) = 0;
  virtual Qt::BrushStyle pattern(const odb::dbTechLayer* layer) = 0;
  virtual QColor placementBlockageColor() = 0;
  virtual Qt::BrushStyle placementBlockagePattern() = 0;
  virtual QColor regionColor() = 0;
  virtual Qt::BrushStyle regionPattern() = 0;
  virtual QColor instanceNameColor() = 0;
  virtual QFont instanceNameFont() = 0;
  virtual QColor itermLabelColor() = 0;
  virtual QFont itermLabelFont() = 0;
  virtual QColor siteColor(odb::dbSite* site) = 0;
  virtual QFont ioPinMarkersFont() const = 0;
  virtual QColor rulerColor() = 0;
  virtual QFont rulerFont() = 0;
  virtual QFont labelFont() = 0;
};

}  // namespace gui
