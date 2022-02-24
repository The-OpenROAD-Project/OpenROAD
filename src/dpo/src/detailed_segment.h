///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class DetailedSeg {
 public:
  DetailedSeg()
      : m_segId(-1),
        m_rowId(-1),
        m_regId(0),
        m_xmin(std::numeric_limits<int>::max()),
        m_xmax(std::numeric_limits<int>::lowest()),
        m_util(0.0) {}
        //m_gapu(0.0)
  virtual ~DetailedSeg() {}

  void setSegId(int segId) { m_segId = segId; }
  int getSegId() const { return m_segId; }

  void setRowId(int rowId) { m_rowId = rowId; }
  int getRowId() const { return m_rowId; }

  void setRegId(int regId) { m_regId = regId; }
  int getRegId() const { return m_regId; }

  void setMinX(int xmin) { m_xmin = xmin; }
  int getMinX() const { return m_xmin; }

  void setMaxX(int xmax) { m_xmax = xmax; }
  int getMaxX() const { return m_xmax; }

  int getWidth() const { return m_xmax-m_xmin; }

  void setUtil(int util) { m_util = util; }
  int getUtil() const { return m_util; }
  void addUtil(int amt) { m_util += amt; }
  void remUtil(int amt) { m_util -= amt; }

 protected:
  // Some identifiers...
  int m_segId;
  int m_rowId;
  int m_regId; 
  // Span of segment...
  int m_xmin;
  int m_xmax;
  // Total width of cells in segment...
  int m_util;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

}  // namespace dpo
