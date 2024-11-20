
///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// Copyright (c) 2024, Dimitris Fotakis
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

#include "odb/wOrder.h"
#include "rcx/ext.h"
#include "rcx/extSolverGen.h"
#include "utl/Logger.h"

namespace rcx {

using utl::Logger;
using utl::RCX;

bool Ext::read_process(const std::string& name, const std::string& file)
{
  _ext->readProcess(name.c_str(), file.c_str());

  return true;
}

bool Ext::rules_gen(const std::string& name,
                    const std::string& dir,
                    const std::string& file,
                    bool write_to_solver,
                    bool read_from_solver,
                    bool run_solver,
                    int pattern,
                    bool keep_file,
                    int len,
                    int version,
                    bool win)
{
  _ext->rulesGen(name.c_str(),
                 dir.c_str(),
                 file.c_str(),
                 pattern,
                 write_to_solver,
                 read_from_solver,
                 run_solver,
                 keep_file,
                 len,
                 version,
                 win);

  return true;
}

bool Ext::metal_rules_gen(const std::string& name,
                          const std::string& dir,
                          const std::string& file,
                          bool write_to_solver,
                          bool read_from_solver,
                          bool run_solver,
                          int pattern,
                          bool keep_file,
                          int metal)
{
  _ext->metRulesGen(name.c_str(),
                    dir.c_str(),
                    file.c_str(),
                    pattern,
                    write_to_solver,
                    read_from_solver,
                    run_solver,
                    keep_file,
                    metal);
  return true;
}

bool Ext::run_solver(const std::string& dir, int net, int shape)
{
  extMainOptions opt;
  opt._topDir = dir.c_str();
  uint netId = net;
  int shapeId = shape;
  _ext->runSolver(&opt, netId, shapeId);
  return true;
}
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
  uint cornerCnt = m->initModel(corners, metal_cnt);
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
  m->readRCvalues(corner, filename, wire, over, under, over_under, diag);
  return true;
}
bool Ext::write_rcx_model(const char* filename)
{
  extRCModel* m = _ext->getCurrentModel();
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

/*
std::istringstream iss(corner_names);
std::string word;
// Extract each word from the string stream and insert it into the list
std::list<std::string> corners;
while (iss >> word) {
    corners.push_back(word);
}
extRCModel* m = new extRCModel("MINTYPMAX", NULL);
uint cornerCnt= m->initModel(corners, metal_cnt);
fprintf(stdout, "Initialed %d corners <%s>\n", cornerCnt, corner_names);
_ext->setCurrentModel(m);
*/

}  // namespace rcx
