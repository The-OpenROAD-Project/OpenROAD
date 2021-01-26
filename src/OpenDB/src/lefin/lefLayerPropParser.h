#pragma once

#include <string>
#include "db.h"
#include "lefin.h"
#include <vector>
#include <map>


namespace odb{
    class lefTechLayerSpacingEolParser
    {
        
    public:
        static dbTechLayerSpacingEolRule* parse(std::string, dbTechLayer*, lefin*);
    };

    class lefTechLayerMinStepParser
    {
        
    public:
        static dbTechLayerMinStepRule* parse(std::string, dbTechLayer*, lefin*);
    };

    class lefTechLayerCornerSpacingParser
    {
        
    public:
        static dbTechLayerCornerSpacingRule* parse(std::string, dbTechLayer*, lefin*);
    };

    class lefTechLayerSpacingTablePrlParser
    {
        
    public:
        std::vector<int> length_tbl;
        std::vector<int> width_tbl;
        std::vector<std::vector<int>> spacing_tbl;
        std::map<unsigned int,std::pair<int,int>> within_map;
        std::vector<std::tuple<int,int,int>> influence_tbl;
        int curWidthIdx = -1;
        dbTechLayerSpacingTablePrlRule* parse(std::string, dbTechLayer*, lefin*);
    };



}

