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
#include <iostream>
#include <string>

#include "lefin.h"
#include "db.h"



namespace lefTechLayerSpacingTablePrl {
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;
    namespace phoenix = boost::phoenix;
   
    using boost::spirit::qi::lit;
    using boost::spirit::ascii::string;
    using boost::spirit::ascii::space_type;
    using boost::fusion::at_c;
    
    using qi::double_;
    using qi::int_;
    // using qi::_1;
    using ascii::space;
    using phoenix::ref;

    void addInfluence(boost::fusion::vector<double,double,double>& params,odb::lefTechLayerSpacingTablePrlParser* parser, odb::lefin* lefin)
    {
        auto width = lefin->dbdist(at_c<0>(params));
        auto distance = lefin->dbdist(at_c<1>(params));
        auto spacing = lefin->dbdist(at_c<2>(params));
        parser->influence_tbl.push_back(std::make_tuple(width,distance,spacing));
    }
    void addLength(double val,odb::lefTechLayerSpacingTablePrlParser* parser, odb::lefin* lefin )
    {
        parser->length_tbl.push_back(lefin->dbdist(val));
    }
    void addWidth(double val,odb::lefTechLayerSpacingTablePrlParser* parser, odb::lefin* lefin )
    {
        parser->width_tbl.push_back(lefin->dbdist(val));
        parser->curWidthIdx++;
        parser->spacing_tbl.push_back(std::vector<int>());
    }
    void addExcluded(boost::fusion::vector<double,double>& params,odb::lefTechLayerSpacingTablePrlParser* parser, odb::lefin* lefin)
    {
        auto low = lefin->dbdist(at_c<0>(params));
        auto high = lefin->dbdist(at_c<1>(params));
        parser->within_map[parser->curWidthIdx] = {low,high};
    }
    void addSpacing(double val,odb::lefTechLayerSpacingTablePrlParser* parser, odb::lefin* lefin)
    {
        parser->spacing_tbl[parser->curWidthIdx].push_back(lefin->dbdist(val));
    }
    void setEolWidth(double val, odb::dbTechLayerSpacingTablePrlRule* rule, odb::lefin* lefin)
    {
        rule->setEolWidth(lefin->dbdist(val));
    }
    template <typename Iterator>
    bool parse(Iterator first, Iterator last, odb::dbTechLayerSpacingTablePrlRule* rule, odb::lefin* lefin,odb::lefTechLayerSpacingTablePrlParser* parser)
    {

        qi::rule<std::string::iterator, space_type> influenceRule = (
            lit("SPACINGTABLE")
            >> lit("INFLUENCE")
            >> +(
                lit("WIDTH") >> double_
                >> lit("WITHIN") >> double_
                >> lit("SPACING") >> double_
            )[boost::bind(&addInfluence,_1,parser,lefin)]
            >>lit(";")
        );
        
        qi::rule<std::string::iterator, space_type> spacingTableRule = (
            lit("SPACINGTABLE")
            >> lit("PARALLELRUNLENGTH")
            >> -lit("WRONGDIRECTION")[boost::bind(&odb::dbTechLayerSpacingTablePrlRule::setWrongDirection,rule,true)]
            >> -lit("SAMEMASK")[boost::bind(&odb::dbTechLayerSpacingTablePrlRule::setSameMask,rule,true)]
            >> -(
                 lit("EXCEPTEOL")[boost::bind(&odb::dbTechLayerSpacingTablePrlRule::setExceeptEol,rule,true)]
                 >> double_ [boost::bind(&setEolWidth,_1,rule,lefin)]
            )
            >> +double_[boost::bind(&addLength,_1,parser,lefin)]
            >> +(
                lit("WIDTH")
                >> double_[boost::bind(&addWidth,_1,parser,lefin)]
                >> -(
                    lit("EXCEPTWITHIN")
                    >> double_ >> double_
                )[boost::bind(&addExcluded,_1,parser,lefin)]
                >> +double_[boost::bind(&addSpacing,_1,parser,lefin)]
            )
            >> -influenceRule
            >> lit(";")
        );

        bool r = qi::phrase_parse(first, last, spacingTableRule, space);
        

        if (first != last) // fail if we did not get a full match
            return false;
        return r;
    }
}


namespace odb{


dbTechLayerSpacingTablePrlRule* lefTechLayerSpacingTablePrlParser::parse(std::string s, dbTechLayer* layer, odb::lefin* l)
{
    dbTechLayerSpacingTablePrlRule* rule = dbTechLayerSpacingTablePrlRule::create(layer);
    if(lefTechLayerSpacingTablePrl::parse(s.begin(), s.end(), rule, l, this) ){
        bool valid = true;
        if(spacing_tbl.size()!=width_tbl.size())
            valid = false;
        for(auto spacing: spacing_tbl)
            if(spacing.size()!=length_tbl.size())
                valid = false;
        if(valid){
            rule->setTable(width_tbl,length_tbl,spacing_tbl,within_map);
            rule->setSpacingTableInfluence(influence_tbl);
            return rule;
        }
        else
        {
            odb::dbTechLayerSpacingTablePrlRule::destroy(rule);
            return NULL;
        }
    }else
    {
        odb::dbTechLayerSpacingTablePrlRule::destroy(rule);
        return NULL;
    }
    
   
} 


}

