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
// File: rectangle.h
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <algorithm>
#include <cmath>
#include <limits>

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////

class Rectangle {
 public:
  Rectangle() {
    reset();
  }
  Rectangle(double xmin, double ymin, double xmax, double ymax)
      : m_xmin(xmin), m_ymin(ymin), m_xmax(xmax), m_ymax(ymax) {}
  Rectangle(const Rectangle& rect)
      : m_xmin(rect.m_xmin),
        m_ymin(rect.m_ymin),
        m_xmax(rect.m_xmax),
        m_ymax(rect.m_ymax) {}
  Rectangle& operator=(const Rectangle& other) {
    if (this != &other) {
      m_xmin = other.m_xmin;
      m_xmax = other.m_xmax;
      m_ymin = other.m_ymin;
      m_ymax = other.m_ymax;
    }
    return *this;
  }
  virtual ~Rectangle() {}

  void reset() {
    m_xmin = std::numeric_limits<double>::max();
    m_ymin = std::numeric_limits<double>::max();
    m_xmax = std::numeric_limits<double>::lowest();
    m_ymax = std::numeric_limits<double>::lowest();
  }

  void enlarge(const Rectangle& r) {
    m_xmin = (m_xmin > r.m_xmin ? r.m_xmin : m_xmin);
    m_ymin = (m_ymin > r.m_ymin ? r.m_ymin : m_ymin);
    m_xmax = (m_xmax < r.m_xmax ? r.m_xmax : m_xmax);
    m_ymax = (m_ymax < r.m_ymax ? r.m_ymax : m_ymax);
  }
  bool intersects(const Rectangle& r) const {
    return !(m_xmin > r.m_xmax || m_xmax < r.m_xmin || m_ymin > r.m_ymax ||
             m_ymax < r.m_ymin);
  }
  bool is_overlap(double xmin, double ymin, double xmax, double ymax) {
    if (xmin >= m_xmax) return false;
    if (xmax <= m_xmin) return false;
    if (ymin >= m_ymax) return false;
    if (ymax <= m_ymin) return false;
    return true;
  }

  bool contains(const Rectangle& r) {
    if (r.m_xmin >= m_xmin && r.m_xmax <= m_xmax && r.m_ymin >= m_ymin &&
        r.m_ymax <= m_ymax) {
      return true;
    }
    return false;
  }

  void addPt(double x, double y) {
    m_xmin = std::min(m_xmin, x);
    m_xmax = std::max(m_xmax, x);
    m_ymin = std::min(m_ymin, y);
    m_ymax = std::max(m_ymax, y);
  }
  double getCenterX() { return 0.5 * (m_xmax + m_xmin); }
  double getCenterY() { return 0.5 * (m_ymax + m_ymin); }
  double getWidth() { return m_xmax - m_xmin; }
  double getHeight() { return m_ymax - m_ymin; }
  void clear() {
    m_xmin = std::numeric_limits<double>::max();
    m_ymin = std::numeric_limits<double>::max();
    m_xmax = -std::numeric_limits<double>::max();
    m_ymax = -std::numeric_limits<double>::max();
  }

  double xmin() const { return m_xmin; }
  double xmax() const { return m_xmax; }
  double ymin() const { return m_ymin; }
  double ymax() const { return m_ymax; }

  double area() const { return (m_xmax - m_xmin) * (m_ymax - m_ymin); }

  void set_xmin(double val) { m_xmin = val; }
  void set_xmax(double val) { m_xmax = val; }
  void set_ymin(double val) { m_ymin = val; }
  void set_ymax(double val) { m_ymax = val; }

 private:
  double m_xmin;
  double m_ymin;
  double m_xmax;
  double m_ymax;
};

}  // namespace dpo
