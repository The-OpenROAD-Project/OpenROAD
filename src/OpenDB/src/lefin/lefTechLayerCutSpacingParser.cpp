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



namespace lefTechLayerCutSpacing {
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;
    namespace phoenix = boost::phoenix;
    using qi::lexeme;
    using ascii::char_;
    using boost::spirit::qi::lit;
    using boost::spirit::ascii::string;
    using boost::spirit::ascii::space_type;
    using boost::fusion::at_c;
    
    using qi::double_;
    using qi::int_;
    // using qi::_1;
    using ascii::space;
    using phoenix::ref;
    void setCutSpacing(double value, odb::lefTechLayerCutSpacingParser* parser, odb::dbTechLayerCutSpacingRule* rule, odb::lefin* lefin)
    {
        parser->subRule = odb::dbTechLayerCutSpacingSubRule::create(rule);
        parser->subRule->setCutSpacing(lefin->dbdist(value));
        parser->subRule->setType(odb::dbTechLayerCutSpacingSubRule::CutSpacingType::NONE);
    }
    void setCenterToCenter(odb::lefTechLayerCutSpacingParser* parser)
    {
        parser->subRule->setCenterToCenter(true);
    }
    void setSameMetal(odb::lefTechLayerCutSpacingParser* parser)
    {
        parser->subRule->setSameMetal(true);
    }
    void setSameNet(odb::lefTechLayerCutSpacingParser* parser)
    {
        parser->subRule->setSameNet(true);
    }
    void setSameVia(odb::lefTechLayerCutSpacingParser* parser)
    {
        parser->subRule->setSameVias(true);
    }
    void addMaxXYSubRule(odb::lefTechLayerCutSpacingParser* parser)
    {
        parser->subRule->setType(odb::dbTechLayerCutSpacingSubRule::CutSpacingType::MAXXY);
    }
    void addLayerSubRule(boost::fusion::vector<std::string, boost::optional<std::string> >& params,
                         odb::lefTechLayerCutSpacingParser* parser)
    {
        parser->subRule->setType(odb::dbTechLayerCutSpacingSubRule::CutSpacingType::LAYER);
        auto name = at_c<0>(params);
        parser->subRule->setSecondLayerName(name.c_str());
        auto stack = at_c<1>(params);
        if(stack.is_initialized())
            parser->subRule->setStack(true);
    }

    void addAdjacentCutsSubRule(boost::fusion::vector<std::string , 
                                                      boost::optional<int>,
                                                      double,
                                                      boost::optional<std::string>,
                                                      boost::optional<std::string>,
                                                      boost::optional<std::string>>& params,
                                odb::lefTechLayerCutSpacingParser* parser, 
                                odb::lefin* lefin)
    {
        parser->subRule->setType(odb::dbTechLayerCutSpacingSubRule::CutSpacingType::ADJACENTCUTS);
        // auto var = at_c<0>(params);
        auto cuts = at_c<0>(params);
        auto aligned = at_c<1>(params);
        auto within = at_c<2>(params);
        auto except_same_pgnet = at_c<3>(params);
        auto className = at_c<4>(params);
        auto sideParallelOverLap = at_c<5>(params);
        uint cuts_int = (uint) cuts[0] - (uint) '0';
        parser->subRule->setAdjacentCuts(cuts_int);
        if(aligned.is_initialized()){
            parser->subRule->setExactAligned(true);
            parser->subRule->setNumCuts(aligned.value());
        }
        parser->subRule->setWithin(lefin->dbdist(within));
        if(except_same_pgnet.is_initialized())
            parser->subRule->setExceptSamePgnet(true);
        if(className.is_initialized())
        {
            parser->subRule->setCutClassValid(true);
            parser->subRule->setCutClassName(className.value().c_str());
        }
        if(sideParallelOverLap.is_initialized())
            parser->subRule->setSideParallelOverlap(true);
    }
    void addParallelOverlapSubRule(boost::optional<std::string> except,
                                odb::lefTechLayerCutSpacingParser* parser)
    {
        parser->subRule->setType(odb::dbTechLayerCutSpacingSubRule::CutSpacingType::PARALLELOVERLAP);
        if(except.is_initialized())
        {
            auto exceptWhat = except.value();
            if(exceptWhat=="EXCEPTSAMENET")
                parser->subRule->setExceptSameNet(true);
            else if(exceptWhat=="EXCEPTSAMEMETAL")
                parser->subRule->setExceptSameMetal(true);
            else if(exceptWhat=="EXCEPTSAMEVIA")
                parser->subRule->setExceptSameVia(true);
        }
    }
    void addParallelWithinSubRule(boost::fusion::vector< double, boost::optional<std::string>>& params,
                                odb::lefTechLayerCutSpacingParser* parser, 
                                odb::lefin* lefin)
    {
        parser->subRule->setType(odb::dbTechLayerCutSpacingSubRule::CutSpacingType::PARALLELWITHIN);
        parser->subRule->setWithin(lefin->dbdist(at_c<0>(params)));
        auto except = at_c<1>(params);
        if(except.is_initialized())
            parser->subRule->setExceptSameNet(true);
    }
    void addSameMetalSharedEdgeSubRule(boost::fusion::vector<double,
                                                             boost::optional<std::string>,
                                                             boost::optional<std::string>,
                                                             boost::optional<std::string>,
                                                             boost::optional<int>>& params,
                                odb::lefTechLayerCutSpacingParser* parser, 
                                odb::lefin* lefin)
    {
        parser->subRule->setType(odb::dbTechLayerCutSpacingSubRule::CutSpacingType::SAMEMETALSHAREDEDGE);
        auto within = at_c<0>(params);
        auto ABOVE = at_c<1>(params);
        auto CUTCLASS = at_c<2>(params);
        auto EXCEPTTWOEDGES = at_c<3>(params);
        auto EXCEPTSAMEVIA = at_c<4>(params);
        parser->subRule->setWithin(lefin->dbdist(within));
        if(ABOVE.is_initialized())
            parser->subRule->setAbove(true);
        if(CUTCLASS.is_initialized())
        {
            auto cutClassName = CUTCLASS.value();
            parser->subRule->setCutClassValid(true);
            parser->subRule->setCutClassName(cutClassName.c_str());
        }
        if(EXCEPTTWOEDGES.is_initialized())
            parser->subRule->setExceptTwoEdges(true);
        if(EXCEPTSAMEVIA.is_initialized()){
            parser->subRule->setExceptSameVia(true);
            auto numCut = EXCEPTSAMEVIA.value();
            parser->subRule->setNumCuts(numCut);
        }
    }
    void addAreaSubRule(double value,
                        odb::lefTechLayerCutSpacingParser* parser, 
                        odb::lefin* lefin)
    {
        parser->subRule->setType(odb::dbTechLayerCutSpacingSubRule::CutSpacingType::AREA);
        parser->subRule->setCutArea(lefin->dbdist(value));
    }
    
