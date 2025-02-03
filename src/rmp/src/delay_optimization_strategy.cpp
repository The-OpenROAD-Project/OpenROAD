// Copyright 2024 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "delay_optimization_strategy.h"

#include "abc_library_factory.h"
#include "base/abc/abc.h"
#include "map/mio/mio.h"
#include "map/scl/sclLib.h"
#include "map/scl/sclSize.h"
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
}  // namespace abc

namespace rmp {

utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> WrapUnique(abc::Abc_Ntk_t* ntk)
{
  return utl::UniquePtrWithDeleter<abc::Abc_Ntk_t>(ntk, &abc::Abc_NtkDelete);
}

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
    AbcLibrary& abc_sc_library)
{
  abc::SC_BusPars buffer_parameters;
  memset(&buffer_parameters, 0, sizeof(abc::SC_BusPars));
  buffer_parameters.GainRatio = 300;
  buffer_parameters.Slew
      = abc::Abc_SclComputeAverageSlew(abc_sc_library.abc_library());
  buffer_parameters.nDegree = 5;
  buffer_parameters.fSizeOnly = 0;
  buffer_parameters.fAddBufs = false;
  buffer_parameters.fBufPis = true;
  buffer_parameters.fUseWireLoads = 0;
  buffer_parameters.fVerbose = true;
  buffer_parameters.fVeryVerbose = false;

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> current_network
      = WrapUnique(abc::Abc_SclBufferingPerform(
          ntk, abc_sc_library.abc_library(), &buffer_parameters));
  return current_network;
}

utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> DelayOptimizationStrategy::Optimize(
    const abc::Abc_Ntk_t* ntk,
    utl::Logger* logger)
{
  auto library = static_cast<abc::Mio_Library_t*>(ntk->pManFunc);
  // Install library for NtkMap
  abc::Abc_FrameSetLibGen(library);

  // Set up libraries for buffering
  AbcLibraryFactory library_factory(logger);
  library_factory.AddDbSta(sta_);
  AbcLibrary abc_sc_library = library_factory.Build();

  AbcPrintStats(ntk);

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> current_network(
      abc::Abc_NtkToLogic(const_cast<abc::Abc_Ntk_t*>(ntk)),
      &abc::Abc_NtkDelete);

  current_network = WrapUnique(abc::Abc_NtkStrash(current_network.get(),
                                                  /*fAllNodes=*/false,
                                                  /*fCleanup=*/true,
                                                  /*fRecord=*/false));
  current_network = WrapUnique(Abc_NtkBalance(current_network.get(),
                                              /*fDuplicate=*/true,
                                              /*fSelective=*/false,
                                              /*fUpdateLevel=*/true));

  current_network = WrapUnique(abc::Abc_NtkMap(current_network.get(),
                                               nullptr,
                                               /*DelayTarget=*/-1.0,
                                               /*AreaMulti=*/0.0,
                                               /*DelayMulti=*/1.0,
                                               /*LogFan=*/10.0,
                                               /*Slew=*/0.0,
                                               /*Gain=*/250.0,
                                               /*nGatesMin=*/0,
                                               /*fRecovery=*/false,
                                               /*fSwitching=*/false,
                                               /*fSkipFanout=*/false,
                                               /*fUseProfile=*/false,
                                               /*fUseBuffs=*/true,
                                               /*fVerbose=*/false));

  abc::Abc_NtkCleanup(current_network.get(), /*fVerbose=*/false);

  current_network = BufferNetwork(current_network.get(), abc_sc_library);

  current_network = WrapUnique(abc::Abc_NtkToNetlist(current_network.get()));

  return current_network;
}

}  // namespace rmp
