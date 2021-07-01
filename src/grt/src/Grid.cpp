/////////////////////////////////////////////////////////////////////////////
//
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
//
///////////////////////////////////////////////////////////////////////////////

#include "Grid.h"

#include <complex>

namespace grt {

void Grid::init(const long lowerLeftX,
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
                const std::map<int, std::vector<odb::Rect>>& obstructions)
{
  _lowerLeftX = lowerLeftX;
  _lowerLeftY = lowerLeftY;
  _upperRightX = upperRightX;
  _upperRightY = upperRightY;
  _tileWidth = tileWidth;
  _tileHeight = tileHeight;
  _xGrids = xGrids;
  _yGrids = yGrids;
  _perfectRegularX = perfectRegularX;
  _perfectRegularY = perfectRegularY;
  _numLayers = numLayers;
  _spacings = spacings;
  _minWidths = minWidths;
  _horizontalEdgesCapacities = horizontalCapacities;
  _verticalEdgesCapacities = verticalCapacities;
  _obstructions = obstructions;
}

void Grid::clear()
{
  _spacings.clear();
  _minWidths.clear();
  _horizontalEdgesCapacities.clear();
  _verticalEdgesCapacities.clear();
  _obstructions.clear();
}

odb::Point Grid::getPositionOnGrid(const odb::Point& position)
{
  int x = position.x();
  int y = position.y();

  // Computing x and y center:
  int gCellId_X = floor((float) ((x - _lowerLeftX) / _tileWidth));
  int gCellId_Y = floor((float) ((y - _lowerLeftY) / _tileHeight));

  if (gCellId_X >= _xGrids)
    gCellId_X--;

  if (gCellId_Y >= _yGrids)
    gCellId_Y--;

  int centerX = (gCellId_X * _tileWidth) + (_tileWidth / 2) + _lowerLeftX;
  int centerY = (gCellId_Y * _tileHeight) + (_tileHeight / 2) + _lowerLeftY;

  return odb::Point(centerX, centerY);
}

std::pair<Grid::TILE, Grid::TILE> Grid::getBlockedTiles(
    const odb::Rect& obstruction,
    odb::Rect& firstTileBds,
    odb::Rect& lastTileBds)
{
  std::pair<TILE, TILE> tiles;
  TILE firstTile;
  TILE lastTile;

  odb::Point lower = obstruction.ll();  // lower bound of obstruction
  odb::Point upper = obstruction.ur();  // upper bound of obstruction

  lower
      = getPositionOnGrid(lower);  // translate lower bound of obstruction to
                                   // the center of the tile where it is inside
  upper
      = getPositionOnGrid(upper);  // translate upper bound of obstruction to
                                   // the center of the tile where it is inside

  // Get x and y indices of first blocked tile
  firstTile._x = (lower.x() - (getTileWidth() / 2)) / getTileWidth();
  firstTile._y = (lower.y() - (getTileHeight() / 2)) / getTileHeight();

  // Get x and y indices of last blocked tile
  lastTile._x = (upper.x() - (getTileWidth() / 2)) / getTileWidth();
  lastTile._y = (upper.y() - (getTileHeight() / 2)) / getTileHeight();

  tiles = std::make_pair(firstTile, lastTile);

  odb::Point llFirstTile = odb::Point(lower.x() - (getTileWidth() / 2),
                                      lower.y() - (getTileHeight() / 2));
  odb::Point urFirstTile = odb::Point(lower.x() + (getTileWidth() / 2),
                                      lower.y() + (getTileHeight() / 2));

  odb::Point llLastTile = odb::Point(upper.x() - (getTileWidth() / 2),
                                     upper.y() - (getTileHeight() / 2));
  odb::Point urLastTile = odb::Point(upper.x() + (getTileWidth() / 2),
                                     upper.y() + (getTileHeight() / 2));

  if ((_upperRightX - urLastTile.x()) / getTileWidth() < 1) {
    urLastTile.setX(_upperRightX);
  }
  if ((_upperRightY - urLastTile.y()) / getTileHeight() < 1) {
    urLastTile.setY(_upperRightY);
  }

  firstTileBds = odb::Rect(llFirstTile, urFirstTile);
  lastTileBds = odb::Rect(llLastTile, urLastTile);

  return tiles;
}

int Grid::computeTileReduce(const odb::Rect& obs,
                            const odb::Rect& tile,
                            int trackSpace,
                            bool first,
                            bool direction)
{
  int reduce = -1;
  if (direction == RoutingLayer::VERTICAL) {
    if (obs.xMin() >= tile.xMin() && obs.xMax() <= tile.xMax()) {
      reduce = ceil(std::abs(obs.xMax() - obs.xMin()) / trackSpace);
    } else if (first) {
      reduce = ceil(std::abs(tile.xMax() - obs.xMin()) / trackSpace);
    } else {
      reduce = ceil(std::abs(obs.xMax() - tile.xMin()) / trackSpace);
    }
  } else {
    if (obs.yMin() >= tile.yMin() && obs.yMax() <= tile.yMax()) {
      reduce = ceil(std::abs(obs.yMax() - obs.yMin()) / trackSpace);
    } else if (first) {
      reduce = ceil(std::abs(tile.yMax() - obs.yMin()) / trackSpace);
    } else {
      reduce = ceil(std::abs(obs.yMax() - tile.yMin()) / trackSpace);
    }
  }

  return reduce;
}

odb::Point Grid::getMiddle()
{
  return odb::Point((_lowerLeftX + (_upperRightX - _lowerLeftX) / 2.0),
                    (_lowerLeftY + (_upperRightY - _lowerLeftY) / 2.0));
}

odb::Rect Grid::getGridArea() const
{
  return odb::Rect(_lowerLeftX, _lowerLeftY, _upperRightX, _upperRightY);
}

}  // namespace grt
