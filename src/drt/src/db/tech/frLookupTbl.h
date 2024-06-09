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

#pragma once

#include <iostream>

#include "frBaseTypes.h"

namespace drt {
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
  fr1DLookupTbl() = default;

  fr1DLookupTbl(const fr1DLookupTbl& in)
      : rows_(in.rows_),
        vals_(in.vals_),
        rowName_(in.rowName_),
        interpolateTypeRow_(in.interpolateTypeRow_),
        interpolateTypeCol_(in.interpolateTypeCol_),
        extrapolateTypeRowLower_(in.extrapolateTypeRowLower_),
        extrapolateTypeRowUpper_(in.extrapolateTypeRowUpper_),
        lowerBound_(in.lowerBound_)
  {
  }
  fr1DLookupTbl(const frString& rowNameIn,
                const frCollection<rowClass>& rowsIn,
                const frCollection<valClass>& valsIn,
                bool lowerBoundIn = true)
      : rows_(rowsIn),
        vals_(valsIn),
        rowName_(rowNameIn),
        extrapolateTypeRowLower_(frExtrapolateType::frcSnapUp),
        extrapolateTypeRowUpper_(frExtrapolateType::frcSnapUp),
        lowerBound_(lowerBoundIn)
  {
  }

  // getters
  frString getRowName() const { return rowName_; }
  frCollection<rowClass> getRows() const { return rows_; }
  frCollection<valClass> getValues() const { return vals_; }

  // others
  valClass find(const rowClass& rowVal) const
  {
    frUInt4 rowIdx = getRowIdx(rowVal);
    return vals_[rowIdx];
  }
  valClass findMin() const { return vals_.front(); }
  valClass findMax() const { return vals_.back(); }
  rowClass getMinRow() const { return rows_.front(); }
  rowClass getMaxRow() const { return rows_.back(); }

 private:
  frUInt4 getRowIdx(const rowClass& rowVal) const
  {
    // currently only implement spacingtable style
    // interpolation
    frUInt4 retIdx;
    if (rowVal >= rows_.front() && rowVal <= rows_.back()) {
      if (lowerBound_) {
        auto pos = lower_bound(rows_.begin(), rows_.end(), rowVal);
        if (pos != rows_.begin()) {
          --pos;
        }
        retIdx = pos - rows_.begin();
      } else {
        auto pos = upper_bound(rows_.begin(), rows_.end(), rowVal);
        retIdx
            = std::min((frUInt4) (pos - rows_.begin()), (frUInt4) rows_.size());
      }
    } else if (rowVal < rows_.front()) {  // lower extrapolation
      retIdx = 0;
    } else {  // upper extrapolation
      retIdx = rows_.size() - 1;
    }
    return retIdx;
  }

  frCollection<rowClass> rows_;
  frCollection<valClass> vals_;

  frString rowName_;

  frInterpolateType interpolateTypeRow_{frInterpolateType::frcSnapDown};
  frInterpolateType interpolateTypeCol_{frInterpolateType::frcSnapDown};
  frExtrapolateType extrapolateTypeRowLower_{frExtrapolateType::frcSnapDown};
  frExtrapolateType extrapolateTypeRowUpper_{frExtrapolateType::frcSnapDown};
  bool lowerBound_{false};
};

// fr2DLookupTbl
template <class rowClass, class colClass, class valClass>
class fr2DLookupTbl
{
 public:
  friend class frLef58SpacingTableConstraint;
  // constructor
  fr2DLookupTbl() = default;
  fr2DLookupTbl(const fr2DLookupTbl& in)
      : rows_(in.rows_),
        cols_(in.cols_),
        vals_(in.vals_),
        rowName_(in.rowName_),
        colName_(in.colName_),
        interpolateTypeRow_(in.interpolateTypeRow_),
        interpolateTypeCol_(in.interpolateTypeCol_),
        extrapolateTypeRowLower_(in.extrapolateTypeRowLower_),
        extrapolateTypeRowUpper_(in.extrapolateTypeRowUpper_),
        extrapolateTypeColLower_(in.extrapolateTypeColLower_),
        extrapolateTypeColUpper_(in.extrapolateTypeColUpper_)
  {
  }
  fr2DLookupTbl(const frString& rowNameIn,
                const frCollection<rowClass>& rowsIn,
                const frString& colNameIn,
                const frCollection<colClass>& colsIn,
                const frCollection<frCollection<valClass>>& valsIn)
      : rows_(rowsIn),
        cols_(colsIn),
        vals_(valsIn),
        rowName_(rowNameIn),
        colName_(colNameIn),
        extrapolateTypeRowLower_(frExtrapolateType::frcSnapUp),
        extrapolateTypeRowUpper_(frExtrapolateType::frcSnapUp)
  {
  }

  // getters
  frString getRowName() const { return rowName_; }
  frString getColName() const { return colName_; }

  frCollection<rowClass> getRows() { return rows_; }
  frCollection<colClass> getCols() { return cols_; }
  frCollection<frCollection<valClass>> getValues() { return vals_; }

  // others
  valClass find(const rowClass& rowVal, const colClass& colVal) const
  {
    valClass retVal;
    frUInt4 rowIdx = getRowIdx(rowVal);
    frUInt4 colIdx = getColIdx(colVal);
    retVal = vals_[rowIdx][colIdx];
    return retVal;
  }
  valClass findMin() const { return vals_.front().front(); }
  valClass findMax() const { return vals_.back().back(); }

  // debug
  void printTbl() const
  {
    std::cout << "rowName: " << rowName_ << std::endl;
    for (auto& m : rows_) {
      std::cout << m << " ";
    }
    std::cout << "\n colName: " << colName_ << std::endl;
    for (auto& m : cols_) {
      std::cout << m << " ";
    }
    std::cout << "\n vals: " << std::endl;
    for (auto& m : vals_) {
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
    auto pos = --(std::lower_bound(rows_.begin(), rows_.end(), rowVal));
    return std::max(0, (int) std::distance(rows_.begin(), pos));
  }
  frUInt4 getColIdx(const colClass& colVal) const
  {
    // currently only implement spacingtable style
    auto pos = --(std::lower_bound(cols_.begin(), cols_.end(), colVal));
    return std::max(0, (int) std::distance(cols_.begin(), pos));
  }

  frCollection<rowClass> rows_;
  frCollection<colClass> cols_;
  frCollection<frCollection<valClass>> vals_;

  frString rowName_;
  frString colName_;

  frInterpolateType interpolateTypeRow_{frInterpolateType::frcSnapDown};
  frInterpolateType interpolateTypeCol_{frInterpolateType::frcSnapDown};
  frExtrapolateType extrapolateTypeRowLower_{frExtrapolateType::frcSnapDown};
  frExtrapolateType extrapolateTypeRowUpper_{frExtrapolateType::frcSnapDown};
  frExtrapolateType extrapolateTypeColLower_{frExtrapolateType::frcSnapDown};
  frExtrapolateType extrapolateTypeColUpper_{frExtrapolateType::frcSnapDown};
};

}  // namespace drt
