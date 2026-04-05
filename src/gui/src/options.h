// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <QColor>
#include <QFont>

namespace odb {
class dbTechLayer;
class dbNet;
class dbInst;
class dbSite;
}  // namespace odb

namespace gui {

// This interface class provides access to the display options to
// clients who are drawing.
class Options
{
 public:
  virtual ~Options() = default;
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
  virtual bool isVisible(const odb::dbTechLayer* layer) = 0;
  virtual bool isSelectable(const odb::dbTechLayer* layer) = 0;
  virtual bool isNetVisible(odb::dbNet* net) = 0;
  virtual bool isNetSelectable(odb::dbNet* net) = 0;
  virtual bool isInstanceVisible(odb::dbInst* inst) = 0;
  virtual bool isInstanceSelectable(odb::dbInst* inst) = 0;
  virtual bool areInstanceNamesVisible() = 0;
  virtual bool areInstancePinsVisible() = 0;
  virtual bool areInstancePinsSelectable() = 0;
  virtual bool areInstancePinNamesVisible() = 0;
  virtual bool areInstanceBlockagesVisible() = 0;
  virtual bool areBlockagesVisible() = 0;
  virtual bool areBlockagesSelectable() = 0;
  virtual bool areObstructionsVisible() = 0;
  virtual bool areObstructionsSelectable() = 0;
  virtual bool areSitesVisible() = 0;
  virtual bool areSitesSelectable() = 0;
  virtual bool isSiteSelectable(odb::dbSite* site) = 0;
  virtual bool isSiteVisible(odb::dbSite* site) = 0;
  virtual bool arePrefTracksVisible() = 0;
  virtual bool areNonPrefTracksVisible() = 0;

  virtual bool areIOPinsVisible() const = 0;
  virtual bool areIOPinsSelectable() const = 0;
  virtual bool areIOPinNamesVisible() const = 0;
  virtual QFont ioPinMarkersFont() const = 0;

  virtual bool areRoutingSegmentsVisible() const = 0;
  virtual bool areRoutingViasVisible() const = 0;
  virtual bool areSpecialRoutingSegmentsVisible() const = 0;
  virtual bool areSpecialRoutingViasVisible() const = 0;
  virtual bool areFillsVisible() const = 0;

  virtual QColor rulerColor() = 0;
  virtual QFont rulerFont() = 0;
  virtual bool areRulersVisible() = 0;
  virtual bool areRulersSelectable() = 0;

  virtual QFont labelFont() = 0;
  virtual bool areLabelsVisible() = 0;
  virtual bool areLabelsSelectable() = 0;

  virtual bool isDetailedVisibility() = 0;

  virtual bool areSelectedVisible() = 0;

  virtual bool isScaleBarVisible() const = 0;
  virtual bool areAccessPointsVisible() const = 0;
  virtual bool areRegionsVisible() const = 0;
  virtual bool areRegionsSelectable() const = 0;
  virtual bool isManufacturingGridVisible() const = 0;

  virtual bool isModuleView() const = 0;

  virtual bool isGCellGridVisible() const = 0;
  virtual bool isFlywireHighlightOnly() const = 0;
  virtual bool areFocusedNetsGuidesVisible() const = 0;
};

}  // namespace gui
