// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/SdcNetwork.hh"

namespace sta {

class dbSdcNetwork : public SdcNetwork
{
 public:
  explicit dbSdcNetwork(Network* network);
  Instance* findInstance(const char* path_name) const override;
  InstanceSeq findInstancesMatching(const Instance* contex,
                                    const PatternMatch* pattern) const override;
  NetSeq findNetsMatching(const Instance*,
                          const PatternMatch* pattern) const override;
  PinSeq findPinsMatching(const Instance* instance,
                          const PatternMatch* pattern) const override;

 protected:
  void findInstancesMatching1(const PatternMatch* pattern,
                              InstanceSeq& insts) const;
  void findNetsMatching1(const PatternMatch* pattern, NetSeq& nets) const;
  void findMatchingPins(const Instance* instance,
                        const PatternMatch* port_pattern,
                        PinSeq& pins) const;
  Pin* findPin(const char* path_name) const override;
  using SdcNetwork::findPin;
};

}  // namespace sta
