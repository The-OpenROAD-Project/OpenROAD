

// COMMENTS:
//
// Presently only handles random moves and swaps.


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <stack>
#include <utility>
#include <cmath>
#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
#include "utility.h"
#include "plotgnu.h"
// For detailed improvement.
#include "detailed_random.h"
#include "detailed_segment.h"
#include "detailed_manager.h"
#include "detailed_orient.h"
// Detailed placement objectives.
#include "detailed_objective.h"
#include "detailed_hpwl.h"
#include "detailed_displacement.h"
#include "detailed_abu.h"
#include "detailed_vertical.h"
#include "detailed_global.h"
//#include "detailed_drc.h"
//#include "detailed_abu.h"
//#include "detailed_pin.h"
// Algorithms.
//#include "detailed_global_vertical.h"
//#include "detailed_reorder.h"
//#include "detailed_lillis.h"

#include "utility.h"


////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

const int       MAX_MOVE_ATTEMPTS               = 5;


namespace aak
{

bool isOperator( char ch )
{
    if( ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^' )
        return true;
    return false;
}

bool isObjective( char ch )
{
    if( ch >= 'a' && ch <= 'z' )
        return true;
    return false;
}

bool isNumber( char ch )
{
    if( ch >= '0' && ch <= '9' )
        return true;
    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedRandom::DetailedRandom( Architecture* arch, Network* network, RoutingParams* rt ):
    m_arch( arch ),
    m_network( network ),
    m_rt( rt ),
    m_movesPerCandidate( 3.0 )
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedRandom::~DetailedRandom( void )
{
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedRandom::run( DetailedMgr* mgrPtr, std::string command )
{
    // A temporary interface to allow for a string which we will decode to create
    // the arguments.
    std::string scriptString = command;
    boost::char_separator<char> separators( " \r\t\n;" );
    boost::tokenizer<boost::char_separator<char> > tokens( scriptString, separators );
    std::vector<std::string> args;
    for( boost::tokenizer<boost::char_separator<char> >::iterator it = tokens.begin();
            it != tokens.end();
            it++ )
    {
        args.push_back( *it );
    }
    run( mgrPtr, args );
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedRandom::run( DetailedMgr* mgrPtr, std::vector<std::string>& args )
{
    // This is, more or less, a greedy or low temperature anneal.  It is capable
    // of handling very complex objectives, etc.  There should be a lot of 
    // arguments provided actually.  But, right now, I am just getting started.

    m_mgrPtr = mgrPtr;

    std::string generatorStr = "";
    std::string objectiveStr = "";
    std::string costStr = "";
    m_movesPerCandidate = 3.0;
    int passes = 1;
    double tol = 0.01;
    for( size_t i = 1; i < args.size(); i++ )
    {
        if( args[i] == "-f" && i+1<args.size() )
        {
            m_movesPerCandidate = std::atof( args[++i].c_str() );
        }
        else if( args[i] == "-p" && i+1<args.size() )
        {
            passes = std::atoi( args[++i].c_str() );
        }
        else if( args[i] == "-t" && i+1<args.size() )
        {
            tol = std::atof( args[++i].c_str() );
        }
        else if( args[i] == "-gen" && i+1<args.size() )
        {
            generatorStr = args[++i];
        }
        else if( args[i] == "-obj" && i+1<args.size() )
        {
            objectiveStr = args[++i];
        }
        else if( args[i] == "-cost" && i+1<args.size() )
        {
            costStr = args[++i];
        }
    }
    tol = std::max( tol, 0.01 );
    passes = std::max( passes, 1 );


    // Generators.
    for( size_t i = 0; i < m_generators.size(); i++ )
    {
        delete m_generators[i];
    }
    m_generators.clear();


    // Additional generators per the command. XXX: Need to write the code for these objects;
    // just a concept now.
    if( generatorStr != "" )
    {
        std::cout << "Generator string: " << generatorStr.c_str() << std::endl;

        boost::char_separator<char> separators( " \r\t\n:" );
        boost::tokenizer<boost::char_separator<char> > tokens( generatorStr, separators );
        std::vector<std::string> gens;
        for( boost::tokenizer<boost::char_separator<char> >::iterator it = tokens.begin();
                it != tokens.end();
                it++ )
        {
            gens.push_back( *it );
        }

        for( size_t i = 0; i < gens.size(); i++ )
        {
            //if( gens[i] == "ro" )       std::cout << "reorder generator requested." << std::endl;
            //else if( gens[i] == "mis" ) std::cout << "set matching generator requested." << std::endl;
            if( gens[i] == "gs" )
            {
                std::cout << "global swap generator requested." << std::endl;
                m_generators.push_back( new DetailedGlobalSwap() );
            }
            else if( gens[i] == "vs" )
            {
                std::cout << "vertical swap generator requested." << std::endl;
                m_generators.push_back( new DetailedVerticalSwap() );
            }
            else if( gens[i] == "rng" )
            {
                std::cout << "random generator requested." << std::endl;
                m_generators.push_back( new RandomGenerator() );
            }
            else if( gens[i] == "disp" )
            {
                std::cout << "displacement generator requested." << std::endl;
                m_generators.push_back( new DisplacementGenerator() );
            }
            else std::cout << "unknown generator requested." << std::endl;
        }
    }
    if( m_generators.size() == 0 )
    {
        // Default generator.
        m_generators.push_back( new RandomGenerator() );
    }
    for( size_t i = 0; i < m_generators.size(); i++ )
    {
        m_generators[i]->init( m_mgrPtr );
    }

    // Objectives.
    for( size_t i = 0; i < m_objectives.size(); i++ )
    {
        delete m_objectives[i];
    }
    m_objectives.clear();


    // Additional objectives per the command. XXX: Need to write the code for these objects;
    // just a concept now.
    if( objectiveStr != "" )
    {
        std::cout << "Objective string: " << objectiveStr.c_str() << std::endl;

        boost::char_separator<char> separators( " \r\t\n:" );
        boost::tokenizer<boost::char_separator<char> > tokens( objectiveStr, separators );
        std::vector<std::string> objs;
        for( boost::tokenizer<boost::char_separator<char> >::iterator it = tokens.begin();
                it != tokens.end();
                it++ )
        {
            objs.push_back( *it );
        }

        for( size_t i = 0; i < objs.size(); i++ )
        {
            //else if( objs[i] == "drc" )   std::cout << "drc objective requested." << std::endl;
            if( objs[i] == "abu" ) 
            {
                std::cout << "abu metric objective requested." << std::endl;
                DetailedABU* objABU = new DetailedABU( m_arch, m_network, m_rt );
                objABU->init( m_mgrPtr, NULL );
                m_objectives.push_back( objABU );
            }
            else if( objs[i] == "disp" )
            {
                std::cout << "displacement objective requested." << std::endl;
                DetailedDisplacement* objDisp = new DetailedDisplacement( m_arch, m_network, m_rt );
                objDisp->init( m_mgrPtr, NULL );
                m_objectives.push_back( objDisp );
            }
            else if( objs[i] == "hpwl" ) 
            {
                std::cout << "wirelength objective requested." << std::endl;
                DetailedHPWL* objHpwl = new DetailedHPWL( m_arch, m_network, m_rt );
                objHpwl->init( m_mgrPtr, NULL );
                m_objectives.push_back( objHpwl );
            }
            else std::cout << "unknown objective requested." << std::endl;
        }
    }
    if( m_objectives.size() == 0 )
    {
        // Default objective.
        DetailedHPWL* objHpwl = new DetailedHPWL( m_arch, m_network, m_rt );
        objHpwl->init( m_mgrPtr, NULL );
        m_objectives.push_back( objHpwl );
    }

    for( size_t i = 0; i < m_objectives.size(); i++ ) 
    {
        std::cout << boost::format( "Objective: id: %2d, name: %s" ) % i % m_objectives[i]->getName().c_str() << std::endl;
    }


    // Should I just be figuring out the objectives needed from the cost string?
    if( costStr != "" )
    {
        // XXX: Work in progress to make things more generic...
        std::cout << "Cost string: " << costStr << std::endl;

        //costStr.erase( std::remove( costStr.begin(), costStr.end(), '(' ), costStr.end() );
        //costStr.erase( std::remove( costStr.begin(), costStr.end(), ')' ), costStr.end() );

        std::cout << "Cost string: " << costStr << std::endl;

        // Replace substrings of objectives with a number.
        for( size_t i = m_objectives.size(); i > 0; )
        {
            --i;
            for(;;) 
            {
                size_t pos = costStr.find( m_objectives[i]->getName() );
                if( pos == std::string::npos )
                {
                    break;
                }
                std::cout << "Objective '" << m_objectives[i]->getName() << "'" << ", "
                    << "Index is " << i << ", "
                    << "Found at " << pos << std::endl;
         
                std::string val;
                val.append( 1, (char)('a'+i) );
                costStr.replace( pos, m_objectives[i]->getName().length(), val );
            }
        }

        std::cout << "Modified cost string: " << costStr << std::endl;

        m_expr.clear();
        for( std::string::iterator it = costStr.begin(); it != costStr.end(); ++it )
        {
            if( *it == '(' || *it == ')' )
            {
            }
            else if( isOperator( *it ) || isObjective( *it ) )
            {
                m_expr.push_back( std::string( 1, *it ) );
            }
            else
            {
                std::string val;
                while( !isOperator( *it ) && !isObjective( *it ) && it != costStr.end() && *it != '(' && *it != ')' )
                {
                    val.append( 1, *it );
                    ++it;
                }
                m_expr.push_back( val );
                --it;
            }
        }
    }
    else
    {
        m_expr.clear();
        m_expr.push_back( std::string( 1, 'a' ) );
        for( size_t i = 1; i < m_objectives.size(); i++ )
        {
            m_expr.push_back( std::string( 1, (char)('a'+i) ) );
            m_expr.push_back( std::string( 1, '+') );
        }
    }

    {
        std::cout << "cost stack: " << std::endl;
        for( size_t i = 0; i < m_expr.size(); i++ )
        {
            std::cout << m_expr[i].c_str() << std::endl;
        }
    }

    double curr_hpwl, init_hpwl, hpwl_x, hpwl_y;

    init_hpwl = Utility::hpwl( m_network, hpwl_x, hpwl_y );
    for( int p = 1; p <= passes; p++ )
    {
        m_mgrPtr->resortSegments(); // Needed?
        double change = go();
        std::cout << "Pass " << p << " of greedy improvement, Impr is " << change*100. << "%%" << std::endl;
        if( change < tol )
        {
            break;
        }
    }
    m_mgrPtr->resortSegments(); // Needed?


    curr_hpwl = Utility::hpwl( m_network, hpwl_x, hpwl_y );
    std::cout << boost::format( "End of passes for random; hpwl is %.6e, total imp is %.2lf%%\n" )
        % curr_hpwl % (((init_hpwl-curr_hpwl)/init_hpwl)*100.) ;

    // Cleanup.
    for( size_t i = 0; i < m_generators.size(); i++ )
    {
        delete m_generators[i];
    }
    m_generators.clear();
    for( size_t i = 0; i < m_objectives.size(); i++ )
    {
        delete m_objectives[i];
    }
    m_objectives.clear();
}


double doOperation( double a, double b, char op )
{
    switch( op )
    {
    case '+': return b+a; break;
    case '-': return b-a; break;
    case '*': return b*a; break;
    case '/': return b/a; break;
    case '^': return std::pow(b,a); break;
    default: break;
    }
    return 0.0;
}

double eval( std::vector<double>& costs, std::vector<std::string>& expr )
{
    std::stack<double> stk;
    for( size_t i = 0; i < expr.size(); i++ )
    {
        std::string& val = expr[i];
        if( isOperator( val[0] ) )
        {
            double a = stk.top();
            stk.pop();
            double b = stk.top();
            stk.pop();
            stk.push( doOperation( a, b, val[0] ) );
        }
        else if( isObjective( val[0] ) )
        {
            stk.push( costs[(int)(val[0]-'a')] );
        }
        else
        {
            // Assume number.
            stk.push( std::stod(val) );
        }
    }
    if( stk.size() != 1 )
    {
        // Error.
        std::cout << "Error." << std::endl;
        exit(-1);
    }
    return stk.top();
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
double DetailedRandom::go( void )
{
    if( m_generators.size() == 0 )
    {
        std::cout << "Improver use of algorithm; requires at least one generator." << std::endl;
        return 0.0;
    }

    // Collect candidate cells.
    collectCandidates();

    // Try to improve.
    int maxAttempts = (int)std::ceil( m_movesPerCandidate * (double)m_candidates.size() );
    Node* ndi = 0;
    Node* ndj = 0;
    Utility::random_shuffle( m_candidates.begin(), m_candidates.end(), m_mgrPtr->m_rng );

    m_deltaCost.resize( m_objectives.size() );
    m_initCost.resize( m_objectives.size() );
    m_currCost.resize( m_objectives.size() );
    m_nextCost.resize( m_objectives.size() );
    for( size_t i = 0; i < m_objectives.size(); i++ )
    {
        m_deltaCost[i] = 0.;
        m_initCost[i] = m_objectives[i]->curr();
        m_currCost[i] = m_initCost[i];
        m_nextCost[i] = m_initCost[i];

        if( m_objectives[i]->getName() == "abu" )
        {
            DetailedABU* ptr = dynamic_cast<DetailedABU*>(m_objectives[i]);
            if( ptr != 0 )
            {
                ptr->measureABU( true );
            }
        }
    }

    // Test.
    std::cout << "Test evaluation: " << eval( m_currCost, m_expr ) << std::endl;

    double currTotalCost;
    double initTotalCost;
    double nextTotalCost;
    initTotalCost = eval( m_currCost, m_expr );
    currTotalCost = initTotalCost;
    nextTotalCost = initTotalCost;

    std::vector<int> gen_count( m_generators.size() );
    std::fill( gen_count.begin(), gen_count.end(), 0 );
    for( int attempt = 0; attempt < maxAttempts; attempt++ )
    {
        // Pick a generator at random.
        int g = (*(m_mgrPtr->m_rng))() % (m_generators.size());
        ++gen_count[g];
        // Generate a move list.
        if( m_generators[g]->generate( m_mgrPtr, m_candidates ) == false )
        {
            // Failed to generate anything so just move on to the next attempt.
            continue;
        }

        // The generator has provided a successful move which is stored in the
        // manager.  We need to evaluate that move to see if we should accept
        // or reject it.  Scan over the objective functions and use the move
        // information to compute the weighted deltas; an overall weighted delta
        // better than zero implies improvement.
        for( size_t i = 0; i < m_objectives.size(); i++ )
        {
            // XXX: NEED TO WEIGHT EACH OBJECTIVE!
            double change = m_objectives[i]->delta( m_mgrPtr->m_nMoved, 
                            m_mgrPtr->m_movedNodes, 
                            m_mgrPtr->m_curX, m_mgrPtr->m_curY, 
                            m_mgrPtr->m_curOri, 
                            m_mgrPtr->m_newX, m_mgrPtr->m_newY, 
                            m_mgrPtr->m_newOri ); 

            m_deltaCost[i] = change;
            m_nextCost[i]  = m_currCost[i] - m_deltaCost[i]; // -delta is +ve is less.
        }
        nextTotalCost = eval( m_nextCost, m_expr );

//        std::cout << boost::format( "Move consisting of %d cells generated benefit of %.2lf; Will %s.\n" )
//            % m_mgrPtr->m_nMoved % delta % ((delta>0.)?"accept":"reject");

//        if( delta > 0.0 )
        if( nextTotalCost <= currTotalCost )
        {
            m_mgrPtr->acceptMove();
            for( size_t i = 0; i < m_objectives.size(); i++ )
            {
                m_objectives[i]->accept();
            }

            // A great, but time-consuming, check here is to recompute the costs from
            // scratch and make sure they are the same as the incrementally computed
            // costs.  Very useful for debugging!  Could do this check ever so often
            // or just at the end...
            ;
            for( size_t i = 0; i < m_objectives.size(); i++ )
            {
                //if( m_objectives[i]->getName() == "abu" )
                //{
                //    double temp = m_objectives[i]->curr();
                //    std::cout << boost::format( "ABU: curr %.6e, delta %.6e, next %.6e, scratch %.6e, diff? %s" )
                //        % m_currCost[i] % m_deltaCost[i] % m_nextCost[i]  
                //        % temp % ( (std::fabs(temp-m_nextCost[i])>1.e-3) ? "Y" : "N" )
                //        << std::endl;
                //}

                m_currCost[i] = m_nextCost[i];
            }
            currTotalCost = nextTotalCost;
        }
        else
        {
            m_mgrPtr->rejectMove();
            for( size_t i = 0; i < m_objectives.size(); i++ )
            {
                m_objectives[i]->reject();
            }
        }
    }
    std::cout << "Generator usage:";
    for( size_t i = 0; i < gen_count.size(); i++ )
    {
        std::cout << " " << gen_count[i];
    }
    std::cout << std::endl;
    std::cout << "Generator stats:" << std::endl;
    for( size_t i = 0; i < m_generators.size(); i++ )
    {
        m_generators[i]->stats();
    }

    std::cout << "Cost check:" << std::endl;
    for( size_t i = 0; i < m_objectives.size(); i++ )
    {
        double scratch = m_objectives[i]->curr();
        m_nextCost[i] = scratch; // Temporary.
        bool error = (std::fabs(scratch - m_currCost[i]) > 1.0e-3 );
        std::cout << boost::format( "Objective %2d, %10s: Init %.2lf, Curr(Scratch): %.2lf, Incr: %.2lf, %s" )
           % i 
           % m_objectives[i]->getName().c_str() 
           % m_initCost[i] 
           % scratch
           % m_currCost[i] 
           % (error?"MISMATCH":"OKAY")
           << std::endl;

        if( m_objectives[i]->getName() == "abu" )
        {
            DetailedABU* ptr = dynamic_cast<DetailedABU*>(m_objectives[i]);
            if( ptr != 0 )
            {
                ptr->measureABU( true );
            }
        }
    }
    nextTotalCost = eval( m_nextCost, m_expr );
    std::cout << boost::format( "Total cost(Scratch): %.2lf, Incr: %.2lf" )
        % eval( m_nextCost, m_expr ) % currTotalCost << std::endl;

    return ((initTotalCost - currTotalCost) / initTotalCost);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedRandom::collectCandidates( void )
{
    m_candidates.erase( m_candidates.begin(), m_candidates.end() );
    m_candidates.insert( m_candidates.end(), 
            m_mgrPtr->m_singleHeightCells.begin(), m_mgrPtr->m_singleHeightCells.end() );
    for( size_t i = 2; i < m_mgrPtr->m_multiHeightCells.size(); i++ )
    {
        m_candidates.insert( m_candidates.end(),
            m_mgrPtr->m_multiHeightCells[i].begin(), m_mgrPtr->m_multiHeightCells[i].end() );
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
RandomGenerator::RandomGenerator( void ):DetailedGenerator()
{
    std::cout << "Allocated a random move generator." << std::endl;

    m_attempts = 0;
    m_moves = 0;
    m_swaps = 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
RandomGenerator::~RandomGenerator( void )
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool RandomGenerator::generate( DetailedMgr* mgr, std::vector<Node*>& candidates )
{
    ++m_attempts;

    m_mgr = mgr;
    m_arch = mgr->getArchitecture();
    m_network = mgr->getNetwork();
    m_rt = mgr->getRoutingParams();

    double  ywid = m_mgr->getSingleRowHeight();
    int     ydim = m_mgr->getNumSingleHeightRows();
    double  xwid = m_arch->m_rows[0]->m_siteSpacing;
    int     xdim = std::max(0,(int)((m_arch->getMaxX() - m_arch->getMinX())/xwid));

    xwid = (m_arch->getMaxX() - m_arch->getMinX())/(double)xdim;
    ywid = (m_arch->getMaxY() - m_arch->getMinY())/(double)ydim;

    Node* ndi = candidates[ (*(m_mgr->m_rng))() % (candidates.size()) ];
    int spanned_i = (int)(ndi->getHeight()/m_mgr->getSingleRowHeight() + 0.5);
    if( spanned_i != 1 )
    {
        return false;
    }
    // Segments for the source.
    std::vector<DetailedSeg*>& segs_i = m_mgr->m_reverseCellToSegs[ndi->getId()];
    // Only working with single height cells right now.
    if( segs_i.size() != 1 )
    {
        std::cout << "Error." << std::endl;
        exit(-1);
    }

    double xi, yi;
    double xj, yj;
    int si; // Row and segment of source.
    int rj, sj; // Row and segment of destination.
    int grid_xi, grid_yi;
    int grid_xj, grid_yj;
    // For the window size.  This should be parameterized.
    int rly = 10;
    int rlx = 10;
    int rel_x, rel_y;
    bool is_move_okay;
    bool is_swap_okay;

    const int tries = 5;
    for( int t = 1; t <= tries; t++ )
    {
        // Position of the source.
        xi      = ndi->getX();
        yi      = ndi->getY();

        // Segment for the source.
        si = segs_i[0]->m_segId;

        // Random position within a box centered about (xi,yi).
        grid_xi = std::min(xdim-1,std::max(0,(int)((xi - m_arch->getMinX())/xwid)));
        grid_yi = std::min(ydim-1,std::max(0,(int)((yi - m_arch->getMinY())/ywid)));

        rel_x = (*(m_mgr->m_rng))() % (2 * rlx + 1);
        rel_y = (*(m_mgr->m_rng))() % (2 * rly + 1);

        grid_xj = std::min(xdim-1,std::max(0,(grid_xi - rlx + rel_x)));
        grid_yj = std::min(ydim-1,std::max(0,(grid_yi - rly + rel_y)));

        // Position of the destination.
        xj = m_arch->getMinX() + grid_xj * xwid;
        yj = m_arch->getMinY() + grid_yj * ywid + 0.5 * ndi->getHeight();

        // Row and segment for the destination.
        rj = (int)((yj - m_arch->getMinY())/m_mgr->getSingleRowHeight());
        rj = std::min( m_mgr->getNumSingleHeightRows()-1, std::max( 0, rj ) );
        sj = -1;
        for( int s = 0; s < m_mgr->m_segsInRow[rj].size(); s++ )
        {
            DetailedSeg* segPtr = m_mgr->m_segsInRow[rj][s];
            if(  xj >= segPtr->m_xmin && xj <= segPtr->m_xmax )
            {
                sj = segPtr->m_segId;
                break;
            }
        }

        // Need to determine validity of things.
        if( sj == -1 || ndi->getRegionId() != m_mgr->m_segments[sj]->getRegId() )
        {
            // The target segment cannot support the candidate cell.
            continue;
        }

        // Try to generate a move or a swap.  The result is stored in the manager.
        is_move_okay = false;
        is_swap_okay = false;
        (void)is_swap_okay;

        if( !is_move_okay )
        {
            if( si != sj )
            {
                if( m_mgr->tryMove1( ndi, xi, yi, si, xj, yj, sj ) == true )
                {
                    is_move_okay = true;
                }
            }
            else
            {
                if( m_mgr->tryMove2( ndi, xi, yi, si, xj, yj, sj ) == true )
                {
                    is_move_okay = true;
                }
            }
        }

        if( is_move_okay )
        {
            ++m_moves;
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void RandomGenerator::stats( void )
{
    std::cout << "Random generator, attempts " << m_attempts << ", "
        << "moves " << m_moves << ", "
        << "swaps " << m_swaps 
        << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DisplacementGenerator::DisplacementGenerator( void ):DetailedGenerator()
{
    std::cout << "Allocated a displacement move generator." << std::endl;

    m_attempts = 0;
    m_moves = 0;
    m_swaps = 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DisplacementGenerator::~DisplacementGenerator( void )
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DisplacementGenerator::generate( DetailedMgr* mgr, std::vector<Node*>& candidates )
{
    ++m_attempts;

    m_mgr = mgr;
    m_arch = mgr->getArchitecture();
    m_network = mgr->getNetwork();
    m_rt = mgr->getRoutingParams();

    double  ywid = m_mgr->getSingleRowHeight();
    int     ydim = m_mgr->getNumSingleHeightRows();
    double  xwid = m_arch->m_rows[0]->m_siteSpacing;
    int     xdim = std::max(0,(int)((m_arch->getMaxX() - m_arch->getMinX())/xwid));

    xwid = (m_arch->getMaxX() - m_arch->getMinX())/(double)xdim;
    ywid = (m_arch->getMaxY() - m_arch->getMinY())/(double)ydim;

    Node* ndi = candidates[ (*(m_mgr->m_rng))() % (candidates.size()) ];
    int spanned_i = (int)(ndi->getHeight()/m_mgr->getSingleRowHeight() + 0.5);
    int spanned_j = -1;

    // Segments for the source.
    std::vector<DetailedSeg*>& segs_i = m_mgr->m_reverseCellToSegs[ndi->getId()];

    double xi, yi;
    double xj, yj;
    int si; // Row and segment of source.
    int rj, sj; // Row and segment of destination.
    int grid_xi, grid_yi;
    int grid_xj, grid_yj;
    // For the window size.  This should be parameterized.
    int rly = 5;
    int rlx = 5;
    int rel_x, rel_y;
    bool is_move_okay;
    bool is_swap_okay;
    std::vector<Node*>::iterator it_j;

    const int tries = 5;
    for( int t = 1; t <= tries; t++ )
    {
        // Position of the source.
        xi      = ndi->getX();
        yi      = ndi->getY();

        // Segment for the source.
        si = segs_i[0]->m_segId;

        // Choices: (i) random position within a box centered at the original
        // position; (ii) random position within a box between the current
        // and original position; (iii) the original position itself.  Should
        // this also be a randomized choice??????????????????????????????????
        if( 1 )
        {
            // Centered at the original position within a box.
            grid_xi = std::min(xdim-1,std::max(0,(int)((ndi->getOrigX() - m_arch->getMinX())/xwid)));
            grid_yi = std::min(ydim-1,std::max(0,(int)((ndi->getOrigY() - m_arch->getMinY())/ywid)));

            rel_x = (*(m_mgr->m_rng))() % (2 * rlx + 1);
            rel_y = (*(m_mgr->m_rng))() % (2 * rly + 1);

            grid_xj = std::min(xdim-1,std::max(0,(grid_xi - rlx + rel_x)));
            grid_yj = std::min(ydim-1,std::max(0,(grid_yi - rly + rel_y)));

            xj = m_arch->getMinX() + grid_xj * xwid;
            yj = m_arch->getMinY() + grid_yj * ywid + 0.5 * ndi->getHeight();
        }
        if( 0 )
        {
            // The original position.
            xj = ndi->getOrigX();
            yj = ndi->getOrigY();
        }
        if( 0 )
        {
            // Somewhere between current position and original position.
            grid_xi = std::min(xdim-1,std::max(0,(int)((ndi->getX()     - m_arch->getMinX())/xwid)));
            grid_yi = std::min(ydim-1,std::max(0,(int)((ndi->getY()     - m_arch->getMinY())/ywid)));

            grid_xj = std::min(xdim-1,std::max(0,(int)((ndi->getOrigX() - m_arch->getMinX())/xwid)));
            grid_yj = std::min(ydim-1,std::max(0,(int)((ndi->getOrigY() - m_arch->getMinY())/ywid)));

            if( grid_xi > grid_xj ) std::swap(grid_xi,grid_xj);
            if( grid_yi > grid_yj ) std::swap(grid_yi,grid_yj);

            int w = grid_xj-grid_xi;
            int h = grid_yj-grid_yi;

            rel_x = (*(m_mgr->m_rng))() % (w + 1); 
            rel_y = (*(m_mgr->m_rng))() % (h + 1);

            grid_xj = std::min(xdim-1,std::max(0,(grid_xi + rel_x)));
            grid_yj = std::min(ydim-1,std::max(0,(grid_yi + rel_y)));

            xj = m_arch->getMinX() + grid_xj * xwid;
            yj = m_arch->getMinY() + grid_yj * ywid + 0.5 * ndi->getHeight();
        }


        // Row and segment for the destination.
        rj = (int)((yj - m_arch->getMinY())/m_mgr->getSingleRowHeight());
        rj = std::min( m_mgr->getNumSingleHeightRows()-1, std::max( 0, rj ) );
        sj = -1;
        for( int s = 0; s < m_mgr->m_segsInRow[rj].size(); s++ )
        {
            DetailedSeg* segPtr = m_mgr->m_segsInRow[rj][s];
            if(  xj >= segPtr->m_xmin && xj <= segPtr->m_xmax )
            {
                sj = segPtr->m_segId;
                break;
            }
        }

        // Need to determine validity of things.
        if( sj == -1 || ndi->getRegionId() != m_mgr->m_segments[sj]->getRegId() )
        {
            // The target segment cannot support the candidate cell.
            continue;
        }

        // Try to generate a move or a swap.  The result is stored in the manager.
        is_move_okay = false;
        is_swap_okay = false;
        (void)is_swap_okay;

        if( !is_move_okay )
        {
            if( spanned_i != 1 )
            {
                if( m_mgr->tryMove3( ndi, xi, yi, si, xj, yj, sj ) == true )
                {
                    is_move_okay = true;
                }
            }
            else
            {
                if( si != sj )
                {
                    if( m_mgr->tryMove1( ndi, xi, yi, si, xj, yj, sj ) == true )
                    {
                        is_move_okay = true;
                    }
                }
                else
                {
                    if( m_mgr->tryMove2( ndi, xi, yi, si, xj, yj, sj ) == true )
                    {
                        is_move_okay = true;
                    }
                }
            }
        }
        if( !is_move_okay )
        {
            if( spanned_i != 1 )
            {
            }
            else
            {
                if( m_mgr->trySwap1( ndi, xi, yi, si, xj, yj, sj ) == true )
                {
                    is_swap_okay = true;
                }
            }
        }

        if( is_move_okay )
        {
            ++m_moves;
            return true;
        }
        if( is_swap_okay )
        {
            ++m_swaps;
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DisplacementGenerator::stats( void )
{
    std::cout << "Displacement generator, attempts " << m_attempts << ", "
        << "moves " << m_moves << ", "
        << "swaps " << m_swaps 
        << std::endl;
}


} // namespace aak

