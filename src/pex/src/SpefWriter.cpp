// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/SpefWriter.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"

namespace pex {

namespace {

std::string number(const double value)
{
  char text[32];
  std::snprintf(text, sizeof(text), "%.7g", value);
  return text;
}

std::string now()
{
  const std::time_t t = std::time(nullptr);
  std::tm local{};
  localtime_r(&t, &local);
  char text[64];
  std::strftime(text, sizeof(text), "%H:%M:%S %A %B %d, %Y", &local);
  return text;
}

// A name with SPEF's special characters escaped: any but letters, digits,
// '_', the hierarchy divider and bus brackets, unless escaped already (ODB
// names keep the escapes of the Verilog they came from).
std::string escaped(const std::string& name, const char divider)
{
  std::string result;
  for (size_t i = 0; i < name.size(); i++) {
    const char c = name[i];
    const bool plain = std::isalnum(static_cast<unsigned char>(c)) != 0
                       || c == '_' || c == '\\' || c == divider || c == '['
                       || c == ']';
    if (!plain && (i == 0 || name[i - 1] != '\\')) {
      result += '\\';
    }
    result += c;
  }
  return result;
}

char direction(const odb::dbIoType type)
{
  if (type == odb::dbIoType::INPUT) {
    return 'I';
  }
  if (type == odb::dbIoType::OUTPUT) {
    return 'O';
  }
  return 'B';
}

class Writer
{
 public:
  Writer(odb::dbBlock* block, std::ostream& out, const SpefOptions& options)
      : block_(block),
        out_(out),
        options_(options),
        divider_(block->getHierarchyDelimiter() == 0
                     ? '/'
                     : block->getHierarchyDelimiter())
  {
  }

  void write()
  {
    if (options_.corner < 0 || options_.corner >= block_->getCornerCount()) {
      throw std::invalid_argument("no parasitic corner "
                                  + std::to_string(options_.corner));
    }
    std::vector<odb::dbNet*> nets;
    std::unordered_set<odb::dbNet*> chosen;
    for (odb::dbNet* net : options_.nets) {
      if (!net->isRCgraph()) {
        throw std::invalid_argument("net " + net->getName()
                                    + " has no RC network");
      }
      if (chosen.insert(net).second) {
        nets.push_back(net);
      }
    }
    // Every net with parasitics is named, the far ends of couplings
    // included.
    for (odb::dbNet* net : block_->getNets()) {
      if (net->isRCgraph() || !net->getCapNodes().empty()) {
        if (options_.nets.empty() && net->isRCgraph()) {
          nets.push_back(net);
        }
        net_ids_[net] = next_id_++;
      }
    }
    for (odb::dbInst* inst : block_->getInsts()) {
      inst_ids_[inst] = next_id_++;
    }
    header();
    nameMap();
    ports();
    for (odb::dbNet* net : nets) {
      dnet(net);
    }
  }

 private:
  void header()
  {
    const std::string date = options_.date.empty() ? now() : options_.date;
    out_ << "*SPEF \"ieee 1481-1999\"\n"
         << "*DESIGN \"" << block_->getName() << "\"\n"
         << "*DATE \"" << date << "\"\n"
         << "*VENDOR \"The OpenROAD Project\"\n"
         << "*PROGRAM \"OpenROAD\"\n"
         << "*VERSION \"" << options_.version << "\"\n"
         << "*DESIGN_FLOW \"NAME_SCOPE LOCAL\" \"PIN_CAP NONE\"\n"
         << "*DIVIDER " << divider_ << "\n"
         << "*DELIMITER :\n"
         << "*BUS_DELIMITER []\n"
         << "*T_UNIT 1 NS\n"
         << "*C_UNIT 1 FF\n"
         << "*R_UNIT 1 OHM\n"
         << "*L_UNIT 1 HENRY\n\n";
  }

