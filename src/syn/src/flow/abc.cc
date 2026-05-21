// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// ABC roundtrip: export the combinational AIG (And/Andnot/Or/Not)
// to ABC's Gia_Man_t, run ABC commands, and reimport.

#include <cassert>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "syn/ir/ControlNet.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "syn/synthesis.h"
#include "utl/Logger.h"

// ABC headers are already compiled within namespace abc by the build system.
// abc_global.h defines macros (ABC_NAMESPACE_HEADER_*) used by abcapis.h, so
// it must precede abcapis.h. main.h must precede cmd.h. abcapis.h provides
// abc::Abc_Frame_t. Do not let clang-format reorder these.
// clang-format off
#include "misc/util/abc_global.h"
#include "base/main/abcapis.h"  // NOLINT(misc-include-cleaner) — provides Abc_Frame_t
#include "aig/gia/gia.h"
// clang-format on

namespace abc {
// Explicitly declare the following functions rather than include abc's
// main.h; this is to work around the pedantic build failing on duplicate
// declarations between main.h and other abc headers
extern void Abc_FrameUpdateGia(Abc_Frame_t* pAbc, Gia_Man_t* pNew);
extern Gia_Man_t* Abc_FrameGetGia(Abc_Frame_t* pAbc);
}  // namespace abc

