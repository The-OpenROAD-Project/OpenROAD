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

class BufferedNet;
enum class BufferedNetType;

using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using BufferedNetSeq = std::vector<BufferedNetPtr>;

using sta::Net;
using sta::Path;
using sta::Pin;

class BufferMove : public BaseMove
{

public:
    using BaseMove::BaseMove;

    virtual bool doMove(const Path* drvr_path,
                        const int drvr_index,
                        PathExpanded* expanded) override;

    const char * name() const override { return "BufferMove"; }

    void rebufferNet(const Pin* drvr_pin);

private:
    int rebuffer_net_count_ = 0;
    LibertyPort* drvr_port_ = nullptr;

    int rebuffer(const Pin* drvr_pin);

    BufferedNetPtr rebufferForTiming(const BufferedNetPtr& bnet);
    BufferedNetPtr recoverArea(const BufferedNetPtr& bnet,
                               sta::Delay slack_target,
                               float alpha);

    void debugCheckMultipleBuffers(Path* path, PathExpanded* expanded);
    bool hasTopLevelOutputPort(Net* net);

    int rebufferTopDown(const BufferedNetPtr& choice,
                        Net* net,
                        int level,
                        Instance* parent,
                        odb::dbITerm* mod_net_drvr,
                        odb::dbModNet* mod_net);
    BufferedNetPtr addWire(const BufferedNetPtr& p,
                           const Point& wire_end,
                           int wire_layer,
                           int level);
    int fanout(Vertex* vertex);
    void addBuffers(BufferedNetSeq& Z1,
                    int level,
                    bool area_oriented = false,
                    sta::Delay threshold = 0);
    float bufferInputCapacitance(LibertyCell* buffer_cell,
                                 const DcalcAnalysisPt* dcalc_ap);
    std::tuple<const Path*, sta::Delay> drvrPinTiming(const BufferedNetPtr& bnet);
    Slack slackAtDriverPin(const BufferedNetPtr& bnet);
    Slack slackAtDriverPin(const BufferedNetPtr& bnet, int index);


};

}


