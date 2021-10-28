
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
#include "plotgnu.h"
#include "router.h"

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////


namespace aak 
{
class DetailedSeg;
class DetailedMgr;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class LegalizeParams
{
public:
    LegalizeParams():
        m_onlyCheckPlacement( false ), 
        m_skipLegalization( false ),
        m_ut( 1.0 )
    {}
public:
    bool        m_onlyCheckPlacement;
    bool        m_skipLegalization;
    double      m_ut;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class Legalize 
{
public:
    Legalize( LegalizeParams& params );
    virtual ~Legalize();

    void legalize( DetailedMgr& mgr );
    //void legalize( Architecture* arch, Network* network, RoutingParams* rt );
    void plotter( PlotGnu* plotter ) { m_plotter = plotter; }


public:


    class Rect
    {
    public:
        Rect():m_l(0),m_r(0),m_b(0),m_t(0)
        {};
        Rect(double l, double r, double b, double t):m_l(l),m_r(r),m_b(b),m_t(t)
        {}
        Rect(const Rect& other):m_l(other.m_l),m_r(other.m_r),m_b(other.m_b),m_t(other.m_t)
        {}
        Rect& operator=(const Rect& other )
        {
            if( this != &other )
            {
                m_l = other.m_l;
                m_r = other.m_r;
                m_b = other.m_b;
                m_t = other.m_t;
            }
            return *this;
        }
    public:
        double m_l, m_r, m_b, m_t;
    };

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
    void set_original_positions( void );
    void restore_original_positions( void );
    void force_cells_into_regions( void );
    bool inside_rectangle( Node* ndi, Rectangle& rect );
    bool fits_in_rectangle( Node* ndi, Rectangle& rect );
    double distance_to_rectangle( Node* ndi, Rectangle& rect );
    void move_into_rectangle( Node* ndi, Rectangle& rect );
    void print_dist_from_start_positions( void );
    void print_dist_from_orig_positions( void );

    void moveCellsBetweenSegments( DetailedSeg* oldPtr, int leftRightTol, double offsetTol, bool ignoreGaps = true );
    void moveCellsBetweenSegments( void );

    void doClumping( std::vector<Node*>& nodes_in, double xmin, double xmax, bool debug = false );
    void doClumping( Clump* r, std::vector<Clump>& clumps, double xmin, double xmax, bool debug = false );

protected:
    // Standard stuff.
    LegalizeParams&         m_params;

    DetailedMgr*            m_mgr;

    Network*                m_network;
    Architecture*           m_arch;
    RoutingParams*          m_rt;

    PlotGnu*                m_plotter;

    std::vector<double>     m_origX;
    std::vector<double>     m_origY;
};

} // namespace aak
