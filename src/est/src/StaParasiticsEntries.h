// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include "sta/NetworkClass.hh"
#include "sta/Parasitics.hh"

namespace est {

// OpenSTA's parasitics store finds and makes parasitics without a lock
// once every driver and its net have an entry, and makeParasiticNetwork
// no longer adds the entry itself. OpenSTA before that change has
// neither call and adds the entry under its own lock, so there is
// nothing to do. Call these only where no other thread uses the store.
template <typename Parasitics = sta::Parasitics>
void makeParasiticsEntries(Parasitics* parasitics)
{
  if constexpr (requires { parasitics->ensureParasitics(); }) {
    parasitics->ensureParasitics();
  }
}

template <typename Parasitics = sta::Parasitics>
void makeParasiticsEntry(Parasitics* parasitics, const sta::Pin* drvr_pin)
{
  if constexpr (requires { parasitics->ensureParasitics(drvr_pin); }) {
    parasitics->ensureParasitics(drvr_pin);
  }
}

}  // namespace est
