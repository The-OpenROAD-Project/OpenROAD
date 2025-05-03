// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <cmath>

#include "BaseMove.hh"
#include "rsz/Resizer.hh"
#include "sta/ExceptionPath.hh"
#include "sta/LibertyClass.hh"
#include "sta/UnorderedMap.hh"

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

    const char * name() const override { return "SwapPinsMove"; }

    void reportSwappablePins();

private:
    void swapPins(Instance* inst,
                  LibertyPort* port1,
                  LibertyPort* port2);
    void equivCellPins(const LibertyCell* cell,
                       LibertyPort* input_port,
                       sta::LibertyPortSet& ports);
    void annotateInputSlews(Instance* inst,
                            const DcalcAnalysisPt* dcalc_ap);
    void findSwapPinCandidate(LibertyPort* input_port,
                              LibertyPort* drvr_port,
                              const sta::LibertyPortSet& equiv_ports,
                              float load_cap,
                              const DcalcAnalysisPt* dcalc_ap,
                              LibertyPort** swap_port);
    void resetInputSlews();


    Map<Instance*, LibertyPortTuple> swapped_pins_;
    InstanceSet all_swapped_pin_inst_set_;

    sta::UnorderedMap<LibertyPort*, sta::LibertyPortSet> equiv_pin_map_;

};

}


