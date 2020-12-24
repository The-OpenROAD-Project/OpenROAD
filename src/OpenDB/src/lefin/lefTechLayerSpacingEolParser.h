#pragma once

#include <string>
#include "db.h"
#include "lefin.h"


namespace odb{

    class lefTechLayerSpacingEolParser
    {
        
    public:
        static dbTechLayerSpacingEolRule* parse(std::string, dbTechLayer*, lefin*);
    };


}

