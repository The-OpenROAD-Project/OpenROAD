///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.




//////////////////////////////////////////////////////////////////////////////
// File: min_movement_floorplanner.h
// Description:
// 	This file contains the MinMovementFloorplanner class.
//////////////////////////////////////////////////////////////////////////////


#pragma once


//////////////////////////////////////////////////////////////////////////////
// Includes.
//////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <map>
#include "architecture.h"
#include "network.h"
#include "seq_pair.h"


namespace dpo
{
class DetailedMgr;


//////////////////////////////////////////////////////////////////////////////
// Classes.
//////////////////////////////////////////////////////////////////////////////



class MinMovementFloorplannerParams
{
public:
    MinMovementFloorplannerParams(): 
        _abortIfNotLegal( false ), 
        _fastMode( false ),
        _allowOrientationChanges( false )
	{
		;
	}

public:
    bool _abortIfNotLegal;
    bool _fastMode;
    bool _allowOrientationChanges;
};


class MinMovementFloorplanner
{
public:
    // These are necessary structures and typedefs.
    enum WhichGraph 
    {
        HGraph	= 0, 
        VGraph	= 1 
    };

    class Slack 
    { 
    public: 
        Slack():
            _tarr( 0. ), 
            _treq( 0. )	
        { 
            ; 
        }

        inline double ComputeSlack() const { return _treq - _tarr; }
        inline double& GetTarr() { return _tarr; }
        inline double& GetTreq() { return _treq; }
    protected:
        double _tarr;
        double _treq;
	};


    class Clump
    {
    public:
        Clump( void ):m_id(-1),m_weight(0.),m_wposn(0.),m_posn(0.) { m_blocks.clear(); }
        virtual ~Clump( void ) {}
    public: 
        int                 m_id;
        double              m_weight;
        double              m_wposn;
        double              m_posn;
        std::vector<int>    m_blocks;
    };



	class Block
	{
	    // In practice, this is a wrapper for nodes in the problem.  Note that
	    // this code SPECIFICALLY does NOT encapsulate netlist nodes -- that's
	    // on purpose!  (Originally, it did, and I found that this led to
	    // abstraction problems.  I made the move away from netlist nodes
	    // [maintaining a separate map] to make it cleaner.)
	public:
		Block( void ) :
			_id( -1 ),
			_availableOrient( Orientation_N    ),
			_currentOrient( Orientation_N    ),
			_type( NodeType_MACROCELL ),
			_fixed( false ),
			_placed( true ),
			_resized( false ),
            _node( NULL )
		{
			_dim[HGraph] = 0.;
			_dim[VGraph] = 0.;
			_pos[HGraph] = 0.;
			_pos[VGraph] = 0.;
			_origDim[HGraph] = 0.;
			_origDim[VGraph] = 0.;
			_origPos[HGraph] = 0.;
			_origPos[VGraph] = 0.;
		}

		~Block( void )
		{
			;
		}

	public:
		Node *CreateDummyNode( void )
		{
		    // Creates a dummy netlist node that looks like this block.
		    // Does not create edges/pins/etc.  NB: Remember to delete
		    // this node when you're done using it.
			Node* tmpNd = new Node;

            tmpNd->setId( GetId() );
			tmpNd->setWidth( GetDim( HGraph ) );
			tmpNd->setHeight( GetDim( VGraph ) );
			tmpNd->setX( GetPos( HGraph ) );
			tmpNd->setY( GetPos( VGraph ) );
			tmpNd->m_currentOrient = GetCurrentOrient();
			tmpNd->m_availOrient = GetAvailableOrient();
			tmpNd->setFixed( GetFixed() );
			tmpNd->setType( GetType() );
			return tmpNd;
		}

        unsigned GetAvailableOrient( void ) const	{ return _availableOrient; }
        unsigned GetCurrentOrient( void ) const		{ return _currentOrient; }
        inline double& GetDim( WhichGraph wg) 			{ return _dim[wg]; }
        inline bool GetFixed( void ) const			{ return _fixed; }
		inline int& GetId( void )				{ return _id; }
		inline double&		GetOrigDim( WhichGraph wg) 		{ return _origDim[wg]; }
		inline double&		GetOrigPos( WhichGraph wg )		{ return _origPos[wg]; }
		inline double&		GetPos( WhichGraph wg )			{ return _pos[wg]; }
		inline Slack*		GetSlack( WhichGraph wg )		{ return &( _slack[wg] ); }
		inline unsigned 	GetType( void ) const			{ return _type; }
		inline bool 		IsPlaced( void ) const			{ return _placed; }
		inline bool 		IsResized( void ) const			{ return _resized; }

