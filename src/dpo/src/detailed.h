



#pragma once


////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <deque>
#include "architecture.h"
#include "network.h"
#include "router.h"


namespace aak
{

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class DetailedSeg;
class DetailedMgr;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class DetailedParams
{
public:
    DetailedParams():
        m_targetMaxMovement( std::numeric_limits<double>::max() ),
        m_targetUt( 1.0 )
    {
        m_script = "";

    }
    virtual ~DetailedParams() {}
public:
    std::string         m_script;
    double              m_targetMaxMovement;
    double              m_targetUt;
};

class Detailed
{
public:

public:
    Detailed( DetailedParams& params ):m_params( params ) {}
    virtual ~Detailed( void );

    // Interface for script.
    //bool improve( Architecture* arch, Network* network, RoutingParams* rt );
    bool improve( DetailedMgr& mgr );

protected:
    void        doDetailedCommand( std::vector<std::string>& args );


public:
    DetailedParams&         m_params;

    DetailedMgr*            m_mgr;

    Architecture*           m_arch;
    Network*                m_network;
    RoutingParams*          m_rt;
};

} // namespace aak
