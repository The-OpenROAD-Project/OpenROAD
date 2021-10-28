


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <stack>
#include <deque>
#include <queue>
#include <cmath>
#include <boost/format.hpp>
#include "utility.h"
#include "plotgnu.h"
#include "legalize.h"
#include "mer.h"
#include "detailed_segment.h"
#include "detailed_manager.h"
#include "legalize_flow_single.h"
#include "legalize_dwsm.h"
#include "min_movement_floorplanner.h"

#include "detailed_random.h"
#include "detailed_mis.h"



////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Local.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Routines.
////////////////////////////////////////////////////////////////////////////////


namespace aak
{

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Legalize::Legalize(
        LegalizeParams& params):
    m_params( params ),
    m_mgr( NULL ),
    m_plotter( NULL )
{
	;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Legalize::~Legalize( void )
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Legalize::legalize( DetailedMgr& mgr )
//void Legalize::legalize( Architecture* arch, Network* network,  RoutingParams* rt )
{
    // New attempt at legalization.  Single entry point.  Hopefully will handle
    // regions!
    int err1, err2, err3, err4, err5;
    (void) err1; (void) err2; (void) err3; (void) err4; (void) err5;

    double init_hpwl, last_hpwl, hpwl_x, hpwl_y;

    bool doImprovement = true;
    bool doLastShift = true;

    char buf[128];
    (void)buf;

    m_mgr = &mgr;

    m_arch = mgr.getArchitecture();
    m_network = mgr.getNetwork();
    m_rt = mgr.getRoutingParams();

    DwsmLegalizerParams dwsmParams;
    DwsmLegalizer dwsm( dwsmParams );

    FlowLegalizerParams flowParams;
    FlowLegalizer flow( flowParams );

    MinMovementFloorplannerParams params;
    MinMovementFloorplanner mmf( params, mgr.m_rng );

    std::vector<std::pair<double,double> > pos;
    std::vector<Node*> multi, single;

    // Vector to record positions at different points.
    pos.resize( m_network->m_nodes.size() );

    std::vector<bool> solved;
    solved.resize( m_arch->m_regions.size() );
    std::fill( solved.begin(), solved.end(), false );

    init_hpwl = Utility::hpwl( m_network, hpwl_x, hpwl_y );

    // Set original positions.
    set_original_positions();
    print_dist_from_start_positions();
    print_dist_from_orig_positions();
    

    // Use the detailed placement manager to categorize the various cells and
    // to create the segments.
    mgr.collectFixedCells();
    mgr.collectSingleHeightCells();
    mgr.collectMultiHeightCells();
    if( mgr.m_multiHeightCells[0].size() != 0 || mgr.m_multiHeightCells[1].size() != 0 )
    {
        std::cout << "Error." << std::endl;
        exit(-1);
    }
    mgr.collectWideCells(); // XXX: This requires segments!
    mgr.findBlockages( mgr.m_fixedCells, false /* exclude blockages. */ );
    mgr.findSegments();

    // Count number of multi-height and single height cells.
    int nSingle = mgr.m_singleHeightCells.size();
    int nMulti = 0;
    for( size_t i = 2; i < mgr.m_multiHeightCells.size(); i++ )
    {
        nMulti += mgr.m_multiHeightCells[i].size();
    }
    std::cout << "Multi-height cells is " << nMulti << ", Single height cells is " << nSingle << std::endl;


    // Do legalization of the cells.  The general flow:
    //
    // 1a. If a region has multi-height cells, then: (i) Use DWSM to remove
    //     overlap, snap cells to segments and apply a minimum shift.
    // 1b. If a region does not have multi-height cells, then: (i) Just snap
    //     cells to segments.
    //
    // 2.  Perform flow for single height cells.  TODO: If the DWSM+shifting
    //     results in a legal region, then do not apply flow to this region.

    // Clear all segments.
    mgr.removeAllCellsFromSegments();

    // Restore original positions.
    restore_original_positions();

    // Grab the original positions.
    for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node* ndi = &(m_network->m_nodes[i]);
        pos[ndi->getId()] = std::make_pair( ndi->getX(), ndi->getY() );
    }

    // Get cells within their regions.
    force_cells_into_regions();

    // Stats.
    print_dist_from_start_positions();
    print_dist_from_orig_positions();

    if( m_plotter != 0 )
    {
        std::cout << "Plotting." << std::endl;
        sprintf( &buf[0], "Stage 1(before)" );
        m_plotter->Draw( m_network, m_arch, buf );
    }


    if( m_params.m_skipLegalization )
    {
        std::cout << "\n===== No legalization; snapping cells to rows and shifting.\n" << std::endl;

        for( int regId = m_arch->m_regions.size(); regId > 0; )
        {
            --regId;

            single.clear();
            multi.clear();
            for( size_t i = 2; i < mgr.m_multiHeightCells.size(); i++ )
            {
                for( size_t n = 0; n < mgr.m_multiHeightCells[i].size(); n++ )
                {
                    Node* ndi = mgr.m_multiHeightCells[i][n];
                    if( ndi->getRegionId() != regId )
                    {
                        continue;
                    }
                    multi.push_back( ndi );
                }
            }
            for( size_t n = 0; n < mgr.m_singleHeightCells.size(); n++ )
            {
                Node* ndi = mgr.m_singleHeightCells[n];
                if( ndi->getRegionId() != regId )
                {
                    continue;
                }
                single.push_back( ndi );
            }
            std::cout << boost::format( "Region %3d, Single height cells is %d, Multi-height cells is %d" )
                    % regId % single.size() % multi.size() << std::endl;

            // The snap to rows.
            mgr.assignCellsToSegments( single );
            mgr.assignCellsToSegments( multi );

            // The shift (which should not do anything if there is no overlap).
            solved[regId] = mmf.test( mgr, regId );
            std::cout << boost::format( "Region %3d, %s be solved with a shift." ) 
                % regId
                % ((solved[regId])?"can":"cannot")
                << std::endl;

            mmf.shift( mgr, regId );
        }
    }
    else
    {
        // Run through legalization (which also includes a step of detailed
        // improvement accounting for displacement).  Should this detailed
        // improvement just be left to the detailed improver????

        std::cout << "\n===== Legalization.\n" << std::endl;

        for( int regId = m_arch->m_regions.size(); regId > 0; )
        {
            --regId;

            single.clear();
            multi.clear();
            for( size_t i = 2; i < mgr.m_multiHeightCells.size(); i++ )
            {
                for( size_t n = 0; n < mgr.m_multiHeightCells[i].size(); n++ )
                {
                    Node* ndi = mgr.m_multiHeightCells[i][n];
                    if( ndi->getRegionId() != regId )
                    {
                        continue;
                    }
                    multi.push_back( ndi );
                }
            }
            for( size_t n = 0; n < mgr.m_singleHeightCells.size(); n++ )
            {
                Node* ndi = mgr.m_singleHeightCells[n];
                if( ndi->getRegionId() != regId )
                {
                    continue;
                }
                single.push_back( ndi );
            }
            std::cout << boost::format( "Region %3d, Single height cells is %d, Multi-height cells is %d" )
                    % regId % single.size() % multi.size() << std::endl;


            dwsm.legalize( mgr, regId );

            mgr.assignCellsToSegments( single );
            mgr.assignCellsToSegments( multi );

            // Test the region which means to imply that overlap can be removed with a shift.
            // If not, then we might want to consider trying to repair the region using 
            // a horizontal constraint graph.  However, this is that effective so we should
            // only try if we have multi-height cells.
            if( !mmf.test( mgr, regId ) && multi.size() != 0 )
            {
                std::cout << "Test failed.  Testing only multi-height cells." << std::endl;
                if( !mmf.test( mgr, regId, true ) )
                {
                    // It's over.  We could not even legalize the multi-height cells.
                    std::cout << "Test failed for multi-height cells." << std::endl;
                }
                else
                {
                    std::cout << "Test passed for multi-height cells.  Possible to repair." << std::endl;
                    // Following code is not that good!
                    mmf.repair( mgr, regId );
                }
            }

            // The final test.  Use shifting if possible.  Note that the
            // cells are positioned in segments in sorted order.  The 
            // shifting uses that order since it is the order which will
            // result in a legal solution.  However, when we shift, we
            // should shift towards the original cell positions; i.e.,
            // the ordering constraints come from the segments, but the
            // objective targets come from the original positions.
            if( mmf.test( mgr, regId ) )
            {
                std::cout << boost::format( "Region %3d, can be solved with a shift." ) % regId << std::endl;

                solved[regId] = true;

                mmf.shift( mgr, regId );
            }
            else
            {
                std::cout << boost::format( "Region %3d, cannot be solved with a shift." ) % regId << std::endl;

                solved[regId] = false;

                mmf.shift( mgr, regId );
            }

            flow.legalize( &mgr, regId );
        }

        err1 = mgr.checkRegionAssignment(); 
        err2 = mgr.checkRowAlignment(); 
        err3 = mgr.checkSiteAlignment(); 
        err4 = mgr.checkOverlapInSegments( 10 ); 
        err5 = mgr.checkEdgeSpacingInSegments(); 

        last_hpwl = Utility::hpwl( m_network, hpwl_x, hpwl_y );
        std::cout << boost::format( "Initial hpwl is %.4lf, Final hpwl is %.4lf\n" ) % init_hpwl % last_hpwl;
        print_dist_from_start_positions();
        print_dist_from_orig_positions();

        // Try some other algorithms to reduce displacement.
        if( doImprovement )
        {
            std::cout << "\n===== Improvement.\n" << std::endl;
            DetailedMis mis( m_arch, m_network, m_rt );
            mis.run( m_mgr, "mis -p 10 -t 0.01 -d" );
            print_dist_from_start_positions();
            print_dist_from_orig_positions();
            DetailedRandom rng( m_arch, m_network, m_rt );
            rng.run( m_mgr, "default -p 5 -f 20 -gen rng:disp -obj hpwl:disp -cost (hpwl)(1.0)(disp(+)(*))" );
            print_dist_from_start_positions();
            print_dist_from_orig_positions();
            std::cout << "\n===== Done detailed improvement aimed at displacement.\n" << std::endl;
        }

        err1 = mgr.checkRegionAssignment(); 
        err2 = mgr.checkRowAlignment(); 
        err3 = mgr.checkSiteAlignment();
        err4 = mgr.checkOverlapInSegments(); 
        err5 = mgr.checkEdgeSpacingInSegments(); 

        // One last shift to reduce displacement in the X-direction.  Could help, but
        // we need to specify the targets.
        if( doLastShift )
        {
            std::vector<std::pair<double,double> > pos;
            pos.resize( m_network->m_nodes.size() );
            for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
            {
                Node* ndi = &(m_network->m_nodes[i]);
                pos[ndi->getId()] = std::make_pair( ndi->getOrigX(), ndi->getOrigY() );
            }
            for( int regId = m_arch->m_regions.size(); regId > 0; )
            {
                --regId;
                if( mmf.test( mgr, regId ) )
                {
                    mmf.shift( mgr, regId, pos );
                }
            }
        }
    }

    err1 = mgr.checkRegionAssignment(); 
    err2 = mgr.checkRowAlignment(); 
    err3 = mgr.checkSiteAlignment();
    err4 = mgr.checkOverlapInSegments(); 
    err5 = mgr.checkEdgeSpacingInSegments(); 
    last_hpwl = Utility::hpwl( m_network, hpwl_x, hpwl_y );

    // Final statistics.
    std::cout << "Final region errors reported is " << err1 << std::endl;
    std::cout << "Final row alignment errors reported is " << err2 << std::endl;
    std::cout << "Final site alignment errors reported is " << err3 << std::endl;
    std::cout << "Final cell overlaps reported is " << err4 << std::endl;
    std::cout << "Final edge spacing violations reported " << err5 << std::endl;
    std::cout << boost::format( "Initial hpwl is %.4lf, Final hpwl is %.4lf\n" ) % init_hpwl % last_hpwl;
    print_dist_from_start_positions();
    print_dist_from_orig_positions();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Legalize::set_original_positions()
{
    m_origX.resize( m_network->m_nodes.size() );
    m_origY.resize( m_network->m_nodes.size() );
    for( int i = 0; i < m_network->m_nodes.size(); i++ ) 
    {
        Node* nd = &(m_network->m_nodes[i]);
        m_origX[nd->getId()] = nd->getX();
        m_origY[nd->getId()] = nd->getY();
    }
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Legalize::restore_original_positions()
{
    for( int i = 0; i < m_network->m_nodes.size(); i++ ) 
    {
        Node* nd = &(m_network->m_nodes[i]);
        if( nd->getType() == NodeType_TERMINAL || nd->getType() == NodeType_TERMINAL_NI )
        {
            continue;
        }
        if( nd->getFixed() != NodeFixed_NOT_FIXED )
        {
            continue;
        }

        nd->setX( m_origX[nd->getId()] );
        nd->setY( m_origY[nd->getId()] );
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Legalize::force_cells_into_regions()
{
    std::cout << "Forcing cells into their assigned regions." << std::endl;

    Architecture::Region* regPtr;
    int cellsInRegions = 0;
    int cellsMovedInRegions = 0;
    int cellsInDefault = 0;
    int cellsMovedInDefault = 0;

    double xx, yy, ww, hh;

    // Place those cells that must be _IN_ a region; skip default region.
    for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node* ndi = &(m_network->m_nodes[i]);

        double xl = ndi->getX() - 0.5 * ndi->getWidth() ;
        double xr = ndi->getX() + 0.5 * ndi->getWidth() ;
        double yb = ndi->getY() - 0.5 * ndi->getHeight();
        double yt = ndi->getY() + 0.5 * ndi->getHeight();

        // Ignore terminals or fixed nodes.
        if( ndi->getType() == NodeType_TERMINAL || ndi->getType() == NodeType_TERMINAL_NI )
        {
            continue;
        }
        if( ndi->getFixed() != NodeFixed_NOT_FIXED )
        {
            continue;
        }

        int regId = ndi->getRegionId();
        if( regId == 0 )
        {
            continue;
        }
        ++cellsInRegions;

        regPtr = m_arch->m_regions[regId];

        int inside = true;
        Rectangle surround( regPtr->m_xmin, regPtr->m_ymin, regPtr->m_xmax, regPtr->m_ymax );
        if( !inside_rectangle( ndi, surround ) )
        {
            // Definitely not inside its region.
            inside = false;
        }
        else
        {
            // Need to examine the sub-rectangles.
            SpaceManager sm( regPtr->m_xmin, regPtr->m_xmax, regPtr->m_ymin, regPtr->m_ymax );
            for( size_t r = 0; r < regPtr->m_rects.size(); r++ )
            {
                Rectangle& rect = regPtr->m_rects[r];
                sm.AddFullRectangle( rect.xmin(), rect.xmax(), rect.ymin(), rect.ymax() );
            }

            // Now, we should not overlap with any of the _free_ rectangles available
            // by the space manager.
            std::vector<Rectangle*>& rects = sm.getFreeRectangles();
            for( size_t r = 0; r < rects.size(); r++ )
            {
                Rectangle* rect = rects[r];

                xx = std::max( xl, rect->xmin() );
                yy = std::max( yb, rect->ymin() );
                ww = std::min( xr, rect->xmax() ) - xx;
                hh = std::min( yt, rect->ymax() ) - yy;
                if( ww >= 1.0e-3 && hh >= 1.0e-3 && ww*hh >= 1.0e-3 )
                {
                    inside = false;
                }
            }
        }

        if( !inside )
        {
            // Cell does not appear to be within its region.  So, look at the rectangles
            // that form the region and try to get the cell inside of one of them.
            double best_dist = std::numeric_limits<double>::max();
            int best_id = -1;
            for( size_t r = 0; r < regPtr->m_rects.size(); r++ )
            {
                Rectangle& rect = regPtr->m_rects[r];
                if( inside_rectangle( ndi, rect ) )
                {
                    inside = true;
                    best_id = r;
                    break;
                }
                if( fits_in_rectangle( ndi, rect ) )
                {
                    double dist = distance_to_rectangle( ndi, rect );
                    if( dist < best_dist )
                    {
                        best_dist = dist;
                        best_id = r;
                    }
                }
            }
            best_id = std::max( 0, best_id );
            move_into_rectangle( ndi, regPtr->m_rects[best_id] );
            ++cellsMovedInRegions;
        }
    }
    // Place those cells in the default region; move them out of regions.  This seems
    // to be a bit tricker since, if we find the cell inside a rectangle, then we 
    // must move it out, but we must also be sure that we don't just move it into 
    // yet another rectangle associated with any region other than the default...
    std::cout << "Creating space manager for default region." << std::endl;
    SpaceManager sm( m_arch->getMinX(), m_arch->getMaxX(), m_arch->getMinY(), m_arch->getMaxY() );
    int inserted = 0;
    for( size_t i = 1; i < m_arch->m_regions.size(); i++ )
    {
        Architecture::Region* regPtr = m_arch->m_regions[i];
        for( size_t r = 0; r < regPtr->m_rects.size(); r++ )
        {
            Rectangle& rect = regPtr->m_rects[r];
            sm.AddFullRectangle( rect.xmin(), rect.xmax(), rect.ymin(), rect.ymax() );
            ++inserted;
        }
    }
    std::cout << boost::format( "Added %d rectangles to block regions; %d maximal rectangles in free space.\n" )
        % inserted % (int)sm.getFreeRectangles().size();

    for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node* ndi = &(m_network->m_nodes[i]);

        // Ignore terminals or fixed nodes.
        if( ndi->getType() == NodeType_TERMINAL || ndi->getType() == NodeType_TERMINAL_NI )
        {
            continue;
        }
        if( ndi->getFixed() != NodeFixed_NOT_FIXED )
        {
            continue;
        }

        int regId = ndi->getRegionId();
        if( regId != 0 )
        {
            continue;
        }
        ++cellsInDefault;

        bool inRegion = false;
        double best_dist = std::numeric_limits<double>::max();
        int best_id = -1;
        std::vector<Rectangle*>& rects = sm.getFreeRectangles();
        for( size_t r = 0; r < rects.size(); r++ )
        {
            Rectangle* rect = rects[r];
            if( inside_rectangle( ndi, *rect ) )
            {
                inRegion = true;
                break;
            }
            if( fits_in_rectangle( ndi, *rect ) )
            {
                double dist = distance_to_rectangle( ndi, *rect );
                if( dist < best_dist )
                {
                    best_dist = dist;
                    best_id = r;
                }
            }
        }
        if( !inRegion )
        {
            if( best_id == -1 )
            {
                std::cout << boost::format( "Unable to fit cell '%s' into default region.\n" )
                    % m_network->m_nodeNames[i].c_str();
                exit(-1);
            }
            move_into_rectangle( ndi, *(rects[best_id]) );
            ++cellsMovedInDefault;
        }
    }

    std::cout << boost::format( "Cells in regions: %d, Moved: %d, Cells in default: %d, Moved:  %d\n" )
        % cellsInRegions % cellsMovedInRegions % cellsInDefault % cellsMovedInDefault;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Legalize::inside_rectangle( Node* ndi, Rectangle& rect )
{
    // Determine if the node is entirely inside the rectangle.
    double xmin = ndi->getX() - 0.5 * ndi->getWidth() ;
    double xmax = ndi->getX() + 0.5 * ndi->getWidth() ;
    double ymin = ndi->getY() - 0.5 * ndi->getHeight();
    double ymax = ndi->getY() + 0.5 * ndi->getHeight();

    if( xmin >= rect.xmin() && xmax <= rect.xmax() && ymin >= rect.ymin() && ymax <= rect.ymax() )
        return true;
    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Legalize::fits_in_rectangle( Node* ndi, Rectangle& rect )
{
    if( ndi->getWidth()  > rect.xmax()-rect.xmin() ) 
        return false;
    if( ndi->getHeight() > rect.ymax()-rect.ymin() )
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Legalize::distance_to_rectangle( Node* ndi, Rectangle& rect )
{
    // Determine the distance such that the node is entirely within the rectangle.
    // XXX: What if the node does not fit????????????????????????????????????????
    double xmin = ndi->getX() - 0.5 * ndi->getWidth() ;
    double xmax = ndi->getX() + 0.5 * ndi->getWidth() ;
    double ymin = ndi->getY() - 0.5 * ndi->getHeight();
    double ymax = ndi->getY() + 0.5 * ndi->getHeight();

    double dx = 0.;
    double dy = 0.;
    if( xmin < rect.xmin() ) dx = rect.xmin() - xmin;
    else if( xmax > rect.xmax() ) dx = xmax - rect.xmax();
    if( ymin < rect.ymin() ) dy = rect.ymin() - ymin;
    else if( ymax > rect.ymax() ) dy = ymax - rect.ymax();
    return dx + dy;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Legalize::move_into_rectangle( Node* ndi, Rectangle& rect )
{
    double xmin = ndi->getX() - 0.5 * ndi->getWidth() ;
    double xmax = ndi->getX() + 0.5 * ndi->getWidth() ;
    double ymin = ndi->getY() - 0.5 * ndi->getHeight();
    double ymax = ndi->getY() + 0.5 * ndi->getHeight();

    if( xmin < rect.xmin() ) ndi->setX( rect.xmin() + 0.5 * ndi->getWidth()  );
    if( xmax > rect.xmax() ) ndi->setX( rect.xmax() - 0.5 * ndi->getWidth()  );
    if( ymin < rect.ymin() ) ndi->setY( rect.ymin() + 0.5 * ndi->getHeight() );
    if( ymax > rect.ymax() ) ndi->setY( rect.ymax() - 0.5 * ndi->getHeight() );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Legalize::print_dist_from_start_positions( void )
{
    // Print displacements from the positions at the start of legalization.
    double tot_disp = 0.;
    double max_disp = 0.;
    for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node* ndi = &(m_network->m_nodes[i]);
        if( ndi->getType() == NodeType_TERMINAL || ndi->getType() == NodeType_TERMINAL_NI )
        {
            continue;
        }
        if( ndi->getFixed() != NodeFixed_NOT_FIXED )
        {
            continue;
        }
        double dx = std::fabs( ndi->getX() - m_origX[ndi->getId()] );
        double dy = std::fabs( ndi->getY() - m_origY[ndi->getId()] );
        tot_disp += (dx + dy);
        max_disp = std::max( max_disp, dx + dy );
    }
    double avg_disp = tot_disp / (double)m_network->m_nodes.size();
    std::cout << boost::format( "Max disp: %.4lf, Tot disp: %.4lf, Avg disp: %.4lf\n" )
        % max_disp % tot_disp % avg_disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Legalize::print_dist_from_orig_positions( void )
{
    // Print displacement from the positions stored in the network; likely the
    // positions parsed from the input files.
    double tot_disp = 0.;
    double max_disp = 0.;
    for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node* ndi = &(m_network->m_nodes[i]);
        if( ndi->getType() == NodeType_TERMINAL || ndi->getType() == NodeType_TERMINAL_NI )
        {
            continue;
        }
        if( ndi->getFixed() != NodeFixed_NOT_FIXED )
        {
            continue;
        }
        double dx = std::fabs( ndi->getX() - ndi->getOrigX() );
        double dy = std::fabs( ndi->getY() - ndi->getOrigY() );
        tot_disp += (dx + dy);
        max_disp = std::max( max_disp, dx + dy );
    }
    double avg_disp = tot_disp / (double)m_network->m_nodes.size();
    std::cout << boost::format( "Max disp: %.4lf, Tot disp: %.4lf, Avg disp: %.4lf\n" )
        % max_disp % tot_disp % avg_disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class MoveRecord
{
public:
    MoveRecord( void ):m_ndi( 0 ),m_seg( -1 ),m_dist( std::numeric_limits<double>::max() )
    {}
    MoveRecord( Node* ndi, int seg, double dist ):m_ndi( ndi ),m_seg( seg ),m_dist( dist )
    {}
    MoveRecord( const MoveRecord& other ):m_ndi( other.m_ndi ),m_seg( other.m_seg ),m_dist( other.m_dist ) 
    {}
    MoveRecord& operator=( const MoveRecord& other )
    {
        if( this != &other )
        {
            m_dist = other.m_dist;
            m_ndi = other.m_ndi;
            m_seg = other.m_seg;
        }
        return *this;
    }
public:
    double  m_dist;
    Node*   m_ndi;
    int     m_seg;
};

struct compareMoveRecords
{
    inline bool operator() ( const MoveRecord& p, const MoveRecord& q ) const
    {
        if( p.m_dist == q.m_dist )
        {
            if( p.m_ndi->getWidth() == q.m_ndi->getWidth() )
            {
                return p.m_ndi->getId() < q.m_ndi->getId();
            }
            return p.m_ndi->getWidth() < q.m_ndi->getWidth();
        }
        return p.m_dist < q.m_dist;
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Legalize::moveCellsBetweenSegments( void )
{
    // Useful at the end of the flow in order to try and reduce the amount
    // of overfilled segments.  This code will, somewhat agressively, try 
    // and move single height cells out of overfilled segments.  In the 
    // first pass, we will attempt to only consider the amount of overfill
    // (overlap).  In the second pass, we will also consider gap requirements.
    //
    // XXX: Not sure how to accommodate issues such as multi-height cells
    // messing things up by causing gaps in the actual placement.  This, 
    // is possibly something that can be fixed by moving a cell to a new
    // position inside of the same segment (i.e., in between multi-height
    // cells).
    //
    // Go in whatever order.  Perhaps some randomization might be useful...
    double width;
    double util;
    double gapu;
    int last, curr, noimp;


    // Attempt 1: Only consider utilization and ignore gap requirements.
    // Try to keep cells close to their original segments.
    curr = 0;
    for( size_t s = 0; s < m_mgr->m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = m_mgr->m_segments[s];
        width = segPtr->m_xmax - segPtr->m_xmin;
        util = segPtr->m_util;
        if( util >= width+1.0e-3 )
        {
            ++curr;
        }
    }
    last = curr;
    noimp = 0;
    for( int pass = 1; pass <= 200; pass++ )
    {
        std::cout << "Pass " << pass << " of moving cells between segments, Violations is " << last << std::endl;
        if( last == 0 || noimp >= 3 )
        {
            break;
        }

        // Try to reduce overfilled segments.
        for( size_t s = 0; s < m_mgr->m_segments.size(); s++ )
        {
            DetailedSeg* segPtr = m_mgr->m_segments[s];
            width = segPtr->m_xmax - segPtr->m_xmin;
            util = segPtr->m_util;
            if( util <= width+1.0e-3 )
            {
                continue;
            }
            moveCellsBetweenSegments( segPtr,    2, 0.1000 );
        }

        // Count overfilled bins after improvement.
        curr = 0;
        for( size_t s = 0; s < m_mgr->m_segments.size(); s++ )
        {
            DetailedSeg* segPtr = m_mgr->m_segments[s];
            width = segPtr->m_xmax - segPtr->m_xmin;
            util = segPtr->m_util;
            if( util >= width+1.0e-3 )
            {
                ++curr;
            }
        }
        if( curr >= last )
        {
            ++noimp;
        }
        last = curr;
    }
     
    // Attempt 2: Only consider utilization and ignore gap requirements.
    // Allow cells to move a bit further from their original segments.
    curr = 0;
    for( size_t s = 0; s < m_mgr->m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = m_mgr->m_segments[s];
        width = segPtr->m_xmax - segPtr->m_xmin;
        util = segPtr->m_util;
        if( util >= width+1.0e-3 )
        {
            ++curr;
        }
    }
    last = curr;
    noimp = 0;
    for( int pass = 1; pass <= 200; pass++ )
    {
        std::cout << "Pass " << pass << " of moving cells between segments, Violations is " << last << std::endl;
        if( last == 0 || noimp >= 3 )
        {
            break;
        }

        // Try to reduce overfilled segments.
        for( size_t s = 0; s < m_mgr->m_segments.size(); s++ )
        {
            DetailedSeg* segPtr = m_mgr->m_segments[s];
            width = segPtr->m_xmax - segPtr->m_xmin;
            util = segPtr->m_util;
            if( util <= width+1.0e-3 )
            {
                continue;
            }
            moveCellsBetweenSegments( segPtr,    2, 0.1000 );
        }

        // Count overfilled bins after improvement.
        curr = 0;
        for( size_t s = 0; s < m_mgr->m_segments.size(); s++ )
        {
            DetailedSeg* segPtr = m_mgr->m_segments[s];
            width = segPtr->m_xmax - segPtr->m_xmin;
            util = segPtr->m_util;
            if( util >= width+1.0e-3 )
            {
                ++curr;
            }
        }
        if( curr >= last )
        {
            ++noimp;
        }
        last = curr;
    }
     
    // Should I also attempt to improve the situation with gaps?  I think that
    // can also be addressed better during detailed improvement.
    ;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Legalize::moveCellsBetweenSegments( DetailedSeg* oldPtr,  
        int leftRightTol, double offsetTol,
        bool ignoreGaps )
{

    // The purpose of this routine is to take an overfilled segment and force
    // some of the cells into different segments to remove the overfill.

    int bestSegment[4];
    double bestPosX[4]; // 0 = Left, 1 = Right, 2 = Below, 3 = Above.
    double bestPosY[4];
    double bestDist[4];
    double currDist, score[4], wl[4];
    double u1, g1;
    double util1, util2, util3, util4;
    double viol1, viol2, viol3, viol4;
    (void)viol1; (void)viol2; (void)viol3; (void)viol4;
    int offset;
    std::vector<MoveRecord> moves;
    double old_wid, new_wid;

    old_wid = oldPtr->m_xmax - oldPtr->m_xmin;
    util1 = oldPtr->m_util + ((ignoreGaps) ? 0.0 : oldPtr->m_gapu);

    if( util1 <= old_wid )
    {
        return;
    }

    std::cout << "Moving single height cells out of segment "
        << oldPtr->m_segId << ", "
        << "util is " << oldPtr->m_util << ", "
        << "spacing is " << oldPtr->m_gapu << ", "
        << "width is " << oldPtr->m_xmax - oldPtr->m_xmin << ", "
        << "potential cells is " << m_mgr->m_cellsInSeg[oldPtr->m_segId].size()
        << std::endl;

    int numSingleHeightRows = m_arch->m_rows.size();
    double singleRowHeight = m_arch->m_rows[0]->getH();

    DetailedSeg* newPtr = 0;

    int old_seg = oldPtr->m_segId;
    int old_row = oldPtr->m_rowId;
    int new_seg;
    int new_row;

    int coreW = m_arch->m_xmax - m_arch->m_xmin;
    int coreH = m_arch->m_ymax - m_arch->m_ymin;

    // Determine where this segment lies within the row's segments.
    int segInRow = -1;
    {
        int i = 0;
        while( m_mgr->m_segsInRow[old_row][i] != oldPtr && i < m_mgr->m_segsInRow[old_row].size() )
        {
            ++i;
        }
        if( i >= m_mgr->m_segsInRow[old_row].size() || m_mgr->m_segsInRow[old_row][i] != oldPtr )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
        segInRow = i;
    }
    if( segInRow == -1 )
    {
        cout << "Error." << endl;
        exit(-1);
    }


    // Compute a score for every single height cell in this segment moving to 
    // other segments.  Then, pick as many cells as possible to reduce the 
    // infeasibility of the current segment.  Avoid making other segments
    // infeasible.
    moves.resize( m_mgr->m_cellsInSeg[old_seg].size() );

    int nsingle = 0;
    int nmoves = 0;
    for( int count = 0; count < m_mgr->m_cellsInSeg[old_seg].size(); count++ )
    {
        Node* nd = m_mgr->m_cellsInSeg[old_seg][count];

        // Set the move for this cell.  We might not get a move for this cell,
        // and we can detect this by setting the new segment ID equal to the
        // old segment ID.
        moves[count].m_dist = std::numeric_limits<double>::max();
        moves[count].m_seg = old_seg;
        moves[count].m_ndi = nd;

        //std::cout << "@ node " << nd->getId() << std::endl;

        int spanned = (int)(nd->getHeight() / singleRowHeight + 0.5);
        if( spanned != 1 )
        {
            continue;
        }

        ++nsingle;

        bestDist[0] = std::numeric_limits<double>::max();
        bestDist[1] = std::numeric_limits<double>::max();
        bestDist[2] = std::numeric_limits<double>::max();
        bestDist[3] = std::numeric_limits<double>::max();

        bestSegment[0] = -1;
        bestSegment[1] = -1;
        bestSegment[2] = -1;
        bestSegment[3] = -1;

        // Left...
        //std::cout << "left..." << std::endl;
        {
            int i = segInRow-1;
            int limit = std::max( 0, segInRow - leftRightTol );
            for( ; i >= limit; --i )
            {
                DetailedSeg* newPtr = m_mgr->m_segsInRow[old_row][i];
                if( newPtr->getRegId() != nd->getRegionId() )
                {
                    continue;
                }

                new_seg = newPtr->m_segId;
                new_row = newPtr->m_rowId;

                double sl = newPtr->m_xmin;
                double sr = newPtr->m_xmax;
                double ww = nd->getWidth();

                // Determine X-position to get this cell into this segment.
                double x1 = sl + 0.5 * ww;
                double x2 = sr - 0.5 * ww;
                double xx = std::min( std::max( x1, nd->getX() ), x2 );
                double yy = nd->getY();
 
                util1 = oldPtr->m_util + ((ignoreGaps) ? 0.0 : oldPtr->m_gapu);
                util2 = newPtr->m_util + ((ignoreGaps) ? 0.0 : newPtr->m_gapu);
                m_mgr->removeCellFromSegmentTest( nd, old_seg, u1, g1 );
                util3 = u1             + ((ignoreGaps) ? 0.0 : g1            );
                m_mgr->addCellToSegmentTest( nd, new_seg, xx, u1, g1 );
                util4 = u1             + ((ignoreGaps) ? 0.0 : g1            );

                // util1 - occupancy of old segment prior to move.
                // util2 - occupancy of new segment prior to move.
                // util3 - occupancy of old segment after move (likely better than util1).
                // util4 - occupancy of new segment after move (likely worse than util2).

                // Utils represent violations...
                new_wid = newPtr->m_xmax - newPtr->m_xmin;

                viol1 = std::max( 0.0, util1 - old_wid );
                viol2 = std::max( 0.0, util2 - new_wid );
                viol3 = std::max( 0.0, util3 - old_wid );
                viol4 = std::max( 0.0, util4 - new_wid );

                // Accept the move if the other segment does not become violated.  We could
                // also consider accepting the move if the worst violation decreases...
                if( util4 <= new_wid )
                {
                    double dist = std::fabs( xx - nd->getX() );
                    if( dist < bestDist[0] )
                    {
                        bestDist[0] = dist; bestSegment[0] = new_seg;
                    }
                }
            }
        }

        // Right...
        //std::cout << "right..." << std::endl;
        {
            int i = segInRow+1;
            int limit = std::min( (int)(m_mgr->m_segsInRow[old_row].size()-1), segInRow+leftRightTol);
            for( ; i <= limit; i++ )
            {
                DetailedSeg* newPtr = m_mgr->m_segsInRow[old_row][i];
                if( newPtr->getRegId() != nd->getRegionId() )
                {
                    continue;
                }

                new_seg = newPtr->m_segId;
                new_row = newPtr->m_rowId;

                double sl = newPtr->m_xmin;
                double sr = newPtr->m_xmax;
                double ww = nd->getWidth();

                // Determine X-position to get this cell into this segment.
                double x1 = sl + 0.5 * ww;
                double x2 = sr - 0.5 * ww;
                double xx = std::min( std::max( x1, nd->getX() ), x2 );
                double yy = nd->getY();

                util1 = oldPtr->m_util + ((ignoreGaps) ? 0.0 : oldPtr->m_gapu);
                util2 = newPtr->m_util + ((ignoreGaps) ? 0.0 : newPtr->m_gapu);
                m_mgr->removeCellFromSegmentTest( nd, old_seg, u1, g1 );
                util3 = u1             + ((ignoreGaps) ? 0.0 : g1            );
                m_mgr->addCellToSegmentTest( nd, new_seg, xx, u1, g1 );
                util4 = u1             + ((ignoreGaps) ? 0.0 : g1            );

                // util1 - occupancy of old segment prior to move.
                // util2 - occupancy of new segment prior to move.
                // util3 - occupancy of old segment after move (likely better than util1).
                // util4 - occupancy of new segment after move (likely worse than util2).

                // Utils represent violations...
                new_wid = newPtr->m_xmax - newPtr->m_xmin;

                viol1 = std::max( 0.0, util1 - old_wid );
                viol2 = std::max( 0.0, util2 - new_wid );
                viol3 = std::max( 0.0, util3 - old_wid );
                viol4 = std::max( 0.0, util4 - new_wid );

                // Accept the move if the other segment does not become violated.  We could
                // also consider accepting the move if the worst violation decreases...
                if( util4 <= new_wid )
                {
                    double dist = std::fabs( xx - nd->getX() );
                    if( dist < bestDist[1] )
                    {
                        bestDist[1] = dist; bestSegment[1] = new_seg;
                    }
                }
            }
        }

        // Below...
        //std::cout << "below..." << std::endl;
        {
            int limit = std::max( 5, (int)(offsetTol * numSingleHeightRows) );
            for( int offset = 1; offset <= limit && old_row-offset >= 0; offset++ )
            {
                new_row = old_row - offset;

                for( int i = 0; i < m_mgr->m_segsInRow[new_row].size(); i++ )
                {
                    DetailedSeg* newPtr = m_mgr->m_segsInRow[new_row][i];
                    if( newPtr->getRegId() != nd->getRegionId() )
                    {
                        continue;
                    }

                    new_seg = newPtr->m_segId;

                    double sl = newPtr->m_xmin;
                    double sr = newPtr->m_xmax;
                    double ww = nd->getWidth();

                    // Determine X-position to get this cell into this segment.
                    double x1 = sl + 0.5 * ww;
                    double x2 = sr - 0.5 * ww;
                    double xx = std::min( std::max( x1, nd->getX() ), x2 );
                    double yy = m_arch->m_rows[new_row]->getY() + 0.5 * nd->getHeight();

                    util1 = oldPtr->m_util + ((ignoreGaps) ? 0.0 : oldPtr->m_gapu);
                    util2 = newPtr->m_util + ((ignoreGaps) ? 0.0 : newPtr->m_gapu);
                    m_mgr->removeCellFromSegmentTest( nd, old_seg, u1, g1 );
                    util3 = u1             + ((ignoreGaps) ? 0.0 : g1            );
                    m_mgr->addCellToSegmentTest( nd, new_seg, xx, u1, g1 );
                    util4 = u1             + ((ignoreGaps) ? 0.0 : g1            );

                    // util1 - occupancy of old segment prior to move.
                    // util2 - occupancy of new segment prior to move.
                    // util3 - occupancy of old segment after move (likely better than util1).
                    // util4 - occupancy of new segment after move (likely worse than util2).

                    // Utils represent violations...
                    new_wid = newPtr->m_xmax - newPtr->m_xmin;

                    viol1 = std::max( 0.0, util1 - old_wid );
                    viol2 = std::max( 0.0, util2 - new_wid );
                    viol3 = std::max( 0.0, util3 - old_wid );
                    viol4 = std::max( 0.0, util4 - new_wid );

                    if( util4 <= new_wid )
                    {
                        double dist = std::fabs( xx - nd->getX() ) + std::fabs( yy - nd->getY() );
                        if( dist < bestDist[2] )
                        {
                            bestDist[2] = dist; bestSegment[2] = new_seg;
                        }
                    }
                }
            }
        }

        // Above...
        //std::cout << "above..." << std::endl;
        {
            int limit = std::max( 5, (int)(offsetTol * numSingleHeightRows) );
            for( int offset = 1; offset <= limit && old_row+offset <= m_arch->m_rows.size()-1; offset++ )
            {
                new_row = old_row + offset;

                for( int i = 0; i < m_mgr->m_segsInRow[new_row].size(); i++ )
                {
                    DetailedSeg* newPtr = m_mgr->m_segsInRow[new_row][i];
                    if( newPtr->getRegId() != nd->getRegionId() )
                    {
                        continue;
                    }

                    new_seg = newPtr->m_segId;

                    double sl = newPtr->m_xmin;
                    double sr = newPtr->m_xmax;
                    double ww = nd->getWidth();

                    // Determine X-position to get this cell into this segment.
                    double x1 = sl + 0.5 * ww;
                    double x2 = sr - 0.5 * ww;
                    double xx = std::min( std::max( x1, nd->getX() ), x2 );
                    double yy = m_arch->m_rows[new_row]->getY() + 0.5 * nd->getHeight();

                    util1 = oldPtr->m_util + ((ignoreGaps) ? 0.0 : oldPtr->m_gapu);
                    util2 = newPtr->m_util + ((ignoreGaps) ? 0.0 : newPtr->m_gapu);
                    m_mgr->removeCellFromSegmentTest( nd, old_seg, u1, g1 );
                    util3 = u1             + ((ignoreGaps) ? 0.0 : g1            );
                    m_mgr->addCellToSegmentTest( nd, new_seg, xx, u1, g1 );
                    util4 = u1             + ((ignoreGaps) ? 0.0 : g1            );

                    // util1 - occupancy of old segment prior to move.
                    // util2 - occupancy of new segment prior to move.
                    // util3 - occupancy of old segment after move (likely better than util1).
                    // util4 - occupancy of new segment after move (likely worse than util2).

                    // Utils represent violations...
                    new_wid = newPtr->m_xmax - newPtr->m_xmin;

                    viol1 = std::max( 0.0, util1 - old_wid );
                    viol2 = std::max( 0.0, util2 - new_wid );
                    viol3 = std::max( 0.0, util3 - old_wid );
                    viol4 = std::max( 0.0, util4 - new_wid );

                    if( util4 <= new_wid )
                    {
                        double dist = std::fabs( xx - nd->getX() ) + std::fabs( yy - nd->getY() );
                        if( dist < bestDist[3] )
                        {
                            bestDist[3] = dist; bestSegment[3] = new_seg;
                        }
                    }
                }
            }
        }

        // Record the valid choices.
        for( int i = 0; i <= 3; i++ )
        {
            if( bestSegment[i] != -1 && bestSegment[i] != old_seg && bestDist[i] < moves[count].m_dist )
            {
                moves[count].m_seg = bestSegment[i];
                moves[count].m_dist = bestDist[i];
            }
        }
        if( moves[count].m_seg != old_seg )
        {
            ++nmoves;
        }
    }
    std::stable_sort( moves.begin(), moves.end(), compareMoveRecords() );

    std::cout << "Number of single height cells in seg is " << nsingle << ", "
        << "potential candidates is " << nmoves << std::endl;

    // Make moves until the issues with the segment is resolved or until
    // we are out of moves.
    int performed = 0;
    for( int count = 0; count < moves.size(); count++ )
    {
        new_seg = moves[count].m_seg;
        newPtr = m_mgr->m_segments[new_seg];
        new_row = newPtr->m_rowId;
        Node* nd = moves[count].m_ndi;

        if( new_seg == old_seg )
        {
            // This implies there was no valid move for this cell.
            continue;
        }

        // Remove the cell from its current segment, determine its new position and then
        // insert it into the new segment.  

        double sl = newPtr->m_xmin;
        double sr = newPtr->m_xmax;
        double ww = nd->getWidth();

        // Determine X-position to get this cell into this segment.
        double x1 = sl + 0.5 * ww;
        double x2 = sr - 0.5 * ww;
        double xx = std::min( std::max( x1, nd->getX() ), x2 );
        double yy = m_arch->m_rows[new_row]->getY() + 0.5 * nd->getHeight();

        old_wid = oldPtr->m_xmax - oldPtr->m_xmin;
        new_wid = newPtr->m_xmax - newPtr->m_xmin;

        // Note: In computing potential cell moves, it might be the case that we
        // end up with many cells targetted at the "same close segment".  Of 
        // course, when computing the moves, we did not actually take into account
        // that cells might have already been moved into this segment.  So, we 
        // need to again confirm that we will not go over capacity.

        util1 = oldPtr->m_util + ((ignoreGaps) ? 0.0 : oldPtr->m_gapu);
        util2 = newPtr->m_util + ((ignoreGaps) ? 0.0 : newPtr->m_gapu);
        m_mgr->removeCellFromSegmentTest( nd, old_seg, u1, g1 );
        util3 = u1             + ((ignoreGaps) ? 0.0 : g1            );
        m_mgr->addCellToSegmentTest( nd, new_seg, xx, u1, g1 );
        util4 = u1             + ((ignoreGaps) ? 0.0 : g1            );

        if( util4 >= new_wid+1.0e-3 )
        {
            continue;
        }

        // Once here, it is okay to actually do the move!
        double ox = nd->getX();
        double oy = nd->getY();
        m_mgr->removeCellFromSegment( nd, old_seg );
        nd->setX( xx );
        nd->setY( yy );
        m_mgr->addCellToSegment( nd, new_seg );

        std::cout << "Moving cell " << nd->getId() << ", width is " << nd->getWidth() << ", "
            << "region is " << nd->getRegionId() << " @ "
            << "(" << ox << "," << oy << ")" << " to "
            << "(" << xx << "," << yy << ")" 
            << std::endl;
        std::cout <<  " => From segment @ "
            << "[" << oldPtr->m_xmin << "," << oldPtr->m_xmax << "]" << ", @ " 
            << m_arch->m_rows[old_row]->getY() + 0.5 * m_arch->m_rows[old_row]->getH() << ", "
            << "util change " << util1 << "-> " << util3 << ", cap is " << old_wid
            << std::endl;
        std::cout <<  " => To segment @ "
            << "[" << newPtr->m_xmin << "," << newPtr->m_xmax << "]" << ", @ " 
            << m_arch->m_rows[new_row]->getY() + 0.5 * m_arch->m_rows[new_row]->getH() << ", "
            << "util change " << util2 << "-> " << util4 << ", cap is " << new_wid
            << std::endl;

        // Stop once we have resolved issues with the current segment.
        if( util3 <= old_wid + 1.0e-3 )
        {
            break;
        }
    }
}

// XXX: Need to move the following code someplace else!!!!!!!!!!!!!!!!!!!!!!!!!!

struct SortNodesX
{
    inline bool operator() ( Node* p, Node* q ) const
    {
        return p->getX() < q->getX();
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Legalize::doClumping( std::vector<Node*>& nodes_in, double xmin, double xmax, bool debug )
{
    // Copy the node vector to avoid changing the provided order.  The positions of the nodes
    // are stored in the nodes...
    std::vector<Node*> nodes = nodes_in;
    std::sort( nodes.begin(), nodes.end(), SortNodesX() );

    std::vector<Clump> clumps;
    clumps.resize( nodes.size() );
    for( int i = 0; i < nodes.size(); i++ ) {
        Node* nd = nodes[i];

        Clump* r = &(clumps[i]);
        r->m_id = i;
        r->m_nodes.push_back( nd );
        r->m_width = nd->getWidth();
        r->m_wposn = nd->getX() - 0.5 * nd->getWidth();
        r->m_weight = 1.;
        r->m_posn = r->m_wposn / r->m_weight;

        // NB: We keep the left edge of the clump within the segment always!
        r->m_posn = std::min( std::max( r->m_posn, xmin ), xmax - r->m_width );
    }

    // Perform the clumping.
    for( int i = 0; i < nodes.size(); i++ ) {
        doClumping( &clumps[i], clumps, xmin, xmax );
    }

    // First, we put nodes at their min shift locations.  Then, we assign into closest sites.
    for( int i = 0; i < nodes.size(); i++ ) {
        Clump* r = &(clumps[i]);
        double offset = 0;
        for( int j = 0; j < r->m_nodes.size(); j++ ) {
            Node* nd = r->m_nodes[j];
            nd->setX( r->m_posn + offset + 0.5 * nd->getWidth() );
            offset += nd->getWidth();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Legalize::doClumping( Clump* r, std::vector<Clump>& clumps, double xmin, double xmax, bool debug )
{
    // Clump cannot overlap with a previous (non-defunct) clump.  If we encounter overlap, then we need
    // to merge the clumps.

    Clump* l = 0;
    int ix = r->m_id;
    while( true ) {
        // Find the previous, non-trivial clump prior to clump 'r'.  

        ix = r->m_id - 1;
        while( ix > 0 && clumps[ix].m_nodes.size() == 0 ) {
            --ix;
        }

        //  If no previous clump with something in it, then break.
        if( ix < 0 || clumps[ix].m_nodes.size() == 0 ) {
            break;
        }

        Clump* l = &(clumps[ix]);


        // If no overlap with previous clump,  then break.
        if( r->m_posn >= l->m_posn + l->m_width ) {
            break;
        }

        // Here, the left edge of clump 'r' is prior to the right edge of clump 'l'.   So, we need
        // to merge the clumps.  Get right of clump 'r' by merging it with 'l' and then determine
        // the new position of the clump.

        double dist = l->m_width;
        for( int i = 0; i < r->m_nodes.size(); i++ ) {
            Node* nd = r->m_nodes[i];
            l->m_nodes.push_back( nd );
            l->m_width += nd->getWidth();
        }
        l->m_wposn += r->m_wposn - dist * r->m_weight;
        l->m_weight += r->m_weight;
        l->m_posn = l->m_wposn / l->m_weight;
        l->m_posn = std::min( std::max( l->m_posn, xmin ), xmax - l->m_width );

        // XXX: For site alignment, we should ask for the closest site to the target
        // position and then adjust to align with the site.  We should keep track of
        // the site too...


        r->m_nodes.erase( r->m_nodes.begin(), r->m_nodes.end() );

        r = l;
    }
}


} // namespace aak
