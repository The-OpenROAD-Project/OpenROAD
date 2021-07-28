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

#include "rcx/extRCap.h"

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

typedef struct {
  int netid;
  double ctot;
  double cref;
  double cdif;
  double rtot;
  double rref;
  double rdif;
} ext_rctot;

void extMain::initIncrementalSpef(const char* origp, const char* newp,
                                  const char* excludeC, bool noBackSlash) {
  if (_origSpefFilePrefix)
    free(_origSpefFilePrefix);
  _origSpefFilePrefix = NULL;
  if (origp)
    _origSpefFilePrefix = strdup((char*)origp);
  if (_newSpefFilePrefix)
    free(_newSpefFilePrefix);
  _newSpefFilePrefix = NULL;
  if (newp)
    _newSpefFilePrefix = strdup((char*)newp);
  if (_excludeCells)
    free(_excludeCells);
  _excludeCells = NULL;
  if (excludeC)
    _excludeCells = strdup((char*)excludeC);
  if (!_origSpefFilePrefix && !_newSpefFilePrefix)
    _bufSpefCnt = 0;
  else
    _bufSpefCnt = 1;
  _incrNoBackSlash = noBackSlash;
}

void extMain::reportTotalCap(const char* file, bool icap, bool ires,
                             double ccmult, const char* ref,
                             const char* rd_file) {
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
    parser.openFile((char*)ref);
    logger_->info(RCX, 184, "Reading ref_file {} ...", ref);
    while (parser.parseNextLine() > 0) {
      // parser.printWords(stdout);

      dbNet* net = NULL;
      if (parser.isDigit(0, 0))
        parser.getInt(0);
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
        fprintf(fp, "%5.1f %13.6f %13.6f %s\n", tot_diff, totCap, ref_tot,
                net->getConstName());
        continue;
      }

      double ref_cc = parser.getDouble(2);
      double cc_diff = 100.0 * (ref_cc - ccCap) / ref_cc;

      fprintf(fp, "%5.1f %5.1f   %13.6f %13.6f   %13.6f %13.6f  %s\n", tot_diff,
              cc_diff, totCap, ref_tot, ccCap, ref_cc, net->getConstName());
    }
  }
  if (file != NULL)
    fclose(fp);
}

/////////////////////////////////////////////////////

typedef struct {
  int netid0;
  int netid1;
  double cctot;
  double ccref;
  double ccdif;
} ext_cctot;

