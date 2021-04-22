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

#include "OpenRCX/extRCap.h"

#include <wire.h>

#ifdef _WIN32
#include "direct.h"
#endif

#include <map>
#include <vector>

#include "darr.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

static int read_total_cap_file(const char* file,
                               double* ctotV,
                               double* rtotV,
                               int nmax,
                               dbBlock* block,
                               Logger* logger)
{
#ifndef BILL_WAY
  Ath__parser parser;
  parser.openFile((char*) file);

  logger->info(RCX, 184, "Reading ref_file {} ... ... ", file);

  while (parser.parseNextLine() > 0) {
    parser.printWords(stdout);

    uint netid = 0;
    if (parser.isDigit(0, 0))
      netid = parser.getInt(0);
    else {
      char* netName = parser.get(0);
      if (block != NULL) {
        dbNet* net = block->findNet(netName);
        if (net == NULL)
          logger->warn(RCX, 185, "Can't find net {} in db", netName);
      }
    }
    double rtot = 0.0;
    if (parser.getWordCnt() > 2)
      rtot = parser.getDouble(2);

    double ctot = parser.getDouble(1);

    ctotV[netid] = ctot;
    rtotV[netid] = rtot;
  }
#else
  FILE* fp = fopen(file, "r");
  if (!fp) {
    logger_->warn(RCX, 27, "Cannot open {}", file);
    return 0;
  }

  char line[256];
  int netid;
  double ctot, rtot;
  while (fgets(line, 256, fp)) {
    if (3 == sscanf(line, "%d %lf %lf", &netid, &ctot, &rtot)) {
      if (netid < 0 || netid >= nmax) {
        logger_->warn(RCX, 186, "Net id {} out of range in {}", netid, file);
        fclose(fp);
        return 0;
      }
      ctotV[netid] = ctot;
      rtotV[netid] = rtot;
    }
  }
  fclose(fp);
#endif
  return 1;
}

typedef struct
{
  int netid;
  double ctot;
  double cref;
  double cdif;
  double rtot;
  double rref;
  double rdif;
} ext_rctot;

static int ext_rctot_cmp_c(const void* a, const void* b)
{
  ext_rctot* x = (ext_rctot*) a;
  ext_rctot* y = (ext_rctot*) b;
  if (x->cdif < y->cdif)
    return 1;
  if (x->cdif > y->cdif)
    return -1;
  return 0;
}

