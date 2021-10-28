
////////////////////////////////////////////////////////////////////////////////
// File: router.cxx
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include "network.h"
#include "architecture.h"
#include "router.h"
#include "utility.h"

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////


namespace aak
{

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
void RoutingParams::postProcess()
{
}

double RoutingParams::get_spacing( int layer,
        double xmin1, double xmax1, double ymin1, double ymax1,
        double xmin2, double xmax2, double ymin2, double ymax2 
        )
{
    double ww = std::max( std::min( ymax1-ymin1, xmax1-xmin1 ), std::min( ymax2-ymin2, xmax2-xmin2 ) );

    // Parallel run-length in the Y-dir.  Will be zero if the objects are above or below each other.
    double py = std::max( 0.0, std::min( ymax1, ymax2 ) - std::max( ymin1, ymin2 ) ); 

    // Parallel run-length in the X-dir.  Will be zero if the objects are left or right of each other.
    double px = std::max( 0.0, std::min( xmax1, xmax2 ) - std::max( xmin1, xmin2 ) ); 

    return get_spacing( layer, ww, std::max( px, py ) ); 
}

double RoutingParams::get_spacing( int layer, double width, double parallel )
{
    std::vector<double>& w = m_spacingTableWidth[layer];
    std::vector<double>& p = m_spacingTableLength[layer];

    if( w.size() == 0 || p.size() == 0 )
    {
        // This means no spacing table is present.  So, return the minimum wire spacing for the layer...
        return m_wire_spacing[layer];
    }

    int i = w.size()-1;
    while( i > 0 && width    <= w[i] ) i--;
    int j = p.size()-1;
    while( j > 0 && parallel <= p[j] ) j--;

    return m_spacingTable[layer][i][j];
}

double RoutingParams::get_maximum_spacing( int layer )
{
    std::vector<double>& w = m_spacingTableWidth[layer];
    std::vector<double>& p = m_spacingTableLength[layer];

    if( w.size() == 0 || p.size() == 0 )
    {
        // This means no spacing table is present.  So, return the minimum wire spacing for the layer...
        return m_wire_spacing[layer];
    }

    int i = w.size()-1;
    int j = p.size()-1;

    return m_spacingTable[layer][i][j];
}


} // namespace aak