static bool read_total_cc_file(const char* file, Darr<ext_cctot>& V,
                               Logger* logger) {
  FILE* fp = fopen(file, "r");
  if (!fp) {
    logger->warn(RCX, 33, "Can't open {}", file);
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

static int ext_cctot_cmp(const void* a, const void* b) {
  ext_cctot* x = (ext_cctot*)a;
  ext_cctot* y = (ext_cctot*)b;
  if (x->ccdif < y->ccdif)
    return 1;
  if (x->ccdif > y->ccdif)
    return -1;
  return 0;
}

static bool read_ref_cc_file(const char* file, int netn, Darr<ext_cctot>& V,
                             Logger* logger) {
  FILE* fp = fopen(file, "r");
  if (!fp) {
    logger->warn(RCX, 34, "Can't open {}", file);
    return false;
  }
  int* indV = (int*)malloc((2 + netn) * sizeof(int));
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

void extMain::reportTotalCc(const char* file, const char* ref,
                            const char* rd_file) {
  Darr<ext_cctot> V;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator nitr;
  dbNet* net0, *net1;
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
      logger_->warn(RCX, 35, "Can't read {}", rd_file);
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
      logger_->warn(RCX, 37, "Can't read {}", ref);
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
    logger_->info(RCX, 196, "Worst abs cctot(net0, net1) diff = {} pF",
                  max_abs_cc);
    logger_->info(RCX, 197, "{} net pairs have absolute cctot diff > {} pF",
                  nccdif, abs_ccthresh);
    logger_->info(RCX, 207, "{} net pairs have absolute cctot diff > {} pF",
                  nccdif2, abs_ccthresh2);
    V.dsort(ext_cctot_cmp);
    dbNet* net0, *net1;
    x = V.get(0);
    if (x.ccdif > 0.0) {
      logger_->info(RCX, 198, "Max cc_tot diff vs ref = {} pF", x.ccdif);
      net0 = dbNet::getNet(_block, x.netid0);
      net1 = dbNet::getNet(_block, x.netid1);
      logger_->info(RCX, 199, "Net0 {} {}", net0->getId(),
                    net0->getConstName());
      logger_->info(RCX, 200, "Net1 {} {}", net1->getId(),
                    net1->getConstName());
      logger_->info(RCX, 201, "Cc_tot {} cc_ref {} cc_dif {}", x.cctot, x.ccref,
                    x.ccdif);
    }
    x = V.get(nn - 1);
    if (x.ccdif < 0.0) {
      logger_->info(RCX, 202, "Min cc_tot diff vs ref = {} pF", x.ccdif);
      net0 = dbNet::getNet(_block, x.netid0);
      net1 = dbNet::getNet(_block, x.netid1);
      logger_->info(RCX, 420, "Net0 {} {}", net0->getId(),
                    net0->getConstName());
      logger_->info(RCX, 419, "Net1 {} {}", net1->getId(),
                    net1->getConstName());
      logger_->info(RCX, 421, "Cc_tot {} cc_ref {} cc_dif {}", x.cctot, x.ccref,
                    x.ccdif);
    }
    if (fp) {
      fprintf(fp, "# netid0 netid1 cctot ccref ccdif\n");
      for (j = 0; j < nn; j++) {
        x = V.get(j);
        if (x.ccdif < abs_ccthresh && x.ccdif > -abs_ccthresh)
          continue;
        fprintf(fp, "%6d %6d %8g %8g %8g\n", x.netid0, x.netid1, x.cctot,
                x.ccref, x.ccdif);
      }
    }
  }
  if (fp)
    fclose(fp);
}

/////////////////////////////////////////////////////

void extMain::extDump(char* file, bool openTreeFile, bool closeTreeFile,
                      bool ccCapGeom, bool ccNetGeom, bool trackCnt,
                      bool signal, bool power, uint layer) {
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
    logger_->warn(RCX, 203, "Filename is not defined (extDump)!");
    return;
  }
  FILE* fp = fopen(file, "w");
  if (!fp) {
    logger_->warn(RCX, 38, "Can't open file {}", file);
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
  bool* etable = (bool*)malloc(100 * sizeof(bool));
  bool init = layer == 0 ? false : true;
  for (uint idx = 0; idx < 100; idx++)
    etable[idx] = init;
  etable[layer] = false;
  targetSdb->resetMaxArea();
  dbBox* tbox = _block->getBBox();
  targetSdb->searchWireIds(tbox->xMin(), tbox->yMin(), tbox->xMax(),
                           tbox->yMax(), false, etable);
  targetSdb->startIterator();
  uint wid, wflags;
  while ((wid = targetSdb->getNextWireId())) {
    if (ccNetGeom) {
      wflags = targetSdb->getSearchPtr()->getWirePtr(wid)->getFlags();
      if ((wflags == 1 && !dumpPowerWire) || (wflags == 2 && !dumpSignalWire))
        continue;
    }
    targetSdb->getSearchPtr()->getCoords(&bb, wid);
    fprintf(fp, "m%d %d %d  %d %d\n", bb.getLevel(), bb.loXY(0), bb.loXY(1),
            bb.hiXY(0), bb.hiXY(1));
  }
  if (fp)
    fclose(fp);
}
void extMain::extCount(bool signalWireSeg, bool powerWireSeg) {
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
    logger_->info(RCX, 204, "{} signal seg ({} wire, {} via)",
                  signalWireCnt + signalViaCnt, signalWireCnt, signalViaCnt);
  if (powerWireSeg)
    logger_->info(RCX, 205, "{} power seg ({} wire, {} via)",
                  powerWireCnt + powerViaCnt, powerWireCnt, powerViaCnt);
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
bool extMain::outOfBounds_i(int limit[2], int v) {
  if ((v < limit[0]) || (v > limit[1]))
    return true;
  else
    return false;
}
bool extMain::outOfBounds_d(double limit[2], double v) {
  if ((v < limit[0]) || (v > limit[1]))
    return true;
  else
    return false;
}
bool extMain::printNetRC(char* buff, dbNet* net, extNetStats* st) {
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
bool extMain::printNetDB(char* buff, dbNet* net, extNetStats* st) {
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
  std::string str;

  ii = 1;
  for (; ii < 12; ii++) {
    if (layerTable[ii] > 0) {
      sprintf(buf1, "%d", ii);
      str += buf1;
      break;
    }
  }
  ii++;
  for (; ii < 12; ii++) {
    if (layerTable[ii] == 0)
      continue;

    sprintf(buf1, ".%d", ii);
    str += buf1;
  }
  sprintf(buff, "V %d Wc %d  L %d  %dM%s T %d B %d", viaCnt, wireCnt,
          len / 1000, layerCnt, str.c_str(), termCnt, btermCnt);
  return true;
}

uint extMain::printNetStats(FILE* fp, dbBlock* block, extNetStats* bounds,
                            bool skipRC, bool skipDb, bool skipPower,
                            std::list<int>* list_of_nets) {
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
void extNetStats::reset() {
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
void extNetStats::update_double_limits(int n, double v1, double v2, double* val,
                                       double units) {
  if (n == 0) {
    val[0] = v1 * units;
    val[1] = v2 * units;
  } else if (n == 1)
    val[0] = v1 * units;
  else if (n == 2)
    val[1] = v2 * units;
}
void extNetStats::update_double(Ath__parser* parser, const char* word,
                                double* val, double units) {
  double v1, v2;
  int n = parser->get2Double(word, ":", v1, v2);

  if (n >= 0)
    update_double_limits(n, v1, v2, val, units);
}
void extNetStats::update_int_limits(int n, int v1, int v2, int* val,
                                    uint units) {
  if (n == 0) {
    val[0] = v1 * units;
    val[1] = v2 * units;
  } else if (n == 1)
    val[0] = v1 * units;
  else if (n == 2)
    val[1] = v2 * units;
}
void extNetStats::update_int(Ath__parser* parser, const char* word, int* val,
                             uint units) {
  int v1, v2;
  int n = parser->get2Int(word, ":", v1, v2);

  if (n >= 0)
    update_int_limits(n, v1, v2, val, units);
}
void extNetStats::update_bbox(Ath__parser* parser, const char* bbox) {
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
