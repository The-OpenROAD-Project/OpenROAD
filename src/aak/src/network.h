
////////////////////////////////////////////////////////////////////////////////
// File: network.h
////////////////////////////////////////////////////////////////////////////////


#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include "architecture.h"
#include "orientation.h"
#include "symmetry.h"

#define ERRMSG(msg) \
    fprintf( stderr, "Error: %s:%d ", __FILE__, __LINE__ ); \
    fprintf( stderr, "%s\n", msg );



namespace aak
{


////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class Node;
class Edge;
class Pin;
class Network;
class Architecture;

const unsigned NodeType_UNKNOWN                      = 0x00000000;
const unsigned NodeType_CELL                         = 0x00000001;
const unsigned NodeType_TERMINAL                     = 0x00000002;
const unsigned NodeType_TERMINAL_NI                  = 0x00000004;
const unsigned NodeType_MACROCELL                    = 0x00000008;
const unsigned NodeType_FILLER                       = 0x00000010;
const unsigned NodeType_SHAPE                        = 0x00000020;
const unsigned NodeType_PEANUT                       = 0x00000040;
const unsigned NodeType_SPREAD                       = 0x00000080;

const unsigned NodeAttributes_EMPTY                  = 0x00000000;
const unsigned NodeAttributes_SHRED                  = 0x00000001;

const unsigned NodeAttributes_IS_FLOP                = 0x00000002;

const unsigned NodeFixed_NOT_FIXED                   = 0x00000000;
const unsigned NodeFixed_FIXED_X                     = 0x00000001;
const unsigned NodeFixed_FIXED_Y                     = 0x00000002;
const unsigned NodeFixed_FIXED_XY                    = 0x00000003; // FIXED_X and FIXED_Y.

#define EDGETYPE_DEFAULT                             (0)

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class Node
{
public:
    Node();
    Node( const Node& other );
    Node& operator=( const Node& other );
    virtual ~Node();

    int             getId( void ) const { return m_id; }
    void            setId( int id ) { m_id = id; }

    void            setHeight( double h ) { m_h = h; }
    double          getHeight( void ) const { return m_h; }

    void            setWidth( double w ) { m_w = w; }
    double          getWidth( void ) const { return m_w; }

    double          getArea( void ) const { return m_w * m_h; }

    void            setX( double x ) { m_x = x; }
    double          getX( void ) const { return m_x; }

    void            setY( double y ) { m_y = y; }
    double          getY( void ) const { return m_y; }

    void            setOrigX( double x ) { m_origX = x; }
    double          getOrigX( void ) const { return m_origX; }

    void            setOrigY( double y ) { m_origY = y; }
    double          getOrigY( void ) const { return m_origY; }


    void            setFixed( unsigned fixed ) { m_fixed = fixed; }
    unsigned        getFixed( void ) const { return m_fixed; }

    void            setType( unsigned type ) { m_type = type; }
    unsigned        getType( void ) const { return m_type; }

    void            setRegionId( int id ) { m_regionId = id; }
    int             getRegionId( void ) const { return m_regionId; }

    void            setCurrOrient( unsigned orient ) { m_currentOrient = orient; }
    unsigned        getCurrOrient( void ) const { return m_currentOrient; }
    void            setAvailOrient( unsigned avail ) { m_availOrient = avail; }

    void            setAttributes( unsigned attributes ) { m_attributes = attributes; }
    unsigned        getAttributes( void ) const { return m_attributes; }
    void            addAttribute( unsigned attribute ) { m_attributes |= attribute; }
    void            remAttribute( unsigned attribute ) { m_attributes &= ~attribute; }

    bool            isFlop( void ) const;

    void            setLeftEdgeType( int etl ) { m_etl = etl; }
    int             getLeftEdgeType( void ) const { return m_etl; }

    void            setRightEdgeType( int etr ) { m_etr = etr; }
    int             getRightEdgeType( void ) const { return m_etr; }

    void            setBottomPower( int bot ) { m_powerBot = bot; }
    int             getBottomPower( void ) const { return m_powerBot; }

    void            setTopPower( int top ) { m_powerTop = top; }
    int             getTopPower( void ) const { return m_powerTop; }


    unsigned        m_firstPin; 
    unsigned        m_lastPin;
    int             m_etl;      // Edge types.
    int             m_etr;
    int             m_regionId; // Regions.
    int             m_powerTop;
    int             m_powerBot;
    unsigned        m_currentOrient;
    unsigned        m_availOrient;

protected:
    // Id.
    int             m_id; 
    // Current position.
    double          m_x;
    double          m_y;
    // Original position.
    double          m_origX;
    double          m_origY;
    // Width and height.
    double          m_w;
    double          m_h;
    // Type.
    unsigned        m_type;
    // Fixed or not fixed.
    unsigned        m_fixed;
    // Place for attributes.
    unsigned        m_attributes;
};

class Edge
{
public:
    Edge();
    virtual ~Edge();