void extMain::initIncrementalSpef(const char* origp,
                                  const char* newp,
                                  const char* excludeC,
                                  bool noBackSlash)
{
  if (_origSpefFilePrefix)
    free(_origSpefFilePrefix);
  _origSpefFilePrefix = NULL;
  if (origp)
    _origSpefFilePrefix = strdup((char*) origp);
  if (_newSpefFilePrefix)
    free(_newSpefFilePrefix);
  _newSpefFilePrefix = NULL;
  if (newp)
    _newSpefFilePrefix = strdup((char*) newp);
  if (_excludeCells)
    free(_excludeCells);
  _excludeCells = NULL;
  if (excludeC)
    _excludeCells = strdup((char*) excludeC);
  if (!_origSpefFilePrefix && !_newSpefFilePrefix)
    _bufSpefCnt = 0;
  else
    _bufSpefCnt = 1;
  _incrNoBackSlash = noBackSlash;
}
#ifndef DEFAULT_BILL_WAY
void extMain::reportTotalCap(const char* file,
                             bool icap,
                             bool ires,
                             double ccmult,
                             const char* ref,
                             const char* rd_file)
{
  FILE* fp = stdout;
  if (file != NULL)
    fp = fopen(file, "w");
  else
    return;

  if (ref == NULL) {
    dbSet<dbNet> nets = _block->getNets();
    dbSet<dbNet>::iterator nitr;
    for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
      dbNet* net = *nitr;
      float totCap = net->getTotalCapacitance(0, true);
      float ccCap = net->getTotalCouplingCap(0);
      fprintf(fp, "%13.6f %13.6f %s\n", totCap, ccCap, net->getConstName());
    }
  } else {
    Ath__parser parser;
    parser.openFile((char*) ref);
    logger_->info(RCX, 184, "Reading ref_file {} ...", ref);
    while (parser.parseNextLine() > 0) {
      // parser.printWords(stdout);

      dbNet* net = NULL;
      uint netid = 0;
      if (parser.isDigit(0, 0))
        netid = parser.getInt(0);
      else {
        char* netName = parser.get(0);
        if (_block != NULL) {
          net = _block->findNet(netName);
          if (net == NULL) {
            logger_->warn(RCX, 185, "Can't find net {} in db", netName);
            continue;
          }
        }
      }
      if (net == NULL)
        continue;

      if (parser.getWordCnt() < 2)
        continue;

      float totCap = net->getTotalCapacitance(0, true);
      float ccCap = net->getTotalCouplingCap(0);

      double ref_tot = parser.getDouble(1);

      double tot_diff = 100.0 * (ref_tot - totCap) / ref_tot;
      if (parser.getWordCnt() < 3) {
        fprintf(fp,
                "%5.1f %13.6f %13.6f %s\n",
                tot_diff,
                totCap,
                ref_tot,
                net->getConstName());
        continue;
      }

      double ref_cc = parser.getDouble(2);
      double cc_diff = 100.0 * (ref_cc - ccCap) / ref_cc;

      fprintf(fp,
              "%5.1f %5.1f   %13.6f %13.6f   %13.6f %13.6f  %s\n",
              tot_diff,
              cc_diff,
              totCap,
              ref_tot,
              ccCap,
              ref_cc,
              net->getConstName());
    }
  }
  if (file != NULL)
    fclose(fp);
}
#else
void extMain::reportTotalCap(const char* file,
                             bool icap,
                             bool ires,
                             double ccmult,
                             const char* ref,
                             const char* rd_file)
{
  bool cap = icap;
  bool res = ires;
  if (!res && !cap)
    res = cap = true;
  if (ccmult != 1.0)
    logger_->info(RCX, 187, "Cc multiplier {}", ccmult);

  int j, nn = 2 + _block->getNets().size();
  double* ctotV = (double*) malloc(nn * sizeof(double));
  double* rtotV = (double*) malloc(nn * sizeof(double));
  for (j = 0; j < nn; j++)
    ctotV[j] = rtotV[j] = 0.0;

  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator nitr;
  dbNet* net;
  dbCapNode* node;
  if (rd_file && rd_file[0]) {
    if (!read_total_cap_file(rd_file, ctotV, rtotV, nn, _block)) {
      logger_->warn(RCX, 27, "Cannot read {}", rd_file);
      return;
    }
  } else
    for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
      net = *nitr;
      dbSet<dbCapNode> nodeSet = net->getCapNodes();
      dbSet<dbCapNode>::iterator c_itr;
      bool has_foreign = false;
      double ctot = 0.0;
      for (c_itr = nodeSet.begin(); c_itr != nodeSet.end(); ++c_itr) {
        node = *c_itr;
        if (node->isForeign())
          has_foreign = true;
        ctot += node->getCapacitance(0);
      }
      double rtot = 0.0;
      dbSet<dbRSeg> rSet = net->getRSegs();
      dbSet<dbRSeg>::iterator r_itr;
      for (r_itr = rSet.begin(); r_itr != rSet.end(); ++r_itr) {
        dbRSeg* r = *r_itr;
        rtot += r->getResistance(0);
        if (!has_foreign)
          ctot += r->getCapacitance(0);
      }
      double cctot = net->getTotalCouplingCap(0);

      ctot += ccmult * cctot;
      ctot *= 1e-3;
      rtotV[net->getId()] = rtot;
      ctotV[net->getId()] = ctot;
    }

  double* crefV = NULL;
  double* rrefV = NULL;
  if (ref && ref[0]) {
    crefV = (double*) malloc(nn * sizeof(double));
    rrefV = (double*) malloc(nn * sizeof(double));
    for (j = 0; j < nn; j++)
      crefV[j] = rrefV[j] = 0.0;
    if (!read_total_cap_file(ref, crefV, rrefV, nn, _block)) {
      logger_->warn(RCX, 27, "Cannot read {}", ref);
      free(crefV);
      crefV = NULL;
      free(rrefV);
      rrefV = NULL;
    }
  }

  FILE* fp = NULL;
  if (file && file[0])
    fp = fopen(file, "w");
  if (fp && !crefV) {
    for (j = 0; j < nn; j++)
      if (ctotV[j] > 0.0 || rtotV[j] > 0.0) {
        if (cap && res)
          fprintf(fp, "%d %g %.2f\n", j, ctotV[j], rtotV[j]);
        else if (cap)
          fprintf(fp, "%d %g\n", j, ctotV[j]);
        else
          fprintf(fp, "%d %.2f\n", j, rtotV[j]);
      }
  } else if (crefV) {
    double max_abs_c = 0.0;
    double max_abs_r = 0.0;
    double abs_cthresh = 0.001;    // pf
    double abs_cthresh2 = 0.0005;  // pf
    double abs_rthresh = 1.0;      // ohms

    double rel_cfloor = 0.010;   // 10 ff
    double rel_cfloor2 = 0.001;  // 1 ff
    double max_rel_c = 0.0;
    double max_rel_c2 = 0.0;
    uint max_rel_c_id = 0;
    uint max_rel_c_id2 = 0;
    double rel_cthresh = 0.02;  // 2 percent
    int rel_cn = 0;             // number over rel_cthresh
    int rel_cn2 = 0;

    ext_rctot* rctotV = (ext_rctot*) malloc(nn * sizeof(ext_rctot));
    int rctotN = 0;
    ext_rctot* rctot;
    double cdif, rdif, cdif_avg = 0.0, rdif_avg = 0.0;
    double crel, cden;
    int ncdif = 0, nrdif = 0, ncdif2 = 0;
    int crel_cnt = 0, crel_cnt2 = 0;
    for (j = 0; j < nn; j++) {
      if (ctotV[j] == 0.0 && rtotV[j] == 0.0 && crefV[j] == 0.0
          && rrefV[j] == 0.0)
        continue;
      cdif = ctotV[j] - crefV[j];
      rdif = rtotV[j] - rrefV[j];
      cdif_avg += cdif;
      rdif_avg += rdif;
      if (cdif >= abs_cthresh || cdif <= -abs_cthresh)
        ncdif++;
      if (cdif >= abs_cthresh2 || cdif <= -abs_cthresh2)
        ncdif2++;
      if (rdif >= abs_rthresh || rdif <= -abs_rthresh)
        nrdif++;
      rctot = rctotV + rctotN++;
      rctot->netid = j;
      rctot->ctot = ctotV[j];
      rctot->cref = crefV[j];
      rctot->cdif = cdif;
      rctot->rtot = rtotV[j];
      rctot->rref = rrefV[j];
      rctot->rdif = rdif;

      if (cdif < 0.0)
        cdif = -cdif;
      if (cdif > max_abs_c) {
        max_abs_c = cdif;
      }
      if (rdif < 0.0)
        rdif = -rdif;
      if (rdif > max_abs_r) {
        max_abs_r = rdif;
      }
      cden = (crefV[j] >= ctotV[j] ? crefV[j] : ctotV[j]);
      if (cden >= rel_cfloor) {
        crel_cnt++;
        crel = cdif / cden;
        if (crel > max_rel_c) {
          max_rel_c = crel;
          max_rel_c_id = j;
        }
        if (crel >= rel_cthresh) {
          rel_cn++;
        }
      }
      if (cden >= rel_cfloor2) {
        crel_cnt2++;
        crel = cdif / cden;
        if (crel > max_rel_c2) {
          max_rel_c2 = crel;
          max_rel_c_id2 = j;
        }
        if (crel >= rel_cthresh) {
          rel_cn2++;
        }
      }
    }
    logger_->info(
        RCX,
        188,
        "Comparing all {} signal nets worst abs ctot diff = {:.4f) pF "
        "worst abs rtot diff = {:.2f) ohms",
        rctotN,
        max_abs_c,
        max_abs_r);

    logger_->info(RCX,
                  189,
                  "Comparing nets with c>={:.4f}, {} nets"
                  " worst percent ctot diff = {:.1f} %% for c>={:.4f}",
                  rel_cfloor,
                  crel_cnt,
                  100.0 * max_rel_c,
                  rel_cfloor);
    if (max_rel_c_id) {
      j = max_rel_c_id;
      net = dbNet::getNet(_block, j);
      logger_->info(
          RCX,
          190,
          "\tNet {} {}  c_tot {:.4f} c_ref {:.4f} r_tot {:.2f} r_ref {:.2f}",
          net->getId(),
          net->getName().c_str(),
          ctotV[j],
          crefV[j],
          rtotV[j],
          rrefV[j]);
    }
    logger_->info(RCX,
                  191,
                  "\t{} nets have ctot diff >= {:.1f} %% for c>={:.4f}",
                  rel_cn,
                  100.0 * rel_cthresh,
                  rel_cfloor);

    logger_->info(RCX,
                  189,
                  "Comparing nets with c>={:.4f}, {} nets"
                  " worst percent ctot diff = {:.1f} %% for c>={:.4f}",
                  rel_cfloor2,
                  crel_cnt2,
                  100.0 * max_rel_c2,
                  rel_cfloor2);

    if (max_rel_c_id2) {
      j = max_rel_c_id2;
      net = dbNet::getNet(_block, j);
      logger_->info(
          RCX,
          190,
          "\tNet {} {}  c_tot {:.4f} c_ref {:.4f} r_tot {:.2f} r_ref {:.2f}",
          net->getId(),
          net->getName().c_str(),
          ctotV[j],
          crefV[j],
          rtotV[j],
          rrefV[j]);
    }

    logger_->info(RCX,
                  189,
                  "Comparing nets with c>={:.4f}, {} nets"
                  " worst percent ctot diff = {:.1f} %% for c>={:.4f}",
                  rel_cfloor2,
                  crel_cnt2,
                  100.0 * max_rel_c2,
                  rel_cfloor2);

    if (rctotN > 0) {
      cdif_avg /= rctotN;
      rdif_avg /= rctotN;
      qsort(rctotV, rctotN, sizeof(ext_rctot), ext_rctot_cmp_c);
      rctot = rctotV;
      if (rctot->cdif > 0.0) {
        logger_->info(
            RCX, 191, "Max c_tot diff vs ref = {:.4f} pF", rctot->cdif);
        net = dbNet::getNet(_block, rctot->netid);
        logger_->info(
            RCX,
            190,
            "\tNet {} {}  c_tot {:.4f} c_ref {:.4f} r_tot {:.2f} r_ref {:.2f}",
            net->getId(),
            net->getName().c_str(),
            rctot->ctot,
            rctot->cref,
            rctot->rtot,
            rctot->rref);
      }
      rctot = rctotV + rctotN - 1;
      if (rctot->cdif < 0.0) {
        logger_->info(
            RCX, 192, "Min c_tot diff vs ref = {:.4f} pF", rctot->cdif);
        net = dbNet::getNet(_block, rctot->netid);
        logger_->info(
            RCX,
            190,
            "\tNet {} {}  c_tot {:.4f} c_ref {:.4f} r_tot {:.2f} r_ref {:.2f}",
            net->getId(),
            net->getName().c_str(),
            rctot->ctot,
            rctot->cref,
            rctot->rtot,
            rctot->rref);
      }
      logger_->info(RCX,
                    193,
                    "Avg ctot diff vs ref = {:.4f} pf"
                    "\nAvg rtot diff vs ref = {:.2f} ohms"
                    "\n{} nets have absolute ctot diff >= {:.4f}",
                    cdif_avg,
                    rdif_avg,
                    ncdif,
                    abs_cthresh);
      logger_->info(RCX,
                    194,
                    "{} nets have absolute ctot diff >= {:.4f}",
                    ncdif2,
                    abs_cthresh2);
      logger_->info(RCX,
                    195,
                    "{} nets have absolute rtot diff >= :.2f}",
                    nrdif,
                    abs_rthresh);
      if (fp && ncdif + nrdif) {
        fprintf(fp, "# netid cdif ctot cref rdif rtot rref\n");
        for (j = 0; j < rctotN; j++) {
          rctot = rctotV + j;
          if (rctot->cdif < abs_cthresh2 && rctot->cdif > -abs_cthresh2
              && rctot->rdif < abs_rthresh && rctot->rdif > -abs_rthresh)
            continue;
          fprintf(fp,
                  "%6d %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f\n",
                  rctot->netid,
                  rctot->cdif,
                  rctot->ctot,
                  rctot->cref,
                  rctot->rdif,
                  rctot->rtot,
                  rctot->rref);
        }
      }
    }
    free(rctotV);
  }
  free(ctotV);
  free(rtotV);
  if (crefV)
    free(crefV);
  if (rrefV)
    free(rrefV);
  if (fp)
    fclose(fp);
}
#endif

