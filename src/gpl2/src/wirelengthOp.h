///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
// Copyright (c) 2024, Antmicro
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "placerBase.h"
#include "placerObjects.h"
#include "utl/Logger.h"

namespace gpl2 {

class WirelengthOp
{
 public:
  WirelengthOp();
  WirelengthOp(PlacerBaseCommon* pbc);
  ~WirelengthOp() = default;

  void computeWireLengthForce(float wlCoeffX,
                              float wlCoeffY,
                              float virtualWeightFactor,
                              const Kokkos::View<float*>& wirelengthForceX,
                              const Kokkos::View<float*>& wirelengthForceY);

  int64_t computeHPWL();
  int64_t computeWeightedHPWL(float virtualWeightFactor);

  void updatePinLocation(const Kokkos::View<const int*>& instDCx, const Kokkos::View<const int*>& instDCy);

 private:
  PlacerBaseCommon* pbc_;
  utl::Logger* logger_;

  // basic information
  int numInsts_;
  int numPins_;
  int numNets_;
  int numPlaceInsts_;

  // refer to hMETIS or TritonPart for this structure
  // map inst to pins
  Kokkos::View<int*> dInstPinIdx_;
  Kokkos::View<int*>
      dInstPinPos_;  // start - end position of pins of each inst
  Kokkos::View<int*> dPinInstId_;  // inst id of each pin
  // map net to pins
  Kokkos::View<int*> dNetPinIdx_;
  Kokkos::View<int*>
      dNetPinPos_;  // start - end position of pins of each net
  Kokkos::View<int*> dPinNetId_;  // net id of each pin

  // Pin information
  Kokkos::View<int*> dPinX_;
  Kokkos::View<int*> dPinY_;
  Kokkos::View<int*> dPinOffsetX_;
  Kokkos::View<int*> dPinOffsetY_;
  Kokkos::View<float*> dPinGradX_;
  Kokkos::View<float*> dPinGradY_;

  // For each pin, we have
  // aPosX, aPosY, aNegX, aNegY,
  // bPosX, bPosY, bNegX, bNegY,
  // cPosX, cPosY, cNegX, cNegY
  Kokkos::View<float*> dPinAPosX_;
  Kokkos::View<float*> dPinAPosY_;
  Kokkos::View<float*> dPinANegX_;
  Kokkos::View<float*> dPinANegY_;
  Kokkos::View<float*> dNetBPosX_;

  Kokkos::View<float*> dPinAPosX_modifiable_b_;
  Kokkos::View<float*> dPinANegX_modifiable_b_;
  Kokkos::View<float*> dPinAPosY_modifiable_b_;
  Kokkos::View<float*> dPinANegY_modifiable_b_;

  Kokkos::View<float*> dPinAPosX_modifiable_c_;
  Kokkos::View<float*> dPinANegX_modifiable_c_;
  Kokkos::View<float*> dPinAPosY_modifiable_c_;
  Kokkos::View<float*> dPinANegY_modifiable_c_;

  // Net information
  Kokkos::View<int*> dNetWidth_;
  Kokkos::View<int*> dNetHeight_;
  Kokkos::View<int*> dNetLx_;
  Kokkos::View<int*> dNetLy_;
  Kokkos::View<int*> dNetUx_;
  Kokkos::View<int*> dNetUy_;
  Kokkos::View<float*> dNetWeight_;
  Kokkos::View<float*> dNetVirtualWeight_;

  Kokkos::View<float*> dNetBPosY_;
  Kokkos::View<float*> dNetBNegX_;
  Kokkos::View<float*> dNetBNegY_;
  Kokkos::View<float*> dNetCPosX_;
  Kokkos::View<float*> dNetCPosY_;
  Kokkos::View<float*> dNetCNegX_;
  Kokkos::View<float*> dNetCNegY_;

  // device memory management
  void initBackend();
};

}  // namespace gpl2
