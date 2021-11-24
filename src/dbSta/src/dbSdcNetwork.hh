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

#ifndef DB_SDC_NETWORK_H
#define DB_SDC_NETWORK_H

#include "sta/SdcNetwork.hh"

namespace sta {

class dbSdcNetwork : public SdcNetwork
{
public:
  dbSdcNetwork(Network *network);
  virtual Instance *findInstance(const char *path_name) const;
  virtual void findInstancesMatching(const Instance *contex,
				     const PatternMatch *pattern,
				     InstanceSeq *insts) const;
  virtual void findNetsMatching(const Instance *,
				const PatternMatch *pattern,
				NetSeq *nets) const;
  virtual void findPinsMatching(const Instance *instance,
				const PatternMatch *pattern,
				PinSeq *pins) const;

protected:
  void findInstancesMatching1(const PatternMatch *pattern,
			      InstanceSeq *insts) const;
  void findNetsMatching1(const PatternMatch *pattern,
			 NetSeq *nets) const;
  void findMatchingPins(const Instance *instance,
			const PatternMatch *port_pattern,
			PinSeq *pins) const;
  Pin *findPin(const char *path_name) const;

  using SdcNetwork::findPin;
};

} // namespace
#endif