/////////////////////////////////////////////////////

typedef struct
{
  int netid0;
  int netid1;
  double cctot;
  double ccref;
  double ccdif;
} ext_cctot;

static bool read_total_cc_file(const char* file,
                               Darr<ext_cctot>& V,
                               Logger* logger)
{
  FILE* fp = fopen(file, "r");
  if (!fp) {
    logger->warn(RCX, 27, "Cannot open {}", file);
    return false;
  }
  char line[256];
  ext_cctot x;
  x.ccref = x.ccdif = 0.0;
  while (fgets(line, 256, fp)) {
    if (3 == sscanf(line, "%d %d %lf", &x.netid0, &x.netid1, &x.cctot)) {
      V.insert(x);
    }
  }
  fclose(fp);
  return true;
}

static int ext_cctot_cmp(const void* a, const void* b)
{
  ext_cctot* x = (ext_cctot*) a;
  ext_cctot* y = (ext_cctot*) b;
  if (x->ccdif < y->ccdif)
    return 1;
  if (x->ccdif > y->ccdif)
    return -1;
  return 0;
}

static bool read_ref_cc_file(const char* file,
                             int netn,
                             Darr<ext_cctot>& V,
                             Logger* logger)
{
  FILE* fp = fopen(file, "r");
  if (!fp) {
    logger->warn(RCX, 27, "Cannot open {}", file);
    return false;
  }
  int* indV = (int*) malloc((2 + netn) * sizeof(int));
  int nn = V.n();
  char line[256];
  ext_cctot x;
  int j;
  int netid0 = 0;
  for (j = 0; j < nn; j++) {
    x = V.get(j);
    if (netid0 < x.netid0) {
      while (++netid0 < x.netid0) {
        indV[netid0] = j;
      }
      indV[netid0] = j;
    }
  }
  indV[netid0 + 1] = j;

  int netid1;
  double ccref;
  while (fgets(line, 256, fp)) {
    if (3 == sscanf(line, "%d %d %lf", &netid0, &netid1, &ccref)) {
      int j0 = indV[netid0];
      int j1 = indV[netid0 + 1];
      if (j0 >= j1)
        continue;
      for (j = j0; j < j1; j++) {
        x = V.get(j);
        if (x.netid1 == netid1) {
          x.ccref = ccref;
          V.set(j, x);
          break;
        }
      }
      if (j < j1)
        continue;
      // not found
      x.netid0 = netid0;
      x.netid1 = netid1;
      x.cctot = 0.0;
      x.ccref = ccref;
      x.ccdif = -ccref;
      V.insert(x);
    }
  }

  free(indV);
  return true;
}

