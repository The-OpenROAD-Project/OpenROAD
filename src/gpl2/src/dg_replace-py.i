%{
#include "ord/OpenRoad.hh"
#include "gpl2/DgReplace.h"
#include "odb/db.h"

namespace ord {
OpenRoad*
getOpenRoad();

gpl2::DgReplace*
getDgReplace();

}

using ord::getOpenRoad;
using ord::getDgReplace;
using gpl2::DgReplace;

%}

%include "../../Exception-py.i"
%include "gpl2/DgReplace.h"
