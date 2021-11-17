///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.



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
#include <set>
#include <utility>
#include <cmath>
#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
#include "utility.h"
#include "plotgnu.h"
#include "detailed_segment.h"
#include "detailed_manager.h"
#include "detailed_orient.h"



namespace dpo
{

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedMgr::DetailedMgr( Architecture* arch, Network* network, RoutingParams* rt ):
    m_arch( arch ), m_network( network ), m_rt( rt ), m_rng( 0 )
{
    m_singleRowHeight = m_arch->m_rows[0]->m_rowHeight;
    m_numSingleHeightRows = m_arch->m_rows.size();

    m_rng = new Placer_RNG;
    m_rng->seed( static_cast<unsigned>(10) );

    // Utilization...
    m_targetUt = 1.0;

    // For generating a move list...
    m_moveLimit = 10;
    m_nMoved = 0;
    m_curX.resize( m_moveLimit );
    m_curY.resize( m_moveLimit );
    m_newX.resize( m_moveLimit );
    m_newY.resize( m_moveLimit );
    m_curOri.resize( m_moveLimit );
    m_newOri.resize( m_moveLimit );
    m_curSeg.resize( m_moveLimit );
    m_newSeg.resize( m_moveLimit );
    m_movedNodes.resize( m_moveLimit );
    for( size_t i = 0; i < m_moveLimit; i++ )
    {
        m_curSeg[i] = std::vector<int>();
        m_newSeg[i] = std::vector<int>();
    }

    // The purpose of this reverse map is to be able to remove the cell from
    // all segments that it has been placed into.  It only works (i.e., is 
    // only up-to-date) if you use the proper routines to add and remove cells
    // to and from segments.
    m_reverseCellToSegs.resize( m_network->m_nodes.size() );
    for( size_t i = 0; i < m_reverseCellToSegs.size(); i++ )
    {
        m_reverseCellToSegs[i] = std::vector<DetailedSeg*>();
    }

    recordOriginalPositions();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedMgr::~DetailedMgr( void )
{
    m_blockages.clear();

    m_segsInRow.clear();

    for( int i = 0; i < m_segments.size(); i++ ) 
    {
        delete m_segments[i];
    }
    m_segments.clear();

    if( m_rng != 0 )
    {
        delete m_rng;
    }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::findBlockages( std::vector<Node*>& fixedCells, bool includeRouteBlockages )
{
    // Blockages come from filler, from fixed nodes (possibly with shapes) and 
    // from larger macros which are now considered fixed...


    m_blockages.clear();

    // Determine the single height segments and blockages.
    m_blockages.resize( m_numSingleHeightRows );
    for( int i = 0; i < m_blockages.size(); i++ )
    {
        m_blockages[i] = std::vector<std::pair<double,double> >();
    }


    for( int i = 0; i < fixedCells.size(); i++ ) 
    {
        Node* nd = fixedCells[i];

        double xmin = std::max( m_arch->m_xmin, nd->getX() - 0.5 * nd->getWidth() );
        double xmax = std::min( m_arch->m_xmax, nd->getX() + 0.5 * nd->getWidth() );
        double ymin = std::max( m_arch->m_ymin, nd->getY() - 0.5 * nd->getHeight() );
        double ymax = std::min( m_arch->m_ymax, nd->getY() + 0.5 * nd->getHeight() );

        // HACK!  So a fixed cell might split a row into multiple
        // segments.  However, I don't take into account the
        // spacing or padding requirements of this cell!  This 
        // means I could get an error later on.
        //
        // I don't think this is guaranteed to fix the problem, 
        // but I suppose I can grab spacing/padding between this
        // cell and "no other cell" on either the left or the
        // right.  This might solve the problem since it will
        // make the blockage wider.
        xmin -= m_arch->getCellSpacing( 0, nd );
        xmax += m_arch->getCellSpacing( nd, 0 );

        for( int r = 0; r < m_numSingleHeightRows; r++ )
        {
            double lb = m_arch->m_ymin + r * m_singleRowHeight;
            double ub = lb + m_singleRowHeight;

            if( !(ymax-1.0e-3 <= lb || ymin+1.0e-3 >= ub) )
            {
                m_blockages[r].push_back( std::pair<double,double>(xmin,xmax) );
            }
        }
    }


    if( includeRouteBlockages )
    {
        if( m_rt != 0 )
        {
            // Turn M1 and M2 routing blockages into placement blockages.  The idea here is to
            // be quite conservative and prevent the possibility of pin access problems.  We
            // *ONLY* consider routing obstacles to be placement obstacles if they overlap with
            // an *ENTIRE* site.

            for( int layer = 0; layer <= 1 && layer < m_rt->m_num_layers; layer++ )
            {
                std::vector<Rectangle>& rects = m_rt->m_layerBlockages[layer];
                for( int b = 0; b  < rects.size(); b++ )
                {
                    double xmin = rects[b].m_xmin;
                    double xmax = rects[b].m_xmax;
                    double ymin = rects[b].m_ymin;
                    double ymax = rects[b].m_ymax;

                    for( int r = 0; r < m_numSingleHeightRows; r++ )
                    {
                        double lb = m_arch->m_ymin + r * m_singleRowHeight;
                        double ub = lb + m_singleRowHeight;

                        if( ymax >= ub && ymin <= lb )
                        {
                            // Blockage overlaps with the entire row span in the Y-dir... Sites
                            // are possibly completely covered!  

                            double originX = m_arch->m_rows[r]->m_subRowOrigin;
                            double siteSpacing = m_arch->m_rows[r]->m_siteSpacing;

                            int i0 = (int)std::floor((xmin-originX)/siteSpacing);
                            int i1 = (int)std::floor((xmax-originX)/siteSpacing);
                            if( originX + i1 * siteSpacing != xmax )
                                ++i1;

                            if( i1 > i0 )
                            {
                                m_blockages[r].push_back( std::pair<double,double>(originX + i0 * siteSpacing, originX + i1 * siteSpacing) );

                            }
                        }
                    }
                }
            }
        }
    }


    // Sort blockages and merge.
    for( int r = 0; r < m_numSingleHeightRows; r++ )
    {
        if( m_blockages[r].size() == 0 )
        {
            continue;
        }

        std::sort( m_blockages[r].begin(), m_blockages[r].end(), compareBlockages() );

        std::stack< std::pair<double,double> > s;
        s.push( m_blockages[r][0] );
        for( int i = 1; i < m_blockages[r].size(); i++ )
        {
            std::pair<double,double> top = s.top(); // copy.
            if( top.second < m_blockages[r][i].first )
            {
                s.push( m_blockages[r][i] ); // new interval.
            }
            else
            {
                if( top.second < m_blockages[r][i].second )
                {
                    top.second = m_blockages[r][i].second; // extend interval.
                }
                s.pop(); // remove old.
                s.push( top ); // expanded interval.
            }
        }

        m_blockages[r].erase( m_blockages[r].begin(), m_blockages[r].end() );
        while( !s.empty() )
        {
            std::pair<double,double> temp = s.top(); // copy.
            m_blockages[r].push_back( temp );
            s.pop();
        }

        // Intervals need to be sorted, but they are currently in reverse order.  Can either
        // resort or reverse.
        std::sort( m_blockages[r].begin(), m_blockages[r].end(), compareBlockages() ); // Sort to get them left to right.
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::findSegments( void )
{
    // Create the segments into which movable cells are placed.  I do make
    // segment ends line up with sites and that segments don't extend off
    // the chip.

    for( int i = 0; i < m_segments.size(); i++ ) 
    {
        delete m_segments[i];
    }
    m_segments.erase( m_segments.begin(), m_segments.end() );

    int numSegments = 0;
    double x1, x2;
    m_segsInRow.resize( m_numSingleHeightRows );
    for( int r = 0; r < m_numSingleHeightRows; r++ )
    {
        double lx = m_arch->m_rows[r]->m_subRowOrigin;
        double rx = m_arch->m_rows[r]->m_subRowOrigin + m_arch->m_rows[r]->m_numSites * m_arch->m_rows[r]->m_siteSpacing;

        m_segsInRow[r] = std::vector<DetailedSeg*>();

        int n = m_blockages[r].size();
        if( n == 0 )
        {
            // Entire row free.

            x1 = std::max( m_arch->m_xmin, lx );
            x2 = std::min( m_arch->m_xmax, rx );

            if( x2 > x1 ) 
            {
                DetailedSeg* segment = new DetailedSeg();
                segment->m_segId = numSegments;
                segment->m_rowId = r;
                segment->m_xmin = m_arch->m_xmin;
                segment->m_xmax = m_arch->m_xmax;

                m_segsInRow[r].push_back( segment );
                m_segments.push_back( segment );

                ++numSegments;
            }
        }
        else
        {
            // Divide row.
            if( m_blockages[r][0].first > std::max( m_arch->m_xmin, lx ) )
            {
                x1 = std::max( m_arch->m_xmin, lx );
                x2 = std::min( std::min( m_arch->m_xmax, rx ), m_blockages[r][0].first );

                if( x2 > x1 )
                {
                    DetailedSeg* segment = new DetailedSeg();
                    segment->m_segId = numSegments;
                    segment->m_rowId = r;
                    segment->m_xmin = x1;
                    segment->m_xmax = x2;

                    m_segsInRow[r].push_back( segment );
                    m_segments.push_back( segment );

                    ++numSegments;
                }
            }
            for( int i = 1; i < n; i++ ) 
            {
                if( m_blockages[r][i].first > m_blockages[r][i-1].second )
                {
                    x1 = std::max( std::max( m_arch->m_xmin, lx ), m_blockages[r][i-1].second );
                    x2 = std::min( std::min( m_arch->m_xmax, rx ), m_blockages[r][i-0].first  );

                    if( x2 > x1 )
                    {
                        DetailedSeg* segment = new DetailedSeg();
                        segment->m_segId = numSegments;
                        segment->m_rowId = r;
                        segment->m_xmin = x1;
                        segment->m_xmax = x2;

                        m_segsInRow[r].push_back( segment );
                        m_segments.push_back( segment );

                        ++numSegments;
                    }
                }
            }
            if( m_blockages[r][n-1].second < std::min( m_arch->m_xmax, rx ) )
            {
                x1 = std::min( std::min( m_arch->m_xmax, rx ), std::max( std::max( m_arch->m_xmin, lx ), m_blockages[r][n-1].second ) );
                x2 = std::min( m_arch->m_xmax, rx );

                if( x2 > x1 )
                {
                    DetailedSeg* segment = new DetailedSeg();
                    segment->m_segId = numSegments;
                    segment->m_rowId = r;
                    segment->m_xmin = x1;
                    segment->m_xmax = x2;

                    m_segsInRow[r].push_back( segment );
                    m_segments.push_back( segment );

                    ++numSegments;
                }
            }
        }
    }

    // Here, we need to slice up the segments to account for regions.
    std::vector<std::vector<std::pair<double,double> > > intervals;
    for( size_t reg = 1; reg < m_arch->m_regions.size(); reg++ )
    {
        Architecture::Region* regPtr = m_arch->m_regions[reg];

        //std::cout << "Finding intervals spanned by region " << regPtr->m_id << " of " << m_arch->m_regions.size() << std::endl;

        findRegionIntervals( regPtr->m_id, intervals );

        int split = 0;

        // Now we need to "cutup" the existing segments.  How best to
        // do this???????????????????????????????????????????????????
        // Perhaps I can use the same ideas as for blockages...
        for( size_t r = 0; r < m_numSingleHeightRows; r++ )
        {
            int n = intervals[r].size();
            if( n == 0 )
            {
                continue;
            }

            // Since the intervals do not overlap, I think the following is fine:
            // Pick an interval and pick a segment.  If the interval and segment
            // do not overlap, do nothing.  If the segment and the interval do
            // overlap, then there are cases.  Let <sl,sr> be the span of the
            // segment.  Let <il,ir> be the span of the interval.  Then:
            //
            // Case 1: il <= sl && ir >= sr: The interval entirely overlaps the
            //         segment.  So, we can simply change the segment's region
            //         type.
            // Case 2: il  > sl && ir >= sr: The segment needs to be split into
            //         two segments.  The left segment remains retains it's 
            //         original type while the right segment is new and assigned
            //         to the region type.
            // Case 3: il <= sl && ir  < sr: Switch the meaning of left and right
            //         per case 2.
            // Case 4: il  > sl && ir  < sr: The original segment needs to be 
            //         split into 2 with the original region type.  A new segment
            //         needs to be created with the new region type.

            for( size_t i = 0; i < intervals[r].size(); i++ )
            {
                double il = intervals[r][i].first ;
                double ir = intervals[r][i].second;
                for( size_t s = 0; s < m_segsInRow[r].size(); s++ )
                {
                    DetailedSeg* segPtr = m_segsInRow[r][s];

                    double sl = segPtr->m_xmin;
                    double sr = segPtr->m_xmax;

                    // Check for no overlap.
                    if( ir <= sl ) continue;
                    if( il >= sr ) continue;

                    // Case 1:       
                    if( il <= sl && ir >= sr )
                    {
                        segPtr->m_regId = reg;
                    }
                    // Case 2:
                    else if( il > sl && ir >= sr )
                    {
                        ++split;

                        segPtr->m_xmax = il;

                        DetailedSeg* newPtr = new DetailedSeg();
                        newPtr->m_segId = numSegments;
                        newPtr->m_rowId = r;
                        newPtr->m_regId = reg;
                        newPtr->m_xmin = il;
                        newPtr->m_xmax = sr;

                        m_segsInRow[r].push_back( newPtr );
                        m_segments.push_back( newPtr );

                        ++numSegments;
                    }
                    // Case 3:
                    else if( ir < sr && il <= sl )
                    {
                        ++split;

                        segPtr->m_xmin = ir;

                        DetailedSeg* newPtr = new DetailedSeg();
                        newPtr->m_segId = numSegments;
                        newPtr->m_rowId = r;
                        newPtr->m_regId = reg;
                        newPtr->m_xmin = sl;
                        newPtr->m_xmax = ir;

                        m_segsInRow[r].push_back( newPtr );
                        m_segments.push_back( newPtr );

                        ++numSegments;
                    }
                    // Case 4:
                    else if( il > sl && ir < sr )
                    {
                        ++split;
                        ++split;

                        segPtr->m_xmax = il;

                        DetailedSeg* newPtr = new DetailedSeg();
                        newPtr->m_segId = numSegments;
                        newPtr->m_rowId = r;
                        newPtr->m_regId = reg;
                        newPtr->m_xmin = il;
                        newPtr->m_xmax = ir;

                        m_segsInRow[r].push_back( newPtr );
                        m_segments.push_back( newPtr );

                        ++numSegments;

                        newPtr = new DetailedSeg();
                        newPtr->m_segId = numSegments;
                        newPtr->m_rowId = r;
                        newPtr->m_regId = segPtr->m_regId;
                        newPtr->m_xmin = ir;
                        newPtr->m_xmax = sr;

                        m_segsInRow[r].push_back( newPtr );
                        m_segments.push_back( newPtr );

                        ++numSegments;
                    }
                    else
                    {
                        std::cout << "Error." << std::endl;
                        exit(-1);
                    }
                }
            }
        }

        //if( split != 0 )
        //{
        //    std::cout << "Region " << reg << " resulted in spliting segments; "
        //        << "Created " << split << " new segments." << std::endl;
        //}
    }

    // Make sure segment boundaries line up with sites.
    for( int s = 0; s < m_segments.size(); s++ )
    {
        int rowId = m_segments[s]->m_rowId;

        double originX     = m_arch->m_rows[rowId]->m_subRowOrigin;
        double siteSpacing = m_arch->m_rows[rowId]->m_siteSpacing;

        int ix;
        
        ix = (int)((m_segments[s]->m_xmin - originX)/siteSpacing);
        if( originX + ix * siteSpacing  < m_segments[s]->m_xmin ) 
            ++ix;

        if( originX + ix * siteSpacing != m_segments[s]->m_xmin )
            m_segments[s]->m_xmin = originX + ix * siteSpacing;

        ix = (int)((m_segments[s]->m_xmax - originX)/siteSpacing);
        if( originX + ix * siteSpacing != m_segments[s]->m_xmax )
            m_segments[s]->m_xmax = originX + ix * siteSpacing;
    }

    // Create the structure for cells in segments.
    m_cellsInSeg.clear();
    m_cellsInSeg.resize( m_segments.size() );
    for( size_t i = 0; i < m_cellsInSeg.size(); i++ )
    {   
        m_cellsInSeg[i] = std::vector<Node*>();
    }

    std::cout << "Created " << m_segments.size() << " segments." << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedSeg* DetailedMgr::findClosestSegment( Node* nd )
{
    // Find the closest segment for the node.  First, we consider those segments which lie in the row
    // closest to the cell. Then, we consider segments in other rows which are both above and below 
    // the closest row.  Note: We skip segments that are not large enough to hold the current node - 
    // assigning a node to a segment which is not wide enough is most certainly going to cause problems
    // later (we will need to re-locate the node), so try to prevent this node.  Further, these sorts
    // of problems typically happen with wide cells which are difficult (a hassle) to move around since
    // they are so large...

    int row = (nd->getY() - m_arch->m_ymin) / m_singleRowHeight;

    double hori;
    double vert;
    double dist1 = std::numeric_limits<double>::max();
    double dist2 = std::numeric_limits<double>::max();
    DetailedSeg* best1 = 0;     // closest segment...
    DetailedSeg* best2 = 0;     // closest segment which is wide enough to accomodate the cell...

    // Segments in the current row...
    for( int k = 0; k < m_segsInRow[row].size(); k++ )
    {
        DetailedSeg* curr = m_segsInRow[row][k];

        // Updated for regions.
        if( nd->getRegionId() != curr->m_regId )
        {
            continue;
        }


        double x1 = curr->m_xmin + 0.5 * nd->getWidth();
        double x2 = curr->m_xmax - 0.5 * nd->getWidth();
        double xx = std::max( x1, std::min( x2, nd->getX() ) );

        hori = std::max( 0.0, std::fabs( xx - nd->getX() ) );
        vert = 0.0;

        bool closer1 = (hori+vert < dist1) ? true : false;
        bool closer2 = (hori+vert < dist2) ? true : false;
        bool fits = (nd->getWidth() <= (curr->m_xmax - curr->m_xmin)) ? true: false;

        // Keep track of the closest segment.
        if( best1 == 0 || (best1 != 0 && closer1))
        {
            best1 = curr; dist1 = hori+vert;
        }
        // Keep track of the closest segment which is wide enough to accomodate the cell.
        if( fits && (best2 == 0 || (best2 != 0 && closer2)) )
        {
            best2 = curr; dist2 = hori+vert;
        }
    }

    // Consider rows above and below the current row.
    for( int offset = 1; offset <= m_numSingleHeightRows; offset++ )
    {
        int below = row-offset;
        vert = offset * m_singleRowHeight;

        if( below >= 0 )
        {
            // Consider the row if we could improve on either of the best segments we are recording.
            if( (vert <= dist1 || vert <= dist2 ) )
            {
                for( int k = 0; k < m_segsInRow[below].size(); k++ )
                {
                    DetailedSeg* curr = m_segsInRow[below][k];

                    // Updated for regions.
                    if( nd->getRegionId() != curr->m_regId )
                    {
                        continue;
                    }

                    double x1 = curr->m_xmin + 0.5 * nd->getWidth();
                    double x2 = curr->m_xmax - 0.5 * nd->getWidth();
                    double xx = std::max( x1, std::min( x2, nd->getX() ) );

                    hori = std::max( 0.0, std::fabs( xx - nd->getX() ) );

                    bool closer1 = (hori+vert < dist1) ? true : false;
                    bool closer2 = (hori+vert < dist2) ? true : false;
                    bool fits = (nd->getWidth() <= (curr->m_xmax - curr->m_xmin)) ? true: false;

                    // Keep track of the closest segment.
                    if( best1 == 0 || (best1 != 0 && closer1))
                    {
                        best1 = curr; dist1 = hori+vert;
                    }
                    // Keep track of the closest segment which is wide enough to accomodate the cell.
                    if( fits && (best2 == 0 || (best2 != 0 && closer2)) )
                    {
                        best2 = curr; dist2 = hori+vert;
                    }
                }
            }
        }

        int above = row+offset;
        vert = offset * m_singleRowHeight;

        if( above <= m_numSingleHeightRows-1 )
        {
            // Consider the row if we could improve on either of the best segments we are recording.
            if( (vert <= dist1 || vert <= dist2 ) )
            {
                for( int k = 0; k < m_segsInRow[above].size(); k++ )
                {
                    DetailedSeg* curr = m_segsInRow[above][k];

                    // Updated for regions.
                    if( nd->getRegionId() != curr->m_regId )
                    {
                        continue;
                    }

                    double x1 = curr->m_xmin + 0.5 * nd->getWidth();
                    double x2 = curr->m_xmax - 0.5 * nd->getWidth();
                    double xx = std::max( x1, std::min( x2, nd->getX() ) );

                    hori = std::max( 0.0, std::fabs( xx - nd->getX() ) );

                    bool closer1 = (hori+vert < dist1) ? true : false;
                    bool closer2 = (hori+vert < dist2) ? true : false;
                    bool fits = (nd->getWidth() <= (curr->m_xmax - curr->m_xmin)) ? true: false;

                    // Keep track of the closest segment.
                    if( best1 == 0 || (best1 != 0 && closer1))
                    {
                        best1 = curr; dist1 = hori+vert;
                    }
                    // Keep track of the closest segment which is wide enough to accomodate the cell.
                    if( fits && (best2 == 0 || (best2 != 0 && closer2)) )
                    {
                        best2 = curr; dist2 = hori+vert;
                    }
                }
            }
        }
    }

    return (best2) ? best2 : best1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::findClosestSpanOfSegmentsDfs(
    Node* ndi,
    DetailedSeg* segPtr, 
    double xmin,
    double xmax,
    int bot, int top,
    std::vector<DetailedSeg*>& stack,
    std::vector<std::vector<DetailedSeg*> >& candidates

    )
{
    std::set<int> debug_set;
    //debug_set.insert( 96609 );


    stack.push_back( segPtr );
    int segId = segPtr->m_segId;
    int rowId = segPtr->m_rowId;

    if( debug_set.end() != debug_set.find( ndi->getId() ) )
    {
        for( size_t i = 0; i <= stack.size(); i++ ) std::cout << " ";
        std::cout << "seg: " << segId << " @ row: " << rowId << " L: " << xmin << " R: " << xmax
            << " X[" << segPtr->m_xmin << "," << segPtr->m_xmax << "]" << std::endl;
    }

    if( rowId < top )
    {
        ++rowId;
        for( size_t s = 0; s < m_segsInRow[rowId].size(); s++ )
        {
            segPtr = m_segsInRow[rowId][s];
            double overlap = std::min( xmax, segPtr->m_xmax ) - std::max( xmin, segPtr->m_xmin );

            if( debug_set.end() != debug_set.find( ndi->getId() ) )
            {
                for( size_t i = 0; i <= stack.size(); i++ ) std::cout << " ";
                std::cout << "up: " << segId << " @ row: " << rowId << " L: " << xmin << " R: " << xmax
                    << " X[" << segPtr->m_xmin << "," << segPtr->m_xmax << "], overlap is " << overlap << std::endl;
            }
            if( overlap >= 1.0e-3 )
            {
                // Must find the reduced X-interval.
                double xl = std::max( xmin, segPtr->m_xmin );
                double xr = std::min( xmax, segPtr->m_xmax );
                findClosestSpanOfSegmentsDfs( ndi, segPtr, xl, xr, bot, top, stack, candidates );
            }
        }
    }
    else
    {
        // Reaching this point should imply that we have a consecutive set of 
        // segments which is potentially valid for placing the cell.
        int spanned = top-bot+1;
        if( stack.size() != spanned )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
        candidates.push_back( stack );
    }
    stack.pop_back();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::findClosestSpanOfSegments( Node* nd, std::vector<DetailedSeg*>& segments )
{
    // Intended for multi-height cells...  Finds the number of rows the cell
    // spans and then attempts to find a vector of segments (in different
    // rows) into which the cell can be assigned.

    int spanned = (int)((nd->getHeight()/m_singleRowHeight) + 0.5);
    if( spanned <= 1 )
    {
        // Not intended for single height cells so error out.
        std::cout << "Error." << std::endl;
        exit(-1);
    }

    std::set<int> debug;
//    debug.insert( 96609 );

    if( debug.end() != debug.find( nd->getId() ) )
    {
        std::cout << "Attempting to assign cell " << nd->getId() << " @ "
            << "(" << nd->getX() << "," << nd->getY() << ")" << " "
            << "Spanning " << spanned << " rows" << ", "
            << "Bottom at " << nd->getY() - 0.5 * nd->getHeight() 
            << std::endl;
    }

    double hori;
    double vert;
    double disp1 = std::numeric_limits<double>::max();
    double disp2 = std::numeric_limits<double>::max();

    std::vector<std::vector<DetailedSeg*> > candidates;
    std::vector<DetailedSeg*> stack;

    std::vector<DetailedSeg*> best1; // closest.
    std::vector<DetailedSeg*> best2; // closest that fits.

    // The efficiency of this is not good.  The information about overlapping segments
    // for multi-height cells could easily be precomputed for efficiency.
    bool flip = false;
    for( size_t r = 0; r < m_arch->m_rows.size(); r++ )
    {

        // XXX: NEW! Check power compatibility of this cell with the row.  A
        // call to this routine will check both the bottom and the top rows
        // for power compatibility.
        if( !m_arch->power_compatible( nd, m_arch->m_rows[r], flip ) )
        {
            if( debug.end() != debug.find( nd->getId() ) )
            {
                std::cout << " => Cell " << nd->getId() << ", "
                    << "not compatible with row @ " << m_arch->m_rows[r]->getY() 
                    << std::endl;
            }
            continue;
        }

        // Scan the segments in this row and look for segments in the required
        // number of rows above and below that result in non-zero interval.
        int b = r;
        int t = r + spanned - 1;
        if( t >= m_arch->m_rows.size() )
        {
            continue;
        }

        if( debug.end() != debug.find( nd->getId() ) )
        {
            std::cout << " => Cell " << nd->getId() << ", "
                    << "trying in rows " << b << " through " << t << ", "
                    << "Y[" << m_arch->m_rows[b]->getY() << ":" << m_arch->m_rows[t]->getY()+m_arch->m_rows[t]->getH() << "]"
                    << std::endl;
        }

        for( size_t sb = 0; sb < m_segsInRow[b].size(); sb++ )
        {
            DetailedSeg* segPtr = m_segsInRow[b][sb];

            if( debug.end() != debug.find( nd->getId() ) )
            {
                std::cout << "Trying to place cell " << nd->getId() << " which spans " << spanned << " rows; "
                    << "Location is @ (" << nd->getX() << "," << nd->getY() << ")" << ", " 
                    << "Row is " << b << ", Segment is " << sb << " of " << m_segsInRow[b].size() << ", "
                    << "X:[" << segPtr->m_xmin << "," << segPtr->m_xmax << "]"
                    << std::endl;
            }

            candidates.clear();
            stack.clear();

            findClosestSpanOfSegmentsDfs( nd, segPtr, segPtr->m_xmin, segPtr->m_xmax, b, t, stack, candidates );
            if( candidates.size() == 0 )
            {
                if( debug.end() != debug.find( nd->getId() ) )
                {
                    std::cout << "No candidates." << std::endl;
                }
                continue;
            }
            else
            {
                if( debug.end() != debug.find( nd->getId() ) )
                {
                    std::cout << "Candidates is " << candidates.size() << std::endl;
                }
            }


            // Evaluate the candidate segments.  Determine the distance of the bottom of the
            // node to the bottom of the first segment.  Determine the overlap in the 
            // interval in the X-direction and determine the required distance.

            for( size_t i = 0; i < candidates.size(); i++ )
            {
                // NEW: All of the segments must have the same region ID and that region ID
                // must be the same as the region ID of the cell.  If not, then we are going
                // to violate a fence region constraint.
                bool regionsOkay = true;
                for( size_t j = 0; j < candidates[i].size(); j++ )
                {
                    DetailedSeg* segPtr = candidates[i][j];
                    if( segPtr->m_regId != nd->getRegionId() )
                    {
                        regionsOkay = false;
                    }
                }

                // XXX: Should region constraints be hard or soft?  If hard, there is more
                // change for failure!
                if( !regionsOkay )
                {
                    //std::cout << "Region is not okay." << std::endl;
                    continue;
                }

                DetailedSeg* segPtr = candidates[i][0];
                double ymin = m_arch->m_rows[segPtr->m_rowId]->getY();
                double xmin = segPtr->m_xmin;
                double xmax = segPtr->m_xmax;
                for( size_t j = 0; j < candidates[i].size(); j++ )
                {
                    segPtr = candidates[i][j];
                    int rr = segPtr->m_rowId;
                    xmin = std::max( xmin, segPtr->m_xmin );
                    xmax = std::min( xmax, segPtr->m_xmax );

                    if( debug.end() != debug.find( nd->getId() ) )
                    {
                        std::cout << "Span segment " << segPtr->m_segId << ", "
                            << "X:" << segPtr->m_xmin << "," << segPtr->m_xmax << "] "
                            << "@ " << m_arch->m_rows[rr]->getY() << std::endl;
                    }
                }

                double dy = std::fabs( nd->getY() - 0.5 * nd->getHeight() - ymin );
                double ww = std::min( nd->getWidth(), xmax-xmin );
                double xx = std::max( xmin+0.5*ww, std::min( xmax-0.5*ww, nd->getX() ) );
                double dx = std::fabs( nd->getX() - xx );

                double disp = dx + dy;

                if( debug.end() != debug.find( nd->getId() ) )
                {
                    std::cout << "Candidate " << i << ", "
                        << "Xmin is " << xmin << ", Xmax is " << xmax << ", "
                        << "Ymin is " << ymin << ", DY is " << dy << ", DX is " << dx << std::endl;
                }

                if( best1.size() == 0 || (dx + dy < disp1) )
                {
                    if( 1 )
                    {
                        best1 = candidates[i];
                        disp1 = dx + dy;
                    }
                }
                if( best2.size() == 0 || (dx + dy < disp2) )
                {
                    if( nd->getWidth() <= xmax-xmin+1.0e-3 )
                    {
                        best2 = candidates[i];
                        disp2 = dx + dy;
                    }
                }

                //std::cout << "--" << std::endl;
                //for( size_t j = 0; j < candidates[i].size(); j++ )
                //{
                //    segPtr = candidates[i][j];
                //    std::cout << "[" << segPtr->m_segId << "," << segPtr->m_rowId << ","
                //        << segPtr->m_xmin << "," << segPtr->m_xmax << "]";
                //}
                //std::cout << "\n--" << std::endl;

            }
        }
    }

    segments.erase( segments.begin(), segments.end() );
    if( best2.size() != 0 )
    {
        /*
        std::cout << "Closest span of segments (fits):" << std::endl;
        for( size_t j = 0; j < best2.size(); j++ )
        {
            DetailedSeg* segPtr = best2[j];
            int rowId = segPtr->m_rowId;
            int segId = segPtr->m_segId;
            std::cout << "[" << segId << "," << rowId << "," << segPtr->m_xmin << "," << segPtr->m_xmax << "@" 
                << m_arch->m_rows[rowId]->getY() << "]";
        }
        std::cout << std::endl;
        std::cout << "Displacement is " << disp2 << std::endl;
        */

        segments = best2;
        return true;
    }
    if( best1.size() != 0 )
    {
        /*
        std::cout << "Closest span of segments (might not fit):" << std::endl;
        for( size_t j = 0; j < best1.size(); j++ )
        {
            DetailedSeg* segPtr = best1[j];
            int rowId = segPtr->m_rowId;
            int segId = segPtr->m_segId;
            std::cout << "[" << segId << "," << rowId << "," << segPtr->m_xmin << "," << segPtr->m_xmax << "@" 
                << m_arch->m_rows[rowId]->getY() << "]";
        }
        std::cout << std::endl;
        std::cout << "Displacement is " << disp1 << std::endl;
        */

        segments = best1;
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::assignCellsToSegments( std::vector<Node*>& nodesToConsider )
{
    // For the provided list of cells which are assumed movable, assign those
    // cells to segments.
    //
    // XXX: Multi height cells are assigned to multiple rows!  In other words,
    // a cell can exist in multiple rows.
    //
    // Hmmm...  What sorts of checks should be done when assigning a multi-height
    // cell to multiple rows?  This presumes that there are segments which are
    // on top of each other...  It is possible not this routine that needs to
    // be fixed, but rather the routine which finds the segments...

    // Assign cells to segments.
    int nAssigned = 0;
    double movementX = 0.;
    double movementY = 0.;
    for( int i = 0; i < nodesToConsider.size(); i++ )
    {
        Node* nd = nodesToConsider[i];

        int nRowsSpanned = (int)((nd->getHeight() / m_singleRowHeight) + 0.5);
        if( !(nRowsSpanned > 0 ) )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }

        if( nRowsSpanned == 1 )
        {
            // Single height.
            DetailedSeg* segPtr = findClosestSegment( nd );
            if( segPtr == 0 )
            {
                cout << "Error." << endl;
                exit(-1);
            }

            int rowId = segPtr->m_rowId;
            int segId = segPtr->m_segId;

            // Add to segment.
            addCellToSegment( nd, segId );
            ++nAssigned;

            // Move the cell's position into the segment.  XXX: Do I even need to do
            // this?????????????????????????????????????????????????????????????????
            double x1 = segPtr->m_xmin + 0.5 * nd->getWidth();
            double x2 = segPtr->m_xmax - 0.5 * nd->getWidth();
            double xx = std::max( x1, std::min( x2, nd->getX() ) );
            double yy = m_arch->m_rows[rowId]->getY() + 0.5 * nd->getHeight();
           
            movementX += std::fabs( nd->getX() - xx );
            movementY += std::fabs( nd->getY() - yy );

            nd->setX( xx );
            nd->setY( yy );
        }
        else
        {
            // Multi height.
            std::vector<DetailedSeg*> segments;
            if( !findClosestSpanOfSegments( nd, segments ) )
            {
                std::cout << "Unable to snap multi-height cell " 
                    << m_network->m_nodeNames[nd->getId()].c_str() << "; "
                    << "Could not find a suitable span of segments." << std::endl;
                exit(-1);
            }
            else
            {
                if( segments.size() != nRowsSpanned )
                {
                    std::cout << "Error." << std::endl;
                    exit(-1);
                }
                // NB: adding a cell to a segment does _not_ change its position.
                DetailedSeg* segPtr = segments[0];
                double xmin = segPtr->m_xmin;
                double xmax = segPtr->m_xmax;
                for( size_t s = 0; s < segments.size(); s++ )
                {
                    segPtr = segments[s];
                    xmin = std::max( xmin, segPtr->m_xmin );
                    xmax = std::min( xmax, segPtr->m_xmax );
                    addCellToSegment( nd, segments[s]->m_segId );
                }
                ++nAssigned;

                // Move the cell's position into the segment.  XXX: Do I even need to do
                // this?????????????????????????????????????????????????????????????????
                segPtr = segments[0];
                int rowId = segPtr->m_rowId;

                double x1 = xmin + 0.5 * nd->getWidth();
                double x2 = xmax - 0.5 * nd->getWidth();
                double xx = std::max( x1, std::min( x2, nd->getX() ) );
                double yy = m_arch->m_rows[rowId]->getY() + 0.5 * nd->getHeight();

//                if( std::fabs( nd->getY() - yy ) > 1.0e-3 )
//                {
//                    std::cout << "Snapping multi-height cell " << nd->getId() << " to seg, "
//                        << "change in y position" << ", "
//                        << "(" << nd->getX() << "," << nd->getY() << ")"
//                        << "->"
//                        << "(" << xx << "," << yy << ")"
//                        << std::endl;
//                }

                movementX += std::fabs( nd->getX() - xx );
                movementY += std::fabs( nd->getY() - yy );

                nd->setX( xx );
                nd->setY( yy );
            }
        }
    }
    if( nAssigned != 0 )
    {
        std::cout << "Assigned " << nAssigned << " cells into segments" << ", "
            << "Movement in X is " << movementX << ", "
            << "Movement in Y is " << movementY << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeCellFromSegment( Node* nd, int seg )
{
    // Removing a node from a segment means a few things...  It means: 1) removing it from the cell list for the
    // segment; 2) removing its width from the segment utilization; 3) updating the required gaps between cells
    // in the segment.

    std::vector<Node*>::iterator it = 
        std::find( m_cellsInSeg[seg].begin(), m_cellsInSeg[seg].end(), nd );
    if( m_cellsInSeg[seg].end() == it )
    {
        // This is odd.  Where does the reverse segment map say the cell should be?
        char buf[256];
        sprintf( &buf[0], "Unable to find cell %d in expected segment %d.", nd->getId(), seg );
        ERRMSG( buf );
        exit(-1);
    }
    int ix = it - m_cellsInSeg[seg].begin();
    int n = m_cellsInSeg[seg].size()-1;

    // Remove this segment from the reverse map.
    std::vector<DetailedSeg*>::iterator its = std::find( 
        m_reverseCellToSegs[nd->getId()].begin(), m_reverseCellToSegs[nd->getId()].end(), 
        m_segments[seg] );
    if( m_reverseCellToSegs[nd->getId()].end() == its )
    {
        char buf[256];
        sprintf( &buf[0], "Unable to find segment %d in reverse map for cell %d.", seg, nd->getId() );
        ERRMSG( buf );
        exit(-1);
    }
    m_reverseCellToSegs[nd->getId()].erase( its );
    

    // Determine gaps with the cell in the row and with the cell not in the row.
    Node* prev_cell = ( ix == 0 ) ? 0 : m_cellsInSeg[seg][ix-1];
    Node* next_cell = ( ix == n ) ? 0 : m_cellsInSeg[seg][ix+1];

    double curr_gap = 0.;
    if( prev_cell )
    {
        curr_gap += m_arch->getCellSpacing( prev_cell, nd );
    }
    if( next_cell )
    {
        curr_gap += m_arch->getCellSpacing( nd, next_cell );
    }
    double next_gap = 0.;
    if( prev_cell && next_cell )
    {
        next_gap += m_arch->getCellSpacing( prev_cell, next_cell );
    }

    m_cellsInSeg[seg].erase( it ); // Removes the cell...
    m_segments[seg]->m_util -= nd->getWidth();  // Removes the utilization...
    m_segments[seg]->m_gapu -= curr_gap; // Updates the required gaps...
    m_segments[seg]->m_gapu += next_gap; // Updates the required gaps...
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeCellFromSegmentTest( Node* nd, int seg, double& util, double& gapu )
{
    // Current utilization and gap requirements.  Then, determine the change...
    util = m_segments[seg]->m_util;
    gapu = m_segments[seg]->m_gapu;

    std::vector<Node*>::iterator it = std::find( m_cellsInSeg[seg].begin(), m_cellsInSeg[seg].end(), nd );
    if( m_cellsInSeg[seg].end() == it )
    {
        cout << "Error." << endl;
        exit(-1);
    }
    int ix = it - m_cellsInSeg[seg].begin();
    int n = m_cellsInSeg[seg].size()-1;

    // Determine gaps with the cell in the row and with the cell not in the row.
    Node* prev_cell = ( ix == 0 ) ? 0 : m_cellsInSeg[seg][ix-1];
    Node* next_cell = ( ix == n ) ? 0 : m_cellsInSeg[seg][ix+1];

    double curr_gap = 0.;
    if( prev_cell )
    {
        curr_gap += m_arch->getCellSpacing( prev_cell, nd );
    }
    if( next_cell )
    {
        curr_gap += m_arch->getCellSpacing( nd, next_cell );
    }
    double next_gap = 0.;
    if( prev_cell && next_cell )
    {
        next_gap += m_arch->getCellSpacing( prev_cell, next_cell );
    }

    util -= nd->getWidth();  // Removes the utilization...
    gapu -= curr_gap; // Updates the required gaps...
    gapu += next_gap; // Updates the required gaps...
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::addCellToSegment( Node* nd, int seg )
{
    // Adding a node to a segment means a few things...  It means: 
    // 1) adding it to the SORTED cell list for the segment; 
    // 2) adding its width to the segment utilization; 
    // 3) adding the required gaps between cells in the segment.

    if( nd->getRegionId() != m_segments[seg]->m_regId )
    {
        int spanned = (int)(nd->getHeight()/m_singleRowHeight + 0.5);
        std::cout << "Warning: Assignment of cell " << nd->getId() << ", region " << nd->getRegionId() << ", "
            << "spanned is " << spanned << ", "
            << "assigned to segment with region " << m_segments[seg]->m_regId
            << std::endl;
    }

    // Need to figure out where the cell goes in the sorted list...
    std::vector<Node*>::iterator it = 
        std::lower_bound( m_cellsInSeg[seg].begin(), m_cellsInSeg[seg].end(), nd->getX(), compareNodesX() );
    if( it == m_cellsInSeg[seg].end() )
    {
        // Cell is at the end of the segment.

        Node* prev_cell = (m_cellsInSeg[seg].size() == 0) ? 0 : m_cellsInSeg[seg].back();
        double curr_gap = 0.;
        double next_gap = 0.;
        if( prev_cell )
        {
            next_gap += m_arch->getCellSpacing( prev_cell, nd );
        }

        m_cellsInSeg[seg].push_back( nd ); // Add the cell...
        m_segments[seg]->m_util += nd->getWidth();  // Adds the utilization...
        m_segments[seg]->m_gapu -= curr_gap; // Updates the required gaps...
        m_segments[seg]->m_gapu += next_gap; // Updates the required gaps...
    }
    else
    {
        int ix = it - m_cellsInSeg[seg].begin();
        Node* next_cell = m_cellsInSeg[seg][ix];
        Node* prev_cell = (ix == 0) ? 0 : m_cellsInSeg[seg][ix-1];

        double curr_gap = 0.;
        if( prev_cell && next_cell )
        {
            curr_gap += m_arch->getCellSpacing( prev_cell, next_cell );
        }
        double next_gap = 0.;
        if( prev_cell )
        {
            next_gap += m_arch->getCellSpacing( prev_cell, nd );
        }
        if( next_cell )
        {
            next_gap += m_arch->getCellSpacing( nd, next_cell );
        }

        m_cellsInSeg[seg].insert( it, nd ); // Adds the cell...
        m_segments[seg]->m_util += nd->getWidth();  // Adds the utilization...
        m_segments[seg]->m_gapu -= curr_gap; // Updates the required gaps...
        m_segments[seg]->m_gapu += next_gap; // Updates the required gaps...
    }

    std::vector<DetailedSeg*>::iterator its = std::find( 
        m_reverseCellToSegs[nd->getId()].begin(), m_reverseCellToSegs[nd->getId()].end(), 
        m_segments[seg] );
    if( m_reverseCellToSegs[nd->getId()].end() != its )
    {
        ERRMSG( "Found segment already present in reverse map when adding cell." );
        exit(-1);
    }
    int spanned = (int)(nd->getHeight()/m_singleRowHeight + 0.5 );
    if( m_reverseCellToSegs[nd->getId()].size() >= spanned )
    {
        ERRMSG( "Reverse cell to segment map in not properly sized." );
        exit(-1);
    }
    m_reverseCellToSegs[nd->getId()].push_back( m_segments[seg] );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::addCellToSegmentTest( Node* nd, int seg, double x, double& util, double& gapu )
{
    // Determine the utilization and required gaps if node 'nd' is added to segment 'seg' at position
    // 'x'.  XXX: This routine will should *BOT* be used to add a node to a segment in which it is
    // already placed!  XXX: The code does not make sense for a swap.

    // Current utilization and gap requirements.  Then, determine the change...
    util = m_segments[seg]->m_util;
    gapu = m_segments[seg]->m_gapu;

    // Need to figure out where the cell goes in the sorted list...
    std::vector<Node*>::iterator it = 
            std::lower_bound( m_cellsInSeg[seg].begin(), m_cellsInSeg[seg].end(), 
            x /* tentative location. */, 
            compareNodesX() );

    if( it == m_cellsInSeg[seg].end() )
    {
        // No node in segment with position that is larger than the current node.  The current node goes
        // at the end...

        Node* prev_cell = (m_cellsInSeg[seg].size() == 0) ? 0 : m_cellsInSeg[seg].back();
        double curr_gap = 0.;
        double next_gap = 0.;
        if( prev_cell )
        {
            next_gap += m_arch->getCellSpacing( prev_cell, nd );
        }

        util += nd->getWidth();  // Adds the utilization...
        gapu -= curr_gap; // Updates the required gaps...
        gapu += next_gap; // Updates the required gaps...
    }
   else
    {
        int ix = it - m_cellsInSeg[seg].begin();
        Node* next_cell = m_cellsInSeg[seg][ix];
        Node* prev_cell = (ix == 0) ? 0 : m_cellsInSeg[seg][ix-1];

        double curr_gap = 0.;
        if( prev_cell && next_cell )
        {
            curr_gap += m_arch->getCellSpacing( prev_cell, next_cell );
        }
        double next_gap = 0.;
        if( prev_cell )
        {
            next_gap += m_arch->getCellSpacing( prev_cell, nd );
        }
        if( next_cell )
        {
            next_gap += m_arch->getCellSpacing( nd, next_cell );
        }

        util += nd->getWidth();  // Adds the utilization...
        gapu -= curr_gap; // Updates the required gaps...
        gapu += next_gap; // Updates the required gaps...
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::recordOriginalDimensions( void )
{
    m_origW.resize( m_network->m_nodes.size() );
    m_origH.resize( m_network->m_nodes.size() );
    for( int i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node& nd = m_network->m_nodes[i];
        m_origW[nd.getId()] = nd.getWidth();
        m_origH[nd.getId()] = nd.getHeight();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::restoreOriginalDimensions( void )
{
    for( int i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node& nd = m_network->m_nodes[i];
        nd.setWidth( m_origW[nd.getId()] );
        nd.setHeight( m_origH[nd.getId()] );
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::recordOriginalPositions( void )
{
    m_origX.resize( m_network->m_nodes.size() );
    m_origY.resize( m_network->m_nodes.size() );
    for( int i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node& nd = m_network->m_nodes[i];
        m_origX[nd.getId()] = nd.getX();
        m_origY[nd.getId()] = nd.getY();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::restoreOriginalPositions( void )
{
    for( int i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node& nd = m_network->m_nodes[i];
        nd.setX( m_origX[nd.getId()] );
        nd.setY( m_origY[nd.getId()] );
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::recordBestPositions( void )
{
    m_bestX.resize( m_network->m_nodes.size() );
    m_bestY.resize( m_network->m_nodes.size() );
    for( int i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node& nd = m_network->m_nodes[i];
        m_bestX[nd.getId()] = nd.getX();
        m_bestY[nd.getId()] = nd.getY();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::restoreBestPositions( void )
{
    // This also required redoing the segments.
    for( int i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node& nd = m_network->m_nodes[i];
        nd.setX( m_bestX[nd.getId()] );
        nd.setY( m_bestY[nd.getId()] );
    }
    assignCellsToSegments( m_singleHeightCells );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedMgr::measureMaximumDisplacement( bool print, bool& violated )
{
    violated = false;

    //double limit = GET_PARAM_FLOAT( PLACER_MAX_DISPLACEMENT );  // XXX: Check the real limit, not with the tolerance.
    double limit = 1.0e8;



    double max_disp = 0.;
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

        double diffX = nd->getX() - m_origX[nd->getId()];
        double diffY = nd->getY() - m_origY[nd->getId()];

        max_disp = std::max( max_disp, std::fabs( diffX ) + std::fabs( diffY ) );
    }

    violated = (max_disp > limit) ? true : false;

    if( print )
    {
        cout << "Maximum displacement: " << max_disp << ", Limit: " << limit << ", "
             << ((max_disp > limit) ? "VIOLATION" : "PASS")
             << endl;
    }
    return max_disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::isNodeAlignedToRow( Node* nd )
{
    // Check to see if the node is aligned to a row.

    // XXX: UPDATE TO ACCOMMODATE MULTI-HEIGHT CELLS.  Likely requires using the bottom of the cell
    // instead of the center of the cell.  Need to assign a cell to multiple segments.


    int numRows = m_arch->m_rows.size();

    double xl = nd->getX() - 0.5 * nd->getWidth();
    double xr = nd->getX() + 0.5 * nd->getWidth();
    double yb = nd->getY() - 0.5 * nd->getHeight();
    double yt = nd->getY() + 0.5 * nd->getHeight();

    int rb = (int)((yb - m_arch->m_ymin) / m_singleRowHeight);
    int rt = (int)((yt - m_arch->m_ymin) / m_singleRowHeight);
    rb = std::min( numRows-1, std::max( 0, rb ) );
    rt = std::min( numRows, std::max( 0, rt ) );
    if( rt == rb ) ++rt;

    double bot_r = m_arch->m_ymin + rb * m_singleRowHeight;
    double top_r = m_arch->m_ymin + rt * m_singleRowHeight;

    if( !(std::fabs( yb - bot_r ) < 1.0e-3 && std::fabs( yt - top_r ) < 1.0e-3) )
    {
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::setupObstaclesForDrc( void )
{
    // Setup rectangular obstacles for short and pin access checks.  Do only as rectangles
    // per row and per layer.  I had used rtrees, but it wasn't working any better.
    double xmin, xmax, ymin, ymax;

    m_obstacles.resize( m_arch->m_rows.size() );

    for( int row_id = 0; row_id < m_arch->m_rows.size(); row_id++ )
    {
        m_obstacles[row_id].resize( m_rt->m_num_layers );

        double originX = m_arch->m_rows[row_id]->m_subRowOrigin;
        double siteSpacing = m_arch->m_rows[row_id]->m_siteSpacing;
        int numSites = m_arch->m_rows[row_id]->m_numSites;

        // Blockages relevant to this row...
        int count = 0;
        for( int layer_id = 0; layer_id < m_rt->m_num_layers; layer_id++ )
        {
            m_obstacles[row_id][layer_id].clear();

            std::vector<Rectangle>& rects = m_rt->m_layerBlockages[layer_id];
            for( int b = 0; b < rects.size(); b++ )
            {
                // Extract obstacles which interfere with this row only.
                xmin = originX;
                xmax = originX + numSites * siteSpacing;
                ymin = m_arch->m_rows[row_id]->getY();
                ymax = m_arch->m_rows[row_id]->getY() + m_arch->m_rows[row_id]->getH();

                if( rects[b].m_xmax <= xmin ) continue;
                if( rects[b].m_xmin >= xmax ) continue;
                if( rects[b].m_ymax <= ymin ) continue;
                if( rects[b].m_ymin >= ymax ) continue;

                m_obstacles[row_id][layer_id].push_back( rects[b] );
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkSegments( double& worst )
{
    // Compute the segment utilization and gap requirements.  Compute the
    // number of violations (return value) and the worst violation.  This
    // also updates the utilizations of the segments.

    int count = 0;

    worst = 0.;
    for( int i = 0; i < m_segments.size(); i++ )
    {
        DetailedSeg* segment = m_segments[i];

        double width = segment->m_xmax - segment->m_xmin;
        int segId = segment->m_segId;

        std::vector<Node*>& nodes = m_cellsInSeg[segId];
        std::stable_sort( nodes.begin(), nodes.end(), compareNodesX() ); 

        double util = 0.;
        for( int k = 0; k < nodes.size(); k++ )
        {
            util += nodes[k]->getWidth();
        }

        double gapu = 0.; 
        for( int k = 1; k < nodes.size(); k++ )
        {
            gapu += m_arch->getCellSpacing( nodes[k-1], nodes[k-0] );
        }

        segment->m_util = util;
        segment->m_gapu = gapu;

        if( util + gapu > std::max( 0.0, width ) )
        {
            ++count;
        }
        worst = std::max( worst, ((util + gapu) - width) );
    }

    return count;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::collectSingleHeightCells( void )
{
    // Routine to collect only the movable single height cells.  
    //
    // XXX: This code also shifts cells to ensure that they are within the 
    // placement area.  It also lines the cell up with its bottom row by
    // assuming rows are stacked continuously one on top of the other which
    // may or may not be a correct assumption.
    // Do I need to do any of this really?????????????????????????????????

    m_singleHeightCells.erase( m_singleHeightCells.begin(), m_singleHeightCells.end() );
    m_singleRowHeight = m_arch->m_rows[0]->m_rowHeight;
    m_numSingleHeightRows = m_arch->m_rows.size();

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

        int nRowsSpanned = (int)((nd->getHeight() / m_singleRowHeight) + 0.5);
        if( nRowsSpanned != 1 )
        {
            continue;
        }

        m_singleHeightCells.push_back( nd );
    }
    cout << "Number of cells is " << m_network->m_nodes.size() << ", "
        << "Number of single height cells is " << m_singleHeightCells.size() << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::collectMultiHeightCells( void )
{
    // Routine to collect only the movable multi height cells.  
    //
    // XXX: This code also shifts cells to ensure that they are within the 
    // placement area.  It also lines the cell up with its bottom row by
    // assuming rows are stacked continuously one on top of the other which
    // may or may not be a correct assumption.
    // Do I need to do any of this really?????????????????????????????????


    m_multiHeightCells.erase( m_multiHeightCells.begin(), m_multiHeightCells.end() );
    // Just in case...  Make the matrix for holding multi-height cells at 
    // least large enough to hold single height cells (although we don't
    // even bothering storing such cells in this matrix).
    m_multiHeightCells.resize( 2 );
    for( size_t i = 0; i < m_multiHeightCells.size(); i++ )
    {
        m_multiHeightCells[i] = std::vector<Node*>();
    }
    m_singleRowHeight = m_arch->m_rows[0]->m_rowHeight;
    m_numSingleHeightRows = m_arch->m_rows.size();

    int m_numMultiHeightCells = 0;
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

        int nRowsSpanned = (int)((nd->getHeight() / m_singleRowHeight) + 0.5);
        if( nRowsSpanned == 1 )
        {
            continue;
        }

        if( nRowsSpanned >= m_multiHeightCells.size() )
        {
            m_multiHeightCells.resize( nRowsSpanned+1, std::vector<Node*>() );
        }
        m_multiHeightCells[nRowsSpanned].push_back( nd );
        ++m_numMultiHeightCells;
    }
    cout << "Number of cells is " << m_network->m_nodes.size() << ", "
        << "Number of multi height cells is " << m_numMultiHeightCells << endl;
    for( size_t i = 0; i < m_multiHeightCells.size(); i++ )
    {
        if( m_multiHeightCells[i].size() == 0 )
        {
            continue;
        }
        std::cout << "Number of multi height cells spanning " << i << " rows is " 
            << m_multiHeightCells[i].size() 
            << std::endl;
    }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::moveMultiHeightCellsToFixed( void )
{
    // Adds multi height cells to the list of fixed cells.  This is a hack
    // if we are only placing single height cells; we really should not 
    // need to do this...

    // Be safe; recompute the fixed cells.
    collectFixedCells(); 

    for( size_t i = 0; i < m_multiHeightCells.size(); i++ )
    {
        m_fixedCells.insert( m_fixedCells.end(), m_multiHeightCells[i].begin(), m_multiHeightCells[i].end() );
        // XXX: Should we mark the multi height cells are temporarily fixed???
    }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::collectFixedCells( void )
{
    // Fixed cells are used only to create blockages which, in turn, are used to
    // create obstacles.  Obstacles are then used to create the segments into 
    // which cells can be placed.
    
    m_fixedMacros.erase( m_fixedMacros.begin(), m_fixedMacros.end() );
    m_fixedCells.erase( m_fixedCells.begin(), m_fixedCells.end() );
    double rowHeight = m_arch->m_rows[0]->m_rowHeight;

    // Insert filler.
    for( size_t i = 0; i < m_network->m_filler.size(); i++ )
    {
        // Filler does not count as fixed macro...
        Node* nd = m_network->m_filler[i];
        m_fixedCells.push_back( nd );
    }
    // Insert fixed items, shapes AND macrocells.
    for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node* nd = &(m_network->m_nodes[i]);

        if( nd->getFixed() == NodeFixed_NOT_FIXED )
        {
            continue;
        }

        // Terminals can have area too!
        //if( nd->getType() == NodeType_TERMINAL || nd->getType() == NodeType_TERMINAL_NI )
        //{
        //    continue;
        //}

        // Fixed or macrocell (with or without shapes).
        if( m_network->m_shapes[nd->getId()].size() == 0 )
        {
            m_fixedMacros.push_back( nd );
            m_fixedCells.push_back( nd );
        }
        else
        {
            // Shape.
            for( int j = 0; j < m_network->m_shapes[nd->getId()].size(); j++ )
            {
                Node* shape = m_network->m_shapes[nd->getId()][j];
                m_fixedMacros.push_back( shape );
                m_fixedCells.push_back( shape );
            }
        }
    }
    cout << "Number of cells is " << m_network->m_nodes.size() << ", "
        << "Number of fixed cells is " << m_fixedCells.size() << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::collectWideCells( void )
{
    // This is sort of a hack.  Some standard cells might be extremely wide and
    // based on how we set up segments (e.g., to take into account blockages of
    // different sorts), we might not be able to find a segment wide enough to 
    //accomodate the cell.  In this case, we will not be able to resolve a bunch
    // of problems.
    //
    // My current solution is to (1) detect such cells; (2) recreate the segments 
    // without blockages; (3) insert the wide cells into segments; (4) fix the
    // wide cells; (5) recreate the entire problem with the wide cells considered
    // as fixed.

    m_wideCells.erase( m_wideCells.begin(), m_wideCells.end() );
    for( int s = 0; s < m_segments.size(); s++ )
    {
        DetailedSeg* curr = m_segments[s];

        std::vector<Node*>& nodes = m_cellsInSeg[s];
        for( int k = 0; k < nodes.size(); k++ )
        {
            Node* ndi = nodes[k];
            if( ndi->getWidth() > curr->m_xmax - curr->m_xmin )
            {   
                m_wideCells.push_back( ndi );
            }
        }
    }
    cout << "Number of cells is " << m_network->m_nodes.size() << ", "
        << "Number of wide cells is " << m_wideCells.size() << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeOverlapMinimumShift( void )
{
    // XXX: UPDATE TO ACCOMMODATE MULTI-HEIGHT CELLS.  Likely requires using the bottom of the cell
    // instead of the center of the cell.  Need to assign a cell to multiple segments.

    //double disp_limit = 0.98 * GET_PARAM_FLOAT( PLACER_MAX_DISPLACEMENT );
    double disp_limit = 1.0e8;

    // Simple heuristic to remove overlap amoung cells in all segments.  If we
    // cannot satisfy gap requirements, we will ignore the gaps.  If the cells
    // are just too wide, we will end up with overlap.
    //
    // We will print some warnings about different sorts of failures.

    int nWidthFails = 0;
    int nDispFails = 0;
    int nGapFails = 0;

    std::vector<double> llx;
    std::vector<double> tmp;
    std::vector<double> wid;

    std::vector<double> tarr;
    std::vector<double> treq;
    double x;
    for( int s = 0; s < m_segments.size(); s++ )
    {
        DetailedSeg* segment = m_segments[s];

        int r = segment->m_rowId;

        std::vector<Node*>& nodes = m_cellsInSeg[segment->m_segId];
        if( nodes.size() == 0 )
        {
            continue;
        }

        std::sort( nodes.begin(), nodes.end(), compareNodesX() );

        double util = 0.;
        for( int j = 0; j < nodes.size(); j++ )
        {
            util += nodes[j]->getWidth();
        }

        double gapu = 0.;
        for( int j = 1; j < nodes.size(); j++ )
        {
            gapu += m_arch->getCellSpacing( nodes[j-1], nodes[j-0] );
        }

        segment->m_util = util;
        segment->m_gapu = gapu;

        int rowId = segment->m_rowId;

        double originX = m_arch->m_rows[rowId]->m_subRowOrigin;
        double siteSpacing = m_arch->m_rows[rowId]->m_siteSpacing;

        llx.resize( nodes.size() );
        tmp.resize( nodes.size() );
        wid.resize( nodes.size() );

        double space = segment->m_xmax- segment->m_xmin;

        for( int i = 0; i < nodes.size(); i++ )
        {
            Node* nd = nodes[i];
            wid[i]   = nd->getWidth();
        }
        // XXX: Adjust cell widths to satisfy gap constraints.  But, we prefer to violate gap constraints rather
        // than having cell overlap.  So, if an increase in width will violate the available space, then simply
        // skip the width increment.
        bool failedToSatisfyGaps = false;
        for( int i = 1; i < nodes.size(); i++ )
        {
            Node* ndl = nodes[i-1];
            Node* ndr = nodes[i-0];

            double gap = m_arch->getCellSpacing( ndl, ndr );
            // 'util' is the total amount of cell width (including any gaps included so far...).  'gap' is the
            // next amount of width to be included.  Skip the gap if we are going to violate the space limit.
            if( gap != 0.0 )
            {
                if( util + gap <= space )
                {
                    wid[i-1] += gap;
                    util += gap;
                }
                else
                {
                    failedToSatisfyGaps = true;
                }
            }
        }

        if( failedToSatisfyGaps )
        {
            ++nGapFails;
        }
        double cell_width = 0.;
        for( int i = 0; i < nodes.size(); i++ )
        {
            cell_width += wid[i];
        }

        if( cell_width > space )
        {
            // Scale... now things should fit, but will overlap...
            double scale = space / cell_width;
            for( int i = 0; i < nodes.size(); i++ )
            {
                wid[i] *= scale;
            }

            ++nWidthFails;
        }
        for( int i = 0; i  < nodes.size(); i++ )
        {
            Node* nd = nodes[i];
            llx[i] = ((int)(((nd->getX() - 0.5 * nd->getWidth()) - originX)/siteSpacing)) * siteSpacing + originX;
        }

        // Determine the leftmost and rightmost location for the edge of the cell while trying
        // to satisfy the displacement limits.  If we discover that we cannot satisfy the
        // limits, then ignore the limits and shift as little as possible to remove the overlap.
        // If we cannot satisfy the limits, then it's a problem we must figure out how to
        // address later.  Note that we might have already violated the displacement limit in the
        // Y-direction in which case then "game is already over".

        tarr.resize( nodes.size() );
        treq.resize( nodes.size() );

        x = segment->m_xmin;
        for( int i = 0; i <  nodes.size(); i++ )
        {
            Node* ndi = nodes[i];

            double limit = std::max( 0.0, disp_limit - std::fabs( ndi->getY() - getOrigY(ndi) ) );
            double pos = (getOrigX(ndi) - 0.5*ndi->getWidth()) - limit;
            pos = std::max( pos, segment->m_xmin );

            long int ix = (long int)((pos - originX)/siteSpacing);
            if( originX + ix * siteSpacing  < pos )
                ++ix;
            if( originX + ix * siteSpacing != pos )
                pos = originX + ix * siteSpacing;

            tarr[i] = std::max( x, pos );
            x = tarr[i] + wid[i];
        }
        x = segment->m_xmax;
        for( int i = nodes.size()-1; i >= 0; i-- )
        {
            Node* ndi = nodes[i];

            double limit = std::max( 0.0, disp_limit - std::fabs( ndi->getY() - getOrigY(ndi) ) );
            double pos = (getOrigX(ndi) + 0.5*ndi->getWidth()) + limit;
            pos = std::min( pos, x );

            long int ix = (long int)((pos - originX)/siteSpacing);

            if( originX + ix * siteSpacing != pos )
                pos = originX + ix * siteSpacing;

            treq[i] = std::min( x, pos );
            x = treq[i] - wid[i];
        }

        // For each node, if treq[i]-tarr[i] >= wid[i], then the cell will fit.  If not, we cannot satisfy
        // the displacement limit, so just shift for minimum movement.
        bool okay_disp = true;
        for( int i = 0; i < nodes.size(); i++ )
        {
            Node* ndi = nodes[i];

            if( treq[i]-tarr[i] < wid[i] )
            {
                okay_disp = false;
            }
        }

        if( okay_disp )
        {
            // Do the shifting while observing the limits...
            x = segment->m_xmax;
            for( int i = nodes.size()-1; i >= 0; i-- )
            {
                Node* ndi = nodes[i];
                // Node cannot go beyond (x-wid[i]), prefers to be at llx[i], but cannot be below tarr[i].
                x = std::min( x, treq[i] );
                llx[i] = std::max( tarr[i], std::min( x-wid[i], llx[i] ) );
                x = llx[i];
            }
        }
        else
        {
            // Do the shifting to minimize movement from current location...

            // The leftmost position for each cell.
            x = segment->m_xmin;
            for( int i = 0; i < nodes.size(); i++ )
            {
                tmp[i] = x;
                x += wid[i];
            }
            //  The rightmost position for each cell.
            x = segment->m_xmax;
            for( int i = nodes.size()-1; i >= 0; i-- )
            {
                // Node cannot be beyond (x-wid[i]), prefers to be at llx[i], but cannot be below tmp[i].
                llx[i] = std::max( tmp[i], std::min( x-wid[i], llx[i] ) );
                x = llx[i];
            }
        }

        for( int i = 0; i < nodes.size(); i++ )
        {
            Node* ndi = nodes[i];

            ndi->setX( llx[i] + 0.5 * ndi->getWidth() );

            double dx = ndi->getX() - getOrigX(ndi);
            double dy = ndi->getY() - getOrigY(ndi);
            double limit = std::fabs( dx ) + std::fabs( dy );
            if( limit > disp_limit )
            {
                ++nDispFails;
            }
        }
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::cleanup( void )
{
    // Various cleanups.
    for( int i = 0; i < m_wideCells.size(); i++ )
    {
        Node* ndi = m_wideCells[i];
        ndi->setFixed( NodeFixed_NOT_FIXED );
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::setup( void )
{
    // XXX: UPDATE TO ACCOMMODATE MULTI-HEIGHT CELLS.  Likely requires using the bottom of the cell
    // instead of the center of the cell.  Need to assign a cell to multiple segments.

    //bool includeBlockages = GET_PARAM_BOOL( PLACER_MARK_M1M2_ROUTE_BLOCKAGES_AS_PLACE_BLOCKAGES );
    bool includeBlockages = false;

    // Setup.  Collect fixed and movable cells.  Create blockages and segments.
    // Insert the movable cells into the segments.
    collectFixedCells();
    collectSingleHeightCells();
    findBlockages( m_fixedCells, includeBlockages );
    findSegments();
    assignCellsToSegments( m_singleHeightCells );

    // Look for very wide single height cells.  If any are found, fix them and
    // redo the previous work.
    collectWideCells();   

    // If no wide cells, then we are done.
    if( m_wideCells.size() == 0 )
    {
        return;
    }

    // Redo blockages and segments without extra blockages and insert only wide cells.
    findBlockages( m_fixedCells, false );   // Recompute. IGNORE blockages.
    findSegments(); // Recompute.
    assignCellsToSegments( m_wideCells ); // Recompute.  ONLY wide cells.

    // Some debug/warning.
    int nFailures = 0;
    for( int s = 0; s < m_segments.size(); s++ )
    {
        DetailedSeg* curr = m_segments[s];

        std::vector<Node*>& nodes = m_cellsInSeg[s];
        for( int k = 0; k < nodes.size(); k++ )
        {
            Node* ndi = nodes[k];
            if( ndi->getWidth() > curr->m_xmax - curr->m_xmin )
            {
                ++nFailures;
            }
        }
    }
    std::cout << "Wide cells, "
        << "Segment issues" << ((nFailures!=0)?" not ":" ") << "resolved." << std::endl;

    removeOverlapMinimumShift();
    DetailedOrient orienter( m_arch, m_network, m_rt );
    orienter.run( this, "orient" );
    // We SHOULD look for DRC issues for the wide cells since we are about to fix them.
    // XXX: Since I am reorganizing code, this option is not yet available.  So, print
    // something so I don't forget about it!
    std::cout << "Warning: " 
        << "Fixing wide cells should include DRC correction (fixme!)." << std::endl;

    // Fix the wide cells.
    for( int i = 0; i < m_wideCells.size(); i++ )
    {
        Node* ndi = m_wideCells[i];
        ndi->setFixed( NodeFixed_FIXED_XY );
    }

    // Redo everything with the wide cells marked as fixed.
    collectFixedCells(); 
    collectSingleHeightCells(); 
    findBlockages( m_fixedCells, includeBlockages );
    findSegments();
    assignCellsToSegments( m_singleHeightCells );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkOverlapInSegments( int max_err_n )
{
    // Scan each segment and check for overlap between adjacenet cells.
    // Also check for cells off the left and right edge of the segments.

    std::vector<Node*> temp;
    temp.reserve( m_network->m_nodes.size() );

    int err_n = 0;
    int err_s = 0;
    int err_g = 0;
    // The following is for some printing if we need help finding a bug.
    // I don't want to print all the potential errors since that could
    // be too overwhelming.
    for( int s = 0; s < m_segments.size(); s++ )
    {
        int rowId = m_segments[s]->m_rowId;
        double rowY = m_arch->m_rows[rowId]->getY() + 0.5 * m_arch->m_rows[rowId]->getH();

        double xmin = m_segments[s]->m_xmin;
        double xmax = m_segments[s]->m_xmax;
        double width = xmax - xmin;
        
        if( m_segments[s]->m_util >= width+1.0e-3 )
        {
            ++err_s;
        }
        if( m_segments[s]->m_util + m_segments[s]->m_gapu >= width+1.0e-3 )
        {
            ++err_g;
        }

        // To be safe, gather cells in each segment and re-sort them.
        temp.erase( temp.begin(), temp.end() );
        for( int j = 0; j < m_cellsInSeg[s].size(); j++ )
        {
            Node* ndj = m_cellsInSeg[s][j];
            temp.push_back( ndj );
        }
        std::sort( temp.begin(), temp.end(), compareNodesX() );

        for( int j = 1; j < temp.size(); j++ )
        {
            Node* ndi = temp[j-1];
            Node* ndj = temp[j];

            double li = ndi->getX() - 0.5*ndi->getWidth() ;
            double ri = ndi->getX() + 0.5*ndi->getWidth() ;

            double lj = ndj->getX() - 0.5*ndj->getWidth() ;
            double rj = ndj->getX() + 0.5*ndj->getWidth() ;

            if( ri >= lj+1.0e-3 )
            {
                err_n++;
                if( err_n <= max_err_n )
                {
                    double ll = std::max( li, lj );
                    double rr = std::min( ri, rj );
                    double overlap = std::max( rr-ll, 0.0 );

                    std::cout << "Adjacent cells overlap in segment " << s << ", "
                        << "Cell " << ndi->getId() << " @ " << "[" << li << "," << ri << "]" << ", " 
                        << "Region is " << ndi->getRegionId() << ", "
                        << "Cell " << ndj->getId() << " @ " << "[" << lj << ","  << rj << "]" << ", "
                        << "Region is " << ndj->getRegionId() << ", "
                        << "Overlap is " << overlap << ", "
                        << "Row position is " << rowY
                        << std::endl;
                }
            }
        }
        for( int j = 0; j < temp.size(); j++ )
        {
            Node* ndi = temp[j];

            double li = ndi->getX() - 0.5*ndi->getWidth() ;
            double ri = ndi->getX() + 0.5*ndi->getWidth() ;

            if( li <= xmin-1.0e-3 || ri >= xmax+1.0e-3 )
            {
                err_n++;

                if( err_n <= max_err_n )
                {

                    std::cout << "Cell out of segment " << s << ", " << "Region is " << m_segments[s]->m_regId << ", "
                        << "Cell " << ndi->getId() << " @ " << "[" << li << "," << ri << "]" << ", " 
                        << "Region is " << ndi->getRegionId() << ", "
                        << "Segment @ [" << xmin << "," << xmax << "]"
                        << std::endl;
                }
            }
        }
    }

    if( err_s == 0 )
    {
        std::cout << "Segment check - no segments exceeding capacity." << std::endl;
    }
    else
    {
        std::cout << "Segment check - found " << err_s << " overfilled segments." << std::endl;
    }

    if( err_g == 0 )
    {
        std::cout << "Segment check - including cell spacings, no segments exceeding capacity." << std::endl;
    }
    else
    {
        std::cout << "Segment check - including cell spacings, found " << err_g << " overfilled segments." << std::endl;
    }

    if( err_n == 0 )
    {
        std::cout << "Segment check - no overlaps in segments." << std::endl;
    }
    else
    {
        std::cout << "Segment check - found " << err_n << " overlap between adjacent cells." << std::endl;
    }
    return err_n;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkEdgeSpacingInSegments( int max_err_n )
{
    // Check for spacing violations according to the spacing table.  Note
    // that there might not be a spacing table in which case we will 
    // return no errors.  I should also check for padding errors although
    // we might not have any paddings either! :).

    std::vector<Node*> temp;
    temp.reserve( m_network->m_nodes.size() );

    double dummyPadding = 0.;
    double rightPadding = 0.;
    double leftPadding = 0.;

    int err_n = 0;
    int err_p = 0;
    for( int s = 0; s < m_segments.size(); s++ )
    {
        double xmin = m_segments[s]->m_xmin;
        double xmax = m_segments[s]->m_xmax;

        // To be safe, gather cells in each segment and re-sort them.
        temp.erase( temp.begin(), temp.end() );
        for( int j = 0; j < m_cellsInSeg[s].size(); j++ )
        {
            Node* ndj = m_cellsInSeg[s][j];
            temp.push_back( ndj );
        }
        std::sort( temp.begin(), temp.end(), compareNodesL() );

        for( int j = 1; j < temp.size(); j++ )
        {
            Node* ndl = temp[j-1];
            Node* ndr = temp[j];

            double llx_l = ndl->getX() - 0.5 * ndl->getWidth();
            double rlx_l = llx_l + ndl->getWidth();

            double llx_r = ndr->getX() - 0.5 * ndr->getWidth();
            double rlx_r = llx_r + ndr->getWidth();

            double gap = llx_r - rlx_l;

            double spacing = m_arch->getCellSpacingUsingTable( ndl->getRightEdgeType(), ndr->getLeftEdgeType() );

            m_arch->getCellPadding( ndl, dummyPadding, rightPadding );
            m_arch->getCellPadding( ndr, leftPadding, dummyPadding );
            double padding = leftPadding + rightPadding;
            
            if( !(gap >= spacing-1.0e-3) )
            {
                ++err_n;
                if( err_n <= max_err_n )
                {
                    std::cout << "Spacing violation" << ", "
                        << "Cell " << ndl->getId() << " @ " << "[" << llx_l << "," << rlx_l << "]" << ", "
                        << "Cell " << ndr->getId() << " @ " << "[" << llx_r << "," << rlx_r << "]" << ", "
                        << std::endl;
                }
            }
            if( !(gap >= padding-1.0e-3) )
            {
                ++err_p;
                if( err_p <= max_err_n )
                {
                    std::cout << "Padding violation" << ", "
                        << "Cell " << ndl->getId() << " @ " << "[" << llx_l << "," << rlx_l << "]" << ", "
                        << "Cell " << ndr->getId() << " @ " << "[" << llx_r << "," << rlx_r << "]" << ", "
                        << std::endl;
                }
            }
        }
    }

    if( err_n == 0 )
    {
        std::cout << "Segment check - no edge spacing violations." << std::endl;
    }
    else 
    {
        std::cout << "Segment check - found " << err_n << " edge spacing violations." << std::endl;
    }

    if( err_p == 0 )
    {
        std::cout << "Segment check - no padding violations." << std::endl;
    }
    else
    {
        std::cout << "Segment check - found " << err_p << " padding violations." << std::endl;
    }

    return err_n+err_p;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkRegionAssignment( int max_err_n )
{
    // Check cells are assigned (within) their proper regions.  This is sort
    // of a hack/cheat.  We assume that we have set up the segments correctly
    // and that all cells are in segments.  Multi-height cells can be in
    // multiple segments.
    //
    // Therefore, if we scan the segments and the cells have a region ID that
    // matches the region ID for the segment, the cell must be within its
    // region.  Note: This is not true if the cell is somehow outside of its
    // assigned segments.  However, that issue would be caught when checking
    // the segments themselves.

    std::vector<Node*> temp;
    temp.reserve( m_network->m_nodes.size() );

    int err_n = 0;
    for( int s = 0; s < m_segments.size(); s++ )
    {
        double xmin = m_segments[s]->m_xmin;
        double xmax = m_segments[s]->m_xmax;

        // To be safe, gather cells in each segment and re-sort them.
        temp.erase( temp.begin(), temp.end() );
        for( int j = 0; j < m_cellsInSeg[s].size(); j++ )
        {
            Node* ndj = m_cellsInSeg[s][j];
            temp.push_back( ndj );
        }
        std::sort( temp.begin(), temp.end(), compareNodesL() );

        for( int j = 0; j < temp.size(); j++ )
        {
            Node* ndi = temp[j];
            if( ndi->getRegionId() != m_segments[s]->m_regId )
            {
                ++err_n;
            }
        }
    }

    if( err_n == 0 )
    {
        std::cout << "Region check - all cells within segment with proper region." << std::endl;
    }
    else
    {
        std::cout << "Region check - found " << err_n << " cells in wrong segments based on regions." << std::endl;
    }
    return err_n;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkSiteAlignment( int max_err_n )
{
    // Ensure that the left edge of each cell is aligned with a site.  We only
    // consider cells that are within segments.
    int err_n = 0;

    double singleRowHeight = getSingleRowHeight();
    int nCellsInSegments = 0;
    int nCellsNotInSegments = 0;
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

        double xl = nd->getX() - 0.5 * nd->getWidth() ;
        double xr = nd->getX() + 0.5 * nd->getWidth() ;
        double yb = nd->getY() - 0.5 * nd->getHeight();
        double yt = nd->getY() + 0.5 * nd->getHeight();

        // Determine the spanned rows. XXX: Is this strictly correct?  It 
        // assumes rows are continuous and that the bottom row lines up
        // with the bottom of the architecture.
        int rb = (int)((yb - m_arch->m_ymin) / singleRowHeight);
        int spanned = (int)((nd->getHeight()/singleRowHeight)+0.5);
        int rt = rb+spanned-1;

        if( m_reverseCellToSegs[nd->getId()].size() == 0 )
        {
            ++nCellsNotInSegments;
            continue;
        }
        else if( m_reverseCellToSegs[nd->getId()].size() != spanned )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
        ++nCellsInSegments;
        
        bool okay = true;
        if( rb < 0 || rt >= m_arch->m_rows.size() )
        {
            // Either off the top of the bottom of the chip, so this is not
            // exactly an alignment problem, but still a problem so count it.
            ++err_n;
            okay = false;
        }
        rb = std::max( rb, 0 );
        rt = std::min( rt, (int)m_arch->m_rows.size()-1 );
       
        for(int r = rb; r <= rt; r++ )
        {
            double siteSpacing = m_arch->m_rows[r]->m_siteSpacing;
            double originX = m_arch->m_rows[r]->m_subRowOrigin;

            // XXX: Should I check the site to the left and right to avoid rounding errors???
            int sid = (int)( ( (xl - originX) / siteSpacing ) + 0.5 );
            double xt = originX + sid * siteSpacing;
            if( std::fabs(xl - xt ) > 1.0e-3 )
            {
                ++err_n;
                okay = false;
            }
        }

        if( !okay )
        {
            if( err_n <= max_err_n )
            {
                std::cout << "Site alignment problem for cell " << nd->getId() << ", "
                    << "X:[" << xl << "," << xr << "]" << ", "
                    << "Y:[" << yb << "," << yt << "]" << ", "
                    << "R:[" << rb << "," << rt << "]" << ", Spanned is " << spanned << ", ";
                std::cout << "alignments in each row:";
                for(int r = rb; r <= rt; r++ )
                {
                    int sid = (int)( (xl - m_arch->m_rows[r]->m_subRowOrigin) / m_arch->m_rows[r]->m_siteSpacing);
                    double xt = m_arch->m_rows[r]->m_subRowOrigin + sid * m_arch->m_rows[r]->m_siteSpacing;
                    std::cout << " " << xt;
                }
                std::cout << std::endl;
            }
        }
    }
    if( err_n == 0 )
    {
        std::cout << "Site check - no site alignment problems" << ", "
            << "Examined " << nCellsInSegments << " cells in segments" << ", "
            << "Skipped " << nCellsNotInSegments << " not in segments."
            << std::endl;
    }
    else
    {
        std::cout << "Site check - found " << err_n << " site alignment problems" << ", "
            << "Out of " << nCellsInSegments << " cells in segments" << ", "
            << "Skipped " << nCellsNotInSegments << " not in segments."
            << std::endl;
    }
    return err_n;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkRowAlignment( int max_err_n )
{
    // Ensure that the bottom of each cell is aligned with a row.
    int err_n = 0;

    double singleRowHeight = getSingleRowHeight();
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
        double xl = nd->getX() - 0.5 * nd->getWidth() ;
        double xr = nd->getX() + 0.5 * nd->getWidth() ;
        double yb = nd->getY() - 0.5 * nd->getHeight();
        double yt = nd->getY() + 0.5 * nd->getHeight();

        // Determine the spanned rows. XXX: Is this strictly correct?  It 
        // assumes rows are continuous and that the bottom row lines up
        // with the bottom of the architecture.
        int rb = (int)((yb - m_arch->m_ymin) / singleRowHeight);
        int spanned = (int)((nd->getHeight()/singleRowHeight)+0.5);
        int rt = rb+spanned-1;

        if( rb < 0 || rt >= m_arch->m_rows.size() )
        {
            // Either off the top of the bottom of the chip, so this is not
            // exactly an alignment problem, but still a problem so count it.
            ++err_n;
            continue;
        }
        double y1 = m_arch->m_ymin + rb * singleRowHeight;
        double y2 = m_arch->m_ymin + rt * singleRowHeight + singleRowHeight;

        if( std::fabs(yb - y1) > 1.0e-3 || std::fabs(yt - y2) > 1.0e-3 )
        {
            ++err_n;
        }
    }
    if( err_n == 0 )
    {
        std::cout << "Row check - no row alignment problems." << std::endl;
    }
    else
    {
        std::cout << "Row check - found " << err_n << " row alignment problems." << std::endl;
    }
    return err_n;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::checkPlacement( void )
{
    // XXX: UPDATE TO ACCOMMODATE MULTI-HEIGHT CELLS.  Likely requires using the bottom of the cell
    // instead of the center of the cell.  Need to assign a cell to multiple segments.

    // Perform various placement checks to see if things are legal.
    double singleRowHeight = getSingleRowHeight();


    int numRows = m_arch->m_rows.size();

    int err_1 = 0; int err_1_print_limit =  5;
    int err_2 = 0; int err_2_print_limit =  5;
    int err_3 = 0; int err_3_print_limit =  5;
    int err_4 = 0; int err_4_print_limit =  5;
    int err_5 = 0; int err_5_print_limit =  5;
    int err_6 = 0; int err_6_print_limit =  5;

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

        double xl = nd->getX() - 0.5 * nd->getWidth();
        double xr = nd->getX() + 0.5 * nd->getWidth();
        double yb = nd->getY() - 0.5 * nd->getHeight();
        double yt = nd->getY() + 0.5 * nd->getHeight();

        int rb = (int)((yb - m_arch->m_ymin) / singleRowHeight);
        int rt = (int)((yt - m_arch->m_ymin) / singleRowHeight);
        rb = std::min( numRows-1, std::max( 0, rb ) );
        rt = std::min( numRows, std::max( 0, rt ) );
        if( rt == rb ) ++rt;
        // Cell outside the image area.
        if( (xl > m_arch->m_xmax) || (xr < m_arch->m_xmin) || (yb > m_arch->m_ymax) || (yt < m_arch->m_ymin) )
        {
            err_1++;

            if( err_1 < err_1_print_limit  )
            {
                cout << "Error 1: Moveable node "
                     << "(" << m_network->m_nodeNames[nd->getId()].c_str() << ") is outside the image. "
                     << "Node: "
                     << "[" << xl << "," << yb << "]-["  << xr << "," << yt  << "] "
                     << "Image: "
                     << "[" << m_arch->m_xmin << "," << m_arch->m_ymin << "]"
                     << "-"
                     << "[" << m_arch->m_xmax << "," << m_arch->m_ymax << "] "
                     << endl;
            }
        }
        else
        {
            if( (yb < m_arch->m_ymin) || (yt > m_arch->m_ymax) )
            {
                err_1++;
                if( err_1 < err_1_print_limit )
                {
                    cout << "Error 1: Moveable node "
                     << "(" << m_network->m_nodeNames[nd->getId()].c_str() << ") is outside the image. "
                     << "Node Y: [" << yb << ","  << yt  << "] "
                     << "Image Y: [" << m_arch->m_ymin << "," << m_arch->m_ymax << "] "
                     << endl;
                }
            }
            for( int r = rb; r < rt; r++ )
            {
                double xmin = m_arch->m_rows[r]->m_subRowOrigin;
                double xmax = m_arch->m_rows[r]->m_subRowOrigin + m_arch->m_rows[r]->m_numSites * m_arch->m_rows[r]->m_siteSpacing;
                if( xl < xmin || xr > xmax )
                {
                    err_1++;
                    if(  err_1 < err_1_print_limit )
                    {
                        cout << "Error 1: Moveable node "
                             << "(" << m_network->m_nodeNames[nd->getId()].c_str() << ") is outside the image. "
                             << "Node X: [" << xl << ","  << xr  << "] "
                             << "Image X: [" << xmin << "," << xmax << "] "
                             << endl;
                    }
                }
            }
        }
    }

    // Check row alignment.
    err_2 = checkRowAlignment();

    // Check site alignment.
    err_3 = checkSiteAlignment();

    // Check overlaps within segments.
    err_4 = checkOverlapInSegments();

    // Check edge spacing rules.
    err_5 = checkEdgeSpacingInSegments();

    // Check region assignment.
    err_6 = checkRegionAssignment();


    // Output.
    if( err_1 >= err_1_print_limit )
    {
        cout << "+ " << err_1 - err_1_print_limit <<  " other type 1 errors." << endl;
    }
    if( err_2 >= err_2_print_limit )
    {
        cout << "+ " << err_2 - err_2_print_limit <<  " other type 2 errors." << endl;
    }
    if( err_3 >= err_3_print_limit )
    {
        cout << "+ " << err_3 - err_3_print_limit <<  " other type 3 errors." << endl;
    }
    if( err_4 >= err_4_print_limit )
    {
        cout << "+ " << err_4 - err_4_print_limit <<  " other type 4 errors." << endl;
    }
    if( err_5 >= err_5_print_limit )
    {
        cout << "+ " << err_5 - err_5_print_limit <<  " other type 5 errors." << endl;
    }
    if( err_6 >= err_6_print_limit )
    {
        cout << "+ " << err_6 - err_6_print_limit <<  " other type 6 errors." << endl;
    }


    int tot_errors = 0;

    tot_errors += err_1;
    tot_errors += err_2;
    tot_errors += err_3;
    tot_errors += err_4;
    tot_errors += err_5;
    tot_errors += err_6;

    if( tot_errors != 0 )
    {
        cout << "Found placement check errors: "
             << err_1 << ", " << err_2 << ", " << err_3 << ", " << err_4 << ", " << err_5 << ", " << err_6
             << "; should look into this!"
             << endl;
    }
    else
    {
        cout  << "Placement check is okay." << endl;
    }


    return (tot_errors == 0);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedMgr::getCellSpacing( Node* ndl, Node* ndr, bool checkPinsOnCells )
{
    // Compute any required spacing between cells.  This could be from an edge type rule, or due to
    // adjacent pins on the cells.  Checking pins on cells is more time consuming.

    if( ndl == 0 || ndr == 0 )
    {
        return 0.0;
    }
    double spacing1 = m_arch->getCellSpacing( ndl, ndl );
    if( !checkPinsOnCells )
    {
        return spacing1;
    }
    double spacing2 = 0.0;
    {
        Pin* pinl = 0;
        Pin* pinr = 0;

        // Right-most pin on the left cell.
        for( int i = ndl->m_firstPin; i < ndl->m_lastPin; i++ )
        {
            Pin* pin = m_network->m_nodePins[i];
            if( pinl == 0 || pin->m_offsetX > pinl->m_offsetX )
            {
                pinl = pin;
            }
        }

        // Left-most pin on the right cell.
        for( int i = ndr->m_firstPin; i < ndr->m_lastPin; i++ )
        {
            Pin* pin = m_network->m_nodePins[i];
            if( pinr == 0 || pin->m_offsetX < pinr->m_offsetX )
            {
                pinr = pin;
            }
        }
        // If pins on the same layer, do something.
        if( pinl != 0 && pinr != 0 && pinl->m_pinLayer == pinr->m_pinLayer )
        {
            // Determine the spacing requirements between these two pins.   Then, translate this into
            // a spacing requirement between the two cells.  XXX: Since it is implicit that the cells
            // are in the same row, we can determine the widest pin and the parallel run length 
            // without knowing the actual location of the cells...  At least I think so...

            double xmin1 = pinl->m_offsetX - 0.5 * pinl->m_pinW;
            double xmax1 = pinl->m_offsetX + 0.5 * pinl->m_pinW;
            double ymin1 = pinl->m_offsetY - 0.5 * pinl->m_pinH;
            double ymax1 = pinl->m_offsetY + 0.5 * pinl->m_pinH;

            double xmin2 = pinr->m_offsetX - 0.5 * pinr->m_pinW;
            double xmax2 = pinr->m_offsetX + 0.5 * pinr->m_pinW;
            double ymin2 = pinr->m_offsetY - 0.5 * pinr->m_pinH;
            double ymax2 = pinr->m_offsetY + 0.5 * pinr->m_pinH;

            double ww = std::max( std::min( ymax1-ymin1, xmax1-xmin1 ), std::min( ymax2-ymin2, xmax2-xmin2 ) );
            double py = std::max( 0.0, std::min( ymax1, ymax2 ) - std::max( ymin1, ymin2 ) );

            spacing2  = m_rt->get_spacing( pinl->m_pinLayer, ww, py );
            double gapl = (+0.5 * ndl->getWidth()) - xmax1;
            double gapr = xmin2 - (-0.5 * ndr->getWidth());
            spacing2 = std::max( 0.0, spacing2 - gapl - gapr );

            if( spacing2 > spacing1 )
            {
                // The spacing requirement due to the routing layer is larger than the spacing
                // requirement due to the edge constraint.  Interesting.
                ;
            }
        }
    }
    return std::max( spacing1, spacing2 );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::getSpaceAroundCell( int seg, int ix, double& space, double& larger, int limit )
{
    // XXX: UPDATE TO ACCOMMODATE MULTI-HEIGHT CELLS.  Likely requires using the bottom of the cell
    // instead of the center of the cell.  Need to assign a cell to multiple segments.

    Node* ndi = m_cellsInSeg[seg][ix];
    Node* ndj = 0;
    Node* ndk = 0;

    int n = m_cellsInSeg[seg].size();
    double xmin = m_segments[seg]->m_xmin;
    double xmax = m_segments[seg]->m_xmax;

    // Space to the immediate left and right of the cell.
    double space_left = 0;
    if( ix == 0 )
    {
        space_left += (ndi->getX() - 0.5 * ndi->getWidth()) - xmin;
    }
    else
    {
        --ix;
        ndj = m_cellsInSeg[seg][ix];

        space_left += (ndi->getX() - 0.5 * ndi->getWidth()) - (ndj->getX() + 0.5 * ndj->getWidth());
        ++ix;
    }

    double space_right = 0;
    if( ix == n-1 )
    {
        space_right += xmax - (ndi->getX() + 0.5 * ndi->getWidth());
    }
    else
    {
        ++ix;
        ndj = m_cellsInSeg[seg][ix];
        space_right += (ndj->getX() - 0.5 * ndj->getWidth()) - (ndi->getX() + 0.5 * ndi->getWidth());
    }
    space = space_left + space_right;

    // Space three cells 'limit' cells to the left and 'limit' cells to the right.
    if( ix < limit )
    {
        ndj = m_cellsInSeg[seg][0];
        larger = ndj->getX() - 0.5 * ndj->getWidth() - xmin;
    }
    else
    {
        larger = 0;
    }
    for( int j = std::max(0,ix-limit); j <= std::min(n-1,ix+limit); j++ )
    {
        ndj = m_cellsInSeg[seg][j];
        if( j < n-1 )
        {
            ndk = m_cellsInSeg[seg][j+1];
            larger += (ndk->getX() - 0.5 * ndk->getWidth()) - (ndj->getX() + 0.5 * ndj->getWidth());
        }
        else
        {
            larger += xmax - (ndj->getX() + 0.5 * ndj->getWidth());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::getSpaceAroundCell( int seg, int ix, double& space_left, double& space_right,
        double& large_left, double& large_right, int limit )
{
    // XXX: UPDATE TO ACCOMMODATE MULTI-HEIGHT CELLS.  Likely requires using the bottom of the cell
    // instead of the center of the cell.  Need to assign a cell to multiple segments.

    Node* ndi = m_cellsInSeg[seg][ix];
    Node* ndj = 0;
    Node* ndk = 0;

    int n = m_cellsInSeg[seg].size();
    double xmin = m_segments[seg]->m_xmin;
    double xmax = m_segments[seg]->m_xmax;

    // Space to the immediate left and right of the cell.
    space_left = 0;
    if( ix == 0 )
    {
        space_left += (ndi->getX() - 0.5 * ndi->getWidth()) - xmin;
    }
    else
    {
        --ix;
        ndj = m_cellsInSeg[seg][ix];
        space_left += (ndi->getX() - 0.5 * ndi->getWidth()) - (ndj->getX() + 0.5 * ndj->getWidth());
        ++ix;
    }

    space_right = 0;
    if( ix == n-1 )
    {
        space_right += xmax - (ndi->getX() + 0.5 * ndi->getWidth());
    }
    else
    {
        ++ix;
        ndj = m_cellsInSeg[seg][ix];
        space_right += (ndj->getX() - 0.5 * ndj->getWidth()) - (ndi->getX() + 0.5 * ndi->getWidth());
    }
    // Space three cells 'limit' cells to the left and 'limit' cells to the right.
    large_left = 0;
    if( ix < limit )
    {
        ndj = m_cellsInSeg[seg][0];
        large_left = ndj->getX() - 0.5 * ndj->getWidth() - xmin;
    }
    for( int j = std::max(0,ix-limit); j < ix; j++ )
    {
        ndj = m_cellsInSeg[seg][j];
        ndk = m_cellsInSeg[seg][j+1];
        large_left += (ndk->getX() - 0.5 * ndk->getWidth()) - (ndj->getX() + 0.5 * ndj->getWidth());
    }
    large_right = 0;
    for( int j = ix; j <= std::min(n-1,ix+limit); j++ )
    {
        ndj = m_cellsInSeg[seg][j];
        if( j < n-1 )
        {
            ndk = m_cellsInSeg[seg][j+1];
            large_right += (ndk->getX() - 0.5 * ndk->getWidth()) - (ndj->getX() + 0.5 * ndj->getWidth());
        }
        else
        {
            large_right += xmax - (ndj->getX() + 0.5 * ndj->getWidth());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::findRegionIntervals( int regId, std::vector<std::vector<std::pair<double,double> > >& intervals )
{
    // Find intervals within each row that are spanned by the specified region.
    // We ignore the default region 0, since it is "everywhere".


    if( regId < 1 || regId >= m_arch->m_regions.size() )
    {
        std::cout << "Error." << std::endl;
        exit(-1);
    }


    // Initialize.
    intervals.clear();
    intervals.resize( m_numSingleHeightRows );
    for( int i = 0; i < intervals.size(); i++ )
    {
        intervals[i] = std::vector<std::pair<double,double> >();
    }


    Architecture::Region* regPtr = m_arch->m_regions[regId];
    if( regPtr->m_id != regId )
    {
        std::cout << "Error." << std::endl;
        exit(-1);
    }


    // Look at the rectangles within the region.
    std::vector<Rectangle>& rects = regPtr->m_rects;
    for( int b = 0; b  < rects.size(); b++ )
    {
        double xmin = rects[b].m_xmin;
        double xmax = rects[b].m_xmax;
        double ymin = rects[b].m_ymin;
        double ymax = rects[b].m_ymax;

        for( int r = 0; r < m_numSingleHeightRows; r++ )
        {
            double lb = m_arch->m_ymin + r * m_singleRowHeight;
            double ub = lb + m_singleRowHeight;

            if( ymax >= ub && ymin <= lb )
            {
                // Blockage overlaps with the entire row span in the Y-dir... Sites
                // are possibly completely covered!  

                double originX = m_arch->m_rows[r]->m_subRowOrigin;
                double siteSpacing = m_arch->m_rows[r]->m_siteSpacing;

                int i0 = (int)std::floor((xmin-originX)/siteSpacing);
                int i1 = (int)std::floor((xmax-originX)/siteSpacing);
                if( originX + i1 * siteSpacing != xmax )
                    ++i1;

                if( i1 > i0 )
                {
                    intervals[r].push_back( std::pair<double,double>(originX + i0 * siteSpacing, originX + i1 * siteSpacing) );
                }
            }
        }
    }


    // Sort intervals and merge.  We merge, since the region might have been defined
    // with rectangles that touch (so it is "wrong" to create an artificial boundary).
    for( int r = 0; r < m_numSingleHeightRows; r++ )
    {
        if( intervals[r].size() == 0 )
        {
            continue;
        }

        // Sort to get intervals left to right.  
        std::sort( intervals[r].begin(), intervals[r].end(), compareBlockages() );

        std::stack< std::pair<double,double> > s;
        s.push( intervals[r][0] );
        for( int i = 1; i < intervals[r].size(); i++ )
        {
            std::pair<double,double> top = s.top(); // copy.
            if( top.second < intervals[r][i].first )
            {
                s.push( intervals[r][i] ); // new interval.
            }
            else
            {
                if( top.second < intervals[r][i].second )
                {
                    top.second = intervals[r][i].second; // extend interval.
                }
                s.pop(); // remove old.
                s.push( top ); // expanded interval.
            }
        }

        intervals[r].erase( intervals[r].begin(), intervals[r].end() );
        while( !s.empty() )
        {
            std::pair<double,double> temp = s.top(); // copy.
            intervals[r].push_back( temp );
            s.pop();
        }

        // Sort to get them left to right.
        std::sort( intervals[r].begin(), intervals[r].end(), compareBlockages() ); 
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeSegmentOverlapSingle( int regId )
{
    // Loops over the segments.  Finds intervals of single height cells and 
    // attempts to do a min shift to remove overlap.
    
    for( size_t s = 0; s < this->m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = this->m_segments[s];

        if( !(segPtr->m_regId == regId || regId == -1) )
        {
            continue;
        }

        int segId = segPtr->m_segId;
        int rowId = segPtr->m_rowId;

        double rowy = m_arch->m_rows[rowId]->getY() + 0.5 * m_arch->m_rows[rowId]->getH();
        double left = segPtr->m_xmin;
        double rite = segPtr->m_xmax;


//        std::cout << "Trying to remove overlap in segment " << segId << ", "
//            << "[" << left << "," << rite << "]" << ", "
//            << segPtr->m_util << ", " << (rite-left) << std::endl;

        std::vector<Node*> nodes;
        for( size_t n = 0; n < this->m_cellsInSeg[segId].size(); n++ )
        {
            Node* ndi = this->m_cellsInSeg[segId][n];

            int spanned = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);
            if( spanned == 1 )
            {
                ndi->setY( rowy );
            }

            if( spanned == 1 )
            {
                nodes.push_back( ndi );
            }
            else
            {
                // Multi-height.
                if( nodes.size() == 0 )
                {
                    left = ndi->getX() + 0.5 * ndi->getWidth();
                }
                else
                {
                    rite = ndi->getX() - 0.5 * ndi->getWidth();
                    // solve.
                    removeSegmentOverlapSingleInner( nodes, left, rite, rowId );
                    // prepare for next.
                    nodes.erase( nodes.begin(), nodes.end() );
                    left = ndi->getX() + 0.5 * ndi->getWidth();
                    rite = segPtr->m_xmax;
                }
            }
        }
        if( nodes.size() != 0 )
        {
            // solve.
            removeSegmentOverlapSingleInner( nodes, left, rite, rowId );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeSegmentOverlapSingleInner( 
    std::vector<Node*>& nodes_in, double xmin, double xmax, int rowId )
{
    // Quickly remove overlap between a range of cells in a segment.  Try to
    // satisfy gaps and try to align with sites.

//    {
//        double tot = 0.;
//        for( size_t i = 0; i < nodes_in.size(); i++ )
//        {
//            tot += nodes_in[i]->getWidth();
//        }
//        std::cout << "Span of " << nodes_in.size() << ", "
//            << "Total node width is " << tot << ", "
//            << "Space is " << xmax-xmin << ", "
//            << "From " << xmin << ", To " << xmax;
//        if( tot > xmax-xmin )
//        {
//            std::cout << "***";
//        }
//        std::cout << std::endl;
//    }

    std::vector<Node*> nodes = nodes_in;
    std::stable_sort( nodes.begin(), nodes.end(),  compareNodesX() );

    std::vector<double> llx;
    std::vector<double> tmp;
    std::vector<double> wid;

    llx.resize( nodes.size() );
    tmp.resize( nodes.size() );
    wid.resize( nodes.size() );

    double x;
    double originX = m_arch->m_rows[rowId]->m_subRowOrigin;
    double siteSpacing = m_arch->m_rows[rowId]->m_siteSpacing;
    int ix;

    double space = xmax-xmin;
    double util = 0.;
    double gapu = 0.;

    for( int k = 0; k < nodes.size(); k++ )
    {
        util += nodes[k]->getWidth();
    }
    for( int k = 1; k < nodes.size(); k++ )
    {
        gapu += m_arch->getCellSpacing( nodes[k-1], nodes[k-0] );
    }


    // Get width for each cell.
    for( int i = 0; i < nodes.size(); i++ )
    {
        Node* nd = nodes[i];
        wid[i]   = nd->getWidth();
    }

    // Try to get site alignment.  Adjust the left and right into which
    // we are placing cells.  If we don't need to shrink cells, then I
    // think this should work.
    {
        bool adjusted = false;
        double tot = 0;
        for( int i = 0; i < nodes.size(); i++ )
        {
            tot += wid[i];
        }
        if( space > tot )
        {
            // Try to fix the left boundary.
            ix = (int)(((xmin) - originX)/siteSpacing);
            if( originX + ix*siteSpacing < xmin )
                ++ix;
            x = originX + ix*siteSpacing;
            if( xmax - x >= tot+1.0e-3 && std::fabs( xmin - x ) > 1.0e-3 )
            {
                adjusted = true;
                xmin = x;
            }
            space = xmax - xmin;
        }
        if( space > tot )
        {
            // Try to fix the right boundary.
            ix = (int)(((xmax) - originX)/siteSpacing);
            if( originX + ix*siteSpacing > xmax )
                --ix;
            x = originX + ix*siteSpacing;
            if( x - xmin >= tot+1.0e-3 && std::fabs( xmax - x ) > 1.0e-3 )
            {
                adjusted = true;
                xmax = x;
            }
            space = xmax - xmin;
        }

        if( adjusted )
        {
//            std::cout << " => Boundaries adjusted, "
//                << "From " << xmin << ", To " << xmax << ", Space is now " << space << std::endl;
        }
    }



    // Try to include the necessary gap as long as we don't exceed the
    // available space.
    for( int i = 1; i < nodes.size(); i++ )
    {
        Node* ndl = nodes[i-1];
        Node* ndr = nodes[i-0];

        double gap = m_arch->getCellSpacing( ndl, ndr );
        if( gap != 0.0 )
        {
            if( util + gap <= space )
            {
                wid[i-1] += gap;
                util += gap;
            }
        }
    }

    double cell_width = 0.;
    for( int i = 0; i < nodes.size(); i++ )
    {
        cell_width += wid[i];
    }
    if( cell_width > space )
    {
        // Scale... now things should fit, but will overlap...
        double scale = space / cell_width;
        for( int i = 0; i < nodes.size(); i++ )
        {
            wid[i] *= scale;
        }
    }

    // The position for the left edge of each cell; this position will be 
    // site aligned.
    for( int i = 0; i  < nodes.size(); i++ )
    {
        Node* nd = nodes[i];
        ix = (int)(((nd->getX() - 0.5*nd->getWidth()) - originX)/siteSpacing);
        llx[i] = originX + ix * siteSpacing;
    }
    // The leftmost position for the left edge of each cell.  Should also
    // be site aligned unless we had to shrink cell widths to make things
    // fit (which means overlap anyway).
    //ix = (int)(((xmin) - originX)/siteSpacing);
    //if( originX + ix*siteSpacing < xmin )
    //    ++ix;
    //x = originX + ix * siteSpacing;
    x = xmin;
    for( int i = 0; i < nodes.size(); i++ )
    {
        tmp[i] = x;
        x += wid[i];
    }

    // The rightmost position for the left edge of each cell.  Should also
    // be site aligned unless we had to shrink cell widths to make things 
    // fit (which means overlap anyway).
    //ix = (int)(((xmax) - originX)/siteSpacing);
    //if( originX + ix*siteSpacing > xmax )
    //    --ix;
    //x = originX + ix * siteSpacing;
    x = xmax;
    for( int i = nodes.size()-1; i >= 0; i-- )
    {
        llx[i] = std::max( tmp[i], std::min( x-wid[i], llx[i] ) );
        x = llx[i]; // Update rightmost position.
    }
    for( int i = 0; i < nodes.size(); i++ )
    {
        nodes[i]->setX( llx[i] + 0.5 * nodes[i]->getWidth() );
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::resortSegments( void )
{
    // Resort the nodes in the segments.  This might be required if we did 
    // something to move cells around and broke the ordering.
    for( size_t i = 0; i < m_segments.size(); i++ )
    {
        DetailedSeg* segPtr = m_segments[i];
        resortSegment( segPtr );
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::resortSegment( DetailedSeg* segPtr )
{
    int segId = segPtr->m_segId;
    std::stable_sort( m_cellsInSeg[segId].begin(), m_cellsInSeg[segId].end(),  compareNodesX() );
    segPtr->m_util = 0.;
    for( size_t n = 0; n < m_cellsInSeg[segId].size(); n++ )
    {
        Node* ndi = m_cellsInSeg[segId][n];
        segPtr->m_util += ndi->getWidth();
    }
    segPtr->m_gapu = 0.;
    for( size_t n = 1; n < m_cellsInSeg[segId].size(); n++ )
    {
        Node* ndl = m_cellsInSeg[segId][n-1];
        Node* ndr = m_cellsInSeg[segId][n];
        segPtr->m_gapu += m_arch->getCellSpacing( ndl, ndr );
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeAllCellsFromSegments( void )
{
    // This routine removes _ALL_ cells from all segments.  It clears all
    // reverse maps and so forth.  Basically, it leaves things as if the
    // segments have all been created, but nothing has been inserted.
    for( size_t i = 0; i < m_segments.size(); i++ )
    {
        DetailedSeg* segPtr = m_segments[i];
        int segId = segPtr->m_segId;
        m_cellsInSeg[segId].erase( m_cellsInSeg[segId].begin(), m_cellsInSeg[segId].end() );
        segPtr->m_util = 0.;
        segPtr->m_gapu = 0.;
    }
    for( size_t i = 0; i < m_reverseCellToSegs.size(); i++ )
    {
        m_reverseCellToSegs[i].erase( 
                m_reverseCellToSegs[i].begin(), m_reverseCellToSegs[i].end() );
    }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct compare_node_segment_x
{
    inline bool operator()( Node*& s, double i ) const {
        return s->getX() < i;
    }
    inline bool operator()( double i, Node*& s ) const {
        return i < s->getX();
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::alignPos( Node* ndi, double& xi, double xl, double xr )
{
    // Given a cell with a target, determine a close site aligned position
    // such that the cell falls entirely within [xl,xr].

    double originX      = m_arch->m_rows[0]->m_subRowOrigin;
    double siteSpacing  = m_arch->m_rows[0]->m_siteSpacing;

    double xp;
    double w = ndi->getWidth();
    int ix;

    // Work with left edge.
    xr -= w; // [xl,xr] is now range for left edge of cell.

    // Left edge of cell within [xl,xr] closest to target.
    xp  = std::max( xl, std::min( xr, xi-0.5*w ) ); 

    ix = (int) ( (xp - originX) / siteSpacing + 0.5 );
    xp = originX + ix * siteSpacing; // Left edge aligned.

    if( xp <= xl-1.0e-3 )       { xp += siteSpacing; }
    else if( xp >= xr+1.0e-3 )  { xp -= siteSpacing; }

    if( xp <= xl-1.0e-3 || xp >= xr+1.0e-3 )
    {
        return false;
    }

    // Set new target.
    xi = xp + 0.5*w;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
DetailedMgr::shift( 
    std::vector<Node*>& cells,
    std::vector<double>& tarX,
    std::vector<double>& posX,
    double left, double right,
    int segId, int rowId
    )
{
    // Given a lis of _ordered_ cells with targets, position the cells
    // within the provided boundaries while observing spacing.
    //
    // XXX: Need to pre-allocate a maximum size for the problem to
    // avoid constant reallocation.

    double originX = m_arch->m_rows[rowId]->m_subRowOrigin;
    double siteSpacing = m_arch->m_rows[rowId]->m_siteSpacing;
    double siteWidth = m_arch->m_rows[rowId]->m_siteWidth;
    int numSites = m_arch->m_rows[rowId]->m_numSites;
    (void)numSites;

    // Number of cells.
    int ncells = cells.size();

    // Sites within the provided range.
    int i0 = (int)((left - originX)/siteSpacing);
    if( originX + i0 * siteSpacing < left )
    {
        ++i0;
    }
    int i1 = (int)((right - originX)/siteSpacing);
    if( originX + i1 * siteSpacing + siteWidth >= right+1.0e-3 )
    {
        --i1;
    }
    int nsites = i1-i0+1;
    int ii, jj;

    // Get cell widths while accounting for spacing/padding.  We
    // ignore spacing/padding at the ends (should adjust left 
    // and right edges prior to calling).  Convert spacing into
    // number of sites.
    // Change cell widths to be in terms of number of sites.
    std::vector<int> swid;
    swid.resize( ncells );
    std::fill( swid.begin(), swid.end(), 0 );
    int rsites = 0;
    for( int i = 0; i < ncells; i++ )
    {
        Node* ndl = (i == 0) ? 0 : cells[i-1];
        Node* ndr = (i == ncells-1) ? 0 : cells[i+1];
        Node* ndi = cells[i]; 

        // XXX: If I split the gap, I'm likely going to get a round up
        // problem and require more space than is really needed.  I 
        // should be smarter about this and perhaps assign all width 
        // to the left...  Reconsider...
        double width = ndi->getWidth();
        if( ndl != 0 ) width += 0.5 * m_arch->getCellSpacing( ndl, ndi );
        if( ndr != 0 ) width += 0.5 * m_arch->getCellSpacing( ndi, ndr );

        swid[i] = (int) std::ceil( width / siteSpacing );  // or siteWidth???
        rsites += swid[i];
    }
    if( rsites > nsites )
    {
        return false;
    }

    // Determine leftmost and rightmost site for each cell.
    std::vector<int> site_l, site_r;
    site_l.resize( ncells );
    site_r.resize( ncells );
    int k = i0;
    for( int i = 0; i < ncells; i++ )
    {
        site_l[i] = k;
        k += swid[i];
    }
    k = i1+1;
    for( int i = ncells-1; i >= 0; i-- )
    {
        site_r[i] = k-swid[i];
        k = site_r[i];
        if( site_r[i] < site_l[i] )
        {
            return false;
        }
    }

    //std::cout << boost::format( "Left %6lf, Right %6lf" ) % left % right << std::endl;
    //for( int i = 0; i < ncells; i++ )
    //{
    //    Node* ndl = (i == 0) ? 0 : cells[i-1];
    //    Node* ndr = (i == ncells-1) ? 0 : cells[i+1];
    //    Node* ndi = cells[i]; 
    //
    //    double width = ndi->getWidth();
    //    if( ndl != 0 ) width += 0.5 * m_arch->getCellSpacing( ndl, ndi );
    //    if( ndr != 0 ) width += 0.5 * m_arch->getCellSpacing( ndi, ndr );
    //
    //    std::cout << boost::format( "Cell %6d, width %6lf, sites %3d (%5d,%5d), Target %lf" )
    //        % ndi->getId() % width % swid[i] % site_l[i] % site_r[i] % tarX[i]
    //        << std::endl;
    //}
    //std::cout << boost::format( "Sites %4d, [%4d,%4d]" ) % nsites % i0 % i1 << std::endl;

    // Create tables.
    std::vector<std::vector<std::pair<int,int> > > prev;
    std::vector<std::vector<double> > tcost;
    std::vector<std::vector<double> > cost;
    tcost.resize( nsites+1 );
    prev.resize( nsites+1 );
    cost.resize( nsites+1 );
    for( size_t i = 0; i <= nsites; i++ )
    {
        tcost[i].resize( ncells+1 );
        prev[i].resize( ncells+1 );
        cost[i].resize( ncells+1 );

        std::fill( tcost[i].begin(), tcost[i].end(), std::numeric_limits<double>::max() );
        std::fill( prev[i].begin(), prev[i].end(), std::make_pair(-1,-1) );
        std::fill( cost[i].begin(), cost[i].end(), 0.0 );
    }

    // Fill in costs of cells to sites.
    for( int j = 1; j <= ncells; j++ )
    {
        Node* ndi = cells[j-1];

        // Skip invalid sites.
        for( int i = 1; i <= nsites; i++ )
        {
            // Cell will cover real sites from [site_id,site_id+width-1].

            int site_id = i0 + i - 1; 
            if( site_id < site_l[j-1] || site_id > site_r[j-1] )
            {
                continue;
            }

            // Figure out cell position if cell aligned to current site.
            int x = originX + site_id * siteSpacing + 0.5 * ndi->getWidth(); 
            cost[i][j] = std::fabs( x - tarX[j-1] );
        }
    }

    // Fill in total costs.
    tcost[0][0] = 0.;
    for( int j = 1; j <= ncells; j++ )
    {
        // Width info; for indexing.
        int prev_wid = (j-1 == 0) ? 1 : swid[j-2];
        int curr_wid = swid[j-1];

        for( int i = 1; i <= nsites; i++ )
        {
            // Current site is site_id and covers [site_id,site_id+width-1].
            int site_id = i0 + i - 1;  

            // Cost if site skipped.
            ii = i-1;
            jj = j;
            {
                double c = tcost[ii][jj];
                if( c < tcost[i][j] )
                {
                    tcost[i][j] = c;
                    prev[i][j] = std::make_pair(ii,jj);
                }
            }

            // Cost if site used; avoid if invalid (too far left or right).
            ii = i - prev_wid;
            jj = j-1;
            if( !(ii < 0 || site_id + curr_wid - 1 > i1) )
            {
                double c = tcost[ii][jj] + cost[i][j]; 
                if( c < tcost[i][j] )
                {
                    tcost[i][j] = c;
                    prev[i][j] = std::make_pair(ii,jj);
                }
            }
        }
    }

    // Test.
    {
        bool okay = false;
        std::pair<int,int> curr = std::make_pair(nsites,ncells);
        while( curr.first != -1 && curr.second != -1 )
        {
            if( curr.first == 0 && curr.second == 0 )
            {
                okay = true;
            }
            curr = prev[curr.first][curr.second];
        }
        if( !okay )
        {
            // Odd.  Should not fail.
            return false;
        }
    }

    // Determine placement.
    {
        std::pair<int,int> curr = std::make_pair(nsites,ncells);
        while( curr.first != -1 && curr.second != -1 )
        {
            if( curr.first == 0 && curr.second == 0 )
            {
                break;
            }
            int curr_i = curr.first ; // Site.
            int curr_j = curr.second; // Cell.

            if( curr_j != prev[curr_i][curr_j].second )
            {
                // We've placed the cell at the site.
                Node* ndi = cells[curr_j-1];

                int ix = i0 + curr_i - 1;
                posX[curr_j-1] = originX + ix * siteSpacing + 0.5 * ndi->getWidth();

                //std::cout << boost::format( "Cell %6d assigned to site %4d/%4d, Span [%6lf,%6lf]" )
                //    % ndi->getId() % ix % curr_i 
                //    % (originX + ix * siteSpacing) % (originX + ix * siteSpacing + ndi->getWidth())
                //    << std::endl;
            }

            curr = prev[curr_i][curr_j];
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::tryMove1( Node* ndi, double xi, double yi, int si, double xj, double yj, int sj )
{
    // Try to move a single height cell from its current position in a new segment.

    int ri = m_segments[si]->m_rowId;
    int rj = m_segments[sj]->m_rowId;


    double row_y = m_arch->m_rows[rj]->getY() + 0.5 * m_arch->m_rows[rj]->getH();
    if( std::fabs( yj - row_y ) >= 1.0e-3 )
    {
        yj = row_y;
    }

    m_nMoved = 0;

    if( sj == si )
    {
        // Same segment.
        return false;
    }
    if( sj == -1 )
    {
        // Huh?
        return false;
    }
    if( ndi->getRegionId() != m_segments[sj]->getRegId() )
    {
        // Region compatibility.
        return false;
    }

    int spanned = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);
    if( spanned != 1 )
    {
        // Multi-height cell.
        return false;
    }

    double util, gapu, width, change;
    double x1, x2;
    int ix, n, site_id;
    double originX      = m_arch->m_rows[rj]->m_subRowOrigin;
    double siteSpacing  = m_arch->m_rows[rj]->m_siteSpacing;
    std::vector<Node*>::iterator it;


    // Find the cells to the left and to the right of the target location.
    Node* ndr   =  0;
    Node* ndl   =  0;
    if( m_cellsInSeg[sj].size() != 0 )
    {
        it = std::lower_bound( m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), xj, compare_node_segment_x() );

        if( it == m_cellsInSeg[sj].end() )
        {
            // Nothing to the right of the target position.  But, there must be something to the left
            // since we know the segment is not empty.
            ndl = m_cellsInSeg[sj].back();
        }
        else
        {
            ndr = *it;
            if( it != m_cellsInSeg[sj].begin() )
            {
                --it;
                ndl = *it;
            }
        }
    }

    // Different cases depending on the presences of cells on the left and the right.
    if( ndl == 0 && ndr == 0 )
    {
        // No left or right cell implies an empty segment.

        if( m_cellsInSeg[sj].size() != 0 )
        {
            return false;
        }

        // Still need to check cell spacing in case of padding.
        DetailedSeg* seg_j = m_segments[sj];
        width = seg_j->m_xmax - seg_j->m_xmin;
        util = seg_j->m_util;
        gapu = seg_j->m_gapu;

        change  = ndi->getWidth();
        change += m_arch->getCellSpacing( 0, ndi );
        change += m_arch->getCellSpacing( ndi, 0 );

        if( util + gapu + change > width )
        {
            return false;
        }
 
        x1 = m_segments[sj]->m_xmin + m_arch->getCellSpacing( 0, ndi );
        x2 = m_segments[sj]->m_xmax - m_arch->getCellSpacing( ndi, 0 );
        if( !alignPos( ndi, xj, x1, x2 ) )
        {
            return false;
        }

        if( m_nMoved >= m_moveLimit )
        {
            return false;
        }
        m_movedNodes[m_nMoved]    = ndi;
        m_curX[m_nMoved]          = ndi->getX();
        m_curY[m_nMoved]          = ndi->getY();
        m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( si );
        m_newX[m_nMoved]          = xj;
        m_newY[m_nMoved]          = yj;
        m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( sj );
        ++m_nMoved;

        return true;
    }
    else if( ndl != 0 && ndr == 0 )
    {
        // End of segment...

        DetailedSeg* seg_j = m_segments[sj];
        width = seg_j->m_xmax - seg_j->m_xmin;
        util = seg_j->m_util;
        gapu = seg_j->m_gapu;

        // Determine required space and see if segment is violated.
        change  = ndi->getWidth(); 
        change += m_arch->getCellSpacing( ndl, ndi );
        change += m_arch->getCellSpacing( ndi, 0 );

        if( util + gapu + change > width )
        {
            return false;
        }

        x1 = (ndl->getX() + 0.5 * ndl->getWidth()) + m_arch->getCellSpacing( ndl, ndi );
        x2 = m_segments[sj]->m_xmax - m_arch->getCellSpacing( ndi, 0 );
        if( !alignPos( ndi, xj, x1, x2 ) )
        {
            return false;
        }

        if( m_nMoved >= m_moveLimit )
        {
            return false;
        }
        m_movedNodes[m_nMoved]    = ndi;
        m_curX[m_nMoved]          = ndi->getX();
        m_curY[m_nMoved]          = ndi->getY();
        m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( si );
        m_newX[m_nMoved]          = xj;
        m_newY[m_nMoved]          = yj;
        m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( sj );
        ++m_nMoved;

        // Shift cells left if necessary.
        it = std::find( m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), ndl );
        if( m_cellsInSeg[sj].end() == it )
        {
            // Error.
            return false;
        }
        ix = it - m_cellsInSeg[sj].begin();
        n = 0;
        while( ix >= n && 
            (ndl->getX()+0.5*ndl->getWidth()+m_arch->getCellSpacing(ndl,ndi) > xj-0.5*ndi->getWidth()) )
        {
            // Cell ndl to the left of ndi overlaps due to placement of ndi.
            // Therefore, we need to shift ndl.  If we cannot, then just fail.
            spanned = (int)(ndl->getHeight() / m_singleRowHeight + 0.5);
            if( spanned != 1 )
            {
                return false;
            }

            xj -= 0.5 * ndi->getWidth();
            xj -= m_arch->getCellSpacing( ndl, ndi );
            xj -= 0.5 * ndl->getWidth();

            // Site alignment.
            site_id = (int) std::floor( ((xj - 0.5 * ndl->getWidth()) - originX) / siteSpacing );
            if( xj > originX + site_id * siteSpacing + 0.5 * ndl->getWidth() )
            {
                xj = originX + site_id * siteSpacing + 0.5 * ndl->getWidth();
            }

            if( m_nMoved >= m_moveLimit )
            {
                return false;
            }
            m_movedNodes[m_nMoved]    = ndl;
            m_curX[m_nMoved]          = ndl->getX();
            m_curY[m_nMoved]          = ndl->getY();
            m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( sj );
            m_newX[m_nMoved]          = xj;
            m_newY[m_nMoved]          = ndl->getY();
            m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( sj );
            ++m_nMoved;

            // Fail if we shift off the end of a segment.
            if( xj-0.5*ndl->getWidth()-m_arch->getCellSpacing( 0, ndl ) < m_segments[sj]->m_xmin )
            {
                return false;
            }
            if( ix == n )
            {
                // We shifted down to the last cell... Everything must be okay!
                break;
            }

            ndi = ndl;
            ndl = m_cellsInSeg[sj][--ix];
        }

        return true;
    }
    else if( ndl == 0 && ndr != 0 )
    {
        // Start of segment...

        DetailedSeg* seg_j = m_segments[sj];
        width = seg_j->m_xmax - seg_j->m_xmin;
        util = seg_j->m_util;
        gapu = seg_j->m_gapu;

        // Determine required space and see if segment is violated.
        change = 0.0;
        change += ndi->getWidth();
        change += m_arch->getCellSpacing( ndi, ndr );

        if( util + gapu + change > width )
        {
            return false;
        }

        x1 = m_segments[sj]->m_xmin;
        x2 = (ndr->getX() - 0.5 * ndr->getWidth()) - m_arch->getCellSpacing( ndi, ndr );
        if( !alignPos( ndi, xj, x1, x2 ) )
        {
            return false;
        }

        if( m_nMoved >= m_moveLimit )
        {
            return false;
        }

        m_movedNodes[m_nMoved]    = ndi;
        m_curX[m_nMoved]          = ndi->getX();
        m_curY[m_nMoved]          = ndi->getY();
        m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( si );
        m_newX[m_nMoved]          = xj;
        m_newY[m_nMoved]          = yj;
        m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( sj );
        ++m_nMoved;

        // Shift cells right if necessary...
        it = std::find( m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), ndr );
        if( m_cellsInSeg[sj].end() == it )
        {
            // Error.
            return false;
        }
        ix = it - m_cellsInSeg[sj].begin();
        n = m_cellsInSeg[sj].size()-1;
        while( ix <= n && ndr->getX()-0.5*ndr->getWidth()-m_arch->getCellSpacing(ndi,ndr) < xj+0.5*ndi->getWidth() )
        {
            // Cell ndr to the right of ndi overlaps due to placement of ndi.
            // Therefore, we need to shift ndr.  If we cannot, then just fail.
            spanned = (int)(ndr->getHeight() / m_singleRowHeight + 0.5);
            if( spanned != 1 )
            {
                return false;
            }

            xj += 0.5 * ndi->getWidth();
            xj += m_arch->getCellSpacing( ndi, ndr );
            xj += 0.5 * ndr->getWidth();

            // Site alignment.
            site_id = (int) std::ceil( ((xj - 0.5 * ndr->getWidth()) - originX) / siteSpacing );
            if( xj < originX + site_id * siteSpacing + 0.5 * ndr->getWidth() )
            {
                xj = originX + site_id * siteSpacing + 0.5 * ndr->getWidth();
            }

            if( m_nMoved >= m_moveLimit )
            {
                return false;
            }
            m_movedNodes[m_nMoved]    = ndr;
            m_curX[m_nMoved]          = ndr->getX();
            m_curY[m_nMoved]          = ndr->getY();
            m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( sj );
            m_newX[m_nMoved]          = xj;
            m_newY[m_nMoved]          = ndr->getY();
            m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( sj );
            ++m_nMoved;

            // Fail if we shift off end of segment.
            if( xj+0.5*ndr->getWidth()+m_arch->getCellSpacing(ndr,0) > m_segments[sj]->m_xmax )
            {
                return false;
            }

            if( ix == n )
            {
                // We shifted down to the last cell... Everything must be okay!
                break;
            }

            ndi = ndr;
            ndr = m_cellsInSeg[sj][++ix];
        }

        //std::cout << "TryMove1: Found move: " << std::endl;
        //printMove();

        return true;
    }
    else if( ndl != 0 && ndr != 0 )
    {
        // Cell to both left and right of target position.

        // In middle of segment...

        DetailedSeg* seg_j = m_segments[sj];
        width = seg_j->m_xmax - seg_j->m_xmin;
        util = seg_j->m_util;
        gapu = seg_j->m_gapu;

        // Determine required space and see if segment is violated.
        change = 0.0;
        change += ndi->getWidth();
        change += m_arch->getCellSpacing( ndl, ndi );
        change += m_arch->getCellSpacing( ndi, ndr );
        change -= m_arch->getCellSpacing( ndl, ndr );

        if( util + gapu + change > width )
        {
            return false;
        }

        x1 = (ndl->getX() + 0.5 * ndl->getWidth()) + m_arch->getCellSpacing( ndl, ndi );
        x2 = (ndr->getX() - 0.5 * ndr->getWidth()) - m_arch->getCellSpacing( ndi, ndr );
        if( !alignPos( ndi, xj, x1, x2 ) )
        {
            return false;
        }

        if( m_nMoved >= m_moveLimit )
        {
            return false;
        }
        m_movedNodes[m_nMoved]    = ndi;
        m_curX[m_nMoved]          = ndi->getX();
        m_curY[m_nMoved]          = ndi->getY();
        m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( si );
        m_newX[m_nMoved]          = xj;
        m_newY[m_nMoved]          = yj;
        m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( sj );
        ++m_nMoved;

        // Shift cells right if necessary...
        it = std::find( m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), ndr );
        if( m_cellsInSeg[sj].end() == it )
        {
            // Error.
            return false;
        }
        ix = it - m_cellsInSeg[sj].begin();
        n = m_cellsInSeg[sj].size()-1;
        while( ix <= n && (ndr->getX()-0.5*ndr->getWidth()-m_arch->getCellSpacing(ndi,ndr) < xj+0.5*ndi->getWidth()) )
        {
            // If here, it means that the cell on the right overlaps with the new location
            // for the cell we are trying to move.  ==> THE RIGHT CELL NEEDS TO BE SHIFTED.
            // We cannot handle multi-height cells right now, so if this is a multi-height
            // cell, we return falure!
            spanned = (int)(ndr->getHeight() / m_singleRowHeight + 0.5);
            if( spanned != 1 )
            {
                return false;
            }

            xj += 0.5 * ndi->getWidth();
            xj += m_arch->getCellSpacing( ndi, ndr );
            xj += 0.5 * ndr->getWidth();

            // Site alignment.
            site_id = (int) std::ceil( ((xj - 0.5 * ndr->getWidth()) - originX) / siteSpacing );
            if( xj < originX + site_id * siteSpacing + 0.5 * ndr->getWidth() )
            {
                xj = originX + site_id * siteSpacing + 0.5 * ndr->getWidth();
            }

            if( m_nMoved >= m_moveLimit )
            {
                return false;
            }
            m_movedNodes[m_nMoved]    = ndr;
            m_curX[m_nMoved]          = ndr->getX();
            m_curY[m_nMoved]          = ndr->getY();
            m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( sj );
            m_newX[m_nMoved]          = xj;
            m_newY[m_nMoved]          = ndr->getY();
            m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( sj );
            ++m_nMoved;

            // Fail if we go off the end of the segment.
            if( xj+0.5*ndr->getWidth()+m_arch->getCellSpacing(ndr,0) > m_segments[sj]->m_xmax )
            {
                return false;
            }

            if( ix == n )
            {
                // We shifted down to the last cell... Everything must be okay!
                break;
            }

            ndi = ndr;
            ndr = m_cellsInSeg[sj][++ix];
        }

        // Shift cells left if necessary...
        ndi = m_movedNodes[0]; // Reset to the original cell.
        xj = m_newX[0];
        it = std::find( m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), ndl );
        if( m_cellsInSeg[sj].end() == it )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
        ix = it - m_cellsInSeg[sj].begin();
        n = 0;
        while( ix >= n && (ndl->getX()+0.5*ndl->getWidth()+m_arch->getCellSpacing(ndl,ndi) > xj-0.5*ndi->getWidth()) )
        {
            // If here, it means that the cell on the left overlaps with the new location
            // for the cell we are trying to move.  ==> THE LEFT CELL NEEDS TO BE SHIFTED.
            // We cannot handle multi-height cells right now, so if this is a multi-height
            // cell, we return falure!
            spanned = (int)(ndl->getHeight() / m_singleRowHeight + 0.5);
            if( spanned != 1 )
            {
                return false;
            }

            xj -= 0.5 * ndi->getWidth();
            xj -= m_arch->getCellSpacing( ndl, ndi );
            xj -= 0.5 * ndl->getWidth();

            // Site alignment.
            site_id = (int) std::floor( ((xj - 0.5 * ndl->getWidth()) - originX) / siteSpacing );
            if( xj > originX + site_id * siteSpacing + 0.5 * ndl->getWidth() )
            {
                xj = originX + site_id * siteSpacing + 0.5 * ndl->getWidth();
            }

            if( m_nMoved >= m_moveLimit )
            {
                return false;
            }
            m_movedNodes[m_nMoved]    = ndl;
            m_curX[m_nMoved]          = ndl->getX();
            m_curY[m_nMoved]          = ndl->getY();
            m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( sj );
            m_newX[m_nMoved]          = xj;
            m_newY[m_nMoved]          = ndl->getY();
            m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( sj );
            ++m_nMoved;

            // Fail if we go off the end of the segment.
            if( xj-0.5*ndl->getWidth()-m_arch->getCellSpacing(0,ndl) < m_segments[sj]->m_xmin )
            {
                return false;
            }
            if( ix == n )
            {
                // We shifted down to the last cell... Everything must be okay!
                break;
            }

            ndi = ndl;
            ndl = m_cellsInSeg[sj][--ix];
        }

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::tryMove2( Node* ndi, double xi, double yi, int si, double xj, double yj, int sj )
{
    // Very simple move within the same segment.

    // Nothing to move.
    m_nMoved = 0;

    if( sj != si )
    {
        return false;
    }

    int ri = m_segments[si]->m_rowId;
    int rj = m_segments[sj]->m_rowId;

    int nn = m_cellsInSeg[si].size()-1;

    double row_y = m_arch->m_rows[rj]->getY() + 0.5 * m_arch->m_rows[rj]->getH();
    if( std::fabs( yj - row_y ) >= 1.0e-3 )
    {
        yj = row_y;
    }

    std::vector<Node*>::iterator it_i;
    std::vector<Node*>::iterator it_j;
    int ix_i = -1;
    int ix_j = -1;
    (void)ix_i; (void)ix_j;

    double originX      = m_arch->m_rows[rj]->m_subRowOrigin;
    double siteSpacing  = m_arch->m_rows[rj]->m_siteSpacing;
    int site_id;
    double x1, x2, xl, xr;
    double space_left_j, space_right_j, large_left_j, large_right_j;

    // Find closest cell to the right of the target location.
    Node* ndj = 0;
    if( m_cellsInSeg[sj].size() != 0 )
    {
        it_j = std::lower_bound( m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), 
                    xj, compare_node_segment_x() );

        if( it_j == m_cellsInSeg[sj].end() )
        {
            // No node in segment with position greater than xj.  So, pick the last node.
            ndj = m_cellsInSeg[sj].back();
            ix_j = m_cellsInSeg[sj].size()-1;
        }
        else
        {
            ndj = *it_j;
            ix_j = it_j - m_cellsInSeg[sj].begin();
        }
    }
    // Not finding a cell is weird, since we should at least find the cell
    // we are attempting to move.
    if( ix_j == -1 || ndj == 0 )
    {
        return false;
    }

    // Note that it is fine if ndj is the same as ndi; we are just trying
    // to move to a new position adjacent to some block.

    // Space on each side of ndj.
    getSpaceAroundCell( sj, ix_j, space_left_j, space_right_j, large_left_j, large_right_j );

    it_i = std::find( m_cellsInSeg[si].begin(), m_cellsInSeg[si].end(), ndi );
    ix_i = it_i - m_cellsInSeg[si].begin();

    if( ndi->getWidth() <= space_left_j )
    {
        // Might fit on the left, depending on spacing.
        Node* prev = (ix_j ==  0) ? 0 : m_cellsInSeg[sj][ix_j-1];
        x1 = ndj->getX() - 0.5 * ndj->getWidth() - space_left_j + m_arch->getCellSpacing( prev, ndi );
        x2 = ndj->getX() - 0.5 * ndj->getWidth() - m_arch->getCellSpacing( ndi, ndj );

        if( !alignPos( ndi, xj, x1, x2 ) )
        {
            return false;
        }

        if( m_nMoved >= m_moveLimit )
        {
            return false;
        }

        m_movedNodes[m_nMoved]  = ndi;
        m_curX[m_nMoved]        = ndi->getX();
        m_curY[m_nMoved]        = ndi->getY();
        m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( si );
        m_newX[m_nMoved]        = xj;
        m_newY[m_nMoved]        = yj;
        m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( sj );
        ++m_nMoved;
        return true;
    }
    if( ndi->getWidth() <= space_right_j )
    {
        // Might fit on the right, depending on spacing.
        Node* next = (ix_j == nn) ? 0 : m_cellsInSeg[sj][ix_j+1];
        x1 = ndj->getX() + 0.5 * ndj->getWidth() + m_arch->getCellSpacing( ndj, ndi );
        x2 = ndj->getX() + 0.5 * ndj->getWidth() + space_right_j - m_arch->getCellSpacing( ndi, next );
        if( !alignPos( ndi, xj, x1, x2 ) )
        {
            return false;
        }

        if( m_nMoved >= m_moveLimit )
        {
            return false;
        }
        m_movedNodes[m_nMoved]  = ndi;
        m_curX[m_nMoved]        = ndi->getX();
        m_curY[m_nMoved]        = ndi->getY();
        m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( si );
        m_newX[m_nMoved]        = xj;
        m_newY[m_nMoved]        = yj;
        m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( sj );
        ++m_nMoved;

        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::tryMove3( Node* ndi, double xi, double yi, int si, double xj, double yj, int sj )
{
    //std::cout << "Trying to move a multi-height cell" << ", "
    //    << "From (" << xi << "," << yi << ")" << ", "
    //    << "Pos [" << ndi->getX()-0.5*ndi->getWidth() << "," << ndi->getX()+0.5*ndi->getWidth() << "]" << ", "
    //    << "To (" << xj << "," << yj << ")" << ", "
    //    << "Width is " << ndi->getWidth()
    //    << std::endl;


    m_nMoved = 0;
    const int move_limit = 10;

    // Code to try and move a multi-height cell to another location.  Simple
    // in that it only looks for gaps.
    
    double singleRowHeight = m_arch->m_rows[0]->getH();
    double originX      = m_arch->m_rows[0]->m_subRowOrigin;
    double siteSpacing  = m_arch->m_rows[0]->m_siteSpacing;

    std::vector<Node*>::iterator it_j;
    double xmin, xmax;
    int site_id;

    // Ensure multi-height, although I think this code should work for single
    // height cells too.
    int spanned = (int)(ndi->getHeight() / singleRowHeight + 0.5 );
    if( spanned <= 1 || spanned != m_reverseCellToSegs[ndi->getId()].size() )
    {
        //std::cout << "Not a multi-height cell." << std::endl;
        return false;
    }

    // Need to turn the target location into a span of rows.  I'm assuming
    // rows are layered on after the other...

    int rb = (((yj - 0.5*ndi->getHeight()) - m_arch->m_rows[0]->getY()) / singleRowHeight);
    while( rb+spanned >= m_arch->m_rows.size() )
    {
        --rb;
    }
    int rt = rb + spanned - 1; // Cell would occupy rows [rb,rt].

    // XXX: Need to check voltages and possibly adjust up or down a bit to 
    // satisfy these requirements.  Or, we could just check those requirements
    // and return false as a failed attempt.
    bool flip = false;
    if( !m_arch->power_compatible( ndi, m_arch->m_rows[rb], flip ) )
    {
        //std::cout << "Not voltage compatible." << std::endl;
        return false;
    }
    yj = m_arch->m_rows[rb]->getY() + 0.5 * ndi->getHeight();

    //std::cout << "Row span is " << rb << " -> " << rt << ", "
    //    << m_arch->m_rows[rb]->getY() << " -> " << m_arch->m_rows[rt]->getY() + m_arch->m_rows[rt]->getH() << std::endl;

    // Next find the segments based on the targeted x location.  We might be outside
    // of our region or there could be a blockage.  So, we need a flag.
    std::vector<int> segs;
    for( int r = rb; r <= rt; r++ )
    {
        bool gotSeg = false;
        for( int s = 0; s < m_segsInRow[r].size() && !gotSeg; s++ )
        {
            DetailedSeg* segPtr = m_segsInRow[r][s];
            if( segPtr->m_regId == ndi->getRegionId() )
            {
                if( xj >= segPtr->m_xmin && xj <= segPtr->m_xmax )
                {
                    gotSeg = true;
                    segs.push_back( segPtr->m_segId );
                }
            }
        }
        if( !gotSeg )
        {
            break;
        }
    }
    // Extra check.
    if( segs.size() != spanned )
    {
        //std::cout << "Could not find segments." << std::endl;
        return false;
    }

    // So, the goal is to try and move the cell into the segments contained within
    // the "segs" vector.  Determine if there is space.  To do this, we loop over
    // the segments and look for the cell to the right of the target location.  We
    // then grab the cell to the left.  We can determine if the the gap is large
    // enough.
    xmin = -std::numeric_limits<double>::max();
    xmax =  std::numeric_limits<double>::max();
    for( size_t s = 0; s < segs.size(); s++ )
    {
        DetailedSeg* segPtr = m_segments[segs[s]];

        int segId = m_segments[segs[s]]->m_segId;
        Node* left = nullptr;
        Node* rite = nullptr;

        if( m_cellsInSeg[segId].size() != 0 )
        {
            it_j = std::lower_bound( m_cellsInSeg[segId].begin(), m_cellsInSeg[segId].end(), xj, compare_node_segment_x() );
            if( it_j == m_cellsInSeg[segId].end() )
            {
                // Nothing to the right; the last cell in the row will be on the left.
                left = m_cellsInSeg[segId].back();

                // If the cell on the left turns out to be the current cell, then we
                // can assume this cell is not there and look to the left "one cell
                // more".
                if( left == ndi )
                {
                    if( it_j != m_cellsInSeg[segId].begin() )
                    {
                        --it_j;
                        left = *it_j;
                        ++it_j;
                    }
                    else 
                    {
                        left = nullptr;
                    }
                }
            }
            else
            {
                rite = *it_j;
                if( it_j != m_cellsInSeg[segId].begin() )
                {
                    --it_j;
                    left = *it_j;
                    if( left == ndi )
                    {
                        if( it_j != m_cellsInSeg[segId].begin() )
                        {
                            --it_j;
                            left = *it_j;
                            ++it_j;
                        }
                        else
                        {
                            left = nullptr;
                        }
                    }
                    ++it_j;
                }
            }
        }

        // If the left or the right cells are the same as the current cell, then
        // we aren't moving.
        if( ndi == left || ndi == rite )
        {
            //std::cout << "Failed - same rows." << std::endl;
            return false;
        }

        double lx = (left == nullptr) ? segPtr->m_xmin : (left->getX() + 0.5 * left->getWidth());
        double rx = (rite == nullptr) ? segPtr->m_xmax : (rite->getX() - 0.5 * rite->getWidth());
        if( left != nullptr ) lx += m_arch->getCellSpacing( left, ndi );
        if( rite != nullptr ) rx -= m_arch->getCellSpacing( ndi, rite );

        if( ndi->getWidth() <= rx - lx )
        {
            // The cell will fit without moving the left and right cell.  
            xmin = std::max( xmin, lx );
            xmax = std::min( xmax, rx );
        }
        else
        {
            // The cell will not fit in between the left and right cell
            // in this segment.  So, we cannot faciliate the single move.
            //std::cout << "Failed - won't fit" << ", "
            //    << "[" << lx << " -> " << rx << "]" 
            //    << std::endl;
            return false;
        }
    }

    //std::cout << "Trying to move multi-height cell " << ndi->getId() << ", "
    //    << "Span is " << spanned << ", ";
    //std::cout << "Current segs/rows is";
    //for( size_t s = 0; s < m_reverseCellToSegs[ndi->getId()].size();s ++ )
    //{
    //    DetailedSeg* segPtr = m_reverseCellToSegs[ndi->getId()][s];
    //    std::cout << " " << segPtr->m_segId << "/" << segPtr->m_rowId;
    //}
    //std::cout << std::endl;
    //std::cout << "Target is (" << xj << "," << yj << ")" << ", ";
    //std::cout << "Target segs/rows is";
    //for( size_t s = 0; s < segs.size(); s++ )
    //{
    //    DetailedSeg* segPtr = m_segments[segs[s]];
    //    std::cout << " " << segPtr->m_segId << "/" << segPtr->m_rowId;
    //}
    //std::cout << ", Space is " << "[" << xmin << " -> " << xmax << "]" << ", "
    //    << (xmax-xmin) << std::endl;

    // Here, we can fit.
    if( ndi->getWidth() <= xmax-xmin )
    {
        if( !alignPos( ndi, xj, xmin, xmax ) )
        {
            return false;
        }

        if( m_nMoved >= move_limit )
        {
            return false;
        }
        m_movedNodes[m_nMoved]  = ndi;
        m_curX[m_nMoved]        = ndi->getX();
        m_curY[m_nMoved]        = ndi->getY();
        m_curSeg[m_nMoved].clear(); 
        for( size_t i = 0; i < m_reverseCellToSegs[ndi->getId()].size(); i++ )
        {
            m_curSeg[m_nMoved].push_back( m_reverseCellToSegs[ndi->getId()][i]->m_segId );
        }
        m_newX[m_nMoved]        = xj;
        m_newY[m_nMoved]        = yj;
        m_newSeg[m_nMoved].clear();
        m_newSeg[m_nMoved].insert( m_newSeg[m_nMoved].end(), segs.begin(), segs.end() );
        ++m_nMoved;

        return true;
    }
    else
    {
        //std::cout << "Target is (" << xj << "," << yj << ")" << ", " << "No fit." << std::endl;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::trySwap1( Node* ndi, 
    double xi, double yi, int si, 
    double xj, double yj, int sj )
{
    // Tries to swap two single height cells.  Swapped cell is one near
    // the target location.  Does not do any shifting so only ndi and
    // ndj are involved.

    const int move_limit = 10;
    m_nMoved = 0;

    std::vector<Node*>::iterator it_i;
    std::vector<Node*>::iterator it_j;
    int ix_i = -1;
    int ix_j = -1;
    Node* prev;
    Node* next;

    Node* ndj = 0;
    if( m_cellsInSeg[sj].size() != 0 )
    {
        it_j = std::lower_bound( 
            m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), xj, compare_node_segment_x() );
        if( it_j == m_cellsInSeg[sj].end() )
        {   
            ndj = m_cellsInSeg[sj].back();
        }
        else
        {
            ndj = *it_j;
        }
    }
    if( ndj == ndi )
    {
        ndj = 0;
    }

    if( ndi == 0 || ndj == 0 )
    {
        return false;
    }

    int spanned_i = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);
    int spanned_j = (int)(ndj->getHeight() / m_singleRowHeight + 0.5);
    if( spanned_i != 1 || spanned_j != 1 )
    {
        return false;
    }

    // Find space around both of the cells.
    double space_left_i, space_right_i, large_left_i, large_right_i;
    double space_left_j, space_right_j, large_left_j, large_right_j;

    it_i = std::find( m_cellsInSeg[si].begin(), m_cellsInSeg[si].end(), ndi );
    ix_i = it_i - m_cellsInSeg[si].begin();

    it_j = std::find( m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), ndj );
    ix_j = it_j - m_cellsInSeg[sj].begin();

    getSpaceAroundCell( sj, ix_j, space_left_j, space_right_j, large_left_j, large_right_j );
    getSpaceAroundCell( si, ix_i, space_left_i, space_right_i, large_left_i, large_right_i );

    double originX;
    double siteSpacing;
    int site_id;
    double x1, x2, x3, x4;

    int ni = m_cellsInSeg[si].size()-1;
    int nj = m_cellsInSeg[sj].size()-1;

    bool adjacent = false;
    if( (si == sj) && ( ix_i+1 == ix_j || ix_j+1 == ix_i ) )
    {
        adjacent = true;
    }


    if( !adjacent )
    {
        // Cells are not adjacent.  Therefore we only need to look at the
        // size of the holes created and see if each cell can fit into 
        // the gap created by removal of the other cell.
        double hole_i = ndi->getWidth() + space_left_i + space_right_i;
        double hole_j = ndj->getWidth() + space_left_j + space_right_j;

        // Need to take into account edge spacing.
        prev = (ix_i ==  0) ? 0 : m_cellsInSeg[si][ix_i-1];
        next = (ix_i == ni) ? 0 : m_cellsInSeg[si][ix_i+1];
        double need_j = ndj->getWidth();
        need_j += m_arch->getCellSpacing( prev, ndj );
        need_j += m_arch->getCellSpacing( ndj, next );

        prev = (ix_j ==  0) ? 0 : m_cellsInSeg[sj][ix_j-1];
        next = (ix_j == nj) ? 0 : m_cellsInSeg[sj][ix_j+1];
        double need_i = ndi->getWidth();
        need_i += m_arch->getCellSpacing( prev, ndi );
        need_i += m_arch->getCellSpacing( ndi, next );

        if( !(need_j <= hole_i+1.0e-3 && need_i <= hole_j+1.0e-3) )
        {
            return false;
        }

        // Need to refine and align positions.
        prev = (ix_j ==  0) ? 0 : m_cellsInSeg[sj][ix_j-1];
        next = (ix_j == nj) ? 0 : m_cellsInSeg[sj][ix_j+1];

        x1 = (ndj->getX() - 0.5 * ndj->getWidth()) - space_left_j  + m_arch->getCellSpacing( prev, ndi );
        x2 = (ndj->getX() + 0.5 * ndj->getWidth()) + space_right_j - m_arch->getCellSpacing( ndi, next );
        if( !alignPos( ndi, xj, x1, x2 ) )
        {
            return false;
        }

        prev = (ix_i ==  0) ? 0 : m_cellsInSeg[si][ix_i-1];
        next = (ix_i == ni) ? 0 : m_cellsInSeg[si][ix_i+1];

        x3 = (ndi->getX() - 0.5 * ndi->getWidth()) - space_left_i  + m_arch->getCellSpacing( prev, ndj );
        x4 = (ndi->getX() + 0.5 * ndi->getWidth()) + space_right_i - m_arch->getCellSpacing( ndj, next );
        if( !alignPos( ndj, xi, x3, x4 ) )
        {
            return false;
        }

        if( m_nMoved >= move_limit )
        {
            return false;
        }
        m_movedNodes[m_nMoved]  = ndi;
        m_curX[m_nMoved]        = ndi->getX();
        m_curY[m_nMoved]        = ndi->getY();
        m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( si );
        m_newX[m_nMoved]        = xj;
        m_newY[m_nMoved]        = ndj->getY();
        m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( sj );
        ++m_nMoved;

        if( m_nMoved >= move_limit )
        {
            return false;
        }
        m_movedNodes[m_nMoved]  = ndj;
        m_curX[m_nMoved]        = ndj->getX();
        m_curY[m_nMoved]        = ndj->getY();
        m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( sj );
        m_newX[m_nMoved]        = xi;
        m_newY[m_nMoved]        = ndi->getY();
        m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( si );
        ++m_nMoved;

        return true;
    }
    else
    {
        // Same row and adjacent.
        if( ix_i+1 == ix_j )
        {
            // cell i is left of cell j.
            double hole = ndi->getWidth() + ndj->getWidth();
            hole += space_right_j;
            hole += space_left_i;
            double gap = (ndj->getX() - 0.5 * ndj->getWidth()) - (ndi->getX() + 0.5 * ndi->getWidth());
            hole += gap;

            Node* prev = (ix_i ==  0) ? nullptr : m_cellsInSeg[si][ix_i-1];
            Node* next = (ix_j == nj) ? nullptr : m_cellsInSeg[sj][ix_j+1];
            double need = ndi->getWidth() + ndj->getWidth();
            need += m_arch->getCellSpacing( prev, ndj );
            need += m_arch->getCellSpacing( ndi, next );
            need += m_arch->getCellSpacing( ndj, ndi );

            if( !(need <= hole+1.0e-3) )
            {
                return false;
            }

            double left  = ndi->getX() - 0.5 * ndi->getWidth() - space_left_i + m_arch->getCellSpacing( prev, ndj );
            double right = ndj->getX() + 0.5 * ndj->getWidth() + space_right_j - m_arch->getCellSpacing( ndi, next );
            std::vector<Node*> cells;
            std::vector<double> tarX;
            std::vector<double> posX;
            cells.push_back( ndj );
            tarX.push_back( xi );
            posX.push_back( 0. );
            cells.push_back( ndi );
            tarX.push_back( xj );
            posX.push_back( 0. );
            if( shift( cells, tarX, posX, left, right, si, m_segments[si]->m_rowId ) == false )
            {
                return false;
            }
            xi = posX[0];
            xj = posX[1];

            /*
            if( prev != 0 )
            {
              std::cout << boost::format("P: %6d (%lf,%lf) ") 
                  % prev->getId() % (prev->getX() - 0.5 * prev->getWidth()) % (prev->getX() + 0.5 * prev->getWidth())
                  << std::endl;
            }
            std::cout << boost::format("L: %lf ") % left << std::endl;
            std::cout << boost::format("J: %6d (%lf,%lf) ") 
                % ndj->getId() % (xi - 0.5 * ndj->getWidth()) % (xi + 0.5 * ndj->getWidth())
                << std::endl;
            std::cout << boost::format("I: %6d (%lf,%lf) ") 
                % ndi->getId() % (xj - 0.5 * ndi->getWidth()) % (xj + 0.5 * ndi->getWidth())
                << std::endl;
            std::cout << boost::format("R: %lf ") % right << std::endl;

            if( next != 0 )
            {
              std::cout << boost::format("P: %6d (%lf,%lf) ") 
                  % next->getId() % (next->getX() - 0.5 * next->getWidth()) % (next->getX() + 0.5 * next->getWidth())
                  << std::endl;
            }

            if( xi+0.5*ndj->getWidth() > xj-0.5*ndi->getWidth() )
            {
                std::cout << "Caused overlap with moved cells." << std::endl;
                exit(-1);
            }
            if( prev != 0 )
            {
                if( prev->getX() + 0.5 * prev->getWidth() > xi-0.5*ndj->getWidth() )
                {
                    std::cout << "Caused overlap on left." << std::endl;
                    exit(-1);
                }
            }
            if( next != 0 )
            {
                if( next->getX() - 0.5 * next->getWidth() < xj+0.5*ndi->getWidth() )
                {
                    std::cout << "Caused overlap on right." << std::endl;
                    exit(-1);
                }
            }
            */
        }
        else if( ix_j+1 == ix_i )
        {
            // cell j is left of cell i.
            double hole = ndi->getWidth() + ndj->getWidth();
            hole += space_right_i;
            hole += space_left_j;
            hole += (ndi->getX() - 0.5 * ndi->getWidth()) - (ndj->getX() + 0.5 * ndj->getWidth());

            Node* prev = (ix_j ==  0) ? nullptr : m_cellsInSeg[sj][ix_j-1];
            Node* next = (ix_i == ni) ? nullptr : m_cellsInSeg[si][ix_i+1];
            double need = ndi->getWidth() + ndj->getWidth();
            need += m_arch->getCellSpacing( prev, ndi );
            need += m_arch->getCellSpacing( ndj, next );
            need += m_arch->getCellSpacing( ndi, ndj );

            if( !(need <= hole+1.0e-3) )
            {
                return false;
            }

            double left  = ndj->getX() - 0.5 * ndj->getWidth() - space_left_j + m_arch->getCellSpacing( prev, ndi );
            double right = ndi->getX() + 0.5 * ndi->getWidth() + space_right_i - m_arch->getCellSpacing( ndj, next );
            std::vector<Node*> cells;
            std::vector<double> tarX;
            std::vector<double> posX;
            cells.push_back( ndi );
            tarX.push_back( xj );
            posX.push_back( 0. );
            cells.push_back( ndj );
            tarX.push_back( xi );
            posX.push_back( 0. );
            if( shift( cells, tarX, posX, left, right, si, m_segments[si]->m_rowId ) == false )
            {
                return false;
            }
            xj = posX[0];
            xi = posX[1];
        }
        else
        {   
            // Shouldn't get here.
            return false;
        }

        if( m_nMoved >= move_limit )
        {
            return false;
        }
        m_movedNodes[m_nMoved]  = ndi;
        m_curX[m_nMoved]        = ndi->getX();
        m_curY[m_nMoved]        = ndi->getY();
        m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( si );
        m_newX[m_nMoved]        = xj;
        m_newY[m_nMoved]        = ndj->getY();
        m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( sj );
        ++m_nMoved;

        if( m_nMoved >= move_limit )
        {
            return false;
        }
        m_movedNodes[m_nMoved]  = ndj;
        m_curX[m_nMoved]        = ndj->getX();
        m_curY[m_nMoved]        = ndj->getY();
        m_curSeg[m_nMoved].clear(); m_curSeg[m_nMoved].push_back( sj );
        m_newX[m_nMoved]        = xi;
        m_newY[m_nMoved]        = ndi->getY();
        m_newSeg[m_nMoved].clear(); m_newSeg[m_nMoved].push_back( si );
        ++m_nMoved;

        return true;
    }
    return false;
}




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::acceptMove( void )
{
    // Moves stored list of cells.  XXX: Only single height cells.

    for( int i = 0; i < m_nMoved; i++ )
    {
        Node* ndi = m_movedNodes[i];

        //std::cout << "@" << i << ", Node is " << ndi->getId() << std::endl;

        int spanned = (int)((ndi->getHeight()/m_singleRowHeight)+0.5);
        //if( spanned != 1 )
        //{
        //    std::cout << "Error." << std::endl;
        //    exit(-1);
        //}

        // Remove node from current segment.
        for( size_t s = 0; s < m_curSeg[i].size(); s++ )
        {
            this->removeCellFromSegment( ndi, m_curSeg[i][s] );
        }

        // Update position and orientation.
        ndi->setX( m_newX[i] );
        ndi->setY( m_newY[i] );
        // XXX: Need to do the orientiation.
        ;

        // Insert into new segment.
        for( size_t s = 0; s < m_newSeg[i].size(); s++ )
        {
            this->addCellToSegment( ndi, m_newSeg[i][s] );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::rejectMove( void )
{
    m_nMoved = 0; // Sufficient, I think.
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::printMove( void )
{
    for( int i = 0; i < m_nMoved; i++ )
    {
        Node* ndi = m_movedNodes[i];

        std::cout << "Move: " << i << ", "
            << "Node " << ndi->getId() << ", "
            << "Orig pos is " << m_curX[i] << "," << m_curY[i] << ", " 
            << "Next pos is " << m_newX[i] << "," << m_newY[i] << ", "
            ;
        std::cout << "Orig segs: [";
        for( size_t s = 0; s < m_curSeg[i].size(); s++ )
        {
            int id = m_curSeg[i][s];
            std::cout << " " << id << "r" << m_segments[id]->m_rowId;
        }
        std::cout << "]";
        std::cout << "Next segs: [";
        for( size_t s = 0; s < m_newSeg[i].size(); s++ )
        {
            int id = m_newSeg[i][s];
            std::cout << " " << id << "r" << m_segments[id]->m_rowId;
        }
        std::cout << "]";
        std::cout << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::debugSegments( void )
{
    // Do some debug checks on segments. 
    


    // Confirm the segments are sorted.
    int err1 = 0;
    for( size_t s = 0; s < m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = m_segments[s];
        int segId = segPtr->m_segId;
        for( size_t i = 1; i < m_cellsInSeg[segId].size(); i++ )
        {
            Node* prev = m_cellsInSeg[segId][i-1];
            Node* curr = m_cellsInSeg[segId][i];
            if( prev->getX() >= curr->getX() + 1.0e-3 )
            {
                ++err1;
            }
        }
    }

    // Confirm that cells are in their mapped segments, as well as the revers.
    int err2 = 0;
    for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node* ndi = &(m_network->m_nodes[i]);
        for( size_t s = 0; s < m_reverseCellToSegs[ndi->getId()].size(); s++ )
        {
            DetailedSeg* segPtr = m_reverseCellToSegs[ndi->getId()][s];
            int segId = segPtr->m_segId;

            std::vector<Node*>::iterator it = 
                    std::find( m_cellsInSeg[segId].begin(), m_cellsInSeg[segId].end(), ndi );
            if( m_cellsInSeg[segId].end() == it )
            {
                ++err2;
            }
        }
    }
    std::vector<std::vector<DetailedSeg*> > reverse;
    reverse.resize( m_network->m_nodes.size() );
    int err3 = 0;
    for( size_t s = 0; s < m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = m_segments[s];
        int segId = segPtr->m_segId;
        for( size_t i = 0; i < m_cellsInSeg[segId].size(); i++ )
        {
            Node* ndi = m_cellsInSeg[segId][i];
            reverse[ndi->getId()].push_back( segPtr );
            std::vector<DetailedSeg*>::iterator it = 
                std::find( m_reverseCellToSegs[ndi->getId()].begin(), m_reverseCellToSegs[ndi->getId()].end(),
                segPtr );
            if( m_reverseCellToSegs[ndi->getId()].end() == it )
            {
                ++err3;
            }
        }
    }
    int err4 = 0;
    for( size_t i = 0; i < m_network->m_nodes.size(); i++ )
    {
        Node* ndi = &(m_network->m_nodes[i]);
        if( m_reverseCellToSegs[ndi->getId()].size() != reverse[ndi->getId()].size() )
        {
            ++err4;
            std::stable_sort( m_reverseCellToSegs[ndi->getId()].begin(), m_reverseCellToSegs[ndi->getId()].end() );
            std::stable_sort( reverse[ndi->getId()].begin(), reverse[ndi->getId()].end() );
            for( size_t s = 0; s < m_reverseCellToSegs[ndi->getId()].size(); s++ )
            {
                if( m_reverseCellToSegs[ndi->getId()][s] != reverse[ndi->getId()][s] )
                {
                    ++err4;
                }
            }
        }
    }

    if( err1 != 0 )
    {
        std::cout << "Segment debug; " << err1 << " times segment not sorted." << std::endl;
    }
    if( err2 != 0 )
    {
        std::cout << "Segment debug; " << err2 << " times cell not found in mapped segment." << std::endl;
    }
    if( err3 != 0 )
    {
        std::cout << "Segment debug; " << err3 << " times cell in segment not in map." << std::endl;
    }
    if( err4 != 0 )
    {
        std::cout << "Segment debug: " << err4 << " times cell to segment map seems the wrong size." << std::endl;
    }
}


} // namespace dpo