  void nameMap()
  {
    out_ << "*NAME_MAP\n";
    for (odb::dbNet* net : block_->getNets()) {
      const auto it = net_ids_.find(net);
      if (it != net_ids_.end()) {
        out_ << '*' << it->second << ' ' << name(net->getName()) << '\n';
      }
    }
    for (odb::dbInst* inst : block_->getInsts()) {
      out_ << '*' << inst_ids_.at(inst) << ' ' << name(inst->getName()) << '\n';
    }
    out_ << '\n';
  }

  void ports()
  {
    out_ << "*PORTS\n";
    for (odb::dbBTerm* bterm : block_->getBTerms()) {
      out_ << name(bterm->getName()) << ' ' << direction(bterm->getIoType())
           << '\n';
    }
    out_ << '\n';
  }

  std::string name(const std::string& text) const
  {
    return escaped(text, divider_);
  }

  std::string nodeName(odb::dbCapNode* node) const
  {
    if (node->isITerm()) {
      odb::dbITerm* iterm = node->getITerm(block_);
      return "*" + std::to_string(inst_ids_.at(iterm->getInst())) + ":"
             + name(iterm->getMTerm()->getName());
    }
    if (node->isBTerm()) {
      return name(node->getBTerm(block_)->getName());
    }
    return "*" + std::to_string(net_ids_.at(node->getNet())) + ":"
           + std::to_string(node->getNode());
  }

  void dnet(odb::dbNet* net)
  {
    const int corner = options_.corner;
    double total = 0;
    for (odb::dbCapNode* node : net->getCapNodes()) {
      total += node->getCapacitance(corner);
      for (odb::dbCCSeg* cc : node->getCCSegs()) {
        total += cc->getCapacitance(corner);
      }
    }
    out_ << "*D_NET *" << net_ids_.at(net) << ' ' << number(total) << '\n';
    // The section is left out when no terminal is on the net's wire: SPEF
    // allows no section but not an empty one.
    bool conn = false;
    for (odb::dbCapNode* node : net->getCapNodes()) {
      if (!node->isITerm() && !node->isBTerm()) {
        continue;
      }
      if (!conn) {
        out_ << "*CONN\n";
        conn = true;
      }
      if (node->isITerm()) {
        odb::dbITerm* iterm = node->getITerm(block_);
        out_ << "*I " << nodeName(node) << ' ' << direction(iterm->getIoType())
             << " *D " << name(iterm->getInst()->getMaster()->getName())
             << '\n';
      } else {
        out_ << "*P " << nodeName(node) << ' '
             << direction(node->getBTerm(block_)->getIoType()) << '\n';
      }
    }
    out_ << "*CAP\n";
    int k = 1;
    for (odb::dbCapNode* node : net->getCapNodes()) {
      out_ << k++ << ' ' << nodeName(node) << ' '
           << number(node->getCapacitance(corner)) << '\n';
    }
    for (odb::dbCapNode* node : net->getCapNodes()) {
      for (odb::dbCCSeg* cc : node->getCCSegs()) {
        uint32_t unused;
        odb::dbCapNode* other = cc->getTheOtherCapn(node, unused);
        out_ << k++ << ' ' << nodeName(node) << ' ' << nodeName(other) << ' '
             << number(cc->getCapacitance(corner)) << '\n';
      }
    }
    // Like *CONN, left out when empty: a net on one node has no resistors.
    k = 1;
    for (odb::dbRSeg* seg : net->getRSegs()) {
      if (k == 1) {
        out_ << "*RES\n";
      }
      out_ << k++ << ' ' << nodeName(seg->getSourceCapNode()) << ' '
           << nodeName(seg->getTargetCapNode()) << ' '
           << number(seg->getResistance(corner)) << '\n';
    }
    out_ << "*END\n\n";
  }

  odb::dbBlock* block_;
  std::ostream& out_;
  const SpefOptions& options_;
  const char divider_;
  std::unordered_map<odb::dbNet*, int> net_ids_;
  std::unordered_map<odb::dbInst*, int> inst_ids_;
  int next_id_ = 1;
};

}  // namespace

void writeSpef(odb::dbBlock* block,
               std::ostream& out,
               const SpefOptions& options)
{
  Writer(block, out, options).write();
}

}  // namespace pex
