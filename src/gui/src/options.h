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

namespace odb {
class dbTechLayer;
class dbNet;
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
  virtual bool isVisible(const odb::dbTechLayer* layer) = 0;
  virtual bool isSelectable(const odb::dbTechLayer* layer) = 0;
  virtual bool isNetVisible(odb::dbNet* net) = 0;
  virtual bool areFillsVisible() = 0;
  virtual bool areRowsVisible() = 0;
  virtual bool arePrefTracksVisible() = 0;
  virtual bool areNonPrefTracksVisible() = 0;

  virtual bool isCongestionVisible() const = 0;
  virtual bool arePinMarkersVisible() const = 0;
  virtual bool showHorizontalCongestion() const = 0;
  virtual bool showVerticalCongestion() const = 0;
  virtual float getMinCongestionToShow() const = 0;
  virtual float getMaxCongestionToShow() const = 0;
  virtual QColor getCongestionColor(float congestion) const = 0;
};

}  // namespace gui