		inline void 		SetAvailableOrient( unsigned t )	{ _availableOrient = t; }
		inline void		SetCurrentOrient( unsigned t )	{ _currentOrient = t; }
		inline void		SetFixed( bool f )			{ _fixed = f; }
		inline void		SetPlaced( bool p )			{ _placed = p; }
		inline void		SetResized( bool r )			{ _resized = r; }
		inline void		SetType( unsigned t )			{ _type = t; }

        void SetNode( Node* ndi ) { _node = ndi; }
        Node* GetNode( void ) const { return _node; }


	public:
		Slack		_slack[2];
		double		_dim[2];
		double		_pos[2];
		double		_origDim[2];
		double		_origPos[2];
		int		_id;

        Node*   _node;

		unsigned _availableOrient;
		unsigned _currentOrient;
		unsigned _type;

		bool		_fixed : 1;
		bool		_placed : 1;
		bool		_resized : 1;

        // For clumping...
        Clump* m_clump;
        double m_offset;
        double m_weight;
        double m_target;
        double m_dim;
	};
	
public:
    MinMovementFloorplanner( MinMovementFloorplannerParams& params, Placer_RNG* rng );
    virtual ~MinMovementFloorplanner();

    bool LegalizeMC( Network* network, Architecture* arch );
    bool LegalizeSC( Network* network, Architecture* arch );

    bool LegalizeMultiHeight( DetailedMgr& mgr );
    bool LegalizeAllHeight( DetailedMgr& mgr );

    bool shift( DetailedMgr& mgr, bool onlyMulti = false );
    bool shift( DetailedMgr& mgr, int regId, bool onlyMulti = false );
    bool shift( DetailedMgr& mgr, int regId, std::vector<std::pair<double,double> >& targets, bool onlyMulti = false );

    bool test( DetailedMgr& mgr, bool onlyMulti = false );
    bool test( DetailedMgr& mgr, int regId, bool onlyMulti = false );

    bool repair( DetailedMgr& mgr, int regId );
    bool repair1( DetailedMgr& mgr, int regId );
    double repair_slack( 
            std::vector<std::vector<std::pair<int,int> > >& gr_out,
            std::vector<std::vector<std::pair<int,int> > >& gr_inp,
            std::vector<double>& Tarr, std::vector<double>& Treq 
            );
    void repair_order( 
            std::vector<std::vector<std::pair<int,int> > >& gr_out,
            std::vector<std::vector<std::pair<int,int> > >& gr_inp,
            std::vector<int>& order 
            );

    bool minshift( DetailedMgr& mgr, double xmin, double xmax, double ymin, double ymax, std::vector<Block*>& blocks );
protected:
    
    void collectNodes();
    void createSingleProblem( std::vector<Block*>& blocks, std::map<Node*,Block*>& nodeToBlockMap );
    void createLargeProblem( std::vector<Block*>& blocks, std::map<Node*,Block*>& nodeToBlockMap );

    int collectBlocks( std::vector<Block*>& blocks, std::map<Node*,Block*>& nodeToBlockMap );
    void setNodePositions( std::map<Node*,Block*>& nodeToBlockMap );
    bool legalize( double xmin, double xmax, double ymin, double ymax, std::vector<Block*>& blocks, std::map<Node*,Block*>& nodeToBlockMap );
	void buildInitialSPSimple();
	void buildInitialSPComplex();

    bool clump( double& amtMovedX, double& amtResizedX );
    void mergeLeft( Clump* r, WhichGraph );
    bool getMostViolatedEdge( Clump* r, WhichGraph, Clump*& l, double& dist );










    bool Legalize( double xmin, double xmax, double ymin, double ymax, Network* );
    bool MinShift( double xmin, double xmax, double ymin, double ymax, std::vector<Block *> & );
    void RefineSolution( std::vector<Block *> & );
    bool LegalizeUsingSP( Network* network, Architecture* arch );

protected:
	class	FPGraph;
	struct	Arc;
	struct	EdgeSet;
	class	Location;
	struct	SortArcs;
	struct	SortBlocksByDimension;
	struct	SortBlocksByX;
	struct	SortEdgeSet;

	struct EdgePair
	{
		unsigned	_src;
		unsigned	_dst;
	};

