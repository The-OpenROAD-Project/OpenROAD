// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <cmath>
#include <vector>
#include <utility>

#include "BaseMove.hh"

namespace rsz {

class CloneMove : public BaseMove
{

public:
    using BaseMove::BaseMove;

    bool doMove(const Path* drvr_path,
                const int drvr_index,
                Slack drvr_slack,
                PathExpanded* expanded,
                float setup_slack_margin) override;

    const char * name() const override { return "CloneMove"; }

private:
    Point computeCloneGateLocation(const Pin* drvr_pin,
                                   const std::vector<std::pair<Vertex*, Slack>>& fanout_slacks);

    bool cloneDriver(const Path* drvr_path,
                     int drvr_index,
                     Slack drvr_slack,
                     PathExpanded* expanded);



};

}  // namespace rsz