    template <typename Iterator>
    bool parse(Iterator first, Iterator last, odb::lefTechLayerCutSpacingParser* parser, odb::dbTechLayerCutSpacingRule* rule, odb::lefin* lefin)
    {   

        qi::rule<Iterator, std::string(), ascii::space_type> _string;
        _string %= lexeme[+(char_-' ')];
        qi::rule<std::string::iterator, space_type> LAYER = (
            lit("LAYER")
            >> _string
            >> -string("STACK")
        )[boost::bind(&addLayerSubRule,_1,parser)];

        qi::rule<std::string::iterator, space_type> ADJACENTCUTS = (
            lit("ADJACENTCUTS")
            >> ( string("1") | string("2") | string("3") )
            >> -(
                lit("EXACTALIGNED")
                >> int_
            )
            >> lit("WITHIN")
            >> double_
            >> -string("EXCEPTSAMEPGNET")
            >> -(
                lit("CUTCLASS")
                >> _string
            )
            >> -string("SIDEPARALLELOVERLAP")
        )[boost::bind(&addAdjacentCutsSubRule,_1,parser,lefin)];
        
        qi::rule<std::string::iterator, space_type> PARALLELOVERLAP = (
            lit("PARALLELOVERLAP")
            >> -( string("EXCEPTSAMENET") | string("EXCEPTSAMEMETAL") | string("EXCEPTSAMEVIA"))
        )[boost::bind(&addParallelOverlapSubRule,_1,parser)];
        
         qi::rule<std::string::iterator, space_type> PARALLELWITHIN = (
            lit("PARALLELWITHIN")
            >> double_
            >> -string("EXCEPTSAMENET")
        )[boost::bind(&addParallelWithinSubRule,_1,parser,lefin)];
        
         qi::rule<std::string::iterator, space_type> SAMEMETALSHAREDEDGE = (
            lit("SAMEMETALSHAREDEDGE")
            >> double_
            >> -string("ABOVE")
            >> -(
                lit("CUTCLASS")
                >> _string
            )
            >> -string("EXCEPTTWOEDGES")
            >> -(
                lit("EXCEPTSAMEVIA")
                >> int_
            )
        )[boost::bind(&addSameMetalSharedEdgeSubRule,_1,parser,lefin)];

        qi::rule<std::string::iterator, space_type> AREA = (
            lit("AREA")
            >> double_
        )[boost::bind(&addAreaSubRule,_1,parser,lefin)];

        qi::rule<std::string::iterator, space_type> LEF58_SPACING = (
            +(
               lit("SPACING")
               >> double_[boost::bind(&setCutSpacing,_1,parser,rule, lefin)]
               >> -(
                   lit("MAXXY")[boost::bind(&addMaxXYSubRule,parser)]
                   |
                   -lit("CENTERTOCENTER")[boost::bind(&setCenterToCenter,parser)]
                   >> -(
                       lit("SAMENET")[boost::bind(&setSameNet,parser)]
                       |
                       lit("SAMEMETAL")[boost::bind(&setSameMetal,parser)]
                       |
                       lit("SAMEVIA")[boost::bind(&setSameVia,parser)]
                   )
                   >> -(LAYER)
               )
               >> lit(";")
            )
        );

        bool r = qi::phrase_parse(first, last, LEF58_SPACING, space);
        

        if (first != last) // fail if we did not get a full match
            return false;
        return r;
    }
}


namespace odb{


dbTechLayerCutSpacingRule* lefTechLayerCutSpacingParser::parse(std::string s, dbTechLayer* layer, odb::lefin* l)
{
    odb::dbTechLayerCutSpacingRule* rule = odb::dbTechLayerCutSpacingRule::create(layer);
    if(lefTechLayerCutSpacing::parse(s.begin(), s.end(), this, rule, l) ){
        subRule = nullptr;
        return rule;
    }
    else 
    {
        odb::dbTechLayerCutSpacingRule::destroy(rule);
        return nullptr;
    }
} 


}