namespace syn {

using Literal = ControlNet;

static bool isAig(const Instance* inst)
{
  return inst->is<And>() || inst->is<Andnot>() || inst->is<Or>()
         || inst->is<Not>() || inst->is<Buffer>();
}

// AND helper matching bitblast.cc: construct And/Andnot/Or
// based on input polarities, returning a Literal.
static Literal AND(Graph& g, Literal a, Literal b)
{
  if (a.isAlways(false) || b.isAlways(false)) {
    return Literal::zero();
  }
  if (a.isAlways(true)) {
    return b;
  }
  if (b.isAlways(true)) {
    return a;
  }

  if (a.isPositive() && b.isPositive()) {
    return Literal::pos(g.add<And>(a.net(), b.net()).asNet());
  }
  if (a.isPositive() && b.isNegative()) {
    return Literal::pos(g.add<Andnot>(a.net(), b.net()).asNet());
  }
  if (a.isNegative() && b.isPositive()) {
    return Literal::pos(g.add<Andnot>(b.net(), a.net()).asNet());
  }
  return Literal::neg(g.add<Or>(a.net(), b.net()).asNet());
}

void abcRoundtrip(Graph& g, const std::string& commands, utl::Logger* logger)
{
  g.normalize();

  // === Phase 1: Export graph to GIA ===

  std::vector<int> net2lit(g.tableSize(), -1);
  std::vector<Net> ci2net;  // CI index → original Net
  std::vector<Net> co2net;  // CO index → PO net to replace

  // Pre-set constants
  net2lit[Graph::netId(Net::zero())] = 0;   // const0 literal
  net2lit[Graph::netId(Net::one())] = 1;    // const1 literal
  net2lit[Graph::netId(Net::undef())] = 0;  // treat undef as const0

  int nCi = 0, nAnd = 0;
  // Count to size the GIA
  g.forEachNet([&](Net net, const Instance* inst, uint32_t) {
    if (inst->is<TieLow>() || inst->is<TieHigh>() || inst->is<TieX>()) {
      return;
    }
    if (inst->outputWidth() == 0) {
      return;
    }
    if (isAig(inst)) {
      nAnd++;
    } else {
      nCi++;
    }
  });

  abc::Gia_Man_t* pGia = abc::Gia_ManStart(1 + nCi + nAnd + nCi);
  char name[] = "syn";  // Abc_UtilStrsav mistakenly uses char* not const char*
  pGia->pName = abc::Abc_UtilStrsav(name);
  abc::Gia_ManHashAlloc(pGia);

  // Build GIA in topological order
  g.forEachNet([&](Net net, const Instance* inst, uint32_t offset) {
    uint32_t id = Graph::netId(net);

    if (inst->is<TieLow>() || inst->is<TieHigh>() || inst->is<TieX>()) {
      return;  // constants already handled
    }
    if (inst->outputWidth() == 0) {
      return;  // sink instances (Output, Name) have no output nets
    }

    if (auto* op = inst->try_as<And>()) {
      int litA = net2lit[Graph::netId(op->a()[offset])];
      int litB = net2lit[Graph::netId(op->b()[offset])];
      assert(litA >= 0 && litB >= 0);
      net2lit[id] = abc::Gia_ManHashAnd(pGia, litA, litB);
    } else if (auto* op = inst->try_as<Andnot>()) {
      int litA = net2lit[Graph::netId(op->a()[offset])];
      int litB = net2lit[Graph::netId(op->b()[offset])];
      assert(litA >= 0 && litB >= 0);
      net2lit[id] = abc::Gia_ManHashAnd(pGia, litA, litB ^ 1);
    } else if (auto* op = inst->try_as<Or>()) {
      int litA = net2lit[Graph::netId(op->a()[offset])];
      int litB = net2lit[Graph::netId(op->b()[offset])];
      assert(litA >= 0 && litB >= 0);
      // Or(a,b) = ~(~a & ~b)
      net2lit[id] = abc::Gia_ManHashAnd(pGia, litA ^ 1, litB ^ 1) ^ 1;
    } else if (auto* op = inst->try_as<Not>()) {
      int litA = net2lit[Graph::netId(op->a()[offset])];
      assert(litA >= 0);
      net2lit[id] = litA ^ 1;
    } else if (auto* op = inst->try_as<Buffer>()) {
      int litA = net2lit[Graph::netId(op->a()[offset])];
      assert(litA >= 0);
      net2lit[id] = litA;
    } else {
      // PI boundary: create a CI
      int lit = abc::Gia_ManAppendCi(pGia);
      net2lit[id] = lit;
      ci2net.push_back(net);
    }
  });

  // Collect POs: nets consumed by non-AIG instances
  // Use a set to avoid duplicates
  std::set<uint32_t> poSet;
  g.forEachInstance([&](const Instance* inst) {
    if (isAig(inst)) {
      return;
    }
    if (inst->is<TieLow>() || inst->is<TieHigh>() || inst->is<TieX>()
        || (inst->is<Name>() && inst->as<Name>()->tentative())
        || inst->is<Not>()) {
      return;
    }
    inst->visit([&](Net fanin) {
      if (fanin.isConst()) {
        return;
      }
      uint32_t fid = Graph::netId(fanin);
      if (net2lit[fid] < 0) {
        return;  // not in the AIG
      }
      // Only add if this net is produced by an AIG gate, or is
      // the complement of such
      auto [producer, off] = g.resolve(fanin);
      if (!isAig(producer)) {
        return;
      }
      if (poSet.insert(fid).second) {
        abc::Gia_ManAppendCo(pGia, net2lit[fid]);
        co2net.push_back(fanin);
      }
    });
  });

  pGia = abc::Gia_ManCleanup(pGia);

  int nAnds = abc::Gia_ManAndNum(pGia);
  int nCis2 = abc::Gia_ManCiNum(pGia);
  int nCos = abc::Gia_ManCoNum(pGia);
  logger->info(utl::SYN,
               20,
               "abc_roundtrip: exported {} ANDs, {} CIs, {} COs",
               nAnds,
               nCis2,
               nCos);

  // === Phase 2: Run ABC commands ===

  abc::Abc_Frame_t* pFrame = abc::Abc_FrameGetGlobalFrame();
  abc::Abc_FrameUpdateGia(pFrame, pGia);

  if (abc::Cmd_CommandExecute(pFrame, commands.c_str())) {
    logger->warn(utl::SYN, 21, "abc_roundtrip: ABC command returned error");
  }

  pGia = abc::Abc_FrameGetGia(pFrame);
  if (!pGia) {
    logger->error(utl::SYN, 22, "abc_roundtrip: no GIA after ABC commands");
    return;
  }

  nAnds = abc::Gia_ManAndNum(pGia);
  logger->info(utl::SYN,
               23,
               "abc_roundtrip: after ABC: {} ANDs, {} CIs, {} COs",
               nAnds,
               abc::Gia_ManCiNum(pGia),
               abc::Gia_ManCoNum(pGia));

  // === Phase 3: Reimport GIA to graph ===

  assert(abc::Gia_ManCiNum(pGia) == (int) ci2net.size());
  assert(abc::Gia_ManCoNum(pGia) == (int) co2net.size());

  // Map GIA object IDs to Literals
  std::vector<Literal> obj2lit(pGia->nObjs, Literal::zero());

  // CIs → original nets
  {
    abc::Gia_Obj_t* pObj;
    int i;
    Gia_ManForEachCi(pGia, pObj, i)
    {
      int objId = abc::Gia_ObjId(pGia, pObj);
      obj2lit[objId] = Literal::pos(ci2net[i]);
    }
  }

  // AND gates → construct And/Andnot/Or using the AND helper
  {
    abc::Gia_Obj_t* pObj;
    int i;
    Gia_ManForEachAnd(pGia, pObj, i)
    {
      int fanin0Var = abc::Gia_ObjFaninId0p(pGia, pObj);
      bool fanin0Compl = abc::Gia_ObjFaninC0(pObj);
      int fanin1Var = abc::Gia_ObjFaninId1p(pGia, pObj);
      bool fanin1Compl = abc::Gia_ObjFaninC1(pObj);

      Literal a = fanin0Compl ? !obj2lit[fanin0Var] : obj2lit[fanin0Var];
      Literal b = fanin1Compl ? !obj2lit[fanin1Var] : obj2lit[fanin1Var];
      obj2lit[i] = AND(g, a, b);
    }
  }

  // COs → replace original nets
  {
    abc::Gia_Obj_t* pObj;
    int i;
    Gia_ManForEachCo(pGia, pObj, i)
    {
      int faninVar = abc::Gia_ObjFaninId0p(pGia, pObj);
      bool faninCompl = abc::Gia_ObjFaninC0(pObj);

      Literal driver = faninCompl ? !obj2lit[faninVar] : obj2lit[faninVar];
      Net result = driver.emitNet(g);
      g.forceReplace(BundleView(co2net[i]), BundleView(result));
    }
  }

  abc::Gia_ManStop(pGia);
}

}  // namespace syn
