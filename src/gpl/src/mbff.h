///////////////////////////////////////////////////////////////////////////////
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include <vector>

#pragma once

struct Point {
    float x = 0, y = 0;
};

struct Tray {
    Point pt;
    std::vector<Point> slots;
    int cand[70];
};

struct Flop {
    Point pt;
    int idx = 0;
    float prob = 0;

    bool operator<(Flop &a) { return prob < a.prob; }
};

struct Path {
    int a = 0, b = 0;
};

namespace gpl {

class MBFF {

  public:
    MBFF(const int num_flops, const int num_paths, const std::vector<float>& x,
         const std::vector<float>& y, const std::vector<std::pair<int, int> >& paths,
         const int threads);
    ~MBFF();

    void Run(const int mx_sz, const float alpha, const float beta);

  private:

    float GetDist(const Point& a, const Point& b);
    int GetRows(const int slot_cnt);

    int GetBitCnt(const int bit_idx);

    std::vector<Point> GetSlots(const Point& tray, const int rows, const int cols);
    Flop GetNewFlop(const std::vector<Flop>& prob_dist, const float tot_dist);

    std::vector<Tray> GetStartTrays(std::vector<Flop> flops, const int num_trays,
                                    const float AR);
    Tray GetOneBit(const Point& pt);

    std::vector<std::pair<int, int> > MinCostFlow(const std::vector<Flop> &flops, std::vector<Tray> &trays, const int sz);

    float GetSilh(const std::vector<Flop>& flops, const std::vector<Tray>& trays,
                  const std::vector<std::pair<int, int> >& clusters);

    std::vector<std::vector<Flop> > KMeans(const std::vector<Flop>& flops, const int K);
    std::vector<std::vector<Flop> > KMeansDecomp(const std::vector<Flop> &flops, const int MAX_SZ);


    float RunLP(const std::vector<Flop>& flops, std::vector<Tray> &trays,
                const std::vector<std::pair<int, int> >& clusters);

    void RunILP(const std::vector<Flop>& flops, const std::vector<Path>& paths,
                const std::vector<std::vector<Tray> >& all_trays, const float alpha,
                const float beta);

    void Remap(const std::vector<Flop> &flops, std::vector<std::vector<Tray> >& trays);
    std::vector<std::vector<Tray> > RunSilh(const std::vector<Flop>& pointset);
    std::vector<std::pair<int, int> > RunCapacitatedKMeans(const std::vector<Flop>& flops, std::vector<Tray>& trays, const int sz, const int iter);

    std::vector<Flop> FLOPS;
    std::vector<Path> PATHS;
    int NUM_THREADS;

};
}
