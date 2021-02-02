#include "lefLayerPropParser.h"
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/bind.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/include/make_vector.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "lefin.h"
#include "db.h"



namespace lefTechLayerCutSpacingTable {
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;
    namespace phoenix = boost::phoenix;
    using qi::lexeme;
    using ascii::char_;
    using boost::spirit::qi::lit;
    using boost::spirit::ascii::string;
    using boost::spirit::ascii::space_type;
    using boost::spirit::ascii::alpha;
    using boost::fusion::at_c;
    
    using qi::double_;
    using qi::int_;
    // using qi::_1;
    using ascii::space;
    using phoenix::ref;
    void createOrthongonalSubRule(std::vector<boost::fusion::vector<double,double>> params, 
                                  odb::dbTechLayerCutSpacingTableRule* rule, 
                                  odb::lefin* lefin)
    {
        odb::dbTechLayerCutSpacingTableOrthSubRule* subRule = odb::dbTechLayerCutSpacingTableOrthSubRule::create(rule);
        std::vector<std::pair<int,int>> table;
        for(auto item: params)
            table.push_back({lefin->dbdist(at_c<0>(item)),lefin->dbdist(at_c<1>(item))});
        subRule->setSpacingTable(table);
    }
    void createDefSubRule( odb::lefTechLayerCutSpacingTableParser* parser, odb::dbTechLayerCutSpacingTableRule* rule)
    {
        parser->defRule = odb::dbTechLayerCutSpacingTableDefSubRule::create(rule);
    }
    void setDefault(double value,odb::lefTechLayerCutSpacingTableParser* parser,odb::lefin* lefin)
    {
        parser->defRule->setDefaultValid(true);
        parser->defRule->setDefault(lefin->dbdist(value));
    }
    void setLayer(std::string value,odb::lefTechLayerCutSpacingTableParser* parser)
    {
        parser->defRule->setLayerValid(true);
        parser->defRule->setSecondLayerName(value.c_str());
    }
    void setPrlForAlignedCut(std::vector<boost::fusion::vector<std::string,std::string>> params,
                             odb::lefTechLayerCutSpacingTableParser* parser)
    {
        parser->defRule->setPrlForAlignedCut(true);
        std::vector<std::pair<char*,char*>> table;
        for(auto item : params)
            table.push_back({strdup(at_c<0>(item).c_str()),strdup(at_c<1>(item).c_str())});
        parser->defRule->setPrlForAlignedCutTable(table);
    }
    void setCenterToCenter(std::vector<boost::fusion::vector<std::string,std::string>> params,
                             odb::lefTechLayerCutSpacingTableParser* parser)
    {
        parser->defRule->setCenterToCenterValid(true);
        std::vector<std::pair<char*,char*>> table;
        for(auto item : params)
            table.push_back({strdup(at_c<0>(item).c_str()),strdup(at_c<1>(item).c_str())});
        parser->defRule->setCenterToCenterTable(table);
    }
    void setCenterAndEdge(std::vector<boost::fusion::vector<std::string,std::string>> params,
                             odb::lefTechLayerCutSpacingTableParser* parser)
    {
        parser->defRule->setCenterAndEdgeValid(true);
        std::vector<std::pair<char*,char*>> table;
        for(auto item : params)
            table.push_back({strdup(at_c<0>(item).c_str()),strdup(at_c<1>(item).c_str())});
        parser->defRule->setCenterAndEdgeTable(table);
    }
    void setPRL(boost::fusion::vector<double,
                                      boost::optional<std::string>,
                                      std::vector<boost::fusion::vector<std::string,std::string,double>>> params,
                             odb::lefTechLayerCutSpacingTableParser* parser,
                             odb::lefin* lefin)
    {
        parser->defRule->setPrlValid(true);
        auto prl = at_c<0>(params);
        auto maxxy = at_c<1>(params);
        auto items = at_c<2>(params);
        parser->defRule->setPrl(lefin->dbdist(prl));
        if(maxxy.is_initialized())
            parser->defRule->setMaxXY(true);
        
        std::vector<std::tuple<char*,char*,int>> table;
        for(auto item : items)
            table.push_back({strdup(at_c<0>(item).c_str()),strdup(at_c<1>(item).c_str()),lefin->dbdist(at_c<2>(item))});
        parser->defRule->setPrlTable(table);
    }
    void setExactAlignedSpacing(boost::fusion::vector<
                                      boost::optional<std::string>,
                                      std::vector<boost::fusion::vector<std::string,double>>> params,
                             odb::lefTechLayerCutSpacingTableParser* parser,
                             odb::lefin* lefin)
    {
        parser->defRule->setExactAlignedSpacingValid(true);
        auto dir = at_c<0>(params);
        auto items = at_c<1>(params);
        if(dir.is_initialized()){
            if(dir.value()=="HORIZONTAL")
                parser->defRule->setHorizontal(true);
            else
                parser->defRule->setVertical(true);
        }
        
        std::vector<std::pair<char*,int>> table;
        for(auto item : items)
            table.push_back({strdup(at_c<0>(item).c_str()),lefin->dbdist(at_c<1>(item))});
        
        parser->defRule->setExactAlignedSpacingTable(table);
    }

