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

#include "Grid.h"

#include <complex>

namespace FastRoute {

Coordinate Grid::getPositionOnGrid(const Coordinate& position)
{
  DBU x = position.getX();
  DBU y = position.getY();

  // Computing x and y center:
  int gCellId_X = floor((float) ((x - _lowerLeftX) / _tileWidth));
  int gCellId_Y = floor((float) ((y - _lowerLeftY) / _tileHeight));

  if (gCellId_X >= _xGrids)
    gCellId_X--;

  if (gCellId_Y >= _yGrids)
    gCellId_Y--;

  DBU centerX = (gCellId_X * _tileWidth) + (_tileWidth / 2) + _lowerLeftX;
  DBU centerY = (gCellId_Y * _tileHeight) + (_tileHeight / 2) + _lowerLeftY;

  return Coordinate(centerX, centerY);
}

std::pair<Grid::TILE, Grid::TILE> Grid::getBlockedTiles(const Box& obstacle,
                                                        Box& firstTileBds,
                                                        Box& lastTileBds)
{
  std::pair<TILE, TILE> tiles;
  TILE firstTile;
  TILE lastTile;

  Coordinate lower = obstacle.getLowerBound();  // lower bound of obstacle
  Coordinate upper = obstacle.getUpperBound();  // upper bound of obstacle

  lower = getPositionOnGrid(lower);  // translate lower bound of obstacle to the
                                     // center of the tile where it is inside
  upper = getPositionOnGrid(upper);  // translate upper bound of obstacle to the
                                     // center of the tile where it is inside

  // Get x and y indices of first blocked tile
  firstTile._x = (lower.getX() - (getTileWidth() / 2)) / getTileWidth();
  firstTile._y = (lower.getY() - (getTileHeight() / 2)) / getTileHeight();

  // Get x and y indices of last blocked tile
  lastTile._x = (upper.getX() - (getTileWidth() / 2)) / getTileWidth();
  lastTile._y = (upper.getY() - (getTileHeight() / 2)) / getTileHeight();

  tiles = std::make_pair(firstTile, lastTile);

  Coordinate llFirstTile = Coordinate(lower.getX() - (getTileWidth() / 2),
                                      lower.getY() - (getTileHeight() / 2));
  Coordinate urFirstTile = Coordinate(lower.getX() + (getTileWidth() / 2),
                                      lower.getY() + (getTileHeight() / 2));

  Coordinate llLastTile = Coordinate(upper.getX() - (getTileWidth() / 2),
                                     upper.getY() - (getTileHeight() / 2));
  Coordinate urLastTile = Coordinate(upper.getX() + (getTileWidth() / 2),
                                     upper.getY() + (getTileHeight() / 2));

  if ((_upperRightX - urLastTile.getX()) / getTileWidth() < 1) {
    urLastTile.setX(_upperRightX);
  }
  if ((_upperRightY - urLastTile.getY()) / getTileHeight() < 1) {
    urLastTile.setY(_upperRightY);
  }

  firstTileBds = Box(llFirstTile, urFirstTile, -1);
  lastTileBds = Box(llLastTile, urLastTile, -1);

  return tiles;
}

int Grid::computeTileReduce(const Box& obs,
                            const Box& tile,
                            int trackSpace,
                            bool first,
                            bool direction)
{
  int reduce = -1;
  if (direction == RoutingLayer::VERTICAL) {
    if (obs.getLowerBound().getX() >= tile.getLowerBound().getX()
        && obs.getUpperBound().getX() <= tile.getUpperBound().getX()) {
      reduce = ceil(
          std::abs(obs.getUpperBound().getX() - obs.getLowerBound().getX())
          / trackSpace);
    } else if (first) {
      reduce = ceil(
          std::abs(tile.getUpperBound().getX() - obs.getLowerBound().getX())
          / trackSpace);
    } else {
      reduce = ceil(
          std::abs(obs.getUpperBound().getX() - tile.getLowerBound().getX())
          / trackSpace);
    }
  } else {
    if (obs.getLowerBound().getY() >= tile.getLowerBound().getY()
        && obs.getUpperBound().getY() <= tile.getUpperBound().getY()) {
      reduce = ceil(
          std::abs(obs.getUpperBound().getY() - obs.getLowerBound().getY())
          / trackSpace);
    } else if (first) {
      reduce = ceil(
          std::abs(tile.getUpperBound().getY() - obs.getLowerBound().getY())
          / trackSpace);
    } else {
      reduce = ceil(
          std::abs(obs.getUpperBound().getY() - tile.getLowerBound().getY())
          / trackSpace);
    }
  }

  if (reduce < 0) {
    std::cout << "[WARNING] Invalid reduction\n";
  }
  return reduce;
}

Coordinate Grid::getMiddle()
{
  return Coordinate((_lowerLeftX + (_upperRightX - _lowerLeftX) / 2.0),
                    (_lowerLeftY + (_upperRightY - _lowerLeftY) / 2.0));
}

}  // namespace FastRoute
