/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cmath>
#include <iostream>
#include <map>
#include <vector>

#include "RoutingLayer.h"
#include "opendb/db.h"

namespace grt {

class Grid
{
 private:
  long _lowerLeftX;
  long _lowerLeftY;
  long _upperRightX;
  long _upperRightY;
  long _tileWidth;
  long _tileHeight;
  int _xGrids;
  int _yGrids;
  bool _perfectRegularX;
  bool _perfectRegularY;
  int _numLayers;
  int _pitchesInTile = 15;
  std::vector<int> _spacings;
  std::vector<int> _minWidths;
  std::vector<int> _horizontalEdgesCapacities;
  std::vector<int> _verticalEdgesCapacities;
  std::map<int, std::vector<odb::Rect>> _obstructions;

 public:
  Grid() = default;
  ~Grid() = default;

  void init(const long lowerLeftX,
            const long lowerLeftY,
            const long upperRightX,
            const long upperRightY,
            const long tileWidth,
            const long tileHeight,
            const int xGrids,
            const int yGrids,
            const bool perfectRegularX,
            const bool perfectRegularY,
            const int numLayers,
            const std::vector<int>& spacings,
            const std::vector<int>& minWidths,
            const std::vector<int>& horizontalCapacities,
            const std::vector<int>& verticalCapacities,
            const std::map<int, std::vector<odb::Rect>>& obstructions);

  typedef struct
  {
    int _x;
    int _y;
  } TILE;

  void clear();

  long getLowerLeftX() const { return _lowerLeftX; }
  long getLowerLeftY() const { return _lowerLeftY; }

  void setLowerLeftX(long x) { _lowerLeftX = x; }
  void setLowerLeftY(long y) { _lowerLeftY = y; }

  long getUpperRightX() const { return _upperRightX; }
  long getUpperRightY() const { return _upperRightY; }

  long getTileWidth() const { return _tileWidth; }
  long getTileHeight() const { return _tileHeight; }

  int getXGrids() const { return _xGrids; }
  int getYGrids() const { return _yGrids; }

  bool isPerfectRegularX() const { return _perfectRegularX; }
  bool isPerfectRegularY() const { return _perfectRegularY; }

  int getNumLayers() const { return _numLayers; }

  void setPitchesInTile(const int pitchesInTile)
  {
    _pitchesInTile = pitchesInTile;
  }
  int getPitchesInTile() const { return _pitchesInTile; }

  const std::vector<int>& getSpacings() const { return _spacings; }
  const std::vector<int>& getMinWidths() const { return _minWidths; }

  void addSpacing(int value, int layer) { _spacings[layer] = value; }
  void addMinWidth(int value, int layer) { _minWidths[layer] = value; }

  const std::vector<int>& getHorizontalEdgesCapacities()
  {
    return _horizontalEdgesCapacities;
  };
  const std::vector<int>& getVerticalEdgesCapacities()
  {
    return _verticalEdgesCapacities;
  };

  void addHorizontalCapacity(int value, int layer)
  {
    _horizontalEdgesCapacities[layer] = value;
  }
  void addVerticalCapacity(int value, int layer)
  {
    _verticalEdgesCapacities[layer] = value;
  }

  void updateHorizontalEdgesCapacities(int layer, int reduction)
  {
    _horizontalEdgesCapacities[layer] = reduction;
  };
  void updateVerticalEdgesCapacities(int layer, int reduction)
  {
    _verticalEdgesCapacities[layer] = reduction;
  };

  const std::map<int, std::vector<odb::Rect>>& getAllObstructions() const
  {
    return _obstructions;
  }
  void addObstruction(int layer, odb::Rect obstruction)
  {
    _obstructions[layer].push_back(obstruction);
  }

  odb::Point getPositionOnGrid(const odb::Point& position);

  std::pair<TILE, TILE> getBlockedTiles(const odb::Rect& obstruction,
                                        odb::Rect& firstTileBds,
                                        odb::Rect& lastTileBds);

  int computeTileReduce(const odb::Rect& obs,
                        const odb::Rect& tile,
                        int trackSpace,
                        bool first,
                        bool direction);

  odb::Point getMiddle();
  odb::Rect getGridArea() const;
};

}  // namespace grt