	struct SortEdgePair
	{
		inline bool operator() ( const EdgePair &p, const EdgePair &q ) const
		{
			if( p._src == q._src ) {
				return p._dst < q._dst;
			}
			return p._src < q._src;
		}
	};

protected:
	bool		adjustSCByInserting();
	bool		adjustEdgesByRandom( void );
	bool		adjustEdgesByInserting( void );
	bool 		adjustEdgesByMovingOrReversing( void );
	bool		adjustEdgesByRotating( void );
	void 		buildInitialCG( void );
	void 		buildInitialTCGandSP( void );
	void 		buildTCGfromSP( void );
	void 		collectBlocks( Network*, std::vector<Block *> &, std::vector<Node *> & );
	double 		computeCostMetric( WhichGraph );
	void 		computeSlack( WhichGraph );
    void        alignToSites( void );
    bool        shiftFast( double& maxMovedX, double& amtMovedX );
	void 		determineEdgeSet( WhichGraph, std::vector<EdgeSet> & );
	bool 		doSPInsertion( Block *, std::vector<Arc> &, unsigned );
	void 		doSPMoveOrReversal( WhichGraph, EdgeSet &, bool );
	void 		drawBlocks( std::string, std::vector<Block *> & );
	void 		drawWorstSlack( WhichGraph );
	void 		extractSubgraph( WhichGraph );
	bool 		getCandidateArcs( WhichGraph, Block *, std::vector<Arc> & );
	Block *		getCandidateBlock( WhichGraph, bool onlySingleHeight = false );
    void        initializeEpsilon( void );
	void		initialize( void );
	bool 		legalizeTCG( void );
	void 		pathCounting( WhichGraph );
	void		refineSolution( double = -1. );
	void 		setNetworkNodes( std::vector<Node *> & );
	bool 		setupAndSolveLP( bool, bool, double, double, double &, double &, double&, double&, double&,
                        bool observeGapsForX = false );
	void 		transitiveClosure( std::vector<std::vector<bool> > & );
	void 		transitiveClosureInner( std::vector<std::vector<bool> > &, std::vector<std::vector<bool> > &, 
				int v, std::vector<int> &, int & );

protected:
	// Used for the graphs and slack computations.
	FPGraph				*_subgraph[2];			// The critical subgraph of the TCG
	std::vector<int>		_subgraphMapping[2];		// Maps blocks to sub-graph vertices
	std::vector<Block *>		_subgraphReverseMapping[2];	// Maps sub-graph vertices back to blocks

	// Used for building sequence pairs and so forth.

	// Used for the linear assignment.
//	std::vector<FlowEdge>		_flowEdges;			// Edges in the flow problem
//	std::vector<FlowNode>		_flowNodes;			// Nodes in the flow problem
//	std::vector<Location *>		_locations;			// Locations for the flow problem
//	std::vector<unsigned>		_mappingBlocksToFlowNodes;	// Maps cells to flow nodes
//	std::vector<unsigned>		_mappingLocationsToFlowNodes;	// Maps Locations to flow nodes
//	std::vector<Location *>		_mappingFlowNodeToLocation;	// Maps FlowNodes to Locations
//	SolveFlow			_solveFlow;

	// Miscellaneous.
	std::map<EdgePair,double,SortEdgePair>	_edgeCrit;		// Used for tracking edge criticalities

public:
    MinMovementFloorplannerParams m_params; // The parameters are public, to allow modification
    Network* m_net;
    Architecture* m_arch;
	Placer_RNG* m_rng;	

    // Dimensions of the region.
    double m_Tmin[2];			
    double m_Tmax[2];	
    double m_worstSlack[2];

    // For sequence pair...
    std::vector< std::vector<bool> > m_matrix[2];
    std::vector<int> m_posInXSeqPair;			
    std::vector<int> m_posInYSeqPair;
    SP m_seqPair;	
    FPGraph* m_graph[2]; // TCG graphs.
    FPGraph* m_backupGraph[2];
    double m_maxTarr[2]; // Used for slack computation.
    double m_minTreq[2]; // Used for slack computation.
    std::vector<int> m_order; // Used for various ordering computations
    std::vector<double> m_pathCountB[2]; // Used for path counting.
    std::vector<double>	m_pathCountF[2]; // Used for path counting.

    // The blocks.
    std::vector<Block*> m_blocks;

    // For clumping.
    std::vector<Clump*> m_clumps;

    // Small tolerance.
    double m_epsilon;	

    // For timing.
    double m_timeLegalize ;
    double m_timeInserting;
    double m_timeMovingOrReversing;

    std::vector<Node*> m_fixedNodes;
    std::vector<Node*> m_largeNodes;
    std::vector<Node*> m_doubleHeightNodes;

};

} // namespace dpo
