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
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"
#include "sta/VerilogWriter.hh"
#include "utl/Logger.h"

namespace rsz {

using sta::Pin;
//using sta::dbStaState;

class SwapPinsMove : public BaseMove
{

public:
    using BaseMove::BaseMove;

    bool doMove(const Path* drvr_path,
               const int drvr_index,
               PathExpanded* expanded) override;

    void reportSwappablePins();

private:
    void swapPins(Instance* inst,
                  LibertyPort* port1,
                  LibertyPort* port2,
                  bool journal);
    void equivCellPins(const LibertyCell* cell,
                       LibertyPort* input_port,
                       sta::LibertyPortSet& ports);
    void findSwapPinCandidate(LibertyPort* input_port,
                              LibertyPort* drvr_port,
                              const sta::LibertyPortSet& equiv_ports,
                              float load_cap,
                              const DcalcAnalysisPt* dcalc_ap,
                              LibertyPort** swap_port);
    void resetInputSlews();

    void journalMove(Instance* inst,
                    LibertyPort* port1,
                    LibertyPort* port2);

    Map<Instance*, LibertyPortTuple> swapped_pins_;
    InstanceSet all_swapped_pin_inst_set_;

    sta::UnorderedMap<LibertyPort*, sta::LibertyPortSet> equiv_pin_map_;

};

}


