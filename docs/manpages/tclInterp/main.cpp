#include <tcl.h>
#include <tclreadline.h>
#include <tclExtend.h>
#include <iostream>

// Minimal example of Tcl shell
int tclAppInit(Tcl_Interp* interp)
{
  return 0;
}

int main(int argc, char* argv[]){
    // Setup the app with tcl
    auto* interp = Tcl_CreateInterp();
    Tclx_Init(interp);
    Tcl_Main(1, argv, tclAppInit);
}
