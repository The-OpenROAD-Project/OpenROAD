

#pragma once





////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <set>
#include <deque>
#include "architecture.h"
#include "network.h"
#include "router.h"
#include "detailed_objective.h"


namespace aak
{


////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////

class DetailedABU : public DetailedObjective
{
// This class maintains the ABU metric which can be used as part of a cost 
// function for detailed improvement.
public:
    DetailedABU( Architecture* arch, Network* network, RoutingParams* rt );
    virtual ~DetailedABU( void );

    // Those that must be overridden.
    double      curr( void );
    double      delta( int n, std::vector<Node*>& nodes, 
                    std::vector<double>& curX, std::vector<double>& curY, 
                    std::vector<unsigned>& curOri, 
                    std::vector<double>& newX, std::vector<double>& newY, 
                    std::vector<unsigned>& newOri
                );
    void        accept( void );
    void        reject( void );


    // Other.
    void        init( DetailedMgr* mgr, DetailedOrient* orient );
    void        init( void );
    double      calculateABU( bool print = false );
    double      measureABU( bool print = false );

    void        updateBins( Node* nd, double x, double y, int addSub );
    void        acceptBins( void );
    void        rejectBins( void );
    void        clearBins( void );

    double      delta( void );

    double      freeSpaceThreshold( void );
    double      binAreaThreshold( void );
    double      alpha( void );

    void        computeUtils( void );
    void        clearUtils( void );

    void        computeBuckets( void );
    void        clearBuckets( void );
    int         getBucketId( int binId, double occ );

protected:
    struct density_bin 
    {
        int         id;
        double      lx, hx;              // low/high x coordinate 
        double      ly, hy;              // low/high y coordinate 
        double      area;                // bin area 
        double      m_util;              // bin's movable cell area 
        double      c_util;              // bin's old movable cell area 
        double      f_util;              // bin's fixed cell area 
        double      free_space;          // bin's freespace area 
    };

protected:
    DetailedMgr*                m_mgrPtr;
    DetailedOrient*             m_orientPtr;

    Architecture*               m_arch;
    Network*                    m_network;
    RoutingParams*              m_rt;

public:
    // Utilization monitoring for ABU (if paying attention to ABU).
    std::vector<density_bin>    m_abuBins;
    double                      m_abuGridUnit;
    int                         m_abuGridNumX;
    int                         m_abuGridNumY;
    int                         m_abuNumBins;

    double                      m_abuTargUt;  
    double                      m_abuTargUt02;
    double                      m_abuTargUt05;
    double                      m_abuTargUt10;
    double                      m_abuTargUt20;

    int                         m_abuChangedBinsCounter;
    std::vector<int>            m_abuChangedBins;
    std::vector<int>            m_abuChangedBinsMask;

    std::vector<std::set<int> > m_utilBuckets;
    std::vector<double>         m_utilTotals;
};

} // namespace aak
