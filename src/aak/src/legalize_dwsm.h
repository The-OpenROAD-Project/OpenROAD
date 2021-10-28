

// A partial implementation of the paper Kris and I wrote a long time ago on
// using whitespace management for legalization.


#pragma once


//////////////////////////////////////////////////////////////////////////////
// Includes.
//////////////////////////////////////////////////////////////////////////////
#include <cmath>
#include <map>
#include <vector>
#include <set>
#include "architecture.h"
#include "network.h"
#include "router.h"
#include "min_movement_floorplanner.h"

namespace aak 
{

//////////////////////////////////////////////////////////////////////////////
// Forward declarations.
//////////////////////////////////////////////////////////////////////////////
class DetailedSeg;
class DetailedMgr;

//////////////////////////////////////////////////////////////////////////////
// Classes.
//////////////////////////////////////////////////////////////////////////////


class DwsmLegalizerParams
{
public:
    DwsmLegalizerParams( void ):
        m_smallProblemThreshold( 5000 ),
        m_useMinShift( true ),
        m_useMMF( true ),
        m_useRefinement( true )
       {
            ;
       }
public:
    int         m_smallProblemThreshold;    
    bool        m_useMinShift;
    bool        m_useMMF;
    bool        m_useRefinement;
};

class DwsmLegalizer
{
public:
    DwsmLegalizer( DwsmLegalizerParams& params );
    virtual ~DwsmLegalizer( void );

    int legalize( DetailedMgr& mgr );
    int legalize( DetailedMgr& mgr, int regId );

    int lal( DetailedMgr& mgr );
    int lal( DetailedMgr&, int regId );

protected:
    class Box
    {
    public:
        Box():m_l(0),m_r(0),m_b(0),m_t(0) { m_unplaced = 0; };
        Box(int l, int r, int b, int t):m_l(l),m_r(r),m_b(b),m_t(t) { m_unplaced = 0; }
        Box(const Box& other):m_l(other.m_l),m_r(other.m_r),m_b(other.m_b),m_t(other.m_t),
            m_unplaced(other.m_unplaced) {}
        Box& operator=(const Box& other ) {
            if( this != &other ) {
                m_l = other.m_l;
                m_r = other.m_r;
                m_b = other.m_b;
                m_t = other.m_t;
                m_unplaced = other.m_unplaced;
            }
            return *this;
        }
    public:
        int m_l, m_r, m_b, m_t;
        int m_unplaced;
    };

    struct compareBoxes
    {
    public:
        inline bool operator()( const Box& box_i, const Box& box_j ) const
        {
            int w_i = (int)( box_i.m_r - box_i.m_l + 1 );
            int h_i = (int)( box_i.m_t - box_i.m_b + 1 );
            int w_j = (int)( box_j.m_r - box_j.m_l + 1 );
            int h_j = (int)( box_j.m_t - box_j.m_b + 1 );

            int a_i = w_i * h_i;
            int a_j = w_j * h_j;
            if( a_i == a_j ) {
                if( w_i == w_j ) {
                    return h_i > h_j;
                }
                return w_i > w_j;
            }
            return a_i > a_j;
        }
    };

    class Block
    {
    // Blocks can be rectangles that need to be legalized, actual blocks that
    // need to be legalized, etc.  It's a "homogeneous" approach to handling
    // the problem.
    public:
        Block( Node* ndi );
        Block( double xmin, double xmax, double ymin, double ymax );
        virtual ~Block( void );

        void        setId( int id )             { m_id = id; }
        int         getId( void ) const         { return m_id; }

        void        setXmin( double x )         { m_xmin = x; }
        void        setXmax( double x )         { m_xmax = x; }
        void        setYmin( double y )         { m_ymin = y; }
        void        setYmax( double y )         { m_ymax = y; }

        double      getXmin( void ) const       { return m_xmin; }
        double      getXmax( void ) const       { return m_xmax; }
        double      getYmin( void ) const       { return m_ymin; }
        double      getYmax( void ) const       { return m_ymax; }

        void        setXinitial( double x )     { m_initialX = x; }
        void        setYinitial( double y )     { m_initialY = y; }

        double      getXinitial( void ) const   { return m_initialX; }
        double      getYinitial( void ) const   { return m_initialY; }

    public:
        std::vector<Block*>     m_blocks;
        Block*                  m_parent;

        int                     m_id;

        double                  m_xmin;
        double                  m_xmax;
        double                  m_ymin;
        double                  m_ymax;

        Node*                   m_node;
        double                  m_initialX;
        double                  m_initialY;
        bool                    m_isPlaced;
        bool                    m_isFixed;
    };