void extMain::reportTotalCc(const char* file,
                            const char* ref,
                            const char* rd_file)
{
  Darr<ext_cctot> V;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator nitr;
  dbNet *net0, *net1;
  std::vector<dbNet*> netV;
  std::vector<double> ccV;
  //  dbSet<dbCCSeg> ccSet;
  std::vector<dbCCSeg*>::iterator cc_itr;

  dbCCSeg* cc;
  int j, nn, netN;
  ext_cctot x;
  x.ccref = 0.0;
  x.ccdif = 0.0;
  if (rd_file) {
    if (!read_total_cc_file(rd_file, V, logger_) || V.n() < 1) {
      logger_->warn(RCX, 27, "Cannot read {}", rd_file);
      return;
    }
  } else
    for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
      net0 = *nitr;
      netN = 0;
      netV.clear();
      ccV.clear();

      std::vector<dbCCSeg*> ccSet1;
      net0->getSrcCCSegs(ccSet1);
      for (cc_itr = ccSet1.begin(); cc_itr != ccSet1.end(); ++cc_itr) {
        cc = *cc_itr;
        net1 = cc->getTargetNet();
        if (net1->getId() > net0->getId())
          continue;
        for (j = 0; j < netN; j++)
          if (net1 == netV[j])
            break;
        if (j == netN) {
          netV.push_back(net1);
          ccV.push_back(0.0);
          netN++;
        }
        ccV[j] += cc->getCapacitance(0);
      }
      std::vector<dbCCSeg*> ccSet2;
      net0->getTgtCCSegs(ccSet2);
      for (cc_itr = ccSet2.begin(); cc_itr != ccSet2.end(); ++cc_itr) {
        cc = *cc_itr;
        net1 = cc->getSourceNet();
        if (net1->getId() > net0->getId())
          continue;
        for (j = 0; j < netN; j++)
          if (net1 == netV[j])
            break;
        if (j == netN) {
          netV.push_back(net1);
          ccV.push_back(0.0);
          netN++;
        }
        ccV[j] += cc->getCapacitance(0);
      }
      for (j = 0; j < netN; j++) {
        x.netid0 = net0->getId();
        x.netid1 = netV[j]->getId();
        x.cctot = 1e-3 * ccV[j];
        V.insert(x);
      }
    }
  if (ref) {
    if (!read_ref_cc_file(ref, nets.size(), V, logger_)) {
      logger_->warn(RCX, 27, "Cannot read {}", ref);
      return;
    }
  }
  nn = V.n();
  FILE* fp = NULL;
  if (file)
    fp = fopen(file, "w");
  if (fp && !ref) {
    for (j = 0; j < nn; j++) {
      x = V.get(j);
      fprintf(fp, "%d %d %g\n", x.netid0, x.netid1, x.cctot);
    }
  } else if (ref) {
    double max_abs_cc = 0.0;
    double abs_ccthresh = 5e-4;   // pf,  so 0.5 ff
    double abs_ccthresh2 = 1e-4;  // 0.1 ff
    int nccdif = 0;
    int nccdif2 = 0;
    for (j = 0; j < nn; j++) {
      x = V.get(j);
      x.ccdif = x.cctot - x.ccref;
      V.set(j, x);
      double difabs = (x.ccdif < 0 ? -x.ccdif : x.ccdif);
      if (difabs >= abs_ccthresh)
        nccdif++;
      if (difabs >= abs_ccthresh2)
        nccdif2++;
      if (difabs > max_abs_cc) {
        max_abs_cc = difabs;
      }
    }
    logger_->info(
        RCX, 196, "Worst abs cctot(net0, net1) diff = {} pF", max_abs_cc);
    logger_->info(RCX,
                  197,
                  "{} net pairs have absolute cctot diff > {} pF",
                  nccdif,
                  abs_ccthresh);
    logger_->info(RCX,
                  197,
                  "{} net pairs have absolute cctot diff > {} pF",
                  nccdif2,
                  abs_ccthresh2);
    V.dsort(ext_cctot_cmp);
    dbNet *net0, *net1;
    x = V.get(0);
    if (x.ccdif > 0.0) {
      logger_->info(RCX, 198, "Max cc_tot diff vs ref = {} pF", x.ccdif);
      net0 = dbNet::getNet(_block, x.netid0);
      net1 = dbNet::getNet(_block, x.netid1);
      logger_->info(
          RCX, 199, "Net0 {} {}", net0->getId(), net0->getConstName());
      logger_->info(
          RCX, 200, "Net1 {} {}", net1->getId(), net1->getConstName());
      logger_->info(
          RCX, 201, "Cc_tot {} cc_ref {} cc_dif {}", x.cctot, x.ccref, x.ccdif);
    }
    x = V.get(nn - 1);
    if (x.ccdif < 0.0) {
      logger_->info(RCX, 202, "Min cc_tot diff vs ref = {} pF", x.ccdif);
      net0 = dbNet::getNet(_block, x.netid0);
      net1 = dbNet::getNet(_block, x.netid1);
      logger_->info(
          RCX, 199, "Net0 {} {}", net0->getId(), net0->getConstName());
      logger_->info(
          RCX, 200, "Net1 {} {}", net1->getId(), net1->getConstName());
      logger_->info(
          RCX, 201, "Cc_tot {} cc_ref {} cc_dif {}", x.cctot, x.ccref, x.ccdif);
    }
    if (fp) {
      fprintf(fp, "# netid0 netid1 cctot ccref ccdif\n");
      for (j = 0; j < nn; j++) {
        x = V.get(j);
        if (x.ccdif < abs_ccthresh && x.ccdif > -abs_ccthresh)
          continue;
        fprintf(fp,
                "%6d %6d %8g %8g %8g\n",
                x.netid0,
                x.netid1,
                x.cctot,
                x.ccref,
                x.ccdif);
      }
    }
  }
  if (fp)
    fclose(fp);
}

