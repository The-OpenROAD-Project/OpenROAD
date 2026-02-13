// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2026, The OpenROAD Authors

#include "gia.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <vector>

#include "aig/aig/aig.h"
#include "aig/gia/giaAig.h"
#include "base/abc/abc.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "map/if/if.h"
#pragma GCC diagnostic pop
#include "aig/gia/gia.h"
#include "cut/abc_library_factory.h"
#include "cut/logic_cut.h"
#include "cut/logic_extractor.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "map/mio/mio.h"
#include "map/scl/sclLib.h"
#include "map/scl/sclSize.h"
#include "misc/extra/extra.h"
#include "misc/nm/nm.h"
#include "misc/util/abc_global.h"
#include "misc/vec/vecPtr.h"
#include "odb/db.h"
#include "proof/dch/dch.h"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Search.hh"
#include "utils.h"
#include "utl/Logger.h"
#include "utl/SuppressStdout.h"
#include "utl/deleter.h"
#include "utl/unique_name.h"

namespace abc {
extern Abc_Ntk_t* Abc_NtkFromAigPhase(Aig_Man_t* pMan);
extern Abc_Ntk_t* Abc_NtkFromCellMappedGia(Gia_Man_t* p, int fUseBuffs);
extern Abc_Ntk_t* Abc_NtkFromDarChoices(Abc_Ntk_t* pNtkOld, Aig_Man_t* pMan);
extern Abc_Ntk_t* Abc_NtkFromMappedGia(Gia_Man_t* p,
                                       int fFindEnables,
                                       int fUseBuffs);
extern Abc_Ntk_t* Abc_NtkMap(Abc_Ntk_t* pNtk,
                             Mio_Library_t* userLib,
                             double DelayTarget,
                             double AreaMulti,
                             double DelayMulti,
                             float LogFan,
                             float Slew,
                             float Gain,
                             int nGatesMin,
                             int fRecovery,
                             int fSwitching,
                             int fSkipFanout,
                             int fUseProfile,
                             int fUseBuffs,
                             int fVerbose);
extern Aig_Man_t* Abc_NtkToDar(Abc_Ntk_t* pNtk, int fExors, int fRegisters);
extern Aig_Man_t* Abc_NtkToDarChoices(Abc_Ntk_t* pNtk);
extern Gia_Man_t* Gia_ManAigSynch2(Gia_Man_t* p,
                                   void* pPars,
                                   int nLutSize,
                                   int nRelaxRatio);
extern Gia_Man_t* Gia_ManCheckFalse(Gia_Man_t* p,
                                    int nSlackMax,
                                    int nTimeOut,
                                    int fVerbose,
                                    int fVeryVerbose);
extern Vec_Ptr_t* Abc_NtkCollectCiNames(Abc_Ntk_t* pNtk);
extern Vec_Ptr_t* Abc_NtkCollectCoNames(Abc_Ntk_t* pNtk);
extern void Abc_NtkRedirectCiCo(Abc_Ntk_t* pNtk);
extern void Abc_FrameSetLibGen(void* pLib);
}  // namespace abc

