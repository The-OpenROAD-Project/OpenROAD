#pragma once

#include <string>
#include "db.h"
#include "lefin.h"


namespace odb{

    class lefTechLayerMinStepParser
    {
        
    public:
        static dbTechLayerMinStepRule* parse(std::string, dbTechLayer*, lefin*);
    };


}

