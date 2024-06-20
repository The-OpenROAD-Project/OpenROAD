///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, Precision Innovations Inc.
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

#include <cassert>
// #define _CRTDBG_MAP_ALLOC

#pragma once

#include <vector>

#include "odb/db.h"

namespace grt {

class GlobalRouter;

class Rudy
{
 public:
  class Tile
  {
   public:
    odb::Rect getRect() const { return rect_; }
    void setRect(int lx, int ly, int ux, int uy);
    void addRudy(float rudy);
    float getRudy() const { return rudy_; }
    void clearRudy() { rudy_ = 0.0; }

   private:
    odb::Rect rect_;
    float rudy_ = 0;
  };

  explicit Rudy(odb::dbBlock* block, grt::GlobalRouter* grouter);

  /**
   * \pre we need to call this function after `setGridConfig` and
   * `setWireWidth`.
   * */
  void calculateRudy();

  /**
   * Set the grid area and grid numbers.
   * Default value will be the die area of block and (40, 40), respectively.
   * */
  void setGridConfig(odb::Rect block, int tile_cnt_x, int tile_cnt_y);

  /**
   * Set the wire length for calculate Rudy.
   * If the layer which name is metal1 and it has getWidth value, then this
   * function will not applied, but it will apply that information.
   * */
  void setWireWidth(int wire_width) { wire_width_ = wire_width; }

  const Tile& getTile(int x, int y) const { return grid_.at(x).at(y); }
  std::pair<int, int> getGridSize() const;
  int getTileSize() const { return tile_size_; }

 private:
  /**
   * \pre This function should be called after `setGridConfig`
   * */
  void makeGrid();
  void getResourceReductions();
  Tile& getEditableTile(int x, int y) { return grid_.at(x).at(y); }
  void processIntersectionSignalNet(odb::Rect net_rect);

  odb::dbBlock* block_;
  odb::Rect grid_block_;
  GlobalRouter* grouter_;
  int tile_cnt_x_ = 40;
  int tile_cnt_y_ = 40;
  int wire_width_ = 100;
  int tile_size_ = 0;
  std::vector<std::vector<Tile>> grid_;
};

}  // namespace grt
