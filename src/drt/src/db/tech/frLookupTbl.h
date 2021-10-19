/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FR_LOOKUP_TBL_H_
#define _FR_LOOKUP_TBL_H_

// #include <map>
#include <iostream>

#include "frBaseTypes.h"

namespace fr {
enum class frInterpolateType
{
  frcSnapDown,
  frcSnapUp,
  frcLinear
};

enum class frExtrapolateType
{
  frcSnapDown,
  frcSnapUp,
  frcLinear
};

// fr1DLookupTbl
template <class rowClass, class valClass>
class fr1DLookupTbl
{
 public:
  // constructor

  fr1DLookupTbl(const fr1DLookupTbl& in)
      : rows(in.rows),
        vals(in.vals),
        rowName(in.rowName),
        interpolateTypeRow(in.interpolateTypeRow),
        interpolateTypeCol(in.interpolateTypeCol),
        extrapolateTypeRowLower(in.extrapolateTypeRowLower),
        extrapolateTypeRowUpper(in.extrapolateTypeRowUpper),
        lowerBound(in.lowerBound)
  {
  }
  fr1DLookupTbl(const frString& rowNameIn,
                const frCollection<rowClass>& rowsIn,
                const frCollection<valClass>& valsIn,
                bool lowerBoundIn = true)
  {
    rowName = rowNameIn;
    rows = rowsIn;
    vals = valsIn;
    interpolateTypeRow = frInterpolateType::frcSnapDown;
    interpolateTypeCol = frInterpolateType::frcSnapDown;
    extrapolateTypeRowLower = frExtrapolateType::frcSnapUp;
    extrapolateTypeRowUpper = frExtrapolateType::frcSnapUp;
    lowerBound = lowerBoundIn;
  }

  // getters
  frString getRowName() const { return rowName; }
  frCollection<rowClass> getRows() const { return rows; }
  frCollection<valClass> getValues() const { return vals; }

  // others
  valClass find(const rowClass& rowVal) const
  {
    valClass retVal;
    frUInt4 rowIdx = getRowIdx(rowVal);
    retVal = vals[rowIdx];
    return retVal;
  }
  valClass findMin() const { return vals.front(); }
  valClass findMax() const { return vals.back(); }
  rowClass getMinRow() const { return rows.front(); }
  rowClass getMaxRow() const { return rows.back(); }

 private:
  frUInt4 getRowIdx(const rowClass& rowVal) const
  {
    // currently only implement spacingtable style
    // interpolation
    frUInt4 retIdx;
    if (rowVal >= rows.front() && rowVal <= rows.back()) {
      if (lowerBound) {
        // if (interpolateTypeRow == frInterpolateType::frcSnapDown) {
        auto pos = lower_bound(rows.begin(), rows.end(), rowVal);
        // if (*pos != rowVal && pos != rows.begin()) {
        //   --pos;
        // }
        if (pos != rows.begin()) {
          --pos;
        }
        retIdx = pos - rows.begin();
      } else {
        auto pos = upper_bound(rows.begin(), rows.end(), rowVal);
        retIdx = std::min((frUInt4)(pos - rows.begin()), (frUInt4) rows.size());
      }
    }
    // lower extrapolation
    else if (rowVal < rows.front()) {
      if (true) {
        // if (extrapolateTypeRowLower == frExtrapolateType::frcSnapUp) {
        retIdx = 0;
      }
    }
    // upper extrapolation
    else {
      if (true) {
        // if (extrapolateTypeRowUpper == frExtrapolateType::frcSnapDown) {
        retIdx = rows.size() - 1;
      }
    }
    return retIdx;
  }

  frCollection<rowClass> rows;
  frCollection<valClass> vals;

  frString rowName;