/////////////////////////////////////////////////////

void extMain::extDump(char* file,
                      bool openTreeFile,
                      bool closeTreeFile,
                      bool ccCapGeom,
                      bool ccNetGeom,
                      bool trackCnt,
                      bool signal,
                      bool power,
                      uint layer)
{
  Ath__searchBox bb;
  ZPtr<ISdb> targetSdb;
  if (closeTreeFile) {
    if (_ptFile)
      fclose(_ptFile);
    _ptFile = NULL;
    _block->setPtFile(NULL);
    return;
  } else if (ccCapGeom)
    targetSdb = _extCcapSDB;
  else if (ccNetGeom || trackCnt)
    targetSdb = _extNetSDB;
  else if (!openTreeFile && !trackCnt)
    return;
  if (!file || !file[0]) {
    logger_->warn(RCX, 203, "Please input extDump filename!");
    return;
  }
  FILE* fp = fopen(file, "w");
  if (!fp) {
    logger_->warn(RCX, 27, "Cannot open file {}", file);
    return;
  }
  if (openTreeFile) {
    _ptFile = fp;
    _block->setPtFile(fp);
    return;
  }
  if (trackCnt) {
    targetSdb->dumpTrackCounts(fp);
    fclose(fp);
    return;
  }
  bool dumpSignalWire = true;
  bool dumpPowerWire = true;
  if (signal && !power)
    dumpPowerWire = false;
  if (!signal && power)
    dumpSignalWire = false;
  bool* etable = (bool*) malloc(100 * sizeof(bool));
  bool init = layer == 0 ? false : true;
  for (uint idx = 0; idx < 100; idx++)
    etable[idx] = init;
  etable[layer] = false;
  targetSdb->resetMaxArea();
  dbBox* tbox = _block->getBBox();
  targetSdb->searchWireIds(
      tbox->xMin(), tbox->yMin(), tbox->xMax(), tbox->yMax(), false, etable);
  targetSdb->startIterator();
  uint wid, wflags;
  while ((wid = targetSdb->getNextWireId())) {
    if (ccNetGeom) {
      wflags = targetSdb->getSearchPtr()->getWirePtr(wid)->getFlags();
      if ((wflags == 1 && !dumpPowerWire) || (wflags == 2 && !dumpSignalWire))
        continue;
    }
    targetSdb->getSearchPtr()->getCoords(&bb, wid);
    fprintf(fp,
            "m%d %d %d  %d %d\n",
            bb.getLevel(),
            bb.loXY(0),
            bb.loXY(1),
            bb.hiXY(0),
            bb.hiXY(1));
  }
  if (fp)
    fclose(fp);
}
void extMain::extCount(bool signalWireSeg, bool powerWireSeg)
{
  if (!signalWireSeg && !powerWireSeg)
    signalWireSeg = powerWireSeg = true;
  uint signalViaCnt = 0;
  uint signalWireCnt = 0;
  uint powerViaCnt = 0;
  uint powerWireCnt = 0;
  dbSet<dbNet> bnets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;
  dbNet* net;
  for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
    net = *net_itr;
    dbSigType type = net->getSigType();
    if ((type == dbSigType::POWER) || (type == dbSigType::GROUND))
      net->getPowerWireCount(powerWireCnt, powerViaCnt);
    else
      net->getSignalWireCount(signalWireCnt, signalViaCnt);
  }
  if (signalWireSeg)
    logger_->info(RCX,
                  204,
                  "{} signal seg ({} wire, {} via)",
                  signalWireCnt + signalViaCnt,
                  signalWireCnt,
                  signalViaCnt);
  if (powerWireSeg)
    logger_->info(RCX,
                  205,
                  "{} power seg ({} wire, {} via)",
                  powerWireCnt + powerViaCnt,
                  powerWireCnt,
                  powerViaCnt);
}
/*
class extNetStats
{
        double _tcap[2];
        double _ccap[2];
        double _cc2tcap[2];
        double _cc[2];
        uint _len[2];
        uint _layerCnt[2];
        uint _wCnt[2];
        uint _vCnt[2];
        uint _resCnt[2];
        uint _ccCnt[2];
        uint _gndCnt[2];
        uint _id;
        bool _layerFilter[20];
        Rect _bbox;
};
*/
bool extMain::outOfBounds_i(int limit[2], int v)
{
  if ((v < limit[0]) || (v > limit[1]))
    return true;
  else
    return false;
}
bool extMain::outOfBounds_d(double limit[2], double v)
{
  if ((v < limit[0]) || (v > limit[1]))
    return true;
  else
    return false;
}
bool extMain::printNetRC(char* buff, dbNet* net, extNetStats* st)
{
  double tCap = net->getTotalCapacitance(0, true);
  if (tCap <= 0.0)
    return false;

  if (outOfBounds_d(st->_tcap, tCap))
    return false;

  double ccCap = net->getTotalCouplingCap();
  if (outOfBounds_d(st->_ccap, ccCap))
    return false;

  double cc2tcap = 0.0;
  if (ccCap > 0.0) {
    cc2tcap = ccCap / tCap;
    if (outOfBounds_d(st->_cc2tcap, cc2tcap))
      return false;
  } else
    return false;

  double res = net->getTotalResistance();
  if (outOfBounds_d(st->_res, res))
    return false;

  //	fprintf(fp, "C %8.3 CC %8.3f CCr %3.1f R %8.2f maxCC= %8.4f ",
  //		tCap, ccCap, cc2tcap, res, maxCC);
  sprintf(buff, "C %.3f CC %.3f CCr %2.1f R %.1f ", tCap, ccCap, cc2tcap, res);

  return true;
}
bool extMain::printNetDB(char* buff, dbNet* net, extNetStats* st)
{
  uint termCnt = net->getTermCount();
  if (outOfBounds_i(st->_termCnt, termCnt))
    return false;
  uint btermCnt = net->getBTermCount();
  if (outOfBounds_i(st->_btermCnt, btermCnt))
    return false;

  uint wireCnt = 0;
  uint viaCnt = 0;
  uint len = 0;
  uint layerCnt = 0;

  uint layerTable[12];
  for (uint jj = 0; jj < 12; jj++)
    layerTable[jj] = 0;

  net->getNetStats(wireCnt, viaCnt, len, layerCnt, layerTable);
  uint ii = 1;
  for (; ii < 12; ii++) {
    if (layerTable[ii] == 0)
      continue;

    layerCnt++;

    if (!st->_layerFilter[ii])
      return false;
  }

  if (outOfBounds_i(st->_vCnt, viaCnt))
    return false;
  if (outOfBounds_i(st->_wCnt, wireCnt))
    return false;
  if (outOfBounds_i(st->_len, len))
    return false;
  if (outOfBounds_i(st->_layerCnt, layerCnt))
    return false;

  char buf1[256];

  sprintf(buf1, "%s", "\0");

  ii = 1;
  for (; ii < 12; ii++) {
    if (layerTable[ii] > 0) {
      sprintf(buf1, "%s%d", buf1, ii);
      break;
    }
  }
  ii++;
  for (; ii < 12; ii++) {
    if (layerTable[ii] == 0)
      continue;

    sprintf(buf1, "%s.%d", buf1, ii);
  }
  sprintf(buff,
          "V %d Wc %d  L %d  %dM%s T %d B %d",
          viaCnt,
          wireCnt,
          len / 1000,
          layerCnt,
          buf1,
          termCnt,
          btermCnt);
  return true;
}

