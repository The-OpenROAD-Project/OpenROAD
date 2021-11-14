
////////////////////////////////////////////////////////////////////////////////
// File: architecture.cxx
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <string.h>

#include "architecture.h"
#include "network.h"
#include "router.h"



namespace aak 
{

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
Architecture::Architecture():
    m_useSpacingTable( false ),
    m_usePadding( false )
{
    m_xmin = std::numeric_limits<double>::max();
    m_xmax = -std::numeric_limits<double>::max();
    m_ymin = std::numeric_limits<double>::max();
    m_ymax = -std::numeric_limits<double>::max();
}

Architecture::~Architecture()
{
    clear_edge_type();

    for( unsigned r = 0; r < m_rows.size(); r++ )
    {
        delete m_rows[r];
    }
    m_rows.clear();

    for( unsigned r = 0; r < m_regions.size(); r++ )
    {
        delete m_regions[r];
    }
    m_regions.clear();

    clearSpacingTable();
}

void Architecture::clearSpacingTable( void )
{
    for( size_t i = 0; i < m_cellSpacings.size(); i++ )
    {
        delete m_cellSpacings[i];
    }
    m_cellSpacings.clear();
}

bool Architecture::isSingleHeightCell( Node* ndi ) const
{
    int spanned = (int)(ndi->getHeight() / m_rows[0]->getH() + 0.5);
    return spanned == 1;
}

bool Architecture::isMultiHeightCell( Node* ndi ) const
{
    int spanned = (int)(ndi->getHeight() / m_rows[0]->getH() + 0.5);
    return spanned != 1;
}

bool Architecture::uniform()
{
    // Determine if the rows are the same in certain key parameters.
    double rowHeight = m_rows[0]->m_rowHeight;
    double siteSpacing = m_rows[0]->m_siteSpacing;
    double siteWidth = m_rows[0]->m_siteWidth;
    for( int r = 1; r < m_rows.size(); r++ ) {
        if( m_rows[r]->m_rowHeight != rowHeight ) 
        {
            std::cout << "Unequal row heights discovered." << std::endl;
            return false;
        }
        if( m_rows[r]->m_siteWidth != siteWidth ) 
        {
            std::cout << "Unequal site widths discovered." << std::endl;
            return false;
        }
        if( m_rows[r]->m_siteSpacing != siteSpacing ) 
        {
            std::cout << "Unequal site spacings discovered." << std::endl;
            return false;
        }
    }
    return true;
}

bool Architecture::post_process()
{
    // XXX: It would be very nice if the sites formed a uniform grid...  Currently, the following
    // code does not make a uniform grid.  

    m_xmin = std::numeric_limits<double>::max();
    m_xmax = -std::numeric_limits<double>::max();
    m_ymin = std::numeric_limits<double>::max();
    m_ymax = -std::numeric_limits<double>::max();

    // Sort rows.
    std::stable_sort( m_rows.begin(), m_rows.end(), SortRow() );
    for( int r = 0; r < m_rows.size(); r++ ) 
    {
        m_rows[r]->m_id = r;
    }

    for( int r = 0; r < m_rows.size(); r++ ) 
    {
        Architecture::Row* row = m_rows[r];

        m_xmin = std::min( m_xmin, row->m_subRowOrigin );
        m_xmax = std::max( m_xmax, row->m_subRowOrigin + row->m_numSites * row->m_siteSpacing );
        m_ymin = std::min( m_ymin, row->m_rowLoc );
        m_ymax = std::max( m_ymax, row->m_rowLoc + row->m_rowHeight );
    }

    // Check to see if any adjacent rows are co-linear.   This really implies that the rows
    // are sub rows, and the code isn't really set up to support that...  XXX: Do other 
    // parts of the code even work correctly if this is the case????????????
    for( int r = 1; r < m_rows.size(); r++ )
    {
        Architecture::Row* rb = m_rows[r-1];
        Architecture::Row* rt = m_rows[r-0];
        if( std::fabs( rb->getY() - rt->getY() ) <= 1.0e-3 )
        {
            std::cout << "Row " << r-1 << " and row " << r << " appear to have the same Y-location (subrows?)." << std::endl;

            // Check if they overlap in the X-dir.
            double s1 = rb->m_subRowOrigin;
            double e1 = rb->m_subRowOrigin + rb->m_numSites * rb->m_siteSpacing;

            double s2 = rt->m_subRowOrigin;
            double e2 = rt->m_subRowOrigin + rt->m_numSites * rt->m_siteSpacing;

            if( (s2 < e1 && s1 < e2) )
            {
                std::cout << "Row " << r-1 << " and row " << r << " overlap in the X-direction!" <<  std::endl;
            }
        }
    }

    std::cout << "Architecture: " << "(" << m_xmin << "," << m_ymin << ")" << "-"
        << "(" << m_xmax << "," << m_ymax << ")" 
        << std::endl;

    return true;
}

int Architecture::find_closest_row( double y )
{
    // Given a "y" (intended to be the bottom of a cell), find the closest row.
    int r = 0;
    if( y > m_rows[0]->getY() ) 
    {
        std::vector<Architecture::Row*>::iterator row_l;
        row_l = std::lower_bound( m_rows.begin(), m_rows.end(), y, Architecture::Row::compare_row() );
        if( row_l == m_rows.end() || (*row_l)->getY() > y ) 
        {
            --row_l;
        }
        r = row_l - m_rows.begin();

        // Should we check actual distance?  The row we find is the one that the bottom of the cell
        // (specified in "y") overlaps with.  But, it could be true that the bottom of the cell is
        // actually closer to the row above...
        if( r < m_rows.size()-1 )
        {  
            if( std::fabs( m_rows[r+1]->getY() - y ) < std::fabs( m_rows[r]->getY() - y ) )
            {
                // Actually, the cell is closer to the row above the one found...
                ++r;
            }
        }
    }

    return r;
}

void Architecture::find_overlapped_rows( double ymin, double ymax, std::vector<Architecture::Row*>& rows )
{
    rows.erase( rows.begin(), rows.end() );
    if( ymin > this->m_ymax || ymax <= this->m_ymin ) {
        return;
    }
    std::vector<Architecture::Row*>::iterator row_l, row_t;
    int b = 0;
    if( ymin > m_rows[0]->getY() ) {
        row_l = std::lower_bound( m_rows.begin(), m_rows.end(), ymin, Architecture::Row::compare_row() );
        if( row_l == m_rows.end() || (*row_l)->getY() > ymin ) {
            --row_l;
        }
        b = row_l - m_rows.begin();
    }
    row_t = std::upper_bound( m_rows.begin(), m_rows.end(), ymax, Architecture::Row::compare_row() );
    int t = row_t - m_rows.begin()-1;
    for( int row_ix = b; row_ix <= t; row_ix++ ) {
        rows.push_back( m_rows[row_ix] );
    }
}

double Architecture::compute_overlap( double xmin1, double xmax1, double ymin1, double ymax1,
        double xmin2, double xmax2, double ymin2, double ymax2 )
{
    if( xmin1 >= xmax2 ) return 0.0;
    if( xmax1 <= xmin2 ) return 0.0;
    if( ymin1 >= ymax2 ) return 0.0;
    if( ymax1 <= ymin2 ) return 0.0;
    double ww = std::min(xmax1,xmax2) - std::max(xmin1,xmin2);
    double hh = std::min(ymax1,ymax2) - std::max(ymin1,ymin2);
    return ww*hh;
}

bool Architecture::power_compatible( Node* ndi, Row* row, bool& flip )
{
    // This routine assumes the node will be placed (start) in the provided row.  Based on this
    // this routine determines if the node and the row are power compatible.  

    flip = false;

    bool okayBot = false;
    bool okayTop = false;

    int spanned = (int)((ndi->getHeight() / row->getH()) + 0.5); // Number of spanned rows.
    int lo = row->m_id;
    int hi = lo + spanned - 1;
    if( hi >= m_rows.size() ) return false; // off the top of the chip.
    if( hi == lo ) 
    {
        // Single height cell.  Actually, is the check for a single height cell any different
        // than a multi height cell?  We could have power/ground at both the top and the
        // bottom...  However, I think this is beyond the current goal...

        int rowBot = m_rows[lo]->m_powerBot;
        int rowTop = m_rows[hi]->m_powerTop;

        int ndBot = ndi->m_powerBot;
        int ndTop = ndi->m_powerTop;
        if( (ndBot == rowBot || ndBot == RowPower_UNK || rowBot == RowPower_UNK) &&
            (ndTop == rowTop || ndTop == RowPower_UNK || rowTop == RowPower_UNK) 
            )
        {
            // Power matches as it is.
            flip = false;
        }
        else
        {
            // Assume we need to flip.
            flip = true;
        }

        return true;
    }
    else
    {
        // Multi-height cell. 
        int rowBot = m_rows[lo]->m_powerBot;
        int rowTop = m_rows[hi]->m_powerTop;

        int ndBot = ndi->m_powerBot;
        int ndTop = ndi->m_powerTop;

        if( (ndBot == rowBot || ndBot == RowPower_UNK || rowBot == RowPower_UNK) &&
            (ndTop == rowTop || ndTop == RowPower_UNK || rowTop == RowPower_UNK) 
            )
        {
            // The power at the top and bottom of the node match either because they are the same
            // or because something is not specified.  No need to flip the node.
            flip = false;
            return true;
        }
        // Swap the node power rails and do the same check.  If we now get a match, things are
        // fine as long as the cell is flipped.
        std::swap( ndBot, ndTop ); 
        if( (ndBot == rowBot || ndBot == RowPower_UNK || rowBot == RowPower_UNK) &&
            (ndTop == rowTop || ndTop == RowPower_UNK || rowTop == RowPower_UNK) 
            )
        {
            flip = true;
            return true;
        }
    }
    return false;
}

void Architecture::create_extra_nodes( Network* network,  RoutingParams* rt )
{
    // This routing creates all extra nodes required by the placer.  This includes:
    // 1. filler nodes for empty gaps in rows...
    // 2. packing peanuts for routing...
    // 3. shreds for macrocells...

    // Filler nodes for gaps in rows...
    for( int i = 0; i < network->m_filler.size(); i++ )
    {
        if( network->m_filler[i] != 0 ) delete network->m_filler[i];
    }
    network->m_filler.clear();

    // Packing peanuts for routability...
    for( int i = 0; i < network->m_peanuts.size(); i++ )
    {
        if( network->m_peanuts[i] != 0 ) delete network->m_peanuts[i];
    }
    network->m_peanuts.clear();

    // Macrocell shreds...  Note this information is stored several ways...
    for( int i = 0; i < network->m_pieces.size(); i++ )
    {
        if( network->m_pieces[i] != 0 ) delete network->m_pieces[i];
    }
    network->m_pieces.clear();
    for( int i = 0; i < network->m_shreds.size(); i++ ) 
    {
        if( network->m_shreds[i] != 0 ) delete network->m_shreds[i]; 
    } 
    network->m_shreds.clear();
    network->m_reverseMap.erase( network->m_reverseMap.begin(), network->m_reverseMap.end() );

    // Set num nodes to ensure proper assignment of ids to nodes...
    network->m_numNodesRaw = network->m_nodes.size();
    network->m_numNodes = network->m_nodes.size();

    create_filler_nodes( network );
    create_peanut_nodes( network, rt );
}

void Architecture::create_filler_nodes( Network* network )
{
    // Assume a continuous placement area, but certain spans might have incomplete rows.  I noticed
    // this one some more recent benchmarks.  Hence, we can crete dummy filler nodes which are 
    // fixed to occupy these spots.  Store them in the network which is arbitrary.

    //    std::cout << "Inserting filler cells... Current node count is " << network->m_numNodes << std::endl;

    for( int r = 0; r < m_rows.size(); r++ ) 
    {
        Architecture::Row* row = m_rows[r];

        double xstrt = row->m_subRowOrigin;
        double xstop = row->m_subRowOrigin + row->m_numSites * row->m_siteSpacing;
        double ystrt = row->m_rowLoc;
        double ystop = row->m_rowLoc + row->m_rowHeight;

        if( xstrt > m_xmin ) {
            Node* nd = new Node();
            nd->setFixed( NodeFixed_FIXED_XY );
            nd->setType( NodeType_FILLER );
            nd->setId( network->m_numNodes );
            nd->setHeight( row->m_rowHeight );
            nd->setWidth( xstrt - m_xmin );
            nd->setY( row->m_rowLoc + 0.5 * row->m_rowHeight );
            nd->setX( 0.5 * (m_xmin + xstrt) );

            network->m_filler.push_back( nd );
            ++network->m_numNodes;
        }
        if( xstop < m_xmax ) {
            Node* nd = new Node();
            nd->setFixed( NodeFixed_FIXED_XY );
            nd->setType( NodeType_FILLER );
            nd->setId( network->m_numNodes );
            nd->setHeight( row->m_rowHeight );
            nd->setWidth( m_xmax - xstop );
            nd->setY( row->m_rowLoc + 0.5 * row->m_rowHeight );
            nd->setX( 0.5 * (xstop + m_xmax) );

            network->m_filler.push_back( nd );
            ++network->m_numNodes;
        }
    }

    //    std::cout << "Filler cells added is " << network->m_filler.size() << ", "
    //         << "Current node count is " << network->m_numNodes 
    //         << std::endl;
}

void Architecture::create_peanut_nodes( Network* network, RoutingParams* rt )
{
    //    std::cout << "Inserting peanut cells... Current node count is " << network->m_numNodes << std::endl;

    if( rt != 0 )
    {
        int numGridX = rt->m_grid_x;
        int numGridY = rt->m_grid_y;
        double tileW = rt->m_tile_size_x;
        double tileH = rt->m_tile_size_y;
        double originX = rt->m_origin_x;
        double originY = rt->m_origin_y;

        network->m_peanuts.resize( numGridX * numGridY );
        int id = 0;
        for( int i = 0; i < numGridX; i++ )
        {
            for( int j = 0; j < numGridY; j++ )
            {
                double x = originX + tileW * i + 0.5 * tileW;
                double y = originY + tileH * j + 0.5 * tileH;

                network->m_peanuts[id] = new Node();
                network->m_peanuts[id]->setFixed( NodeFixed_FIXED_XY );
                network->m_peanuts[id]->setType( NodeType_PEANUT ); // XXX: Or should it be filler???????????
                network->m_peanuts[id]->setId( network->m_numNodes );
                network->m_peanuts[id]->setHeight( 0.0 );
                network->m_peanuts[id]->setWidth( 0.0 );
                network->m_peanuts[id]->setY( y );
                network->m_peanuts[id]->setX( x );

                ++id;
                ++network->m_numNodes;
            }
        }
    }

    //    std::cout << "Peanut cells added is " << network->m_peanuts.size() << ", "
    //         << "Current node count is " << network->m_numNodes 
    //         << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::addCellSpacingUsingTable( 
    int firstEdge, int secondEdge, double sep )
{
    m_cellSpacings.push_back( 
        new Architecture::Spacing( firstEdge, secondEdge, sep  ) 
        );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::addCellPadding( Node* ndi, 
    double leftPadding, double rightPadding )
{
    m_cellPaddings[ndi->getId()] = std::make_pair(leftPadding,rightPadding);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::getCellPadding( Node* ndi, 
    double& leftPadding, double& rightPadding )
{
    std::map<int,std::pair<double,double> >::iterator it;
    if( m_cellPaddings.end() == (it = m_cellPaddings.find( ndi->getId() )) )
    {
        rightPadding = 0;
        leftPadding = 0;
        return false;
    }
    rightPadding = it->second.second;
    leftPadding = it->second.first;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Architecture::getCellSpacing( Node* leftNode, Node* rightNode )
{
    // Return the required separation between the two cells.  We use
    // either spacing tables or padding information, or both.  If 
    // we use both, then we return the largest spacing.
    //
    // I've updated this as well to account for the situation where
    // one of the provided cells is null.  Even in the case of null,
    // I suppose that we should account for padding; e.g., we might
    // be at the end of a segment.
    double retval = 0.;
    if( m_useSpacingTable )
    {
        // Don't need this if one of the cells is null.
        int i1 = (leftNode == 0) ? -1 : leftNode->getRightEdgeType();
        int i2 = (rightNode == 0) ? -1 : rightNode->getLeftEdgeType();
        retval = std::max( retval, getCellSpacingUsingTable( i1, i2 ) );
    }
    if( m_usePadding )
    {
        // Separation is padding to the right of the left cell plus 
        // the padding to the left of the right cell.
        std::map<int,std::pair<double,double> >::iterator it;

        double separation = 0.;
        if( leftNode != 0 )
        {
            if( m_cellPaddings.end() != (it = m_cellPaddings.find( leftNode->getId() )) )
            {
                separation += it->second.second;
            }
        }
        if( rightNode != 0 )
        {
            if( m_cellPaddings.end() != (it = m_cellPaddings.find( rightNode->getId() )) )
            {
                separation += it->second.first ;
            }
        }
        retval = std::max( retval, separation );
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Architecture::getCellSpacingUsingTable( int i1, int i2 )
{
    // In the event that one of the left or right indices is
    // void (-1), return the worst possible spacing.  This
    // is very pessimistic, but will ensure all issues are
    // resolved.

    double spacing = 0.0;

    if( i1 == -1 && i2 == -1 )
    {
        for( int i = 0; i < m_cellSpacings.size(); i++ )
        {
            Architecture::Spacing* ptr = m_cellSpacings[i];
            spacing = std::max( spacing, ptr->getSeparation() );
        }
        return spacing;
    }
    else if( i1 == -1 )
    {
        for( int i = 0; i < m_cellSpacings.size(); i++ )
        {
            Architecture::Spacing* ptr = m_cellSpacings[i];
            if( ptr->getFirstEdge() == i2 || ptr->getSecondEdge() == i2 )
            {
                spacing = std::max( spacing, ptr->getSeparation() );
            }
        }
        return spacing;
    }
    else if( i2 == -1 )
    {
        for( int i = 0; i < m_cellSpacings.size(); i++ )
        {
            Architecture::Spacing* ptr = m_cellSpacings[i];
            if( ptr->getFirstEdge() == i1 || ptr->getSecondEdge() == i1 )
            {
                spacing = std::max( spacing, ptr->getSeparation() );
            }
        }
        return spacing;
    }

    for( int i = 0; i < m_cellSpacings.size(); i++ )
    {
        Architecture::Spacing* ptr = m_cellSpacings[i];
        if( (ptr->getFirstEdge() == i1 && ptr->getSecondEdge() == i2) || 
            (ptr->getFirstEdge() == i2 && ptr->getSecondEdge() == i1) ) 
        {
            spacing = std::max( spacing, ptr->getSeparation() );
        }
    }
    return spacing;
}

void Architecture::clear_edge_type()
{
    for( int i = 0; i < m_edgeTypes.size(); i++ )
    {
        if( m_edgeTypes[i].first != 0 ) delete[] m_edgeTypes[i].first;
    }
    m_edgeTypes.clear();
}

void Architecture::init_edge_type()
{
    clear_edge_type();
    char* newName = new char[strlen("DEFAULT")+1];
    strcpy(newName,"DEFAULT");
    m_edgeTypes.push_back( std::pair<char*,int>( newName, EDGETYPE_DEFAULT ) );
}

int Architecture::add_edge_type( const char* name )
{
    for( int i = 0; i < m_edgeTypes.size(); i++ )
    {
        std::pair<char*, int>& temp = m_edgeTypes[i];
        if( strcmp( temp.first, name ) == 0 ) 
        {
            // Edge type already exists.
            return temp.second;
        }
    }
    char* newName = new char[strlen(name)+1];
    strcpy(newName,name);
    int n = m_edgeTypes.size();
    m_edgeTypes.push_back( std::pair<char*,int>( newName, n ) );
    return n;
}

Architecture::Row::Row():
    m_rowLoc(0),
    m_rowHeight(0),
    m_siteWidth(0),
    m_siteSpacing(0),
    m_subRowOrigin(0),
    m_numSites(0)
{
    m_powerTop = RowPower_UNK;
    m_powerBot = RowPower_UNK;
}

Architecture::Row::~Row()
{
}

Architecture::Spacing::Spacing( int i1, int i2, double sep ):
    m_i1(i1),m_i2(i2),m_sep(sep)
{
}

Architecture::Spacing::~Spacing() 
{
}

Architecture::Region::Region( void ):
    m_id(-1),
    m_xmin(  std::numeric_limits<double>::max() ),
    m_ymin(  std::numeric_limits<double>::max() ),
    m_xmax( -std::numeric_limits<double>::max() ),
    m_ymax( -std::numeric_limits<double>::max() )
{
}

Architecture::Region::~Region( void )
{
    m_rects.clear();
}

} // namespace aak

