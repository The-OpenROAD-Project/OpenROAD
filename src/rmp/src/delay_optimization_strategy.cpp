// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "delay_optimization_strategy.h"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstring>
#include <mutex>

#include "base/abc/abc.h"
#include "cut/abc_library_factory.h"
#include "map/mio/mio.h"
#include "map/scl/sclLib.h"
#include "map/scl/sclSize.h"
#include "utils.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace abc {
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
extern void Abc_FrameSetLibGen(void* pLib);
extern void Abc_FrameSetDrivingCell(char* pName);
}  // namespace abc

namespace rmp {

void AbcPrintStats(const abc::Abc_Ntk_t* ntk)
{
  abc::Abc_NtkPrintStats(const_cast<abc::Abc_Ntk_t*>(ntk),
                         /*fFactored=*/false,
                         /*fSaveBest=*/false,
                         /*fDumpResult=*/false,
                         /*fUseLutLib=*/false,
                         /*fPrintMuxes=*/false,
                         /*fPower=*/false,
                         /*fGlitch=*/false,
                         /*fSkipBuf=*/false,
                         /*fSkipSmall=*/false,
                         /*fPrintMem=*/false);
}

utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> BufferNetwork(
    abc::Abc_Ntk_t* ntk,
    cut::AbcLibrary& abc_sc_library)
{
  abc::SC_BusPars buffer_parameters;
  memset(&buffer_parameters, 0, sizeof(abc::SC_BusPars));
  buffer_parameters.GainRatio = 300;
  buffer_parameters.Slew
      = abc::Abc_SclComputeAverageSlew(abc_sc_library.abc_library());
  buffer_parameters.nDegree = 8;
  buffer_parameters.fSizeOnly = 0;
  buffer_parameters.fAddBufs = true;
  buffer_parameters.fBufPis = true;
  buffer_parameters.fUseWireLoads = false;
  buffer_parameters.fVerbose = false;
  buffer_parameters.fVeryVerbose = false;

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> current_network
      = WrapUnique(abc::Abc_SclBufferingPerform(
          ntk, abc_sc_library.abc_library(), &buffer_parameters));
  return current_network;
}

// Exclusive lock to protect the as of yet static unsafe ABC functions.
std::mutex abc_library_mutex;

utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> DelayOptimizationStrategy::Optimize(
    const abc::Abc_Ntk_t* ntk,
    cut::AbcLibrary& abc_library,
    utl::Logger* logger)
{
  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> current_network(
      abc::Abc_NtkToLogic(const_cast<abc::Abc_Ntk_t*>(ntk)),
      &abc::Abc_NtkDelete);

  current_network = WrapUnique(abc::Abc_NtkStrash(current_network.get(),
                                                  /*fAllNodes=*/false,
                                                  /*fCleanup=*/true,
                                                  /*fRecord=*/false));
  current_network = WrapUnique(Abc_NtkBalance(current_network.get(),
                                              /*fDuplicate=*/true,
                                              /*fSelective=*/true,
                                              /*fUpdateLevel=*/true));
  {
    // Lock the tech mapping and buffer since they rely on static variables.
    const std::lock_guard<std::mutex> lock(abc_library_mutex);

    auto library = static_cast<abc::Mio_Library_t*>(ntk->pManFunc);

    // Install library for NtkMap
    abc::Abc_FrameSetLibGen(library);
    abc::Mio_Gate_t* buffer_cell = abc::Mio_LibraryReadBuf(library);
    if (!buffer_cell) {
      logger->error(
          utl::RMP,
          49,
          "Cannot find buffer cell in abc library (ensure buffer "
          "exists in your PDK), if it does please report this internal error.");
    }
    abc::Abc_FrameSetDrivingCell(strdup(abc::Mio_GateReadName(buffer_cell)));

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

    abc::Abc_NtkCleanup(current_network.get(), /*fVerbose=*/false);

    current_network = BufferNetwork(current_network.get(), abc_library);
  }

  current_network = WrapUnique(abc::Abc_NtkToNetlist(current_network.get()));

  return current_network;
}

}  // namespace rmp
