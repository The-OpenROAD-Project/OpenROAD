#include "lefTechLayerMinStepParser.h"
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
    
    void minAdjacentParser (const boost::fusion::vector<double,boost::optional< boost::variant<std::string, double> > >& params, odb::dbTechLayerMinStepRule* rule, odb::lefin* l)
    {
        auto adjLength1 = at_c<0>(params);
        rule->setMinAdjLength1(l->dbdist(adjLength1));
        rule->setMinAdjLength1Valid(true);
        auto opt = at_c<1>(params);
        if(opt.is_initialized())
        {
            boost::variant<std::string, double> var = opt.value();
            if(var.type()==typeid(std::string))
            {
                rule->setConvexCorner(true);
            }else
            {
                rule->setMinAdjLength2(l->dbdist(boost::get<double>(var)));
                rule->setMinAdjLength2Valid(true);
            }
        }
    }
    void minBetweenLngthParser(double length, odb::dbTechLayerMinStepRule* rule, odb::lefin* l)
    {
        rule->setMinBetweenLength(l->dbdist(length));
        rule->setMinBetweenLengthValid(true);
    }
    void minStepLengthParser(double length, odb::dbTechLayerMinStepRule* rule, odb::lefin* l)
    {
        rule->setMinStepLength(l->dbdist(length));
    }
    void maxEdgesParser(int edges, odb::dbTechLayerMinStepRule* rule, odb::lefin* l)
    {
        rule->setMaxEdges(edges);
        rule->setMaxEdgesValid(true);
    }

    template <typename Iterator>
    bool parse(Iterator first, Iterator last, odb::dbTechLayerMinStepRule* rule, odb::lefin* l)
    {

        qi::rule<std::string::iterator, space_type> minAdjacentRule = (
                                                                        lit("MINADJACENTLENGTH") 
                                                                        >> double_ 
                                                                        >> -(string("CONVEXCORNER") | double_)
                                                                      );
        qi::rule<std::string::iterator, space_type> minBetweenLngthRule = ( lit("MINBETWEENLENGTH") 
                                                                        >> (double_ [boost::bind(&minBetweenLngthParser, _1, rule, l)])
                                                                        >> -(lit("EXCEPTSAMECORNERS") [boost::bind(&odb::dbTechLayerMinStepRule::setExceptSameCorners, rule, true)])
                                                                        );
        qi::rule<std::string::iterator, space_type> minstepRule = (
                                                                        lit("MINSTEP") 
                                                                        >> double_ [boost::bind(&minStepLengthParser, _1, rule, l)]
                                                                        >> -(lit("MAXEDGES") >> int_ [boost::bind(&maxEdgesParser, _1, rule, l)] )
                                                                        >> -( minAdjacentRule | minBetweenLngthRule )     
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
    if(lefTechLayerMinStep::parse(s.begin(), s.end(), rule, l) )
        return rule;
    else 
    {
        // should delete rule 
        return NULL;
    }
} 


}

