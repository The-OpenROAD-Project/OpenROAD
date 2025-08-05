#include "global.h"
#include "Design.h"
#include "CUGR.h"

int main(int argc, char* argv[]) {
    logeol(2);
    log() << "GLOBAL ROUTER CUGR" << std::endl;
    logeol(2);
    // Parse parameters
    Parameters parameters(argc, argv);
    
    // Read LEF/DEF
    Design design(parameters);
    
    // Global router
    CUGR globalRouter(design, parameters);
    globalRouter.route();
    globalRouter.write();
    
    logeol();
    log() << "Terminated." << std::endl;
    loghline();
    logmem();
    logeol();
}