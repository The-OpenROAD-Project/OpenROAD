
#pragma once

#include <map>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "db/obj/frAccess.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frInst.h"
#include "db/obj/frMPin.h"
#include "db/obj/frMTerm.h"
#include "db/obj/frTrackPattern.h"
#include "db/tech/frTechObject.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "global.h"
#include "odb/db.h"
#include "odb/dbTypes.h"

namespace drt {
struct cgIntraCellEdge
{
  frMTerm* term0;
  frAccessPoint* pa0;
  frMTerm* term1;
  frAccessPoint* pa1;

  cgIntraCellEdge(frMTerm* t0, frAccessPoint* p0, frMTerm* t1, frAccessPoint* p1)
      : term0(t0), pa0(p0), term1(t1), pa1(p1)
  {
  }
};

struct cgCellEdge
{
  frInstTerm* term0;
  frAccessPoint* pa0;
  frInstTerm* term1;
  frAccessPoint* pa1;

  cgCellEdge(frInstTerm* t0, frAccessPoint* p0, frInstTerm* t1, frAccessPoint* p1)
      : term0(t0), pa0(p0), term1(t1), pa1(p1)
  {
  }
};

struct cgConflictGraph {
  std::string path;
  odb::Rect window;
  std::vector<cgCellEdge> edges;
};

}  // namespace drt