    void setNonOppositeEnclosureSpacing(std::vector<boost::fusion::vector<std::string,double>> params,
                             odb::lefTechLayerCutSpacingTableParser* parser,
                             odb::lefin* lefin)
    {
        parser->defRule->setNonOppositeEnclosureSpacingValid(true);
        std::vector<std::pair<char*,int>> table;
        for(auto item : params)
            table.push_back({strdup(at_c<0>(item).c_str()),lefin->dbdist(at_c<1>(item))});
        parser->defRule->setNonOppEncSpacingTable(table);
    }

    void setOppositeEnclosureResizeSpacing(std::vector<boost::fusion::vector<std::string,double,double,double>> params,
                             odb::lefTechLayerCutSpacingTableParser* parser,
                             odb::lefin* lefin)
    {
        parser->defRule->setOppositeEnclosureResizeSpacingValid(true);
        std::vector<std::tuple<char*,int,int,int>> table;
        for(auto item : params)
            table.push_back({strdup(at_c<0>(item).c_str()),lefin->dbdist(at_c<1>(item)),lefin->dbdist(at_c<2>(item)),lefin->dbdist(at_c<3>(item))});
        parser->defRule->setOppEncSpacingTable(table);
    }

    void setEndExtension(boost::fusion::vector<double,
                                      std::vector<boost::fusion::vector<std::string,double>>> params,
                             odb::lefTechLayerCutSpacingTableParser* parser,
                             odb::lefin* lefin)
    {
        parser->defRule->setEndExtensionValid(true);
        auto ext = at_c<0>(params);
        auto items = at_c<1>(params);
        parser->defRule->setExtension(lefin->dbdist(ext));        
        std::vector<std::pair<char*,int>> table;
        for(auto item : items)
            table.push_back({strdup(at_c<0>(item).c_str()),lefin->dbdist(at_c<1>(item))});
        parser->defRule->setEndExtensionTable(table);
    }
    void setSideExtension(std::vector<boost::fusion::vector<std::string,double>> params,
                          odb::lefTechLayerCutSpacingTableParser* parser,
                          odb::lefin* lefin)
    {
        parser->defRule->setSideExtensionValid(true);
        std::vector<std::pair<char*,int>> table;
        for(auto item : params)
            table.push_back({strdup(at_c<0>(item).c_str()),lefin->dbdist(at_c<1>(item))});
        parser->defRule->setSideExtensionTable(table);
    }
    
    void setSameMask(odb::lefTechLayerCutSpacingTableParser* parser)
    {
        parser->defRule->setSameMask(true);
    }
    void setSameNet(odb::lefTechLayerCutSpacingTableParser* parser)
    {
        parser->defRule->setSameNet(true);
    }
    void setSameMetal(odb::lefTechLayerCutSpacingTableParser* parser)
    {
        parser->defRule->setSameMetal(true);
    }
    void setSameVia(odb::lefTechLayerCutSpacingTableParser* parser)
    {
        parser->defRule->setSameVia(true);
    }
    void setNoStack(odb::lefTechLayerCutSpacingTableParser* parser)
    {
        parser->defRule->setNoStack(true);
    }
    void setNonZeroEnclosure(odb::lefTechLayerCutSpacingTableParser* parser)
    {
        parser->defRule->setNonZeroEnclosure(true);
    }
    void setNoPrl(odb::lefTechLayerCutSpacingTableParser* parser)
    {
        parser->defRule->setNoPrl(true);
    }

