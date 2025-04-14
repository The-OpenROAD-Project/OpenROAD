// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "journal.h"
namespace dpo {
void Journal::clearJournal()
{
  actions_.clear();
  affected_nodes_.clear();
}
}  // namespace dpo
