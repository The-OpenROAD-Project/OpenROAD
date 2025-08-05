#include "global.h"
#include "obj/Design.h"
#include "gr/GlobalRouter.h"

int main(int argc, char* argv[]) {
    logeol(2);
    log() << "GLOBAL ROUTER CUGR" << std::endl;
    logeol(2);
    // Parse parameters
    Parameters parameters(argc, argv);
    
    // Read LEF/DEF
    Design design(parameters);
    
    // Global router
    GlobalRouter globalRouter(design, parameters);
    globalRouter.route();
    globalRouter.write();
    
    logeol();
    log() << "Terminated." << std::endl;
    loghline();
    logmem();
    logeol();
}