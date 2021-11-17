

// A partial implementation of the paper Kris and I wrote a long time ago on
// using whitespace management for legalization.

//////////////////////////////////////////////////////////////////////////////
// Includes.
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <iostream>
#include <vector>
#include <deque>
#include <set>
#include <boost/format.hpp>
#include "tbb/tick_count.h"
#include "mer.h"
#include "legalize_dwsm.h"
#include "min_movement_floorplanner.h"
#include "detailed_segment.h"
#include "detailed_manager.h"


namespace aak
{

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
DwsmLegalizer::Block::Block( Node* ndi ):
    m_node( ndi ),
    m_parent( 0 ),
    m_id( -1 ),
    m_isPlaced( false ),
    m_isFixed( false )
{
    m_xmin = ndi->getX() - 0.5 * ndi->getWidth() ;
    m_xmax = ndi->getX() + 0.5 * ndi->getWidth() ;
    m_ymin = ndi->getY() - 0.5 * ndi->getHeight();
    m_ymax = ndi->getY() + 0.5 * ndi->getHeight();

    // Or should this be set to ndi->getOrigX() and ndi->getOrigY() ???
    //m_initialX = ndi->getX();
    //m_initialY = ndi->getY();
    m_initialX = ndi->getOrigX();
    m_initialY = ndi->getOrigY();
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
DwsmLegalizer::Block::Block( double xmin, double xmax, double ymin, double ymax ):
    m_node( 0 ),
    m_parent( 0 ),
    m_id( -1 ),
    m_isPlaced( false ),
    m_isFixed( false ),
    m_xmin( xmin ),
    m_xmax( xmax ),
    m_ymin( ymin ),
    m_ymax( ymax )
{
    m_initialX = 0.5 * (m_xmin + m_xmax);
    m_initialY = 0.5 * (m_ymin + m_ymax);
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
DwsmLegalizer::Block::~Block( void )
{
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
DwsmLegalizer::DwsmLegalizer( DwsmLegalizerParams& params ):
    m_params( params ),
    m_mgr( 0 ),
    m_arch( 0 ),
    m_network( 0 ),
    m_rt( 0 ),
    m_root( 0 )
{
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
DwsmLegalizer::~DwsmLegalizer( void )
{
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int DwsmLegalizer::legalize( DetailedMgr& mgr )
{
    // Use DWSM to legalize _ALL_ regions.
    std::cout << "Dwsm legalizer." << std::endl;

    m_mgr = &mgr;

    m_arch = m_mgr->getArchitecture();
    m_network = m_mgr->getNetwork();
    m_rt = m_mgr->getRoutingParams();

    for( int regId = m_arch->m_regions.size(); regId > 0; )
    {
        --regId;
        legalize( mgr, regId );
    }
    return 0;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int DwsmLegalizer::legalize( DetailedMgr& mgr, int regId )
{
    std::cout << "\nDwsm legalizer, region is " << regId << std::endl;

    m_mgr = &mgr;

    m_arch = m_mgr->getArchitecture();
    m_network = m_mgr->getNetwork();
    m_rt = m_mgr->getRoutingParams();

    m_singleRowHeight = m_arch->m_rows[0]->getH();
    m_numSingleHeightRows = m_arch->m_rows.size();
    m_siteSpacing = m_arch->m_rows[0]->m_siteSpacing;

    m_curReg = regId;

    // Record original positions.
    m_origPosX.resize( m_network->m_nodes.size() );
    m_origPosY.resize( m_network->m_nodes.size() );
    std::fill( m_origPosX.begin(), m_origPosX.end(), 0.0 );
    std::fill( m_origPosY.begin(), m_origPosY.end(), 0.0 );
    for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node* ndi = &(m_network->m_nodes[i]);
        m_origPosX[ ndi->getId() ] = ndi->getX();
        m_origPosY[ ndi->getId() ] = ndi->getY();
    }



    // Set things up.
    init();

//    {
//        char buf[256];
//        sprintf( &buf[0], "tetrisroot%d", m_curReg );
//        draw( m_root, buf );
//    }

    int unplaced = run( m_root, 1 );

    int check = 0;
    for( size_t i = 0; i < m_root->m_blocks.size(); i++ )
    {
        Block* ptri = m_root->m_blocks[i];
        if( ptri->m_isFixed )
        {
            continue;
        }
        if( !ptri->m_isPlaced )
        {
            ++check;
        }
    }

    // Update cell positions from the block placement.
    for( size_t i = 0; i < m_root->m_blocks.size(); i++ )
    {
        Block* ptri = m_root->m_blocks[i];

        Node* ndi = ptri->m_node;
        if( ndi == 0 )
        {
            continue;
        }

        double x = 0.5 * ( ptri->getXmin() + ptri->getXmax() );
        double y = 0.5 * ( ptri->getYmin() + ptri->getYmax() );
        if( ndi->getFixed() != NodeFixed_NOT_FIXED )
        {
            // Fixed node, debug check.
            double dx = std::fabs( ndi->getX() - x );
            double dy = std::fabs( ndi->getY() - y );
            if( dx > 1.0e-3 || dy > 1.0e-3 )
            {
                ERRMSG( "Error, Unexpected movement of a fixed block." );
                exit(-1);
            }
            continue;
        }
        ndi->setX( x );
        ndi->setY( y );
    }


    // Measure displacement.
    double max_disp = 0.;
    double tot_disp = 0.;
    double avg_disp = 0.;
    int count = 0;
    for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node* ndi = &(m_network->m_nodes[i]);
        if( ndi->getRegionId() != m_curReg )
        {
            continue;
        }

        // Skip terminals.
        if( ndi->getType() == NodeType_TERMINAL || ndi->getType() == NodeType_TERMINAL_NI )
        {
            continue;
        }

        if( ndi->getFixed() != NodeFixed_NOT_FIXED )
        {
            continue;
        }

        ++count;
        double dx = std::fabs( ndi->getX() - m_origPosX[ndi->getId()] );
        double dy = std::fabs( ndi->getY() - m_origPosY[ndi->getId()] );
        max_disp = std::max( max_disp, dx + dy );
        tot_disp += dx + dy;
    }
    if( count != 0 ) avg_disp = tot_disp / (double)count;
    (void) avg_disp;

    //std::cout << "Running dwsm on region " << m_curReg << ", "
    //    << "Unplaced cells is " << unplaced << "/" << check << ", "
    //    << "Max disp is " << max_disp << ", Tot disp is " << tot_disp << ", Avg disp is " << avg_disp << std::endl;

    cleanup();

    return 0;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DwsmLegalizer::cleanup( void )
{

    if( m_root != 0 )
    {
        std::deque<Block*> Q;
        std::set<Block*> block;
        Q.push_back( m_root );
        while( !Q.empty() ) 
        {
            Block* curr = Q.front();
            Q.pop_front();
            for( size_t i = 0; i < curr->m_blocks.size(); i++ ) 
            {
                Q.push_back( curr->m_blocks[i] );
            }
            block.insert( curr );
        }
        for( std::set<Block*>::iterator site = block.begin(); 
                site != block.end();
                ++site )
        {
            delete *site;
        }
        m_root = 0;
    }
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DwsmLegalizer::init( void )
{
    cleanup();

    Block* ptr = 0;

    double xmin = m_arch->m_regions[m_curReg]->m_xmin;
    double xmax = m_arch->m_regions[m_curReg]->m_xmax;
    double ymin = m_arch->m_regions[m_curReg]->m_ymin;
    double ymax = m_arch->m_regions[m_curReg]->m_ymax;


    m_root = new Block( xmin, xmax, ymin, ymax );

    // Create a block for each cell in the region.  This includes both
    // movable and fixed cells.  We mark the blocks are required.
    int nBlocks1 = 0;
    for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node* ndi = &(m_network->m_nodes[i]);
        if( ndi->getRegionId() != m_curReg )
        {
            continue;
        }

        // Skip terminals.
        if( ndi->getType() == NodeType_TERMINAL || ndi->getType() == NodeType_TERMINAL_NI )
        {
            continue;
        }

        if( ndi->getFixed() != NodeFixed_NOT_FIXED )
        {
            // Something fixed.  Might have shape information.
            if( m_network->m_shapes[ndi->getId()].size() == 0 )
            {
                ptr = new Block( ndi );
                ptr->m_isPlaced = true;
                ptr->m_isFixed = true;
                m_root->m_blocks.push_back( ptr );
                ++nBlocks1;
            }
            else
            {
                // Shape.
                for( int j = 0; j < m_network->m_shapes[ndi->getId()].size(); j++ )
                {
                    Node* shape = m_network->m_shapes[ndi->getId()][j];

                    xmin = shape->getX() - 0.5 * shape->getWidth();
                    xmax = shape->getX() + 0.5 * shape->getWidth();
                    ymin = shape->getY() - 0.5 * shape->getHeight();
                    ymax = shape->getY() + 0.5 * shape->getHeight();

                    ptr = new Block( xmin, xmax, ymin, ymax );
                    ptr->m_isPlaced = true;
                    ptr->m_isFixed = true;
                    m_root->m_blocks.push_back( ptr );
                    ++nBlocks1;
                }
            }
        }
        else
        {
            // Something movable.
            ptr = new Block( ndi );
            ptr->m_isPlaced = false;
            ptr->m_isFixed = false;
            m_root->m_blocks.push_back( ptr );
            ++nBlocks1;
        }
    }
    // Create blocks to "blackout" space that cannot be used within the region.
    // We should be able to use information about the segments to figure this
    // out and to add that information row by row.
    int nBlocks2 = 0;
    for( int r = 0; r < m_arch->m_rows.size(); r++ )
    {
        // Figure out segments of usable space.
        double rowB = m_arch->m_rows[r]->getY();
        double rowT = rowB + m_arch->m_rows[r]->getH();
        ymin = std::max( rowB, m_root->m_ymin );
        ymax = std::min( rowT, m_root->m_ymax );
        if( ymax <= ymin+1.0e-3 )
        {
            continue;
        }
        std::vector<std::pair<double,double> > intervals;
        for( int s = 0; s < m_mgr->m_segsInRow[r].size(); s++ )
        {
            DetailedSeg* segPtr = m_mgr->m_segsInRow[r][s];

            // Find valid segments to create valid intervals.
            if( segPtr->getRegId() != m_curReg )
            {
                continue;
            }

            xmin = std::max( segPtr->m_xmin, m_root->m_xmin );
            xmax = std::min( segPtr->m_xmax, m_root->m_xmax );

            if( xmax <= xmin+1.0e-3 )
            {
                continue;
            }
            intervals.push_back( std::make_pair(xmin,xmax) );
        }
        std::stable_sort( intervals.begin(), intervals.end(), DetailedMgr::compareBlockages() );

        int n = intervals.size();
        if( n == 0 )
        {
            continue;
        }

        if( intervals[0].first > m_root->m_xmin )
        {
            xmin = m_root->m_xmin;
            xmax = std::min( m_root->m_xmax, intervals[0].first );



            if( xmax > xmin )
            {   
                ptr = new Block( xmin, xmax, ymin, ymax );
                ptr->m_isPlaced = true;
                ptr->m_isFixed = true;
                m_root->m_blocks.push_back( ptr );
                ++nBlocks2;
            }
        }
        for( int i = 1; i < intervals.size(); i++ )
        {
            if( intervals[i].first > intervals[i-1].second )
            {
                xmin = std::max( m_root->m_xmin, intervals[i-1].second );
                xmax = std::min( m_root->m_xmax, intervals[i-0].first  );
                if( xmax > xmin )
                {
                    ptr = new Block( xmin, xmax, ymin, ymax );
                    ptr->m_isPlaced = true;
                    ptr->m_isFixed = true;
                    m_root->m_blocks.push_back( ptr );
                    ++nBlocks2;
                }
            }
        }
        if( intervals[n-1].second < m_root->m_xmax )
        {
            xmin = std::max( m_root->m_xmin, intervals[n-1].second );
            xmax = m_root->m_xmax;
            if( xmax > xmin )
            {
                ptr = new Block( xmin, xmax, ymin, ymax );
                ptr->m_isPlaced = true;
                ptr->m_isFixed = true;
                m_root->m_blocks.push_back( ptr );
                ++nBlocks2;
            }
        }
    }
//    std::cout << "Total blocks from network is " << nBlocks1 << std::endl;
//    std::cout << "Creating black outs..." << std::endl;
//    std::cout << "Total black-out blocks is " << nBlocks2 << std::endl;

    // Some stats.
    int nMove = 0;
    int nFixed = 0;
    int nBlock = 0;
    for( size_t i = 0; i < m_root->m_blocks.size(); i++ )
    {
        ptr = m_root->m_blocks[i];
        if( ptr->m_node != 0 )
        {
            if( ptr->m_isFixed )
                ++nFixed;
            else
                ++nMove;
        }
        else
        {
            if( !ptr->m_isFixed )
            {
                ERRMSG( "Error, Problem when creating blackouts for region." );
                exit(-1);
            }
            ++nBlock;
        }
    }

    std::cout << boost::format( "Region %3d, Initial blocks is %d, Movable is %d, Fixed is %d, Blockages is %d\n" )
            % m_curReg % m_root->m_blocks.size() % nMove % nFixed % nBlock;

    std::cout << boost::format( "Region %3d, Surrounding box is X[%.1lf,%.1lf], Y[%.1lf,%.1lf]\n" )
            % m_curReg 
            % m_arch->m_regions[m_curReg]->m_xmin
            % m_arch->m_regions[m_curReg]->m_xmax
            % m_arch->m_regions[m_curReg]->m_ymin
            % m_arch->m_regions[m_curReg]->m_ymax
            ;

    std::cout << boost::format( "Rectangles:\n" );
    for( size_t r = 0; r < m_arch->m_regions[m_curReg]->m_rects.size(); r++ )
    {
        double xl = m_arch->m_regions[m_curReg]->m_rects[r].m_xmin;
        double xr = m_arch->m_regions[m_curReg]->m_rects[r].m_xmax;
        double yb = m_arch->m_regions[m_curReg]->m_rects[r].m_ymin;
        double yt = m_arch->m_regions[m_curReg]->m_rects[r].m_ymax;
        std::cout << boost::format( "[%.1lf,%.1lf -> %.1lf,%.1lf]") % xl % yb %xr % yt << std::endl;
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int DwsmLegalizer::run( Block* b, int depth )
{
    // TODO:
    // - Handling of really large blocks (e.g., floorplanning?).

    double occ = 0.;
    double cap = 0.;
    calculateOccAndCap( b, occ, cap );

    // Compute the average height/width of the UNPLACED cells.
    double singleRowHeight = m_arch->m_rows[0]->getH();
    const int maxTetrisPasses = 10; // How large???
    int     count = 0;
    double  avgW = 0.;
    double  avgH = 0.;
    int     unplaced = 0;
    double x, y;

    std::vector<double> origXmin;
    std::vector<double> origXmax;
    std::vector<double> origYmin;
    std::vector<double> origYmax;

    std::vector<double> mmfpXmin;
    std::vector<double> mmfpXmax;
    std::vector<double> mmfpYmin;
    std::vector<double> mmfpYmax;

    std::vector<double> bestXmin;
    std::vector<double> bestXmax;
    std::vector<double> bestYmin;
    std::vector<double> bestYmax;

    std::vector<bool> bestPlaced;
    std::map<Block*,int> tag;

    char buf[256];



    for( int i = 0; i < b->m_blocks.size(); i++ ) 
    {
        Block* ptri = b->m_blocks[i];

        if( ptri->m_isPlaced || ptri->m_isFixed )
        {
            continue;
        }

        avgW += (ptri->m_xmax - ptri->m_xmin);
        avgH += (ptri->m_ymax - ptri->m_ymin);

        ++count;
    }
    if( count > 0 ) 
    {
        avgW /= (double)count;
        avgH /= (double)count;
    }

    // Set a threshold about what is considered large or small.
    double largeCellThreshold = 0.;
    if( depth == 1 ) 
    {
        largeCellThreshold = 100.;
    } 
    else if( depth == 2 ) 
    {
        largeCellThreshold =  50.;
    } 
    else 
    {
        largeCellThreshold =  25.;
    } 
    largeCellThreshold *= avgW * avgH;

    // Categorize the cells.
    std::set<Block*> preplaced;
    std::vector<Block*> placed;
    std::vector<Block*> other;
    std::vector<Block*> fixed;
    std::vector<Block*> small;
    std::vector<Block*> medium;
    std::vector<Block*> large;
    for( size_t i = 0; i < b->m_blocks.size(); i++ )
    {
        Block* ptri = b->m_blocks[i];

        if( ptri->m_isFixed )
        {
            preplaced.insert( ptri );
            fixed.push_back( ptri );
        }
        else 
        {
            double ww = ptri->m_xmax - ptri->m_xmin;
            double hh = ptri->m_ymax - ptri->m_ymin;

            // Large if: (i) a big area; (ii) taller than a certain threshold; (iii) wider
            // that a certain threshold.
            if( ww*hh >= largeCellThreshold || hh > 4.0*singleRowHeight+1.0e-3 || ww > 10.0 * avgW )
            {
                large.push_back( ptri );
            }
            else
            {
                if( ptri->m_node != 0 )
                {
                    int span = (int)(ptri->m_node->getHeight() / singleRowHeight + 0.5);
                    if( span > 3 )
                    {
//                        medium.push_back( ptri );
                    }
                }
                small.push_back( ptri );
            }
        }
    }

    for( size_t d = 0; d < depth; d++ ) std::cout << " ";
    std::cout << boost::format( "Running on block [%.2lf,%.2lf]->[%.2lf,%.2lf], " )
            % b->m_xmin % b->m_ymin % b->m_xmax % b->m_ymax;
    std::cout << boost::format( "Preplaced blocks is %d/%d, Large is %d, Medum is %d, Small is %d, " )
            % preplaced.size() % fixed.size() % large.size() % medium.size() % small.size();
    std::cout << boost::format( "Capacity is %.2e, Occupancy is %.2e, Density is %.2lf%%" )
            % cap % occ % (100.*(occ/cap)) << std::endl;

    // If we have detected large cells, then we should attempt to handle
    // those cells first.
    if( large.size() != 0 )
    {
        // Work only with the large and preplaced cells.
        other.clear();
        other.insert( other.end(), large.begin(), large.end() );

        for( size_t i = 0; i < other.size(); i++ )
        {
            tag[other[i]] = i;
        }

        /*
        // XXX: This isn't quite complete.  Just doing a shift will not get the
        // large cells aligned with rows...  If I fix this, then it should be
        // okay.
        {
            char buf[128];
            sprintf( &buf[0], "floorplan (orig)" );
            draw( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, large, buf );
        }           

        // Record original positions.
        origXmin.resize( other.size() );
        origXmax.resize( other.size() );
        origYmin.resize( other.size() );
        origYmax.resize( other.size() );
        for( size_t i = 0; i < other.size(); i++ )
        {
            Block* ptri = other[i];
            int idx = tag[ptri];
            origXmin[idx] = ptri->getXmin();
            origXmax[idx] = ptri->getXmax();
            origYmin[idx] = ptri->getYmin();
            origYmax[idx] = ptri->getYmax();
        }

        // Try original placement with a minshift.
        int up0 = mmfFloorplan( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, other );

        // Record original positions.
        mmfpXmin.resize( other.size() );
        mmfpXmax.resize( other.size() );
        mmfpYmin.resize( other.size() );
        mmfpYmax.resize( other.size() );
        for( size_t i = 0; i < other.size(); i++ )
        {
            Block* ptri = other[i];
            int idx = tag[ptri];
            mmfpXmin[idx] = ptri->getXmin();
            mmfpXmax[idx] = ptri->getXmax();
            mmfpYmin[idx] = ptri->getYmin();
            mmfpYmax[idx] = ptri->getYmax();
        }

        {
            char buf[128];
            sprintf( &buf[0], "floorplan (shift)" );
            draw( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, large, buf );
        }           

        // Return placement to original positions and try tetris and a minshift.
        for( size_t i = 0; i < other.size(); i++ )
        {
            Block* ptri = other[i];
            int idx = tag[ptri];
            ptri->setXmin( origXmin[idx] );
            ptri->setXmax( origXmax[idx] );
            ptri->setYmin( origYmin[idx] );
            ptri->setYmax( origYmax[idx] );
        }
        */

        // Sometimes calling tetris multiple times can yield an improved result!
        int bestUnplaced = std::numeric_limits<int>::max();
        bestPlaced.resize( other.size() );
        bestXmin.resize( other.size() );
        bestXmax.resize( other.size() );
        bestYmin.resize( other.size() );
        bestYmax.resize( other.size() );
        for( int pass = 1; pass <= maxTetrisPasses; pass++ )
        {
            unplaced = tetris( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, other );
            std::cout << "Pass " << pass << " of tetris, unplaced cells is " << unplaced << std::endl;
            if( unplaced == 0 )
            {
                break;
            }
            if( unplaced < bestUnplaced )
            {
                //std::cout << "Saved block placement with only " << unplaced << " unplaced blocks." << std::endl;
                bestUnplaced = unplaced;
                // Save placement.
                for( size_t i = 0; i < other.size(); i++ )
                {
                    Block* ptri = other[i];
                    int idx = tag[ptri];
                    bestPlaced[idx] = ptri->m_isPlaced;
                    bestXmin[idx] = ptri->getXmin();
                    bestXmax[idx] = ptri->getXmax();
                    bestYmin[idx] = ptri->getYmin();
                    bestYmax[idx] = ptri->getYmax();
                }
            }
        }

        if( unplaced > 0 && bestUnplaced < unplaced )
        {
            // Restore placement.
            unplaced = 0;
            for( size_t i = 0; i < other.size(); i++ )
            {
                Block* ptri = other[i];
                int idx = tag[ptri];
                ptri->m_isPlaced = bestPlaced[idx];
                ptri->setXmin( bestXmin[idx] );
                ptri->setXmax( bestXmax[idx] );
                ptri->setYmin( bestYmin[idx] );
                ptri->setYmax( bestYmax[idx] );
                if( !ptri->m_isPlaced )
                {
                    ++unplaced;
                }
            }
        }

        int up1 = mmfFloorplan( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, other );
        for( size_t i = 0; i < other.size(); i++ )
        {
            Block* ptri = other[i];
            int idx = tag[ptri];
            bestXmin[idx] = ptri->getXmin();
            bestXmax[idx] = ptri->getXmax();
            bestYmin[idx] = ptri->getYmin();
            bestYmax[idx] = ptri->getYmax();
        }

        for( size_t d = 0; d < depth; d++ ) std::cout << " ";
        std::cout << boost::format( "Unplaced large cells is %d\n" ) % unplaced;
    }

    // I don't think there is anything else that can be done with large cells
    // so just make them all fixed.  This means they are removed from any
    // future problems.
    for( size_t i = 0; i < large.size(); i++ )
    {
        Block* ptri = large[i];
        ptri->m_isFixed = true;
        fixed.push_back( ptri );
    }


    // Should we do anything special with medium sized blocks?  For example,
    // we could handle all of the multi-height cells if we have detected
    // them...  XXX: This worked effectively for the multi-height cells, but
    // I then got a weird result from the remaining small cells.  I need to
    // investigate.
    if( medium.size() != 0 )
    {
    // XXX: This is a hack since I am just snapping to segments and shifting.

        std::cout << "Trying to resolve multi-height cells with a shift." << std::endl;

        other.erase( other.begin(), other.end() );
        other.insert( other.end(), medium.begin(), medium.end() );
        other.insert( other.end(), large.begin(), large.end() );

        std::vector<Node*> multi;
        for( size_t i = 0; i < other.size(); i++ )
        {
            Block* ptri = other[i];
            ptri->m_isPlaced = false;
            ptri->m_isFixed = false;

            Node* ndi = other[i]->m_node;
            multi.push_back( ndi );
        }

        // To use the code we want, we need to assign the cells to segments.
        m_mgr->assignCellsToSegments( multi );

        MinMovementFloorplannerParams params;
        MinMovementFloorplanner mmf( params, m_mgr->m_rng );
        if( mmf.test( *m_mgr, m_curReg, true ) )
        {
            std::cout << "Shift successful.  Fixing multi-height cells." << std::endl;
            mmf.shift( *m_mgr, m_curReg, true );
            for( size_t i = 0; i < other.size(); i++ )
            {
                Block* ptri = other[i];

                Node* ndi = ptri->m_node;

                ptri->m_isPlaced = true;
                ptri->m_isFixed = true;
                ptri->setXmin( ndi->getX() - 0.5 * ndi->getWidth() );
                ptri->setXmax( ndi->getX() + 0.5 * ndi->getWidth() );
                ptri->setYmin( ndi->getY() - 0.5 * ndi->getHeight() );
                ptri->setYmax( ndi->getY() + 0.5 * ndi->getHeight() );
                fixed.push_back( ptri );

                std::vector<Block*>::iterator it = std::find( small.begin(), small.end(), ptri );
                if( small.end() != it )
                {
                    small.erase( it );
                }
            }
        }
        else
        {
            std::cout << "Shift unsuccessful." << std::endl;
        }

        // To avoid a later problem, we need to remove the cells we
        // assigned to segments.
        for( size_t i = 0; i < multi.size(); i++ )
        {
            Node* ndi = multi[i];

            std::vector<DetailedSeg*> segs = m_mgr->m_reverseCellToSegs[ndi->getId()];
            for( size_t s = 0; s < segs.size(); s++ )
            {
                m_mgr->removeCellFromSegment( ndi, segs[s]->m_segId );
            }
        }
    }

    if( occ <= 0.8500 * cap )
    {
        // Possibly using something like lal to partition into regions rather
        // than recursive bisection is better at reducing movement...

        unplaced = lal( b );
        //{
        //    char buf[256];
        //    sprintf( &buf[0], "tetris%d", m_curReg );
        //    draw( b, buf );
        //}
        return unplaced;
    }

    // At this point, the only movable cells should be "what looks small".  If
    // the number of small cells left is reasonable, then just try to solve it.
    // Otherwise, split the block and recurse.  
    if( small.size() < m_params.m_smallProblemThreshold )
    {
//        {
//            char buf[256];
//            sprintf( &buf[0], "tetris%d", m_curReg );
//            draw( b, buf );
//        }

        // Problem is smallish, so go after it in a single shot.
        other.erase( other.begin(), other.end() );
        other.insert( other.end(), small.begin(), small.end() );

        // Sometimes calling tetris multiple times can yield an improved result!
        int bestUnplaced = std::numeric_limits<int>::max();
        for( size_t i = 0; i < other.size(); i++ )
        {
            tag[other[i]] = i;
        }
        bestPlaced.resize( other.size() );
        bestXmin.resize( other.size() );
        bestXmax.resize( other.size() );
        bestYmin.resize( other.size() );
        bestYmax.resize( other.size() );
        for( int pass = 1; pass <= maxTetrisPasses; pass++ )
        {
            unplaced = tetris( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, other );
            //std::cout << "Pass " << pass << " of tetris, unplaced cells is " << unplaced << std::endl;
            if( unplaced == 0 )
            {
                break;
            }
            if( unplaced < bestUnplaced )
            {
                //std::cout << "Saved block placement with only " << unplaced << " unplaced blocks." << std::endl;
                bestUnplaced = unplaced;
                // Save placement.
                for( size_t i = 0; i < other.size(); i++ )
                {
                    Block* ptri = other[i];
                    int idx = tag[ptri];
                    bestPlaced[idx] = ptri->m_isPlaced;
                    bestXmin[idx] = ptri->getXmin();
                    bestXmax[idx] = ptri->getXmax();
                    bestYmin[idx] = ptri->getYmin();
                    bestYmax[idx] = ptri->getYmax();
                }
            }
        }
        if( unplaced > 0 && bestUnplaced < unplaced )
        {
            // Restore placement.
            unplaced = 0;
            for( size_t i = 0; i < other.size(); i++ )
            {
                Block* ptri = other[i];
                int idx = tag[ptri];
                ptri->m_isPlaced = bestPlaced[idx];
                ptri->setXmin( bestXmin[idx] );
                ptri->setXmax( bestXmax[idx] );
                ptri->setYmin( bestYmin[idx] );
                ptri->setYmax( bestYmax[idx] );
                if( !ptri->m_isPlaced )
                {
                    ++unplaced;
                }
            }
            //std::cout << "Restored block placement with " << unplaced << " unplaced blocks." << std::endl;
        }

        // If we have failed, is it worth removing some of the smaller single height
        // cells and re-attempting?  Ending with unplaced multi-height cells is bad
        // since they are much harder to deal with later on...  I expect that this is
        // only a problem when the density is quite high.
        if( unplaced > 0 )
        {
            std::vector<Block*> single;
            std::vector<Block*> multi;
            std::vector<Block*> misc;
            for( size_t i = 0; i < other.size(); i++ )
            {
                Block* ptri = other[i];
                Node* ndi = ptri->m_node;
                if( ndi != 0 )
                {
                    int spanned = (int)(ndi->getHeight() / singleRowHeight + 0.5);
                    if( spanned == 1 )
                    {
                        single.push_back( other[i] );
                    }
                    else
                    {
                        multi.push_back( other[i] );
                    }
                }
                else
                {
                    misc.push_back( other[i] );
                }
            }
            std::cout << "Divided blocks, "
                << "Single height is " << single.size() << ", "
                << "Multi-height is " << multi.size() << ", "
                << "Miscellaneous is " << misc.size()
                << std::endl;

            if( multi.size() != 0 )
            {
                for( size_t i = 0; i < other.size(); i++ )
                {
                    Block* ptri = other[i];
                    ptri->m_isPlaced = false;
                }
                multi.insert( multi.end(), misc.begin(), misc.end() );
                misc.clear();

                // Only multi-height.
                int up0 = tetris( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, multi );

                // Fix the multi-height.
                for( size_t i = 0; i < multi.size(); i++ )
                {
                    multi[i]->m_isFixed = true;
                    fixed.push_back( multi[i] );
                }

                // Only the single height.
                int up1 = tetris( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, single );

                // Undo multi-height fix.
                for( size_t i = 0; i < multi.size(); i++ )
                {
                    multi[i]->m_isFixed = false;
                }

                std::cout << "Divided blocks, Unplaced single height cells is " << up1 << ", "
                    << "Unplaced multi-height cells is " << up0 << std::endl;
            }
        }

//        {
//            char buf[256];
//            sprintf( &buf[0], "tetris%d", m_curReg );
//            draw( b, buf );
//        }
    }
    else
    {

        other.erase( other.begin(), other.end() );
        other.insert( other.end(), small.begin(), small.end() );
        Block* p0;
        Block* p1;
        //std::cout << "Blocks before split is " << b->m_blocks.size() << std::endl;
        split( b, p0, p1, fixed, other );
        int up0 = run( p0, depth+1 );
        int up1 = run( p1, depth+1 );

        // Undo the partitioning as we "back up". 
        if( b->m_blocks.size() != 2 || p0 == 0 || p1 == 0 )
        {
            ERRMSG( "Error, Problem when merging partitions." );
            exit(-1);
        }
        std::set<Block*> merged;
        merged.insert( p0->m_blocks.begin(), p0->m_blocks.end() );
        merged.insert( p1->m_blocks.begin(), p1->m_blocks.end() );
        delete p0;
        delete p1;
        b->m_blocks.erase( b->m_blocks.begin(), b->m_blocks.end() );
        b->m_blocks.insert( b->m_blocks.end(), merged.begin(), merged.end() );

        unplaced = up0 + up1;
        if( up0 > 0 || up1 > 0 )
        {
            // We have unplaced cells.  We can try and repair the placement.

            placed.erase( placed.begin(), placed.end() );
            fixed.erase( fixed.begin(), fixed.end() );
            other.erase( other.begin(), other.end() );

            for( size_t i = 0; i < b->m_blocks.size(); i++ )
            {
                Block* ptri = b->m_blocks[i];

                if( ptri->m_isPlaced && !ptri->m_isFixed )
                {
                    // Placed cells (but not fixed).
                    placed.push_back( ptri );
                }
                else if( (ptri->m_isPlaced && ptri->m_isFixed) || preplaced.end() != preplaced.find( ptri ) )
                {
                    // Fixed cells.  Either prelaced cells or cells that have been fixed somehow.
                    if( !ptri->m_isFixed )
                    {
                        ERRMSG( "Error, Fixed cell expected." );
                        exit(-1);
                    }
                    fixed.push_back( ptri );
                }
                else
                {
                    // Unplaced cells.
                    if( ptri->m_isPlaced )
                    {
                        ERRMSG( "Error, Expecting an unplaced cell when merging partitions." );
                        exit(-1);
                    }
                    other.push_back( ptri );
                }
            }

            if( other.size() == 0 || other.size() != up0 + up1 )
            {
                sprintf( &buf[0], "Error, Mismatch in unplaced cells when merging partitions, Expected %d, Found %d",
                        up0+up1, (int)other.size() );
                ERRMSG( buf );
                exit(-1);
            }

            // Temporarily fix all of the placed cells.
            for( size_t i = 0; i < placed.size(); i++ )
            {
                placed[i]->m_isFixed = true;
                fixed.push_back( placed[i] );
            }

            // Try to place the unplaced cells.
            unplaced = repair( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, other );

            // Undo temporarily fixed cells.
            for( size_t i = 0; i < placed.size(); i++ )
            {
                placed[i]->m_isFixed = false;
            }
        }
    }

    // We need to properly count the number of unplaced cell in this block.  The
    // only cells we should ignore are those that are "preplaced".
    unplaced = 0;
    for( size_t i = 0; i < b->m_blocks.size(); i++ ) 
    {
        Block* ptri = b->m_blocks[i];
        if( preplaced.end() != preplaced.find( ptri ) )
        {
            continue;
        }
        if( !ptri->m_isPlaced )
        {
            ++unplaced;
        }
    }

    // Return the number of unplaced items.
    if( unplaced != 0 )
    {
        for( size_t d = 0; d < depth; d++ ) std::cout << " ";
        std::cout << boost::format( "Running on block [%.2lf,%.2lf]->[%.2lf,%.2lf], Unplaced cells is %d" )
            % b->m_xmin % b->m_ymin % b->m_xmax % b->m_ymax % unplaced << std::endl;
    }
    return unplaced;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DwsmLegalizer::calculateOccAndCap( Block* ptrb, double& occ, double& cap )
{
    // Given a block, examine the blocks within and determine the occupancy.
    // For fixed blocks, we need to determine the intersection of the fixed
    // blocks with the "owner block".  Because we have inserted fixed blocks
    // to black out space, the capacity _is_ just the size of the block.
    double ww = ptrb->getXmax() - ptrb->getXmin();
    double hh = ptrb->getYmax() - ptrb->getYmin();
    double xx, yy;

    cap = ww * hh;

    occ = 0.;
    for( size_t i = 0; i < ptrb->m_blocks.size(); i++ )
    {
        Block* ptri = ptrb->m_blocks[i];
        if( ptri->m_isFixed )
        {
            xx = std::max( ptri->getXmin(), ptrb->getXmin() );
            yy = std::max( ptri->getYmin(), ptrb->getYmin() );
            ww = std::min( ptri->getXmax(), ptrb->getXmax() ) - xx;
            hh = std::min( ptri->getYmax(), ptrb->getYmax() ) - yy;
        }
        else
        {
            ww = ptri->getXmax() - ptri->getXmin();
            hh = ptri->getYmax() - ptri->getYmin();
        }
        if( ww > 1.0e-3 && hh > 1.0e-3 && ww*hh > 1.0e-3 )
        {
            occ += ww * hh;
        }
    }   
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DwsmLegalizer::split( Block* b, Block*& p0, Block*& p1,
        std::vector<Block*>& fixed, 
        std::vector<Block*>& other )
{
//    std::cout << "Splitting a block." << std::endl;

    p0 = NULL;
    p1 = NULL;

    std::vector<Block*> collection;
    Block* ptri;
    double avgW, avgH, totA, curA;
    double ratio = 0.5000;
    double cutline = 0.;
    double ww, hh, xx, yy;
    int count;
    unsigned assigned;
    double occ[3], cap[3];


    // Determine split direction; just go in the widest direction.
    ww = b->getXmax() - b->getXmin();
    hh = b->getYmax() - b->getYmin();
    int dir = (ww >= hh) ? 0 : 1;

    // Put all blocks into a single vector.
    collection.erase( collection.begin(), collection.end() );
    collection.insert( collection.end(), fixed.begin(), fixed.end() );
    collection.insert( collection.end(), other.begin(), other.end() );

    // Some statistics to help with the split.  Get the total cell area
    // and the average width and height of movable cells.
    totA = 0.;
    avgW = 0.;
    avgH = 0.;
    count = 0;
    for( size_t i = 0; i < collection.size(); ++i ) 
    {
        ptri = collection[i];
        ww = ptri->getXmax() - ptri->getXmin();
        hh = ptri->getYmax() - ptri->getYmin();
        totA += ww * hh;

        if( !ptri->m_isFixed ) 
        {
            avgW += ww;
            avgH += hh;
            ++count;
        }
    }
    avgW /= (double)count;
    avgH /= (double)count;

    // Try to determine the location of the cutline near the mid-point of
    // the cell area.  Try to account for large cells that might favour 
    // one side or the other.
    BlockSorter sorter;
    sorter._t = (dir == 0) ? BlockSorter::L : BlockSorter::B;
    std::sort( collection.begin(), collection.end(), sorter );

    ratio = 0.5;    // For sanity.
    curA = 0.;
    for( size_t i = 0; i < collection.size(); ++i ) 
    {
        ptri = collection[i];
        ww = ptri->getXmax() - ptri->getXmin();
        hh = ptri->getYmax() - ptri->getYmin();

        curA += ww * hh;
        if( curA >= totA / 2. ) 
        {
            ratio = ( curA / totA );
            break;
        }
    }
    ratio = std::max( (double)0.30, std::min( (double)0.70, ratio ) );

    if( dir == 0 ) 
    {
        cutline = std::floor(ratio * ( b->getXmax() - b->getXmin() ) + b->getXmin());
        
        p0 = new Block( b->getXmin(), cutline, b->getYmin(), b->getYmax() );
        p1 = new Block( cutline, b->getXmax(), b->getYmin(), b->getYmax() );
    } 
    else 
    {
        cutline = ratio * ( b->getYmax() - b->getYmin() ) + b->getYmin();

        // Adjust the cutline to get it lined up with a row.
        int row_b = -1;
        for( size_t row_c = 1; row_c < m_arch->m_rows.size(); row_c++ )
        {
            double yc = m_arch->m_rows[row_c]->getY();
            if( yc < b->getYmin() )
            {
                continue;
            }
            if( yc > b->getYmax() )
            {
                continue;
            }

            if( row_b == -1 )
            {
                row_b = row_c;
            }
            else
            {
                double yb = m_arch->m_rows[row_b]->getY();
                if( std::fabs( yc - cutline ) < std::fabs( yb - cutline ) )
                {
                    row_b = row_c;
                }
            }
        }
        if( row_b == -1 )
        {
            // Weird... could not find a row within the block.  This is worth
            // investigating.
            ERRMSG( "Error, Could not find a row within a block being partitioned." );
            exit(-1);
        }
        cutline = m_arch->m_rows[row_b]->getY();

        p0 = new Block( b->getXmin(), b->getXmax(), b->getYmin(), cutline );
        p1 = new Block( b->getXmin(), b->getXmax(), cutline, b->getYmax() );
    }

    // Process the fixed blocks and assign them to the partitions.  A fixed block
    // can, actually, be assigned to both partitions if it crosses the cutline.
    // XXX: Do we actually need to "duplicate" the fixed block before assigning it?
    // I don't think so...  I think the way the code works is that at the end, we
    // grab all of the blocks, insert them into a set (to avoid duplication) and 
    // then delete them all.
    for( size_t i = 0; i < fixed.size(); i++ ) 
    {
        ptri = fixed[i];

        assigned = 0x00;

        if ( ((dir == 0) ? ptri->getXmin() : ptri->getYmin()) <= cutline ) 
        {
            assigned |= 0x01;
        }
        if ( ((dir == 0) ? ptri->getXmax() : ptri->getYmax()) >= cutline ) 
        {
            assigned |= 0x02;
        }

        if( (assigned&0x01) != 0x00 ) 
        {
            p0->m_blocks.push_back( ptri );
        }
        if( (assigned&0x02) != 0x00 ) 
        {
            p1->m_blocks.push_back( ptri );
        }
    }
    fixed.erase( fixed.begin(), fixed.end() );

    // Process the movable blocks according to the actual occupancy and
    // capacity available.  We first attempt to assign blocks based on
    // their actual locations with respect to the cutline.  Then, at the
    // end, we jam them into a partition based on fit.

    calculateOccAndCap( b , occ[2], cap[2] );
    calculateOccAndCap( p0, occ[0], cap[0] );
    calculateOccAndCap( p1, occ[1], cap[1] );

//    std::cout << "Splitting block " 
 //       << "[" << b->getXmin() << "," << b->getYmin() << " -> " << b->getXmax() << "," << b->getYmax() << "]" << ", "
 //       << "occ is " << occ[2] << ", cap is " << cap[2] 
 //       << std::endl;

//    for( size_t pass = 0; pass <= 1; pass++ )
//    {
//        Block* partPtr = (pass == 0) ? p0 : p1;
//        std::cout << "Partitioned block before assignment is " 
//            << "[" 
//            << partPtr->getXmin() << "," << partPtr->getYmin() 
//            << " -> " 
//            << partPtr->getXmax() << "," << partPtr->getYmax() 
//            << "]" 
//            << ", occ is " << occ[pass] << ", cap is " << cap[pass] << ", blocks is " << partPtr->m_blocks.size() 
//            << std::endl;
//    }

    ratio = std::max( (double)0.99, occ[2]/cap[2] );   
    if( dir == 0 ) 
    {
        for( int pass = 0; pass <= 2; pass++ ) 
        {
            sorter._t = (pass == 0) ? BlockSorter::L : BlockSorter::R;
            std::sort( other.begin(), other.end(), sorter );
            std::reverse( other.begin(), other.end() );

            Block* partPtr = pass == 0 ? p0 : p1;
            for( size_t i = other.size(); i > 0; ) 
            {
                --i;
                ptri = other[i];

                xx = 0.5 * (ptri->getXmax() + ptri->getXmin());

                if( pass == 0 && xx > cutline )     break;
                if( pass == 1 && xx < cutline )     break;

                ww = ptri->getXmax() - ptri->getXmin();
                hh = ptri->getYmax() - ptri->getYmin();

                if( (occ[pass]+ww*hh)/cap[pass] <= ratio ) 
                {
                    occ[pass] += ww*hh;
                    partPtr->m_blocks.push_back( ptri );
                    other.erase( other.begin() + i );
                }
            }
        }
    } 
    else 
    {
        for( int pass = 0; pass <= 1; pass++ ) 
        {
            sorter._t = (pass == 0) ? BlockSorter::B : BlockSorter::T;
            std::sort( other.begin(), other.end(), sorter );
            std::reverse( other.begin(), other.end() );
            
            Block* partPtr = pass == 0 ? p0 : p1;
            for( size_t i = other.size(); i > 0; ) 
            {
                --i;
                ptri = other[i];

                yy = 0.5*(ptri->getYmax() + ptri->getYmin());

                if( pass == 0 && yy > cutline )     break;
                if( pass == 1 && yy < cutline )     break;

                ww = ptri->getXmax() - ptri->getXmin();
                hh = ptri->getYmax() - ptri->getYmin();

                if( (occ[pass]+ww*hh)/cap[pass] <= ratio )
                {
                    occ[pass] += ww*hh;
                    partPtr->m_blocks.push_back( ptri );
                    other.erase( other.begin() + i );
                }
            }
        }
    }
    while( !other.empty() ) 
    {
        ptri = other.back();
        other.pop_back();

        ww = ptri->getXmax() - ptri->getXmin();
        hh = ptri->getYmax() - ptri->getYmin();
        if( (occ[0]+ww*hh)/cap[0] < (occ[1]+ww*hh)/cap[1] ) 
        {
            occ[0] += ww*hh;
            p0->m_blocks.push_back( ptri );
        } 
        else 
        {
            occ[1] += ww*hh;
            p1->m_blocks.push_back( ptri );
        }
    }

//    for( size_t pass = 0; pass <= 1; pass++ )
//    {
//        Block* partPtr = (pass == 0) ? p0 : p1;
//        std::cout << "Partitioned block after assignment is " 
//            << "[" 
//            << partPtr->getXmin() << "," << partPtr->getYmin() 
//            << " -> " 
//            << partPtr->getXmax() << "," << partPtr->getYmax() 
//            << "]" 
//            << ", occ is " << occ[pass] << ", cap is " << cap[pass] << ", blocks is " << partPtr->m_blocks.size() 
//            << std::endl;
//    }

    // Lastly, we clear the current blocks, and add the partitions.  (Until
    // the partitions get merged again, this block will not have any cells
    // in it other than the partitions.)
    b->m_blocks.erase( b->m_blocks.begin(), b->m_blocks.end() );
    b->m_blocks.push_back( p0 );
    b->m_blocks.push_back( p1 );
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int DwsmLegalizer::tetris( double xmin, double xmax, double ymin, double ymax,
        std::vector<Block*>& preplaced, std::vector<Block*>& blocks )
{
    // Use tetris to legalize the rectangle.  Blockages, etc. have been added
    // as blocks and are in the 'preplaced' vector.
    //
    // This routine checks to occupancy and the capacity of the block and 
    // inserts filler cells.  The purpose of the filler cells is to try and
    // prevent cells from moving too far from their original positions in
    // the situation that we have a lot of whitespace.
    
    double xx, yy, ww, hh;
    SpaceManager sm;
    double cap = (xmax-xmin)*(ymax-ymin);
    double occ = 0.;
    std::vector<Block*> filler;
    std::vector<Block*> temp;
    int unplaced = 0;
    double x1, x2, y1, y2;

    //{
    //    char buf[128];
    //    sprintf( &buf[0], "filler" );
    //    draw( xmin, xmax, ymin, ymax, preplaced, blocks, buf );
    //}           

    // XXX: Need to reset the block ids!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    for( size_t i = 0; i < blocks.size(); i++ ) 
    {
        blocks[i]->setId( i );
    }
    std::vector<double> origXmin, origXmax, origYmin, origYmax;
    origXmin.resize( blocks.size() );
    origXmax.resize( blocks.size() );
    origYmin.resize( blocks.size() );
    origYmax.resize( blocks.size() );
    for( size_t i = 0; i < blocks.size(); i++ )
    {
        Block* ptri = blocks[i];
        origXmin[ptri->getId()] = ptri->getXmin();
        origXmax[ptri->getId()] = ptri->getXmax();
        origYmin[ptri->getId()] = ptri->getYmin();
        origYmax[ptri->getId()] = ptri->getYmax();
    }

    for( size_t i = 0; i < preplaced.size(); i++ ) 
    {
        Block* ptri = preplaced[i];

        xx = std::max( ptri->getXmin(), xmin );
        yy = std::max( ptri->getYmin(), ymin );
        ww = std::min( ptri->getXmax(), xmax ) - xx;
        hh = std::min( ptri->getYmax(), ymax ) - yy;

        if( ww >= 1.0e-3 && hh >= 1.0e-3 && ww*hh >= 1.0e-3 )
        {
            occ += ww*hh;
        }
    }
    for( size_t i = 0; i < blocks.size(); i++ ) 
    {
        Block* ptri = blocks[i];

        ww = ptri->getXmax() - ptri->getXmin();
        hh = ptri->getYmax() - ptri->getYmin();

        if( ww >= 1.0e-3 && hh >= 1.0e-3 && ww*hh >= 1.0e-3 )
        {
            occ += ww*hh;
        }
    }

    //std::cout << "Tetris on block" << ", "
    //    << "X:[" << xmin << "," << xmax << "]" << ", "
    //    << "Y:[" << ymin << "," << ymax << "]" << ", "
    //    << "Occ is " << occ << ", Cap is " << cap << ", "
    //    << "Density is " << occ/cap
    //    << std::endl;

    for( double ratio = 0.9000; ratio >= 0.6000; ratio -= 0.0500 )
    {
        for( size_t i = 0; i < blocks.size(); i++ )
        {
            Block* ptri = blocks[i];
            ptri->setXmin( origXmin[ptri->getId()] );
            ptri->setXmax( origXmax[ptri->getId()] );
            ptri->setYmin( origYmin[ptri->getId()] );
            ptri->setYmax( origYmax[ptri->getId()] );

            ptri->m_isPlaced = false;
        }

        sm.Reset( xmin, xmax, ymin, ymax );
        if( occ < ratio * cap )
        {
            // The occupancy is below our target.  Therefore, we need to
            // add some filler.  We insert the filler around the center
            // of whitespace.  We don't want too many filler cells, so we
            // limit that.  
            for( size_t i = 0; i < preplaced.size(); i++ ) 
            {
                Block* ptri = preplaced[i];
                sm.AddFullRectangle( ptri->getXmin(), ptri->getXmax(), ptri->getYmin(), ptri->getYmax() );
            }
            for( size_t i = 0; i < blocks.size(); i++ ) 
            {
                Block* ptri = blocks[i];
                sm.AddFullRectangle( ptri->getXmin(), ptri->getXmax(), ptri->getYmin(), ptri->getYmax() );
            }

            // Filler.
            double area = ratio*cap - occ;
            hh = m_singleRowHeight;
            ww = m_siteSpacing;
            int n = (int)std::floor( area / hh / ww );
            int limit = std::max( 1000, (int)(m_params.m_smallProblemThreshold - blocks.size()) );
            while( n >= limit )
            {
                if( ww < hh ) { ww += m_siteSpacing; }
                else { hh += m_singleRowHeight; ww = m_siteSpacing; }
                n = (int)std::floor( area / hh / ww );
            }
            area = occ;
            for( int f = 0; f < n; f++ )
            {
                if( area + ww * hh > ratio*cap )
                {
                    break;
                }

                if( sm.GetLargestFreeSpace( x1, x2, y1, y2 ) ) 
                {
                    xx = 0.5*(x1+x2);
                    yy = 0.5*(y1+y2);
                    Block* ptri = new Block( xx-0.5*ww, xx+0.5*ww, yy-0.5*hh, yy+0.5*hh );
                    ptri->m_isFixed = false;
                    ptri->m_isPlaced = false;
                    ptri->setId( blocks.size() + f ); // !!!
                    filler.push_back( ptri );
                    sm.AddFullRectangle( ptri->getXmin(), ptri->getXmax(), ptri->getYmin(), ptri->getYmax() );
                    area += ww * hh;
                } 
                else 
                {
                    break;
                }

            }
        }
        else
        {
            ww = 0.;
            hh = 0.;
        }
        //std::cout << "Inserting " << filler.size() << " filler cells" << ", "
        //        << "Width is " << ww << ", Height is " << hh << ", "
        //        << "Modified density is " << ((occ + filler.size()*ww*hh)/cap) << std::endl;

        // Solve with filler.
        temp.erase( temp.begin(), temp.end() );
        temp.insert( temp.end(), blocks.begin(), blocks.end() );
        temp.insert( temp.end(), filler.begin(), filler.end() );
        //{
        //    char buf[128];
        //    sprintf( &buf[0], "filler" );
        //    for( size_t i = 0; i < filler.size(); i++ )
        //    {
        //        filler[i]->m_isPlaced = true;
        //    }
        //    draw( xmin, xmax, ymin, ymax, preplaced, temp, buf );
        //    for( size_t i = 0; i < filler.size(); i++ )
        //    {
        //        filler[i]->m_isPlaced = false;
        //    }
        //}           
        unplaced = tetris_inner( xmin, xmax, ymin, ymax, preplaced, temp );
        //{
        //    char buf[128];
        //    sprintf( &buf[0], "filler" );
        //    for( size_t i = 0; i < filler.size(); i++ )
        //    {
        //        filler[i]->m_isPlaced = true;
        //    }
        //    draw( xmin, xmax, ymin, ymax, preplaced, temp, buf );
        //    for( size_t i = 0; i < filler.size(); i++ )
        //    {
        //        filler[i]->m_isPlaced = false;
        //    }
        //}           

        // Cleanup.
        bool usedFiller = (filler.size() != 0 ) ? true : false;
        for( size_t i = 0; i < filler.size(); i++ )
        {
            delete filler[i];
        }
        filler.erase( filler.begin(), filler.end() );

        if( unplaced == 0 || !usedFiller )
        {
            break;
        }
    }

    return unplaced;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int DwsmLegalizer::repair( double xmin, double xmax, double ymin, double ymax,
        std::vector<Block*>& preplaced, std::vector<Block*>& blocks )
{
    std::cout << "Trying to repair placement by placing " << (int)blocks.size() << " unplaced blocks." << std::endl;

    const int max_attempts = 5;
    SpaceManager sm;
    BlockSorter sorter;
    std::vector<double> origXmin, origXmax, origYmin, origYmax;
    std::vector<double> bestXmin, bestXmax, bestYmin, bestYmax;
    Block* ptri;
    std::vector<bool> currPlaced;
    std::vector<bool> bestPlaced;
    int bestUnplaced = std::numeric_limits<int>::max();
    int currUnplaced;
    double bestMovement = std::numeric_limits<double>::max();
    double currMovement;
    std::vector<Rectangle*> spots;
    double shiftX, shiftY;
    std::vector<double> radius;
    double avgW = 0.;
    double avgH = 0.;


    sorter._xmin = xmin;
    sorter._xmax = xmax;
    sorter._ymin = ymin;
    sorter._ymax = ymax;

    bestXmin.resize( blocks.size() );
    bestXmax.resize( blocks.size() );
    bestYmin.resize( blocks.size() );
    bestYmax.resize( blocks.size() );

    origXmin.resize( blocks.size() );
    origXmax.resize( blocks.size() );
    origYmin.resize( blocks.size() );
    origYmax.resize( blocks.size() );

    for( size_t i = 0; i < blocks.size(); i++ )
    {
        blocks[i]->setId( i );
    }

    for( size_t i = 0; i < blocks.size(); i++ ) 
    {
        ptri = blocks[i];

        origXmin[ptri->getId()] = ptri->getXmin();
        origXmax[ptri->getId()] = ptri->getXmax();
        origYmin[ptri->getId()] = ptri->getYmin();
        origYmax[ptri->getId()] = ptri->getYmax();

        bestXmin[ptri->getId()] = ptri->getXmin();
        bestXmax[ptri->getId()] = ptri->getXmax();
        bestYmin[ptri->getId()] = ptri->getYmin();
        bestYmax[ptri->getId()] = ptri->getYmax();
    }

    // Resizing.
    currPlaced.resize( blocks.size() );
    bestPlaced.resize( blocks.size() );

    // Determine radii for searching.
    avgW = 0.;
    avgH = 0.;
    for( size_t i = 0; i < blocks.size(); i++ )
    {
        ptri = blocks[i];

        double ww = ptri->getXmax() - ptri->getXmin();
        double hh = ptri->getYmax() - ptri->getYmin();

        avgW += ww;
        avgH += hh;
    }
    avgW /= (double)(preplaced.size()+blocks.size());
    avgH /= (double)(preplaced.size()+blocks.size());

    radius.resize( blocks.size() );
    std::fill( radius.begin(), radius.end(), std::sqrt( avgW*avgW + avgH*avgH ));

    // Expand the radius for a block if it overlaps with something fixed; we
    // need to search farther to push it out of the fixed block.
    for( size_t i = 0; i < blocks.size(); i++ )
    {
        ptri = blocks[i];

        for( size_t j = 0; j < preplaced.size(); j++ ) 
        {
            if( ptri->getXmax() < preplaced[j]->getXmin() ) continue;
            if( ptri->getXmin() > preplaced[j]->getXmax() ) continue;
            if( ptri->getYmax() < preplaced[j]->getYmin() ) continue;
            if( ptri->getYmin() > preplaced[j]->getYmax() ) continue;

            double ww = (preplaced[j]->getXmax() - preplaced[j]->getXmin());
            double hh = (preplaced[j]->getYmax() - preplaced[j]->getYmin());
            radius[ptri->getId()] = std::max( radius[ptri->getId()], std::sqrt(ww*ww + hh*hh) );
        }
    }

    for( int attempt = 1; attempt <= max_attempts; attempt++ )
    {
        // Mark all blocks as unplaced.
        std::fill( currPlaced.begin(), currPlaced.end(), false );

        // Reset the block positions.
        for( size_t i = 0; i < blocks.size(); i++ ) 
        {
            ptri = blocks[i];

            ptri->setXmin( origXmin[ptri->getId()] );
            ptri->setXmax( origXmax[ptri->getId()] );
            ptri->setYmin( origYmin[ptri->getId()] );
            ptri->setYmax( origYmax[ptri->getId()] );
        }

        // Shuffle the order of the blocks.
        Utility::random_shuffle( blocks.begin(), blocks.end(), m_mgr->m_rng );
        if( attempt == 1 )
        {
            sorter._t = BlockSorter::A;
            std::stable_sort( blocks.begin(), blocks.end(), sorter );
            std::reverse( blocks.begin(), blocks.end() );
        } 

        // Reset the space manager then populate it with the preplaced blocks.
        sm.Reset( xmin, xmax, ymin, ymax );
        for( size_t i = 0; i < preplaced.size(); i++ ) 
        {
            ptri = preplaced[i];
            sm.AddFullRectangle( ptri->getXmin(), ptri->getXmax(), ptri->getYmin(), ptri->getYmax() );
        }

        currUnplaced = 0;
        for( size_t i = 0; i < blocks.size(); i++ )
        {
            ptri = blocks[i];

            // XXX: If this block corresponds to a cell which spans more than a single row,
            // should we consider alignment when finding rectangles?  It might be we ask
            // for and find only a single block into which the cell fits, but cannot be
            // aligned.  In this case, it might be better to have multiple blocks....
            bool getMultiSpots = false;
            Node* ndi = ptri->m_node;
            int spanned = -1; // Set to a +ve number if this is a cell.
            if( ndi != 0 )
            {
                spanned = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);
                if( spanned > 1 )
                {
                    getMultiSpots = true;
                }
            }

            // Query the space manager to get different sorts of 
            // rectangles into which the block can be placed.
            spots.erase( spots.begin(), spots.end() );
            if( spots.empty() )
            {
                // Try short range.
                sm.GetClosestRectanglesLarge( !getMultiSpots, 
                    ptri->getXmin(), 
                    ptri->getXmax(), 
                    ptri->getYmin(),
                    ptri->getYmax(),
                    spots, 
                    radius[ptri->getId()] 
                    );
            }
            if( spots.empty() )
            {
                // Try long range.
                sm.GetClosestRectanglesLarge( !getMultiSpots,
                    ptri->getXmin(), 
                    ptri->getXmax(), 
                    ptri->getYmin(),
                    ptri->getYmax(), 
                    spots, 
                    std::numeric_limits<double>::max() 
                    );
            }

            // If this is a multi-height cell, check the spots to see if one spot
            // is better than another for lining cells up.  Spots are sorted by
            // distance so it should be true that the first one found is going to
            // be a good pick.
            if( spots.size() > 1 && ndi != 0 && spanned > 1 )
            {
                for( size_t s = 0; s < spots.size(); s++ )
                {
                    Rectangle* p = spots[s];

                    bool good = false;
                    for( size_t row_c = 0; row_c < m_arch->m_rows.size(); row_c++ )
                    {
                        Architecture::Row* rptr = m_arch->m_rows[row_c];
                        double yb = rptr->getY();
                        double yt = yb + ndi->getHeight();
                        if( yb < p->ymin() || yt > p->ymax() )
                        {
                            continue;
                        }
                        bool flip = false;
                        if( spanned >= 2 && !m_arch->power_compatible( ndi, rptr, flip ) )
                        {
                            continue;
                        }
                        // Here, this rectangle offers a compatible position.
                        good = true;
                        break;
                    }
                    if( good )
                    {
                        // Put this rectangle first and then terminate.  We can remove
                        // all of the other rectangles.
                        if( s > 0 ) 
                        {
                            //std::cout << "Multi-height cell; skipping closest spot for alignment (picking " << s << "-th spot)." << std::endl;
                            std::swap( spots[0], spots[s] );
                        }
                        break;
                    }
                }
                spots.erase( spots.begin()+1, spots.end() );
            }

            // If we still have no spots, then this cell is not going
            // to get placed to our liking.
            if( spots.empty() ) 
            {
                currPlaced[ptri->getId()] = false;
                ++currUnplaced;
            } 
            else 
            {
                currPlaced[ptri->getId()] = true;

                // For the moment, pick the first spot.
                Rectangle* p = spots[0];

                // The cell must fit into the rectangle.
                if( ptri->getXmax() - ptri->getXmin() >= (p->xmax() - p->xmin())+1.0e-3 )
                {
                    ERRMSG( "Error, While repairing placement, found a block that does not fit inside returned rectangle." );
                    exit(-1);
                }
                if( ptri->getYmax() - ptri->getYmin() >= (p->ymax() - p->ymin())+1.0e-3 )
                {
                    ERRMSG( "Error, While repairing placement, found a block that does not fit inside returned rectangle." );
                    exit(-1);
                }

                shiftX = 0.;
                shiftY = 0.;

                double widX = ptri->getXmax() - ptri->getXmin();
                double posX = 0.5 * ( p->xmax() + p->xmin() );
                if( p->xmax() - p->xmin() > widX )
                {
                    if( ptri->getXmin() < p->xmin() )
                    {
                        shiftX = p->xmin() - ptri->getXmin();
                    }
                    else if( ptri->getXmax() > p->xmax() )
                    {
                        shiftX = p->xmax() - ptri->getXmax();
                    }
                    else
                    {
                        shiftX = 0.;
                    }
                } 
                else 
                {
                    shiftX = posX - 0.5*(ptri->getXmax() + ptri->getXmin());
                }

                double widY = ptri->getYmax() - ptri->getYmin();
                double posY = 0.5 * ( p->ymax() + p->ymin() );
                if( p->ymax() - p->ymin() > widY )
                {
                    if( ptri->getYmin() < p->ymin() )
                    {
                        shiftY = p->ymin() - ptri->getYmin();
                    }
                    else if( ptri->getYmax() > p->ymax() )
                    {
                        shiftY = p->ymax() - ptri->getYmax();
                    }
                    else
                    {
                        shiftY = 0.;
                    }
                } 
                else 
                {
                    shiftY = posY - 0.5*(ptri->getYmax() + ptri->getYmin());
                }

                // XXX: Should I try to align to rows or just leave it and hope the
                // overlap is removed??????????????????????????????????????????????
                Node* ndi = ptri->m_node;
                if( ndi != 0 )
                {
                    int spanned = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);
                    double bot = ptri->getYmin() + shiftY; // The new bottom.

                    // Find the closest row within the spot that still accommodates the cell.
                    int row_b = -1;
                    for( size_t row_c = 0; row_c < m_arch->m_rows.size(); row_c++ )
                    {
                        Architecture::Row* rptr = m_arch->m_rows[row_c];
                        // Find bottom and top of cell if the bottom of the cell is aligned.
                        // with this row.
                        double yb = rptr->getY();
                        double yt = yb + ndi->getHeight();
                        // If cell falls outside of the spot, then ignore this row.
                        if( yb < p->ymin() || yt > p->ymax() )
                        {
                            continue;
                        }

                        // So, we can line the cell up with this row, assuming the row is 
                        // compatible.  Single-height cells are always compatible.
                        bool flip = false;
                        if( spanned >= 2 && !m_arch->power_compatible( ndi, rptr, flip ) )
                        {
                            continue;
                        }

                        // Is this current, compatible, row closer to the target position we
                        // have already computed for this block?  If so, then record it.
                        if( row_b == -1 )
                        {
                            row_b = row_c;
                        }
                        else 
                        {
                            double dist_c = std::fabs( bot - m_arch->m_rows[row_c]->getY() );
                            double dist_b = std::fabs( bot - m_arch->m_rows[row_b]->getY() );
                            if( dist_c < dist_b )
                            {
                                row_b = row_c;
                            }
                        }
                    }
                    if( row_b != -1 )
                    {
                        // If here, then we have been able to adjust the cell position so that it
                        // aligns with voltage compatible rows.  Adjust the Y-shift.
                        if( bot < m_arch->m_rows[row_b]->getY() )
                        {
                            shiftY += (m_arch->m_rows[row_b]->getY()-bot); // Shift up a bit.
                        }
                        else if( bot > m_arch->m_rows[row_b]->getY() )
                        {
                            shiftY -= (bot-m_arch->m_rows[row_b]->getY());
                        }
                    }
                    else
                    {
                        if( currPlaced[ptri->getId()] )
                        {
                            currPlaced[ptri->getId()] = false;
                            ++currUnplaced;
                        }
                    }
                }

                ptri->setXmin( ptri->getXmin() + shiftX );
                ptri->setXmax( ptri->getXmax() + shiftX );
                ptri->setYmin( ptri->getYmin() + shiftY );
                ptri->setYmax( ptri->getYmax() + shiftY );

            }

            // Add this block to the space manager at its new location to block out that space.
            sm.AddFullRectangle( ptri->getXmin(), ptri->getXmax(), ptri->getYmin(), ptri->getYmax() );
        }

        // Measure the amount of movement with respect to the block
        // _INITIAL_ positions.
        if( currUnplaced > bestUnplaced )
        {
            currMovement = std::numeric_limits<double>::max();
        }
        else
        {
            currMovement = 0.;
            for( size_t i = 0; i < blocks.size(); i++ ) 
            {
                ptri = blocks[i];

                double distX = std::fabs( ptri->getXmin() - ptri->getXinitial() );
                double distY = std::fabs( ptri->getYmin() - ptri->getYinitial() );
                currMovement += distX + distY;
            }
        }

        // Decide if this solution is better than our previous best.  Better if
        // less unplaced cells or equal unplaced cells and less movement.
        bool keep = false;
        if( currUnplaced < bestUnplaced )
        {
            keep = true;
        } 
        else if( currUnplaced == bestUnplaced && currMovement < bestMovement )
        {
            keep = true;
        }

        if( keep )
        {
            bestMovement = currMovement;
            bestUnplaced = currUnplaced;

            for( size_t i = 0; i < blocks.size(); i++ ) 
            {
                ptri = blocks[i];

                bestPlaced[ptri->getId()] = currPlaced[ptri->getId()];
                bestXmin[ptri->getId()] = ptri->getXmin();
                bestXmax[ptri->getId()] = ptri->getXmax();
                bestYmin[ptri->getId()] = ptri->getYmin();
                bestYmax[ptri->getId()] = ptri->getYmax();
            }
        }

        if( currUnplaced == 0 )
        {
            break;
        }
    }

    std::cout << "Repair: "
            << "Unplaced cells is " << bestUnplaced << ", "
            << "Movement is " << bestMovement
            << std::endl;


    // Update the block placement to the best solution found.  
    for( size_t i = 0; i < blocks.size(); i++ ) 
    {
        ptri = blocks[i];

        ptri->setXmin( bestXmin[ptri->getId()] );
        ptri->setXmax( bestXmax[ptri->getId()] );
        ptri->setYmin( bestYmin[ptri->getId()] );
        ptri->setYmax( bestYmax[ptri->getId()] );

        ptri->m_isPlaced = bestPlaced[ptri->getId()];
    }

    return bestUnplaced;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int DwsmLegalizer::tetris_inner( double xmin, double xmax, double ymin, double ymax,
        std::vector<Block*>& preplaced, std::vector<Block*>& blocks )
{
    // Use tetris to try and legalize the rectangle.  Any blockages, etc.
    // have been encoded as blocks and added to the "preplaced" vector.
    // Those movable blocks that we will sort and pack are the ones that
    // we need to place.
    //
    // XXX: Don't call this routine directly!  It required block ids to
    // have been reset...
    //
    // When counting unplaced, I give preference to unplaced multi-height
    // cells since these are hard to fix.

    char buf[256];
    //sprintf( &buf[0], "tetris_start" );
    //draw( m_root, buf );

    std::set<int> debug;
//    debug.insert( 96038 );

    SpaceManager sm0;       // Generic; any height blocks.
    SpaceManager sm1;       // For single height block queries.
    BlockSorter sorter;
    std::vector<double> origXmin, origXmax, origYmin, origYmax;
    std::vector<double> bestXmin, bestXmax, bestYmin, bestYmax;
    Block* ptri;
    std::vector<bool> currPlaced;
    std::vector<bool> bestPlaced;
    std::vector<int> unplacedSingle;
    std::vector<int> unplacedMulti;
    std::vector<double> movement;
    std::vector<Rectangle*> spots;
    double shiftX, shiftY;
    std::vector<double> radius;
    double avgW = 0.;
    double avgH = 0.;


    sorter._xmin = xmin;
    sorter._xmax = xmax;
    sorter._ymin = ymin;
    sorter._ymax = ymax;

    bestXmin.resize( blocks.size() );
    bestXmax.resize( blocks.size() );
    bestYmin.resize( blocks.size() );
    bestYmax.resize( blocks.size() );

    origXmin.resize( blocks.size() );
    origXmax.resize( blocks.size() );
    origYmin.resize( blocks.size() );
    origYmax.resize( blocks.size() );

    for( size_t i = 0; i < blocks.size(); i++ ) 
    {
        ptri = blocks[i];

        origXmin[ptri->getId()] = ptri->getXmin();
        origXmax[ptri->getId()] = ptri->getXmax();
        origYmin[ptri->getId()] = ptri->getYmin();
        origYmax[ptri->getId()] = ptri->getYmax();

        bestXmin[ptri->getId()] = ptri->getXmin();
        bestXmax[ptri->getId()] = ptri->getXmax();
        bestYmin[ptri->getId()] = ptri->getYmin();
        bestYmax[ptri->getId()] = ptri->getYmax();
    }

    // Resizing.
    currPlaced.resize( blocks.size() );
    bestPlaced.resize( blocks.size() );

    // Determine radii for searching.
    avgW = 0.;
    avgH = 0.;
    for( size_t i = 0; i < preplaced.size(); i++ )
    {
        ptri = preplaced[i];

        double xx = std::max( xmin, ptri->getXmin() );
        double yy = std::max( ymin, ptri->getYmin() );
        double ww = std::min( xmax, ptri->getXmax() ) - xx;
        double hh = std::min( ymax, ptri->getYmax() ) - yy;

        avgW += ww;
        avgH += hh;
    }
    for( size_t i = 0; i < blocks.size(); i++ )
    {
        ptri = blocks[i];

        double ww = ptri->getXmax() - ptri->getXmin();
        double hh = ptri->getYmax() - ptri->getYmin();

        avgW += ww;
        avgH += hh;
    }
    avgW /= (double)(preplaced.size()+blocks.size());
    avgH /= (double)(preplaced.size()+blocks.size());

    radius.resize( blocks.size() );
    std::fill( radius.begin(), radius.end(), std::sqrt( avgW*avgW + avgH*avgH ));

    // Expand the radius for a block if it overlaps with something fixed; we
    // need to search farther to push it out of the fixed block.
    for( size_t i = 0; i < blocks.size(); i++ )
    {
        ptri = blocks[i];

        for( size_t j = 0; j < preplaced.size(); j++ ) 
        {
            if( ptri->getXmax() < preplaced[j]->getXmin() ) continue;
            if( ptri->getXmin() > preplaced[j]->getXmax() ) continue;
            if( ptri->getYmax() < preplaced[j]->getYmin() ) continue;
            if( ptri->getYmin() > preplaced[j]->getYmax() ) continue;

            double ww = (preplaced[j]->getXmax() - preplaced[j]->getXmin());
            double hh = (preplaced[j]->getYmax() - preplaced[j]->getYmin());
            radius[ptri->getId()] = std::max( radius[ptri->getId()], std::sqrt(ww*ww + hh*hh) );
        }
    }

    // Try tetris in each direction.
    int max_direction = 8;

    unplacedSingle.resize( max_direction+1 );
    std::fill( unplacedSingle.begin(), unplacedSingle.end(), 0 );
    unplacedMulti.resize( max_direction+1 );
    std::fill( unplacedMulti.begin(), unplacedMulti.end(), 0 );
    movement.resize( max_direction+1 );
    std::fill( movement.begin(), movement.end(), 0 );
    int best_direction = 0;
    for( int direction = 0; direction <= max_direction ; direction++ ) 
    {
        // Reset block positions.
        for( size_t i = 0; i < blocks.size(); i++ ) 
        {
            ptri = blocks[i];

            ptri->setXmin( origXmin[ptri->getId()] );
            ptri->setXmax( origXmax[ptri->getId()] );
            ptri->setYmin( origYmin[ptri->getId()] );
            ptri->setYmax( origYmax[ptri->getId()] );
        }

        // Sort blocks in specified direction.
        switch( direction ) 
        {
        case 0: sorter._t = BlockSorter::L ; break;
        case 1: sorter._t = BlockSorter::R ; break;
        case 2: sorter._t = BlockSorter::B ; break;
        case 3: sorter._t = BlockSorter::T ; break;
        case 4: sorter._t = BlockSorter::BL; break;
        case 5: sorter._t = BlockSorter::BR; break;
        case 6: sorter._t = BlockSorter::TL; break;
        case 7: sorter._t = BlockSorter::TR; break;
        case 8: sorter._t = BlockSorter::A ; break;
        default: 
            ERRMSG( "Error, Unexpected sort direction." );
            exit(-1);
            break;
        }
        std::sort( blocks.begin(), blocks.end(), sorter );

        // Mark blocks as unplaced.
        std::fill( currPlaced.begin(), currPlaced.end(), false );

        // Make two passes.  The first pass considers all blocks.  The second pass
        // considers blocks placed in the first pass as obstacles.  The second 
        // pass also ignores filler.  Consequently, the second pass attempts to
        // basically place those actual cells that did not get placed in the first
        // pass.
        for( int pass = 1; pass <= 1; pass++ )
        {
            // Reset space managers.
            sm0.Reset( xmin, xmax, ymin, ymax );
            //sm1.Reset( xmin, xmax, ymin, ymax );
            sm1.single( xmin, xmax, ymin, ymax, m_arch );
            // Populate with fixed blocks.
            for( size_t i = 0; i < preplaced.size(); i++ ) 
            {
                ptri = preplaced[i];
                sm0.AddFullRectangle( ptri->getXmin(), ptri->getXmax(), ptri->getYmin(), ptri->getYmax() );
                sm1.AddFullRectangle( ptri->getXmin(), ptri->getXmax(), ptri->getYmin(), ptri->getYmax() );
            }
            // Populate with placed blocks, unless they are filler which are skipped.
            // This should only happen in the second pass.
            if( pass == 2 )
            {
                for( size_t i = 0; i < blocks.size(); i++ ) 
                {
                    ptri = blocks[i];
                    if( ptri->m_node != 0 && currPlaced[ptri->getId()] )
                    {
                        // This block corresponds to a placed cell which we now consider
                        // as an obstacle.
                        sm0.AddFullRectangle( ptri->getXmin(), ptri->getXmax(), ptri->getYmin(), ptri->getYmax() );
                        sm1.AddFullRectangle( ptri->getXmin(), ptri->getXmax(), ptri->getYmin(), ptri->getYmax() );
                    }
                }
            }

            // Try to place each block.  In the second pass, skip the filler.
            for( size_t i = 0; i < blocks.size(); i++ )
            {
                ptri = blocks[i];

                if( pass == 2 && ptri->m_node == 0 )
                {
                    // Skip filler in the second pass.
                    continue;
                }
                if( currPlaced[ptri->getId()] )
                {
                    // Skip blocks that are placed.  This _should_ not happen in
                    // the first pass!
                    continue;
                }

                // Determine span of rows for the block.
                double height = ptri->getYmax() - ptri->getYmin();
                int spanned = (int)(height / m_singleRowHeight + 0.5);
                bool getMultiSpots = (spanned > 1 );

                SpaceManager* cursm = &sm0;
                if( spanned == 1 )
                {
                    cursm = &sm1;
                }

                Node* ndi = ptri->m_node;

                // Query space manager to get locations.
                //
                // XXX: I think there is an issue...  If we are querying within a radius
                // but still want multiple spots, it is possible that we will not find
                // a spot in which alignment can be performed.  I therefore think this 
                // needs to be enhanced...  I think we can improve the space manager to
                // also accept a radius, the cell and the architecture to ensure that we
                // look for a rectangle suitable for the row alignment...
                spots.erase( spots.begin(), spots.end() );
                if( spots.empty() )
                {
                        // Try short range.
                    if( spanned > 1 && ndi != 0 )
                    {
                        cursm->GetClosestRectanglesLarge( !getMultiSpots, 
                            ptri->getXmin(), 
                            ptri->getXmax(), 
                            ptri->getYmin(),
                            ptri->getYmax(),
                            spots, 
                            m_arch, ndi,
                            radius[ptri->getId()] 
                            );
                    }
                    else
                    {
                        cursm->GetClosestRectanglesLarge( !getMultiSpots, 
                            ptri->getXmin(), 
                            ptri->getXmax(), 
                            ptri->getYmin(),
                            ptri->getYmax(),
                            spots, 
                            radius[ptri->getId()] 
                            );
                    }
                }
                if( spots.empty() )
                {
                    // Try long range.
                    if( spanned > 1 && ndi != 0 )
                    {
                        cursm->GetClosestRectanglesLarge( !getMultiSpots,
                            ptri->getXmin(), 
                            ptri->getXmax(), 
                            ptri->getYmin(),
                            ptri->getYmax(), 
                            spots, 
                            m_arch, ndi,
                            std::numeric_limits<double>::max() 
                            );
                    }
                    else
                    {
                        cursm->GetClosestRectanglesLarge( !getMultiSpots,
                            ptri->getXmin(), 
                            ptri->getXmax(), 
                            ptri->getYmin(),
                            ptri->getYmax(), 
                            spots, 
                            std::numeric_limits<double>::max() 
                            );
                    }
                }

                // If the block is multi-height, then check spots to ensure we can line
                // up the block with a proper row.  If the block does not have an
                // associated cell, then any row is fine.  Note that we are not looking
                // for the best row, only if the spot is good.
                if( spanned > 1 )
                {
                  if( spots.size() > 0 )
                  {
                    bool good = false;
                    for( size_t s = 0; s < spots.size(); s++ )
                    {
                        Rectangle* p = spots[s];

                        for( size_t row_c = 0; row_c < m_arch->m_rows.size(); row_c++ )
                        {
                            Architecture::Row* rptr = m_arch->m_rows[row_c];
                            double yb = rptr->getY();
                            double yt = yb + (ptri->getYmax()-ptri->getYmin());
                            if( yb < p->ymin() || yt > p->ymax() )
                            {
                                continue;
                            }
                            bool flip = false;
                            if( ndi != 0 && !m_arch->power_compatible( ndi, rptr, flip ) )
                            {
                                continue;
                            }
                            // Here, this rectangle offers a compatible position.
                            good = true;
                            break;
                        }
                        if( good )
                        {
                            // Put this rectangle first and then terminate.  We can remove
                            // all of the other rectangles.
                            if( s > 0 ) 
                            {
                                std::swap( spots[0], spots[s] );
                            }
                            break;
                        }
                    }
                    spots.erase( spots.begin()+1, spots.end() );
                  }
                  else
                  {
                  }
                }

                // If no spots, mark as unplaced.
                if( spots.empty() ) 
                {
                    currPlaced[ptri->getId()] = false;
                } 
                else 
                {
                    currPlaced[ptri->getId()] = true;
                }

                if( spots.empty() )
                {
                    // Get anything.
                    cursm->GetClosestRectanglesRange( true,
                        ptri->getXmin(), 
                        ptri->getXmax(), 
                        ptri->getYmin(),
                        ptri->getYmax(), 
                        spots, 
                        std::numeric_limits<double>::max() 
                        );
                }

                // Based on what we found, shift the cell.
                if( spots.empty() ) 
                {
                    // No spots.  Thus, leave the cell alone unless it is outside
                    // of the region.
                    if( ptri->getXmin() < xmin ) 
                    {
                        shiftX = xmin - ptri->getXmin();
                    } 
                    else if( ptri->getXmax() > xmax ) 
                    {
                        shiftX = xmax - ptri->getXmax();
                    } 
                    else 
                    {
                        shiftX = 0.;
                    }
                    if( ptri->getYmin() < ymin ) 
                    {
                        shiftY = ymin - ptri->getYmin();
                    }
                    else if( ptri->getYmax() > ymax ) 
                    {
                        shiftY = ymax - ptri->getYmax();
                    }
                    else
                    {
                        shiftY = 0.;
                    }

                    ptri->setXmin( ptri->getXmin() + shiftX );
                    ptri->setXmax( ptri->getXmax() + shiftX );
                    ptri->setYmin( ptri->getYmin() + shiftY );
                    ptri->setYmax( ptri->getYmax() + shiftY );
                }
                else
                {
                    Rectangle* p = spots[0];

                    shiftX = 0.;
                    shiftY = 0.;

                    // Shift, depending on the sort direction.
                    switch( direction ) {
                    case 0: // L.
                        shiftX = p->xmin() - ptri->getXmin();
                        if( ptri->getYmin() < p->ymin() ) 
                        {
                            shiftY = p->ymin() - ptri->getYmin();
                        } 
                        else if( ptri->getYmax() > p->ymax() ) 
                        {
                            shiftY = p->ymax() - ptri->getYmax();
                        } 
                        else 
                        {
                            shiftY = 0.;
                        }
                        break;
                    case 1: // R.
                        shiftX = p->xmax() - ptri->getXmax();
                        if( ptri->getYmin() < p->ymin() ) 
                        {
                            shiftY = p->ymin() - ptri->getYmin();
                        }
                        else if( ptri->getYmax() > p->ymax() ) 
                        {
                            shiftY = p->ymax() - ptri->getYmax();
                        } 
                        else 
                        {
                            shiftY = 0.;
                        }
                        break;
                    case 2: // B.
                        shiftY = p->ymin() - ptri->getYmin();
                        if( ptri->getXmin() < p->xmin() ) 
                        {
                            shiftX = p->xmin() - ptri->getXmin();
                        }
                        else if( ptri->getXmax() > p->xmax() ) 
                        {
                            shiftX = p->xmax() - ptri->getXmax();
                        }
                        else 
                        {
                            shiftX = 0.;
                        }
                        break;
                    case 3: // T.
                        shiftY = p->ymax() - ptri->getYmax();
                        if( ptri->getXmin() < p->xmin() ) 
                        {
                            shiftX = p->xmin() - ptri->getXmin();
                        }
                        else if( ptri->getXmax() > p->xmax() ) 
                        {
                            shiftX = p->xmax() - ptri->getXmax();
                        }
                        else 
                        {
                            shiftX = 0.;
                        }
                        break;
                    case 4: // BL.
                        shiftY = p->ymin() - ptri->getYmin();
                        shiftX = p->xmin() - ptri->getXmin();
                        break;
                    case 5: // BR.
                        shiftY = p->ymin() - ptri->getYmin();
                        shiftX = p->xmax() - ptri->getXmax();
                        break;
                    case 6: // TL.
                        shiftY = p->ymax() - ptri->getYmax();
                        shiftX = p->xmin() - ptri->getXmin();
                        break;
                    case 7: // TR.
                        shiftY = p->ymax() - ptri->getYmax();
                        shiftX = p->xmax() - ptri->getXmax();
                        break;
                    case 8: // A.
                        if( ptri->getXmin() < p->xmin() ) 
                        {
                            shiftX = p->xmin() - ptri->getXmin();
                        }
                        else if( ptri->getXmax() > p->xmax() ) 
                        {
                            shiftX = p->xmax() - ptri->getXmax();
                        }
                        else 
                        {
                            shiftX = 0.;
                        }
                        if( ptri->getYmin() < p->ymin() ) 
                        {
                            shiftY = p->ymin() - ptri->getYmin();
                        } 
                        else if( ptri->getYmax() > p->ymax() ) 
                        {
                            shiftY = p->ymax() - ptri->getYmax();
                        } 
                        else 
                        {
                            shiftY = 0.;
                        }
                        break;
                    default:
                        ERRMSG( "Error, Unexpected sort direction." );
                        exit(-1);
                        break;
                    }

                    // Row alignment and extra shift.
                    {
                        double bot = ptri->getYmin() + shiftY; // The new bottom.

                        // Find the closest row within the spot that still accommodates the block.
                        int row_b = -1;
                        for( size_t row_c = 0; row_c < m_arch->m_rows.size(); row_c++ )
                        {
                            Architecture::Row* rptr = m_arch->m_rows[row_c];
                            // Bottom and top of block if aligned with this row.
                            double yb = rptr->getY();
                            double yt = yb + (ptri->getYmax()-ptri->getYmin());

                            // Test outside of spot.
                            if( yb <= p->ymin()-1.0e-3 || yt >= p->ymax()+1.0e-3 )
                            {
                                continue;
                            }

                            // Test compatibility.  Single height blocks or multi-height blocks
                            // with no associated cell are always compatible.
                            bool flip = false;
                            if( !(spanned == 1 || ndi == 0 ||
                                (spanned >= 2 && ndi != 0 && m_arch->power_compatible( ndi, rptr, flip ))) )
                            {
                                continue;
                            }

                            // Record if closer row.
                            bool keep = false;
                            if( !keep )
                            {
                                if( row_b == -1 )
                                {
                                    keep = true;
                                }
                            }
                            if( !keep )
                            {
                                double dist_c = std::fabs( bot - m_arch->m_rows[row_c]->getY() );
                                double dist_b = std::fabs( bot - m_arch->m_rows[row_b]->getY() );
                                if( dist_c < dist_b )
                                {
                                    keep = true;
                                }
                            }

                            if( keep )
                            {
                                row_b = row_c;
                            }
                        }
                        if( row_b != -1 )
                        {
                            // If here, then we have been able to adjust the cell position so that it
                            // aligns with voltage compatible rows.  Adjust the Y-shift.
                            if( bot < m_arch->m_rows[row_b]->getY() )
                            {
                                shiftY += (m_arch->m_rows[row_b]->getY()-bot); // Shift up a bit.
                            }
                            else if( bot > m_arch->m_rows[row_b]->getY() )
                            {
                                shiftY -= (bot-m_arch->m_rows[row_b]->getY());
                            }
                        }
                        else
                        {
                            // Not aligned, so not placed.
                            currPlaced[ptri->getId()] = false;
                        }
                    }

                    ptri->setXmin( ptri->getXmin() + shiftX );
                    ptri->setXmax( ptri->getXmax() + shiftX );
                    ptri->setYmin( ptri->getYmin() + shiftY );
                    ptri->setYmax( ptri->getYmax() + shiftY );
                }

                // Add this block to the space manager at its new location to block out that space.
                sm0.AddFullRectangle( ptri->getXmin(), ptri->getXmax(), ptri->getYmin(), ptri->getYmax() );
                sm1.AddFullRectangle( ptri->getXmin(), ptri->getXmax(), ptri->getYmin(), ptri->getYmax() );
            }

            // Count unplaced cells; only consider actual cells.
            unplacedSingle[direction] = 0;
            unplacedMulti[direction] = 0;
            for( size_t i = 0; i < blocks.size(); i++ )
            {
                ptri = blocks[i];

                Node* ndi = ptri->m_node;
                if( ndi != 0 && !currPlaced[ptri->getId()] )
                {
                    int spanned = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);
                    if( spanned == 1 )
                    {
                        ++unplacedSingle[direction];
                    }
                    else
                    {
                        ++unplacedMulti[direction];
                    }
                }
            }

            // If there are no unplaced cells after a pass, there is not need
            // to try the next pass.
            if( unplacedSingle[direction] == 0 && unplacedMulti[direction] == 0 )
            {
                break;
            }
        }

        // Meansure the movement.  I think we only really care about real cells.
        movement[direction] = 0.;
        for( size_t i = 0; i < blocks.size(); i++ ) 
        {
            ptri = blocks[i];
            if( ptri->m_node == 0 )
            {
                continue;
            }

            double distX = std::fabs( ptri->getXmin() - ptri->getXinitial() );
            double distY = std::fabs( ptri->getYmin() - ptri->getYinitial() );
            movement[direction] += distX + distY;
        }

        //sprintf( &buf[0], "tetris_%d", direction );
        //draw( m_root, buf );

        // Decide if the current placement is the best.
        bool keep = false;
        if( !keep )
        {
            if( direction == 0 )
            {
                keep = true;
            }
        }
        if( !keep )
        {
            if( unplacedMulti[direction] < unplacedMulti[best_direction] )
            {
                keep = true;
            }
        }
        if( !keep )
        {
            if( unplacedMulti[direction] == unplacedMulti[best_direction] )
            {
                if( unplacedSingle[direction] < unplacedSingle[best_direction] )
                {
                    keep = true;
                }
            }
        }
        if( !keep )
        {
            if( unplacedMulti[direction] == unplacedMulti[best_direction] )
            {
                if( unplacedSingle[direction] == unplacedSingle[best_direction] )
                {
                    if( movement[direction] < movement[best_direction] )
                    {
                        keep = true;
                    }
                }
            }
        }

        if( keep )
        {
            best_direction = direction;

            for( size_t i = 0; i < blocks.size(); i++ ) 
            {
                ptri = blocks[i];

                bestPlaced[ptri->getId()] = currPlaced[ptri->getId()];
                bestXmin[ptri->getId()] = ptri->getXmin();
                bestXmax[ptri->getId()] = ptri->getXmax();
                bestYmin[ptri->getId()] = ptri->getYmin();
                bestYmax[ptri->getId()] = ptri->getYmax();
            }
        }
    }

    //std::cout << "Tetris: best direction is " << best_direction << ", "
    //        << "unplaced cells is " 
    //        << unplacedSingle[best_direction]+unplacedMulti[best_direction] << ", "
    //        << "movement is " << movement[best_direction]
    //        << std::endl;


    // Update the block placement to the best solution found. 
    for( size_t i = 0; i < blocks.size(); i++ ) 
    {
        ptri = blocks[i];

        ptri->setXmin( bestXmin[ptri->getId()] );
        ptri->setXmax( bestXmax[ptri->getId()] );
        ptri->setYmin( bestYmin[ptri->getId()] );
        ptri->setYmax( bestYmax[ptri->getId()] );

        ptri->m_isPlaced = bestPlaced[ptri->getId()];
    }

    return (unplacedSingle[best_direction] + unplacedMulti[best_direction]);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DwsmLegalizer::draw(
    double xmin, double xmax, double ymin, double ymax,
    std::vector<Block*>& blocks0,
    std::vector<Block*>& blocks1,
    const std::string& str )
{
    Block* tmp = new Block( xmin, xmax, ymin, ymax );

    tmp->m_blocks.reserve( blocks0.size() + blocks1.size() );

    for( size_t i = 0; i < blocks0.size(); i++ ) 
    {
        tmp->m_blocks.push_back( blocks0[i] );
    }
    for( size_t i = 0; i < blocks1.size(); i++ ) 
    {
        tmp->m_blocks.push_back( blocks1[i] );
    }
    draw( tmp, str );
    delete tmp;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DwsmLegalizer::draw( Block *b, const std::string& str )
{
    const unsigned            BUFFER_SIZE = 2048;
    static char             buf[BUFFER_SIZE + 1];
    static char             filename[BUFFER_SIZE + 1];
    FILE                    *fp;
    unsigned                  i, n;
    static unsigned           counter = 0;
    double x = 0.; (void) x;
    double y = 0.; (void) y;
    double w = 0.;
    double h = 0.;
    double xMinBox = b->getXmin();
    double xMaxBox = b->getXmax();
    double yMinBox = b->getYmin();
    double yMaxBox = b->getYmax();
    std::vector<Block*>     blocks[3];
    double          x1, x2, y1, y2;
    double          xtol = 0.01*(xMaxBox-xMinBox);
    double          ytol = 0.01*(yMaxBox-yMinBox);
    std::string     buf1;

    blocks[0].reserve( b->m_blocks.size() );
    blocks[1].reserve( b->m_blocks.size() );
    blocks[2].reserve( b->m_blocks.size() );

    for( size_t i = 0; i < b->m_blocks.size(); i++ ) 
    {
        Block* ptr = b->m_blocks[i];
        if( ptr->m_isFixed )
        {
            blocks[0].push_back( ptr );
        } 
        else if( !ptr->m_isPlaced )
        {
            blocks[1].push_back( ptr );
        } 
        else 
        {
            blocks[2].push_back( ptr );
        }
    }

    std::cout << boost::format( "tetris.boxes.%05d.gp" ) % counter << std::endl;

    snprintf( filename, BUFFER_SIZE, "tetris.boxes.%05d.gp", counter );
    ++counter;

    if( ( fp = fopen( filename, "w" ) ) == 0 ) 
    {
        return;
    }

    // Output the header.
    fprintf( fp,    "# Use this file as a script for gnuplot\n"
                    "set title \"Tetris Plot (%s), X[%.2f,%.2f]Y[%.2f,%.2f]\" font \"Times, 8\"\n"
                    "set noxtics\n"
                    "set noytics\n"
                    "set nokey\n"
                    "set terminal x11\n"
                    "#Uncomment these two lines starting with \"set\"\n"
                    "#to save an EPS file for inclusion into a latex document\n"
                    "#set terminal postscript eps color solid 20\n"
                    "#set output \"%s\"\n\n",
//            (int) blocks[0].size(), (int) blocks[1].size(), (int) blocks[2].size(),
                    str.c_str(),
                    b->getXmin(), b->getXmax(), b->getYmin(), b->getYmax(),
                    filename );

    // Setup some line types.
    fprintf( fp, "set style line 1 lt 1 lw 2 lc rgb \"purple\"\n" );
    fprintf( fp, "set style line 2 lt 1 lw 1 lc rgb \"black\"\n" );
    fprintf( fp, "set style line 3 lt 1 lw 1 lc rgb \"green\"\n" );
    fprintf( fp, "set style line 4 lt 1 lw 1 lc rgb \"red\"\n" );
    fprintf( fp, "set style line 5 lt 1 lw 1 lc rgb \"orange\"\n" );
    fprintf( fp, "set style line 6 lt 1 lw 3 lc rgb \"blue\"\n" );
    fprintf( fp, "set style line 7 lt 1 lw 1 lc rgb \"brown\"\n" );

    // Output the plot command.
    buf1 = "plot[:][:] ";
    if( 1 || blocks[0].size() != 0 ) 
    {
        buf1 += "'-' with lines ls 1, ";
    }
    if( 1 || blocks[1].size() != 0 ) 
    {
        buf1 += "'-' with lines ls 3, ";
    }
    if( 1 || blocks[2].size() != 0 ) 
    {
        buf1 += "'-' with lines ls 4, ";
    }
    if( 1 || b->m_blocks.size() != 0 ) 
    {
        buf1 += "'-' with lines ls 5, ";
    }
    buf1 += "'-' with lines ls 6";

    fprintf( fp, "%s\n", buf1.c_str() );


    // Display nodes.
    for( unsigned type = 0; type <= 2 ; type++ ) 
    {
        if( blocks[type].empty() ) 
        {
            fprintf( fp, "\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "\n",
                        xMinBox, yMinBox,
                        xMinBox, yMaxBox,
                        xMaxBox, yMaxBox,
                        xMaxBox, yMinBox,
                        xMinBox, yMinBox );
        }
        for( size_t n = 0; n < blocks[type].size(); ++n ) 
        {
            Block *curr = blocks[type][n];

            w = curr->getXmax() - curr->getXmin();
            h = curr->getYmax() - curr->getYmin();

            xtol = 0;
            ytol = 0;
            if( !curr->m_isFixed )
            {
                // Shrink placed and unplaced to be able to see better.
                xtol = 0.02 * w;
                ytol = 0.02 * h;
            }
            x1 = curr->getXmin() + xtol;
            x2 = curr->getXmax() - xtol;
            y1 = curr->getYmin() + ytol;
            y2 = curr->getYmax() - ytol;

            fprintf( fp, "\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "\n",
                            x1, y1,
                            x1, y2,
                            x2, y2,
                            x2, y1,
                            x1, y1 );

            fprintf( fp, "\n" "%lf %lf\n" "%lf %lf\n" "\n", x2-0.10*w, y2, x2, y2-0.10*h );

            x = 0.5*(curr->getXmax()+curr->getXmin());
            y = 0.5*(curr->getYmax()+curr->getYmin());
        }

        fprintf(fp, "\nEOF\n" );
    }

    // Display displacement.
    for( size_t i = 0; i < b->m_blocks.size(); i++ ) 
    {
        Block* ptr = b->m_blocks[i];
        if( ptr->m_isFixed )
        {
            continue;
        }
        if( ptr->m_node == 0 )
        {
            continue;
        }

        x1 = 0.5 * (ptr->getXmax() + ptr->getXmin());
        y1 = 0.5 * (ptr->getYmax() + ptr->getYmin());

        x2 = ptr->getXinitial();
        y2 = ptr->getYinitial();

        fprintf( fp, "\n" "%lf %lf\n" "%lf %lf\n" "\n", x1, y1, x2, y2 );
    }

    fprintf( fp, "\nEOF\n" );


    // Output the bounding box ...
    fprintf( fp, "\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "\n",
                        xMinBox, yMinBox,
                        xMinBox, yMaxBox,
                        xMaxBox, yMaxBox,
                        xMaxBox, yMinBox,
                        xMinBox, yMinBox );

    fprintf( fp, "\nEOF\n" );

    // Cleanup.
    fclose( fp );
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int DwsmLegalizer::lal( DetailedMgr& mgr )
{
    // Use DWSM to legalize _ALL_ regions.
    std::cout << "Dwsm legalizer; lal." << std::endl;

    m_mgr = &mgr;

    m_arch = m_mgr->getArchitecture();
    m_network = m_mgr->getNetwork();
    m_rt = m_mgr->getRoutingParams();

    for( int regId = m_arch->m_regions.size(); regId > 0; )
    {
        --regId;
        lal( mgr, regId );
    }
    return 0;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int DwsmLegalizer::lal( DetailedMgr& mgr, int regId )
{
    std::cout << "\nDwsm legalizer; lal, running on region " << regId << std::endl;


    m_mgr = &mgr;

    m_arch = m_mgr->getArchitecture();
    m_network = m_mgr->getNetwork();
    m_rt = m_mgr->getRoutingParams();

    m_singleRowHeight = m_arch->m_rows[0]->getH();
    m_numSingleHeightRows = m_arch->m_rows.size();
    m_siteSpacing = m_arch->m_rows[0]->m_siteSpacing;

    m_curReg = regId;

    // Record original positions.
    m_origPosX.resize( m_network->m_nodes.size() );
    m_origPosY.resize( m_network->m_nodes.size() );
    std::fill( m_origPosX.begin(), m_origPosX.end(), 0.0 );
    std::fill( m_origPosY.begin(), m_origPosY.end(), 0.0 );
    for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node* ndi = &(m_network->m_nodes[i]);
        m_origPosX[ ndi->getId() ] = ndi->getX();
        m_origPosY[ ndi->getId() ] = ndi->getY();
    }

    // Set things up.  This creates blocks for all cells to be placed
    // as well as for fixed objects.  It also creates extra fixed 
    // objects to blackout space that we cannot use.
    init();

    lal( m_root );

    return 0;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int DwsmLegalizer::lal( Block* bptr )
{
    const int TAG_EMPTY         = 0x00000000;
    const int TAG_VISITED       = 0x00000001;
    const int TAG_EXPANDED      = 0x00000002;
    const int TAG_WORKING       = 0x00000004;


    std::cout << "\nDwsm legalizer; lal, running on block." << std::endl;

    // Create a grid.
    buildGrid( bptr );

    double xmin, xmax, ymin, ymax;
    double cap, occ;
    (void)cap; (void)occ;
    int l, r, b, t;
    int addedSinceUnplaced = 0;

    // Tags for buckets which are used to help ensure that we consider
    // all buckets over the passes.
    std::vector<std::vector<unsigned> > tags;
    tags.resize( m_xdim );
    for( int i = 0; i < m_xdim; i++ )
    {
        tags[i].resize( m_ydim );
        std::fill( tags[i].begin(), tags[i].end(), TAG_EMPTY );
    }

    // Passes.
    std::deque<std::pair<int,int> > Q;
    int maxPasses = 10;
    for( int pass = 1; pass <= maxPasses; pass++ )
    {
        populateGrid( bptr );
        buildDP();

        // Find unvisited overfilled buckets in the grid.  These are restricted to
        // buckets which have not been part of any previous problem; i.e., those 
        // marked as unvisited.
        int visited = 0;
        int buckets = 0;
        Q.erase( Q.begin(), Q.end() );
        for( int i = 0; i < m_xdim; i++ )
        {
            for( int j = 0; j < m_ydim; j++ )
            {
                if( (tags[i][j] & TAG_VISITED) == 0x0 )
                {
                    // This bucket has not been processed.
                    if( m_gridOcc[i][j] > m_gridCap[i][j] )
                    {
                        Q.push_back( std::make_pair( i, j ) );
                    }
                }
                visited += ((tags[i][j] & TAG_VISITED) != 0x0) ? 1: 0;
                buckets += 1;
            }
        }
        std::cout << "Pass " << pass << ", "
            << "Buckets is " << buckets << ", "
            << "Visited is " << visited << ", "
            << "Overfilled is " << Q.size()
            << std::endl;

        // TODO: If we did not find any seed buckets, perhaps we should also scan the
        // grid to see if there are buckets with unplaced blocks.  These could
        // possibly form seeds, since we do need to place all blocks...
        if( Q.size() == 0 )
        {
            for( int i = 0; i < m_xdim; i++ )
            {
                for( int j = 0; j < m_ydim; j++ )
                {
                    // Should we ignore the visited flag here?  It is possible that we
                    // visited this bucket before, but just failed.  This might allow
                    // us to try again???????????????????????????????????????????????
                    // Although, we already did have a shot at this bucket so perhaps
                    // there is no point and we need to clean this up later.
                    if( (tags[i][j] & TAG_VISITED) == 0x0 || addedSinceUnplaced > 0 )
                    {
                        bool isUnplaced = false;
                        for( size_t k = 0; k < m_blocksInGrid[i][j].size(); k++ )
                        {
                            Block* ptri = m_blocksInGrid[i][j][k];
                            if( !ptri->m_isPlaced )
                            {
                                isUnplaced = true;
                            }
                        }
                        if( isUnplaced )
                        {
                            Q.push_back( std::make_pair( i, j ) );
                        }
                    }
                }
            }
            std::cout << "Pass " << pass << ", "
                << "Added " << Q.size() << " seeds due to unplaced cells." << std::endl;

            ++addedSinceUnplaced;
        }

        // Break if there are no seeds.
        if( Q.size() == 0 )
        {
            break;
        }

        // Tag buckets.
        for( size_t i = 0; i < m_xdim; i++ )
        {
            for( size_t j = 0; j < m_ydim; j++ )
            {
                tags[i][j] &= ~TAG_WORKING;
            }
        }
        std::cout << "Pass " << pass << ", Overfilled buckets is " << Q.size() << std::endl;

        // Expand each of the seeds to get the potential regions in which
        // we will attempt to legalize.
        std::vector<Box> expanded_boxes;
        while( Q.size() > 0 )
        {
            std::pair<int,int> curr = Q.front();
            Q.pop_front();

            l = curr.first ;
            r = curr.first ;
            b = curr.second;
            t = curr.second;

            if( (tags[l][r] & TAG_WORKING) != 0x0 )
            {
                // The current seed has been absorbed into a region generated
                // by expanding from another seed.  There is no reason to 
                // consider this seed.
                continue;
            }

            expandGrid( l, r, b, t );
            if( r < l || t < b )
            {
                std::cout << "Error." << std::endl;
                exit(-1);
            }

            expanded_boxes.push_back( Box( l, r, b, t ) );

            for( int i = l; i <= r; i++ )
            {
                for( int j = b; j <= t; j++ )
                {
                    tags[i][j] |= TAG_WORKING;
                }
            }
        }
        std::cout << "Pass " << pass << ", "
            << "Expanded boxes is " << expanded_boxes.size() 
            << std::endl;

        std::stable_sort( expanded_boxes.begin(), expanded_boxes.end(), compareBoxes() );

        // Tag buckets.
        for( size_t i = 0; i < m_xdim; i++ )
        {
            for( size_t j = 0; j < m_ydim; j++ )
            {
                tags[i][j] &= ~TAG_WORKING;
            }
        }

        // Select the set of expanded, non-overlapping, rectangles which we
        // will attempt to legalize.  Note that we can either "solve as we
        // go" or save the set of boxes which we will legalize.  Solving as
        // we go prevents a few things...
        std::vector<Box> solved_boxes;
        for( size_t ix = 0; ix < expanded_boxes.size(); ix++ )
        {
            l = expanded_boxes[ix].m_l;
            r = expanded_boxes[ix].m_r;
            b = expanded_boxes[ix].m_b;
            t = expanded_boxes[ix].m_t;

            bool skip = false;
            for( int i = l; i <= r && !skip; i++ )
            {
                for( int j = b; j <= t && !skip; j++ )
                {
                    if( (tags[i][j] & TAG_WORKING) != 0x0 )
                    {
                        // This bucket has been involved in another problem in the current pass.
                        // This means we overlap and need to avoid this situation.
                        skip = true;
                    }
                }
            }
            if( skip )
            {
                continue;
            }

            for( int i = l; i <= r; i++ )
            {
                for( int j = b; j <= t; j++ )
                {
                    tags[i][j] |= TAG_WORKING; // Avoid overlapping rectangles.
                    tags[i][j] |= TAG_VISITED; // Avoid future seed selection.
                }
            }

            solved_boxes.push_back( expanded_boxes[ix] );
        }
        std::cout << "Pass " << pass << ", "
            << "Boxes to solve is " << solved_boxes.size() 
            << std::endl;

        // Now, build the blocks we need to solve.  Cells which are not
        // involved in a block must be considered as fixed.
        std::vector<Block*> solved_blocks;
        std::set<Block*> involved;
        for( size_t ix = 0; ix < solved_boxes.size(); ix++ )
        {
            l = solved_boxes[ix].m_l;
            r = solved_boxes[ix].m_r;
            b = solved_boxes[ix].m_b;
            t = solved_boxes[ix].m_t;

            xmin = std::max( m_arch->m_xmin + l * m_xwid         , m_arch->m_xmin );
            xmax = std::min( m_arch->m_xmin + r * m_xwid + m_xwid, m_arch->m_xmax );
            ymin = std::max( m_arch->m_ymin + b * m_ywid         , m_arch->m_ymin );
            ymax = std::min( m_arch->m_ymin + t * m_ywid + m_ywid, m_arch->m_ymax );

            Block* iptr = new Block( xmin, xmax, ymin, ymax );
            solved_blocks.push_back( iptr );

            // Truncate region due to regions.
            iptr->m_xmin = std::max( iptr->m_xmin, bptr->m_xmin );
            iptr->m_xmax = std::min( iptr->m_xmax, bptr->m_xmax );
            iptr->m_ymin = std::max( iptr->m_ymin, bptr->m_ymin );
            iptr->m_ymax = std::min( iptr->m_ymax, bptr->m_ymax );

            //std::cout << "Block to solve, "
            //    << "(" << l << "," << b << ")" << " -> "
            //    << "(" << r << "," << t << ")" << ", "
            //    << "(" << iptr->m_xmin << "," << iptr->m_ymin << ")" << " -> "
            //    << "(" << iptr->m_xmax << "," << iptr->m_ymax << ")" << ", "
            //    << std::endl;

            // Gather movable blocks inside.
            for( int i = l; i <= r; i++ )
            {
                for( int j = b; j <= t; j++ )
                {
                    iptr->m_blocks.insert( iptr->m_blocks.end(), 
                        m_blocksInGrid[i][j].begin(), m_blocksInGrid[i][j].end() 
                        );
                }
            }
            involved.insert( iptr->m_blocks.begin(), iptr->m_blocks.end() );
        }
        std::cout << "Pass " << pass << ", "
            << "Blocks to solve is " << solved_blocks.size() << ", "
            << "Involved cells is " << involved.size()
            << std::endl;

        // Temporarily fix any blocks not involved in a subproblem. 
        std::set<Block*> removed;
        for( size_t ix = 0; ix < bptr->m_blocks.size(); ix++ )
        {
            Block* ptri = bptr->m_blocks[ix];
            if( ptri->m_isFixed || involved.end() != involved.find( ptri ) )
            {
                continue;
            }
            removed.insert( ptri );
            ptri->m_isFixed = true;
        }
        std::cout << "Pass " << pass << ", "
            << "Temporarily fixed cells is " << removed.size()
            << std::endl;

        // Finally solve.  We need to insert any blocks that are not
        // involved in a subproblem that overlap with the block.
        for( size_t ix = 0; ix < solved_blocks.size(); ix++ )
        {
            Block* iptr = solved_blocks[ix];

            for( size_t i = 0; i < bptr->m_blocks.size(); i++ )
            {
                Block* ptri = bptr->m_blocks[i];
                if( !(ptri->m_isFixed || removed.end() != removed.find( ptri )) )
                {
                    continue;
                }

                // Overlapping?
                double xx = std::max( ptri->m_xmin, iptr->m_xmin );
                double yy = std::max( ptri->m_ymin, iptr->m_ymin );
                double ww = std::min( ptri->m_xmax, iptr->m_xmax ) - xx;
                double hh = std::min( ptri->m_ymax, iptr->m_ymax ) - yy;

                if( ww > 1.0e-3 && hh > 1.0e-3 && ww*hh > 1.0e-3 )
                {
                    iptr->m_blocks.push_back( ptri );
                }
            }

            solve( iptr, 1 );
            delete iptr;
        }

        // Unfix temporaries.
        for( std::set<Block*>::iterator it = removed.begin(); it != removed.end(); ++it )
        {
            Block* ptri = *it;
            ptri->m_isFixed = false;
        }
    }

    // We should scan the original block and determine the number of
    // unplaced cells.
    int unplaced = 0;
    for( size_t i = 0; i < bptr->m_blocks.size(); i++ )
    {
        Block* ptri = bptr->m_blocks[i];
        if( ptri->m_isFixed )
        {
            continue;
        }
        if( !ptri->m_isPlaced )
        {
            ++unplaced;
        }
    }
    std::cout << "Unplaced cells after lal is " << unplaced << std::endl;

    if( unplaced > 0 )
    {
        std::vector<Block*> placed;
        std::vector<Block*> fixed;
        std::vector<Block*> other;

        for( size_t i = 0; i < bptr->m_blocks.size(); i++ )
        {
            Block* ptri = bptr->m_blocks[i];
            if( ptri->m_isPlaced )
            {
                fixed.push_back( ptri );
            }
            else 
            {
                if( ptri->m_isPlaced )
                {
                    ptri->m_isFixed = true; // Temporary.
                    placed.push_back( ptri );
                    fixed.push_back( ptri );
                }
                else
                {
                    other.push_back( ptri );
                }
            }
        }

        // Temporary fixing.

        if( other.size() != 0 )
        {
            repair( bptr->m_xmin, bptr->m_xmax, bptr->m_ymin, bptr->m_ymax, fixed, other );
        }

        for( size_t i = 0; i < placed.size(); i++ )
        {
            Block* ptri = placed[i];
            ptri->m_isFixed = false;
        }

        // Recount.
        unplaced = 0;
        for( size_t i = 0; i < bptr->m_blocks.size(); i++ )
        {
            Block* ptri = bptr->m_blocks[i];
            if( ptri->m_isFixed )
            {
                continue;
            }
            if( !ptri->m_isPlaced )
            {
                ++unplaced;
            }
        }
        std::cout << "Unplaced cells after lal is now " << unplaced << std::endl;
    }

    return unplaced;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int DwsmLegalizer::solve( Block* b, int depth )
{
    // Solves a block either directly or by splitting and recursing.

    double occ = 0.;
    double cap = 0.;
    calculateOccAndCap( b, occ, cap );

    // Compute the average height/width of the UNPLACED cells.
    double singleRowHeight = m_arch->m_rows[0]->getH();
    const int maxTetrisPasses = 10; // How large???
    int     unplaced = 0;
    std::vector<double> bestXmin;
    std::vector<double> bestXmax;
    std::vector<double> bestYmin;
    std::vector<double> bestYmax;
    std::vector<bool> bestPlaced;
    std::map<Block*,int> tag;

    char buf[256];


    // Categorize the cells.
    std::set<Block*> preplaced;
    std::vector<Block*> placed;
    std::vector<Block*> other;
    std::vector<Block*> fixed;
    std::vector<Block*> small;
    for( size_t i = 0; i < b->m_blocks.size(); i++ )
    {
        Block* ptri = b->m_blocks[i];

        if( ptri->m_isFixed )
        {
            preplaced.insert( ptri );
            fixed.push_back( ptri );
        }
        else 
        {
            ptri->m_isPlaced = false;
            small.push_back( ptri );
        }
    }

    //for( size_t d = 0; d < depth; d++ ) std::cout << " ";
    //std::cout << boost::format( "Solving on block [%.2lf,%.2lf]->[%.2lf,%.2lf], " )
    //        % b->m_xmin % b->m_ymin % b->m_xmax % b->m_ymax;
    //std::cout << boost::format( "Preplaced blocks is %d/%d, Unplaced blocks is %d, " )
    //        % preplaced.size() % fixed.size() % small.size();
    //std::cout << boost::format( "Capacity is %.2e, Occupancy is %.2e, Density is %.2lf%%" )
    //        % cap % occ % (100.*(occ/cap)) << std::endl;

    
    // At this point, the only movable cells should be "what looks small".  If
    // the number of small cells left is reasonable, then just try to solve it.
    // Otherwise, split the block and recurse.  
    if( small.size() < m_params.m_smallProblemThreshold )
    {
        // Problem is smallish, so go after it in a single shot.
        other.erase( other.begin(), other.end() );
        other.insert( other.end(), small.begin(), small.end() );

        // Sometimes calling tetris multiple times can yield an improved result!
        int bestUnplaced = std::numeric_limits<int>::max();
        for( size_t i = 0; i < other.size(); i++ )
        {
            tag[other[i]] = i;
        }
        bestPlaced.resize( other.size() );
        bestXmin.resize( other.size() );
        bestXmax.resize( other.size() );
        bestYmin.resize( other.size() );
        bestYmax.resize( other.size() );
        for( int pass = 1; pass <= maxTetrisPasses; pass++ )
        {
            unplaced = tetris( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, other );
            //std::cout << "Pass " << pass << " of tetris, unplaced cells is " << unplaced << std::endl;
            if( unplaced == 0 )
            {
                break;
            }
            if( unplaced < bestUnplaced )
            {
                //std::cout << "Saved block placement with only " << unplaced << " unplaced blocks." << std::endl;
                bestUnplaced = unplaced;
                // Save placement.
                for( size_t i = 0; i < other.size(); i++ )
                {
                    Block* ptri = other[i];
                    int idx = tag[ptri];
                    bestPlaced[idx] = ptri->m_isPlaced;
                    bestXmin[idx] = ptri->getXmin();
                    bestXmax[idx] = ptri->getXmax();
                    bestYmin[idx] = ptri->getYmin();
                    bestYmax[idx] = ptri->getYmax();
                }
            }
        }
        if( unplaced > 0 && bestUnplaced < unplaced )
        {
            // Restore placement.
            unplaced = 0;
            for( size_t i = 0; i < other.size(); i++ )
            {
                Block* ptri = other[i];
                int idx = tag[ptri];
                ptri->m_isPlaced = bestPlaced[idx];
                ptri->setXmin( bestXmin[idx] );
                ptri->setXmax( bestXmax[idx] );
                ptri->setYmin( bestYmin[idx] );
                ptri->setYmax( bestYmax[idx] );
                if( !ptri->m_isPlaced )
                {
                    ++unplaced;
                }
            }
            //std::cout << "Restored block placement with " << unplaced << " unplaced blocks." << std::endl;
        }

        // If we have failed, is it worth removing some of the smaller single height
        // cells and re-attempting?  Ending with unplaced multi-height cells is bad
        // since they are much harder to deal with later on...  I expect that this is
        // only a problem when the density is quite high.
        if( unplaced > 0 )
        {
            std::vector<Block*> single;
            std::vector<Block*> multi;
            std::vector<Block*> misc;
            for( size_t i = 0; i < other.size(); i++ )
            {
                Block* ptri = other[i];
                Node* ndi = ptri->m_node;
                if( ndi != 0 )
                {
                    int spanned = (int)(ndi->getHeight() / singleRowHeight + 0.5);
                    if( spanned == 1 )
                    {
                        single.push_back( other[i] );
                    }
                    else
                    {
                        multi.push_back( other[i] );
                    }
                }
                else
                {
                    misc.push_back( other[i] );
                }
            }
            //std::cout << "Divided blocks, "
            //    << "Single height is " << single.size() << ", "
            //    << "Multi-height is " << multi.size() << ", "
            //    << "Miscellaneous is " << misc.size()
            //    << std::endl;

            if( multi.size() != 0 )
            {
                for( size_t i = 0; i < other.size(); i++ )
                {
                    Block* ptri = other[i];
                    ptri->m_isPlaced = false;
                }
                multi.insert( multi.end(), misc.begin(), misc.end() );
                misc.clear();

                // Only multi-height.
                int up0 = tetris( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, multi );

                // Fix the multi-height.
                for( size_t i = 0; i < multi.size(); i++ )
                {
                    multi[i]->m_isFixed = true;
                    fixed.push_back( multi[i] );
                }

                // Only the single height.
                int up1 = tetris( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, single );

                // Undo multi-height fix.
                for( size_t i = 0; i < multi.size(); i++ )
                {
                    multi[i]->m_isFixed = false;
                }

                //std::cout << "Divided blocks, Unplaced single height cells is " << up1 << ", "
                //    << "Unplaced multi-height cells is " << up0 << std::endl;
            }
        }
    }
    else
    {
        other.erase( other.begin(), other.end() );
        other.insert( other.end(), small.begin(), small.end() );
        Block* p0;
        Block* p1;
        //std::cout << "Blocks before split is " << b->m_blocks.size() << std::endl;
        split( b, p0, p1, fixed, other );
        int up0 = solve( p0, depth+1 );
        int up1 = solve( p1, depth+1 );

        // Undo the partitioning as we "back up". 
        if( b->m_blocks.size() != 2 || p0 == 0 || p1 == 0 )
        {
            ERRMSG( "Error, Problem when merging partitions." );
            exit(-1);
        }
        std::set<Block*> merged;
        merged.insert( p0->m_blocks.begin(), p0->m_blocks.end() );
        merged.insert( p1->m_blocks.begin(), p1->m_blocks.end() );
        delete p0;
        delete p1;
        b->m_blocks.erase( b->m_blocks.begin(), b->m_blocks.end() );
        b->m_blocks.insert( b->m_blocks.end(), merged.begin(), merged.end() );

        unplaced = up0 + up1;
        if( up0 > 0 || up1 > 0 )
        {
            // We have unplaced cells.  We can try and repair the placement.

            placed.erase( placed.begin(), placed.end() );
            fixed.erase( fixed.begin(), fixed.end() );
            other.erase( other.begin(), other.end() );

            for( size_t i = 0; i < b->m_blocks.size(); i++ )
            {
                Block* ptri = b->m_blocks[i];

                if( ptri->m_isPlaced && !ptri->m_isFixed )
                {
                    // Placed cells (but not fixed).
                    placed.push_back( ptri );
                }
                else if( (ptri->m_isPlaced && ptri->m_isFixed) || preplaced.end() != preplaced.find( ptri ) )
                {
                    // Fixed cells.  Either prelaced cells or cells that have been fixed somehow.
                    if( !ptri->m_isFixed )
                    {
                        ERRMSG( "Error, Fixed cell expected." );
                        exit(-1);
                    }
                    fixed.push_back( ptri );
                }
                else
                {
                    // Unplaced cells.
                    if( ptri->m_isPlaced )
                    {
                        ERRMSG( "Error, Expecting an unplaced cell when merging partitions." );
                        exit(-1);
                    }
                    other.push_back( ptri );
                }
            }

            if( other.size() == 0 || other.size() != up0 + up1 )
            {
                sprintf( &buf[0], "Error, Mismatch in unplaced cells when merging partitions, Expected %d, Found %d",
                        up0+up1, (int)other.size() );
                ERRMSG( buf );
                exit(-1);
            }

            // Temporarily fix all of the placed cells.
            for( size_t i = 0; i < placed.size(); i++ )
            {
                placed[i]->m_isFixed = true;
                fixed.push_back( placed[i] );
            }

            // Try to place the unplaced cells.
            unplaced = repair( b->m_xmin, b->m_xmax, b->m_ymin, b->m_ymax, fixed, other );

            // Undo temporarily fixed cells.
            for( size_t i = 0; i < placed.size(); i++ )
            {
                placed[i]->m_isFixed = false;
            }
        }
    }

    // We need to properly count the number of unplaced cell in this block.  The
    // only cells we should ignore are those that are "preplaced".
    unplaced = 0;
    for( size_t i = 0; i < b->m_blocks.size(); i++ ) 
    {
        Block* ptri = b->m_blocks[i];
        if( preplaced.end() != preplaced.find( ptri ) )
        {
            continue;
        }
        if( !ptri->m_isPlaced )
        {
            ++unplaced;
        }
    }

    // Return the number of unplaced items.
    //if( 1 || unplaced != 0 )
    //{
    //    for( size_t d = 0; d < depth; d++ ) std::cout << " ";
    //    std::cout << boost::format( "Solving on block [%.2lf,%.2lf]->[%.2lf,%.2lf], Unplaced cells is %d" )
    //        % b->m_xmin % b->m_ymin % b->m_xmax % b->m_ymax % unplaced << std::endl;
    //}
    return unplaced;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool DwsmLegalizer::expandGrid( int& l, int& r, int& b, int& t )
{
    double aspect = 1.0;
    const double max_aspect = 3.00;
    bool done = false;
    double occ = 0.;
    double cap = 0.;
    const double tol = 0.9600; // Tries to leave some space for flexibility.

    int best_l = 0;
    int best_b = 0;
    int best_r = m_xdim-1;
    int best_t = m_ydim-1;

    double best_w = m_arch->m_xmax - m_arch->m_xmin;
    double best_h = m_arch->m_ymax - m_arch->m_ymin;
    double best_a = best_w * best_h;

    while( aspect <= max_aspect )
    {
        int max_rx = std::max( m_xdim-1-r, l );
        int max_ry = std::max( m_ydim-1-t, b );
        for( int rx = 0; rx <= max_rx; rx++ )
        {
            int new_l = std::max( l-rx,        0 );
            int new_r = std::min( r+rx, m_xdim-1 );

            for( int ry = 0; ry <= max_ry; ry++ )
            {
                int new_b = std::max( b-ry,        0 );
                int new_t = std::min( t+ry, m_ydim-1 );

                double xmin = m_arch->m_xmin + new_l * m_xwid;
                double xmax = m_arch->m_xmin + new_r * m_xwid + m_xwid;
                double ymin = m_arch->m_ymin + new_b * m_ywid;
                double ymax = m_arch->m_ymin + new_t * m_ywid + m_ywid;
                xmax = std::min( xmax, m_arch->m_xmax );
                ymax = std::min( ymax, m_arch->m_ymax );

                double new_w = xmax - xmin;
                double new_h = ymax - ymin;
                double new_a = new_w * new_h;

                // HACK.  Sometimes, we can get a bin with a small height
                // at the top of the grid.  So, make sure the height is
                // always at least what is expected.
                if( new_h <= m_ywid-1.0e-3 )
                {
                    continue;
                }

                if( new_w > aspect * new_h )
                {
                    continue;
                }
                if( new_h > aspect * new_w || new_a > best_a )
                {
                    break;
                }
                cap = ( m_gridCap_DP[new_r][new_t]
                            - ((new_l != 0) ? m_gridCap_DP[new_l-1][new_t] : 0.0)
                            - ((new_b != 0) ? m_gridCap_DP[new_r][new_b-1] : 0.0)
                            + ((new_l != 0 && new_b != 0) ? m_gridCap_DP[new_l-1][new_b-1] : 0.0))
                            ;

                occ = ( m_gridOcc_DP[new_r][new_t]
                            - ((new_l != 0) ? m_gridOcc_DP[new_l-1][new_t] : 0.0)
                            - ((new_b != 0) ? m_gridOcc_DP[new_r][new_b-1] : 0.0)
                            + ((new_l != 0 && new_b != 0) ? m_gridOcc_DP[new_l-1][new_b-1] : 0.0))
                            ;

                if( occ < tol * cap )
                {
                    if( !( new_h > aspect * new_w /* Too tall */ || new_w > aspect * new_h /* Too wide */ ) )
                    {
                        if( !done || new_a < best_a )
                        {
                            best_l = new_l;
                            best_r = new_r;
                            best_b = new_b;
                            best_t = new_t;

                            best_w = new_w;
                            best_h = new_h;
                            best_a = new_a;

                            done = true;
                        }
                    }
                }
            }
        }
        //if( !done ) {
        //    aspect += 1.0;
        //}
        aspect += 1.0;
    }

    l = best_l;
    r = best_r;
    b = best_b;
    t = best_t;

    return done;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DwsmLegalizer::buildGrid( Block* bptr )
{
    // Size the grid.  Keep it below a certain size.
    double avgW = 0.;
    double maxW = 0.;
    double avgH = 0.;
    double maxH = 0.;
    int count = 0;
    for( int i = 0; i < bptr->m_blocks.size(); i++ )
    {
        Block* ptri = bptr->m_blocks[i];

        Node* ndi = ptri->m_node;
        if( ptri->m_isFixed )
        {  
            continue;
        }

        double width = ptri->getXmax() - ptri->getXmin();
        avgW += width;
        if( ndi != 0 ) maxW = std::max( maxW, width );
        double height = ptri->getYmax() - ptri->getYmin(); 
        avgH += height;
        maxH  = std::max( maxH, height );

        ++count;
    }
    avgW /= (double)count;
    avgH /= (double)count;

    // XXX: This overlays a grid on top of the entire placement region.  It's fine...
    double H = m_arch->m_ymax - m_arch->m_ymin;
    double W = m_arch->m_xmax - m_arch->m_xmin;
    (void)H; (void)W;

    // Maximum grid size.
    m_maxGridDimY = 512;
    m_maxGridDimX = 512;

    // For multiheight cells, we will fail if the bucket is not large enough to
    // allow for some movement up and down.

    int temp = (int) std::ceil( (double)m_numSingleHeightRows / (double)m_maxGridDimY );
    int span = (int)(maxH/m_singleRowHeight + 0.5);

    m_numRowsPerGridCell = std::max( 2 * span + 1, temp );
    m_ywid = m_numRowsPerGridCell * m_singleRowHeight;
    m_ydim = 1;
    while( m_ydim * m_ywid < H )
    {
        ++m_ydim;
    }

    m_xwid = 5.0000 * maxW;
    std::cout << "Maximum cell width is " << maxW << ", Target width is " << m_xwid << std::endl;
    m_xdim = 1;
    while( m_xdim * m_xwid < W )
    {
        ++m_xdim;
    }
    m_xdim = std::min( m_xdim, m_ydim );
    m_xwid = W / (double) m_xdim;

    std::cout << "Grid is " << m_xdim << " x " << m_ydim << ", " 
        << "Rows per grid cell is " << m_numRowsPerGridCell << ", "
        << "Covers rows up to " << m_ydim * m_numRowsPerGridCell << " (" << m_arch->m_rows.size() << ")" << ", "
        << m_arch->m_ymin + m_ydim * m_ywid << " x " << m_arch->m_xmin + m_xdim * m_xwid << ", "
        << "Dimensions are " << m_ywid << " x " << m_xwid << ", " << "Span is " << span
             << std::endl;


    // Structures for capacity and occupancy.
    m_gridCap.resize( m_xdim );
    m_gridOcc.resize( m_xdim );
    for( int i = 0; i < m_xdim; i++ )
    {
        m_gridCap[i].resize( m_ydim );
        m_gridOcc[i].resize( m_ydim );
        for( int j = 0; j < m_ydim; j++ )
        {
            double lx = std::max( m_arch->m_xmin, m_arch->m_xmin + i * m_xwid );
            double rx = std::min( m_arch->m_xmax, m_arch->m_xmin + i * m_xwid + m_xwid );
            double yb = std::max( m_arch->m_ymin, m_arch->m_ymin + j * m_ywid );
            double yt = std::min( m_arch->m_ymax, m_arch->m_ymin + j * m_ywid + m_ywid );

            m_gridCap[i][j] = 0.0;
            m_gridOcc[i][j] = 0.0;

            // Default capacity is the overlap with the top block.
            double xx = std::max( bptr->m_xmin, lx );
            double yy = std::max( bptr->m_ymin, yb );
            double ww = std::min( bptr->m_xmax, rx ) - xx;
            double hh = std::min( bptr->m_ymax, yt ) - yy;

            if( ww > 1.0e-3 && hh > 1.0e-3 && ww*hh > 1.0e-3 )
            {
                m_gridCap[i][j] = std::max( 0.0, ww*hh );
            }
        }
    }

    // Reduce capacity based on the overlap with fixed cells and/or other blockages.
    for( int ix = 0; ix < bptr->m_blocks.size(); ix++ )
    {
        Block* ptri = bptr->m_blocks[ix];
        if( !ptri->m_isFixed )
        {
            continue;
        }

        double xmin = ptri->getXmin();
        double xmax = ptri->getXmax();
        double ymin = ptri->getYmin();
        double ymax = ptri->getYmax();

        int l = std::min(std::max( (int)(floor((xmin - m_arch->m_xmin)/m_xwid)), 0 ), (m_xdim-1) );
        int r = std::min(std::max( (int)(floor((xmax - m_arch->m_xmin)/m_xwid)), 0 ), (m_xdim-1) );
        int b = std::min(std::max( (int)(floor((ymin - m_arch->m_ymin)/m_ywid)), 0 ), (m_ydim-1) );
        int t = std::min(std::max( (int)(floor((ymax - m_arch->m_ymin)/m_ywid)), 0 ), (m_ydim-1) );
        for( int i = l; i <= r; i++ )
        {
            for( int j = b; j <= t; j++ )
            {
                double lx = std::max( m_arch->m_xmin, m_arch->m_xmin + i * m_xwid );
                double rx = std::min( m_arch->m_xmax, m_arch->m_xmin + i * m_xwid + m_xwid );
                double yb = std::max( m_arch->m_ymin, m_arch->m_ymin + j * m_ywid );
                double yt = std::min( m_arch->m_ymax, m_arch->m_ymin + j * m_ywid + m_ywid );

                double xx = std::max( bptr->m_xmin, lx );
                double yy = std::max( bptr->m_ymin, yb );
                double ww = std::min( bptr->m_xmax, rx ) - xx;
                double hh = std::min( bptr->m_ymax, yt ) - yy;

                if( ww >= 1e-3 && hh >= 1e-3 )
                {
                    m_gridCap[i][j] = std::max( 0.0, m_gridCap[i][j] - ww*hh );
                }
            }
        }
    }
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DwsmLegalizer::populateGrid( Block* bptr )
{
    // Clear grid occupancies and then insert the movable blocks into the grid.
    m_blocksInGrid.resize( m_xdim );
    for( int i = 0; i < m_xdim; i++ )
    {
        m_blocksInGrid[i].resize( m_ydim );
        for( int j = 0; j < m_ydim; j++ )
        {
            m_blocksInGrid[i][j].clear();

            m_gridOcc[i][j] = 0.0;
        }
    }

    double x, y, w, h;
    int l, b;
    for( int i = 0; i < bptr->m_blocks.size(); i++ )
    {
        Block* ptri = bptr->m_blocks[i];
        if( ptri->m_isFixed )
        {
            continue;
        }

        x = 0.5 * (ptri->getXmax() + ptri->getXmin());
        y = 0.5 * (ptri->getYmax() + ptri->getYmin());
        w = ptri->getXmax() - ptri->getXmin();
        h = ptri->getYmax() - ptri->getYmin();

        l = std::min(std::max( (int)(floor((x-m_arch->m_xmin)/m_xwid)), 0 ), (m_xdim-1) );
        b = std::min(std::max( (int)(floor((y-m_arch->m_ymin)/m_ywid)), 0 ), (m_ydim-1) );

        m_blocksInGrid[l][b].push_back( ptri );
        m_gridOcc[l][b] += w * h;
    }
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DwsmLegalizer::buildDP( void )
{
    m_gridOcc_DP.resize(m_xdim);
    m_gridCap_DP.resize(m_xdim);
    for(int i = 0; i < m_xdim; i++ ) 
    {
        m_gridOcc_DP[i].resize(m_ydim);
        m_gridCap_DP[i].resize(m_ydim);

        std::fill( m_gridOcc_DP[i].begin(), m_gridOcc_DP[i].end(), 0.0 );
        std::fill( m_gridCap_DP[i].begin(), m_gridCap_DP[i].end(), 0.0 );
    }

    m_gridOcc_DP[0][0] = std::max( m_gridOcc[0][0], 0.0 ); // = MOVEABLE cells.
    m_gridCap_DP[0][0] = std::max( m_gridCap[0][0], 0.0 ); // = ADJUSTED SPACE for MOVEABLE cells.

    for( int i = 1; i < m_xdim; i++ ) {
        m_gridOcc_DP[i][0] = std::max( m_gridOcc[i][0], 0.0 ); // = MOVEABLE cells.
        m_gridCap_DP[i][0] = std::max( m_gridCap[i][0], 0.0 ); // = ADJUSTED SPACE for MOVEABLE cells.

        m_gridOcc_DP[i][0] = std::max( m_gridOcc_DP[i][0] + m_gridOcc_DP[i-1][0], 0.0 );
        m_gridCap_DP[i][0] = std::max( m_gridCap_DP[i][0] + m_gridCap_DP[i-1][0], 0.0 );
    }

    for( int j = 1; j < m_ydim; j++ ) {
        m_gridOcc_DP[0][j] = std::max( m_gridOcc[0][j], 0.0 ); // = MOVEABLE cells.
        m_gridCap_DP[0][j] = std::max( m_gridCap[0][j], 0.0 ); // = ADJUSTED SPACE for MOVEABLE cells.

        m_gridOcc_DP[0][j] = std::max( m_gridOcc_DP[0][j] + m_gridOcc_DP[0][j-1], 0.0 );
        m_gridCap_DP[0][j] = std::max( m_gridCap_DP[0][j] + m_gridCap_DP[0][j-1], 0.0 );
    }

    for( int i = 1; i < m_xdim; i++ ) {
        for( int j = 1; j < m_ydim; j++ ) {
            m_gridOcc_DP[i][j] = std::max( m_gridOcc[i][j], 0.0 ); // = MOVEABLE cells.
            m_gridCap_DP[i][j] = std::max( m_gridCap[i][j], 0.0 ); // = ADJUSTED SPACE for MOVEABLE cells.

            m_gridOcc_DP[i][j] = std::max( m_gridOcc_DP[i][j] + m_gridOcc_DP[i][j-1] + m_gridOcc_DP[i-1][j] - m_gridOcc_DP[i-1][j-1], 0.0 );
            m_gridCap_DP[i][j] = std::max( m_gridCap_DP[i][j] + m_gridCap_DP[i][j-1] + m_gridCap_DP[i-1][j] - m_gridCap_DP[i-1][j-1], 0.0 );
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool DwsmLegalizer::mmfSetupProblem( std::vector<Block *> &preplaced, std::vector<Block *> &blocks )
{
    // Floorplanner uses a different type of block so we need to convert.

    Block               *blk;
    int                 count = 0;
    int                 i;
    MinMovementFloorplanner::Block  *mmfBlk;

    m_mmfpBlocks.clear();
    m_mmfpBlocks.reserve( preplaced.size() + blocks.size() );

    m_mmfpBlocksToBlockMap.clear();

    for( i = 0; i < blocks.size(); i++ ) 
    {
        blk = blocks[i];

        // Cannot handle zero dimension cells.  Screws up the floorplanner.
        if( (blk->getXmax() - blk->getXmin()) < 1e-3 )
        {
            continue;
        }
        if( (blk->getYmax() - blk->getYmin()) < 1e-3 ) 
        {
            continue;
        }

        mmfBlk = new MinMovementFloorplanner::Block;
        mmfBlk->GetId() = count;
        mmfBlk->SetFixed( false );
        mmfBlk->SetPlaced( blk->m_isPlaced );
        mmfBlk->SetResized( false );
        mmfBlk->GetDim(MinMovementFloorplanner::HGraph) = blk->getXmax() - blk->getXmin();
        mmfBlk->GetDim(MinMovementFloorplanner::VGraph) = blk->getYmax() - blk->getYmin();
        mmfBlk->GetPos(MinMovementFloorplanner::HGraph) = 0.5 * ( blk->getXmax() + blk->getXmin() );
        mmfBlk->GetPos(MinMovementFloorplanner::VGraph) = 0.5 * ( blk->getYmax() + blk->getYmin() );
        mmfBlk->GetOrigDim(MinMovementFloorplanner::HGraph) = mmfBlk->GetDim(MinMovementFloorplanner::HGraph);
        mmfBlk->GetOrigDim(MinMovementFloorplanner::VGraph) = mmfBlk->GetDim(MinMovementFloorplanner::VGraph);
        mmfBlk->GetOrigPos(MinMovementFloorplanner::HGraph) = blk->getXinitial();
        mmfBlk->GetOrigPos(MinMovementFloorplanner::VGraph) = blk->getYinitial();
        // XXX: Currently, we aren't replicating any rotational information.

        m_mmfpBlocks.push_back( mmfBlk );
        m_mmfpBlocksToBlockMap[blk] = mmfBlk;
        ++count;
    }

    if( m_mmfpBlocks.empty() ) 
    {
        // We have nothing to legalize.
        return false;
    }

    for( i = 0; i < preplaced.size(); i++ ) 
    {
        blk = preplaced[i];

        // We simply can't legalize zero-dimensional cells -- it screws
        // up the sequence pairs in the floorplanner.
        if( (blk->getXmax() - blk->getXmin()) < 1e-3 )
        {
            continue;
        }
        if( (blk->getYmax() - blk->getYmin()) < 1e-3 ) 
        {
            continue;
        }
        mmfBlk = new MinMovementFloorplanner::Block;
        mmfBlk->GetId() = count;
        mmfBlk->SetFixed( true );
        mmfBlk->SetPlaced( blk->m_isPlaced );
        mmfBlk->SetResized( false );
        mmfBlk->GetDim(MinMovementFloorplanner::HGraph) = blk->getXmax() - blk->getXmin();
        mmfBlk->GetDim(MinMovementFloorplanner::VGraph) = blk->getYmax() - blk->getYmin();
        mmfBlk->GetPos(MinMovementFloorplanner::HGraph) = 0.5 * ( blk->getXmax() + blk->getXmin() );
        mmfBlk->GetPos(MinMovementFloorplanner::VGraph) = 0.5 * ( blk->getYmax() + blk->getYmin() );
        mmfBlk->GetOrigDim(MinMovementFloorplanner::HGraph) = mmfBlk->GetDim(MinMovementFloorplanner::HGraph);
        mmfBlk->GetOrigDim(MinMovementFloorplanner::VGraph) = mmfBlk->GetDim(MinMovementFloorplanner::VGraph);
        mmfBlk->GetOrigPos(MinMovementFloorplanner::HGraph) = mmfBlk->GetPos(MinMovementFloorplanner::HGraph);
        mmfBlk->GetOrigPos(MinMovementFloorplanner::VGraph) = mmfBlk->GetPos(MinMovementFloorplanner::VGraph);
        // XXX: Currently, we aren't replicating any rotational information.

        m_mmfpBlocks.push_back( mmfBlk );
        m_mmfpBlocksToBlockMap[blk] = mmfBlk;
        ++count;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int DwsmLegalizer::mmfFloorplan( 
            double xmin, double xmax, double ymin, double ymax,
            std::vector<Block*> &preplaced, std::vector<Block*> &blocks )
{
    if( !mmfSetupProblem( preplaced, blocks ) )
    {
        // This means there was nothing to legalize, so nothing unplaced.
        return 0;
    }

    MinMovementFloorplannerParams params;
    MinMovementFloorplanner mmf( params, m_mgr->m_rng );
    mmf.minshift( *m_mgr, xmin, xmax, ymin, ymax, m_mmfpBlocks );

    // Copy solution from floorplanning back to the blocks.
    int unplaced = 0;
    for( size_t i = 0; i < blocks.size(); i++ )
    {
        Block* ptri = blocks[i];
        std::map<Block*,MinMovementFloorplanner::Block*>::iterator it = m_mmfpBlocksToBlockMap.find( ptri );
        if( m_mmfpBlocksToBlockMap.end() == it )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
        MinMovementFloorplanner::Block* mmfBlk = it->second;

        // A block is considered unplaced if did not get placed or got resized.
        if( !mmfBlk->IsPlaced() || mmfBlk->IsResized() )
        {
            unplaced = true;
        }

        double x = mmfBlk->GetPos(MinMovementFloorplanner::HGraph);
        double y = mmfBlk->GetPos(MinMovementFloorplanner::VGraph);
        double w = ptri->getXmax() - ptri->getXmin();
        double h = ptri->getYmax() - ptri->getYmin();

        ptri->setXmax( x + 0.5*w );
        ptri->setXmin( x - 0.5*w );
        ptri->setYmax( y + 0.5*h );
        ptri->setYmin( y - 0.5*h );
    }

    // Cleanup.
    for( size_t i = 0; i < m_mmfpBlocks.size(); ++i ) 
    {
        delete m_mmfpBlocks[i];
    }
    m_mmfpBlocks.clear();

    return unplaced;
}

}


