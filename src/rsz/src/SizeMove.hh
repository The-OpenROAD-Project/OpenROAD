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
//using sta::dbStaState;

class SizeMove : public BaseMove
{

public:
    using BaseMove::BaseMove;

    bool doMove(const Path* drvr_path,
               const int drvr_index,
               PathExpanded* expanded) override;

    const char * name() const override { return "SizeMove"; }

private:
    LibertyCell* upsizeCell(LibertyPort* in_port,
                         LibertyPort* drvr_port,
                         const float load_cap,
                         const float prev_drive,
                         const DcalcAnalysisPt* dcalc_ap);
    bool replaceCell(Instance* inst,
                     const LibertyCell* replacement);


};

}


