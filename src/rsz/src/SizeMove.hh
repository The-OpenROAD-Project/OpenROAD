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

class SizeMove : public BaseMove
{

public:
    using BaseMove::BaseMove;

    void init() override;

    bool doMove(const Path* drvr_path,
               const int drvr_index,
               PathExpanded* expanded) override;
    void undoMove(int num=0) override;

    double deltaSlack() override;
    double deltaPower() override;
    double deltaArea() override;

    void clear() override;
private:
    LibertyCell* upsizeCell(LibertyPort* in_port,
                         LibertyPort* drvr_port,
                         const float load_cap,
                         const float prev_drive,
                         const DcalcAnalysisPt* dcalc_ap);
    bool replaceCell(Instance* inst,
                     const LibertyCell* replacement,
                     const bool journal);
    void journalMove(Instance* inst);


    Map<Instance*, LibertyCell*> resized_inst_map_;
    InstanceSet all_sized_inst_set_;


};

}


