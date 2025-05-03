// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include "BaseMove.hh"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/Liberty.hh"
#include "sta/Parasitics.hh"
#include "sta/PathExpanded.hh"
#include "sta/Path.hh"
#include "sta/Sdc.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"
#include "sta/VerilogWriter.hh"
#include "utl/Logger.h"

namespace rsz {

using sta::Pin;

class UnbufferMove : public BaseMove
{

public:
    using BaseMove::BaseMove;

    bool doMove(const Path* drvr_path,
                LibertyCell* drvr_cell,
                const int drvr_index,
                PathExpanded* expanded,
                const float setup_slack_margin);

    const char * name() const override { return "UnbufferMove"; }

    bool removeBufferIfPossible(Instance* buffer, bool honorDontTouchFixed);

private:

    void removeBuffer(Instance* buffer);
    bool canRemoveBuffer(Instance* buffer, bool honorDontTouchFixed);
    bool bufferBetweenPorts(Instance* buffer);

    static constexpr int buffer_removal_max_fanout_ = 10;

};

}