    class BlockSorter
    {
    public:
        enum Type { L, R, T, B, BL, BR, TL, TR, A, P };
        BlockSorter( double xmin = 0., double xmax = 0., double ymin = 0., double ymax = 0., Type t = BlockSorter::L ):
            _xmin(xmin),_xmax(xmax),
            _ymin(ymin),_ymax(ymax),
            _t(t)
        {}
        inline bool operator()( const Block* ptri, const Block* ptrj ) const
        {
            double  wi = ptri->getXmax() - ptri->getXmin();
            double  wj = ptrj->getXmax() - ptrj->getXmin();
            double  hi = ptri->getYmax() - ptri->getYmin();
            double  hj = ptrj->getYmax() - ptrj->getYmin();
            double  ai  = wi * hi;
            double  aj  = wj * hj;

            if     ( _t == BlockSorter::L  ) 
            {
                if( ptri->getXmin() == ptrj->getXmin() ) {
                    if( ptri->getYmin() == ptrj->getYmin() ) {
                        if( ai == aj ) {
                            if( hi == hj ) {
                                return wi > wj;
                            }
                            return hi > hj;
                        }
                        return ai > aj;
                    }
                    return ptri->getYmin() < ptrj->getYmin();
                }
                return ptri->getXmin() < ptrj->getXmin();
            } 
            else if( _t == BlockSorter::R  ) 
            {
                if( ptri->getXmax() == ptrj->getXmax() ) {
                    if( ptri->getYmin() == ptrj->getYmin() ) {
                        if( ai == aj ) {
                            if( hi == hj ) {
                                return wi > wj;
                            }
                            return hi > hj;
                        }
                        return ai > aj;
                    }
                    return ptri->getYmin() < ptrj->getYmin();
                }
                return ptri->getXmax() > ptrj->getXmax();
           } 
           else if( _t == BlockSorter::B ) 
           {
                if( ptri->getYmin() == ptrj->getYmin() ) {
                    if( ptri->getXmin() == ptrj->getXmin() ) {
                        if( ai == aj ) {
                            if( wi == wj ) {
                                return hi > hj;
                            }
                            return wi > wj;
                        }
                        return ai > aj;
                    }
                    return ptri->getXmin() < ptrj->getXmin();
                }
                return ptri->getYmin() < ptrj->getYmin();

            }
            else if( _t == BlockSorter::T  ) 
            {
                if( ptri->getYmax() == ptrj->getYmax() ) {
                    if( ptri->getXmin() == ptrj->getXmin() ) {
                        if( ai == aj ) {
                            if( wi == wj ) {
                                return hi > hj;
                            }
                            return wi > wj;
                        }
                        return ai > aj;
                    }
                    return ptri->getXmin() < ptrj->getXmin();
                }
                return ptri->getYmax() > ptrj->getYmax();
           }
           else if( _t == BlockSorter::BL ) 
           {
                double  di  = std::fabs(ptri->getXmin()-_xmin) + std::fabs(ptri->getYmin()-_ymin);
                double  dj  = std::fabs(ptrj->getXmin()-_xmin) + std::fabs(ptrj->getYmin()-_ymin);
                if( di == dj ) {
                                if( ptri->getXmin() == ptrj->getXmin() ) {
                                        if( ptri->getYmin() == ptrj->getYmin() ) {
                                                if( ai == aj ) {
                                                        if( hi == hj ) {
                                                                return wi > wj;
                                                        }
                                                        return hi > hj;
                                                }
                                                return ai > aj;
                                        }
                                        return ptri->getYmin() < ptrj->getYmin();
                                }
                                return ptri->getXmin() < ptrj->getXmin();
                }
                return di < di;
           } 
           else if( _t == BlockSorter::BR ) 
           {
                double  di  = std::fabs(ptri->getXmax()-_xmax) + std::fabs(ptri->getYmin()-_ymin);
                double  dj  = std::fabs(ptrj->getXmax()-_xmax) + std::fabs(ptrj->getYmin()-_ymin);
                if( di == dj ) {
                    if( ptri->getXmax() == ptrj->getXmax() ) {
                        if( ptri->getYmin() == ptrj->getYmin() ) {
                            if( ai == aj ) {
                                if( hi == hj ) {
                                    return wi > wj;
                                }
                                return hi > hj;
                            }
                            return ai > aj;
                        }
                        return ptri->getYmin() < ptrj->getYmin();
                    }
                    return ptri->getXmax() > ptrj->getXmax();
                }
                return di < dj;
            } 
            else if( _t == BlockSorter::TL ) 
            {
                double  di  = std::fabs(ptri->getXmin()-_xmin) + std::fabs(ptri->getYmax()-_ymax);
                double  dj  = std::fabs(ptrj->getXmin()-_xmin) + std::fabs(ptrj->getYmax()-_ymax);
                if( di == dj ) {
                                if( ptri->getXmin() == ptrj->getXmin() ) {
                                        if( ptri->getYmin() == ptrj->getYmin() ) {
                                                if( ai == aj ) {
                                                        if( hi == hj ) {
                                                                return wi > wj;
                                                        }
                                                        return hi > hj;
                                                }
                                                return ai > aj;
                                        }
                                        return ptri->getYmin() < ptrj->getYmin();
                                }
                                return ptri->getXmin() < ptrj->getXmin();
                }
                return di < dj;
           }
           else if( _t == BlockSorter::TR ) 
           {
                double  di  = std::fabs(ptri->getXmax()-_xmax) + std::fabs(ptri->getYmax()-_ymax);
                double  dj  = std::fabs(ptrj->getXmax()-_xmax) + std::fabs(ptrj->getYmax()-_ymax);
                if( di == dj ) {
                    if( ptri->getXmax() == ptrj->getXmax() ) {
                        if( ptri->getYmin() == ptrj->getYmin() ) {
                            if( ai == aj ) {
                                if( hi == hj ) {
                                    return wi > wj;
                                }
                                return hi > hj;
                            }
                            return ai > aj;
                        }
                        return ptri->getYmin() < ptrj->getYmin();
                    }
                    return ptri->getXmax() > ptrj->getXmax();
                }
                return di < dj;
            }
            else if( _t == BlockSorter::A ) 
            {
                if( ai == aj ) 
                {
                    if( wi == wj ) 
                    {
                        return hi < hj;
                    }
                    return wi < wj;
                }
                return ai < aj;
            }
            else
            {
                // Unknown sort specified.
                exit(-1);
            }
        }
    public:
        double          _xmin, _xmax;
        double          _ymin, _ymax;
        BlockSorter::Type   _t;
    };

protected:
    void cleanup( void );
    void init( void );
    int run( Block* ptr, int depth );
    int solve( Block* ptr, int depth );
    int tetris( double xmin, double xmax, double ymin, double ymax, 
            std::vector<Block*>& preplaced, std::vector<Block*>& movable );
    int tetris_inner( double xmin, double xmax, double ymin, double ymax, 
            std::vector<Block*>& preplaced, std::vector<Block*>& movable );
    void split( Block* b, Block*& l, Block*& r,
            std::vector<Block*>& fixed,
            std::vector<Block*>& other );
    void calculateOccAndCap( Block* ptrb, double& occ, double& cap );

