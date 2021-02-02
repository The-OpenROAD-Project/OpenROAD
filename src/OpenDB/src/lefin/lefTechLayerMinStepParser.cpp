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



namespace lefTechLayerMinStep {
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

    void createSubRule(odb::lefTechLayerMinStepParser* parser, odb::dbTechLayerMinStepRule* rule)
    {
        parser->minSubRule = odb::dbTechLayerMinStepSubRule::create(rule);
    }
    void setMinAdjacentLength1(double length, odb::lefTechLayerMinStepParser* parser, odb::lefin* l)
    {
        parser->minSubRule->setMinAdjLength1(l->dbdist(length));
        parser->minSubRule->setMinAdjLength1Valid(true);
    }
    void setMinAdjacentLength2(double length, odb::lefTechLayerMinStepParser* parser, odb::lefin* l)
    {
        parser->minSubRule->setMinAdjLength2(l->dbdist(length));
        parser->minSubRule->setMinAdjLength2Valid(true);
    }
    void minBetweenLngthParser(double length, odb::lefTechLayerMinStepParser* parser, odb::lefin* l)
    {
        parser->minSubRule->setMinBetweenLength(l->dbdist(length));
        parser->minSubRule->setMinBetweenLengthValid(true);
    }
    void minStepLengthParser(double length, odb::lefTechLayerMinStepParser* parser, odb::lefin* l)
    {
        parser->minSubRule->setMinStepLength(l->dbdist(length));
    }
    void maxEdgesParser(int edges, odb::lefTechLayerMinStepParser* parser, odb::lefin* l)
    {
        parser->minSubRule->setMaxEdges(edges);
        parser->minSubRule->setMaxEdgesValid(true);
    }

    void setConvexCorner(odb::lefTechLayerMinStepParser* parser)
    {
        parser->minSubRule->setConvexCorner(true);
    }
    
    void setExceptSameCorners(odb::lefTechLayerMinStepParser* parser)
    {
        parser->minSubRule->setExceptSameCorners(true);
    }

    template <typename Iterator>
    bool parse(Iterator first, Iterator last, odb::lefTechLayerMinStepParser* parser, odb::dbTechLayerMinStepRule* rule, odb::lefin* l)
    {

        qi::rule<std::string::iterator, space_type> minAdjacentRule = (
                                                                        lit("MINADJACENTLENGTH") 
                                                                        >> double_[boost::bind(&setMinAdjacentLength1, _1, parser, l)]
                                                                        >> -(string("CONVEXCORNER")[boost::bind(&setConvexCorner, parser)] 
                                                                            |
                                                                            double_[boost::bind(&setMinAdjacentLength2, _1, parser, l)])
                                                                      );
                                                                      
        qi::rule<std::string::iterator, space_type> minBetweenLngthRule = ( lit("MINBETWEENLENGTH") 
                                                                        >> (double_ [boost::bind(&minBetweenLngthParser, _1, parser, l)])
                                                                        >> -(lit("EXCEPTSAMECORNERS") [boost::bind(&setExceptSameCorners, parser)])
                                                                        );
        qi::rule<std::string::iterator, space_type> minstepRule = ( +(
                                                                        lit("MINSTEP") [boost::bind(&createSubRule, parser, rule)]
                                                                        >> double_ [boost::bind(&minStepLengthParser, _1, parser, l)]
                                                                        >> -(lit("MAXEDGES") >> int_ [boost::bind(&maxEdgesParser, _1, parser, l)] )
                                                                        >> -( minAdjacentRule | minBetweenLngthRule ) 
                                                                        >> lit(";")
                                                                    )    
                                                                  );

        bool r = qi::phrase_parse(first, last, minstepRule, space);
        

        if (first != last) // fail if we did not get a full match
            return false;
        return r;
    }
}


namespace odb{


dbTechLayerMinStepRule* lefTechLayerMinStepParser::parse(std::string s, dbTechLayer* layer, odb::lefin* l)
{
    dbTechLayerMinStepRule* rule = dbTechLayerMinStepRule::create(layer);
    if(lefTechLayerMinStep::parse(s.begin(), s.end(), this, rule, l) )
        return rule;
    else 
    {
        odb::dbTechLayerMinStepRule::destroy(rule);
        return nullptr;
    }
} 


}

