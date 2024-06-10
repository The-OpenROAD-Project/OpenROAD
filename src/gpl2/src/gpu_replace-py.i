%{
#include "ord/OpenRoad.hh"
#include "gpl2/GpuReplace.h"
#include "odb/db.h"

namespace ord {
OpenRoad*
getOpenRoad();

gpl2::GpuReplace*
getGpuReplace();

}

using ord::getOpenRoad;
using ord::getGpuReplace;
using gpl2::GpuReplace;

%}

%include "../../Exception-py.i"
%include "gpl2/GpuReplace.h"
