// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cstdint>
#include <cstdio>
#include <list>
#include <sstream>
#include <string>

#include "odb/wOrder.h"
#include "rcx/ext.h"
#include "rcx/extRCap.h"
#include "rcx/extSolverGen.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

// ----------------------------- dkf 092024 --------------------------------
bool Ext::init_rcx_model(const char* corner_names, int metal_cnt)
{
  std::istringstream iss(corner_names);
  std::string word;
  // Extract each word from the string stream and insert it into the list
  std::list<std::string> corners;
  while (iss >> word) {
    corners.push_back(word);
  }
  extRCModel* m = new extRCModel("MINTYPMAX", logger_);
  uint32_t cornerCnt = m->initModel(corners, metal_cnt);
  fprintf(stdout, "Initialed %d corners <%s>\n", cornerCnt, corner_names);
  _ext->setCurrentModel(m);
  return true;
}
bool Ext::read_rcx_tables(const char* corner,
                          const char* filename,
                          int wire,
                          bool over,
                          bool under,
                          bool over_under,
                          bool diag)
{
  extRCModel* m = _ext->getCurrentModel();
  if (!m) {
    logger_->error(RCX, 513, "No model present.");
  }
  m->readRCvalues(corner, filename, wire, over, under, over_under, diag);
  return true;
}
bool Ext::write_rcx_model(const char* filename)
{
  extRCModel* m = _ext->getCurrentModel();
  if (!m) {
    logger_->error(RCX, 512, "No model to write.");
  }
  std::list<std::string> corner_list;
  m->getCorners(corner_list);

  m->GenExtModel(corner_list, filename, "FasterCap Integration", "v2.0", 0);
  return true;
}
// ----------------------------- dkf 092024 --------------------------------
bool Ext::gen_solver_patterns(const char* process_file,
                              const char* process_name,
                              int version,
                              int wire_cnt,
                              int len,
                              int over_dist,
                              int under_dist,
                              const char* w_list,
                              const char* s_list)
{
  extSolverGen* p = new extSolverGen(64, 128, logger_);

  p->readProcess(process_name, (char*) process_file);
  p->writeProcess("process.out");
  _ext->setCurrentSolverGen(p);

  p->genSolverPatterns(process_name,
                       version,
                       wire_cnt,
                       len,
                       over_dist,
                       under_dist,
                       w_list,
                       s_list);

  return true;
}

}  // namespace rcx
