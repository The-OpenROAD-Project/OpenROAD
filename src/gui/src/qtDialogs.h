// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "gui/gui.h"
#include "odb/db.h"

namespace sta {
class dbSta;
}

namespace gui {

// Qt implementation of the dialogs the Descriptor actions need.  Installed
// on the Gui singleton by Gui::init(), which only runs in a binary that
// links the Qt gui.
class QtDialogs : public Dialogs
{
 public:
  std::optional<int> chooseItem(const std::string& title,
                                const std::string& label,
                                const std::vector<std::string>& items,
                                int current) override;

  odb::dbInst* insertBuffer(odb::dbNet* net, sta::dbSta* sta) override;
};

}  // namespace gui
