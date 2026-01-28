// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

// See https://home.engineering.iastate.edu/~cnchu/pubs/j29.pdf for algorithm
// details.
// POWV = potentially optimal wirelength vectors
// POST = potentially optimal Steiner tree

#include "stt/flute.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "boost/multi_array.hpp"
#include "stt/SteinerTreeBuilder.h"
#include "utl/decode.h"

namespace stt::flt {

/*****************************/
/*  User-Defined Parameters  */
/*****************************/
// true to construct routing, false to estimate WL only
static constexpr bool kConstructRouting = true;

// Suggestion: Set to true if ACCURACY >= 5
static constexpr bool kLocalRefinement = true;

// LUT is used for d <= kMaxLutDegree
static constexpr int kMaxLutDegree = 9;
static_assert(kMaxLutDegree <= 9, "Max LUT degree is 9.");

static consteval size_t factorial(const size_t n)
{
  return (n <= 1) ? 1 : n * factorial(n - 1);
}

static constexpr int get_max_powv(const int d)
{
  if (d <= 7) {
    return 15;
  }
  if (d == 8) {
    return 33;
  }
  return 79;
}

static constexpr int kMaxGroup = factorial(std::max(kMaxLutDegree, 7));
static constexpr int kMaxPowv = get_max_powv(kMaxLutDegree);

struct Flute::Csoln
{
  unsigned char parent;
  // Add: 0..i, Sub: j..10; seg[i+1]=seg[j-1]=0
  unsigned char seg[11];
  // row = rowcol[]/16, col = rowcol[]%16,
  unsigned char rowcol[kMaxLutDegree - 2];
  unsigned char neighbor[2 * kMaxLutDegree - 2];
};

struct Point
{
  int x, y;
  int o;
};

extern const char* post9[];
extern const char* powv9[];

////////////////////////////////////////////////////////////////

void Flute::readLUT()
{
  makeLUT(lut_, numsoln_);

  // Only init to d=8 on startup because d=9 is big and slow.
  initLUT(kLutInitialDegree, lut_, numsoln_);
}

void Flute::makeLUT(LutType& lut, NumSoln& numsoln)
{
  lut = new boost::multi_array<std::shared_ptr<Csoln[]>, 2>(
      // NOLINTNEXTLINE(misc-include-cleaner)
      boost::extents[kMaxLutDegree + 1][kMaxGroup]);
  numsoln = new int*[kMaxLutDegree + 1];
  for (int d = 4; d <= kMaxLutDegree; d++) {
    numsoln[d] = new int[kMaxGroup];
  }
}

void Flute::deleteLUT()
{
  deleteLUT(lut_, numsoln_);
}

void Flute::deleteLUT(LutType& lut, NumSoln& numsoln)
{
  if (lut) {
    delete lut;
    for (int d = 4; d <= kMaxLutDegree; d++) {
      delete[] numsoln[d];
    }
    delete[] numsoln;
  }
}

static unsigned char charNum(const unsigned char c)
{
  if (isdigit(c)) {
    return c - '0';
  }
  if (c >= 'A') {
    return c - 'A' + 10;
  }
  return 0;
}

static const char* readDecimalInt(const char* s, int& value)
{
  value = 0;
  const bool negative = (*s == '-');
  if (negative || *s == '+') {
    ++s;
  }
  constexpr int zero_code = int('0');
  while (*s >= '0' && *s <= '9') {
    value = 10 * value + (int(*s) - zero_code);
    ++s;
  }
  if (negative) {
    value = -value;
  }
  return s;
}

// Init LUTs from base64 encoded string variables.
void Flute::initLUT(const int to_d, LutType lut, NumSoln numsoln)
{
  const std::string pwv_string = utl::base64_decode(powv9);
  const char* pwv = pwv_string.c_str();

  std::string prt_string;
  const char* prt = nullptr;
  if (kConstructRouting) {
    prt_string = utl::base64_decode(post9);
    prt = prt_string.c_str();
  }

  for (int d = 4; d <= to_d; d++) {
    if (pwv[0] == 'd' && pwv[1] == '=') {
      pwv = readDecimalInt(pwv + 2, d);
    }
    ++pwv;
    if (kConstructRouting) {
      if (prt[0] == 'd' && prt[1] == '=') {
        prt = readDecimalInt(prt + 2, d);
      }
      ++prt;
    }
    for (int k = 0; k < kNumGroup[d]; k++) {
      const int ns = charNum(*pwv++);
      if (ns == 0) {  // same as some previous group
        int kk;
        pwv = readDecimalInt(pwv, kk) + 1;
        numsoln[d][k] = numsoln[d][kk];
        (*lut)[d][k] = (*lut)[d][kk];
      } else {
        pwv++;  // '\n'
        numsoln[d][k] = ns;
        Csoln* p = new Csoln[ns];
        (*lut)[d][k] = std::shared_ptr<Csoln[]>(p);
        for (int i = 1; i <= ns; i++) {
          p->parent = charNum(*pwv++);

          int j = 0;
          unsigned char ch, seg;
          do {
            ch = *pwv++;
            seg = charNum(ch);
            p->seg[j++] = seg;
          } while (seg != 0);

          j = 10;
          if (ch == '\n') {
            p->seg[j] = 0;
          } else {
            do {
              ch = *pwv++;
              seg = charNum(ch);
              p->seg[j--] = seg;
            } while (seg != 0);
          }

          if (kConstructRouting) {
            const int nn = 2 * d - 2;
            for (int j = d; j < nn; j++) {
              p->rowcol[j - d] = charNum(*prt++);
            }

            for (int j = 0; j < nn;) {
              unsigned char c = *prt++;
              p->neighbor[j++] = c / 16;
              p->neighbor[j++] = c % 16;
            }
            prt++;  // \n
          }
          p++;
        }
      }
    }
  }
  lut_valid_d_ = to_d;
}

void Flute::ensureLUT(const int d)
{
  if (lut_ == nullptr) {
    readLUT();
  }
  if (d > lut_valid_d_ && d <= kMaxLutDegree) {
    initLUT(kMaxLutDegree, lut_, numsoln_);
  }
}

////////////////////////////////////////////////////////////////

int Flute::flute_wl(const int d,
                    const std::vector<int>& x,
                    const std::vector<int>& y,
                    const int acc)
{
  if (d <= 1) {
    return 0;
  }

  if (d == 2) {
    return std::abs(x[0] - x[1]) + std::abs(y[0] - y[1]);
  }

  if (d == 3) {
    const auto [xl, xu] = std::ranges::minmax(x);
    const auto [yl, yu] = std::ranges::minmax(y);
    return (xu - xl) + (yu - yl);
  }

  ensureLUT(d);

  /* allocate the dynamic pieces on the heap rather than the stack */
  std::vector<Point> pt(d + 1);
  std::vector<Point*> ptp(d + 1);

  for (int i = 0; i < d; i++) {
    pt[i].x = x[i];
    pt[i].y = y[i];
    ptp[i] = &pt[i];
  }

  // sort x
  std::sort(ptp.begin(), ptp.begin() + d, [](const Point* a, const Point* b) {
    return a->x < b->x;
  });

  std::vector<int> xs(d);
  for (int i = 0; i < d; i++) {
    xs[i] = ptp[i]->x;
    ptp[i]->o = i;
  }

  // sort y to find s[]
  std::sort(ptp.begin(), ptp.begin() + d, [](const Point* a, const Point* b) {
    return a->y < b->y;
  });

  std::vector<int> s(d);
  std::vector<int> ys(d);
  for (int i = 0; i < d; i++) {
    ys[i] = ptp[i]->y;
    s[i] = ptp[i]->o;
  }

  return flutes_wl_all_degree(d, xs, ys, s, acc);
}

// For low-degree, i.e., 2 <= d <= kMaxLutDegree
int Flute::flutes_wl_low_degree(const int d,
                                const std::vector<int>& xs,
                                const std::vector<int>& ys,
                                const std::vector<int>& s)
{
  if (d <= 3) {
    return xs[d - 1] - xs[0] + ys[d - 1] - ys[0];
  }

  ensureLUT(d);

  int k = 0;
  if (s[0] < s[2]) {
    k++;
  }
  if (s[1] < s[2]) {
    k++;
  }

  for (int i = 3; i <= d - 1; i++) {  // p0=0 always, skip i=1 for symmetry
    int pi = s[i];
    for (int j = d - 1; j > i; j--) {
      if (s[j] < s[i]) {
        pi--;
      }
    }
    k = pi + (i + 1) * k;
  }

  int dd[2 * kMaxLutDegree
         - 2];  // 0..kMaxLutDegree-2 for v, kMaxLutDegree-1..2*D-3 for h
  if (k < kNumGroup[d]) {  // no horizontal flip
    for (int i = 1; i <= d - 3; i++) {
      dd[i] = ys[i + 1] - ys[i];
      dd[d - 1 + i] = xs[i + 1] - xs[i];
    }
  } else {
    k = 2 * kNumGroup[d] - 1 - k;
    for (int i = 1; i <= d - 3; i++) {
      dd[i] = ys[i + 1] - ys[i];
      dd[d - 1 + i] = xs[d - 1 - i] - xs[d - 2 - i];
    }
  }

  int l[kMaxPowv + 1];
  l[0] = xs[d - 1] - xs[0] + ys[d - 1] - ys[0];
  int minl = l[0];
  Csoln* rlist = (*lut_)[d][k].get();
  for (int i = 0; rlist->seg[i] > 0; i++) {
    minl += dd[rlist->seg[i]];
  }

  l[1] = minl;
  int j = 2;
  while (j <= numsoln_[d][k]) {
    rlist++;
    int sum = l[rlist->parent];
    for (int i = 0; rlist->seg[i] > 0; i++) {
      sum += dd[rlist->seg[i]];
    }
    for (int i = 10; rlist->seg[i] > 0; i--) {
      sum -= dd[rlist->seg[i]];
    }
    minl = std::min(minl, sum);
    l[j++] = sum;
  }

  return minl;
}

// For medium-degree, i.e., kMaxLutDegree+1 <= d
int Flute::flutes_wl_medium_degree(int d,
                                   const std::vector<int>& xs,
                                   const std::vector<int>& ys,
                                   const std::vector<int>& s,
                                   int acc)
{
  ensureLUT(d);

  int extral = 0;

  const int degree = d + 1;

  std::vector<int> x1(degree), x2(degree), y1(degree), y2(degree);
  std::vector<int> s1(degree), s2(degree);

  if (s[0] < s[d - 1]) {
    int ms = std::max(s[0], s[1]);
    for (int i = 2; i <= ms; i++) {
      ms = std::max(ms, s[i]);
    }
    if (ms <= d - 3) {
      for (int i = 0; i <= ms; i++) {
        x1[i] = xs[i];
        y1[i] = ys[i];
        s1[i] = s[i];
      }
      x1[ms + 1] = xs[ms];
      y1[ms + 1] = ys[ms];
      s1[ms + 1] = ms + 1;

      s2[0] = 0;
      for (int i = 1; i <= d - 1 - ms; i++) {
        s2[i] = s[i + ms] - ms;
      }

      std::vector<int> tmp_xs(xs.begin() + ms, xs.end());
      std::vector<int> tmp_ys(ys.begin() + ms, ys.end());
      return flutes_wl_all_degree(ms + 2, x1, y1, s1, acc)
             + flutes_wl_all_degree(d - ms, tmp_xs, tmp_ys, s2, acc);
    }
  } else {  // (s[0] > s[d-1])
    int ms = std::min(s[0], s[1]);
    for (int i = 2; i <= d - 1 - ms; i++) {
      ms = std::min(ms, s[i]);
    }
    if (ms >= 2) {
      x1[0] = xs[ms];
      y1[0] = ys[0];
      s1[0] = s[0] - ms + 1;
      for (int i = 1; i <= d - 1 - ms; i++) {
        x1[i] = xs[i + ms - 1];
        y1[i] = ys[i];
        s1[i] = s[i] - ms + 1;
      }
      x1[d - ms] = xs[d - 1];
      y1[d - ms] = ys[d - 1 - ms];
      s1[d - ms] = 0;

      s2[0] = ms;
      for (int i = 1; i <= ms; i++) {
        s2[i] = s[i + d - 1 - ms];
      }

      std::vector<int> tmp_ys(ys.begin() + d - 1 - ms, ys.end());
      return flutes_wl_all_degree(d + 1 - ms, x1, y1, s1, acc)
             + flutes_wl_all_degree(ms + 1, xs, tmp_ys, s2, acc);
    }
  }

  // Find inverse si[] of s[]
  std::vector<int> si(degree);
  for (int r = 0; r < d; r++) {
    si[s[r]] = r;
  }

  // Determine breaking directions and positions dp[]
  const int lb = std::max((d - 2 * acc + 2) / 4, 2);
  const int ub = d - 1 - lb;

  // Compute scores
  constexpr double AAWL = 0.6;
  constexpr double BBWL = 0.3;
  const float CCWL = 7.4 / ((d + 10.) * (d - 3.));
  const float DDWL = 4.8 / (d - 1);

  // Compute penalty[]
  std::vector<float> penalty(degree);
  const float dx = CCWL * (xs[d - 2] - xs[1]);
  const float dy = CCWL * (ys[d - 2] - ys[1]);
  float pnlty = 0;
  for (int r = d / 2; r >= 0; r--, pnlty += dx) {
    penalty[r] = pnlty;
    penalty[d - 1 - r] = pnlty;
  }
  pnlty = dy;
  for (int r = d / 2 - 1; r >= 0; r--, pnlty += dy) {
    penalty[s[r]] += pnlty;
    penalty[s[d - 1 - r]] += pnlty;
  }

  // Compute distx[], disty[]
  std::vector<int> distx(degree), disty(degree);
  const int xydiff = (xs[d - 1] - xs[0]) - (ys[d - 1] - ys[0]);
  int mins, maxs;
  if (s[0] < s[1]) {
    mins = s[0];
    maxs = s[1];
  } else {
    mins = s[1];
    maxs = s[0];
  }
  int minsi, maxsi;
  if (si[0] < si[1]) {
    minsi = si[0];
    maxsi = si[1];
  } else {
    minsi = si[1];
    maxsi = si[0];
  }
  for (int r = 2; r <= ub; r++) {
    if (s[r] < mins) {
      mins = s[r];
    } else if (s[r] > maxs) {
      maxs = s[r];
    }
    distx[r] = xs[maxs] - xs[mins];
    if (si[r] < minsi) {
      minsi = si[r];
    } else if (si[r] > maxsi) {
      maxsi = si[r];
    }
    disty[r] = ys[maxsi] - ys[minsi] + xydiff;
  }

  if (s[d - 2] < s[d - 1]) {
    mins = s[d - 2], maxs = s[d - 1];
  } else {
    mins = s[d - 1], maxs = s[d - 2];
  }
  if (si[d - 2] < si[d - 1]) {
    minsi = si[d - 2], maxsi = si[d - 1];
  } else {
    minsi = si[d - 1], maxsi = si[d - 2];
  }
  for (int r = d - 3; r >= lb; r--) {
    if (s[r] < mins) {
      mins = s[r];
    } else if (s[r] > maxs) {
      maxs = s[r];
    }
    distx[r] += xs[maxs] - xs[mins];
    if (si[r] < minsi) {
      minsi = si[r];
    } else if (si[r] > maxsi) {
      maxsi = si[r];
    }
    disty[r] += ys[maxsi] - ys[minsi];
  }

  std::vector<float> score(size_t(2) * degree);
  int nbp = 0;
  for (int r = lb; r <= ub; r++) {
    if (si[r] == 0 || si[r] == d - 1) {
      score[nbp] = (xs[r + 1] - xs[r - 1]) - penalty[r]
                   - AAWL * (ys[d - 2] - ys[1]) - DDWL * disty[r];
    } else {
      score[nbp] = (xs[r + 1] - xs[r - 1]) - penalty[r]
                   - BBWL * (ys[si[r] + 1] - ys[si[r] - 1]) - DDWL * disty[r];
    }
    nbp++;

    if (s[r] == 0 || s[r] == d - 1) {
      score[nbp] = (ys[r + 1] - ys[r - 1]) - penalty[s[r]]
                   - AAWL * (xs[d - 2] - xs[1]) - DDWL * distx[r];
    } else {
      score[nbp] = (ys[r + 1] - ys[r - 1]) - penalty[s[r]]
                   - BBWL * (xs[s[r] + 1] - xs[s[r] - 1]) - DDWL * distx[r];
    }
    nbp++;
  }

  int newacc;
  if (acc <= 3) {
    newacc = 1;
  } else {
    newacc = acc / 2;
    if (acc >= nbp) {
      acc = nbp - 1;
    }
  }

  int minl = (int) INT_MAX;
  for (int i = 0; i < acc; i++) {
    int maxbp = 0;
    for (int bp = 1; bp < nbp; bp++) {
      if (score[maxbp] < score[bp]) {
        maxbp = bp;
      }
    }
    score[maxbp] = -9e9;

    auto break_pt = [lb](const int bp) { return bp / 2 + lb; };
    auto break_in_x = [](const int bp) { return (bp) % 2 == 0; };
    const int p = break_pt(maxbp);
    // Breaking in p
    int ll;
    if (break_in_x(maxbp)) {  // break in x
      int n1 = 0;
      int n2 = 0;
      for (int r = 0; r < d; r++) {
        if (s[r] < p) {
          s1[n1] = s[r];
          y1[n1] = ys[r];
          n1++;
        } else if (s[r] > p) {
          s2[n2] = s[r] - p;
          y2[n2] = ys[r];
          n2++;
        } else {  // if (s[r] == p)  i.e.,  r = si[p]
          s1[n1] = p;
          s2[n2] = 0;
          if (r == d - 1 || r == d - 2) {
            y1[n1] = y2[n2] = ys[r - 1];
            extral = ys[r] - ys[r - 1];
          } else if (r == 0 || r == 1) {
            y1[n1] = y2[n2] = ys[r + 1];
            extral = ys[r + 1] - ys[r];
          } else {
            y1[n1] = y2[n2] = ys[r];
            extral = 0;
          }
          n1++;
          n2++;
        }
      }
      std::vector<int> tmp_xs(xs.begin() + p, xs.end());
      ll = extral + flutes_wl_all_degree(p + 1, xs, y1, s1, newacc)
           + flutes_wl_all_degree(d - p, tmp_xs, y2, s2, newacc);
    } else {  // if (!break_in_x(maxbp))
      int n1 = 0;
      int n2 = 0;
      for (int r = 0; r < d; r++) {
        if (si[r] < p) {
          s1[si[r]] = n1;
          x1[n1] = xs[r];
          n1++;
        } else if (si[r] > p) {
          s2[si[r] - p] = n2;
          x2[n2] = xs[r];
          n2++;
        } else {  // if (si[r] == p)  i.e.,  r = s[p]
          s1[p] = n1;
          s2[0] = n2;
          if (r == d - 1 || r == d - 2) {
            x1[n1] = x2[n2] = xs[r - 1];
            extral = xs[r] - xs[r - 1];
          } else if (r == 0 || r == 1) {
            x1[n1] = x2[n2] = xs[r + 1];
            extral = xs[r + 1] - xs[r];
          } else {
            x1[n1] = x2[n2] = xs[r];
            extral = 0;
          }
          n1++;
          n2++;
        }
      }
      std::vector<int> tmp_ys(ys.begin() + p, ys.end());
      ll = extral + flutes_wl_all_degree(p + 1, x1, ys, s1, newacc)
           + flutes_wl_all_degree(d - p, x2, tmp_ys, s2, newacc);
    }
    minl = std::min(minl, ll);
  }

  return minl;
}

static bool orderx(const Point* a, const Point* b)
{
  return a->x < b->x;
}

static bool ordery(const Point* a, const Point* b)
{
  return a->y < b->y;
}

Tree Flute::flute(const std::vector<int>& x,
                  const std::vector<int>& y,
                  const int acc)
{
  const int d = x.size();

  if (d < 2) {
    return {.deg = 1, .length = 0, .branch{}};
  }

  if (d == 2) {
    return {.deg = 2,
            .length = std::abs(x[0] - x[1]) + std::abs(y[0] - y[1]),
            .branch{{.x = x[0], .y = y[0], .n = 1},
                    {.x = x[1], .y = y[1], .n = 1}}};
  }

  ensureLUT(d);

  std::vector<Point> pt(d + 1);
  std::vector<Point*> ptp(d + 1);

  for (int i = 0; i < d; i++) {
    pt[i].x = x[i];
    pt[i].y = y[i];
    ptp[i] = &pt[i];
  }

  // sort x
  if (d < 200) {
    for (int i = 0; i < d - 1; i++) {
      int minval = ptp[i]->x;
      int minidx = i;
      for (int j = i + 1; j < d; j++) {
        if (minval > ptp[j]->x) {
          minval = ptp[j]->x;
          minidx = j;
        }
      }
      std::swap(ptp[i], ptp[minidx]);
    }
  } else {
    std::stable_sort(ptp.begin(), ptp.end() - 1, orderx);
  }

  std::vector<int> xs(d);
  for (int i = 0; i < d; i++) {
    xs[i] = ptp[i]->x;
    ptp[i]->o = i;
  }

  // sort y to find s[]
  std::vector<int> ys(d);
  std::vector<int> s(d);
  if (d < 200) {
    for (int i = 0; i < d - 1; i++) {
      int minval = ptp[i]->y;
      int minidx = i;
      for (int j = i + 1; j < d; j++) {
        if (minval > ptp[j]->y) {
          minval = ptp[j]->y;
          minidx = j;
        }
      }
      ys[i] = ptp[minidx]->y;
      s[i] = ptp[minidx]->o;
      ptp[minidx] = ptp[i];
    }
    ys[d - 1] = ptp[d - 1]->y;
    s[d - 1] = ptp[d - 1]->o;
  } else {
    std::stable_sort(ptp.begin(), ptp.end() - 1, ordery);
    for (int i = 0; i < d; i++) {
      ys[i] = ptp[i]->y;
      s[i] = ptp[i]->o;
    }
  }

  return flutes(xs, ys, s, acc);
}

int Flute::flutes_wl_all_degree(const int d,
                                const std::vector<int>& xs,
                                const std::vector<int>& ys,
                                const std::vector<int>& s,
                                const int acc)
{
  if (d <= kMaxLutDegree) {
    return flutes_wl_low_degree(d, xs, ys, s);
  }
  return flutes_wl_medium_degree(d, xs, ys, s, acc);
}

Tree Flute::flutes_all_degree(const int d,
                              const std::vector<int>& xs,
                              const std::vector<int>& ys,
                              const std::vector<int>& s,
                              const int acc)
{
  if (d <= kMaxLutDegree) {
    return flutes_low_degree(d, xs, ys, s);
  }
  return flutes_medium_degree(d, xs, ys, s, acc);
}

// For low-degree, i.e., 2 <= d <= kMaxLutDegree
Tree Flute::flutes_low_degree(int d,
                              const std::vector<int>& xs,
                              const std::vector<int>& ys,
                              const std::vector<int>& s)
{
  if (d == 2) {
    return {.deg = d,
            .length = xs[1] - xs[0] + ys[1] - ys[0],
            .branch{{.x = xs[s[0]], .y = ys[0], .n = 1},
                    {.x = xs[s[1]], .y = ys[1], .n = 1}}};
  }
  if (d == 3) {
    return {.deg = d,
            .length = xs[2] - xs[0] + ys[2] - ys[0],
            .branch{
                {.x = xs[s[0]], .y = ys[0], .n = 3},
                {.x = xs[s[1]], .y = ys[1], .n = 3},
                {.x = xs[s[2]], .y = ys[2], .n = 3},
                {.x = xs[1], .y = ys[1], .n = 3},
            }};
  }

  ensureLUT(d);

  int k = 0;
  if (s[0] < s[2]) {
    k++;
  }
  if (s[1] < s[2]) {
    k++;
  }

  for (int i = 3; i <= d - 1; i++) {  // p0=0 always, skip i=1 for symmetry
    int pi = s[i];
    for (int j = d - 1; j > i; j--) {
      if (s[j] < s[i]) {
        pi--;
      }
    }
    k = pi + (i + 1) * k;
  }

  int dd[2 * kMaxLutDegree - 2];  // 0..D-2 for v, D-1..2*D-3 for h
  bool hflip;
  if (k < kNumGroup[d]) {  // no horizontal flip
    hflip = false;
    for (int i = 1; i <= d - 3; i++) {
      dd[i] = ys[i + 1] - ys[i];
      dd[d - 1 + i] = xs[i + 1] - xs[i];
    }
  } else {
    hflip = true;
    k = 2 * kNumGroup[d] - 1 - k;
    for (int i = 1; i <= d - 3; i++) {
      dd[i] = ys[i + 1] - ys[i];
      dd[d - 1 + i] = xs[d - 1 - i] - xs[d - 2 - i];
    }
  }

  int l[kMaxPowv + 1];
  l[0] = xs[d - 1] - xs[0] + ys[d - 1] - ys[0];
  int minl = l[0];
  Csoln* rlist = (*lut_)[d][k].get();
  for (int i = 0; rlist->seg[i] > 0; i++) {
    minl += dd[rlist->seg[i]];
  }
  Csoln* bestrlist = rlist;
  l[1] = minl;
  int j = 2;
  while (j <= numsoln_[d][k]) {
    rlist++;
    int sum = l[rlist->parent];
    for (int i = 0; rlist->seg[i] > 0; i++) {
      sum += dd[rlist->seg[i]];
    }
    for (int i = 10; rlist->seg[i] > 0; i--) {
      sum -= dd[rlist->seg[i]];
    }
    if (sum < minl) {
      minl = sum;
      bestrlist = rlist;
    }
    l[j++] = sum;
  }

  Tree t;
  t.deg = d;
  t.branch.resize(2 * d - 2);

  t.branch[0].x = xs[s[0]];
  t.branch[0].y = ys[0];
  t.branch[1].x = xs[s[1]];
  t.branch[1].y = ys[1];
  for (int i = 2; i < d - 2; i++) {
    t.branch[i].x = xs[s[i]];
    t.branch[i].y = ys[i];
    t.branch[i].n = bestrlist->neighbor[i];
  }
  t.branch[d - 2].x = xs[s[d - 2]];
  t.branch[d - 2].y = ys[d - 2];
  t.branch[d - 1].x = xs[s[d - 1]];
  t.branch[d - 1].y = ys[d - 1];
  if (hflip) {
    if (s[1] < s[0]) {
      t.branch[0].n = bestrlist->neighbor[1];
      t.branch[1].n = bestrlist->neighbor[0];
    } else {
      t.branch[0].n = bestrlist->neighbor[0];
      t.branch[1].n = bestrlist->neighbor[1];
    }
    if (s[d - 1] < s[d - 2]) {
      t.branch[d - 2].n = bestrlist->neighbor[d - 1];
      t.branch[d - 1].n = bestrlist->neighbor[d - 2];
    } else {
      t.branch[d - 2].n = bestrlist->neighbor[d - 2];
      t.branch[d - 1].n = bestrlist->neighbor[d - 1];
    }
    for (int i = d; i < 2 * d - 2; i++) {
      t.branch[i].x = xs[d - 1 - bestrlist->rowcol[i - d] % 16];
      t.branch[i].y = ys[bestrlist->rowcol[i - d] / 16];
      t.branch[i].n = bestrlist->neighbor[i];
    }
  } else {  // !hflip
    if (s[0] < s[1]) {
      t.branch[0].n = bestrlist->neighbor[1];
      t.branch[1].n = bestrlist->neighbor[0];
    } else {
      t.branch[0].n = bestrlist->neighbor[0];
      t.branch[1].n = bestrlist->neighbor[1];
    }
    if (s[d - 2] < s[d - 1]) {
      t.branch[d - 2].n = bestrlist->neighbor[d - 1];
      t.branch[d - 1].n = bestrlist->neighbor[d - 2];
    } else {
      t.branch[d - 2].n = bestrlist->neighbor[d - 2];
      t.branch[d - 1].n = bestrlist->neighbor[d - 1];
    }
    for (int i = d; i < 2 * d - 2; i++) {
      t.branch[i].x = xs[bestrlist->rowcol[i - d] % 16];
      t.branch[i].y = ys[bestrlist->rowcol[i - d] / 16];
      t.branch[i].n = bestrlist->neighbor[i];
    }
  }

  t.length = minl;

  return t;
}

// For medium-degree, i.e., kMaxLutDegree+1 <= d
Tree Flute::flutes_medium_degree(const int d,
                                 const std::vector<int>& xs,
                                 const std::vector<int>& ys,
                                 const std::vector<int>& s,
                                 int acc)
{
  const int degree = d + 1;
  std::vector<float> score(size_t(2) * degree);
  std::vector<float> penalty(degree);

  std::vector<int> x1(degree), x2(degree), y1(degree), y2(degree);
  std::vector<int> distx(degree), disty(degree);
  std::vector<int> si(degree), s1(degree), s2(degree);
  int ms;

  if (s[0] < s[d - 1]) {
    ms = std::max(s[0], s[1]);
    for (int i = 2; i <= ms; i++) {
      ms = std::max(ms, s[i]);
    }
    if (ms <= d - 3) {
      for (int i = 0; i <= ms; i++) {
        x1[i] = xs[i];
        y1[i] = ys[i];
        s1[i] = s[i];
      }
      x1[ms + 1] = xs[ms];
      y1[ms + 1] = ys[ms];
      s1[ms + 1] = ms + 1;

      s2[0] = 0;
      for (int i = 1; i <= d - 1 - ms; i++) {
        s2[i] = s[i + ms] - ms;
      }

      const Tree t1 = flutes_all_degree(ms + 2, x1, y1, s1, acc);

      std::vector<int> tmp_xs(xs.begin() + ms, xs.end());
      std::vector<int> tmp_ys(ys.begin() + ms, ys.end());
      const Tree t2 = flutes_all_degree(d - ms, tmp_xs, tmp_ys, s2, acc);
      return d_merge_tree(t1, t2);
    }
  } else {  // (s[0] > s[d-1])
    ms = std::min(s[0], s[1]);
    for (int i = 2; i <= d - 1 - ms; i++) {
      ms = std::min(ms, s[i]);
    }
    if (ms >= 2) {
      x1[0] = xs[ms];
      y1[0] = ys[0];
      s1[0] = s[0] - ms + 1;
      for (int i = 1; i <= d - 1 - ms; i++) {
        x1[i] = xs[i + ms - 1];
        y1[i] = ys[i];
        s1[i] = s[i] - ms + 1;
      }
      x1[d - ms] = xs[d - 1];
      y1[d - ms] = ys[d - 1 - ms];
      s1[d - ms] = 0;

      s2[0] = ms;
      for (int i = 1; i <= ms; i++) {
        s2[i] = s[i + d - 1 - ms];
      }

      const Tree t1 = flutes_all_degree(d + 1 - ms, x1, y1, s1, acc);

      std::vector<int> tmp_ys(ys.begin() + d - 1 - ms, ys.end());
      const Tree t2 = flutes_all_degree(ms + 1, xs, tmp_ys, s2, acc);
      return d_merge_tree(t1, t2);
    }
  }

  // Find inverse si[] of s[]
  for (int r = 0; r < d; r++) {
    si[s[r]] = r;
  }

  // Determine breaking directions and positions dp[]
  const int lb = std::max((d - 2 * acc + 2) / 4, 2);
  const int ub = d - 1 - lb;

  // Compute scores
  constexpr double AA = 0.6;  // 2.0*BB
  constexpr double BB = 0.3;

  const float CC = 7.4 / ((d + 10.) * (d - 3.));
  const float DD = 4.8 / (d - 1);

  // Compute penalty[]
  const float dx = CC * (xs[d - 2] - xs[1]);
  const float dy = CC * (ys[d - 2] - ys[1]);
  float pnlty = 0;
  for (int r = d / 2; r >= 2; r--, pnlty += dx) {
    penalty[r] = pnlty, penalty[d - 1 - r] = pnlty;
  }
  penalty[1] = pnlty, penalty[d - 2] = pnlty;
  penalty[0] = pnlty, penalty[d - 1] = pnlty;
  pnlty = dy;
  for (int r = d / 2 - 1; r >= 2; r--, pnlty += dy) {
    penalty[s[r]] += pnlty, penalty[s[d - 1 - r]] += pnlty;
  }
  penalty[s[1]] += pnlty, penalty[s[d - 2]] += pnlty;
  penalty[s[0]] += pnlty, penalty[s[d - 1]] += pnlty;

  // Compute distx[], disty[]
  int mins = std::min(s[0], s[1]);
  int maxs = std::max(s[0], s[1]);
  int minsi = std::min(si[0], si[1]);
  int maxsi = std::max(si[0], si[1]);
  const int xydiff = (xs[d - 1] - xs[0]) - (ys[d - 1] - ys[0]);
  for (int r = 2; r <= ub; r++) {
    if (s[r] < mins) {
      mins = s[r];
    } else if (s[r] > maxs) {
      maxs = s[r];
    }
    distx[r] = xs[maxs] - xs[mins];
    if (si[r] < minsi) {
      minsi = si[r];
    } else if (si[r] > maxsi) {
      maxsi = si[r];
    }
    disty[r] = ys[maxsi] - ys[minsi] + xydiff;
  }

  mins = std::min(s[d - 2], s[d - 1]);
  maxs = std::max(s[d - 2], s[d - 1]);
  minsi = std::min(si[d - 2], si[d - 1]);
  maxsi = std::max(si[d - 2], si[d - 1]);

  for (int r = d - 3; r >= lb; r--) {
    if (s[r] < mins) {
      mins = s[r];
    } else if (s[r] > maxs) {
      maxs = s[r];
    }
    distx[r] += xs[maxs] - xs[mins];
    if (si[r] < minsi) {
      minsi = si[r];
    } else if (si[r] > maxsi) {
      maxsi = si[r];
    }
    disty[r] += ys[maxsi] - ys[minsi];
  }

  int nbp = 0;
  for (int r = lb; r <= ub; r++) {
    if (si[r] <= 1) {
      score[nbp] = (xs[r + 1] - xs[r - 1]) - penalty[r] - AA * (ys[2] - ys[1])
                   - DD * disty[r];
    } else if (si[r] >= d - 2) {
      score[nbp] = (xs[r + 1] - xs[r - 1]) - penalty[r]
                   - AA * (ys[d - 2] - ys[d - 3]) - DD * disty[r];
    } else {
      score[nbp] = (xs[r + 1] - xs[r - 1]) - penalty[r]
                   - BB * (ys[si[r] + 1] - ys[si[r] - 1]) - DD * disty[r];
    }
    nbp++;

    if (s[r] <= 1) {
      score[nbp] = (ys[r + 1] - ys[r - 1]) - penalty[s[r]]
                   - AA * (xs[2] - xs[1]) - DD * distx[r];
    } else if (s[r] >= d - 2) {
      score[nbp] = (ys[r + 1] - ys[r - 1]) - penalty[s[r]]
                   - AA * (xs[d - 2] - xs[d - 3]) - DD * distx[r];
    } else {
      score[nbp] = (ys[r + 1] - ys[r - 1]) - penalty[s[r]]
                   - BB * (xs[s[r] + 1] - xs[s[r] - 1]) - DD * distx[r];
    }
    nbp++;
  }

  int newacc;
  if (acc <= 3) {
    newacc = 1;
  } else {
    newacc = acc / 2;
    if (acc >= nbp) {
      acc = nbp - 1;
    }
  }

  auto break_pt = [lb](const int bp) { return bp / 2 + lb; };
  auto break_in_x = [](const int bp) { return (bp) % 2 == 0; };

  int nn1 = 0;
  int nn2 = 0;
  int minl = std::numeric_limits<int>::max();
  Tree bestt1, bestt2;
  int bestbp = 0;
  for (int i = 0; i < acc; i++) {
    int maxbp = 0;
    for (int bp = 1; bp < nbp; bp++) {
      if (score[maxbp] < score[bp]) {
        maxbp = bp;
      }
    }
    score[maxbp] = -9e9;

    int ll;
    Tree t1, t2;
    int p = break_pt(maxbp);
    // Breaking in p
    if (break_in_x(maxbp)) {  // break in x
      int n1 = 0;
      int n2 = 0;
      for (int r = 0; r < d; r++) {
        if (s[r] < p) {
          s1[n1] = s[r];
          y1[n1] = ys[r];
          n1++;
        } else if (s[r] > p) {
          s2[n2] = s[r] - p;
          y2[n2] = ys[r];
          n2++;
        } else {  // if (s[r] == p)  i.e.,  r = si[p]
          s1[n1] = p;
          s2[n2] = 0;
          y1[n1] = y2[n2] = ys[r];
          nn1 = n1;
          nn2 = n2;
          n1++;
          n2++;
        }
      }

      t1 = flutes_all_degree(p + 1, xs, y1, s1, newacc);

      std::vector<int> tmp_xs(xs.begin() + p, xs.end());
      t2 = flutes_all_degree(d - p, tmp_xs, y2, s2, newacc);
      ll = t1.length + t2.length;
      const int coord1 = t1.branch[t1.branch[nn1].n].y;
      const int coord2 = t2.branch[t2.branch[nn2].n].y;
      if (t2.branch[nn2].y > std::max(coord1, coord2)) {
        ll -= t2.branch[nn2].y - std::max(coord1, coord2);
      } else if (t2.branch[nn2].y < std::min(coord1, coord2)) {
        ll -= std::min(coord1, coord2) - t2.branch[nn2].y;
      }
    } else {  // if (!break_in_x(maxbp))
      int n1 = 0;
      int n2 = 0;
      for (int r = 0; r < d; r++) {
        if (si[r] < p) {
          s1[si[r]] = n1;
          x1[n1] = xs[r];
          n1++;
        } else if (si[r] > p) {
          s2[si[r] - p] = n2;
          x2[n2] = xs[r];
          n2++;
        } else {  // if (si[r] == p)  i.e.,  r = s[p]
          s1[p] = n1;
          s2[0] = n2;
          x1[n1] = x2[n2] = xs[r];
          n1++;
          n2++;
        }
      }

      t1 = flutes_all_degree(p + 1, x1, ys, s1, newacc);

      std::vector<int> tmp_ys(ys.begin() + p, ys.end());
      t2 = flutes_all_degree(d - p, x2, tmp_ys, s2, newacc);
      ll = t1.length + t2.length;
      const int coord1 = t1.branch[t1.branch[p].n].x;
      const int coord2 = t2.branch[t2.branch[0].n].x;
      if (t2.branch[0].x > std::max(coord1, coord2)) {
        ll -= t2.branch[0].x - std::max(coord1, coord2);
      } else if (t2.branch[0].x < std::min(coord1, coord2)) {
        ll -= std::min(coord1, coord2) - t2.branch[0].x;
      }
    }
    if (minl > ll) {
      minl = ll;
      bestt1 = t1;
      bestt2 = t2;
      bestbp = maxbp;
    }
  }

  Tree t;
  if (kLocalRefinement) {
    if (break_in_x(bestbp)) {
      t = h_merge_tree(bestt1, bestt2, s);
      local_refinement(degree, &t, si[break_pt(bestbp)]);
    } else {
      t = v_merge_tree(bestt1, bestt2);
      local_refinement(degree, &t, break_pt(bestbp));
    }
  } else {
    if (break_in_x(bestbp)) {
      t = h_merge_tree(bestt1, bestt2, s);
    } else {
      t = v_merge_tree(bestt1, bestt2);
    }
  }

  return t;
}

Tree Flute::d_merge_tree(const Tree& t1, const Tree& t2) const
{
  Tree t;

  const int d = t1.deg + t2.deg - 2;
  t.deg = d;
  t.length = t1.length + t2.length;
  t.branch.resize(2 * d - 2);
  const int offset1 = t2.deg - 2;
  const int offset2 = 2 * t1.deg - 4;

  for (int i = 0; i <= t1.deg - 2; i++) {
    t.branch[i].x = t1.branch[i].x;
    t.branch[i].y = t1.branch[i].y;
    t.branch[i].n = t1.branch[i].n + offset1;
  }
  for (int i = t1.deg - 1; i <= d - 1; i++) {
    t.branch[i].x = t2.branch[i - t1.deg + 2].x;
    t.branch[i].y = t2.branch[i - t1.deg + 2].y;
    t.branch[i].n = t2.branch[i - t1.deg + 2].n + offset2;
  }
  for (int i = d; i <= d + t1.deg - 3; i++) {
    t.branch[i].x = t1.branch[i - offset1].x;
    t.branch[i].y = t1.branch[i - offset1].y;
    t.branch[i].n = t1.branch[i - offset1].n + offset1;
  }
  for (int i = d + t1.deg - 2; i <= 2 * d - 3; i++) {
    t.branch[i].x = t2.branch[i - offset2].x;
    t.branch[i].y = t2.branch[i - offset2].y;
    t.branch[i].n = t2.branch[i - offset2].n + offset2;
  }

  int prev = t2.branch[0].n + offset2;
  int curr = t1.branch[t1.deg - 1].n + offset1;
  int next = t.branch[curr].n;
  while (curr != next) {
    t.branch[curr].n = prev;
    prev = curr;
    curr = next;
    next = t.branch[curr].n;
  }
  t.branch[curr].n = prev;

  return t;
}

Tree Flute::h_merge_tree(const Tree& t1,
                         const Tree& t2,
                         const std::vector<int>& s)
{
  Tree t;

  t.deg = t1.deg + t2.deg - 1;
  t.length = t1.length + t2.length;
  t.branch.resize(2 * t.deg - 2);
  const int offset1 = t2.deg - 1;
  const int offset2 = 2 * t1.deg - 3;

  const int p = t1.deg - 1;
  int n1 = 0;
  int n2 = 0;
  int nn1 = 0;
  int nn2 = 0;
  int ii = 0;
  for (int i = 0; i < t.deg; i++) {
    if (s[i] < p) {
      t.branch[i].x = t1.branch[n1].x;
      t.branch[i].y = t1.branch[n1].y;
      t.branch[i].n = t1.branch[n1].n + offset1;
      n1++;
    } else if (s[i] > p) {
      t.branch[i].x = t2.branch[n2].x;
      t.branch[i].y = t2.branch[n2].y;
      t.branch[i].n = t2.branch[n2].n + offset2;
      n2++;
    } else {
      t.branch[i].x = t2.branch[n2].x;
      t.branch[i].y = t2.branch[n2].y;
      t.branch[i].n = t2.branch[n2].n + offset2;
      nn1 = n1;
      nn2 = n2;
      ii = i;
      n1++;
      n2++;
    }
  }
  for (int i = t.deg; i <= t.deg + t1.deg - 3; i++) {
    t.branch[i].x = t1.branch[i - offset1].x;
    t.branch[i].y = t1.branch[i - offset1].y;
    t.branch[i].n = t1.branch[i - offset1].n + offset1;
  }
  for (int i = t.deg + t1.deg - 2; i <= 2 * t.deg - 4; i++) {
    t.branch[i].x = t2.branch[i - offset2].x;
    t.branch[i].y = t2.branch[i - offset2].y;
    t.branch[i].n = t2.branch[i - offset2].n + offset2;
  }
  const int extra = 2 * t.deg - 3;
  const int coord1 = t1.branch[t1.branch[nn1].n].y;
  const int coord2 = t2.branch[t2.branch[nn2].n].y;
  if (t2.branch[nn2].y > std::max(coord1, coord2)) {
    t.branch[extra].y = std::max(coord1, coord2);
    t.length -= t2.branch[nn2].y - t.branch[extra].y;
  } else if (t2.branch[nn2].y < std::min(coord1, coord2)) {
    t.branch[extra].y = std::min(coord1, coord2);
    t.length -= t.branch[extra].y - t2.branch[nn2].y;
  } else {
    t.branch[extra].y = t2.branch[nn2].y;
  }
  t.branch[extra].x = t2.branch[nn2].x;
  t.branch[extra].n = t.branch[ii].n;
  t.branch[ii].n = extra;

  int prev = extra;
  int curr = t1.branch[nn1].n + offset1;
  int next = t.branch[curr].n;
  while (curr != next) {
    t.branch[curr].n = prev;
    prev = curr;
    curr = next;
    next = t.branch[curr].n;
  }
  t.branch[curr].n = prev;

  return t;
}

Tree Flute::v_merge_tree(const Tree& t1, const Tree& t2) const
{
  Tree t;

  t.deg = t1.deg + t2.deg - 1;
  t.length = t1.length + t2.length;
  t.branch.resize(2 * t.deg - 2);
  const int offset1 = t2.deg - 1;
  const int offset2 = 2 * t1.deg - 3;

  for (int i = 0; i <= t1.deg - 2; i++) {
    t.branch[i].x = t1.branch[i].x;
    t.branch[i].y = t1.branch[i].y;
    t.branch[i].n = t1.branch[i].n + offset1;
  }
  for (int i = t1.deg - 1; i <= t.deg - 1; i++) {
    t.branch[i].x = t2.branch[i - t1.deg + 1].x;
    t.branch[i].y = t2.branch[i - t1.deg + 1].y;
    t.branch[i].n = t2.branch[i - t1.deg + 1].n + offset2;
  }
  for (int i = t.deg; i <= t.deg + t1.deg - 3; i++) {
    t.branch[i].x = t1.branch[i - offset1].x;
    t.branch[i].y = t1.branch[i - offset1].y;
    t.branch[i].n = t1.branch[i - offset1].n + offset1;
  }
  for (int i = t.deg + t1.deg - 2; i <= 2 * t.deg - 4; i++) {
    t.branch[i].x = t2.branch[i - offset2].x;
    t.branch[i].y = t2.branch[i - offset2].y;
    t.branch[i].n = t2.branch[i - offset2].n + offset2;
  }
  const int extra = 2 * t.deg - 3;
  const int coord1 = t1.branch[t1.branch[t1.deg - 1].n].x;
  const int coord2 = t2.branch[t2.branch[0].n].x;
  if (t2.branch[0].x > std::max(coord1, coord2)) {
    t.branch[extra].x = std::max(coord1, coord2);
    t.length -= t2.branch[0].x - t.branch[extra].x;
  } else if (t2.branch[0].x < std::min(coord1, coord2)) {
    t.branch[extra].x = std::min(coord1, coord2);
    t.length -= t.branch[extra].x - t2.branch[0].x;
  } else {
    t.branch[extra].x = t2.branch[0].x;
  }
  t.branch[extra].y = t2.branch[0].y;
  t.branch[extra].n = t.branch[t1.deg - 1].n;
  t.branch[t1.deg - 1].n = extra;

  int prev = extra;
  int curr = t1.branch[t1.deg - 1].n + offset1;
  int next = t.branch[curr].n;
  while (curr != next) {
    t.branch[curr].n = prev;
    prev = curr;
    curr = next;
    next = t.branch[curr].n;
  }
  t.branch[curr].n = prev;

  return t;
}

void Flute::local_refinement(const int deg, Tree* tp, const int p)
{
  const int root = tp->branch[p].n;

  // Reverse edges to point to root
  int prev = root;
  int curr = tp->branch[prev].n;
  int next = tp->branch[curr].n;
  while (curr != next) {
    tp->branch[curr].n = prev;
    prev = curr;
    curr = next;
    next = tp->branch[curr].n;
  }
  tp->branch[curr].n = prev;
  tp->branch[root].n = root;

  // Find Steiner nodes that are at pins
  const int degree = deg + 1;
  std::vector<int> steiner_pin(size_t(2) * degree);
  const int d = tp->deg;
  for (int i = d; i <= 2 * d - 3; i++) {
    steiner_pin[i] = -1;
  }
  for (int i = 0; i < d; i++) {
    next = tp->branch[i].n;
    if (tp->branch[i].x == tp->branch[next].x
        && tp->branch[i].y == tp->branch[next].y) {
      steiner_pin[next] = i;  // Steiner 'next' at Pin 'i'
    }
  }
  steiner_pin[root] = p;

  // Find pins that are directly connected to root
  std::vector<int> index(size_t(2) * degree);
  std::vector<int> x(degree);
  int dd = 0;
  for (int i = 0; i < d; i++) {
    curr = tp->branch[i].n;
    if (steiner_pin[curr] == i) {
      curr = tp->branch[curr].n;
    }
    while (steiner_pin[curr] < 0) {
      curr = tp->branch[curr].n;
    }
    if (curr == root) {
      x[dd] = tp->branch[i].x;
      if (steiner_pin[tp->branch[i].n] == i && tp->branch[i].n != root) {
        index[dd++] = tp->branch[i].n;  // Steiner node
      } else {
        index[dd++] = i;  // Pin
      }
    }
  }

  if (4 <= dd && dd <= kMaxLutDegree) {
    // Find Steiner nodes that are directly connected to root
    int ii = dd;
    for (int i = 0; i < dd; i++) {
      curr = tp->branch[index[i]].n;
      while (steiner_pin[curr] < 0) {
        index[ii++] = curr;
        steiner_pin[curr] = INT_MAX;
        curr = tp->branch[curr].n;
      }
    }
    index[ii] = root;

    std::vector<int> ss(degree);
    std::vector<int> xs(degree), ys(degree);
    for (ii = 0; ii < dd; ii++) {
      ss[ii] = 0;
      for (int j = 0; j < ii; j++) {
        if (x[j] < x[ii]) {
          ss[ii]++;
        }
      }
      for (int j = ii + 1; j < dd; j++) {
        if (x[j] <= x[ii]) {
          ss[ii]++;
        }
      }
      xs[ss[ii]] = x[ii];
      ys[ii] = tp->branch[index[ii]].y;
    }

    const Tree tt = flutes_low_degree(dd, xs, ys, ss);

    // Find new wirelength
    tp->length += tt.length;
    for (ii = 0; ii < 2 * dd - 3; ii++) {
      const int i = index[ii];
      const int j = tp->branch[i].n;
      tp->length -= std::abs(tp->branch[i].x - tp->branch[j].x)
                    + std::abs(tp->branch[i].y - tp->branch[j].y);
    }

    // Copy tt into t
    for (ii = 0; ii < dd; ii++) {
      tp->branch[index[ii]].n = index[tt.branch[ii].n];
    }
    for (; ii <= 2 * dd - 3; ii++) {
      tp->branch[index[ii]].x = tt.branch[ii].x;
      tp->branch[index[ii]].y = tt.branch[ii].y;
      tp->branch[index[ii]].n = index[tt.branch[ii].n];
    }
  }
}

int Flute::wirelength(const Tree& t)
{
  int l = 0;

  for (int i = 0; i < 2 * t.deg - 2; i++) {
    const int j = t.branch[i].n;
    l += std::abs(t.branch[i].x - t.branch[j].x)
         + std::abs(t.branch[i].y - t.branch[j].y);
  }

  return l;
}

// Output in a format that can be plotted by gnuplot
void Flute::plottree(const Tree& t)
{
  for (int i = 0; i < 2 * t.deg - 2; i++) {
    printf("%d %d\n", t.branch[i].x, t.branch[i].y);
    printf("%d %d\n\n", t.branch[t.branch[i].n].x, t.branch[t.branch[i].n].y);
  }
}

Tree Flute::flutes(const std::vector<int>& xs,
                   const std::vector<int>& ys,
                   const std::vector<int>& s,
                   const int acc)
{
  const int d = xs.size();
  return flutes_all_degree(d, xs, ys, s, acc);
}

}  // namespace stt::flt
