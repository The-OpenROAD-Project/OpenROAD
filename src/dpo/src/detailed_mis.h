

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#pragma once



////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <cmath>
#include <bitset>
#include <deque>
#include <list>
#include <set>
#include <vector>
#include <map>
#include "architecture.h"
#include "network.h"
#include "router.h"


namespace aak 
{

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class DetailedMisParams;
class DetailedMis;
class DetailedSeg;
class DetailedMgr;


////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class DetailedMisParams 
{
public:
	enum Strategy {
		KDTree = 0,
		Binning = 1,
		Colour = 2,
	};

public:
	DetailedMisParams( void ) :
					_maxDifferenceInMetric( 0.03 ),
					_maxNumNodes( 15 ),
					_maxPasses( 1 ),
					_sizeTol( 1.10 ),
					_skipNetsLargerThanThis( 50 ),
					_strategy( Binning ),
					_useSameSize( true )
	// ************************************************************
	{ 
		;
	}

public:
	double		_maxDifferenceInMetric;	// How much we allow the routine to reintroduce overlap into placement
	unsigned		_maxNumNodes;		// Only consider this many number of nodes for B&B (<= MAX_BB_NODES)
	unsigned		_maxPasses;		// Maximum number of B&B passes
	double		_sizeTol;		// Tolerance for what is considered same-size
	unsigned		_skipNetsLargerThanThis; // Skip nets larger than this amount.
	Strategy	_strategy;		// The type of strategy to consider
	bool		_useSameSize;		// If 'false', cells can swap with approximately same-size locations
};



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class DetailedMis
{
    // Flow-based solver for replacing nodes using matching.
    enum Objective { Hpwl, Disp };
public:
    DetailedMis( Architecture* arch, Network* network, RoutingParams* rt );
    virtual ~DetailedMis( void );

    void run( DetailedMgr* mgrPtr, std::string command );
    void run( DetailedMgr* mgrPtr, std::vector<std::string>& args );

/*
public:
	void				Place( void );
	void 				Place( std::vector<Node *> &, std::vector<SwapLocation *> &, unsigned );
	void				SetNetlist( Netlist * );
*/
protected:
	class Bucket;

/*
	class BBox;
	struct NodeBin;
	struct SortNodesByArea;

	void 				binTheNodes( std::vector<Node *> & );
	void 				binTheNodes( unsigned, unsigned );
	void				calculateCOG( void );
	void				colourNodes( void );
	void				gatherNodesWithColour( void );
	void 				gatherNodesWithOutwardScan( void );
	void 				gatherNodesWithWindow( void );
	double 				getCostOfPlacement( void );
	double 				getDetailedMisCost( Node *, SwapLocation * );
	double 				init( std::vector<Node *> & );
	inline bool 			isSameSize( Node *, SwapLocation * );
	inline bool 			isSameSize( Node *, Node * );
	void				place( std::vector<Node *> &, std::vector<SwapLocation *> &, 
						unsigned, std::deque<SwapLocation *> & );
 */

    void place( void );
    void collectMovableCells( void );
    void colorCells( void );
	void buildGrid( void );
	void clearGrid( void );
    void populateGrid( void );
    void gatherNeighbours( Node* ndi );
    void solveMatch( void );
    double getHpwl( Node* ndi, double xi, double yi );
    double getDisp( Node* ndi, double xi, double yi );

public:
/*
	DetailedMisParams			_params;
    */


    DetailedMgr*        m_mgrPtr;

    Architecture*       m_arch;
    Network*            m_network;
    RoutingParams*      m_rt;

    std::vector<Node*>  m_candidates;
    std::vector<bool>   m_movable;
    std::vector<int>    m_colors;
    std::vector<Node*>  m_neighbours;

    // Grid used for binning and locating cells.
	std::vector<std::vector<Bucket*> > m_grid;	
    int                 m_dimW;
    int                 m_dimH;
    double              m_stepX;
    double              m_stepY;
    std::map<Node*,Bucket*> m_cellToBinMap;

    std::vector<int>    m_timesUsed;

    // Other.
    int                 m_skipEdgesLargerThanThis;
    int                 m_maxProblemSize;
    int                 m_traversal;
    bool                m_useSameSize;
    bool                m_useSameColor;
    int                 m_maxTimesUsed;
    Objective           m_obj;
};

} // namespace aak
