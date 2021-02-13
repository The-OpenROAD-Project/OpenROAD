%module mpl

%{
#include "openroad/OpenRoad.hh"
#include "mpl/TritonMacroPlace.h"

namespace ord {
// Defined in OpenRoad.i
mpl::TritonMacroPlace*
getMacroPlacer();
}

using ord::getMacroPlacer;

%}

%include "../../Exception.i"

%inline %{

namespace mpl {

void
set_macro_place_global_config_cmd(const char* file) 
{
  TritonMacroPlace* tritonMp = getMacroPlacer();
  tritonMp->setGlobalConfig(file); 
}

void
set_macro_place_local_config_cmd(const char* file)
{
  TritonMacroPlace* tritonMp = getMacroPlacer();
  tritonMp->setLocalConfig(file); 
}

void
set_macro_place_fence_region_cmd(double lx, double ly, double ux, double uy)
{
  TritonMacroPlace* tritonMp = getMacroPlacer();
  tritonMp->setFenceRegion(lx, ly, ux, uy); 
}

void
place_macros_cmd()
{
  TritonMacroPlace* tritonMp = getMacroPlacer();
  tritonMp->placeMacros(); 
} 

int
get_macro_place_solution_count_cmd()
{
  TritonMacroPlace* tritonMp = getMacroPlacer();
  return tritonMp->getSolutionCount();
} 

}

%} // inline
