// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/cdl.h"

#include <strings.h>

#include <cstddef>
#include <ios>
#include <iterator>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "odb/db.h"
#include "utl/Logger.h"
#include "utl/ScopedTemporaryFile.h"

namespace odb {

void writeLine(std::ostream& f, const std::string& s)
{
  std::size_t bufferBegin = 0, currentPos = -1;
  while ((currentPos = s.find(' ', currentPos + 1)) != std::string::npos) {
    if (currentPos - bufferBegin < 57) {
      continue;
    }

    f << s.substr(bufferBegin, currentPos - bufferBegin);
    bufferBegin = currentPos + 1;

    if (bufferBegin < s.size() - 1) {
      f << "\n+ ";
    }
  }

  if (bufferBegin < s.size() - 1) {
    f << s.substr(bufferBegin);
  }

  f << "\n";
}

std::string getUnconnectedNet(dbBlock* block, int& unconnectedNets)
{
  while (block->findNet(
             std::string("_unconnected_" + std::to_string(unconnectedNets++))
                 .c_str())
         != nullptr) {
    ;
  }
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

  dbBTerm* bterm = net->get1stBTerm();
  if (bterm) {
    return bterm->getName();
  }

  return net->getName();
}

// Look for .subckt lines and record the terminal order for module to
// be used when writing instances.
std::unordered_map<dbMaster*, std::vector<dbMTerm*>>
readMasters(utl::Logger* logger, dbBlock* block, const char* fileName)
{
  std::unordered_map<dbMaster*, std::vector<dbMTerm*>> mtermMap;

  try {
    utl::InStreamHandler file(fileName);

    // Read the whole file and join any continued lines to simplify
    // later parsing.
    std::string contents((std::istreambuf_iterator<char>(file.getStream())),
                         (std::istreambuf_iterator<char>()));
    std::regex continued("\\n\\+");
    contents = regex_replace(contents, continued, "");

    std::regex token("\\s*([^ ]+)\\s*");
    std::istringstream contents_stream(contents);
    std::string line;
    while (getline(contents_stream, line)) {
      if (strncasecmp(line.c_str(), ".subckt", 7) != 0) {
        continue;
      }

      line = line.substr(7);  // drop leading .subckt

      dbMaster* master = nullptr;
      std::vector<dbMTerm*>* mterms = nullptr;
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
            logger->warn(utl::ODB,
                         285,
                         "Master {} seen more than once in {}.",
                         token,
                         fileName);
            break;
          }
          mterms = &mtermMap[master];
        } else {
          // Replace CDL <> (normal and escaped) to []
          token = std::regex_replace(token, std::regex(R"(\\?<)"), "[");
          token = std::regex_replace(token, std::regex(R"(\\?>)"), "]");
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
  } catch (const std::ios_base::failure& f) {
    logger->error(utl::ODB, 283, "Can't open masters file {}.", fileName);
  }

  return mtermMap;
}

bool cdl::writeCdl(utl::Logger* logger,
                   dbBlock* block,
                   const char* outFileName,
                   const std::vector<const char*>& mastersFileNames,
                   bool includeFillers)
{
  std::unordered_map<dbMaster*, std::vector<dbMTerm*>> mtermMap;
  for (const char* mastersFileName : mastersFileNames) {
    auto submtermMap = readMasters(logger, block, mastersFileName);
    mtermMap.insert(submtermMap.begin(), submtermMap.end());
  }
  int unconnectedNets = 0;
  utl::OutStreamHandler fileHandler(outFileName, false);
  std::ostream& f = fileHandler.getStream();

  writeLine(f, "* CDL Netlist generated by OpenROAD");
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
    if (!includeFillers && master->isFiller()) {
      continue;
    }

    line = "X" + inst->getName();
    auto it = mtermMap.find(master);
    if (it == mtermMap.end()) {
      if (master->getMTermCount() == 0) {
        logger->warn(utl::ODB,
                     357,
                     "Master {} was not in the masters CDL files, but master "
                     "has no pins.",
                     master->getName());
      } else {
        logger->error(utl::ODB,
                      287,
                      "Master {} was not in the masters CDL files.",
                      master->getName());
      }
    } else {
      for (auto&& mterm : it->second) {
        line += " " + getNetName(block, inst, mterm, unconnectedNets);
      }
    }

    line += " " + master->getName();
    writeLine(f, line);
  }

  writeLine(f, ".ENDS " + block->getName());

  return true;
}

}  // namespace odb