    int getId( void ) const { return m_id; }
    void setId( int id ) { m_id = id; }


    unsigned m_firstPin;
    unsigned m_lastPin;

    int m_ndr;  // Refers to a routing rule inside of the routing parameters class.
protected:
    int             m_id;
};

class Pin
{
public:
    enum Direction { Dir_IN, Dir_OUT, Dir_INOUT, Dir_UNKNOWN };
public:
    Pin();
    Pin( const Pin& other );
    Pin& operator=( const Pin& other );
    virtual ~Pin();

    void setId( int id ) { m_id = id; }
    int getId( void ) const { return m_id; }

    void setNodeId( int id ) { m_nodeId = id; }
    int getNodeId( void ) const { return m_nodeId; }

    void setEdgeId( int id ) { m_edgeId = id; }
    int getEdgeId( void ) const { return m_edgeId; }

    void setOffsetX( double offsetX ) { m_offsetX = offsetX; }
    double getOffsetX( void ) const { return m_offsetX; }

    void setOffsetY( double offsetY ) { m_offsetY = offsetY; }
    double getOffsetY( void ) const { return m_offsetY; }

    void setPinLayer( int layer ) { m_pinLayer = layer; }
    int getPinLayer( void ) const { return m_pinLayer; }

    void setPinWidth( double width ) { m_pinW = width; }
    double getPinWidth( void ) const { return m_pinW; }

    void setPinHeight( double height ) { m_pinH = height; }
    double getPinHeight( void ) const { return m_pinH; }



#ifdef USE_ICCAD14
    bool isSink() const { return m_dir == Pin::Dir_IN; }
    bool isSource() const { return m_dir == Pin::Dir_OUT; }
    bool isBiDir() const 
    {  
        return (m_dir == Pin::Dir_IN || m_dir == Pin::Dir_OUT) ? false : true;
    }
    Node* getOwner( Network* network ) const;
    bool getName( Network* network, std::string& name ) const;
    bool isFlopInput( Network* network ) const;
    bool isPi( Network* nw ) const;
    bool isPo( Network* nw ) const;
#endif


    int         m_id;
    int         m_nodeId;
    int         m_edgeId;
    int         m_dir;
    double      m_offsetX;
    double      m_offsetY;
    int         m_pinLayer; // Layer for the pin.
    double      m_pinW; // Width for the pin.
    double      m_pinH; // Height for the pin.

#ifdef USE_ICCAD14
    char* m_portName;   // Name of the port.

    // From .sdc file
    double m_cap;       // Load capacitance of an output.
    double m_delay;     // Delay at inputs/outputs wrt a specified clock.
    double m_rTran;     // Rise transition time at input.
    double m_fTran;     // Fall transition time at input.
    int m_driverType;   // Library cell assumed to be driving the input.
    // From timer
    double m_earlySlack;        // Clearly, slack...
    double m_lateSlack;         // Clearly, slack...
    // Computed
    double m_crit;              // The criticality of the pin...
#endif
protected:
};

class Network
{
public:
    struct compareNodesByW
    {
        bool operator()( Node* ndi, Node* ndj )
        {
            if( ndi->getWidth() == ndj->getWidth() )
            {
                return ndi->getId() < ndj->getId();
            }
            return ndi->getWidth() < ndj->getWidth();
        }
    };

    struct comparePinsByNodeId
    {
        bool operator()( const Pin* a, const Pin* b )
        {
#ifdef USE_ICCAD14
            // To mimic the order of pins to the evaluation script, we should also sort
            // by pin name...
            if( a->m_nodeId == b->m_nodeId )
            {
                return (strcmp( a->m_portName, b->m_portName ) < 0);
            }
#endif
            return a->m_nodeId < b->m_nodeId;
        }
    };