namespace rmp {
using utl::RMP;

static void replaceGia(GiaOp::AigManager& gia, abc::Gia_Man_t* new_gia)
{
  // Operation does not allocate new gia
  if (gia.get() == new_gia) {
    return;
  }
  if (gia->vNamesIn && !new_gia->vNamesIn) {
    std::swap(gia->vNamesIn, new_gia->vNamesIn);
  }
  if (gia->vNamesOut && !new_gia->vNamesOut) {
    std::swap(gia->vNamesOut, new_gia->vNamesOut);
  }
  if (gia->vNamesNode && !new_gia->vNamesNode) {
    std::swap(gia->vNamesNode, new_gia->vNamesNode);
  }
  gia.reset(new_gia);
}

std::vector<GiaOp> GiaOps(utl::Logger* logger)
{
  // GIA ops as lambdas
  // All the magic numbers are defaults from abc/src/base/abci/abc.c
  // Or from the ORFS abc_speed script
  auto step = [logger](const char* msg, auto func) {
    return [logger, msg, func](auto& gia) {
      debugPrint(logger, RMP, "gia", 1, msg);
      replaceGia(gia, func(gia));
    };
  };

  std::vector<GiaOp::OpExecutor> all_ops = {
      // &st
      step("Starting rehash &st",
           [](auto& g) { return Gia_ManRehash(g.get(), false); }),
      // &dch
      [logger](auto& g) {
        if (!g->pReprs) {
          debugPrint(
              logger, RMP, "gia", 1, "Computing choices before equiv reduce");
          abc::Dch_Pars_t pars = {};
          Dch_ManSetDefaultParams(&pars);
          replaceGia(g, Gia_ManPerformDch(g.get(), &pars));
        }
        debugPrint(logger, RMP, "gia", 1, "Starting equiv reduce");
        replaceGia(g, Gia_ManEquivReduce(g.get(), true, false, false, false));
      },
      // &syn2
      step("Starting syn2",
           [](auto& g) {
             return Gia_ManAigSyn2(
                 g.get(), false, true, 0, 20, 0, false, false);
           }),
      // &syn3
      step("Starting syn3",
           [](auto& g) { return Gia_ManAigSyn3(g.get(), false, false); }),
      // &syn4
      step("Starting syn4",
           [](auto& g) { return Gia_ManAigSyn4(g.get(), false, false); }),
      // &retime
      step("Starting retime",
           [](auto& g) { return Gia_ManRetimeForward(g.get(), 100, false); }),
      // &dc2
      step("Starting heavy rewriting",
           [](auto& g) { return Gia_ManCompress2(g.get(), true, false); }),
      // &b
      step("Starting &b",
           [](auto& g) {
             return Gia_ManAreaBalance(
                 g.get(), false, ABC_INFINITY, false, false);
           }),
      // &b -d
      step(
          "Starting &b - d",
          [](auto& g) { return Gia_ManBalance(g.get(), false, false, false); }),
      // &reduce
      [logger](auto& g) {
        if (!g->pReprs) {
          debugPrint(
              logger, RMP, "gia", 1, "Computing choices before equiv reduce");
          abc::Dch_Pars_t pars = {};
          Dch_ManSetDefaultParams(&pars);
          replaceGia(g, Gia_ManPerformDch(g.get(), &pars));
        }
        debugPrint(logger, RMP, "gia", 1, "Starting equiv reduce and remap");
        replaceGia(g, Gia_ManEquivReduceAndRemap(g.get(), true, false));
      },

      // &if -g -K 6
      [logger](auto& g) {
        if (Gia_ManHasMapping(g.get())) {
          debugPrint(logger,
                     RMP,
                     "gia",
                     1,
                     "GIA has mapping - rehashing before mapping");
          replaceGia(g, Gia_ManRehash(g.get(), false));
        }
        abc::If_Par_t pars = {};
        Gia_ManSetIfParsDefault(&pars);
        pars.fDelayOpt = true;
        pars.nLutSize = 6;
        pars.fTruth = true;
        pars.fCutMin = true;
        pars.fExpRed = false;
        debugPrint(logger, RMP, "gia", 1, "Starting SOP balancing");
        replaceGia(g, Gia_ManPerformMapping(g.get(), &pars));
      },

      // &synch2
      [logger](auto& g) {
        abc::Dch_Pars_t pars = {};
        Dch_ManSetDefaultParams(&pars);
        pars.nBTLimit = 100;
        debugPrint(logger, RMP, "gia", 1, "Starting synch2");
        replaceGia(g, Gia_ManAigSynch2(g.get(), &pars, 6, 20));
      }};
  std::vector<GiaOp> ops;
  ops.reserve(all_ops.size());
  for (size_t i = 0; i < all_ops.size(); ++i) {
    ops.emplace_back(i, all_ops[i]);
  }
  return ops;
}

static GiaOp::AigManager initGia(abc::Abc_Ntk_t* ntk, utl::Logger* logger)
{
  debugPrint(logger, RMP, "gia", 1, "Converting to GIA");
  assert(!Abc_NtkIsStrash(ntk));
  // derive comb GIA
  auto strash = WrapUnique(Abc_NtkStrash(ntk, false, true, false));
  auto aig = WrapUnique(Abc_NtkToDar(strash.get(), false, false));
  abc::Gia_Man_t* gia = Gia_ManFromAig(aig.get());
  // perform undc/zero
  auto inits = Abc_NtkCollectLatchValuesStr(ntk);
  {
    auto temp = WrapUnique(gia);
    gia = Gia_ManDupZeroUndc(gia, inits, 0, false, false);
  }
  ABC_FREE(inits);
  // copy names
  gia->vNamesIn = abc::Abc_NtkCollectCiNames(ntk);
  gia->vNamesOut = abc::Abc_NtkCollectCoNames(ntk);
  return WrapUnique(gia);
}

void RunGia(sta::dbSta* sta,
            const std::vector<sta::Vertex*>& candidate_vertices,
            cut::AbcLibrary& abc_library,
            const std::vector<GiaOp>& gia_ops,
            size_t resize_iters,
            utl::UniqueName& name_generator,
            utl::Logger* logger)
{
  sta::dbNetwork* network = sta->getDbNetwork();

  // Disable incremental timing.
  sta->graphDelayCalc()->delaysInvalid();
  sta->search()->arrivalsInvalid();
  sta->search()->endpointsInvalid();

  cut::LogicExtractorFactory logic_extractor(sta, logger);
  for (sta::Vertex* negative_endpoint : candidate_vertices) {
    logic_extractor.AppendEndpoint(negative_endpoint);
  }

  cut::LogicCut cut = logic_extractor.BuildLogicCut(abc_library);

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> mapped_abc_network
      = cut.BuildMappedAbcNetwork(abc_library, network, logger);

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> current_network(
      abc::Abc_NtkToLogic(
          const_cast<abc::Abc_Ntk_t*>(mapped_abc_network.get())),
      &abc::Abc_NtkDelete);

  auto* library
      = static_cast<abc::Mio_Library_t*>(mapped_abc_network->pManFunc);

  // Install library for NtkMap
  abc::Abc_FrameSetLibGen(library);

  debugPrint(logger,
             RMP,
             "gia",
             1,
             "Mapped ABC network has {} nodes and {} POs.",
             abc::Abc_NtkNodeNum(current_network.get()),
             abc::Abc_NtkPoNum(current_network.get()));

  current_network->pManFunc = library;

  {
    auto gia = initGia(current_network.get(), logger);

    // Run all the given GIA ops
    for (auto& op : gia_ops) {
      op.op(gia);
    }
    debugPrint(logger, RMP, "gia", 1, "Converting GIA to network");
    abc::Extra_UtilGetoptReset();

    if (Gia_ManHasCellMapping(gia.get())) {
      current_network
          = WrapUnique(abc::Abc_NtkFromCellMappedGia(gia.get(), false));
    } else if (Gia_ManHasMapping(gia.get()) || gia->pMuxes) {
      current_network
          = WrapUnique(Abc_NtkFromMappedGia(gia.get(), false, false));
    } else {
      if (Gia_ManHasDangling(gia.get()) != 0) {
        debugPrint(logger, RMP, "gia", 1, "Rehashing before conversion");
        replaceGia(gia, Gia_ManRehash(gia.get(), false));
      }
      assert(Gia_ManHasDangling(gia.get()) == 0);
      auto aig = WrapUnique(abc::Gia_ManToAig(gia.get(), false));
      current_network = WrapUnique(Abc_NtkFromAigPhase(aig.get()));
      current_network->pName = abc::Extra_UtilStrsav(aig->pName);
    }

    assert(gia->vNamesIn);
    for (int i = 0; i < abc::Abc_NtkCiNum(current_network.get()); i++) {
      assert(i < Vec_PtrSize(gia->vNamesIn));
      abc::Abc_Obj_t* obj = abc::Abc_NtkCi(current_network.get(), i);
      assert(obj);
      Nm_ManDeleteIdName(current_network->pManName, obj->Id);
      Abc_ObjAssignName(
          obj, static_cast<char*>(Vec_PtrEntry(gia->vNamesIn, i)), nullptr);
    }
    assert(gia->vNamesOut);
    for (int i = 0; i < abc::Abc_NtkCoNum(current_network.get()); i++) {
      assert(i < Vec_PtrSize(gia->vNamesOut));
      abc::Abc_Obj_t* obj = Abc_NtkCo(current_network.get(), i);
      assert(obj);
      Nm_ManDeleteIdName(current_network->pManName, obj->Id);
      assert(Abc_ObjIsPo(obj));
      Abc_ObjAssignName(
          obj, static_cast<char*>(Vec_PtrEntry(gia->vNamesOut, i)), nullptr);
    }

    // decouple CI/CO with the same name
    if (!Abc_NtkIsStrash(current_network.get())
        && (gia->vNamesIn || gia->vNamesOut)) {
      abc::Abc_NtkRedirectCiCo(current_network.get());
    }
  }

  if (!Abc_NtkIsStrash(current_network.get())) {
    current_network = WrapUnique(
        abc::Abc_NtkStrash(current_network.get(), false, true, false));
  }

  {
    utl::SuppressStdout nostdout(logger);
    current_network = WrapUnique(abc::Abc_NtkMap(current_network.get(),
                                                 nullptr,
                                                 /*DelayTarget=*/1.0,
                                                 /*AreaMulti=*/0.0,
                                                 /*DelayMulti=*/2.5,
                                                 /*LogFan=*/0.0,
                                                 /*Slew=*/0.0,
                                                 /*Gain=*/250.0,
                                                 /*nGatesMin=*/0,
                                                 /*fRecovery=*/true,
                                                 /*fSwitching=*/false,
                                                 /*fSkipFanout=*/false,
                                                 /*fUseProfile=*/false,
                                                 /*fUseBuffs=*/false,
                                                 /*fVerbose=*/false));
  }

  abc::Abc_NtkCleanup(current_network.get(), /*fVerbose=*/false);

  current_network = WrapUnique(abc::Abc_NtkDupDfs(current_network.get()));

  if (resize_iters > 0) {
    // All the magic numbers are defaults from abc/src/base/abci/abc.c
    utl::SuppressStdout nostdout(logger);
    abc::SC_SizePars pars = {};
    pars.nIters = resize_iters;
    pars.nIterNoChange = 50;
    pars.Window = 1;
    pars.Ratio = 10;
    pars.Notches = 1000;
    pars.DelayUser = 0;
    pars.DelayGap = 0;
    pars.TimeOut = 0;
    pars.BuffTreeEst = 0;
    pars.BypassFreq = 0;
    pars.fUseDept = true;
    abc::Abc_SclUpsizePerform(
        abc_library.abc_library(), current_network.get(), &pars, nullptr);
    abc::Abc_SclDnsizePerform(
        abc_library.abc_library(), current_network.get(), &pars, nullptr);
  }

  current_network = WrapUnique(abc::Abc_NtkToNetlist(current_network.get()));

  cut.InsertMappedAbcNetwork(
      current_network.get(), abc_library, network, name_generator, logger);
}
}  // namespace rmp