uint extMain::printNetStats(FILE* fp,
                            dbBlock* block,
                            extNetStats* bounds,
                            bool skipRC,
                            bool skipDb,
                            bool skipPower,
                            std::list<int>* list_of_nets)
{
  Rect bbox(bounds->_ll[0], bounds->_ll[1], bounds->_ur[0], bounds->_ur[1]);

  char buff1[256];
  char buff2[256];
  uint cnt = 0;
  dbSet<dbNet> bnets = block->getNets();
  dbSet<dbNet>::iterator net_itr;
  for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    //		uint viaCnt = 0;
    //		uint wireCnt = 0;

    dbSigType type = net->getSigType();
    if ((type == dbSigType::POWER) || (type == dbSigType::GROUND)) {
      if (skipPower)
        continue;
      // net->getPowerWireCount (powerWireCnt, powerViaCnt);
      continue;
    }
    if (!net->isEnclosed(&bbox))
      continue;
    if (!skipRC) {
      if (!printNetRC(buff1, net, bounds))
        continue;
    }
    if (!skipDb) {
      if (!printNetDB(buff2, net, bounds))
        continue;
    }
    fprintf(fp, "%s %s ", buff1, buff2);
    net->printNetName(fp, true, true);

    list_of_nets->push_back(net->getId());
    cnt++;
  }
  return cnt;
}
void extNetStats::reset()
{
  _tcap[0] = 0.0;
  _tcap[1] = 1.0 * INT_MAX;
  _ccap[0] = 0.0;
  _ccap[1] = 1.0 * INT_MAX;
  _cc2tcap[0] = 0.0;
  _cc2tcap[1] = 1.0 * INT_MAX;
  _cc[0] = 0.0;
  _cc[1] = 1.0 * INT_MAX;
  _res[0] = 0.0;
  _res[1] = 1.0 * INT_MAX;

  _len[0] = 0;
  _len[1] = INT_MAX;
  _layerCnt[0] = 0;
  _layerCnt[1] = INT_MAX;
  _wCnt[0] = 0;
  _wCnt[1] = INT_MAX;
  _vCnt[0] = 0;
  _vCnt[1] = INT_MAX;
  _resCnt[0] = 0;
  _resCnt[1] = INT_MAX;
  _ccCnt[0] = 0;
  _ccCnt[1] = INT_MAX;
  _gndCnt[0] = 0;
  _gndCnt[1] = INT_MAX;
  _termCnt[0] = 0;
  _termCnt[1] = INT_MAX;
  _btermCnt[0] = 0;
  _btermCnt[1] = INT_MAX;
  _id = 0;
  _ll[0] = INT_MIN;
  _ll[1] = INT_MIN;
  _ur[0] = INT_MAX;
  _ur[1] = INT_MAX;

  for (uint ii = 0; ii < 20; ii++)
    _layerFilter[ii] = true;
}
void extNetStats::update_double_limits(int n,
                                       double v1,
                                       double v2,
                                       double* val,
                                       double units)
{
  if (n == 0) {
    val[0] = v1 * units;
    val[1] = v2 * units;
  } else if (n == 1)
    val[0] = v1 * units;
  else if (n == 2)
    val[1] = v2 * units;
}
void extNetStats::update_double(Ath__parser* parser,
                                const char* word,
                                double* val,
                                double units)
{
  double v1, v2;
  int n = parser->get2Double(word, ":", v1, v2);

  if (n >= 0)
    update_double_limits(n, v1, v2, val, units);
}
void extNetStats::update_int_limits(int n, int v1, int v2, int* val, uint units)
{
  if (n == 0) {
    val[0] = v1 * units;
    val[1] = v2 * units;
  } else if (n == 1)
    val[0] = v1 * units;
  else if (n == 2)
    val[1] = v2 * units;
}
void extNetStats::update_int(Ath__parser* parser,
                             const char* word,
                             int* val,
                             uint units)
{
  int v1, v2;
  int n = parser->get2Int(word, ":", v1, v2);

  if (n >= 0)
    update_int_limits(n, v1, v2, val, units);
}
void extNetStats::update_bbox(Ath__parser* parser, const char* bbox)
{
  int n = parser->mkWords(bbox, " ");

  if (n <= 3) {
    logger_->warn(RCX, 206, "Invalid bbox <{}>! Assuming entire block.", bbox);
    return;
  }
  _ll[0] = parser->getInt(0);
  _ll[1] = parser->getInt(1);
  _ur[0] = parser->getInt(2);
  _ur[1] = parser->getInt(3);
}
}  // namespace rcx
