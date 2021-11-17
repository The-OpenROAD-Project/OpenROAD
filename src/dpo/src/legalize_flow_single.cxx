



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
#include <boost/unordered_map.hpp>
#include <boost/format.hpp>
#include "tbb/tick_count.h"
#include "combination.h"
#include "utility.h"
#include "plotgnu.h"
#include "color.h"
#include "legalize_flow_single.h"
#include "detailed_segment.h"
#include "detailed_manager.h"


#include <lemon/smart_graph.h>
#include <lemon/list_graph.h>
#include <lemon/cycle_canceling.h>
#include <lemon/cost_scaling.h>
#include <lemon/preflow.h>

//#include "CoinBuild.hpp"
//#include "CoinModel.hpp"
//#include "OsiClpSolverInterface.hpp"
//#include "CbcModel.hpp"


////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////



namespace aak 
{


////////////////////////////////////////////////////////////////////////////////
// Routines.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class NodeSorterForFlow
{
public:
    NodeSorterForFlow( FlowLegalizer* flowPtr, FlowLegalizerNode* srcPtr, FlowLegalizerNode* snkPtr ):
        m_flowPtr( flowPtr ), m_srcPtr( srcPtr ), m_snkPtr( snkPtr )
    {
        m_srcBinId  =   m_srcPtr->m_binId;
        m_srcY      =   m_srcPtr->getCenterY();
        m_srcX      =   m_srcPtr->getCenterX();

        m_snkBinId  =   m_snkPtr->m_binId;
        m_snkY      =   m_snkPtr->getCenterY();
        m_snkX      =   m_snkPtr->getCenterX();
        std::cout << "Sink 1: " << m_snkX << "," << m_snkY << std::endl;
    }
    bool operator()( const Node* ndi, const Node* ndj ) const 
    {
        double dispi = m_flowPtr->getDisp( ndi, m_snkX, m_snkY );
        double dispj = m_flowPtr->getDisp( ndj, m_snkX, m_snkY );

        bool retval = dispi < dispj;
        return dispi < dispj;
    }
public:
    FlowLegalizer*         m_flowPtr;
    FlowLegalizerNode*     m_srcPtr;
    FlowLegalizerNode*     m_snkPtr;

    int                     m_srcBinId;
    double                  m_srcY;
    double                  m_srcX;

    int                     m_snkBinId;
    double                  m_snkY;
    double                  m_snkX;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
FlowLegalizer::FlowLegalizer( FlowLegalizerParams& params ):
     m_params( params ), m_moveCounterLimit( 1 ),
     m_mgr( 0 ), m_plotter( 0 )
{
	;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
FlowLegalizer::~FlowLegalizer( void )
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizer::legalize( DetailedMgr* mgr )
{
    // Do all regions at the same time.
//    legalize(mgr, -1 );

    // Do all regions one-by-one.
    Architecture* arch = mgr->getArchitecture();
    int nRegions = arch->m_regions.size();
    for( int r = nRegions; r > 0; )
    {
        --r;
        legalize( mgr, r );
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizer::legalize( DetailedMgr* mgr, int regId )
{
    m_mgr = mgr;
    m_curRegionId = regId;

    m_arch = m_mgr->getArchitecture();
    m_network = m_mgr->getNetwork();
    m_rt = m_mgr->getRoutingParams();






    // Segments are all created, cells are assigned to segments, etc.  We
    // should only need to create the bins and then assign cells to bins.
    // From the bins, we can perform flow in a variety of ways.

    if( m_mgr->m_singleHeightCells.size() == 0 )
    {
        return;
    }


    collectSingleHeightCellStatistics();
    if( m_singleHeightCells.size() == 0 )
    {
        return;
    }
    std::cout << "Legalization of " << m_singleHeightCells.size() << " single height cells with flow" << ", "
        << "Region is " << m_curRegionId << std::endl;

    initBinWidth();
    double origBinWidth = m_binWidth;
    int pass = 1;
    int over = 0;

    // Setup and create bins.
    extractBins1();
    connectBins();

    // Assign cells to bins.
    assignCellsToBins();
    int nomerges = 0;
    while( true )
    {
        // Connect bins _after_ cell insertion.  It might be that we get bins
        // with no connections to other bins.  This is not a problem if the
        // disconnected bins don't have cells inside of them.

        over = countOverfilledBins();

        std::cout << "Outer pass " << pass << ", Bins is " << m_bins.size() << ", "
            << "Bin width is " << m_binWidth << ", "
            << "Bins overfilled is " << over
            << std::endl;

        if( over == 0 )
        {
            // Nothing more to do.
            break;
        }

        // Perform flow to reduce the number of overfilled bins.  The flow moves cells
        // between bins and updates bin and segment assignments, but the cell positions
        // are _not_ changed.
        pathFlow();

        if( m_binWidth > m_arch->getWidth() )
        {
            // Every segment was its own bin.  Nothing more to do.  Flow has degraded 
            // to a simple form of row juggling.
            break;
        }

        ++pass;

        m_binWidth = pass * origBinWidth;

        if( mergeBins() == 0 )
        {
            if( ++nomerges > 1 )
            {
                // If unable to merge bins after a few increases in bin width, then
                // we are stuck with the same problem over and over so quit.
                break;
            }
        }
    }
    // Merge all the bins.
    m_binWidth = m_arch->m_xmax - m_arch->m_xmin;
    mergeBins();

    //if( m_plotter )
    //{
    //    char buf[128];
    //    sprintf( &buf[0], "Flow Bins(end)" );
    //    m_plotter->Draw( m_network, m_arch, buf );
    //}


    // Whoops.  I think I need to be careful here.  The flow will move cells
    // between bins and will update segment contents, but will _NOT_ change
    // the positions of the cells within the segments!  We need to make a 
    // call to reposition cells in their segments.  However, if there are
    // multi-height cells present, this will mess things up since the order
    // of cells imposed by the binning might not correspond to the order of
    // the cells in the segments.  
    //
    // I think the simplest (best?) solution is to simply space cells out 
    // within their assigned bins.  This _will_ get the cells ordered 
    // correctly within their assigned bins which, in turn, should mean
    // the ordering within the segments will also be suitable.
    removeBinOverlap();

    m_mgr->removeSegmentOverlapSingle( m_curRegionId );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizer::collectSingleHeightCellStatistics( void )
{
    m_singleHeightCells.clear();
    m_singleRowHeight = m_arch->m_rows[0]->m_rowHeight;
    m_numSingleHeightRows = m_arch->m_rows.size();

    std::vector<double> widths;

    m_maxCellWidth = 0.;
    m_minCellWidth = std::numeric_limits<double>::max();
    m_avgCellWidth = 0.;
    m_medCellWidth = 0.;
    int count = 0;
    for( size_t i = 0; i < m_mgr->m_singleHeightCells.size(); i++ ) 
    {
        Node* ndi = m_mgr->m_singleHeightCells[i];

        // Skip if not in the current region.
        if( !(ndi->getRegionId() == m_curRegionId || m_curRegionId == -1) )
        {
            continue;
        }

        int spanned = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);
        if( spanned == 1 )
        {
            m_singleHeightCells.push_back( ndi );
        }

        m_maxCellWidth  = std::max( m_maxCellWidth, ndi->getWidth() );
        m_minCellWidth  = std::min( m_minCellWidth, ndi->getWidth() );
        m_avgCellWidth += ndi->getWidth(); 
        widths.push_back( ndi->getWidth() );
        ++count;
    }
    if( count != 0 )
    {
        m_avgCellWidth /= (double)count;

        std::sort( widths.begin(), widths.end() );
        m_medCellWidth = widths[widths.size() / 2];
    }

    std::cout  << "Min cell width is " << m_minCellWidth << ", "
        << "Max cell width is " << m_maxCellWidth << ", "
        << "Avg cell width is " << m_avgCellWidth << ", "
        << "Med cell width is " << m_medCellWidth
        << std::endl;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizer::initBinWidth( void )
{
    m_binWidth  = std::ceil( std::max( 1.1000 * m_maxCellWidth, m_params.m_tol * m_medCellWidth ) / m_params.m_ut );
    m_binHeight = m_singleRowHeight;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizer::extractBins1( void )
{
    // This routine creates bins over top of the segments.  It goes segment
    // by segment.  One thing it also does is to turn multi-height cells 
    // into blockages.  Thus, the created bins will not overlap with the
    // multi-height cells.
    //
    // TODO: I suppose I could skip segments which do not correspond to the
    // current region being considered.  However, I don't think there is a
    // problem with leaving them.

    for( size_t i = 0; i < m_bins.size(); i++ )
    {
        delete m_bins[i];
    }
    m_bins.erase( m_bins.begin(), m_bins.end() );

    m_binsInRow.clear();
    m_binsInSeg.clear();

    m_binsInRow.resize( m_numSingleHeightRows );
    m_binsInSeg.resize( m_mgr->m_segments.size() );
    int numBins = 0;

    double xmin = m_arch->m_xmin;
    double xmax = m_arch->m_xmax;
    unsigned maxBinsInRow = (int)std::ceil((xmax-xmin)/m_binWidth);

    findBlockagesForMultiHeightCells();

    for( int r = 0; r < m_numSingleHeightRows; r++ )
    {
        double siteSpacing  = m_arch->m_rows[r]->m_siteSpacing;

        double minY = m_arch->m_rows[r]->getY();
        double maxY = m_arch->m_rows[r]->getY() + m_arch->m_rows[r]->getH();

        std::vector<std::pair<double,double> >& blockages = m_multi_blockages[r];
        for( size_t s = 0; s < m_mgr->m_segsInRow[r].size(); s++ )
        {
            DetailedSeg* curr = m_mgr->m_segsInRow[r][s];

            // Figure out a list of sub-segments which avoids the blockages.
            std::vector<std::pair<double,double> > subsegs;

            double lx = curr->m_xmin;
            double rx = curr->m_xmax;

            int n = blockages.size();
            if( n == 0 )
            {
                subsegs.push_back( std::make_pair( lx, rx ) );
            }
            else
            {
                if( blockages[0].first > lx )
                {
                    double x1 = lx;
                    double x2 = std::min( rx, blockages[0].first );
                    if( x2 > x1 )
                    {
                        subsegs.push_back( std::make_pair( x1, x2 ) );
                    }
                }
                for( int i = 1; i < n; i++ )
                {
                    if( blockages[i].first > blockages[i-1].second )
                    {
                        double x1 = std::max( lx, blockages[i-1].second );
                        double x2 = std::min( rx, blockages[i-0].first  );

                        if( x2 > x1 )
                        {
                            subsegs.push_back( std::make_pair( x1, x2 ) );
                        }
                    }
                }
                if( blockages[n-1].second < rx )
                {
                    double x1 = std::min( rx, std::max( lx, blockages[n-1].second ) );
                    double x2 = rx;

                    if( x2 > x1 )
                    {
                        subsegs.push_back( std::make_pair( x1, x2 ) );
                    }
                }
            }
        
            // If there are no subsegments, but we somehow have _single height_ cells
            // in the segment, we sort of have a problem; the current code for 
            // assigning cells to bins will fail.  To avoid this problem, create a 
            // dummy bin with no width centered in the middle of the segments and 
            // come back and fix this later.  Note that this is not an issue if there
            // are no single height cells in the segment.
            if( subsegs.size() == 0 )
            {
                bool isSingle = false;
                for( size_t n = 0; n < m_mgr->m_cellsInSeg[curr->m_segId].size(); n++ )
                {
                    Node* ndi = m_mgr->m_cellsInSeg[curr->m_segId][n];

                    // Skip if not in the current region.
                    if( !(ndi->getRegionId() == m_curRegionId || m_curRegionId == -1) )
                    {
                        continue;
                    }

                    int spanned = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);
                    if( spanned == 1 )
                    {
                        isSingle = true;
                        break;
                    }
                }
                if( isSingle )
                {
                    std::cout << "Warning.  No sub-segments in a segment containing single height cells." << " "
                        << "Creating a dummy (null) bin." << std::endl;

                    double lx = 0.5 * (curr->m_xmin + curr->m_xmax);
                    double rx = 0.5 * (curr->m_xmin + curr->m_xmax);

                    // Single bin for segment.
                    FlowLegalizerNode* binPtr = new FlowLegalizerNode;
                    binPtr->m_binId = numBins;
                    binPtr->m_segId = curr->m_segId;
                    binPtr->m_regId = curr->m_regId; 
                    binPtr->setMinX( lx );
                    binPtr->setMaxX( rx );
                    binPtr->setMinY( minY );
                    binPtr->setMaxY( maxY );
                    binPtr->setMinRow( r );
                    binPtr->setMaxRow( r );
                    binPtr->setUt( m_params.m_ut );

                    m_binsInRow[r].push_back( binPtr );
                    m_binsInSeg[curr->m_segId].push_back( binPtr );
                    m_bins.push_back( binPtr );
                    ++numBins;

                    std::cout << boost::format( "Bin %6d, [ %.1lf , %.1lf ] (dummy)\n" )
                        % binPtr->m_binId % binPtr->getMinX() % binPtr->getMaxX();
                }
            }

            for( size_t s = 0; s < subsegs.size(); s++ )
            {
                double lx = subsegs[s].first ;
                double rx = subsegs[s].second;

                if( rx - lx <= m_binWidth+1.0e-3 )
                {
                    // Single bin for segment.
                    FlowLegalizerNode* binPtr = new FlowLegalizerNode;
                    binPtr->m_binId = numBins;
                    binPtr->m_segId = curr->m_segId;
                    binPtr->m_regId = curr->m_regId; 
                    binPtr->setMinX( lx );
                    binPtr->setMaxX( rx );
                    binPtr->setMinY( minY );
                    binPtr->setMaxY( maxY );
                    binPtr->setMinRow( r );
                    binPtr->setMaxRow( r );
                    binPtr->setUt( m_params.m_ut );

                    m_binsInRow[r].push_back( binPtr );
                    m_binsInSeg[curr->m_segId].push_back( binPtr );
                    m_bins.push_back( binPtr );
                    ++numBins;
                }
                else
                {
                    // Overlay regular bins.
                    FlowLegalizerNode* lastBinPtr = 0;
                    for( int b = 0;; ++b )
                    {
                        // Span of the bin.
                        double x1 = m_arch->m_xmin + b * m_binWidth;
                        double x2 = x1 + m_binWidth;

                        if( x2 <= lx ) continue;
                        if( x1 >= rx ) break;

                        double bl = std::max( x1, lx );
                        double br = std::min( x2, rx );
                        if( bl < br )
                        {
                            bool enlarge = true;
                            if( enlarge )
                            {
                                if( lastBinPtr == 0 )
                                {
                                    enlarge = false;
                                }
                            }
                            if( enlarge )
                            {
                                if( lastBinPtr->getMaxX() != bl )
                                {
                                    enlarge = false;
                                }
                            }
                            if( enlarge )
                            {
                                if( !((br-bl) <= m_binWidth-1.0e-3 || lastBinPtr->getWidth() <= m_binWidth-1.0e-3) )
                                {
                                    enlarge = false;
                                }
                            }
                            if( enlarge )
                            {
                                if( lastBinPtr->m_segId != curr->m_segId )
                                {
                                    enlarge = false;
                                }
                            }

                            if( enlarge )
                            {
                                lastBinPtr->setMinX( std::min( lastBinPtr->getMinX(), bl ) );
                                lastBinPtr->setMaxX( std::max( lastBinPtr->getMaxX(), br ) );
                            }
                            else
                            {
                                FlowLegalizerNode* binPtr = new FlowLegalizerNode;
                                binPtr->m_binId = numBins;
                                binPtr->m_segId = curr->m_segId;
                                binPtr->m_regId = curr->m_regId; 
                                binPtr->setMinX( bl );
                                binPtr->setMaxX( br );
                                binPtr->setMinY( minY );
                                binPtr->setMaxY( maxY );
                                binPtr->setMinRow( r );
                                binPtr->setMaxRow( r );
                                binPtr->setUt( m_params.m_ut );

                                m_binsInRow[r].push_back( binPtr );
                                m_binsInSeg[curr->m_segId].push_back( binPtr );
                                m_bins.push_back( binPtr );
                                ++numBins;

                                lastBinPtr = binPtr;
                            }
                        }
                    }
                }
            }
        }
    }

    // Some information about bins.
    std::vector<int> hist;
    for( size_t b = 0; b < m_bins.size(); b++ )
    {
        FlowLegalizerNode* ptr = m_bins[b];
        double width = ptr->getMaxX() - ptr->getMinX();

        int ix = (int) width / m_minCellWidth;
        if( ix >= hist.size() )
        {
            hist.resize( ix+1, 0 );
        }
        ++hist[ix];
    }

    if( 0 )
    {
        std::cout << "Histogram of bins widths:" << std::endl;
        for( size_t i = 0; i < hist.size(); i++ )
        {
            if( hist[i] == 0 )
            {
                continue;
            }
            int l = (int)(m_minCellWidth * (i));
            int r = (int)(m_minCellWidth * (i+1));
            std::cout << boost::format( "Widths [%8d,%8d]: %8d\n" ) % l % r % hist[i];
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizer::findBlockagesForMultiHeightCells( void )
{
    // Determine the single height segments and blockages.
    m_multi_blockages.resize( m_numSingleHeightRows );
    for( int i = 0; i < m_multi_blockages.size(); i++ )
    {
        m_multi_blockages[i] = std::vector<std::pair<double,double> >();
    }

    std::vector<Node*> multi;
    for( size_t i = 2; i < m_mgr->m_multiHeightCells.size(); i++ )
    {
        multi.insert( multi.end(), m_mgr->m_multiHeightCells[i].begin(), m_mgr->m_multiHeightCells[i].end() );
    }
    if( multi.size() == 0 )
    {
        return;
    }

    for( int i = 0; i < multi.size(); i++ )
    {
        Node* nd = multi[i];

        // Only concerned with cells in the specified region.
        if( !(nd->getRegionId() == m_curRegionId || m_curRegionId == -1) )
        {
            continue;
        }

        double xmin = std::max( m_arch->m_xmin, nd->getX() - 0.5 * nd->getWidth() );
        double xmax = std::min( m_arch->m_xmax, nd->getX() + 0.5 * nd->getWidth() );
        double ymin = std::max( m_arch->m_ymin, nd->getY() - 0.5 * nd->getHeight() );
        double ymax = std::min( m_arch->m_ymax, nd->getY() + 0.5 * nd->getHeight() );

        for( int r = 0; r < m_numSingleHeightRows; r++ )
        {
            double lb = m_arch->m_ymin + r * m_singleRowHeight;
            double ub = lb + m_singleRowHeight;

            if( !(ymax-1.0e-3 <= lb || ymin+1.0e-3 >= ub) )
            {
                m_multi_blockages[r].push_back( std::pair<double,double>(xmin,xmax) );
            }
        }
    }

    // Sort blockages and merge.
    for( int r = 0; r < m_numSingleHeightRows; r++ )
    {
        if( m_multi_blockages[r].size() == 0 )
        {
            continue;
        }

        std::sort( m_multi_blockages[r].begin(), m_multi_blockages[r].end(), compareBlockages() );

        std::stack< std::pair<double,double> > s;
        s.push( m_multi_blockages[r][0] );
        for( int i = 1; i < m_multi_blockages[r].size(); i++ )
        {
            std::pair<double,double> top = s.top(); // copy.
            if( top.second < m_multi_blockages[r][i].first )
            {
                s.push( m_multi_blockages[r][i] ); // new interval.
            }
            else
            {
                if( top.second < m_multi_blockages[r][i].second )
                {
                    top.second = m_multi_blockages[r][i].second; // extend interval.
                }
                s.pop(); // remove old.
                s.push( top ); // expanded interval.
            }
        }

        m_multi_blockages[r].erase( m_multi_blockages[r].begin(), m_multi_blockages[r].end() );
        while( !s.empty() )
        {
            std::pair<double,double> temp = s.top(); // copy.
            m_multi_blockages[r].push_back( temp );
            s.pop();
        }

        // Resort intervals.
        std::stable_sort( m_multi_blockages[r].begin(), m_multi_blockages[r].end(), DetailedMgr::compareBlockages() ); 
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int FlowLegalizer::mergeBins( void )
{
    // Merge bins into a coarser grid.  Try to keep the grid uniform.

    std::cout << "Merging bins with target width of " << m_binWidth << std::endl;

    double xmin = m_arch->m_xmin;
    double xmax = m_arch->m_xmax;
    unsigned maxBinsInRow = (int)std::ceil((xmax-xmin)/m_binWidth);
    std::vector<Node*>::iterator it;

    std::set<FlowLegalizerNode*> eliminated;
    for( int r = 0; r < m_numSingleHeightRows; r++ )
    {
        double siteSpacing  = m_arch->m_rows[r]->m_siteSpacing;
        double binWidth = std::floor( m_binWidth / siteSpacing ) * siteSpacing;

        double ymin = m_arch->m_rows[r]->getY();
        double ymax = m_arch->m_rows[r]->getY() + m_arch->m_rows[r]->getH();

        if( m_binsInRow[r].size() <= 1 )
        {
            continue;
        }

        int b = 0;
        while( b < m_binsInRow[r].size() )
        {
            FlowLegalizerNode* snkPtr = m_binsInRow[r][b];
            int ix = (int)((snkPtr->getCenterX() - xmin) / m_binWidth );
            double x1 = xmin + ix * m_binWidth;
            double x2 = x1 + m_binWidth;

            // See if adjacent bins also fall within this bucket.
            ++b;
            while( b < m_binsInRow[r].size() )
            {
                FlowLegalizerNode* srcPtr = m_binsInRow[r][b];
                int jx = (int)((srcPtr->getCenterX() - xmin) / m_binWidth );

                if( !(jx == ix && adjacentBins( snkPtr, srcPtr )) )
                {
                    break;
                }
                // Merge bins.

                // Move all cells from merged bin into the seed bin.  Need to be
                // careful about fractional cells.

                int srcId = srcPtr->m_binId;
                int snkId = snkPtr->m_binId;
                while( m_cellsInBin[srcId].size() != 0 )
                {
                    Node* ndi = m_cellsInBin[srcId].back();
                    int size = (int)ndi->getWidth();

                    FlowLegalizerNode* v = m_cellToBins[ndi->getId()].first ;
                    FlowLegalizerNode* w = m_cellToBins[ndi->getId()].second;
                    int area1 = m_areaToBins[ndi->getId()].first ;
                    int area2 = m_areaToBins[ndi->getId()].second;

                    if( v != srcPtr ) { std::swap(v,w); std::swap(area1,area2); }
                    if( v != srcPtr )
                    {
                        std::cout << "Error." << std::endl;
                        exit(-1);
                    }
                    // Add the cell to the sink.
                    it = std::find( m_cellsInBin[snkId].begin(), m_cellsInBin[snkId].end(), ndi );
                    if( w == snkPtr )
                    {
                        // Fractional assignment so the cell should already be in the sink.
                        if( m_cellsInBin[snkId].end() == it )
                        {
                            std::cout << "Error." << std::endl;
                            exit(-1);
                        }
                        snkPtr->addOcc( area1 );

                        m_cellToBins[ndi->getId()] = std::make_pair(snkPtr,(FlowLegalizerNode*)0);
                        m_areaToBins[ndi->getId()] = std::make_pair(size,0);
                    }
                    else
                    {
                        // No fractional assignment of this cell with the sink.
                        if( m_cellsInBin[snkId].end() != it )
                        {
                            std::cout << "Error." << std::endl;
                            exit(-1);
                        }
                        m_cellsInBin[snkId].push_back( ndi );
                        snkPtr->addOcc( area1 );

                        m_cellToBins[ndi->getId()] = std::make_pair(snkPtr,w);
                        m_areaToBins[ndi->getId()] = std::make_pair(area1,area2);
                    }

                    // Remove from the source.
                    m_cellsInBin[srcId].pop_back();
                    srcPtr->subOcc( area1 );

                    v = m_cellToBins[ndi->getId()].first ;
                    w = m_cellToBins[ndi->getId()].second;
                    area1 = m_areaToBins[ndi->getId()].first ;
                    area2 = m_areaToBins[ndi->getId()].second;
                }

                // Change the size of the merged bin and the seed bin.
                double xl = std::min( snkPtr->getMinX(), srcPtr->getMinX() );
                double xr = std::max( snkPtr->getMaxX(), srcPtr->getMaxX() );

                snkPtr->setMinX( xl );
                snkPtr->setMaxX( xr );
                srcPtr->setMinX( xr );
                srcPtr->setMaxX( xr );

                eliminated.insert( srcPtr );

                ++b;
            }
        }
    }
    std::cout << "Merging eliminated " << eliminated.size() << " bins." << std::endl;

    // Should we re-create certain structures to remove the set of eliminated
    // bins?  Or, is this more trouble than it is worth?  The code should
    // still work despite having these bins removed.  I think the biggest issue
    // is how I stupidly created a separate array for the cells within a bin.
    // However, it's fine if I don't use the bin Id to index the m_bins...

    // Debug.  Eliminated bins should not have cells in them.
    for( size_t i = 0; i < m_bins.size(); i++ )
    {
        FlowLegalizerNode* binPtr = m_bins[i];
        if( eliminated.end() == eliminated.find( binPtr ) )
        {
            continue;
        }
        int binId = binPtr->m_binId;
        if( m_cellsInBin[binId].size() != 0 )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
    }

    // We need to redo the links to avoid merged bins.
    std::cout << "Disconnecting bins." << std::endl;
    disconnectBins();

    // Remove the eliminated bins from several structures.
    std::cout << "Removing eliminated bins." << std::endl;
    for( size_t s = 0; s < m_binsInSeg.size(); s++ )
    {
        for( size_t i = m_binsInSeg[s].size(); i > 0; )
        {
            FlowLegalizerNode* binPtr = m_binsInSeg[s][--i];
            if( eliminated.end() != eliminated.find( binPtr ) )
            {
                m_binsInSeg[s].erase( m_binsInSeg[s].begin() + i );
            }
        }
    }
    for( size_t r = 0; r < m_binsInRow.size(); r++ )
    {
        for( size_t i = m_binsInRow[r].size(); i > 0; )
        {
            FlowLegalizerNode* binPtr = m_binsInRow[r][--i];
            if( eliminated.end() != eliminated.find( binPtr ) )
            {
                m_binsInRow[r].erase( m_binsInRow[r].begin() + i );
            }
        }
    }
    for( size_t i = m_bins.size(); i > 0; )
    {
        FlowLegalizerNode* binPtr = m_bins[--i];
        if( eliminated.end() == eliminated.find( binPtr ) )
        {
            continue;
        }
        m_bins.erase( m_bins.begin() + i );

        delete binPtr;
    }

    std::cout << "Connecting remaining bins." << std::endl;
    connectBins();

    int over = countOverfilledBins();
    std::cout << "Overfilled bins after bin merging is " << over << std::endl;

    return eliminated.size();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool FlowLegalizer::adjacentBins( FlowLegalizerNode* a, FlowLegalizerNode* b )
{
    // Checks of two bins are adjacent and, in other words, can be merged.

    // Avoid if either bin is NULL.
    if( a == NULL || b == NULL ) return false;
    // Avoid if bins are not in the same row.
    if( a->getMinRow() != b->getMinRow() ) return false;
    // Avoid if bins are in different regions.
    if( a->getRegId() != b->getRegId() ) return false;
    // Avoid if bins are in different segments.
    if( a->getSegId() != b->getSegId() ) return false;

    // Must now be adjacent on the left or the right.
    if( b->getMaxX() < a->getMaxX() )
    {
        return std::fabs( b->getMaxX() - a->getMinX() ) < 1.0e-3;
    }
    if( b->getMinX() > a->getMinX() )
    {
        return std::fabs( a->getMaxX() - b->getMinX() ) < 1.0e-3;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizer::connectBins( void )
{
    // Hook up bins.  Only hook up bins which are in the same region.  This will
    // prevent us from flowing cells into incorrect regions.  For narrow bins,
    // allow flow out of the bins, but do not allow flow into the bins.  The 
    // purpose here is to avoid getting "stuck" trying to push stuff into and
    // out of bins which can't accommodate anything.
    //
    // I've added something to try and avoid smaller bins.  These bins occur
    // between multi-height cells and they can prevent the flow from succeeding.
    // Basically, you get "stuck" if you need to find a path through a cell 
    // which cannot accommodate the incoming flow, but yet does not have 
    // enough occupancy to create the needed space.  To solve this problem,
    // I check if I have added edges to bins which are larger than some minimum
    // which will enable the path search to possibly bypass the smaller cells.

    tbb::tick_count tm1, tm2;

    tm1 = tbb::tick_count::now();

    // Horizontal links.
    int hcount1 = 0;
    for( int r = 0; r < m_numSingleHeightRows; r++ )
    {
        //std::cout << "Hori @ " << r << " of " << m_numSingleHeightRows << ", "
        //    << "Bins is " << m_binsInRow[r].size() << std::endl;
        for( size_t c = 0; c < m_binsInRow[r].size(); c++ )
        {
            FlowLegalizerNode* cBinPtr = m_binsInRow[r][c];

            // Find the first valid bin to the left.
            int nl = c;
            while( nl > 0 )
            {
                --nl;
                FlowLegalizerNode* lBinPtr = m_binsInRow[r][nl];
                if( cBinPtr->m_regId == lBinPtr->m_regId )
                {
                    // Avoid edge pointing at the left bin if the left bin is too narrow.
                    double width = lBinPtr->getMaxX() - lBinPtr->getMinX();
                    if( width >= m_minCellWidth-1.0e-3 )
                    {
                        ++hcount1;
                        cBinPtr->m_outEdges.push_back( new FlowLegalizerEdge( cBinPtr, lBinPtr ) );
                        if( width >= m_maxCellWidth-1.0e-3 )
                        {
                            break;
                        }
                    }
                }
            }
            int nr = c+1;
            while( nr < m_binsInRow[r].size() )
            {
                FlowLegalizerNode* rBinPtr = m_binsInRow[r][nr];
                if( cBinPtr->m_regId == rBinPtr->m_regId )
                {
                    // Avoid edge pointing at the left bin if the left bin is too narrow.
                    double width = rBinPtr->getMaxX() - rBinPtr->getMinX();
                    if( width >= m_minCellWidth-1.0e-3 )
                    {
                        ++hcount1;
                        cBinPtr->m_outEdges.push_back( new FlowLegalizerEdge( cBinPtr, rBinPtr ) );
                        if( width >= m_maxCellWidth-1.0e-3 )
                        {
                            break;
                        }
                    }
                }
                ++nr;
            }
        }
    }

    // Vertical links.  
    int vcount1 = 0;
    for( int r = 0; r < m_numSingleHeightRows; r++ )
    {
        for( size_t b = 0; b < m_binsInRow[r].size(); b++ )
        {
            FlowLegalizerNode* cBinPtr = m_binsInRow[r][b];

            // Above...
            bool af = false;
            for( int ar = r+1; ar < m_numSingleHeightRows; ar++ )
            {
                for( unsigned bb = 0; bb < m_binsInRow[ar].size(); bb++ )
                {
                    FlowLegalizerNode* aBinPtr = m_binsInRow[ar][bb];

                    double lx = std::max( cBinPtr->getMinX(), aBinPtr->getMinX() );
                    double rx = std::min( cBinPtr->getMaxX(), aBinPtr->getMaxX() );
                    if( rx-lx > 1.0e-3 )
                    {
                        if( cBinPtr->m_regId == aBinPtr->m_regId )
                        {
                            double width = aBinPtr->getMaxX() - aBinPtr->getMinX();
                            if( width >= m_minCellWidth-1.0e-3 )
                            {
                                ++vcount1;
                                cBinPtr->m_outEdges.push_back( new FlowLegalizerEdge( cBinPtr, aBinPtr ) );
                                if( width >= m_maxCellWidth-1.0e-3 )
                                {
                                    af = true;
                                }
                            }
                        }
                    }
                }
                if( af )
                {
                    break;
                }
            }

            // Below.
            bool bf = false;
            for( int rl = r-1; rl >= 0; rl-- )
            {
                for( unsigned bb = 0; bb < m_binsInRow[rl].size(); bb++ )
                {
                    FlowLegalizerNode* bBinPtr = m_binsInRow[rl][bb];

                    double lx = std::max( cBinPtr->getMinX(), bBinPtr->getMinX() );
                    double rx = std::min( cBinPtr->getMaxX(), bBinPtr->getMaxX() );
                    if( rx-lx > 1.0e-3 )
                    {
                        if( cBinPtr->m_regId == bBinPtr->m_regId )
                        {
                            double width = bBinPtr->getMaxX() - bBinPtr->getMinX();
                            if( width >= m_minCellWidth-1.0e-3 )
                            {
                                ++vcount1;
                                cBinPtr->m_outEdges.push_back( new FlowLegalizerEdge( cBinPtr, bBinPtr ) );
                                if( width >= m_maxCellWidth-1.0e-3 )
                                {
                                    bf = true;
                                }
                            }
                        }
                    }
                }
                if( bf )
                {
                    break;
                }
            }
        }
    }
    //std::cout << "Horizontal joins is " << hcount1 << ", Vertical is " << vcount1 << std::endl;

    tm2 = tbb::tick_count::now();
    std::cout << boost::format( "Connecting bins took %.4lf seconds." ) % (tm2-tm1).seconds() << std::endl;

    // Debug.  Look for bins that do not have any outgoing edges.  This could
    // be an issue if there are cells assigned to these bins.
    for( size_t b = 0; b < m_bins.size(); b++ )
    {
        FlowLegalizerNode* binPtr = m_bins[b];
        int binId = binPtr->getBinId();
        if( binPtr->m_outEdges.size() == 0 )
        {
            std::cout << "Warning, bin " << binId << " has no outgoing edges." << std::endl;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizer::disconnectBins( void )
{
    // Remove all edges.
    for( size_t i = 0; i < m_bins.size(); i++ )
    {
        m_bins[i]->deleteEdges();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizer::assignCellsToBins( void )
{
    // This routine assigned cells to bins.  It differs depending on whether or
    // not things are global or detailed.  If detailed, it will work off of
    // segments.  If global, it will insert into bins directly.

    m_cellToBins.erase( m_cellToBins.begin(), m_cellToBins.end() );
    m_cellToBins.resize( m_network->m_nodes.size() );
    m_areaToBins.erase( m_areaToBins.begin(), m_areaToBins.end() );
    m_areaToBins.resize( m_network->m_nodes.size() );

    m_cellsInBin.erase( m_cellsInBin.begin(), m_cellsInBin.end() );
    m_cellsInBin.resize( m_bins.size() );
    for( int i = 0; i < m_bins.size(); i++ ) 
    {
        m_bins[i]->setOcc( 0.0 );
        m_bins[i]->setGap( 0.0 );
    }
    int nSingle = 0;
    int nMulti = 0;
    int nTotal = 0;

    double minX, maxX, posY;
    int rowId;
    for( size_t i = 0; i < m_mgr->m_segments.size(); i++ )
    {
        DetailedSeg* segPtr = m_mgr->m_segments[i];
        int segId = segPtr->m_segId;

        nTotal += m_mgr->m_cellsInSeg[segId].size();
        for( size_t n = 0; n < m_mgr->m_cellsInSeg[segId].size(); n++ ) 
        {
            Node* nd = m_mgr->m_cellsInSeg[segId][n];

            // Skip if not in the current region.
            if( !(nd->getRegionId() == m_curRegionId || m_curRegionId == -1) )
            {
                continue;
            }

            // Debug.  This should be a single height cell.
            int nspanned = (int)((nd->getHeight() / m_singleRowHeight) + 0.5);
            if( nspanned != 1 )
            {
                ++nMulti;
                continue;
            }
            ++nSingle;

            FlowLegalizerNode* binPtr = findClosestBin( segPtr, nd );
            if( binPtr == 0 )
            {
                std::cout << "Error 7." << std::endl;
                exit(-1);
            }

            m_cellsInBin[binPtr->m_binId].push_back( nd );

            binPtr->addOcc( nd );
        }
    }
    // Is it necessary to sort the bins???
    for( size_t i = 0; i < m_bins.size(); i++ )
    {
        FlowLegalizerNode* binPtr = m_bins[i];
        int binId = binPtr->m_binId;
        std::stable_sort( m_cellsInBin[binId].begin(), m_cellsInBin[binId].end(), DetailedMgr::compareNodesX() );
    }

    // Right now, each cell exists in exactly one bin.
    for( size_t b = 0; b < m_bins.size(); b++ )
    {
        FlowLegalizerNode* binPtr = m_bins[b];
        int binId = binPtr->m_binId;
        for( size_t i = 0; i < m_cellsInBin[binId].size(); i++ )
        {
            Node* ndi = m_cellsInBin[binId][i];

            double size = (int)ndi->getWidth();

            m_cellToBins[ndi->getId()] = std::make_pair(binPtr,(FlowLegalizerNode*)0);
            m_areaToBins[ndi->getId()] = std::make_pair(size,0);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
FlowLegalizerNode* FlowLegalizer::findClosestBin( DetailedSeg* segment, Node* nd )
{
    // Only consider bins within the segment.  XXX: Updated to avoid picking
    // bins with no outgoing edges; we will not be able to flow out of such
    // bins...
    FlowLegalizerNode* retPtr = 0;
    double x = nd->getX();
    double y = nd->getY();
    double x1, x2, xx, dist1, dist2;
    int segId = segment->m_segId;

    if( m_binsInSeg[segId].size() == 0 )
    {
        std::cout << "No bins associated with segment " << segId << std::endl;
        exit(-1);
    }

    for( size_t b = 0; b < m_binsInSeg[segId].size(); b++ )
    {
        FlowLegalizerNode* binPtr = m_binsInSeg[segId][b];

        if( binPtr->m_outEdges.size() == 0 )
        {
            continue;
        }

        if( retPtr == 0 )
        {
            retPtr = binPtr;
        }
        else 
        {
            x1 = retPtr->getMinX() + 0.5 * nd->getWidth();
            x2 = retPtr->getMaxX() - 0.5 * nd->getWidth();
            xx = std::max( x1, std::min( x2, x ) );
            dist1 = std::fabs( xx - x );

            x1 = binPtr->getMinX() + 0.5 * nd->getWidth();
            x2 = binPtr->getMaxX() - 0.5 * nd->getWidth();
            xx = std::max( x1, std::min( x2, x ) );
            dist2 = std::fabs( xx - x );

            if( dist2 < dist1 )
            {
                retPtr = binPtr;
            }
        }
    }
    return retPtr;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizer::pathFlow( void )
{
    // This routine sets up a single flow problem and then costs a min-cost
    // max-flow solver; it tries to reduce as much overflow as possible.  It
    // will do multiple passes until it can't do anything else.
    bool didFindPaths = false;
    double dispFac = 0.5000;
    double dispInc = 0.5000;

    m_moveId = 0;
    m_moveCellCounter.resize( m_network->m_nodes.size() );
    m_moveCellMask.resize( m_network->m_nodes.size() );
    std::fill( m_moveCellMask.begin(), m_moveCellMask.end(), m_moveId );

    // Later on, I am going to want to know which segment each cell was 
    // orignally assigned to.  So, grab this information now.
    std::vector<int> orig_seg;
    orig_seg.resize( m_network->m_nodes.size() );
    std::fill( orig_seg.begin(), orig_seg.end(), -1 );
    for( size_t i = 0; i < m_mgr->m_segments.size(); i++ )
    {
        DetailedSeg* segPtr = m_mgr->m_segments[i];

        int segId = segPtr->m_segId;
        for( size_t n = 0; n < m_mgr->m_cellsInSeg[segId].size(); n++ ) 
        {
            Node* ndi = m_mgr->m_cellsInSeg[segId][n];

            // Skip if not in the current region.
            if( !(ndi->getRegionId() == m_curRegionId || m_curRegionId == -1) )
            {
                continue;
            }

            // If multi-height, we will overwrite previous values, but later
            // we only need the information for single height cells so this 
            // is entirely okay.
            orig_seg[ndi->getId()] = segId;
        }
    }

    // Continue to try and solve flow until the maximum displacement limit
    // is effectively across the entire chip.  At that point, we are done.
    // Stopping based on the maximum displacement is not the best way, but
    // we will eventually finish.
    m_maxDisplacement = dispFac * std::max( m_binWidth, m_binHeight );
    int pass = 0;
    int over = 0;
    while( true )
    {
        ++pass;

        if( m_maxDisplacement >= std::max(m_arch->getWidth(), m_arch->getHeight() ) )
        {
            //std::cout << "Hit displacement limit prior to pass " << pass << "; stopping." << std::endl;
            break;
        }

        over = countOverfilledBins();
        std::cout << " Inner pass " << pass << ", Max disp is " << m_maxDisplacement << ", "
            << "Bins overfilled is " << over << std::endl;

        // Keep searching for paths with the current displacement as long as we
        // find some.  Sooner or later, we will not find paths.  This could 
        // happen simply because cells are limited in how many times they are
        // allowed to move...
        std::fill( m_moveCellCounter.begin(), m_moveCellCounter.end(), 0 );
        if( over == 0 )
        {
            break;
        }

        didFindPaths = true;
        while( didFindPaths )
        {
            if( !setupFlowPaths( didFindPaths ) )
            {
                // This means something went wrong.
                break;
            }
            if( didFindPaths )
            {
                // This means we found some paths and moved some cells.  It could
                // be that continuing with this displacement limit will resolve
                // more issues.

                //std::cout << "Found paths, keeping current displacment." << std::endl;
            }
            else
            {
                // We could not find any paths.  We therefore must increase the
                // displacement limit.
                
                //std::cout << "Found no paths." << std::endl;
            }
        }

        //std::cout << "Pass " << pass << ", Out of paths." << std::endl;

        m_maxDisplacement += dispInc * std::max( m_binWidth, m_binHeight );
    }

    std::cout << "Correcting cell to segment assignments." << std::endl;

    // Single height cells have been reassigned to bins.  We need to do 
    // quite a few things now...  We need to get cells assigned to the
    // proper segments.  We need to deal with any fractional assignments
    // (possibly hard in the Y-direction if that was allowed?).  We need
    // to reposition the cells.  XXX: How to handle the presence of the
    // multi-height cells???

    // Scan all the bins.  For each cell in each bin check if it needs to
    // change segments and make that change.  For fractional cells, count
    // them.
    int changed = 0;
    int xu_frac = 0;
    int xt_frac = 0;
    int yu_frac = 0;
    int yt_frac = 0;
    for( size_t i = 0; i < m_bins.size(); i++ )
    {
        FlowLegalizerNode* u = m_bins[i];
        int binId = u->m_binId;
        int segId = u->m_segId;
        for( size_t j = 0; j < m_cellsInBin[binId].size(); j++ )
        {
            Node* ndi = m_cellsInBin[binId][j];

            // Is this a fractional node?  If it is, then we need to be a bit careful.
            FlowLegalizerNode* v = m_cellToBins[ndi->getId()].first ;
            FlowLegalizerNode* w = m_cellToBins[ndi->getId()].second;
            bool isFractional = ( v != 0 && w != 0 ) ? true : false;
            (void)isFractional;

            bool flag = true;
            if( v != 0 && w != 0 )
            {
                flag = false;
                // The bin "u" should be one of these two bins!
                if( !(u == v || u == w) )
                {
                    std::cout << "Error 8." << std::endl;
                    exit(-1);
                }
                if( std::fabs( v->getCenterY() - w->getCenterY() ) < 1.0e-3 )
                {
                    ++xt_frac;
                    // Fractional in the X-direction.  If the two bins are adjacent,
                    // then there is no issue, otherwise we have a problem.  If 
                    // the bins are within the same segment, then they should be
                    // adjacent.
                    if( v->m_segId == w->m_segId )
                    {
                        flag = true;
                    }
                    else
                    {
                        ++xu_frac;
                    }
                }
                else
                {
                    // Fractional in the Y-direction.  This is currently an issue.
                    ++yt_frac;
                    ++yu_frac;
                }
            }

            if( !flag )
            {
                std::cout << "Cell " << ndi->getId() << " skipped; unresolved fractional assignment." << std::endl;
                continue;
            }

            if( orig_seg[ndi->getId()] != segId )
            {
                // Cell was moved to a different segment.  We need to 
                // make this change.
                ++changed;

                m_mgr->removeCellFromSegment( ndi, orig_seg[ndi->getId()] );
                m_mgr->addCellToSegment( ndi, segId );

                // XXX: Update the segment for the cell.  This resolves any issues
                // with fractional cells.  Consider a fractional cell (X-direction).
                // it might have flowed out of its original (single) bin and 
                // original (single) segment.  Now, it is in two (different) bins
                // in a different (single) segment.  We will encounter this cell
                // twice when scanning the bins.  The first time, we will move it
                // to its new segment.  But, when we encounter it the second time,
                // it is already where it needs to be.  If we don't update the 
                // original segment information, then we will attempt to remove it
                // from the wrong place the second time.
                orig_seg[ndi->getId()] = segId;
            }
        }
    }
    std::cout << "Cells that changed segments is " << changed << std::endl;
    std::cout << "Encountered fractional cells in the Y-direction is " << yt_frac << std::endl;
    std::cout << "Encountered fractional cells in the X-direction is " << xt_frac << std::endl;
    std::cout << "Unresolved fractional cells in the Y-direction is " << yu_frac << std::endl;
    std::cout << "Unresolved fractional cells in the X-direction is " << xu_frac << std::endl;

    if( yu_frac != 0 || xu_frac != 0 )
    {
        std::cout << "Error 9." << std::endl;
        exit(-1);
    }
}

class compareBinsByDist
{
public:
    bool operator()( FlowLegalizerNode* a, FlowLegalizerNode* b ) const
    {
        if( a->m_dist == b->m_dist )
        {
            return a->m_binId < b->m_binId;
        }
        return a->m_dist > b->m_dist;
    }
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int FlowLegalizer::countOverfilledBins( void )
{
    std::vector<FlowLegalizerNode*> overfilled;
    findOverfilledBins( overfilled );
    return overfilled.size();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool FlowLegalizer::setupFlowPaths( bool& didFindPaths, int verbosity )
{
    didFindPaths = false;

    double cap;
    double occ;
    int demand;
    int supply;
    int nopaths;
    std::vector<Node*> nodes;

    // Find the overfilled bins.  XXX: Do we need to sort to work on the worst
    // ones first?  Does this really matter?
    std::vector<FlowLegalizerNode*> overfilled;
    findOverfilledBins( overfilled );
    if( overfilled.size() == 0 )
    {
        //std::cout << "No overfilled bins." << std::endl;
        return true;
    }
    //std::cout << "Identified " << overfilled.size() << " overfilled bins." << std::endl;

    // Setup costs on edges.
    for( size_t i = 0; i < m_bins.size(); i++ )
    {
        FlowLegalizerNode* v = m_bins[i];

        double srcX = v->getCenterX();
        double srcY = v->getCenterY();
        for( size_t j = 0; j < v->m_outEdges.size(); j++ )
        {
            FlowLegalizerEdge* e = v->m_outEdges[j];
            FlowLegalizerNode* u = e->m_snkPtr;

            double snkX = u->getCenterX();
            double snkY = u->getCenterY();
            // Cost from v->u.
            e->m_cost = std::fabs( srcX-snkX ) + std::fabs( srcY-snkY );
        }
    }

    std::vector<std::pair<FlowLegalizerNode*,double> > neighbours;
    for( size_t binIx = 0; binIx < overfilled.size(); binIx++ )
    {
        FlowLegalizerNode* rootPtr = overfilled[binIx];
        int rootId = rootPtr->m_binId;

        cap = rootPtr->getCap();
        occ = rootPtr->getOcc();
        if( occ <= cap+1.0e-3 )
        {
            // Not overfilled now so skip.
            continue;
        }

        // Set the demand that we are looking to achieve.  Right now, it is limited
        // to either the overflow or the maximum cell width, whichever is less.

        // Search for paths.
        nopaths = 0;
        int counter = 0;
        for(;;)
        {   
            // Compute the demand which we will attempt to achieve.
            cap = rootPtr->getCap();
            occ = rootPtr->getOcc();
            demand = (int)std::min( std::max( occ-cap, 0.0 ), m_maxCellWidth );
            if( demand == 0 )
            {
                break;
            }
            demand = (nopaths > 0) ? (int)m_minCellWidth : demand;

            // Initializations.
            for( size_t i = 0; i < m_bins.size(); i++ )
            {
                FlowLegalizerNode* binPtr = m_bins[i];
                binPtr->m_dist = std::numeric_limits<int>::max();
                binPtr->m_parent = NULL;
                binPtr->m_demand = 0;
            }

            // For back-tracking.
            rootPtr->m_demand = demand; 
            rootPtr->m_parent = NULL;
            rootPtr->m_dist = 0;

            // Search from the root bin.
            std::priority_queue<FlowLegalizerNode*,std::vector<FlowLegalizerNode*>,compareBinsByDist> bfsQ;
            bfsQ.push( rootPtr );
            bool found = false;
            FlowLegalizerNode* endPtr = 0;

            //std::cout << "Start, " << rootPtr->m_binId << std::endl;
            while( bfsQ.size() != 0 )
            {
                FlowLegalizerNode* v = bfsQ.top();
                bfsQ.pop();

                // How to tell when done?  We are done if the current bin is not
                // the root and can sink all of the remaining demand.
                {
                    if( v != rootPtr )
                    {
                        demand = v->m_demand;
                        cap = v->getCap();
                        occ = v->getOcc();
                        supply = std::max( cap-occ, 0.0 );
                        if( supply >= demand-1.0e-3 )
                        {
                            ++counter;
                            /*
                            std::cout << "Found path " << counter << ", Cost is " << v->m_dist << std::endl;
                            std::vector<FlowLegalizerNode*> path;
                            FlowLegalizerNode* t = v;
                            for( ; v != NULL; v = v->m_parent )
                            {
                                path.push_back( v );
                            }
                            for( size_t i = 0; i < path.size()-1; i++ )
                            {
                                FlowLegalizerNode* u = path[i];
                                FlowLegalizerNode* v = path[i+1];
                                std::cout << "v->u is " << v->m_binId << "->" << u->m_binId << std::endl;
                            }
                            v = t;
                            */
                            //if( counter == 5 )
                            {
                                found = true;
                                endPtr = v;
                                break;
                            }
                        }
                    }
                }

                // Determine the demand entering into v, determine how much of 
                // this demand can be absorbed by v, and then determine the 
                // amount which needs to leave v.

                // Flow entering into v.
                demand = v->m_demand; 

                // Determine how much flow needs to leave v.
                if( v != rootPtr )
                {
                    // Non-root bin can absorb some of the flow.
                    cap = v->getCap();
                    occ = v->getOcc();
                    supply = (int)std::max( cap-occ, 0.0 );
                    demand = std::max(demand - supply, 0 );
                }

                if( demand <= 1.0e-3 )
                {
                    continue;
                }

                // Determine neighbours of v.  Generally this is the outgoing edges of v.  
                // However, we might have "bad" neighbours.  A bad neighbour would be,
                // for example, a neighbour which cannot even accommodate the smallest 
                // cell in bin v and has no cells; we will never be able to satisfy the
                // incoming demand.
                // v -> u.
                for( size_t i = 0; i < v->m_outEdges.size(); i++ )
                {
                    FlowLegalizerEdge* e = v->m_outEdges[i];
                    double weight = e->m_cost;
                
                    FlowLegalizerNode* u = e->m_snkPtr;



                    // See if we can achieve the flow.
                    double hpwl = 0.;
                    double disp = 0.;
                    int nMoveLimit = 0;
                    int nDispLimit = 0;
                    int nFractions = 0;
                    int result = computeFlow1( v, u, nodes, demand, hpwl, disp, nMoveLimit, nDispLimit, nFractions, verbosity );
                    if( result < demand )
                    {
                        continue;
                    }
                    // It might be that we flowed more than required so reset the demand to how
                    // much we actually will try to move into u.
                    demand = result;

                    // Determine if this path is better.
                    //if( u->m_dist > v->m_dist + weight )
                    if( u->m_dist > v->m_dist + disp )
                    {
                        //std::cout << "adding v->u is " << v->m_binId << "->" << u->m_binId << ", cost is " << v->m_dist + disp << std::endl;
                        u->m_parent = v;
                        u->m_demand = demand;
                        //u->m_dist = v->m_dist + weight;    
                        u->m_dist = v->m_dist + disp;    
                        bfsQ.push( u );
                    }
                }
            }

            if( found )
            {
                didFindPaths = true;

                // We've found a path to reduce the overflow.  Trace the
                // path and move cells.
                if( endPtr == 0 )
                {
                    std::cout << "Error 12." << std::endl;
                    exit(-1);
                }

                std::vector<FlowLegalizerNode*> path;
                FlowLegalizerNode* v = endPtr;
                for( ; v != NULL; v = v->m_parent )
                {
                    path.push_back( v );
                }
                //std::cout << "Found path!  src is " << rootPtr->m_binId << ", end is " << endPtr->m_binId << ", "
                //    << "Consists of " << path.size() << " bins." << std::endl;


                // Scan the path and move the cells.  Process bins in reverse order
                // to avoid picking cells which have moved multiple times.
                for( size_t i = 0; i < path.size()-1; i++ )
                {
                    FlowLegalizerNode* u = path[i];
                    FlowLegalizerNode* v = path[i+1];
                    demand = u->m_demand;

                    //std::cout << "v->u is " << v->m_binId << "->" << u->m_binId << ", flow is " << demand << std::endl;

                    double hpwl = 0.;
                    double disp = 0.;
                    int nMoveLimit = 0;
                    int nDispLimit = 0;
                    int nFractions = 0;
                    int result = computeFlow1( v, u, nodes, demand, hpwl, disp, nMoveLimit, nDispLimit, nFractions, verbosity );
                    if( result != demand )
                    {
                        std::cout << "Error - 14." << ", " << result << ", " << demand << std::endl;
                        computeFlow1( v, u, nodes, demand, hpwl, disp, nMoveLimit, nDispLimit, nFractions, 10 );
                        exit(-1);
                    }
                    else 
                    {
                        ;
                    }
                    int moved = moveFlow1( v, u, nodes, demand, verbosity );
                    if( result != moved )
                    {
                        std::cout << "Error 14." << std::endl;
                        exit(-1);
                    }
                }
            }
            else
            {
                ++nopaths;
                if( nopaths > 1 )
                {
                    break;
                }
            }
        }

        //std::cout << "Done bin " << rootId << ", "
        //    << "Cap is " << rootPtr->getCap() << ", "
        //    << "Occ is " << rootPtr->getOcc() << ", "
        //    << std::endl;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizer::findOverfilledBins( std::vector<FlowLegalizerNode*>& overfilled )
{
    overfilled.clear();
    for( size_t i = 0; i < m_bins.size(); i++ )
    {
        FlowLegalizerNode* binPtr = m_bins[i];
        int binId = binPtr->m_binId;

        if( binPtr->getOcc() >= binPtr->getCap() + 1.0e-3 )
        {
            overfilled.push_back( binPtr );
        }
    }
}

struct compareNodeDistPair
{
    inline bool operator()( std::pair<Node*,double> i1, std::pair<Node*,double> i2 ) const
    {
        if( i1.second == i2.second )
        {
            if( (int)i1.first->getWidth() == (int)i2.first->getWidth() )
            {
                return (int)i1.first->getId() < (int)i2.first->getId();
            }
            return i1.first->getWidth() < i2.first->getWidth();
        }
        return i1.second < i2.second;
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool FlowLegalizer::subsetSum( std::vector<int>& a, int n, int sum )
{
    // Crappy algo to check for subset sum.
    if( sum == 0 ) return true;
    if( n < 0 || sum < 0 ) return false;
    return subsetSum( a, n-1, sum-a[n] ) || subsetSum( a, n-1, sum );
}

/**
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool FlowLegalizer::knapsack( std::vector<double>& c, std::vector<int>& a, int W, std::vector<int>& x )
{
    // Solve a knapsack problem: min \sum c_i x_i :sum a_i x_i >= W.

    std::fill( x.begin(), x.end(), 0 );

    int n = c.size();

    if( x.size() >=  10 ) return false; // Problem too large.
    if( x.size() ==   0 ) return true; // Bogus.
    // XXX: Should I bail if the problem is too large?
    if( x.size() == 1 ) 
    {
        x[0] = (a[0] >= W) ? 1 : 0;
        return (a[0] >= W);
    }

    // Find upper limit of what we can get; i.e., closest value >= W.
    int sum = 0;
    for( int i = 0; i < n ; i++ )
    {
        sum += a[i];
        if( sum >= W ) break;
    }
    if( sum < W ) return false; // No possible solution.
    int U = W;
    for( ; U <= sum; U++ )
    {
        if( subsetSum( a, n-1, U ) ) break;
    }
//    std::cout << "=> Should get == " << U << std::endl;

    CoinModel build;
    for( int i = 0; i < n; i++ )
    {
        build.setInteger( i );
        build.setColumnBounds( i, 0.0, 1.0 );
        build.setColumnObjective( i, c[i] );
        build.setElement( 0, i, (double)a[i] );

//        std::cout << "i: " << i << ", c: " << c[i] << ", a: " << a[i] << std::endl;
    }
    build.setRowLower( 0, (double)W );
    build.setRowUpper( 0, (double)U );
//    std::cout << "W: " << W << std::endl;

    OsiClpSolverInterface solver;
    if( solver.loadFromCoinModel( build ) != 0 )
    {
        std::cout << "Error." << std::endl;
        exit(-1);
    }
    solver.setLogLevel( 0 );
    solver.messageHandler()->setLogLevel( 0 );

    CbcModel model( solver );
    model.setLogLevel( 0 );
    model.branchAndBound();

    if( model.isProvenOptimal() )
    {
        for( int i = 0; i < n; i++ )
        {
            x[i] = model.bestSolution()[i];
//            std::cout << "Solution " << i << ", " << model.bestSolution()[i] << std::endl;
        }
        return true;
    }
    else
    {
//        std::cout << "No solution." << std::endl;
    }
    return false;
}
*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int FlowLegalizer::computeFlow1( FlowLegalizerNode* srcPtr, FlowLegalizerNode* snkPtr, 
    std::vector<Node*>& nodes, int demand, double& hpwl, double& disp,
    int& nMoveLimitExceeded, int& nDispLimitExceeded, int& nFractional, int verbosity )
{
    // Upon return, the nodes are those which can be moved from the source bin to
    // the sink bin.  One of the cells could be a fractional cell.  We take into
    // account displacement, but if there is a fractional cell, then we have to
    // ignore displacement.
    //
    // For debugging, it would be useful to record some things to understand why 
    // the flow has not been possible.

    hpwl = 0.;
    disp = 0.;

    nodes.erase( nodes.begin(), nodes.end() );

    int srcId = srcPtr->m_binId;

    //bool useKnapsack = false; // Makes code very slow, but more "optimal". :(.  On or off???

    double srcX = srcPtr->getCenterX();
    double srcY = srcPtr->getCenterY();

    double snkX = snkPtr->getCenterX();
    double snkY = snkPtr->getCenterY();

    std::vector<std::pair<Node*,double> > sorted;
    std::vector<double> obj;
    std::vector<int> wts;
    std::vector<int> sln;

    // XXX: Right now avoid fractional flows between bins that are not adjacent
    // within the same segment.  This avoids fractional flows in the Y-direction
    // as well as in the X-direction if bins are adjacent.
    bool avoidFractional = false;
    if( !adjacentBins( srcPtr, snkPtr ) )
    {
        avoidFractional = true;
    }

    if( verbosity > 1 )
    {
        std::cout << "v->u is " << srcPtr->m_binId << "->" << snkPtr->m_binId << ", "
            << "Avoid fractional? " << (avoidFractional?"Y":"N") << std::endl;
    }

    nMoveLimitExceeded = 0;
    nDispLimitExceeded = 0;
    nFractional = 0;

    int nfrac = 0;
    int flow = 0;

    ++m_moveId;

    // Step 1. See if there is a fractional cell.  
    if( flow < demand )
    {
        if( !avoidFractional )
        {
            for( size_t i = 0; i < m_cellsInBin[srcId].size(); i++ )
            {
                Node* ndi = m_cellsInBin[srcId][i];

                if( m_moveCellMask[ndi->getId()] == m_moveId )
                {
                    continue;
                }

                int size = (int)ndi->getWidth();

                FlowLegalizerNode* binPtr1 = m_cellToBins[ndi->getId()].first ;
                FlowLegalizerNode* binPtr2 = m_cellToBins[ndi->getId()].second;
                int area1 = m_areaToBins[ndi->getId()].first ;
                int area2 = m_areaToBins[ndi->getId()].second;
                if( !((srcPtr == binPtr1 && snkPtr == binPtr2) || (srcPtr == binPtr2 && snkPtr == binPtr1)) )
                {
                    continue;
                }

                // Fractional between the two bins.  We either move all of the remaining
                // area in the source bin or just enough more to satisfy the flow.
                m_moveCellMask[ndi->getId()] = m_moveId;

                if( srcPtr == binPtr2 )
                {
                    std::swap( binPtr1, binPtr2 );
                    std::swap( area1, area2 );
                }

                // TODO: Could be more accurate; since this cell is fractional, we know exactly
                // where it goes.
                double xl = snkPtr->getMinX() + 0.5 * ndi->getWidth();
                double xr = snkPtr->getMaxX() - 0.5 * ndi->getWidth();
                double xx = std::max( xl, std::min( xr, ndi->getOrigX() ) );
                double yy = snkY;
                double dx = std::fabs( xx - ndi->getOrigX() );
                double dy = std::fabs( yy - ndi->getOrigY() );
                sorted.push_back( std::make_pair( ndi, (dx+dy) ) );
                //sorted.push_back( std::make_pair( ndi, getDisp( ndi, snkX, snkY ) ) );
                flow += std::min( area1, demand );

                if( verbosity > 1 )
                {
                    std::cout << "Cell " << ndi->getId() << " fractional in "
                        << binPtr1->m_binId << "," << binPtr2->m_binId
                        << ", " << area1 << "," << area2 
                        << std::endl;
                }

                ++nfrac;
            }
        }
    }
    if( nfrac >= 2 )
    {
        std::cout << "Error 15." << std::endl;
        exit(-1);
    }
    // Step 2. Gather up entire cells which we can consider moving.  Cells cannot
    // be fractional.  Skip cells which have moved too many times or violate some
    // other sort of constraint (e.g., displacement).
    if( flow < demand )
    {
        for( size_t i = 0; i < m_cellsInBin[srcId].size(); i++ )
        {
            Node* ndi = m_cellsInBin[srcId][i];
            int size = (int)ndi->getWidth();

            if( verbosity > 0 )
            {
                std::cout << "Considering cell " << ndi->getId()
                    << ", Size " << size << ", disp is " << getDisp( ndi, snkX, snkY ) << std::endl;
            }

            // Already moved.
            if( m_moveCellMask[ndi->getId()] == m_moveId )
            {
                if( verbosity > 0 ) std::cout << "Skip - already in list." << std::endl;
                continue;
            }
            // Moved too many times.
            if( m_moveCellCounter[ndi->getId()] >= m_moveCounterLimit )
            {
                if( verbosity > 0 ) std::cout << "Skip - moved too much." << std::endl;
                ++nMoveLimitExceeded;
                continue;
            }
            // Violates the maximum displacement.
            if( getDisp( ndi, snkX, snkY ) > getMaxDisp() )
            {
                if( verbosity > 0 ) std::cout << "Skip - violates disp limit." << std::endl;
                ++nDispLimitExceeded;
                continue;
            }
            // Fractional.
            FlowLegalizerNode* binPtr1 = m_cellToBins[ndi->getId()].first ;
            FlowLegalizerNode* binPtr2 = m_cellToBins[ndi->getId()].second;
            if( binPtr1 != 0 && binPtr2 != 0 )
            {
                if( verbosity > 0 ) std::cout << "Skip - fractional." << std::endl;
                ++nFractional;
                continue;
            }

            double xl = snkPtr->getMinX() + 0.5 * ndi->getWidth();
            double xr = snkPtr->getMaxX() - 0.5 * ndi->getWidth();
            double xx = std::max( xl, std::min( xr, ndi->getOrigX() ) );
            double yy = snkY;
            double dx = std::fabs( xx - ndi->getOrigX() );
            double dy = std::fabs( yy - ndi->getOrigY() );
            sorted.push_back( std::make_pair( ndi, (dx+dy) ) );
            //sorted.push_back( std::make_pair( ndi, getDisp( ndi, snkX, snkY ) ) );

            m_moveCellMask[ndi->getId()] = m_moveId;
        }
    }

    if( verbosity > 0 )
    {
        std::cout << "Fractional count is " << nfrac << std::endl;
    }

    // TODO: Something smart to reduce the number of cells being considered and
    // improve the quality of the selection; e.g., Quality of selection would 
    // be to use knapsack to pick subset of cells to reduce displacement.  Given
    // cells of width K... if n*K >= flow, then we can immediately remove cells
    // which are further if we have > n of them since we would never pick these
    // ones.
    ;

    // Sort.
    std::stable_sort( sorted.begin()+nfrac, sorted.end(), compareNodeDistPair() );

    /*
    if( useKnapsack )
    {
        obj.clear();
        wts.clear();
        sln.clear();
        for( size_t i = nfrac; i < sorted.size(); i++ )
        {
            obj.push_back( sorted[i].second );
            wts.push_back( (int)sorted[i].first->getWidth() );
            sln.push_back( 0 );
        }

        if( knapsack( obj, wts, (int)(demand-flow), sln ) )
        {
            for( size_t i = sln.size(); i > 0; )
            {
                if( sln[--i] == 0 )
                {
                    sorted.erase( sorted.begin() + i + nfrac );
                }
            }
        }
    }
    */

    // Consider nodes in sorted order.  Select cells to obtain the desired flow.
    // If we are trying to avoid fractional flow, we really should be smarter
    // about things (e.g., some sort of subset sum), but try to get a flow 
    // which is as close (or slightly larger) than the target flow.
    if( avoidFractional )
    {
        for( size_t i = nfrac; i < sorted.size(); i++ )
        {
            Node* ndi = sorted[i].first;
            int size = (int)ndi->getWidth();
            int newflow = flow + size;

            if( verbosity > 0 )
            {
                std::cout << "Cell " << ndi->getId() << ", Size " << size << ", new flow " << newflow << std::endl;
            }

            if( newflow > demand )
            {
                // If we include this cell, then we are going to exceed
                // the flow we actually need.  It would possibly be 
                // better to consider future cells to see if we can get
                // something that helps us to match the flow exactly
                // (or not... just let the flow get exceeded in which case
                // we _should_ include this cell).
                ++i;
                sorted.erase( sorted.begin()+i, sorted.end() ); 
                flow = newflow;
                break;
            }
            else if( newflow == demand )
            {
                // Should include this cell and remove remaining cells.
                ++i;
                sorted.erase( sorted.begin()+i, sorted.end() );
                flow = newflow;
                break;
            }
            else
            {
                // Still have cells and not at the flow so keep going.
                flow = newflow; 
            }
        }

        if( verbosity > 0 )
        {
            std::cout << "Node is " << sorted.size() << ", Flow is " << flow << std::endl;
        }
    }
    else
    {
        for( size_t i = nfrac; i < sorted.size(); i++ )
        {
            Node* ndi = sorted[i].first;
            int size = (int)ndi->getWidth();

            // Compute the flow if we include this node.  If fractional is
            // allowed, then we can max out at the demand.
            int newflow = flow + size;
            newflow = (newflow <= demand) ? newflow : demand;

            // Decide what to do.
            if( newflow > demand )
            {
                // We should not include this cell or any of the remaining
                // cells.  Erase them and quit without updating the flow.
                sorted.erase( sorted.begin()+i, sorted.end() );
                break;
            }
            else if( newflow == demand )
            {
                // We should include this cell (possibly fractional), but
                // exclude any remaining cells and quit.  We should update
                // the flow.
                ++i;
                sorted.erase( sorted.begin()+i, sorted.end() );
                flow = newflow;
                break;
            }
            else
            {
                // Still have cells and not at the flow so keep going.
                flow = newflow; 
            }
        }
    }

    nodes.erase( nodes.begin(), nodes.end() );
    disp = 0.;
    for( size_t i = 0; i < sorted.size(); i++ )
    {
        nodes.push_back( sorted[i].first );
        disp += sorted[i].second;
    }


    return flow;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int FlowLegalizer::moveFlow1( FlowLegalizerNode* srcPtr, FlowLegalizerNode* snkPtr, 
    std::vector<Node*>& nodes, int demand, int verbosity )
{
    // We are provided a list of nodes involved in the flow and the demand we
    // should be able to achieve.  The first node might be fractional and
    // the last node might need to be made fractional.   Nodes are expected 
    // to be sorted in the order we are moving them.

    int srcId = srcPtr->m_binId;
    int snkId = snkPtr->m_binId;

    double srcX = srcPtr->getCenterX();
    double srcY = srcPtr->getCenterY();

    double snkX = snkPtr->getCenterX();
    double snkY = snkPtr->getCenterY();

    bool avoidFractional = false;
    if( !adjacentBins( srcPtr, snkPtr ) )
    {
        avoidFractional = true;
    }

    if( verbosity > 0 )
    {
        std::cout << "Moving cells: "
            << "SRC: " << srcPtr->getCenterX() << "," << srcPtr->getCenterY() << " -> "
            << "SNK: " << snkPtr->getCenterX() << "," << snkPtr->getCenterY() << ", "
            << "SRC: Cap is " << srcPtr->getCap() << ", Occ is " << srcPtr->getOcc() << ", "
            << "SNK: Cap is " << snkPtr->getCap() << ", Occ is " << snkPtr->getOcc() << ", "
            << "Candidates is " << nodes.size() << ", "
            << "Demand is " << demand 
            << std::endl;
    }

    std::vector<Node*>::iterator it;
    int flow = 0;
    for( size_t i = 0; i < nodes.size(); i++ )
    {
        Node* ndi = nodes[i];
        int size = (int)ndi->getWidth();

        FlowLegalizerNode* binPtr1 = m_cellToBins[ndi->getId()].first ;
        FlowLegalizerNode* binPtr2 = m_cellToBins[ndi->getId()].second;
        int area1 = m_areaToBins[ndi->getId()].first ;
        int area2 = m_areaToBins[ndi->getId()].second;
        if( binPtr1 != 0 && binPtr2 != 0 )
        {
            if( avoidFractional )
            {
                std::cout << "Error 16." << std::endl;
                exit(-1);
            }

            // Fractional node.  
            if( i != 0 )
            {
                std::cout << "Error 17." << std::endl;
                exit(-1);
            }

            if( srcPtr == binPtr2 )
            {
                std::swap( binPtr1, binPtr2 );
                std::swap( area1, area2 );
            }
            if( srcPtr != binPtr1 || snkPtr != binPtr2 )
            {
                std::cout << "Error 18." << std::endl;
                exit(-1);
            }
     
            // Move the cell.  If the cell remains fractional, we can just update
            // things and return.  If the cell moves entirely, then we need to 
            // make the cell "not fractional" and proceed to move other cells.
            if( flow+area1 <= demand )
            {
                if( verbosity > 0 )
                {
                    std::cout << "Moving fractional cell entirely, "
                        << "Size is " << size << ", Area1 is " << area1 << ", Area2 is " << area2
                        << std::endl;
                }

                // Cell ceases to be fractional.  Remove it from the source bin and
                // make sure it is in the destination bin.  Update the information
                // about fractions.

                // Fractional stuff.
                m_cellToBins[ndi->getId()] = std::make_pair(snkPtr,(FlowLegalizerNode*)0);
                m_areaToBins[ndi->getId()] = std::make_pair(size,0);

                // Remove from source bin.
                it = std::find( m_cellsInBin[srcId].begin(), m_cellsInBin[srcId].end(), ndi );
                if( m_cellsInBin[srcId].end() == it )
                {
                    std::cout << "Error 19." << std::endl;
                    exit(-1);
                }
                m_cellsInBin[srcId].erase( it );

                // Add to destination bin (it's fractional so it should be there).
                it = std::find( m_cellsInBin[snkId].begin(), m_cellsInBin[snkId].end(), ndi );
                if( m_cellsInBin[snkId].end() == it )
                {
                    std::cout << "Error 20." << std::endl;
                    exit(-1);
                }

                // Update occupancies.
                srcPtr->subOcc( (double)area1 );
                snkPtr->addOcc( (double)area1 );

                // If a fractional cell has moved entirely into a new bin, then we consider
                // it as moved.
                ++m_moveCellCounter[ndi->getId()];

                // Update flow.
                flow += area1;
            }
            else 
            {
                // We just need to move a bit more of the cell.  It still remains in both bins.

                int diff = demand-flow;

                if( verbosity > 0 )
                {
                    std::cout << "Moving fractional cell fractionally, "
                        << "Size is " << size << ", "
                        << "Area1 is " << area1 << ", "
                        << "Area2 is " << area2 << ", "
                        << "Diff is " << diff
                        << std::endl;
                }

                if( diff <= 0 || diff >= area1 )
                {
                    std::cout << "Error 21." << std::endl;
                    exit(-1);
                }
                area1 -= diff;
                area2 += diff;
                if( area1 <= 0 || area1+area2 != size )
                {
                    std::cout << "Error 22." << std::endl;
                    exit(-1);
                }

                m_cellToBins[ndi->getId()] = std::make_pair(srcPtr,snkPtr);
                m_areaToBins[ndi->getId()] = std::make_pair(area1,area2);
                it = std::find( m_cellsInBin[srcId].begin(), m_cellsInBin[srcId].end(), ndi );
                if( m_cellsInBin[srcId].end() == it )
                {
                    std::cout << "Error 23." << std::endl;
                    exit(-1);
                }
                it = std::find( m_cellsInBin[snkId].begin(), m_cellsInBin[snkId].end(), ndi );
                if( m_cellsInBin[snkId].end() == it )
                {
                    std::cout << "Error 24." << std::endl;
                    exit(-1);
                }

                // Update occupancies.
                srcPtr->subOcc( (double)diff );
                snkPtr->addOcc( (double)diff );

                // If a fractional cell has still only moved fractionally, then we do not 
                // consider it as moved.
                //++m_moveCellCounter[ndi->getId()];

                // Update flow.
                flow += diff;
            }
        }
        else
        {
            // Not fractional.  Could become fractional if the last node.

            if( flow + size <= demand || avoidFractional )
            {
                // Entire cell.

                if( verbosity > 0 ) 
                {
                    std::cout << "Moving entire cell entirely, "
                        << "Size is " << size
                        << std::endl;
                }

                // Fractional stuff.
                m_cellToBins[ndi->getId()] = std::make_pair(snkPtr,(FlowLegalizerNode*)0);
                m_areaToBins[ndi->getId()] = std::make_pair(size,0);

                // Remove from source bin.
                it = std::find( m_cellsInBin[srcId].begin(), m_cellsInBin[srcId].end(), ndi );
                if( m_cellsInBin[srcId].end() == it )
                {
                    std::cout << "Error 25." << std::endl;
                    exit(-1);
                }
                m_cellsInBin[srcId].erase( it );

                // Add to destination bin (not fractional so it should not be there).
                it = std::find( m_cellsInBin[snkId].begin(), m_cellsInBin[snkId].end(), ndi );
                if( m_cellsInBin[snkId].end() != it )
                {
                    std::cout << "Error 26." << std::endl;
                    exit(-1);
                }
                m_cellsInBin[snkId].push_back( ndi );

                // Update occupancies.
                srcPtr->subOcc( (double)size );
                snkPtr->addOcc( (double)size );

                // We have moved an entire cell so we consider it as being moved.
                ++m_moveCellCounter[ndi->getId()];

                // Update flow.
                flow += size;
            }
            else
            {
                // Partial cell.  This should only happen if we are at the last cell.

                if( avoidFractional )
                {
                    std::cout << "Error 27." << std::endl;
                    exit(-1);
                }


                if( i != nodes.size()-1 )
                {
                    std::cout << "Error 28." << std::endl;
                    exit(-1);
                }

                int area2 = demand-flow;
                int area1 = size-area2;

                if( verbosity > 0 ) 
                {
                    std::cout << "Moving entire cell fractionally, "
                        << "Size is " << size << ", Area1 is " << area1 << ", Area2 is " << area2 
                        << std::endl;
                }

                if( area1 <= 0 || area1+area2 != size )
                {
                    std::cout << "Error 29." << std::endl;
                    exit(-1);
                }

                // Make fractional.
                m_cellToBins[ndi->getId()] = std::make_pair(srcPtr,snkPtr);
                m_areaToBins[ndi->getId()] = std::make_pair(area1,area2);

                // Remove from source bin (actually, it should remain here, but debug).
                it = std::find( m_cellsInBin[srcId].begin(), m_cellsInBin[srcId].end(), ndi );
                if( m_cellsInBin[srcId].end() == it )
                {
                    std::cout << "Error 30." << std::endl;
                    exit(-1);
                }

                // Add to destination bin.
                it = std::find( m_cellsInBin[snkId].begin(), m_cellsInBin[snkId].end(), ndi );
                if( m_cellsInBin[snkId].end() != it )
                {
                    std::cout << "Error 31." << std::endl;
                    exit(-1);
                }
                m_cellsInBin[snkId].push_back( ndi );

                // Update occupancies.
                srcPtr->subOcc( (double)area2 );
                snkPtr->addOcc( (double)area2 );

                // We have moved an cell fractionally so it is not considered as moved.
                //++m_moveCellCounter[ndi->getId()];

                // Update flow.
                flow += area2;
            }
        }
    }

    if( verbosity > 0 )
    {
        std::cout << "Done moving cells: "
            << "SRC: " << srcPtr->getCenterX() << "," << srcPtr->getCenterY() << " -> "
            << "SNK: " << snkPtr->getCenterX() << "," << snkPtr->getCenterY() << ", "
            << "SRC: Cap is " << srcPtr->getCap() << ", Occ is " << srcPtr->getOcc() << ", "
            << "SNK: Cap is " << snkPtr->getCap() << ", Occ is " << snkPtr->getOcc() << ", "
            << "Candidates is " << nodes.size() << ", "
            << "Demand is " << demand 
            << std::endl;
    }

    return flow;
}

class MoveRecord
{
public:
    MoveRecord( void ):
        m_ndi( 0 ),
        m_bin( 0 ),
        m_dist( 0 )
    {
        m_swaps.clear();
    }
    MoveRecord( Node* ndi, FlowLegalizerNode* bin, double dist ):
        m_ndi( ndi ),
        m_bin( bin ),
        m_dist( dist )
    {
        m_swaps.clear();
    }
    MoveRecord( const MoveRecord& other ):
        m_ndi( other.m_ndi ),
        m_bin( other.m_bin ),
        m_dist( other.m_dist )
    {
        m_swaps = other.m_swaps;
    }
    MoveRecord& operator=( const MoveRecord& other )
    {
        if( this != &other )
        {
            m_dist = other.m_dist;
            m_ndi = other.m_ndi;
            m_bin = other.m_bin;
            m_swaps = other.m_swaps;
        }
        return *this;
    }
public:
    double                  m_dist;
    Node*                   m_ndi;
    FlowLegalizerNode*      m_bin;
    std::vector<Node*>      m_swaps;
};

struct compareMoveRecords
{
    inline bool operator() ( const MoveRecord* p, const MoveRecord* q ) const
    {
        if( p->m_dist == q->m_dist )
        {
            Node* ndp = p->m_ndi;
            Node* ndq = q->m_ndi;
            if( ndp->getWidth() == ndq->getWidth() )
            {
                return ndp->getId() < ndq->getId();
            }
            return ndp->getWidth() < ndq->getWidth();
        }
        return p->m_dist < q->m_dist;
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizer::removeBinOverlap( void )
{
    std::cout << "Removing cell overlap within bins." << std::endl;

    std::vector<double> tarr;
    std::vector<double> treq;
    std::vector<double> wid;
    int binId, segId, rowId;
    double rowY, left, right, space;
    double occ, gap, x, totCellWidth;

    for( size_t s = 0; s < this->m_bins.size(); s++ )
    {
        FlowLegalizerNode* binPtr = this->m_bins[s];
        binId = binPtr->m_binId;

        if( m_cellsInBin[binId].size() == 0 )
        {
            continue;
        }

        segId = binPtr->m_segId;
        rowId = m_mgr->m_segments[segId]->m_rowId;

        rowY = m_arch->m_rows[rowId]->getY() + 0.5 * m_arch->m_rows[rowId]->getH();

        // TODO: Site align the boundaries.
        right = binPtr->getMaxX();
        left = binPtr->getMinX();
        space = right - left;

        std::vector<Node*>& nodes = m_cellsInBin[binId];
        std::stable_sort( nodes.begin(), nodes.end(),  DetailedMgr::compareNodesX() );


        treq.resize( nodes.size() );
        tarr.resize( nodes.size() );
        std::fill( treq.begin(), treq.end(), right );
        std::fill( tarr.begin(), tarr.end(), left );
        wid.resize( nodes.size() );
        std::fill( wid.begin(), wid.end(), 0.0 );

        // Get widths and total width.
        occ = 0.;
        for( size_t i = 0; i < nodes.size(); i++ )
        {
            wid[i] = nodes[i]->getWidth() ;
            occ += nodes[i]->getWidth() ;
        }

        // Increase cell widths to account for gaps.
        for( size_t i = 1; i < nodes.size(); i++ )
        {
            double gap = m_arch->getCellSpacing( nodes[i-1], nodes[i] );
            if( gap > 1.0e-3 )
            {
                if( occ + gap <= space+1.0e-3 )
                {
                    wid[i-1]    += gap;
                    occ         += gap;
                }
            }
        }

        // If too much occupancy, we need to scale down the widths.  This will
        // result in overlap, but nothing else to do...
        if( occ >= space-1.0e-3 )
        {
            double scale = space / occ;
            occ = 0;
            for( size_t i = 0; i < nodes.size(); i++ )
            {
                wid[i] = std::floor( wid[i] * scale + 0.5 );
                occ += wid[i];
            }
        }

        // The position of the left edge of each cell.
        x = left;
        for( size_t i = 0; i < nodes.size(); i++ )
        {
            tarr[i] = x;
            x += wid[i];
        }
        x = right;
        for( size_t i = nodes.size(); i > 0; )
        {
            --i;
            x -= wid[i];
            treq[i]= x;
        }
        for( size_t i = 0; i < nodes.size(); i++ )
        {
            Node* ndi = nodes[i];
            x = std::max( tarr[i], std::min( treq[i], ndi->getX() - 0.5 * ndi->getWidth() ) );
            ndi->setX( x + 0.5 * ndi->getWidth() );
            ndi->setY( rowY );
        }

        // XXX: For debugging.  Check the overlap in the bin; I don't think there
        // should be any unless the bin is overfilled.
        int err_1 = 0;
        int err_2 = 0;
        for( size_t i = 0; i < nodes.size(); i++ )
        {
            Node* ndi = nodes[i];
            if( ndi->getX() - 0.5 * ndi->getWidth() <= left-1.0e-3 || ndi->getX() + 0.5 * ndi->getWidth() >= right+1.0e-3 )
            {
                ++err_1;
            }
        }
        for( size_t i = 0; i < nodes.size()-1; i++ )
        {
            Node* ndi = nodes[i];
            Node* ndj = nodes[i+1];
            if( ndi->getX() + 0.5*ndi->getWidth() > ndj->getX()-0.5*ndj->getWidth() )
            {
                ++err_2;
            }
        }
    }

    // Since we have changed node positions, we should resort the segments.
    m_mgr->resortSegments();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void FlowLegalizerNode::deleteEdges( void )
{
    for( size_t i = 0; i < m_outEdges.size(); i++ )
    {
        delete m_outEdges[i];
    }
    m_outEdges.clear();
}


}
