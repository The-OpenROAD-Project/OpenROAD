///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
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

#ifndef __REPLACE_FFT__
#define __REPLACE_FFT__

#include <vector>

namespace gpl {

class FFT {
  public:
    FFT();
    FFT(int binCntX, int binCntY, int binSizeX, int binSizeY);
    ~FFT();

    // input func
    void updateDensity(int x, int y, float density);

    // do FFT
    void doFFT();

    // returning func
    std::pair<float, float> getElectroForce(int x, int y) const;
    float getElectroPhi(int x, int y) const;

  private:
    // 2D array; width: binCntX_, height: binCntY_;
    // No hope to use Vector at this moment...
    float** binDensity_;
    float** electroPhi_;
    float** electroForceX_;
    float** electroForceY_;

    // cos/sin table (prev: w_2d)
    // length:  max(binCntX, binCntY) * 3 / 2
    std::vector<float> csTable_;

    // wx. length:  binCntX_
    std::vector<float> wx_;
    std::vector<float> wxSquare_;

    // wy. length:  binCntY_
    std::vector<float> wy_;
    std::vector<float> wySquare_;

    // work area for bit reversal (prev: ip)
    // length: round(sqrt( max(binCntX_, binCntY_) )) + 2
    std::vector<int> workArea_;

    int binCntX_;
    int binCntY_;
    int binSizeX_;
    int binSizeY_;

    void init();
};


//
// The following FFT library came from
// http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html
//
//
/// 1D FFT ////////////////////////////////////////////////////////////////
void cdft(int n, int isgn, float *a, int *ip, float *w);
void ddct(int n, int isgn, float *a, int *ip, float *w);
void ddst(int n, int isgn, float *a, int *ip, float *w);

/// 2D FFT ////////////////////////////////////////////////////////////////
void cdft2d(int, int, int, float **, float *, int *, float *);
void rdft2d(int, int, int, float **, float *, int *, float *);
void ddct2d(int, int, int, float **, float *, int *, float *);
void ddst2d(int, int, int, float **, float *, int *, float *);
void ddsct2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);
void ddcst2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);

/// 3D FFT ////////////////////////////////////////////////////////////////
void cdft3d(int, int, int, int, float ***, float *, int *, float *);
void rdft3d(int, int, int, int, float ***, float *, int *, float *);
void ddct3d(int, int, int, int, float ***, float *, int *, float *);
void ddst3d(int, int, int, int, float ***, float *, int *, float *);
void ddscct3d(int, int, int, int isgn, float ***, float *, int *, float *);
void ddcsct3d(int, int, int, int isgn, float ***, float *, int *, float *);
void ddccst3d(int, int, int, int isgn, float ***, float *, int *, float *);

}

#endif
