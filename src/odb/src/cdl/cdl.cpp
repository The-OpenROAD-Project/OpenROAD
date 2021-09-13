/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <fstream>
#include <list>
#include <regex>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "odb/cdl.h"
#include "utl/Logger.h"

namespace odb {

void writeLine(FILE* f, const std::string& s)
{
  std::size_t bufferBegin = 0, currentPos = -1;
  while ((currentPos = s.find(' ', currentPos + 1)) != std::string::npos) {
    if (currentPos - bufferBegin < 57)
      continue;

    fwrite(s.substr(bufferBegin, currentPos - bufferBegin).c_str(),
           sizeof(char),
           currentPos - bufferBegin,
           f);
    bufferBegin = currentPos + 1;

    if (bufferBegin < s.size() - 1)
      fwrite("\n+ ", sizeof(char), 3, f);
  }

  if (bufferBegin < s.size() - 1)
    fwrite(
        s.substr(bufferBegin).c_str(), sizeof(char), s.size() - bufferBegin, f);
  fwrite("\n", sizeof(char), 1, f);
}

std::string getUnconnectedNet(dbBlock* block, int& unconnectedNets)
{
  while (block->findNet(
             std::string("_unconnected_" + std::to_string(unconnectedNets++))
                 .c_str())
         != NULL)
    ;
  return "_unconnected_" + std::to_string(unconnectedNets - 1);
}

std::string getNetName(dbBlock* block,
                       dbInst* inst,
                       dbMTerm* mterm,
                       int& unconnectedNets)
{
  if (mterm == nullptr) {
    return getUnconnectedNet(block, unconnectedNets);
  }

  dbITerm* iterm = inst->getITerm(mterm);
  if (iterm == nullptr) {
    return getUnconnectedNet(block, unconnectedNets);
  }

  dbNet* net = iterm->getNet();
  if (net == nullptr) {
    return getUnconnectedNet(block, unconnectedNets);
  }

  return net->getName();
}

// Look for .subckt lines and record the terminal order for module to
// be used when writing instances.
std::unordered_map<dbMaster*, std::vector<dbMTerm*>>
readMasters(utl::Logger* logger, dbBlock* block, const char* fileName)
{
  std::ifstream file(fileName);
  if (!file) {
    logger->error(utl::ODB, 283, "Can't open masters file {}.", fileName);
  }

  // Read the whole file and join any continued lines to simplify
  // later parsing.
  std::string contents((std::istreambuf_iterator<char>(file)),
                       (std::istreambuf_iterator<char>()));
  std::regex continued("\\n\\+");
  contents = regex_replace(contents, continued, "");

  std::unordered_map<dbMaster*, std::vector<dbMTerm*>> mtermMap;

  std::regex token("\\s*([^ ]+)\\s*");
  std::istringstream contents_stream(contents);
  std::string line;
  while (getline(contents_stream, line)) {
    if (strncasecmp(line.c_str(), ".subckt", 7) != 0) {
      continue;
    }

    line = line.substr(7);  // drop leading .subckt

    dbMaster* master = nullptr;
    std::vector<dbMTerm*>* mterms;
    std::sregex_iterator tokens_begin(line.begin(), line.end(), token);
    std::sregex_iterator tokens_end;
    for (auto i = tokens_begin; i != tokens_end; ++i) {
      std::string token = (*i)[1].str();
      if (!master) {  // The first token is the master name
        master = block->getDb()->findMaster(token.c_str());
        if (!master) {
          logger->warn(utl::ODB, 284, "Master {} not found.", token);
          break;
        }
        if (mtermMap.find(master) != mtermMap.end()) {
          logger->warn(utl::ODB, 285, "Master {} seen more than once in {}.",
                       token, fileName);
          break;
        }
        mterms = &mtermMap[master];
      } else {
        dbMTerm* mterm = master->findMTerm(token.c_str());
        if (!mterm) {
          logger->warn(utl::ODB,
                       286,
                       "Terminal {} of CDL master {} not found in LEF.",
                       token,
                       master->getName());
        }
        // push even if null, a dangling net must be assigned.
        mterms->push_back(mterm);
      }
    }
  }

  return mtermMap;
}

bool cdl::writeCdl(utl::Logger* logger,
                   dbBlock* block,
                   const char* outFileName,
                   const char* mastersFileName,
                   bool includeFillers)
{
  auto mtermMap = readMasters(logger, block, mastersFileName);
  int unconnectedNets = 0;
  FILE* f = fopen(outFileName, "w");

  if (f == NULL) {
    error(1, "cannot open file %s", outFileName);
    return false;
  }

  writeLine(f, "$ CDL Netlist generated by OpenROAD");
  writeLine(f, "");
  writeLine(f, "*.BUSDELIMITER [");
  writeLine(f, "");

  std::string line = ".SUBCKT " + block->getName();
  for (auto&& pin : block->getBTerms()) {
    line += " " + pin->getName();
  }

  writeLine(f, line);

  for (auto&& inst : block->getInsts()) {
    dbMaster* master = inst->getMaster();
    if (!includeFillers && master->isFiller())
      continue;

    line = "X" + inst->getName();
    auto it = mtermMap.find(master);
    if (it == mtermMap.end()) {
      logger->error(utl::ODB,
                    287,
                    "Master {} was not in the masters CDL file {}.",
                    master->getName(),
                    mastersFileName);
    }

    for (auto&& mterm : it->second) {
      line += " " + getNetName(block, inst, mterm, unconnectedNets);
    }

    line += " " + master->getName();
    writeLine(f, line);
  }

  writeLine(f, ".ENDS " + block->getName());

  fclose(f);
  return true;
}

}  // namespace odb
