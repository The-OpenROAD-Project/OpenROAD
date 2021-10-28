

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Description:
// Primarily for maintaining the segments.

#pragma once




////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <deque>
#include "architecture.h"
#include "network.h"
#include "router.h"
#include "rectangle.h"
#include "utility.h"


namespace aak
{

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class DetailedSeg;


////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class DetailedMgr
{
public:
    DetailedMgr( Architecture* arch, Network* network, RoutingParams* rt );
    virtual ~DetailedMgr( void );

    void                cleanup( void );
    void                setup( void );

    Architecture*       getArchitecture( void ) const { return m_arch; }
    Network*            getNetwork( void ) const { return m_network; }
    RoutingParams*      getRoutingParams( void ) const { return m_rt; }

    void                setupObstaclesForDrc( void );

    void                findBlockages( std::vector<Node*>& fixedCells, bool includeRouteBlockages = true );
    void                findRegionIntervals( int regId, std::vector<std::vector<std::pair<double,double> > >& intervals );

    void                findSegments( void );
    DetailedSeg*        findClosestSegment( Node* nd );
    void                findClosestSpanOfSegmentsDfs( Node* ndi, DetailedSeg* segPtr, 
                                double xmin, double xmax, int bot, int top,
                                std::vector<DetailedSeg*>& stack, 
                                std::vector<std::vector<DetailedSeg*> >& candidates );
    bool                findClosestSpanOfSegments( Node *nd, std::vector<DetailedSeg*>& segments );

    void                assignCellsToSegments( std::vector<Node*>& nodesToConsider );
    int                 checkSegments( double& worst );
    int                 checkOverlapInSegments( int max_err_n = 0 );
    int                 checkEdgeSpacingInSegments( int max_err_n = 0 );
    int                 checkSiteAlignment( int max_err_n = 0 );
    int                 checkRowAlignment( int max_err_n = 0 );
    int                 checkRegionAssignment( int max_err_n = 0 );
    bool                checkPlacement( void );

    void                removeCellFromSegmentTest( Node* nd, int seg, double& util, double& gapu );
    void                addCellToSegmentTest( Node* nd, int seg, double x, double& util, double& gapu );
    void                removeCellFromSegment( Node* nd, int seg );
    void                addCellToSegment( Node* nd, int seg );
    double              getCellSpacing( Node* ndl, Node* ndr, bool checkPinsOnCells );

    void                collectSingleHeightCells( void );
    void                collectMultiHeightCells( void );
    void                moveMultiHeightCellsToFixed( void );
    void                collectFixedCells( void );
    void                collectWideCells( void );

    void                restoreOriginalPositions( void );
    void                recordOriginalPositions( void );
    void                restoreOriginalDimensions( void );
    void                recordOriginalDimensions( void );

    void                restoreBestPositions( void );
    void                recordBestPositions( void );

    void                resortSegments( void );
    void                resortSegment( DetailedSeg* segPtr );
    void                removeAllCellsFromSegments( void );

    inline double       getOrigX( Node* nd ) const { return m_origX[nd->getId()]; }
    inline double       getOrigY( Node* nd ) const { return m_origY[nd->getId()]; }
    inline double       getOrigW( Node* nd ) const { return m_origW[nd->getId()]; }
    inline double       getOrigH( Node* nd ) const { return m_origH[nd->getId()]; }

    bool                isNodeAlignedToRow( Node* nd );

    double              measureMaximumDisplacement( bool print, bool& violated );
    void                removeOverlapMinimumShift( void );

    inline int          getNumSingleHeightRows( void ) const { return m_numSingleHeightRows; }
    inline int          getSingleRowHeight( void ) const { return m_singleRowHeight; }

    void                getSpaceAroundCell( int seg, int ix, double& space, double& larger, int limit = 3 );
    void                getSpaceAroundCell( int seg, int ix, double& space_left, double& space_right, 
                            double& large_left, double& large_right, int limit = 3 );

    bool                fixSegments( void );
    void                moveCellsBetweenSegments( int iteration );
    void                pushCellsBetweenSegments( int iteration );
    void                moveCellsBetweenSegments( DetailedSeg* segment, 
                            int leftRightTol, double offsetTol, double scoringTol );

    void                removeSegmentOverlapSingle( int regId = -1 );
    void                removeSegmentOverlapSingleInner( std::vector<Node*>& nodes, double l, double r, int rowId );

    void                debugSegments( void );

    bool alignPos( Node* ndi, double& xi, double xl, double xr );
    bool tryMove1( Node* ndi, double xi, double yi, int si, double xj, double yj, int sj );
    bool tryMove2( Node* ndi, double xi, double yi, int si, double xj, double yj, int sj );
    bool tryMove3( Node* ndi, double xi, double yi, int si, double xj, double yj, int sj );

    bool trySwap1( Node* ndi, double xi, double yi, int si, double xj, double yj, int sj );

    void acceptMove( void );
    void rejectMove( void );
    void printMove( void );



public:
    struct compareBlockages
    {
        inline bool operator()( std::pair<double,double> i1, std::pair<double,double> i2 ) const
        {
            if( i1.first == i2.first )
            {
                return i1.second < i2.second;
            }
            return i1.first < i2.first;
        }
    };

    struct compareNodesX
    {
        inline bool operator() ( Node* p, Node* q ) const
        {
            return p->getX() < q->getX();
        }
        inline bool operator()( Node*& s, double i ) const 
        {
            return s->getX() < i;
        }
        inline bool operator()( double i, Node*& s ) const 
        {
            return i < s->getX();
        }
    };

    struct compareNodesL
    {
        inline bool operator() ( Node* p, Node* q ) const
        {
            return p->getX() - 0.5 * p->getWidth() < q->getX() - 0.5 * q->getWidth();
        }
    };

protected:
    // Standard stuff.
    Architecture*           m_arch;
    Network*                m_network;
    RoutingParams*          m_rt;

    // Info about rows.
    int                     m_numSingleHeightRows;
    double                  m_singleRowHeight;

public:
    // Blockages and segments.
    std::vector<std::vector<std::pair<double,double> > > m_blockages;
    std::vector<std::vector<Node*> >            m_cellsInSeg;
    std::vector<std::vector<DetailedSeg*> >     m_segsInRow;
    std::vector<DetailedSeg*>                   m_segments;
    std::vector<std::vector<DetailedSeg*> >     m_reverseCellToSegs;

    // For short and pin access stuff...
    std::vector<std::vector<std::vector<Rectangle> > > m_obstacles;

    // Random number generator.
    Placer_RNG*                                 m_rng;

    // Info about cells.
    std::vector<Node*>                          m_singleHeightCells; // Single height cells.
    std::vector<std::vector<Node*> >            m_multiHeightCells; // Multi height cells by height.
    std::vector<Node*>                          m_fixedCells;    // Fixed; filler, macros, temporary, etc.
    std::vector<Node*>                          m_fixedMacros;   // Fixed; only macros.
    std::vector<Node*>                          m_wideCells;     // Wide movable cells.  Can be single of multi.

    // Info about original cell positions and dimensions.
    std::vector<double>                         m_origX;
    std::vector<double>                         m_origY;
    std::vector<double>                         m_origW;
    std::vector<double>                         m_origH;

    std::vector<double>                         m_bestX;
    std::vector<double>                         m_bestY;

    std::vector<Rectangle>                      m_boxes;

    // For generating a move list...
    std::vector<double>                         m_curX;
    std::vector<double>                         m_curY;
    std::vector<double>                         m_newX;
    std::vector<double>                         m_newY;
    std::vector<unsigned>                       m_curOri;
    std::vector<unsigned>                       m_newOri;
    std::vector<std::vector<int> >              m_curSeg;
    std::vector<std::vector<int> >              m_newSeg;
    std::vector<Node*>                          m_movedNodes;
    int                                         m_nMoved;
    int                                         m_moveLimit;
};



} // namespace akk