  frInterpolateType interpolateTypeRow;
  frInterpolateType interpolateTypeCol;
  frExtrapolateType extrapolateTypeRowLower;
  frExtrapolateType extrapolateTypeRowUpper;
  bool lowerBound;
};

// fr2DLookupTbl
template <class rowClass, class colClass, class valClass>
class fr2DLookupTbl
{
 public:
  friend class frLef58SpacingTableConstraint;
  // constructor
  fr2DLookupTbl()
      : interpolateTypeRow(frInterpolateType::frcSnapDown),
        interpolateTypeCol(frInterpolateType::frcSnapDown),
        extrapolateTypeRowLower(frExtrapolateType::frcSnapDown),
        extrapolateTypeRowUpper(frExtrapolateType::frcSnapDown),
        extrapolateTypeColLower(frExtrapolateType::frcSnapDown),
        extrapolateTypeColUpper(frExtrapolateType::frcSnapDown)
  {
  }
  fr2DLookupTbl(const fr2DLookupTbl& in)
      : rows(in.rows),
        cols(in.cols),
        vals(in.vals),
        rowName(in.rowName),
        colName(in.colName),
        interpolateTypeRow(in.interpolateTypeRow),
        interpolateTypeCol(in.interpolateTypeCol),
        extrapolateTypeRowLower(in.extrapolateTypeRowLower),
        extrapolateTypeRowUpper(in.extrapolateTypeRowUpper),
        extrapolateTypeColLower(in.extrapolateTypeColLower),
        extrapolateTypeColUpper(in.extrapolateTypeColUpper)
  {
  }
  fr2DLookupTbl(const frString& rowNameIn,
                const frCollection<rowClass>& rowsIn,
                const frString& colNameIn,
                const frCollection<colClass>& colsIn,
                const frCollection<frCollection<valClass>>& valsIn)
  {
    rowName = rowNameIn;
    rows = rowsIn;
    colName = colNameIn;
    cols = colsIn;
    vals = valsIn;
    interpolateTypeRow = frInterpolateType::frcSnapDown;
    interpolateTypeCol = frInterpolateType::frcSnapDown;
    extrapolateTypeRowLower = frExtrapolateType::frcSnapUp;
    extrapolateTypeRowUpper = frExtrapolateType::frcSnapUp;
    extrapolateTypeColLower = frExtrapolateType::frcSnapDown;
    extrapolateTypeColUpper = frExtrapolateType::frcSnapDown;
  }

  // getters
  frString getRowName() const { return rowName; }
  frString getColName() const { return colName; }

  frCollection<rowClass> getRows() { return rows; }
  frCollection<colClass> getCols() { return cols; }
  frCollection<frCollection<valClass>> getValues() { return vals; }

  // others
  valClass find(const rowClass& rowVal, const colClass& colVal) const
  {
    valClass retVal;
    frUInt4 rowIdx = getRowIdx(rowVal);
    frUInt4 colIdx = getColIdx(colVal);
    // std::cout << "rowIdx = " << rowIdx << ", colIdx = " << colIdx <<
    // std::endl <<std::flush;
    retVal = vals[rowIdx][colIdx];
    return retVal;
  }
  valClass findMin() const { return vals.front().front(); }
  valClass findMax() const { return vals.back().back(); }

  // debug
  void printTbl() const
  {
    std::cout << "rowName: " << rowName << std::endl;
    for (auto& m : rows) {
      std::cout << m << " ";
    }
    std::cout << "\n colName: " << colName << std::endl;
    for (auto& m : cols) {
      std::cout << m << " ";
    }
    std::cout << "\n vals: " << std::endl;
    for (auto& m : vals) {
      for (auto& n : m) {
        std::cout << n << " ";
      }
      std::cout << std::endl;
    }
  }

 private:
  frUInt4 getRowIdx(const rowClass& rowVal) const
  {
    // currently only implement spacingtable style
    auto pos = --(std::lower_bound(rows.begin(), rows.end(), rowVal));
    return std::max(0, (int) std::distance(rows.begin(), pos));
  }
  frUInt4 getColIdx(const colClass& colVal) const
  {
    // currently only implement spacingtable style
    auto pos = --(std::lower_bound(cols.begin(), cols.end(), colVal));
    return std::max(0, (int) std::distance(cols.begin(), pos));
  }

  frCollection<rowClass> rows;
  frCollection<colClass> cols;
  frCollection<frCollection<valClass>> vals;

  frString rowName;
  frString colName;

  frInterpolateType interpolateTypeRow;
  frInterpolateType interpolateTypeCol;
  frExtrapolateType extrapolateTypeRowLower;
  frExtrapolateType extrapolateTypeRowUpper;
  frExtrapolateType extrapolateTypeColLower;
  frExtrapolateType extrapolateTypeColUpper;
};

}  // namespace fr

#endif
