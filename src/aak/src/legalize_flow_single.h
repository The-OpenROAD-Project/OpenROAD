

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <deque>
#include "architecture.h"
#include "network.h"
#include "router.h"
#include "plotgnu.h"

////////////////////////////////////////////////////////////////////////////////
// Foward declarations.
////////////////////////////////////////////////////////////////////////////////


namespace aak
{
class DetailedSeg;
class DetailedMgr;


class FlowLegalizerPath;
class FlowLegalizerEdge;
class FlowLegalizerNode;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////


class FlowLegalizerParams
{
public:
    FlowLegalizerParams():
        m_ut( 1.0 ),
        m_global( false ),
        m_useWL( false ),
        m_tol( 10.0 ),
        m_outerIter( 40 ),
        m_innerIter( 100 )
    {}
public:
    double          m_ut;
    bool            m_global;
    bool            m_useWL;
    double          m_tol;
    int             m_outerIter;
    int             m_innerIter;
};

class FlowLegalizerPath
{
public:
    FlowLegalizerPath() {}
public:
    std::vector<FlowLegalizerNode*>     m_path;
};

class FlowLegalizerNode
{
public:
    FlowLegalizerNode():
        m_binId(-1),
        m_segId(-1), 
        m_xmin(  std::numeric_limits<double>::max() ),
        m_xmax( -std::numeric_limits<double>::max() ),
        m_ymin(  std::numeric_limits<double>::max() ),
        m_ymax( -std::numeric_limits<double>::max() ),
        m_travId( -1 ),
        m_parent( 0 )
    {}
    virtual ~FlowLegalizerNode( void )
    {
        deleteEdges();
    }

    void        deleteEdges( void );

    void        setUt( double ut ) { m_ut = ut; }
    double      getUt( void ) const { return m_ut; }

    double      getCap( void )
    {
        return (m_xmax - m_xmin) * m_ut;
    }
    double      getOcc( void ) const { return m_util; }
    double      setOcc( double occ ) { m_util = occ; return m_util; }
    double      addOcc( double occ ) { m_util += occ; return m_util; }
    double      addOcc( Node* ndi )
    {
        m_util += ndi->getWidth();
        return m_util;
    }
    double      subOcc( double occ ) { m_util -= occ; return m_util; }
    double      subOcc( Node* ndi )
    {
        m_util -= ndi->getWidth();
        return m_util;
    }

    double      getOccLast( void ) const { return m_utilLast; }
    void        setOccLast( double occ ) { m_utilLast = occ; }
    
    void        setGap( double gapu ) { m_gapu = gapu; }
    double      getGap( void ) const { return m_gapu; }

    void        setMinX( double xmin ) { m_xmin = xmin; }
    void        setMaxX( double xmax ) { m_xmax = xmax; }
    void        setMinY( double ymin ) { m_ymin = ymin; }
    void        setMaxY( double ymax ) { m_ymax = ymax; }

    void        setBinId( int binId ) { m_binId = binId; }
    int         getBinId( void ) const { return m_binId; }

    void        setRegId( int regId ) { m_regId = regId; }
    int         getRegId( void ) const { return m_regId; }

    void        setSegId( int segId ) { m_segId = segId; }
    int         getSegId( void ) const { return m_segId; }

    void        setMinRow( int rmin ) { m_rmin = rmin; }
    void        setMaxRow( int rmax ) { m_rmax = rmax; }
    int         getMinRow( void ) { return m_rmin; }
    int         getMaxRow( void ) { return m_rmax; }

    double      getMinX( void ) { return m_xmin; }
    double      getMaxX( void ) { return m_xmax; }
    double      getMinY( void ) { return m_ymin; }
    double      getMaxY( void ) { return m_ymax; }
    double      getCenterX( void ) { return (0.5 * (m_xmax + m_xmin)); }
    double      getCenterY( void ) { return (0.5 * (m_ymax + m_ymin)); }
    double      getWidth( void )  { return m_xmax-m_xmin; }
    double      getHeight( void ) { return m_ymax-m_ymin; }

