// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <cmath>

#include "BaseMove.hh"
#include "rsz/Resizer.hh"
#include "sta/ExceptionPath.hh"

namespace rsz {

using sta::Pin;
//using sta::dbStaState;

class SizeMove : public BaseMove
{

public:
    using BaseMove::BaseMove;

    bool doMove(const Path* drvr_path,
               int drvr_index,
               PathExpanded* expanded) override;

    const char * name() const override { return "SizeMove"; }

private:
    LibertyCell* upsizeCell(LibertyPort* in_port,
                         LibertyPort* drvr_port,
                         float load_cap,
                         float prev_drive,
                         const DcalcAnalysisPt* dcalc_ap);
    bool replaceCell(Instance* inst,
                     const LibertyCell* replacement);


};

}  // namespace rsz


