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
        odb::dbTechLayerMinStepSubRule* minSubRule;
        lefTechLayerMinStepParser(){minSubRule = nullptr;};
        ~lefTechLayerMinStepParser(){};
        dbTechLayerMinStepRule* parse(std::string, dbTechLayer*, lefin*);
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

    class lefTechLayerRightWayOnGridOnlyParser
    {
        
    public:
        static dbTechLayerRightWayOnGridOnlyRule* parse(std::string, dbTechLayer*, lefin*);
    };
    
    class lefTechLayerRectOnlyParser
    {
        
    public:
        static dbTechLayerRectOnlyRule* parse(std::string, dbTechLayer*, lefin*);
    };

    class lefTechLayerCutClassParser
    {
        
    public:
        static void parse(std::string, dbTechLayer*, lefin*);
    };

    class lefTechLayerCutSpacingParser
    {
    
    public:
        odb::dbTechLayerCutSpacingSubRule* subRule;
        lefTechLayerCutSpacingParser(){subRule = nullptr;};
        ~lefTechLayerCutSpacingParser(){};
        dbTechLayerCutSpacingRule* parse(std::string, dbTechLayer*, lefin*);
    };

    class lefTechLayerCutSpacingTableParser
    {
        
    public:
        odb::dbTechLayerCutSpacingTableDefSubRule* defRule;
        lefTechLayerCutSpacingTableParser(){defRule = nullptr;};
        ~lefTechLayerCutSpacingTableParser(){};
        dbTechLayerCutSpacingTableRule* parse(std::string, dbTechLayer*, lefin*);
    };


}

