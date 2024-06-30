///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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
  virtual ~Options() {}
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
  virtual bool areRoutingSegmentsVisible() const = 0;
  virtual bool areRoutingViasVisible() const = 0;
  virtual bool areSpecialRoutingSegmentsVisible() const = 0;
  virtual bool areSpecialRoutingViasVisible() const = 0;
  virtual bool areFillsVisible() const = 0;
  virtual QFont pinMarkersFont() const = 0;

  virtual QColor rulerColor() = 0;
  virtual QFont rulerFont() = 0;
  virtual bool areRulersVisible() = 0;
  virtual bool areRulersSelectable() = 0;

  virtual bool isDetailedVisibility() = 0;

  virtual bool areSelectedVisible() = 0;

  virtual bool isScaleBarVisible() const = 0;
  virtual bool areAccessPointsVisible() const = 0;
  virtual bool areRegionsVisible() const = 0;
  virtual bool areRegionsSelectable() const = 0;
  virtual bool isManufacturingGridVisible() const = 0;

  virtual bool isModuleView() const = 0;

  virtual bool isGCellGridVisible() const = 0;
};

}  // namespace gui
