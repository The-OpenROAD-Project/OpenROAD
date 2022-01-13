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

#pragma once

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include <algorithm>
#include <cmath>
#include <limits>

namespace dpo {

template<typename T>
class Rectangle_b {
 public:
  Rectangle_b() {
    reset();
  }
  Rectangle_b(T xmin, T ymin, T xmax, T ymax)
      : m_xmin(xmin), m_ymin(ymin), m_xmax(xmax), m_ymax(ymax) {}
  Rectangle_b(const Rectangle_b& rect)
      : m_xmin(rect.m_xmin),
        m_ymin(rect.m_ymin),
        m_xmax(rect.m_xmax),
        m_ymax(rect.m_ymax) {}
  Rectangle_b& operator=(const Rectangle_b& other) {
    if (this != &other) {
      m_xmin = other.m_xmin;
      m_xmax = other.m_xmax;
      m_ymin = other.m_ymin;
      m_ymax = other.m_ymax;
    }
    return *this;
  }
  virtual ~Rectangle_b() {}

  void reset() {
    m_xmin = std::numeric_limits<T>::max();
    m_ymin = std::numeric_limits<T>::max();
    m_xmax = std::numeric_limits<T>::lowest();
    m_ymax = std::numeric_limits<T>::lowest();
  }

  void enlarge(const Rectangle_b& r) {
    m_xmin = (m_xmin > r.m_xmin ? r.m_xmin : m_xmin);
    m_ymin = (m_ymin > r.m_ymin ? r.m_ymin : m_ymin);
    m_xmax = (m_xmax < r.m_xmax ? r.m_xmax : m_xmax);
    m_ymax = (m_ymax < r.m_ymax ? r.m_ymax : m_ymax);
  }
  bool intersects(const Rectangle_b& r) const {
    return !(m_xmin > r.m_xmax || m_xmax < r.m_xmin || m_ymin > r.m_ymax ||
             m_ymax < r.m_ymin);
  }
  bool is_overlap(T xmin, T ymin, T xmax, T ymax) {
    if (xmin >= m_xmax) return false;
    if (xmax <= m_xmin) return false;
    if (ymin >= m_ymax) return false;
    if (ymax <= m_ymin) return false;
    return true;
  }

  bool contains(const Rectangle_b& r) {
    if (r.m_xmin >= m_xmin && r.m_xmax <= m_xmax && r.m_ymin >= m_ymin &&
        r.m_ymax <= m_ymax) {
      return true;
    }
    return false;
  }

  void addPt(T x, T y) {
    m_xmin = std::min(m_xmin, x);
    m_xmax = std::max(m_xmax, x);
    m_ymin = std::min(m_ymin, y);
    m_ymax = std::max(m_ymax, y);
  }
  T getCenterX() { return 0.5 * (m_xmax + m_xmin); }
  T getCenterY() { return 0.5 * (m_ymax + m_ymin); }
  T getWidth() { return m_xmax - m_xmin; }
  T getHeight() { return m_ymax - m_ymin; }
  void clear() { reset(); }

  T xmin() const { return m_xmin; }
  T xmax() const { return m_xmax; }
  T ymin() const { return m_ymin; }
  T ymax() const { return m_ymax; }

  T area() const { return (m_xmax - m_xmin) * (m_ymax - m_ymin); }

  void set_xmin(T val) { m_xmin = val; }
  void set_xmax(T val) { m_xmax = val; }
  void set_ymin(T val) { m_ymin = val; }
  void set_ymax(T val) { m_ymax = val; }

 private:
  T m_xmin;
  T m_ymin;
  T m_xmax;
  T m_ymax;
};

using Rectangle = Rectangle_b<double>; // Legacy.

using Rectangle_d = Rectangle_b<double>;
using Rectangle_i = Rectangle_b<int>;

}  // namespace dpo