    class comparePinsByEdgeId
    {
    public:
        comparePinsByEdgeId():m_nw(0) {}
        comparePinsByEdgeId( Network* nw ):m_nw(nw) {}
        bool operator()( const Pin* a, const Pin* b )
        {
            // To mimic the order of pins to the evaluation script, we should also sort by pin name.  The pin
            // name also involves the instance name which must be obtained from the network...  :(
            if( a->m_edgeId == b->m_edgeId )
            {
#ifdef USE_ICCAD14
                std::string name_a, name_b;
                a->getName( m_nw, name_a );
                b->getName( m_nw, name_b );
                return ( strcmp( name_a.c_str(), name_b.c_str() ) < 0 );
#endif
            }
            return a->m_edgeId < b->m_edgeId;
        }
        Network* m_nw;
    };

    class comparePinsByOffset
    {
    public:
        comparePinsByOffset():m_nw(0) {}
        comparePinsByOffset( Network* nw ):m_nw(nw) {}
        bool operator()( const Pin* a, const Pin*b )
        {
            if( a->m_offsetX == b->m_offsetX )
            {
                return a->m_offsetY < b->m_offsetY;
            }
            return a->m_offsetX < b->m_offsetX;
        }
        Network* m_nw;
    };

public:
    class Shape
    {
    public:
        Shape():
            m_llx( 0.0 ),
            m_lly( 0.0 ),
            m_w( 0.0 ),
            m_h( 0.0 )
        {
            ;
        }
        Shape( double llx, double lly, double w, double h ):
            m_llx( llx ),
            m_lly( lly ),
            m_w( w ),
            m_h( h )
        {
            ;
        }
    public:
        double m_llx;
        double m_lly;
        double m_w;
        double m_h;
    };

public:
    Network();
    virtual ~Network();

    size_t getNumNodes() const { return m_nodes.size(); }
    size_t getNumEdges() const { return m_edges.size(); }

    Node* getNode( int i ) { return &(m_nodes[i]); }
    Edge* getEdge( int i ) { return &(m_edges[i]); }




public:
    // Netlist representation...
    std::vector<Node>       m_nodes;    // The nodes in the netlist...
    std::vector<Edge>       m_edges;    // The edges in the netlist...
    std::vector<Pin>        m_pins;     // All the pins in the netlist...

    std::vector<Pin*> m_nodePins;        // For accessing pins on any node...  Pointer to static allocation...
    std::vector<Pin*> m_edgePins;        // For accessing pins on any edge...  Pointer to static allocation...

    // Names...
    std::vector<std::string> m_nodeNames;
    std::vector<std::string> m_edgeNames;

    // Shapes for non-rectangular nodes...
    std::vector<std::vector<Node*> > m_shapes;

    // For shredding of macros...
    std::vector<std::vector<Node*>*> m_shreds; // Shreds for each macrocell.
    std::vector<Node*> m_pieces; // List of the shreds.
    std::map<Node*,Node*> m_reverseMap; // Maps shreds to original macrocell.

    // For filler...
    std::vector<Node*> m_filler;

    // For peanuts...
    std::vector<Node*> m_peanuts;

    // For indexing nodes...  Since we add a bunch of bogus nodes here
    // and there, we need to make sure node ids are unique!!!
    int m_numNodesRaw; // XXX: Always equal to the number of network nodes.
    int m_numNodes; // XXX: Total nodes...

    // XXX: Specific for ICCAD2014 contest...
    
#ifdef USE_ICCAD14
    std::string team;
    std::string directory;
    std::string benchmark;
    double LOCAL_WIRE_CAP_PER_MICRON,  LOCAL_WIRE_RES_PER_MICRON;        /* Ohm & Farad per micro meter */
    double GLOBAL_WIRE_CAP_PER_MICRON, GLOBAL_WIRE_RES_PER_MICRON;       /* Ohm & Farad per micro meter */
    double MAX_WIRE_SEGMENT_IN_MICRON;                                   /* in micro meter  */
    double DEF_DISTANCE_IN_MICRONS;

    std::vector<Pin*> m_Pis;        // Primary input pins.
    std::vector<Pin*> m_Pos;        // Primary output pins.
    std::vector<Pin*> m_PisPos;     // Primary bidir pins.

    // Following is sort of bad... It assumes a single clock across the entire circuit which might not be true.
    std::string m_clockName;    // SDC identifier for the clock.
    std::string m_clockPort;    // Port to which the clock is bound.
    double m_clockPeriod;       // Period of the clock.
#endif

protected:
};


} // namespace aak
