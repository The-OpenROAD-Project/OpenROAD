

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// TODO: 
// - Flip multi-height cells to get power alignment correct.  I don't think 
//   this code flips properly for multi-height cells so I need to dig a bit
//   deeper into multi-height cells.  Or, maybe for the time being, I will
//   get the power alignment correct (e.g., Assume SYMMETRY_X) and just 
//   assume any flips are okay in the other direction.
// - Add flipping to reduce edge spacing violations.


////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <stack>
#include <utility>
#include <cmath>
#include <boost/tokenizer.hpp>
#include "utility.h"
#include "detailed_segment.h"
#include "detailed_manager.h"
#include "detailed_orient.h"



namespace aak
{

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedOrient::DetailedOrient( Architecture* arch, Network* network, RoutingParams* rt ):
    m_arch( arch ),
    m_network( network ),
    m_rt( rt ),
    m_skipNetsLargerThanThis( 100 )
{
    m_traversal = 0;
    m_edgeMask.resize( m_network->m_edges.size() );
    std::fill( m_edgeMask.begin(), m_edgeMask.end(), m_traversal );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedOrient::~DetailedOrient( void )
{
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedOrient::run( DetailedMgr* mgrPtr, std::string command )
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
void DetailedOrient::run( DetailedMgr* mgrPtr, std::vector<std::string>& args )
{
    // This routine scans all segments and ensures each cell is properly 
    // oriented given its row assignment.  Then, depending on the 
    // arguments, will perform cell flipping.

    m_mgrPtr = mgrPtr;
    orientCellsForRow();

    bool doFlip = false;

    for( size_t i = 1; i < args.size(); i++ )
    {
        if( args[i] == "-f" )
        {
            doFlip = true;
        }
    }
    if( doFlip )
    {
        flipCells();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedOrient::orientCellForRow( Node* ndi, int row )
{
    // Correct the node orientation such that it is correct for the specified row.
    // The routine is presently quite simple...  XXX: It only fixes the simple 
    // issues and might not correct all of the problems with orientation!

    unsigned rowOri     = m_arch->m_rows[row]->m_siteOrient;
    unsigned siteSym    = m_arch->m_rows[row]->m_siteSymmetry;
    unsigned cellOri    = ndi->m_currentOrient;

    if( rowOri == Orientation_N || rowOri == Orientation_FN )
    {
        if( cellOri == Orientation_N || cellOri == Orientation_FN )
        {
            return true;
        }

        if( cellOri == Orientation_FS )
        {
            for( int pi = ndi->m_firstPin; pi < ndi->m_lastPin; pi++ )
            {
                Pin* pin = m_network->m_nodePins[pi];
                pin->m_offsetY *= -1;
            }
            ndi->m_currentOrient = Orientation_N ;
            return true;
        }
        else if( cellOri == Orientation_S )
        {
            for( int pi = ndi->m_firstPin; pi < ndi->m_lastPin; pi++ )
            {
                Pin* pin = m_network->m_nodePins[pi];
                pin->m_offsetY *= -1;
            }
            ndi->m_currentOrient = Orientation_FN;
            return true;
        }
        return false;
    }
    else if( rowOri == Orientation_FS || Orientation_S )
    {
        if( cellOri == Orientation_FS || cellOri == Orientation_S )
        {
            return true;
        }

        if( cellOri == Orientation_N )
        {
            for( int pi = ndi->m_firstPin; pi < ndi->m_lastPin; pi++ )
            {
                Pin* pin = m_network->m_nodePins[pi];
                pin->m_offsetY *= -1;
            }
            ndi->m_currentOrient = Orientation_FS;
        }
        else if( cellOri == Orientation_FN )
        {
            for( int pi = ndi->m_firstPin; pi < ndi->m_lastPin; pi++ )
            {
                Pin* pin = m_network->m_nodePins[pi];
                pin->m_offsetY *= -1;
            }
            ndi->m_currentOrient = Orientation_S ;
        }
        return false;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedOrient::orientAdjust( Node* ndi, unsigned newOri )
{
    unsigned curOri = ndi->m_currentOrient;
    if( curOri == newOri )
    {
        return false;
    }

    // Determine how pins need to be flipped.  I guess the easiest thing to do it to first
    // return the node to the N orientation and then figure out how to get it into the new
    // orientation!
    int mY = 1;     // Multiplier to adjust pin offsets for a flip around the X-axis.
    int mX = 1;     // Multiplier to adjust pin offsets for a flip around the Y-axis.

    switch( curOri ) 
    { 
    case Orientation_N :                       break; 
    case Orientation_S : mX *= -1; mY *= -1;   break; 
    case Orientation_FS:           mY *= -1;   break; 
    case Orientation_FN: mX *= -1;             break; 
    default            :                       break; 
    }

    // Here, assume the cell is in the North Orientation...
    switch( newOri ) 
    { 
    case Orientation_N :                       break; 
    case Orientation_S : mX *= -1; mY *= -1;   break; 
    case Orientation_FS:           mY *= -1;   break; 
    case Orientation_FN: mX *= -1;             break; 
    default            :                       break; 
    }

    for( int pi = ndi->m_firstPin; pi < ndi->m_lastPin; pi++ )
    {
        Pin* pin = m_network->m_nodePins[pi];

        if( mX == -1 )      pin->m_offsetX *= (double)mX;
        if( mY == -1 )      pin->m_offsetY *= (double)mY;
    }
    ndi->m_currentOrient = newOri;

    if( mX == -1 ) 
    {
        ndi->swapEdgeTypes();
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
unsigned DetailedOrient::orientFind( Node* ndi, int row )
{
    // Given a node, determine a valid orientation for the node if the node is placed into
    // the specified row.  Actually, we could just return the row's orientation, but this  
    // might be a little smarter if cells have been flipped around the Y-axis previously to
    // improve WL...

    unsigned rowOri     = m_arch->m_rows[row]->m_siteOrient;
    unsigned siteSym    = m_arch->m_rows[row]->m_siteSymmetry;
    unsigned cellOri    = ndi->m_currentOrient;

    if( rowOri == Orientation_N || rowOri == Orientation_FN )
    {
        if( cellOri == Orientation_N || cellOri == Orientation_FN )
        {
            return cellOri;
        }
        if( cellOri == Orientation_FS )
        {
            return Orientation_N;
        }
        if( cellOri == Orientation_S  )
        {
            return Orientation_FN;
        }
    }
    else if( rowOri == Orientation_FS || rowOri == Orientation_S )
    {
        if( cellOri == Orientation_FS || cellOri == Orientation_S )
        {
            return cellOri;
        }
        if( cellOri == Orientation_N  )
        {
            return Orientation_FS;
        }
        if( cellOri == Orientation_FN )
        {
            return Orientation_S ;
        }
    }
    return rowOri;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
int DetailedOrient::orientCellsForRow( void )
{
    // Scans all segments and makes sure that each cell is in a correct orientation
    // for its assigned row.
    int retval = 0;
    for( int s = 0; s < m_mgrPtr->m_segments.size(); s++ )
    {
        DetailedSeg* segment = m_mgrPtr->m_segments[s];

        int row = segment->m_rowId;

        std::vector<Node*>& nodes = m_mgrPtr->m_cellsInSeg[segment->m_segId];
        for( int i = 0; i < nodes.size(); i++ )
        {
            bool success = orientCellForRow( nodes[i], row );
            if( !success )
            {
                ++retval;
            }
        }
    }
    return retval;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
int DetailedOrient::flipCells( void )
{
    // Flip cells within a row in order to reduce WL.  We only do the flipping if the row
    // supports SYMMETRY_Y.  Note that we *ASSUME* cells are properly oriented for the
    // row already; we are just going to change to the "flipped about the Y-symmetry" so
    // if not correctly oriented for the row, they will still get flipped but still likely
    // be in an invalid orientation.

    Node* ndl;
    Node* ndr;
    double gapl, gapr, spacel, spacer;
    double old_wl, new_wl;
    int nflips = 0;
    double old_xmin, old_xmax;
    double new_xmin, new_xmax;

    for( int s = 0; s < m_mgrPtr->m_segments.size(); s++ )
    {
        DetailedSeg* segment = m_mgrPtr->m_segments[s];

        int row = segment->m_rowId;

        if( (m_arch->m_rows[row]->m_siteSymmetry & Symmetry_Y) == 0 )
        {
            continue;
        }

        std::vector<Node*>& nodes = m_mgrPtr->m_cellsInSeg[segment->m_segId];
        for( int i = 0; i < nodes.size(); i++ )
        {
            Node* ndi = nodes[i];

            // Check to ensure that flipping does not violate any gap requirements between cells.
            // Make the check locally since this means we should not have to adjust the row
            // later on....  Note that we check the required gap *ASSUMING* the cell is flipped!

            ndl = (i == 0) ? 0 : nodes[i-1];
            ndr = (i == nodes.size()-1) ? 0 : nodes[i+1];

            gapl = 0.;
            gapr = 0.;
            if( ndl != 0 )
            {
                gapl = m_arch->getCellSpacing( ndl, ndi );
            }
            if( ndr != 0 )
            {
                gapr = m_arch->getCellSpacing( ndi, ndr );
            }
            spacel = (ndi->getX() - 0.5 * ndi->getWidth()) - ((ndl == 0) ? segment->m_xmin : (ndl->getX() + 0.5 * ndl->getWidth()));
            spacer = ((ndr == 0) ? segment->m_xmax : (ndr->getX() - 0.5 * ndr->getWidth())) - (ndi->getX() + 0.5 * ndi->getWidth());
            if( gapl > spacel || gapr > spacer )
            {
                continue;
            }

            // Get the WL with the cell in its current orientation and with it flipped.
            old_wl = new_wl = 0.;

            ++m_traversal;
            for( int pi = ndi->m_firstPin; pi < ndi->m_lastPin; pi++ )
            {
                Pin* pini = m_network->m_nodePins[pi];

                Edge* edi = &(m_network->m_edges[pini->m_edgeId]);

                int npins = edi->m_lastPin - edi->m_firstPin;
                if( npins <= 1 || npins >= m_skipNetsLargerThanThis )
                {
                    continue;
                }
                if( m_edgeMask[edi->getId()] == m_traversal )
                {
                    continue;
                }
                m_edgeMask[edi->getId()] = m_traversal;

                old_xmin =  std::numeric_limits<double>::max();
                old_xmax = -std::numeric_limits<double>::max();

                new_xmin =  std::numeric_limits<double>::max();
                new_xmax = -std::numeric_limits<double>::max();

                for( int pj = edi->m_firstPin; pj < edi->m_lastPin; pj++ )
                {
                    Pin* pinj = m_network->m_edgePins[pj];

                    Node* ndj = &(m_network->m_nodes[pinj->m_nodeId]);

                    old_xmin = std::min( old_xmin, ndj->getX() + pinj->m_offsetX );
                    old_xmax = std::max( old_xmax, ndj->getX() + pinj->m_offsetX );

                    if( ndj == ndi )
                    {
                        new_xmin = std::min( new_xmin, ndj->getX() - pinj->m_offsetX );
                        new_xmax = std::max( new_xmax, ndj->getX() - pinj->m_offsetX );
                    }
                    else
                    {
                        new_xmin = std::min( new_xmin, ndj->getX() + pinj->m_offsetX );
                        new_xmax = std::max( new_xmax, ndj->getX() + pinj->m_offsetX );
                    }

                }
                old_wl += old_xmax - old_xmin;
                new_wl += new_xmax - new_xmin;
            }
            if( new_wl < old_wl )
            {
                // Perform the flipping, assuming we  have orientations that we can understand.
                if     ( ndi->m_currentOrient == Orientation_N  )
                {
                    for( int pi = ndi->m_firstPin; pi < ndi->m_lastPin; pi++ )
                    {
                        Pin* pin = m_network->m_nodePins[pi];
                        pin->m_offsetX *= -1;
                    }
                    ndi->swapEdgeTypes();

                    ndi->m_currentOrient = Orientation_FN;

                    ++nflips;
                }
                else if( ndi->m_currentOrient == Orientation_S  )
                {
                    for( int pi = ndi->m_firstPin; pi < ndi->m_lastPin; pi++ )
                    {
                        Pin* pin = m_network->m_nodePins[pi];
                        pin->m_offsetX *= -1;
                    }
                    ndi->swapEdgeTypes();

                    ndi->m_currentOrient = Orientation_FS;

                    ++nflips;
                }
                else if( ndi->m_currentOrient == Orientation_FN )
                {
                    for( int pi = ndi->m_firstPin; pi < ndi->m_lastPin; pi++ )
                    {
                        Pin* pin = m_network->m_nodePins[pi];
                        pin->m_offsetX *= -1;
                    }
                    ndi->swapEdgeTypes();

                    ndi->m_currentOrient = Orientation_N ;

                    ++nflips;
                }
                else if( ndi->m_currentOrient == Orientation_FS )
                {
                    for( int pi = ndi->m_firstPin; pi < ndi->m_lastPin; pi++ )
                    {
                        Pin* pin = m_network->m_nodePins[pi];
                        pin->m_offsetX *= -1;
                    }
                    ndi->swapEdgeTypes();

                    ndi->m_currentOrient = Orientation_S ;

                    ++nflips;
                }
            }
        }
    }
    return nflips;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
bool DetailedOrient::isLegalSym( unsigned rowOri, unsigned siteSym, unsigned cellOri )
{
    // Messy...
    if( siteSym == Symmetry_Y )
    {
        if     ( rowOri == Orientation_N  )
        {
            if( cellOri == Orientation_N || cellOri == Orientation_FN ) return true;
            else return false;
        }
        else if( rowOri == Orientation_FS )
        {
            if( cellOri == Orientation_S || cellOri == Orientation_FS ) return true;
            else return false;
        }
        else
        {
            // XXX: Odd...
            return false;
        }
    }
    if( siteSym == Symmetry_X )
    {
        if     ( rowOri == Orientation_N  )
        {
            if( cellOri == Orientation_N || cellOri == Orientation_FS ) return true;
            else return false;
        }
        else if( rowOri == Orientation_FS )
        {
            if( cellOri == Orientation_N || cellOri == Orientation_FS ) return true;
            else return false;
        }
        else
        {
            // XXX: Odd...
            return false;
        }
    }
    if( siteSym == (Symmetry_X | Symmetry_Y) )
    {
        if     ( rowOri == Orientation_N  )
        {
            if( cellOri == Orientation_N || cellOri == Orientation_FN ||
                cellOri == Orientation_S || cellOri == Orientation_FS ) return true;
            else return false;
        }
        else if( rowOri == Orientation_FS )
        {
            if( cellOri == Orientation_N || cellOri == Orientation_FN ||
                cellOri == Orientation_S || cellOri == Orientation_FS ) return true;
            else return false;
        }
        else
        {
            // XXX: Odd...
            return false;
        }
    }
    if( siteSym == Symmetry_UNKNOWN )
    {
        if     ( rowOri == Orientation_N  )
        {
            if( cellOri == Orientation_N  ) return true;
            else return false;
        }
        else if( rowOri == Orientation_FS )
        {
            if( cellOri == Orientation_FS ) return true;
            else return false;
        }
        else
        {
            // XXX: Odd...
            return false;
        }
    }
    if( siteSym == Symmetry_ROT90 )
    {
        // XXX: Not handled.
        return true;
    }
    // siteSym = X Y ROT90... Anything is okay.
    return true;
}



} // namespace aak