    void setCutClass(boost::fusion::vector<
                        std::vector<
                            boost::fusion::vector<
                                std::string,
                                boost::optional<std::string>
                            >
                        >,
                        std::vector<
                            boost::fusion::vector<
                                boost::variant<std::string,double>,
                                boost::variant<std::string,double>
                            >
                        >,
                        std::vector<
                            boost::fusion::vector<
                                std::string,
                                boost::optional<std::string>,
                                std::vector<
                                    boost::fusion::vector<
                                        boost::variant<std::string,double>,
                                        boost::variant<std::string,double>
                                    >
                                >
                            >
                        >
                    > &params, odb::lefTechLayerCutSpacingTableParser* parser,odb::lefin* lefin)
    {
        auto colsNamesAndFirstRowName = at_c<0>(params);
        auto firstRowWithOutName = at_c<1>(params);
        auto allRows = at_c<2>(params);
        std::map<std::string,uint> cols;
        std::map<std::string,uint> rows;
        std::vector<std::vector<std::pair<int,int>>> table;
        uint colSz = colsNamesAndFirstRowName.size()-1;
        for(uint i = 0; i < colSz; i++)
        {
            std::string name = at_c<0>(colsNamesAndFirstRowName[i]);
            auto OPTION = at_c<1>(colsNamesAndFirstRowName[i]);
            if(OPTION.is_initialized())
            {
                name+="/"+OPTION.value();
            }
            cols[name] = i;
        }
        
        boost::fusion::vector<
                                std::string,
                                boost::optional<std::string>,
                                std::vector<
                                    boost::fusion::vector<
                                        boost::variant<std::string,double>,
                                        boost::variant<std::string,double>
                                    >
                                >
                            > firstRow(at_c<0>(colsNamesAndFirstRowName[colSz]),at_c<1>(colsNamesAndFirstRowName[colSz]),firstRowWithOutName);
        allRows.insert(allRows.begin(),firstRow);
        uint rowSz = allRows.size();
        for(uint i = 0;i<rowSz;i++)
        {
            std::string name = at_c<0>(allRows[i]);
            auto OPTION = at_c<1>(allRows[i]);
            auto items = at_c<2>(allRows[i]);
            if(OPTION.is_initialized())
            {
                name+="/"+OPTION.value();
            }
            rows[name] = i;
            table.push_back(std::vector<std::pair<int,int>>(colSz));
            for(uint j = 0; j<items.size(); j++)
            {
                auto item = items[j];
                auto spacing1 = at_c<0>(item);
                auto spacing2 = at_c<1>(item);
                int sp1,sp2;
                if(spacing1.which()==0)
                    sp1 = parser->defRule->getDefault();
                else
                    sp1 = lefin->dbdist(boost::get<double>(spacing1));
                if(spacing2.which()==0)
                    sp2 = parser->defRule->getDefault();
                else
                    sp2 = lefin->dbdist(boost::get<double>(spacing2));
                table[i][j] = {sp1,sp2};
            }
        }
        for(auto it = cols.cbegin(); it != cols.cend(); )
        {
            std::string col = (*it).first;
            int i = (*it).second;
            size_t idx = col.find_last_of('/');
            if(idx == std::string::npos)
            {
                if(cols.find(col+"/SIDE")!=cols.end())
                    cols[col+"/END"] = i;
                else if(cols.find(col+"/END")!=cols.end())
                    cols[col+"/SIDE"] = i;
                else
                {
                    cols[col+"/SIDE"] = i;
                    cols[col+"/END"] = colSz++;
                    for(size_t k = 0;k < table.size();k++)
                        table[k].push_back(table[k][i]);
                }
                cols.erase(it++);
            }else
                ++it;
        }
        for(auto it = rows.cbegin(); it != rows.cend(); )
        {
            std::string row = (*it).first;
            int i = (*it).second;
            size_t idx = row.find_last_of('/');
            if(idx == std::string::npos)
            {
                if(rows.find(row+"/SIDE")!=rows.end())
                    rows[row+"/END"] = i;
                else if(rows.find(row+"/END")!=rows.end())
                    rows[row+"/SIDE"] = i;
                else
                {
                    rows[row+"/SIDE"] = i;
                    rows[row+"/END"] = rowSz++;
                    table.push_back(table[i]);
                }
                rows.erase(it++);
            }else
                ++it;
        }
        parser->defRule->setSpacingTable(table,rows,cols);
    }
    void print(std::string str)
    {
        std::cout<<str<<std::endl;
    }

