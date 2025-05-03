// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <cmath>

#include "BaseMove.hh"
#include "rsz/Resizer.hh"

namespace rsz {

class SplitLoadMove : public BaseMove
{

public:
    using BaseMove::BaseMove;

    bool doMove(const Path* drvr_path,
                const int drvr_index,
                const Slack drvr_slack,
                PathExpanded* expanded) override;

    const char * name() const override { return "SplitLoadMove"; }

private:


};

}


