// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "annealing_strategy.h"

#include <fcntl.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <utility>
#include <vector>

#include "aig/aig/aig.h"
#include "aig/gia/gia.h"
#include "aig/gia/giaAig.h"
#include "base/abc/abc.h"
#include "base/main/main.h"
#include "cut/abc_library_factory.h"
#include "cut/logic_cut.h"
#include "cut/logic_extractor.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "map/if/if.h"
#include "map/mio/mio.h"
#include "map/scl/sclLib.h"
#include "map/scl/sclSize.h"
#include "misc/extra/extra.h"
#include "misc/nm/nm.h"
#include "misc/vec/vecPtr.h"
#include "odb/db.h"
#include "proof/dch/dch.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/MinMax.hh"
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
}  // namespace abc
namespace rmp {
using utl::RMP;

static void replaceGia(abc::Gia_Man_t*& gia, abc::Gia_Man_t* new_gia)
{
  if (gia == new_gia) {
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
  abc::Gia_ManStop(gia);
  gia = new_gia;
}

void AnnealingStrategy::OptimizeDesign(sta::dbSta* sta,
                                       utl::UniqueName& name_generator,
                                       rsz::Resizer* resizer,
                                       utl::Logger* logger)
{
  sta->ensureGraph();
  sta->ensureLevelized();
  sta->searchPreamble();
  for (auto mode : sta->modes()) {
    sta->ensureClkNetwork(mode);
  }
  auto block = sta->db()->getChip()->getBlock();

  auto candidate_vertices = GetEndpoints(sta, resizer, slack_threshold_);
  if (candidate_vertices.empty()) {
    logger->info(utl::RMP,
                 51,
                 "All endpoints have slack above threshold, nothing to do.");
    return;
  }

  cut::AbcLibraryFactory factory(logger);
  factory.AddDbSta(sta);
  factory.AddResizer(resizer);
  factory.SetCorner(corner_);
  cut::AbcLibrary abc_library = factory.Build();

  // GIA ops as lambdas
  // All the magic numbers are defaults from abc/src/base/abci/abc.c
  // Or from the ORFS abc_speed script
  std::vector<GiaOp> all_ops
      = {[&](auto& gia) {
           // &st
           debugPrint(logger, RMP, "annealing", 1, "Starting rehash");
           replaceGia(gia, Gia_ManRehash(gia, false));
         },

         [&](auto& gia) {
           // &dch
           if (!gia->pReprs) {
             debugPrint(logger,
                        RMP,
                        "annealing",
                        1,
                        "Computing choices before equiv reduce");
             abc::Dch_Pars_t pars = {};
             Dch_ManSetDefaultParams(&pars);
             replaceGia(gia, Gia_ManPerformDch(gia, &pars));
           }
           debugPrint(logger, RMP, "annealing", 1, "Starting equiv reduce");
           replaceGia(gia, Gia_ManEquivReduce(gia, true, false, false, false));
         },

         [&](auto& gia) {
           // &syn2
           debugPrint(logger, RMP, "annealing", 1, "Starting syn2");
           replaceGia(gia,
                      Gia_ManAigSyn2(gia, false, true, 0, 20, 0, false, false));
         },

         [&](auto& gia) {
           // &syn3
           debugPrint(logger, RMP, "annealing", 1, "Starting syn3");
           replaceGia(gia, Gia_ManAigSyn3(gia, false, false));
         },

         [&](auto& gia) {
           // &syn4
           debugPrint(logger, RMP, "annealing", 1, "Starting syn4");
           replaceGia(gia, Gia_ManAigSyn4(gia, false, false));
         },

         [&](auto& gia) {
           // &retime
           debugPrint(logger, RMP, "annealing", 1, "Starting retime");
           replaceGia(gia, Gia_ManRetimeForward(gia, 100, false));
         },

         [&](auto& gia) {
           // &dc2
           debugPrint(logger, RMP, "annealing", 1, "Starting heavy rewriting");
           replaceGia(gia, Gia_ManCompress2(gia, true, false));
         },

         [&](auto& gia) {
           // &b
           debugPrint(logger, RMP, "annealing", 1, "Starting &b");
           replaceGia(
               gia, Gia_ManAreaBalance(gia, false, ABC_INFINITY, false, false));
         },

         [&](auto& gia) {
           // &b -d
           debugPrint(logger, RMP, "annealing", 1, "Starting &b -d");
           replaceGia(gia, Gia_ManBalance(gia, false, false, false));
         },

         [&](auto& gia) {
           // &false
           debugPrint(
               logger, RMP, "annealing", 1, "Starting false path elimination");
           utl::SuppressStdout nostdout(logger);
           replaceGia(gia, Gia_ManCheckFalse(gia, 0, 0, false, false));
         },

         [&](auto& gia) {
           // &reduce
           if (!gia->pReprs) {
             debugPrint(logger,
                        RMP,
                        "annealing",
                        1,
                        "Computing choices before equiv reduce");
             abc::Dch_Pars_t pars = {};
             Dch_ManSetDefaultParams(&pars);
             replaceGia(gia, Gia_ManPerformDch(gia, &pars));
           }
           debugPrint(
               logger, RMP, "annealing", 1, "Starting equiv reduce and remap");
           replaceGia(gia, Gia_ManEquivReduceAndRemap(gia, true, false));
         },

         [&](auto& gia) {
           // &if -g -K 6
           if (Gia_ManHasMapping(gia)) {
             debugPrint(logger,
                        RMP,
                        "annealing",
                        1,
                        "GIA has mapping - rehashing before mapping");
             replaceGia(gia, Gia_ManRehash(gia, false));
           }
           abc::If_Par_t pars = {};
           Gia_ManSetIfParsDefault(&pars);
           pars.fDelayOpt = true;
           pars.nLutSize = 6;
           pars.fTruth = true;
           pars.fCutMin = true;
           pars.fExpRed = false;
           debugPrint(logger, RMP, "annealing", 1, "Starting SOP balancing");
           replaceGia(gia, Gia_ManPerformMapping(gia, &pars));
         },

         [&](auto& gia) {
           // &synch2
           abc::Dch_Pars_t pars = {};
           Dch_ManSetDefaultParams(&pars);
           pars.nBTLimit = 100;
           debugPrint(logger, RMP, "annealing", 1, "Starting synch2");
           replaceGia(gia, Gia_ManAigSynch2(gia, &pars, 6, 20));
         }};
  /* Some ABC functions/commands that could be used, but crash in some
     permutations:
      * &nf. Call it like this:
            namespace abc {
            extern Gia_Man_t* Nf_ManPerformMapping(Gia_Man_t* pGia,
                                                   Jf_Par_t* pPars);
            }
            abc::Jf_Par_t pars = {};
            Nf_ManSetDefaultPars(&pars);
            new_gia = Nf_ManPerformMapping(gia, &pars);
        It crashes on a null pointer due to a missing time manager. We can make
        the time manager:
            gia->pManTime = abc::Tim_ManStart(Gia_ManCiNum(new_gia),
                                              Gia_ManCoNum(new_gia));
        But then, an assert deep in &nf fails.
      * &dsd. Call it like this:
            namespace abc {
            extern Gia_Man_t* Gia_ManCollapseTest(Gia_Man_t* p, int fVerbose);
            }
            new_gia = Gia_ManCollapseTest(gia, false);
        An assert fails.

      Some functions/commands don't actually exist:
      * &resub
      * &reshape, &reshape -a
      These are just stubs that return null.
   */

  // Computes a random neighbor of a given GIA op list
  const auto neighbor = [&](std::vector<GiaOp> ops) {
    enum Move
    {
      ADD,
      REMOVE,
      SWAP,
      COUNT
    };
    Move move = ADD;
    if (ops.size() > 1) {
      move = Move(random_() % (COUNT));
    }
    switch (move) {
      case ADD: {
        debugPrint(logger, RMP, "annealing", 2, "Adding a new GIA operation");
        size_t i = random_() % (ops.size() + 1);
        size_t j = random_() % all_ops.size();
        ops.insert(ops.begin() + i, all_ops[j]);
      } break;
      case REMOVE: {
        debugPrint(logger, RMP, "annealing", 2, "Removing a GIA operation");
        size_t i = random_() % ops.size();
        ops.erase(ops.begin() + i);
      } break;
      case SWAP: {
        debugPrint(
            logger, RMP, "annealing", 2, "Swapping adjacent GIA operations");
        size_t i = random_() % (ops.size() - 1);
        std::swap(ops[i], ops[i + 1]);
      } break;
      case COUNT:
        // unreachable
        std::abort();
    }
    return ops;
  };

  // Initial solution and slack
  debugPrint(logger,
             RMP,
             "annealing",
             1,
             "Generating and evaluating the initial solution");
  std::vector<GiaOp> ops;
  ops.reserve(initial_ops_);
  for (size_t i = 0; i < initial_ops_; i++) {
    ops.push_back(all_ops[random_() % all_ops.size()]);
  }

  // The magic numbers are defaults from abc/src/base/abci/abc.c
  const size_t SEARCH_RESIZE_ITERS = 100;
  const size_t FINAL_RESIZE_ITERS = 1000;

  odb::dbDatabase::beginEco(block);

  RunGia(sta,
         candidate_vertices,
         abc_library,
         ops,
         SEARCH_RESIZE_ITERS,
         name_generator,
         logger);

  odb::dbDatabase::endEco(block);

  float worst_slack;
  sta::Vertex* worst_vertex;
  sta->worstSlack(corner_, sta::MinMax::max(), worst_slack, worst_vertex);

  odb::dbDatabase::undoEco(block);

  if (!temperature_) {
    sta::Delay required = sta->required(worst_vertex,
                                        sta::RiseFallBoth::riseFall(),
                                        sta->scenes(),
                                        sta::MinMax::max());
    temperature_ = required;
  }

  logger->info(RMP, 52, "Resynthesis: starting simulated annealing");
  logger->info(RMP,
               53,
               "Initial temperature: {}, worst slack: {}",
               *temperature_,
               worst_slack);

  float best_worst_slack = worst_slack;
  auto best_ops = ops;
  size_t worse_iters = 0;

  for (unsigned i = 0; i < iterations_; i++) {
    float current_temp
        = *temperature_ * (static_cast<float>(iterations_ - i) / iterations_);

    if (revert_after_ && worse_iters >= *revert_after_) {
      logger->info(RMP, 57, "Reverting to the best found solution");
      ops = best_ops;
      worst_slack = best_worst_slack;
      worse_iters = 0;
    }

    if ((i + 1) % 10 == 0) {
      logger->info(RMP,
                   54,
                   "Iteration: {}, temperature: {}, best worst slack: {}",
                   i + 1,
                   current_temp,
                   best_worst_slack);
    } else {
      debugPrint(logger,
                 RMP,
                 "annealing",
                 1,
                 "Iteration: {}, temperature: {}, best worst slack: {}",
                 i + 1,
                 current_temp,
                 best_worst_slack);
    }

    odb::dbDatabase::beginEco(block);

    auto new_ops = neighbor(ops);
    RunGia(sta,
           candidate_vertices,
           abc_library,
           new_ops,
           SEARCH_RESIZE_ITERS,
           name_generator,
           logger);

    odb::dbDatabase::endEco(block);

    float worst_slack_new;
    sta->worstSlack(corner_, sta::MinMax::max(), worst_slack_new, worst_vertex);

    odb::dbDatabase::undoEco(block);

    if (worst_slack_new < best_worst_slack) {
      worse_iters++;
    } else {
      worse_iters = 0;
    }

    if (worst_slack_new < worst_slack) {
      float accept_prob
          = current_temp == 0
                ? 0
                : std::exp((worst_slack_new - worst_slack) / current_temp);
      debugPrint(
          logger,
          RMP,
          "annealing",
          1,
          "Current worst slack: {}, new: {}, accepting new ABC script with "
          "probability {}",
          worst_slack,
          worst_slack_new,
          accept_prob);
      if (std::uniform_real_distribution<float>(0, 1)(random_) < accept_prob) {
        debugPrint(logger,
                   RMP,
                   "annealing",
                   1,
                   "Accepting new ABC script with worse slack");
      } else {
        debugPrint(logger,
                   RMP,
                   "annealing",
                   1,
                   "Rejecting new ABC script with worse slack");
        continue;
      }
    } else {
      debugPrint(logger,
                 RMP,
                 "annealing",
                 1,

                 "Current worst slack: {}, new: {}, accepting new ABC script",
                 worst_slack,
                 worst_slack_new);
    }

    ops = std::move(new_ops);
    worst_slack = worst_slack_new;

    if (worst_slack > best_worst_slack) {
      best_worst_slack = worst_slack;
      best_ops = ops;
    }
  }

  logger->info(
      RMP, 55, "Resynthesis: End of simulated annealing, applying operations");
  logger->info(RMP, 56, "Resynthesis: Applying ABC operations");

  // Apply the ops
  RunGia(sta,
         candidate_vertices,
         abc_library,
         best_ops,
         FINAL_RESIZE_ITERS,
         name_generator,
         logger);
}

void AnnealingStrategy::RunGia(
    sta::dbSta* sta,
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

  {
    auto library
        = static_cast<abc::Mio_Library_t*>(mapped_abc_network->pManFunc);

    // Install library for NtkMap
    abc::Abc_FrameSetLibGen(library);

    debugPrint(logger,
               RMP,
               "annealing",
               1,
               "Mapped ABC network has {} nodes and {} POs.",
               abc::Abc_NtkNodeNum(current_network.get()),
               abc::Abc_NtkPoNum(current_network.get()));

    current_network->pManFunc = library;
    abc::Gia_Man_t* gia = nullptr;

    {
      debugPrint(logger, RMP, "annealing", 1, "Converting to GIA");
      auto ntk = current_network.get();
      assert(!Abc_NtkIsStrash(ntk));
      // derive comb GIA
      auto strash = Abc_NtkStrash(ntk, false, true, false);
      auto aig = Abc_NtkToDar(strash, false, false);
      Abc_NtkDelete(strash);
      gia = Gia_ManFromAig(aig);
      Aig_ManStop(aig);
      // perform undc/zero
      auto inits = Abc_NtkCollectLatchValuesStr(ntk);
      auto temp = gia;
      gia = Gia_ManDupZeroUndc(gia, inits, 0, false, false);
      Gia_ManStop(temp);
      ABC_FREE(inits);
      // copy names
      gia->vNamesIn = abc::Abc_NtkCollectCiNames(ntk);
      gia->vNamesOut = abc::Abc_NtkCollectCoNames(ntk);
    }

    // Run all the given GIA ops
    for (auto& op : gia_ops) {
      op(gia);
    }

    {
      debugPrint(logger, RMP, "annealing", 1, "Converting GIA to network");
      abc::Extra_UtilGetoptReset();

      if (Gia_ManHasCellMapping(gia)) {
        current_network = WrapUnique(abc::Abc_NtkFromCellMappedGia(gia, false));
      } else if (Gia_ManHasMapping(gia) || gia->pMuxes) {
        current_network = WrapUnique(Abc_NtkFromMappedGia(gia, false, false));
      } else {
        if (Gia_ManHasDangling(gia) != 0) {
          debugPrint(
              logger, RMP, "annealing", 1, "Rehashing before conversion");
          replaceGia(gia, Gia_ManRehash(gia, false));
        }
        assert(Gia_ManHasDangling(gia) == 0);
        auto aig = Gia_ManToAig(gia, false);
        current_network = WrapUnique(Abc_NtkFromAigPhase(aig));
        current_network->pName = abc::Extra_UtilStrsav(aig->pName);
        Aig_ManStop(aig);
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

      Gia_ManStop(gia);
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
  }

  cut.InsertMappedAbcNetwork(
      current_network.get(), abc_library, network, name_generator, logger);
}
}  // namespace rmp
