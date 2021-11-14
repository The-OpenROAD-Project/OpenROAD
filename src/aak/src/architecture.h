
////////////////////////////////////////////////////////////////////////////////
// File: architecture.h
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <limits>
#include "orientation.h"
#include "symmetry.h"
#include "rectangle.h"


namespace aak
{

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class RoutingParams;
class Architecture;
class Network;
class Node;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class Architecture
{
// This class represents information about the layout area.  It's not as advanced
// as the one used in the legalizer, but it's sufficient for now (we can change 
// it later as required)...  Right now, we only keep track of row information and
// not explicit site information... 
//
// XXX: What about sub-rows being introduced????
// XXX: Should routing information be stored here????

public:
    class Row;
    class Spacing;
    class Region;

public:
    Architecture();
    virtual ~Architecture();

    int getNumRows( void ) const { return m_rows.size(); }
    Architecture::Row* getRow( int r ) const { return m_rows[r]; }

    bool isSingleHeightCell( Node* ndi ) const;
    bool isMultiHeightCell( Node* ndi ) const;

    bool post_process();
    //void mark_sites_unoccupied();
    //void mark_sites_occupied( Network* network );
    double compute_overlap( double xmin1, double  xmax1, double ymin1, double ymax1, double xmin2, double xmax2, double ymin2, double ymax2 );
    bool uniform();

    void create_extra_nodes(Network* network, RoutingParams* rt);

    void find_overlapped_rows( double ymin, double ymax, std::vector<Architecture::Row*>& rows );
    int find_closest_row( double y );


    void clear_edge_type();
    void init_edge_type();
    int add_edge_type( const char* name );

    double  getMinX( void ) const { return m_xmin; }
    double  getMaxX( void ) const { return m_xmax; }
    double  getMinY( void ) const { return m_ymin; }
    double  getMaxY( void ) const { return m_ymax; }

    void    setMinX( double xmin ) { m_xmin = xmin; }
    void    setMaxX( double xmax ) { m_xmax = xmax; }
    void    setMinY( double ymin ) { m_ymin = ymin; }
    void    setMaxY( double ymax ) { m_ymax = ymax; }

    double  getWidth( void ) const { return m_xmax - m_xmin; }
    double  getHeight( void ) const { return m_ymax - m_ymin; }

    bool    power_compatible( Node* ndi, Row* row, bool& flip );

    // Well, sometimes I see padding used and other times I see
    // spacing tables used.  I don't think both should be used.
    // We need to give priority to one over the other in the
    // event that both are set to true!

    // Using tables...
    void    setUseSpacingTable( bool val = true ) { m_useSpacingTable = val; }
    bool    getUseSpacingTable( void ) const { return m_useSpacingTable; }
    void    clearSpacingTable( void );
    double  getCellSpacingUsingTable( int firstEdge, int secondEdge );
    void    addCellSpacingUsingTable( int firstEdge, int secondEdge, double sep );

    // Using padding...
    void    setUsePadding( bool val = true ) { m_usePadding = val; }
    bool    getUsePadding( void ) const { return m_usePadding; }
    void    addCellPadding( Node* ndi, double leftPadding, double rightPadding );
    bool    getCellPadding( Node* ndi, double& leftPadding, double& rightPadding );

    double  getCellSpacing( Node* leftNode, Node* rightNode );

protected:
    void create_filler_nodes( Network* network );
    void create_peanut_nodes( Network* network, RoutingParams* rt );
public:
    // Rows...
    std::vector<Row*>       m_rows;

    // Die...
    double                  m_xmin;
    double                  m_xmax;
    double                  m_ymin;
    double                  m_ymax;

    // Regions...
    std::vector<Region*>    m_regions;
    std::vector<int>        m_numNodesInRegion;

    bool                    m_useSpacingTable;
    bool                    m_usePadding;
    std::vector<std::pair<char*, int> > 
                            m_edgeTypes;
    std::vector<Spacing*>   m_cellSpacings;
    std::map<int,std::pair<double,double> >
                            m_cellPaddings; // Padding to left,right.
};

class Architecture::Spacing
{
public:
    Spacing( int i1, int i2, double sep );
    virtual ~Spacing();

    int getFirstEdge( void )    const { return m_i1; }
    int getSecondEdge( void )   const { return m_i2; }
    double getSeparation( void )   const { return m_sep; }
public:
    int m_i1;
    int m_i2;
    double m_sep;
};

const unsigned RowPower_UNK             = 0x00000001;
const unsigned RowPower_VDD             = 0x00000002;
const unsigned RowPower_VSS             = 0x00000004;

class Architecture::Row
{
public:

    Row();
    virtual ~Row();

    inline double getY() { return m_rowLoc; }
    inline double getH() { return m_rowHeight; }
public:
    // XXX: Ignores site orientation and symmetry right now...
    double m_rowLoc; // Y-location of the row.
    double m_rowHeight; // Height of the row.
    double m_siteWidth; // Width of sites in the row.
    double m_siteSpacing; // Spacing between sites in the row. XXX: Likely assumed to be the same as the width...
    double m_subRowOrigin; // Starting X location (xmin) of the row.
    unsigned m_siteOrient;    // Orientation of sites in the row.
    unsigned m_siteSymmetry; // Symmetry of sites in the row.  Symmetry allows for certain orientations...
    int m_numSites; // Number of sites...  Ending X location (xmax) is = m_subRowOrigin + m_numSites * m_siteSpacing;
    int m_id; // Every row  needs an id...  Filled in after sorting.

    // The following is to try and monitor voltages at the top and bottom of the row.
    // It is from the ICCAD 2017 contest.  I'm not sure it is the most general, but 
    // I think it will work for the contest.
    int m_powerTop;
    int m_powerBot;


    struct compare_row
    {
        inline bool operator()( Architecture::Row*& s, double i ) const {
              return s->getY() < i;
        }
        inline bool operator()( double i, Architecture::Row*& s ) const {
              return i < s->getY();
        }
    };
};

struct SortRow
{
    inline bool operator() ( Architecture::Row *p, Architecture::Row *q ) const 
    {
        return p->getY() < q->getY();
    } 
};

class Architecture::Region
{
public:
    Region();
    virtual ~Region();
public:
    int             m_id;       // Artificial ID of the region.

    // Box around all sub-rectangles.
    double          m_xmin;
    double          m_xmax;
    double          m_ymin;
    double          m_ymax;

    // Rectangles forming the rectilinear region.
    std::vector<Rectangle> m_rects;
};

} // namespace aak