    void        setTravId( int travId ) { m_travId = travId; }
    int         getTravId( void ) const { return m_travId; }
public:
    int                             m_binId;
    int                             m_segId;
    int                             m_regId;

    // For search.
    std::vector<FlowLegalizerEdge*>  
                                    m_outEdges;
    FlowLegalizerNode*              m_parent;
    int                             m_demand;
    double                          m_dist;

protected:
    double                          m_xmin;
    double                          m_xmax;
    double                          m_ymin;
    double                          m_ymax;
    int                             m_rmin;
    int                             m_rmax;
    double                          m_ut;   // To reduce utilization locally.
    double                          m_util;
    double                          m_utilLast;
    double                          m_gapu; // Presently ignored.

    int                             m_travId;   // For searching.
};

class FlowLegalizerEdge
{
public:
    FlowLegalizerEdge( FlowLegalizerNode* srcPtr, FlowLegalizerNode* snkPtr ):m_srcPtr(srcPtr),m_snkPtr(snkPtr)
    {}
public:
    FlowLegalizerNode*              m_srcPtr;
    FlowLegalizerNode*              m_snkPtr;

    // For path searching.
    double                          m_cost;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class FlowLegalizer 
{
public:
    FlowLegalizer( FlowLegalizerParams& params );
    virtual ~FlowLegalizer();

    void legalize( DetailedMgr* mgr );
    void legalize( DetailedMgr* mgr, int regId );
    void plotter( PlotGnu* plotter ) { m_plotter = plotter; }





    bool Run( void );


protected:
    // Various classes used internally.


    // Comparision routines.
    struct compareBlockages
    {
        inline bool operator()( std::pair<double,double> i1, std::pair<double,double> i2 ) const
        {
            if( i1.first == i2.first )
            {
                return i1.first < i2.first;
            }
            return i1.first < i2.first;
        }
    };



    struct compareFractionalNodesId
    {
        inline bool operator() ( std::pair<Node*,int>& p, std::pair<Node*,int>& q ) const
        {
            if( p.first->getId() == q.first->getId() )
                return p.second > q.second;
            return p.first->getId() < q.first->getId();
        }
    };

protected:

    // For performing flow.
    void            collectSingleHeightCellStatistics( void );
    void            initBinWidth( void );
    void            extractBins( void );
    void            extractBins1( void );
    void            modifyBins( void );
    void            connectBins( void );
    void            disconnectBins( void );
    int             mergeBins( void );
    bool            adjacentBins( FlowLegalizerNode* a, FlowLegalizerNode* b );
    void            findBlockagesForMultiHeightCells( void );
    int             countOverfilledBins( void );

    bool            knapsack( std::vector<double>& c, std::vector<int>& a, int W, std::vector<int>& x );
    bool            subsetSum( std::vector<int>& a, int n, int sum );

    void            assignCellsToBins( void );
    FlowLegalizerNode* findClosestBin( DetailedSeg* segPtr, Node* ndi );

    bool            bulkFlow( void );
    bool            setupFlowLemon( bool& didFindPaths, int verbosity = 0 );
    void            pathFlow( void );
    bool            setupFlowPaths( bool& didFindPaths, int verbosity = 0 );
    int             computeFlow1( FlowLegalizerNode* srcPtr, FlowLegalizerNode* snkPtr, 
                        std::vector<Node*>& nodes, int demand, double& hpwl, double& disp,
                        int& nMoveLimit, int& nDispLimit, int& nFractional, int verbosity = 0 );
    int             moveFlow1( FlowLegalizerNode* srcPtr, FlowLegalizerNode* snkPtr, 
                        std::vector<Node*>& nodes, int demand, int verbosity = 0 );
    void            findOverfilledBins( std::vector<FlowLegalizerNode*>& overfilled );
    void            getNeighbours( FlowLegalizerNode* v, int desired,
                        std::vector<std::pair<FlowLegalizerNode*,double> >& neighbours );

    void            moveCellsBetweenBins( FlowLegalizerNode* oldPtr,
                        int leftRightTol, double offsetTol );
    void            removeBinOverlap( void );
    void            findSubset( std::vector<Node*>& nodes, double occ, double flow, double cap,
                            std::vector<Node*>& swaps );


public:




    class Clump
    {
    public:
        Clump() {}
        virtual ~Clump() {}
    public:
        int m_id;
        std::vector<Node*> m_nodes;
        double m_width;
        double m_wposn;
        double m_weight;
        double m_posn;
    };



protected:

    void doClumping( std::vector<Node*>& nodes, double xmin, double xmax, bool debug = false );
    void doClumping( Clump* r, std::vector<Clump>& clumps, double xmin, double xmax, bool debug = false );

public:
    double getDisp( const Node* ndi, double x, double y )
    {
        // XXX: Flow does not actually move any cells until the flow is done. 
        // Therefore, should we be minimizing displacement from the cell's
        // original positions or from the positions at the start of the flow?
        // I think it should be the start of the flow since cells might have
        // already been moved...
        //double dx = x - ndi->getOrigX();
        //double dy = y - ndi->getOrigY();
        double dx = x - ndi->getX();;
        double dy = y - ndi->getY();
        // For L1...
#if !defined(USE_QUADRATIC_DISPLACEMENT)
        double disp = std::fabs( dx ) + std::fabs( dy );
#else
        // For Euclidean...
        //double disp = std::sqrt( dx*dx + dy*dy );
        // For L2...
        double disp = dx*dx + dy*dy;
#endif

        return disp;
    }

    double getMaxDisp( void )
    {
#if !defined(USE_QUADRATIC_DISPLACEMENT)
        return m_maxDisplacement;
#else
        return m_maxDisplacement * m_maxDisplacement;
#endif
    }


protected:
    // Standard stuff.
    FlowLegalizerParams&                    m_params;

    // Detailed manager.
    DetailedMgr*                            m_mgr;

    // Other.
    Architecture*                           m_arch;
    Network*                                m_network;
    RoutingParams*                          m_rt;

    // Plotting.
    PlotGnu*                                m_plotter;

    // Misc.
    double                                  m_singleRowHeight;
    int                                     m_numSingleHeightRows;
    double                                  m_avgCellWidth;
    double                                  m_medCellWidth;
    double                                  m_maxCellWidth;
    double                                  m_minCellWidth;
    std::vector<Node*>                      m_singleHeightCells; // Cells within the region.

    // Bin information.
    double                                  m_binWidth;
    double                                  m_binHeight;
    std::vector<FlowLegalizerNode*>         m_bins;
    std::vector<std::vector<Node*> >        m_cellsInBin; // Fractional.
    std::vector<std::vector<FlowLegalizerNode*> >   
                                            m_binsInSeg;
    std::vector<std::vector<FlowLegalizerNode*> > 
                                            m_binsInRow;

    // Following supports fractional assignment.
    std::vector<std::pair<int,int> >        m_areaToBins;
    std::vector<std::pair<FlowLegalizerNode*,FlowLegalizerNode*> > 
                                            m_cellToBins;

    // Which region.
    int                                     m_curRegionId;

    // Other.
    std::vector<FlowLegalizerNode*>         m_overfilledBins;
    int                                     m_moveId;
    std::vector<int>                        m_moveCellMask;
    std::vector<int>                        m_moveCellCounter;
    int                                     m_moveCounterLimit;
    std::vector<double>                     m_moveFracMask;
    double                                  m_maxDisplacement;

    std::vector<std::vector<std::pair<double,double> > > 
                                            m_multi_blockages;
};


} // namespace aak

