// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "MoveCandidate.hh"

namespace rsz {

MoveCandidate::MoveCandidate(Resizer& resizer, const Target& target)
    : resizer_(resizer), target_(target)
{
}

MoveResult MoveCandidate::rejectedMove() const
{
  return {
      .accepted = false,
      .type = type(),
      .touched_instances = {},
  };
}

}  // namespace rsz