    int repair( double xmin, double xmax, double ymin, double ymax, 
            std::vector<Block*>& preplaced, std::vector<Block*>& movable );

    int lal( Block* bptr );

    void draw( Block *b, const std::string& str );
    void draw( double xmin, double xmax, double ymin, double ymax, 
            std::vector<Block*>& blocks0, std::vector<Block*>& blocks1, const std::string& str );

    // For LAL-styled legalization.
    void buildGrid( Block* bptr );
    void populateGrid( Block* bptr );
    bool expandGrid( int& l, int& r, int& b, int& t );
    void buildDP( void );

    // For flooplanning large cells.
    bool mmfSetupProblem( std::vector<Block *> &preplaced, std::vector<Block *> &blocks );
    int mmfFloorplan( double xmin, double xmax, double ymin, double ymax, 
                std::vector<Block*> &preplaced, std::vector<Block*> &blocks );



protected:
    DwsmLegalizerParams&        m_params;
    
    DetailedMgr*                m_mgr;
    Architecture*               m_arch;
    Network*                    m_network;
    RoutingParams*              m_rt;

    std::vector<double>         m_origPosX;
    std::vector<double>         m_origPosY;

    int                         m_curReg;
    double                      m_singleRowHeight;
    int                         m_numSingleHeightRows;
    double                      m_siteSpacing;

    Block*                      m_root;

    // For LAL styled legalization.
    int                         m_maxGridDimY;
    int                         m_maxGridDimX;
    int                         m_numRowsPerGridCell;
    double                      m_ywid;
    int                         m_ydim;
    double                      m_xwid;
    int                         m_xdim;
    std::vector<std::vector<std::vector<Block*> > >  m_blocksInGrid;
    std::vector<std::vector<double> >   m_gridCap;
    std::vector<std::vector<double> >   m_gridOcc;
    std::vector<std::vector<double> >   m_gridCap_DP;
    std::vector<std::vector<double> >   m_gridOcc_DP;

    // For floorplanning.
    std::vector<MinMovementFloorplanner::Block*>  m_mmfpBlocks;
    std::map<Block*,MinMovementFloorplanner::Block*> m_mmfpBlocksToBlockMap;
};

} // namespace aak







