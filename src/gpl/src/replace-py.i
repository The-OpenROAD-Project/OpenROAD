%{
#include "ord/OpenRoad.hh"
#include "gpl/Replace.h"
#include "odb/db.h"

namespace ord {
OpenRoad*
getOpenRoad();

gpl::Replace*
getReplace();

}

using ord::getOpenRoad;
using ord::getReplace;
using gpl::Replace;

%}

%include "../../Exception-py.i"
%include "gpl/Replace.h"
