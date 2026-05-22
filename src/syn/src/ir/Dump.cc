// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Dump: outputs graphs in text IR form

#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "syn/ir/NetTableEntry.h"

namespace syn {

void writeNet(std::ostream& os, const Graph& g, Net net)
{
  if (net == Net::zero()) {
    os << "0";
  } else if (net == Net::one()) {
    os << "1";
  } else if (net == Net::undef()) {
    os << "X";
  } else if (net == Net::sentinel()) {
    os << "E";
  } else {
    auto [inst, offset] = g.resolve(net);
    NetTableId base = Graph::netId(net) - offset;
    if (offset > 0) {
      os << "%" << base << "+" << offset;
    } else {
      os << "%" << base;
    }
  }
}

static void writeControlNet(std::ostream& os, const Graph& g, ControlNet cn)
{
  if (cn.isNegative()) {
    os << "!";
  }
  writeNet(os, g, cn.net());
}

void writeValue(std::ostream& os, const Graph& g, BundleView bv)
{
  if (bv.width() == 0) {
    os << "[]";
    return;
  }

  if (bv.isConst()) {
    // Print MSB-first constant
    for (int i = (int) bv.width() - 1; i >= 0; i--) {
      Net n = bv[i];
      if (n == Net::zero()) {
        os << "0";
      } else if (n == Net::one()) {
        os << "1";
      } else {
        os << "X";
      }
    }
    return;
  }

  // Check if all bits are consecutive net IDs (from a single cell output)
  if (bv.width() > 0) {
    auto [inst0, off0] = g.resolve(bv[0]);
    NetTableId first_id = Graph::netId(bv[0]);
    bool consecutive = true;
    if (off0 + bv.width() > inst0->outputWidth()) {
      consecutive = false;
    }
    for (uint32_t i = 1; i < bv.width() && consecutive; i++) {
      if (Graph::netId(bv[i]) != first_id + i) {
        consecutive = false;
      }
    }
    if (consecutive) {
      NetTableId base = first_id - off0;
      if (bv.width() == 1 && off0 == 0) {
        os << "%" << base;
      } else if (off0 == 0) {
        os << "%" << base << ":" << bv.width();
      } else {
        os << "%" << base << "+" << off0 << ":" << bv.width();
      }
      return;
    }
  }

  // General case: concatenation — group consecutive runs from the same cell
  os << "[ ";
  int i = (int) bv.width() - 1;
  bool first = true;
  while (i >= 0) {
    if (!first) {
      os << " ";
    }
    first = false;

    Net n = bv[i];
    // Check if this is a constant
    if (n.isConst()) {
      // Collect consecutive constant bits
      int start = i;
      while (i >= 0) {
        Net nc = bv[i];
        if (!nc.isConst()) {
          break;
        }
        i--;
      }
      // Print MSB-first
      for (int j = start; j > i; j--) {
        Net nc = bv[j];
        if (nc == Net::zero()) {
          os << "0";
        } else if (nc == Net::one()) {
          os << "1";
        } else {
          os << "X";
        }
      }
    } else {
      // Non-constant: find consecutive run from same cell
      auto [inst0, off0] = g.resolve(n);
      int start = i;
      i--;
      while (i >= 0) {
        Net nc = bv[i];
        auto [inst_i, off_i] = g.resolve(nc);
        if (inst_i != inst0 || off_i != off0 - (start - i)) {
          break;
        }
        i--;
      }
      uint32_t run_len = start - i;
      uint32_t run_offset = off0 - (run_len - 1);
      NetTableId base = Graph::netId(bv[start]) - off0;
      if (run_len > 1) {
        os << "%" << base << "+" << run_offset << ":" << run_len;
      } else if (off0 > 0) {
        os << "%" << base << "+" << off0;
      } else {
        os << "%" << base;
      }
    }
  }
  os << " ]";
}

const char* cellKeyword(EntryType type)
{
  switch (type) {
    case EntryType::kBufferFine:
    case EntryType::kBufferWide:
      return "buf";
    case EntryType::kNotFine:
    case EntryType::kNotWide:
      return "not";
    case EntryType::kAndFine:
    case EntryType::kAndWide:
      return "and";
    case EntryType::kOrFine:
    case EntryType::kOrWide:
      return "or";
    case EntryType::kAndnotFine:
    case EntryType::kAndnotWide:
      return "andnot";
    case EntryType::kXorFine:
    case EntryType::kXorWide:
      return "xor";
    case EntryType::kMuxFine:
    case EntryType::kMuxWide:
      return "mux";
    case EntryType::kAdcFine:
    case EntryType::kAdcWide:
      return "adc";
    case EntryType::kEq:
      return "eq";
    case EntryType::kULt:
      return "ult";
    case EntryType::kSLt:
      return "slt";
    case EntryType::kShl:
      return "shl";
    case EntryType::kUShr:
      return "ushr";
    case EntryType::kSShr:
      return "sshr";
    case EntryType::kXShr:
      return "xshr";
    case EntryType::kMul:
      return "mul";
    case EntryType::kUDiv:
      return "udiv";
    case EntryType::kUMod:
      return "umod";
    case EntryType::kSDivTrunc:
      return "sdiv_trunc";
    case EntryType::kSDivFloor:
      return "sdiv_floor";
    case EntryType::kSModTrunc:
      return "smod_trunc";
    case EntryType::kSModFloor:
      return "smod_floor";
    case EntryType::kDff:
      return "dff";
    case EntryType::kLoopBreaker:
      return "loop_breaker";
    case EntryType::kInput:
      return "input";
    case EntryType::kDangling:
      return "dangling";
    case EntryType::kOutput:
      return "output";
    case EntryType::kName:
      return "name";
    case EntryType::kTarget:
      return "target";
    case EntryType::kOther:
      return "other";
    default:
      return "?";
  }
}

static void writeOperands(std::ostream& os,
                          const Graph& g,
                          const Instance* inst,
                          EntryType type,
                          NetTableId id = 0)
{
  switch (type) {
    case EntryType::kBufferFine:
    case EntryType::kBufferWide: {
      auto* buf = static_cast<const Buffer*>(inst);
      os << " ";
      writeValue(os, g, buf->a());
      break;
    }
    case EntryType::kNotFine:
    case EntryType::kNotWide: {
      auto* op = static_cast<const Not*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      break;
    }
    case EntryType::kAndFine:
    case EntryType::kAndWide: {
      auto* op = static_cast<const And*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kOrFine:
    case EntryType::kOrWide: {
      auto* op = static_cast<const Or*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kAndnotFine:
    case EntryType::kAndnotWide: {
      auto* op = static_cast<const Andnot*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kXorFine:
    case EntryType::kXorWide: {
      auto* op = static_cast<const Xor*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kMuxFine:
    case EntryType::kMuxWide: {
      auto* mux = static_cast<const Mux*>(inst);
      os << " ";
      writeNet(os, g, mux->sel());
      os << " ";
      writeValue(os, g, mux->a());
      os << " ";
      writeValue(os, g, mux->b());
      break;
    }
    case EntryType::kAdcFine:
    case EntryType::kAdcWide: {
      auto* adc = static_cast<const Adc*>(inst);
      os << " ";
      writeValue(os, g, adc->a());
      os << " ";
      writeValue(os, g, adc->b());
      os << " ";
      writeNet(os, g, adc->cin());
      break;
    }
    case EntryType::kEq: {
      auto* op = static_cast<const Eq*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kULt: {
      auto* op = static_cast<const ULt*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kSLt: {
      auto* op = static_cast<const SLt*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kShl: {
      auto* op = static_cast<const Shl*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      os << " #" << op->stride();
      break;
    }
    case EntryType::kUShr: {
      auto* op = static_cast<const UShr*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      os << " #" << op->stride();
      break;
    }
    case EntryType::kSShr: {
      auto* op = static_cast<const SShr*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      os << " #" << op->stride();
      break;
    }
    case EntryType::kXShr: {
      auto* op = static_cast<const XShr*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      os << " #" << op->stride();
      break;
    }
    case EntryType::kMul: {
      auto* op = static_cast<const Mul*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kUDiv: {
      auto* op = static_cast<const UDiv*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kUMod: {
      auto* op = static_cast<const UMod*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kSDivTrunc: {
      auto* op = static_cast<const SDivTrunc*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kSDivFloor: {
      auto* op = static_cast<const SDivFloor*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kSModTrunc: {
      auto* op = static_cast<const SModTrunc*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kSModFloor: {
      auto* op = static_cast<const SModFloor*>(inst);
      os << " ";
      writeValue(os, g, op->a());
      os << " ";
      writeValue(os, g, op->b());
      break;
    }
    case EntryType::kDff: {
      auto* dff = static_cast<const Dff*>(inst);
      os << " ";
      writeValue(os, g, BundleView(dff->data()));
      os << " clk=";
      writeControlNet(os, g, dff->clock());
      if (!dff->clear().isAlways(false)) {
        os << " clr=";
        writeControlNet(os, g, dff->clear());
      }
      if (!dff->reset().isAlways(false)) {
        os << " rst=";
        writeControlNet(os, g, dff->reset());
      }
      if (!dff->enable().isAlways(true)) {
        os << " en=";
        writeControlNet(os, g, dff->enable());
      }
      break;
    }
    case EntryType::kLoopBreaker: {
      auto* lb = static_cast<const LoopBreaker*>(inst);
      os << " ";
      writeValue(os, g, BundleView(lb->a()));
      break;
    }
    case EntryType::kInput: {
      auto* inp = static_cast<const Input*>(inst);
      os << " \"" << inp->name() << "\"";
      break;
    }
    case EntryType::kDangling: {
      break;
    }
    case EntryType::kOutput: {
      auto* out = static_cast<const Output*>(inst);
      os << " \"" << out->name() << "\" ";
      writeValue(os, g, BundleView(out->value()));
      break;
    }
    case EntryType::kName: {
      auto* nm = static_cast<const Name*>(inst);
      os << " \"" << nm->nameStr() << "\" " << nm->from() << " " << nm->to();
      if (nm->tentative()) {
        os << " tentative";
      }
      if (nm->isVector()) {
        os << " vector";
      }
      os << " ";
      writeValue(os, g, BundleView(nm->value()));
      break;
    }
    case EntryType::kTarget: {
      auto* tgt = static_cast<const Target*>(inst);
      sta::LibertyCell* cell = tgt->cell();
      os << " \"" << cell->name() << "\" {\n";
      uint32_t in_offset = 0;
      uint32_t out_offset = 0;
      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        sta::LibertyPort* port = port_iter.next();
        if (port->isPwrGnd()) {
          continue;
        }
        uint32_t pw = port->size();
        if (port->direction()->isInput()) {
          os << "  input \"" << port->name() << "\" = ";
          writeValue(os, g, BundleView(tgt->inputs(), in_offset, pw));
          os << "\n";
          in_offset += pw;
        } else if (port->direction()->isOutput()) {
          os << "  %" << (id + out_offset) << ":" << pw << " = output \""
             << port->name() << "\"\n";
          out_offset += pw;
        }
      }
      os << "}";
      break;
    }
    case EntryType::kOther: {
      auto* other = static_cast<const Other*>(inst);
      os << " \"" << other->cellType() << "\" {\n";
      uint32_t out_offset = 0;
      for (auto& port : other->ports()) {
        switch (port.direction) {
          case Other::Port::kInput:
            os << "  input \"" << port.name << "\" = ";
            writeValue(os, g, BundleView(port.value));
            os << "\n";
            break;
          case Other::Port::kOutput:
            os << "  %" << (id + out_offset) << ":" << port.width
               << " = output \"" << port.name << "\"\n";
            out_offset += port.width;
            break;
          case Other::Port::kInOut:
            os << "  %" << (id + out_offset) << ":" << port.width << " = io \""
               << port.name << "\" = ";
            writeValue(os, g, BundleView(port.value));
            os << "\n";
            out_offset += port.width;
            break;
        }
      }
      os << "}";
      break;
    }
    default:
      break;
  }
}

void Graph::dumpInstance(std::ostream& os, const Instance* inst) const
{
  NetTableId id = baseId(inst);
  EntryType type = inst->entryType();
  uint32_t w = inst->outputWidth();
  os << "%" << id << ":" << w;
  os << " = " << cellKeyword(type);
  writeOperands(os, *this, inst, type, id);
}

void Graph::dump(std::ostream& os) const
{
  forEachInstance([&](const Instance* inst) {
    if (inst->is<TieLow>() || inst->is<TieHigh>() || inst->is<TieX>()) {
      return;
    }
    dumpInstance(os, inst);
    os << "\n";
  });
}

void Graph::dumpFaninCone(std::ostream& os,
                          Net root,
                          int max_depth,
                          bool stop_at_stateful) const
{
  if (root.isConst()) {
    os << "%" << netId(root) << " is constant\n";
    return;
  }

  // BFS backward from root.
  struct Entry
  {
    Net net;
    int depth;
  };
  std::vector<Entry> queue;
  std::unordered_set<uint32_t> visited;
  std::set<const Instance*> printedInstances;

  queue.push_back({root, 0});
  visited.insert(netId(root));

  for (size_t qi = 0; qi < queue.size(); qi++) {
    auto [net, depth] = queue[qi];
    if (net.isConst()) {
      continue;
    }

    auto [inst, offset] = resolve(net);

    // Print with indentation.
    if (!printedInstances.contains(inst)) {
      for (int d = 0; d < depth; d++) {
        os << "  ";
      }
      dumpInstance(os, inst);
      os << "\n";
      printedInstances.insert(inst);
    }

    // Stop expanding if at depth limit.
    if (depth >= max_depth) {
      continue;
    }

    // Stop at stateful boundaries if requested.
    if (stop_at_stateful && inst->hasState()) {
      continue;
    }

    // Enqueue all fanin nets.
    inst->visitSlice(offset, [&](Net fanin) {
      if (fanin.isConst()) {
        return;
      }
      uint32_t fid = netId(fanin);
      if (visited.contains(fid)) {
        return;
      }
      visited.insert(fid);
      queue.push_back({fanin, depth + 1});
    });
  }
}

void Graph::dumpFanoutCone(std::ostream& os,
                           Net root,
                           int max_depth,
                           bool stop_at_stateful) const
{
  if (root.isConst()) {
    os << "%" << netId(root) << " is constant\n";
    return;
  }

  // Build consumer index: for each net, the list of instances that use it.
  std::unordered_map<uint32_t, std::vector<const Instance*>> consumers;
  forEachInstance([&](const Instance* inst) {
    inst->visit([&](Net fanin) {
      if (!fanin.isConst()) {
        consumers[netId(fanin)].push_back(inst);
      }
    });
  });

  // BFS forward from root.
  struct Entry
  {
    Net net;
    int depth;
  };
  std::vector<Entry> queue;
  std::unordered_set<uint32_t> visited;
  std::set<const Instance*> printedInstances;

  queue.push_back({root, 0});
  visited.insert(netId(root));

  for (size_t qi = 0; qi < queue.size(); qi++) {
    auto [net, depth] = queue[qi];

    auto it = consumers.find(netId(net));
    if (it == consumers.end()) {
      continue;
    }

    for (const Instance* inst : it->second) {
      // Print with indentation.
      if (!printedInstances.contains(inst)) {
        for (int d = 0; d < depth; d++) {
          os << "  ";
        }
        dumpInstance(os, inst);
        os << "\n";
        printedInstances.insert(inst);
      }

      // Stop expanding if at depth limit.
      if (depth >= max_depth) {
        continue;
      }

      // Stop at stateful boundaries if requested.
      if (stop_at_stateful && inst->hasState()) {
        continue;
      }

      // Enqueue all output nets of this instance.
      BundleView out = output(inst);
      for (uint32_t i = 0; i < out.width(); i++) {
        Net onet = out[i];
        if (onet.isConst()) {
          continue;
        }
        uint32_t oid = netId(onet);
        if (visited.contains(oid)) {
          continue;
        }
        visited.insert(oid);
        queue.push_back({onet, depth + 1});
      }
    }
  }
}

}  // namespace syn
