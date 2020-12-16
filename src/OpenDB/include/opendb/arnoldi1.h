///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

// This source code is covered by mutual NDA
// between Athena Design Systems and Blaze DFM
//
// Author: W. Scott
//

#pragma once

#include "odb.h"

namespace odb {

//
// workspace for pole-residue -> delay calculations
//
struct delay_work;
delay_work* delay_work_default();
delay_work* delay_work_create();
void        delay_work_set_thresholds(delay_work* D,
                                      double      lo_thresh,
                                      double      hi_thresh,
                                      bool        rising,
                                      double      slew_derate,
                                      double      mid_thresh);
double      delay_work_get_lo(delay_work* D);
double      delay_work_get_hi(delay_work* D);
double      delay_work_get_slew_derate(delay_work* D);
double      delay_work_get_slew_factor(delay_work* D);
void    delay_work_get_constants(delay_work* D, double* c_log, double* c_smin);
double* delay_work_get_poles(delay_work* D);
double* delay_work_get_residues(delay_work* D, int term_index);

struct delay_c
{
  double slew_derate;
  double vlo;
  double vhi;
  double vlg;
  double smin;
  double x1;
  double y1;
  double vmid;  // falling convention, should be >= 0.5
};
delay_c* delay_work_get_c(delay_work* D);

//
// single-driver arnoldi model
//
class arnoldi1
{
 public:
  int      order;
  int      n;  // number of terms, including driver
  double*  d;  // [order]
  double*  e;  // [order-1]
  double** U;  // [order][n]
  double   ctot;
  double   sqc;

 private:
  static arnoldi1* _tmp_mod;
  static int       _tmp_n;
  static int       _tmp_order;

 public:
  //
  // basic
  //
  arnoldi1()
  {
    order = 0;
    n     = 0;
    d     = NULL;
    e     = NULL;
    U     = NULL;
    ctot  = 0.0;
    sqc   = 0.0;
  }
  ~arnoldi1();
  static arnoldi1* get_tmp(int nterms, int order);
  arnoldi1*        copy();
  void             copy_from(arnoldi1*);
  void             set_lumped(double ctot);
  double           elmore(int term_index);
  double           elmore();  // max
  void             scaleC(double ratio);
  void             scaleCE(double cratio, double eratio);

  //
  // calculate poles/residues for given rdrive
  //
  void calculate_poles_res(delay_work* D, double rdrive);
};

//
// from poles and residues, solve for t20,t50,t80
//
void pr_solve1(double  s,
               int     order,
               double* p,
               double* rr,
               double  v1,
               double* t1);
void pr_solve3(double  s,
               int     order,
               double* p,
               double* rr,
               double  vhi,
               double* thi,
               double  vmid,
               double* tmid,
               double  vlo,
               double* tlo);
void pr_solve(double  s,
              int     order,
              double* p,
              double* rr,
              double* t20,
              double* t50,
              double* t80);

//
// get a waveform point
//
void pr_get_v(double t, double s, int order, double* p, double* rr, double* va);

//
// routines for linear drive model and ceff
//
double pr_ceff(double  s,
               double  rdrive,
               int     order,
               double* p,
               double* rr,
               double  ceff_time);
double ra_solve_for_t(double p, double s, double v);
void   ra_solve_for_s_2080(double p, double t2080, double& s);
void   ra_solve_for_pt(double ps, double v, double* pt, double* d);
void   ra_calc_c(double  lo,
                 double  hi,
                 double* c_smin,
                 double* c_x1,
                 double* c_y1);

//
// workspace for RC network reduction
// Normally, only one instance is used, ar1_reduce::get_default()
//
class ar1_reduce
{
 public:
  static ar1_reduce* get_default();
  ar1_reduce();
  ~ar1_reduce();
  void init(int nterms, int npoints, int max_order);
  void setTerm(int j, int term_index);
  void addR(int j, int k, double r);
  void addC(int j, double c);
  // After init,setTerm,addR,addC, call wrap
  bool wrap();
  //
  // reduce() will return a temporary arnoldi1 model
  // or nil if disconnected
  // This model must be copied or discarded before
  // reduce() is called again.
  //
  arnoldi1*   reduce(int idrv, bool* disc, bool* loop, bool verbose = false);
  int         get_term_index(int i);  // get_term_index(0)=idrv
  static void cleanup();              // only at end, for memory debug
 private:
  void                 dfs(int idrv, bool* disc, bool* loop);
  void                 getR();
  void                 orderTerms(int idrv);
  void                 makeRcmodelFromTs(bool verbose);
  void                 makeRcmodelFromLu(bool verbose);
  arnoldi1*            getTmpArnoldi1();
  struct ar1_internal* W;
  static ar1_reduce*   _default;
};

}  // namespace odb

