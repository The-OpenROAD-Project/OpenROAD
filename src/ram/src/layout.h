/////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Precision Innovations Inc.
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
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>

#include "odb/db.h"

namespace ram {

////////////////////////////////////////////////////////////////

class Layout;

class Element
{
 public:
  Element(odb::dbInst* inst);
  Element(std::unique_ptr<Layout> layout);

  // Return the bbox of the positioned element
  odb::Rect position(odb::Point origin);

 private:
  odb::dbInst* inst_ = nullptr;
  std::unique_ptr<Layout> layout_;
};

class Layout
{
 public:
  Layout(odb::Orientation2D orientation);

  void addElement(std::unique_ptr<Element> element);

  // Return the bbox of the positioned layout
  odb::Rect position(odb::Point origin, int offset = 0);

  std::vector<std::unique_ptr<Element>>& getElements ();

 private:
  odb::Orientation2D orientation_;
  std::vector<std::unique_ptr<Element>> elements_;
};

class Cell 
{
 public:
   Cell(odb::dbInst* inst = nullptr);

   void setOrigin (odb::Point global_pos);

   void setInstPosition();

   odb::Rect placeCell();

   void addInst (odb::dbInst* inst);

   int getWidth();

   int getHeight();

 private:
   std::vector<odb::dbInst*> insts_;
   odb::Point origin_;
   odb::Rect bbox;

};

class CellLayout
{
 public:
  CellLayout(odb::Orientation2D orientation);
  
  void setOrigin(odb::Point position);

  odb::Rect placeLayout();
  
  void addCell(std::unique_ptr<Cell> cell);

  int getWidth();

  int getHeight(); 

 private:
  odb::Orientation2D orientation_;
  odb::Point origin_;
  odb::Rect bbox;
  std::vector<std::unique_ptr<Cell>> cells_;
};

class Grid 
{
 public:
  Grid(odb::Orientation2D orientation);

 private:
  odb::Orientation2D orientation_;
  odb::Rect origin;
  std::vector<std::unique_ptr<CellLayout>> layouts_;

};


}  // namespace ram