    template <typename Iterator>
    bool parse(Iterator first, Iterator last, odb::lefTechLayerCutSpacingTableParser* parser, odb::dbTechLayerCutSpacingTableRule* rule, odb::lefin* lefin)
    {   
        qi::rule<Iterator, std::string(), ascii::space_type> _string;
        _string %= lexeme[(alpha >> *(char_ - ' ' -'\n'))];

        qi::rule<std::string::iterator, space_type> ORTHOGONAL = (
            lit ("SPACINGTABLE")
            >> lit("ORTHOGONAL")
            >> +( lit("WITHIN") >> double_ >> lit("SPACING") >> double_)
            >> lit(";")
        )[boost::bind(&createOrthongonalSubRule,_1,rule,lefin)];
        
        qi::rule<std::string::iterator, space_type> LAYER = (
            lit("LAYER")
            >> _string[boost::bind(&setLayer,_1,parser)]
            >> -lit("NOSTACK")[boost::bind(&setNoStack,parser)]
            >> -(
                lit("NONZEROENCLOSURE")[boost::bind(&setNonZeroEnclosure,parser)]
                |(
                    lit("PRLFORALIGNEDCUT")
                    >> +(_string >> lit("TO") >> _string)
                )[boost::bind(&setPrlForAlignedCut,_1,parser)]
            )
        );

        qi::rule<std::string::iterator, space_type> CENTERTOCENTER = (
            lit("CENTERTOCENTER")>> +(_string >> lit("TO") >> _string)
        )[boost::bind(&setCenterToCenter,_1,parser)];

        qi::rule<std::string::iterator, space_type> CENTERANDEDGE = (
            lit("CENTERANDEDGE")
            >> -lit("NOPRL")[boost::bind(&setNoPrl,parser)]
            >> (+(_string >> lit("TO") >> _string))[boost::bind(&setCenterAndEdge,_1,parser)]
        );
        qi::rule<std::string::iterator, space_type> PRL = (
            lit("PRL")
            >> double_
            >> -string("MAXXY")
            >> *(_string >> lit("TO") >> _string >> double_)
        )[boost::bind(&setPRL,_1,parser,lefin)];
        qi::rule<std::string::iterator, space_type> EXTENSION = (
            (
                lit("ENDEXTENSION")
                >> double_
                >> *(lit("TO") >> _string >> double_)
            )[boost::bind(&setEndExtension,_1,parser,lefin)]
            >>-(
                lit("SIDEEXTENSION")
                >> +(lit("TO") >> _string >> double_)
            )[boost::bind(&setSideExtension,_1,parser,lefin)]
        );

        qi::rule<std::string::iterator, space_type> EXACTALIGNEDSPACING = (
            lit("EXACTALIGNEDSPACING")
            >> -(string("HORIZONTAL") | string("VERTICAL"))
            >> +(lit("TO") >> _string >> double_)
        )[boost::bind(&setExactAlignedSpacing,_1,parser,lefin)];

        qi::rule<std::string::iterator, space_type> NONOPPOSITEENCLOSURESPACING = (
            lit("NONOPPOSITEENCLOSURESPACING")
            >> +(_string >> double_)
        )[boost::bind(&setNonOppositeEnclosureSpacing,_1,parser,lefin)];

        qi::rule<std::string::iterator, space_type> OPPOSITEENCLOSURERESIZESPACING = (
            lit("OPPOSITEENCLOSURERESIZESPACING")
            >> +(_string >> double_ >> double_ >> double_)
        )[boost::bind(&setOppositeEnclosureResizeSpacing,_1,parser,lefin)];

        qi::rule<std::string::iterator, space_type> CUTCLASS = (
            lit("CUTCLASS")
            >> +(
                _string
                >> -(string("SIDE") | string("END") )
                )
            >> +(
                (string("-") | double_)
                >> (string("-") | double_)
            )
            >> *(
                _string >> -(string("SIDE") | string("END"))
                >> +(
                    (string("-") | double_)
                    >> (string("-") | double_)
                )
            )
        )[boost::bind(&setCutClass,_1,parser,lefin)]; 

        qi::rule<std::string::iterator, space_type> DEFAULT = (
            lit("SPACINGTABLE")[boost::bind(&createDefSubRule,parser,rule)]
            >> -(
                lit("DEFAULT")
                >> double_[boost::bind(&setDefault,_1,parser,lefin)]
            )
            >> -lit("SAMEMASK")[boost::bind(&setSameMask,parser)]
            >> -(
                lit("SAMENET")[boost::bind(&setSameNet,parser)]
                | lit("SAMEMETAL")[boost::bind(&setSameMetal,parser)]
                | lit("SAMEVIA")[boost::bind(&setSameVia,parser)]
            )
            >> -LAYER
            >> -CENTERTOCENTER
            >> -CENTERANDEDGE
            >> -PRL
            >> -EXTENSION
            >> -EXACTALIGNEDSPACING
            >> -NONOPPOSITEENCLOSURESPACING
            >> -OPPOSITEENCLOSURERESIZESPACING
            >> CUTCLASS
            >>lit(";")
        );

        qi::rule<std::string::iterator, space_type> LEF58_SPACINGTABLE = (
            +( ORTHOGONAL | DEFAULT )
        );
        bool r = qi::phrase_parse(first, last, LEF58_SPACINGTABLE, space);
        if (first != last) // fail if we did not get a full match
            return false;
        return r;
    }
}


namespace odb{


dbTechLayerCutSpacingTableRule* lefTechLayerCutSpacingTableParser::parse(std::string s, dbTechLayer* layer, odb::lefin* l)
{
    odb::dbTechLayerCutSpacingTableRule* rule = odb::dbTechLayerCutSpacingTableRule::create(layer);
    if(lefTechLayerCutSpacingTable::parse(s.begin(), s.end(), this, rule, l) )
        return rule;
    else 
    {
        odb::dbTechLayerCutSpacingTableRule::destroy(rule);
        return nullptr;
    }
} 


}

