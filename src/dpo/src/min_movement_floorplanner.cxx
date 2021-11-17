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
//
// Description:
// 	This file contains a flow-based legalizer for standard cell designs.
//////////////////////////////////////////////////////////////////////////////
#include <boost/config.hpp>
#include <boost/dynamic_bitset.hpp>
#include <iostream>
#include <unistd.h>
#include <list>
#include <algorithm>
#include "utility.h"
#include "min_movement_floorplanner.h"
#include "detailed_segment.h"
#include "detailed_manager.h"

//#include "ClpSimplex.hpp"
//#include "CoinHelperFunctions.hpp"
//#include "CoinTime.hpp"
//#include "CoinBuild.hpp"
//#include "CoinModel.hpp"



// DEFINES ===================================================================
#define	BUFFER_SIZE		2047
#define	DISCOUNT(a,b)		std::exp( -a / b )
#define	NUM_BLOCKS_CUTOFF	1000

#define LEGALIZE_SC_ROW_BY_ROW


namespace dpo
{


// LOCAL STRUCTS =============================================================
class MinMovementFloorplanner::Location
// ************************************
{
public:
	Location( void ) :
		_blk( NULL ),
		_id( -1 )
	{
		_dim[HGraph] = 0.;
		_dim[VGraph] = 0.;
		_pos[HGraph] = 0.;
		_pos[VGraph] = 0.;
	}

	~Location( void )
	{
		;
	}

public:
	inline Block * &	GetBlock( void )		{ return _blk; }
	inline double &		GetDim( WhichGraph wg) 		{ return _dim[wg]; }
	inline int &		GetId( void )			{ return _id; }
	inline double &		GetPos( WhichGraph wg )		{ return _pos[wg]; }

protected:
	// Items are organized this way to improve memory packing.
	double		_dim[2];
	double		_pos[2];
	Block		*_blk;
	int		_id;
};


class MinMovementFloorplanner::FPGraph
// ***********************************
// This is a specialized graph that I use for representing the TCG of a
// placement.  It is very efficient at traversing nodes as well as a node's
// in/out edges.  (It is completely worthless for traversing the edge set --
// that functionality just adds overhead and is not important.)
{
protected:
	typedef std::pair<unsigned, unsigned>	Edge_t;

	FPGraph( void )
	// ************
	{
		_numEdges = 0;
		_topoSortIsValid = false;
		_isReduced = false;
	}

	inline void AddEdge( unsigned src, unsigned dest )
	// *******************************************
	{
		_outEdges[src].push_back( dest );
		_inEdges[dest].push_back( src );
		++_numEdges;

		// Adding an edge invalidates a topological sort.
		_topoSortIsValid = false;
		_isReduced = false;
	}

	inline void Clear( void )
	// **********************
	{
		unsigned	i;

		for( i = 0; i < _outEdges.size(); ++i ) {
			_outEdges[i].clear();
		}
		_outEdges.clear();

		for( i = 0; i < _inEdges.size(); ++i ) {
			_inEdges[i].clear();
		}
		_inEdges.clear();

		_nodes.clear();
		_numEdges = 0;
		_tmpVec.clear();
		_visit.clear();
		_topoSortIsValid = false;
		_isReduced = false;
	}

	inline void Copy( FPGraph &src )
	// *****************************
	{
		_inEdges = src._inEdges;
		_nodes = src._nodes;
		_numEdges = src._numEdges;
		_outEdges = src._outEdges;

		_isReduced = src._isReduced;
		_tmpVec = src._tmpVec;
		_topoSortCached = src._topoSortCached;
		_topoSortIsValid = src._topoSortIsValid;
		_visit = src._visit;
	}

	inline bool DoesEdgeExist( unsigned src, unsigned dest )
	// *************************************************
	// Returns true if the edge exists, false otherwise.
	{
		unsigned	 i;

		for( i = 0; i < _outEdges[src].size(); ++i ) {
			if( _outEdges[src][i] == dest ) {
				return true;
			}
		}
		return false;
	}

	bool DoesPathExist( unsigned src, unsigned dest )
	// ******************************************
	// Returns true if there is a path (direct or transitive) from src to
	// dest.
	{
		unsigned			cur, i;

		std::fill( _visit.begin(), _visit.end(), false );

		_tmpVec.clear();
		_tmpVec.push_back( src );

		while( !_tmpVec.empty() ) {
			cur = _tmpVec.back();
			_tmpVec.pop_back();

			if( _visit[cur] )	continue;
			_visit[cur] = true;

			if( cur == dest )	return true;

			for( i = 0; i < _outEdges[cur].size(); ++i ) {
				_tmpVec.push_back( _outEdges[cur][i] );
			}
		}
		return false;
	}

	inline unsigned		GetInEdgeSrc( unsigned src, unsigned idx )	{ return _inEdges[src][idx]; }
	inline unsigned		GetOutEdgeDest( unsigned src, unsigned idx ){ return _outEdges[src][idx]; }
	inline unsigned		GetNumEdges( void )			{ return _numEdges; }
	inline unsigned		GetNumNodes( void )			{ return _nodes.size(); }
	inline unsigned		GetNumInEdges( unsigned src )		{ return _inEdges[src].size(); }
	inline unsigned		GetNumOutEdges( unsigned src )		{ return _outEdges[src].size(); }

	void RemEdge( unsigned src, unsigned dest )
	// ************************************
	// This is the most time-consuming operation, but we can speed it up by
	// fiddling with the vector order.
	{
		unsigned				i;
		std::vector<unsigned>::iterator	iteOut, iteIn, lastIte;
		std::vector<unsigned>		*vecptr;
		unsigned				lookingFor;

		for( i = 0; i < 2; ++i ) {
			if( i == 0 ) {
				vecptr = &( _outEdges[src] );
				lookingFor = dest;
			} else {
				vecptr = &( _inEdges[dest] );
				lookingFor = src;
			}

			iteOut = std::find( vecptr->begin(), vecptr->end(), lookingFor );

			// Swap with the back, then pop the back.
			lastIte = vecptr->end() - 1;
			if( iteOut != vecptr->end() - 1 ) {
				std::swap( *iteOut, *lastIte );
			}
			vecptr->pop_back();
		}
		_numEdges--;
	}

	inline void Resize( unsigned size )
	// ******************************
	{
		Clear();

		_nodes.resize( size );
		_outEdges.resize( size );
		_inEdges.resize( size );
		_tmpVec.reserve( size );
		_visit.resize( size );

		for( unsigned i = 0; i < size; ++i ) { 
			_outEdges[i].clear();
			_outEdges[i].reserve( 5 );	// Reserve a bit of space up front for edges.
			_inEdges[i].clear();
			_inEdges[i].reserve( 5 );	// Reserve a bit of space up front for edges.
		}
	}

	void TopologicalSort( std::vector<int> &order )
	// ***********************************************
	// Computes a topological ordering of the graph.
	{
		unsigned		i, j, k;
		unsigned		tgt;

		// I cache a copy of the topological sort for performance.
		if( _topoSortIsValid ) {
			order = _topoSortCached;
			return;
		}

		_tmpVec.resize( _nodes.size() );

		std::fill( _visit.begin(), _visit.end(), false );
		std::fill( _tmpVec.begin(), _tmpVec.end(), 0 );

		order.clear();
		order.reserve( _nodes.size() );

		// Setup the in-degree of the nodes.
		for( i = 0; i < _nodes.size(); ++i ) {
			_tmpVec[i] = this->GetNumInEdges( i );
			if( _tmpVec[i] == 0 ) {
				order.push_back( i );
				_visit[i] = true;
			}
		}

		// Sanity.

		i = 0 ;
		while( i < order.size() ) {
			j = order[i];
			++i;

			for( k = 0; k < this->GetNumOutEdges( j ); ++k ) {
				tgt = this->GetOutEdgeDest( j, k );
				--( _tmpVec[tgt] );

				if( _tmpVec[tgt] == 0 ) {
					// Sanity.

					_visit[tgt] = true;
					order.push_back( tgt );
				}
			}
		}

        if( order.size() != _nodes.size() ) {
            std::cout << "Number of nodes is " << _nodes.size() << ", Order is " << order.size() << std::endl;
        }

		_topoSortCached = order;
		_topoSortIsValid = true;
	}

	void TransitiveReduction( void )
	{
	    // Perform transitive reduction on the graph.  It works as follows: Topologically order the
        // nodes.  Proceed in reverse order.  Given a node 'i' consider its neighbors S_i.  Find 
        // out what nodes are reachable from S_i and call this R_i.  Then, consider 'j' in S_i.  
        // If 'j' is in R_i, then 'j' is reachable from 'k' in S_i.  That is, we have i -> j, but
        // we also have i -> k -> ... -> j.   So, we do not need i -> j.
		int					i;
		unsigned					k;
		std::vector<int> 			order;
		//std::vector< boost::dynamic_bitset<> >	reachable;
        std::vector< std::set<unsigned> > reachable;
		std::vector< Edge_t >			redundantEdges;
		unsigned					src, tgt;
		Edge_t					tmpEdge;

		if( _isReduced || this->GetNumNodes() <= 1 || this->GetNumEdges() <= 1 ) {
			return;
		}

		redundantEdges.reserve( this->GetNumEdges() );
		reachable.resize( _nodes.size() );

        // XXX: This is going to kill memory...
		//for( i = 0; (unsigned)i < reachable.size(); ++i ) {
		//	reachable[i].resize( _nodes.size() );
		//}

		TopologicalSort( order );

		// NB: Walk through the topologically-sorted nodes in REVERSE order!
		for( i = order.size() - 1; i >= 0; i-- ) {
			src = order[i];

			// Update the bitset.
			for( k = 0; k < this->GetNumOutEdges( src ); ++k ) {
				tgt = this->GetOutEdgeDest( src, k );
				//reachable[src] |= reachable[tgt];
                reachable[src].insert( reachable[tgt].begin(), reachable[tgt].end() );
			}

			// Scan for redundancies....
			for( k = 0; k < this->GetNumOutEdges( src ); ++k ) {
				tgt = this->GetOutEdgeDest( src, k );

				//if( reachable[src][tgt] ) {
				if( reachable[src].end() != reachable[src].find( tgt ) ) {
					tmpEdge.first = src;
					tmpEdge.second = tgt;
					redundantEdges.push_back( tmpEdge );
				} else {
                    reachable[src].insert( tgt );
					//reachable[src][tgt] = true;
				}
			}
		}

		// Remove the redundant edges.
		for( i = 0; (unsigned)i < redundantEdges.size(); ++i ) {
			this->RemEdge( redundantEdges[i].first, redundantEdges[i].second );
		}
		_isReduced = true;
	}

	void WriteGraphViz( std::string str,
			std::vector<Block *> &blocks, 
			WhichGraph wg )
	// **********************************************
	// Dumps the graph in GraphViz format.  
	// 
	// blocks => used for naming blocks
	// str => filename
	// wg => to draw graph in left-right or bottom-up fashion
	{
		static unsigned	counter = 0;
		static char	filename[BUFFER_SIZE + 1];
		FILE		*fp;
		unsigned		i, j;
        char buf[1024];
        (void) buf;

        sprintf(&buf[0], "Graphviz counter: %d", counter );

		snprintf( filename, BUFFER_SIZE, "%s.%d.gv", str.c_str(), counter++ );

		if( ( fp = fopen( filename, "w" ) ) == 0 ) 
        {
            sprintf( &buf[0], "Error opening file (%s)", filename );
            std::cout << buf << std::endl;
			return;
		}
		fprintf( fp, 	"digraph G {\n"
				"rankdir=%s\n"
				"center=1\n",
				wg == HGraph ? "LR" : "BT" );

		double worstSlack = INFINITY;
		for( i = 0; i < _nodes.size(); ++i ) {
			Slack *slk = blocks[i]->GetSlack( wg );
			worstSlack = std::min( worstSlack, slk->ComputeSlack() );
		}

		for( i = 0; i < _nodes.size(); ++i ) {
			Slack *slk = blocks[i]->GetSlack( wg );
			if( slk->ComputeSlack() > worstSlack )	continue;
			fprintf( fp, "%d [ shape=polygon label=\"%d\\n%.1f x %f\\n(%.1f, %.1f)\" ];\n", i, i,
					blocks[i]->GetDim( HGraph ),
					blocks[i]->GetDim( VGraph ),
					blocks[i]->GetPos( HGraph ),
					blocks[i]->GetPos( VGraph ) );
		}
		for( i = 0; i < _nodes.size(); ++i ) {
			Slack *slki = blocks[i]->GetSlack( wg );
			if( slki->ComputeSlack() > worstSlack )	continue;

			for( j = 0; j < _outEdges[i].size(); ++j ) {
				Slack *slkj = blocks[_outEdges[i][j]]->GetSlack( wg );
				if( slkj->ComputeSlack() > worstSlack )	continue;

				fprintf( fp, "%d->%d ;\n", i, _outEdges[i][j] );
			}
		}
		fprintf( fp, "}\n" );
		fclose( fp );
	}

	friend class MinMovementFloorplanner;

protected:
	std::vector< std::vector< unsigned > >	_inEdges;		// In edges for each node.
	std::vector<unsigned>			_nodes;			// Nodes in the problem
	unsigned					_numEdges;		// Num edges in the problem
	std::vector< std::vector< unsigned > >	_outEdges;		// Out edges for each node.

	// Miscellaneous variables used in the algorithms.  These are
	// pre-allocated vectors -- this way, I don't have to keep
	// re-allocating them every time an algorithm gets called.
	bool					_isReduced;		// Used for recording if the graph is reduced.
	std::vector<unsigned>			_tmpVec;		// Used for queueing, counting.
	std::vector<int>			_topoSortCached;	// Cached topological sort
	bool					_topoSortIsValid;	// Used for caching the topo sort
	std::vector<bool>			_visit;			// Used for tracking visitors.
};


struct MinMovementFloorplanner::Arc
// ********************************
{
	EdgePair	_ep;
	double		_crit;
	double		_dist;
	WhichGraph	_wg;
};


struct MinMovementFloorplanner::SortArcs
// *************************************
{
	inline bool operator() ( const Arc &p, const Arc &q ) const
	// ****************************************************************
	{
		if( p._dist == q._dist ) {
			return p._crit < q._crit;	// Prefer least-critical
		}
		return p._dist < q._dist;		// Prefer closest, above all else
	}
};


struct MinMovementFloorplanner::SortBlocksByDimension
// **************************************************
{
	inline bool operator() ( Block *p, Block *q ) const
	// ************************************************
	{
		if( p->GetDim(HGraph) == q->GetDim(HGraph) ) {
			if( p->GetDim(VGraph) == q->GetDim(VGraph) ) {
				// Same dimension, so there's no other way to
				// order these blocks.
				return false;
			}
			return p->GetDim(VGraph) < q->GetDim(VGraph);
		}
		return p->GetDim(HGraph) < q->GetDim(HGraph);
	}
};

struct MinMovementFloorplanner::SortBlocksByX
{
    inline bool operator()(Block* p, Block* q ) const {
        if(  p->GetPos(HGraph) == q->GetPos(HGraph) ) {
            return p->GetId() < q->GetId();
        }
        return p->GetPos(HGraph) < q->GetPos(HGraph);
    }
};


struct BlockSet
// ************
{
	double					_crit;
	MinMovementFloorplanner::Block		*_blk;
};


struct SortBlocksInXInc
// ********************
{
	inline bool operator() ( MinMovementFloorplanner::Block *p, 
				MinMovementFloorplanner::Block *q ) const
	// **************************************************************
	{
		if( p->GetPos(MinMovementFloorplanner::HGraph) == q->GetPos(MinMovementFloorplanner::HGraph) ) { 
			double	pXmin = p->GetPos(MinMovementFloorplanner::HGraph) - 
				0.5 * p->GetDim(MinMovementFloorplanner::HGraph);
			double	qXmin = q->GetPos(MinMovementFloorplanner::HGraph) - 
				0.5 * q->GetDim(MinMovementFloorplanner::HGraph);

			if( pXmin == qXmin ) {
				return p->GetId() < q->GetId();
			}
			return pXmin < qXmin;
		}
		return( p->GetPos(MinMovementFloorplanner::HGraph) < 
				q->GetPos(MinMovementFloorplanner::HGraph) );
	}
};


struct SortBlockSet
// ****************
{
	inline bool operator() ( const BlockSet &p, const BlockSet &q ) const
	// ****************************************************************
	{
		return p._crit > q._crit;		// Prefer most critical blocks
	}
};


struct MinMovementFloorplanner::EdgeSet
// ************************************
{
	std::vector<EdgePair>	_pairs;
	double			_crit;
};


struct MinMovementFloorplanner::SortEdgeSet
// ****************************************
{
	inline bool operator() ( const EdgeSet &p, const EdgeSet &q ) const
	// ****************************************************************
	{
		return p._crit > q._crit;		// Prefer most critical edges
	}
};


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 
// MAIN CODE STARTS HERE...
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

MinMovementFloorplanner::MinMovementFloorplanner( 
		MinMovementFloorplannerParams& params, Placer_RNG* rng ) :
			m_params( params ),
            m_rng( rng )
{
	m_graph[HGraph] = new FPGraph;
	m_graph[VGraph] = new FPGraph;

	m_backupGraph[HGraph] = new FPGraph;
	m_backupGraph[VGraph] = new FPGraph;

	_subgraph[HGraph] = new FPGraph;
	_subgraph[VGraph] = new FPGraph;

    m_timeInserting = 0;
    m_timeMovingOrReversing = 0;
}


MinMovementFloorplanner::~MinMovementFloorplanner( void )
{
	delete m_graph[HGraph];
	delete m_graph[VGraph];

	delete m_backupGraph[HGraph];
	delete m_backupGraph[VGraph];

	delete _subgraph[HGraph];
	delete _subgraph[VGraph];
}

bool MinMovementFloorplanner::adjustSCByInserting( void )
{
	m_pathCountB[HGraph].resize( m_blocks.size() );
	m_pathCountB[VGraph].resize( m_blocks.size() );
	m_pathCountF[HGraph].resize( m_blocks.size() );
	m_pathCountF[VGraph].resize( m_blocks.size() );

    // Attempt to move standard cells around a little bit in order to reduce any slack
    // violations.  Basically, moves cells between rows into nearby positions to reduce
    // violations.
    std::vector<Arc> arcs;

	computeSlack( HGraph );
    std::cout << "Worst slack: " << m_worstSlack[HGraph] << std::endl;

    Block* bptr = 0;
	for( int p = 0; p < m_blocks.size(); p++ ) {
		if( m_worstSlack[HGraph] >= 0. ) {
            // Done...  Things fit.
			goto cleanup;
		}

		// Find a critical block to move.
        std::cout << "Path counting..." << std::endl;
		pathCounting( HGraph );
        std::cout << "Getting block..." << std::endl;
        if( (bptr = getCandidateBlock( HGraph )) == 0 ) {
            std::cout << "No block." << std::endl;
            break;
        }

        // Find appealing arcs.  Arcs should be close...
		arcs.clear();
		arcs.reserve( m_graph[HGraph]->GetNumEdges() + m_graph[VGraph]->GetNumEdges() );
		getCandidateArcs( HGraph, bptr, arcs );
        if( arcs.empty() ) {
            continue;
        }

		Utility::random_shuffle( arcs.begin(), arcs.end(), m_rng );
		std::sort( arcs.begin(), arcs.end(), SortArcs() );

        // Move the chosen block into the chosen arc.  We need to make some modifications 
        // to the graph when we do this by removing and inserting some edges.  I *think*
        // for a standard cell, there should only be one incoming and one outgoing edge
        // at the most...
        std::cout << "Trying to insert block " << bptr->GetId() << " @ " << "(" << bptr->GetPos(HGraph) << "," << bptr->GetPos(VGraph) << ")" << ", "
             << "Number in edges is " << m_graph[HGraph]->GetNumInEdges( bptr->GetId() ) << ", "
             << "Number out edges is " << m_graph[HGraph]->GetNumOutEdges( bptr->GetId() ) 
             << std::endl;

        if( m_graph[HGraph]->GetNumInEdges( bptr->GetId() ) >= 2 || m_graph[HGraph]->GetNumOutEdges( bptr->GetId() ) >= 2 ) {
            std::cout << "Weird edge count."  << std::endl;
            continue;
        }

        int r = ( m_graph[HGraph]->GetNumOutEdges(bptr->GetId()) == 1 ) 
                        ? m_graph[HGraph]->GetOutEdgeDest( bptr->GetId(), 0 )
                        : -1
                        ;
        int l = ( m_graph[HGraph]->GetNumInEdges(bptr->GetId()) == 1 ) 
                        ? m_graph[HGraph]->GetInEdgeSrc( bptr->GetId(), 0 )
                        : -1
                        ;

        if( l != -1 && r != -1 && !m_blocks[l]->GetFixed() && !m_blocks[r]->GetFixed() ) {
            // Add edge from l -> r.
            m_graph[HGraph]->AddEdge( l, r );
        }
        if( l != -1 ) {
            // Remove edge l -> bptr->GetId().
            m_graph[HGraph]->RemEdge( l, bptr->GetId() );
        }
        if( r != -1 ) {
            // Remove edge bptr->GetId() -> r.
            m_graph[HGraph]->RemEdge( bptr->GetId(), r );
        }

        // Now, add edges for the new constraints and update position of bptr.
        l = arcs[0]._ep._src;
        r = arcs[0]._ep._dst;
        m_graph[HGraph]->AddEdge( l, bptr->GetId() );
        m_graph[HGraph]->AddEdge( bptr->GetId(), r );

        // Hmmmm... Need to update the Y-position...  How to do this????

        // Recompute slack.
        computeSlack( HGraph );
        std::cout << "Worst slack: " << m_worstSlack[HGraph] << std::endl;
	}
cleanup:
	return( m_worstSlack[HGraph] >= 0. );
}



bool MinMovementFloorplanner::minshift( 
            DetailedMgr& mgr,
            double xmin, double xmax, 
            double ymin, double ymax,
            std::vector<Block*> &blocks )
{
    m_net = mgr.getNetwork();
    m_arch = mgr.getArchitecture();

    // Legalize the blocks with minimum movement heuristics.  Return true for success of 
    // some sort (this does not mean legalized, it only means the answer is meaningfull).
    // Return false for a nervous breakdown.
    //
    // This routine does *NOT* change positions of the nodes in the network.  It modifies
    // positions of the blocks.  

	m_Tmin[HGraph] = xmin;
	m_Tmax[HGraph] = xmax;
	m_Tmin[VGraph] = ymin;
	m_Tmax[VGraph] = ymax;

    // Turn the blocks into a member variable.
	m_blocks = blocks;

    // Compute a tolerance for certain checks.
	initialize();

    // Construct a constraint graph in the X-direction only.
    m_graph[HGraph]->Resize( m_blocks.size() );
    m_graph[VGraph]->Clear();

    std::vector<Block*> sortedBlocks;
    for( int i = 0; i < m_blocks.size(); i++ ) 
    {
        sortedBlocks.push_back( m_blocks[i] );
    }
    std::sort( sortedBlocks.begin(), sortedBlocks.end(), SortBlocksInXInc() );


    for( int i = 0; i < sortedBlocks.size(); i++ ) 
    {
        Block* blk_i = sortedBlocks[i];

        double ymin_i = blk_i->GetPos( VGraph ) - 0.5 * blk_i->GetDim( VGraph ) + 1.0e-3;
        double ymax_i = blk_i->GetPos( VGraph ) + 0.5 * blk_i->GetDim( VGraph ) - 1.0e-3;

        for( int j = i+1; j < sortedBlocks.size(); j++ ) {
            Block* blk_j = sortedBlocks[j];

            double ymin_j = blk_j->GetPos( VGraph ) - 0.5 * blk_j->GetDim( VGraph ) + 1.0e-3;
            double ymax_j = blk_j->GetPos( VGraph ) + 0.5 * blk_j->GetDim( VGraph ) - 1.0e-3;

            // Handle the various cases.
            bool doAdd = false;
            if     ( ymax_j >= ymax_i && ymin_j >= ymin_i && ymin_j < ymax_i ) {
                doAdd = true; // Extends above.
            } 
            else if( ymax_j <= ymax_i && ymax_j > ymin_i && ymin_j <= ymin_i ) {
                doAdd = true; // Extends below.
            } 
            //else if( ymax_j >= ymin_i && ymin_j <= ymin_i ) // Error?
            else if( ymax_j >= ymax_i && ymin_j <= ymin_i ) 
            {
                doAdd = true; // Extends throughout.
            } 
            else if( ymax_j <= ymax_i && ymin_j >= ymin_i ) {
                doAdd = true; // Exists inside.
            }
            if( doAdd ) 
            { 
                if( !m_graph[HGraph]->DoesEdgeExist( blk_i->GetId(), blk_j->GetId() ) ) 
                { 
                    m_graph[HGraph]->AddEdge( blk_i->GetId(), blk_j->GetId() ); 
                }
            } 
        }
    }

    // Reduces the number of constraints.
	//m_graph[HGraph]->TransitiveReduction();

	computeSlack(HGraph);

    bool legal = m_worstSlack[HGraph] >= 0.;

    // XXX: Removed the LP solver and resorted to clumping.
    double amtResizedX = 0.;
    double amtMovedX = 0.;
    bool shift = clump( amtMovedX, amtResizedX );
    std::cout << "Clumping, total quadratic movement in X-direction is " << amtMovedX << ", "
        << "Resized is " << amtResizedX << std::endl;

    return shift;
}

void MinMovementFloorplanner::collectNodes()
{
    // Get the nodes involved in the problem.  This includes fixed blocks, filler cells, macro cells
    // and double height standard cells.  Note that we can ignore fixed blocks if they are entirely
    // outside of the core area.

    double rowHeight = m_arch->m_rows[0]->m_rowHeight;

    std::map<Node*,std::vector<Network::Shape>*>::iterator it;
    int fixed = 0;
    int other = 0;
    int count = 0;
    int shape = 0;
    int filler = 0;

    m_fixedNodes.erase( m_fixedNodes.begin(), m_fixedNodes.end() );
    m_largeNodes.erase( m_largeNodes.begin(), m_largeNodes.end() );
    m_doubleHeightNodes.erase( m_doubleHeightNodes.begin(), m_doubleHeightNodes.end() );

    // Architectural filler to account for gaps in rows...
    for( int i = 0; i < m_net->m_filler.size(); i++ ) {
        Node* ndi = m_net->m_filler[i];

        double xmin = ndi->getX() - 0.5 * ndi->getWidth();
        double xmax = ndi->getX() + 0.5 * ndi->getWidth();
        double ymin = ndi->getY() - 0.5 * ndi->getHeight();
        double ymax = ndi->getY() + 0.5 * ndi->getHeight();

        if( !(xmax <= m_arch->m_xmin || xmin >= m_arch->m_xmax ||
              ymax <= m_arch->m_ymin || ymin >= m_arch->m_ymax) ) 
        {
            m_fixedNodes.push_back( ndi );
        }

    }
    // Network nodes which are fixed or have shapes...
    for( int i = 0; i < m_net->m_nodes.size(); i++ ) {
        Node* ndi = &(m_net->m_nodes[i]);
        if( ndi->getType() == NodeType_TERMINAL_NI ) {
            continue;
        }
        if( ndi->getFixed() == NodeFixed_NOT_FIXED ) {
            continue;
        }

        // Fixed node...
        std::vector<Node*>& shapes = m_net->m_shapes[ndi->getId()];
        if( shapes.size() == 0 ) {
            // No shape information.

            double xmin = ndi->getX() - 0.5 * ndi->getWidth();
            double xmax = ndi->getX() + 0.5 * ndi->getWidth();
            double ymin = ndi->getY() - 0.5 * ndi->getHeight();
            double ymax = ndi->getY() + 0.5 * ndi->getHeight();

            if( !(xmax <= m_arch->m_xmin || xmin >= m_arch->m_xmax ||
                  ymax <= m_arch->m_ymin || ymin >= m_arch->m_ymax) ) 
            {
                m_fixedNodes.push_back( ndi );
            }
        }
        else {
            // Shape information.

            for( int j = 0; j < shapes.size(); j++ ) {
                Node* ndj = shapes[j];

                double xmin = ndj->getX() - 0.5 * ndj->getWidth();
                double xmax = ndj->getX() + 0.5 * ndj->getWidth();
                double ymin = ndj->getY() - 0.5 * ndj->getHeight();
                double ymax = ndj->getY() + 0.5 * ndj->getHeight();

                if( !(xmax <= m_arch->m_xmin || xmin >= m_arch->m_xmax ||
                      ymax <= m_arch->m_ymin || ymin >= m_arch->m_ymax) ) 
                {
                    m_fixedNodes.push_back( ndj );
                }
            }
        }
    }
    // end of fixed...

    // Large macrocells and/or double height cells...
    for( int i = 0; i < m_net->m_nodes.size(); i++ ) {
        Node* ndi = &(m_net->m_nodes[i]);
        if( ndi->getType() == NodeType_TERMINAL_NI ) {
            continue;
        }
        if( ndi->getFixed() != NodeFixed_NOT_FIXED ) {
            continue;
        }
        if( ndi->getHeight() - 1.0e-3 <= rowHeight  ) {
            continue;
        }

        if( ndi->getHeight() - 1.0e-3 >= 2*rowHeight ) {
            m_largeNodes.push_back( ndi );
        }
        else {
            m_doubleHeightNodes.push_back( ndi );
        }
    }
}

void MinMovementFloorplanner::createSingleProblem( std::vector<Block*>& blocks, std::map<Node*,Block*>& nodeToBlockMap )
{
    // Creates blocks for all involved nodes.

    double rowHeight = m_arch->m_rows[0]->m_rowHeight;

    for( int i = 0; i < blocks.size(); i++ ) {
        delete blocks[i];
    }
	blocks.clear();
	nodeToBlockMap.clear();
    Block* bptr = 0;

    int count = 0;
    for( int i = 0; i < m_fixedNodes.size(); i++ ) {
        Node* nd = m_fixedNodes[i];

        double xmin = std::max( nd->getX() - 0.5 * nd->getWidth(), m_arch->m_xmin );
        double xmax = std::min( nd->getX() + 0.5 * nd->getWidth(), m_arch->m_xmax );
        double ymin = std::max( nd->getY() - 0.5 * nd->getHeight(), m_arch->m_ymin );
        double ymax = std::min( nd->getY() + 0.5 * nd->getHeight(), m_arch->m_ymax );

        if( xmax <= m_arch->m_xmin ) continue;
        if( xmin >= m_arch->m_xmax ) continue;
        if( ymax <= m_arch->m_ymin ) continue;
        if( ymin >= m_arch->m_ymax ) continue;

        bptr = new Block;
        bptr->SetFixed( true );
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = xmax-xmin;
        bptr->GetOrigDim( VGraph ) = ymax-ymin;
        bptr->GetOrigPos( HGraph ) = 0.5*(xmax+xmin);
        bptr->GetOrigPos( VGraph ) = 0.5*(ymax+ymin);

        bptr->GetDim( HGraph ) = xmax-xmin;
        bptr->GetDim( VGraph ) = ymax-ymin;
        bptr->GetPos( HGraph ) = 0.5*(xmax+xmin);
        bptr->GetPos( VGraph ) = 0.5*(ymax+ymin);

        blocks.push_back( bptr );
        ++count;
    }

    for( int i = 0; i < m_largeNodes.size(); i++ ) {
        Node* nd = m_largeNodes[i];

        bptr = new Block;
        bptr->SetFixed( false );
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = nd->getWidth();
        bptr->GetOrigDim( VGraph ) = nd->getHeight();
        bptr->GetOrigPos( HGraph ) = nd->getX();
        bptr->GetOrigPos( VGraph ) = nd->getY();

        bptr->GetDim( HGraph ) = nd->getWidth();
        bptr->GetDim( VGraph ) = nd->getHeight();
        bptr->GetPos( HGraph ) = nd->getX();
        bptr->GetPos( VGraph ) = nd->getY();

        blocks.push_back( bptr );
        ++count;

        nodeToBlockMap[nd] = bptr;
    }

    for( int i = 0; i < m_doubleHeightNodes.size(); i++ ) {
        Node* nd = m_doubleHeightNodes[i];

        bptr = new Block;
        bptr->SetFixed( false );
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = nd->getWidth();
        bptr->GetOrigDim( VGraph ) = nd->getHeight();
        bptr->GetOrigPos( HGraph ) = nd->getX();
        bptr->GetOrigPos( VGraph ) = nd->getY();

        bptr->GetDim( HGraph ) = nd->getWidth();
        bptr->GetDim( VGraph ) = nd->getHeight();
        bptr->GetPos( HGraph ) = nd->getX();
        bptr->GetPos( VGraph ) = nd->getY();

        blocks.push_back( bptr );
        ++count;

        nodeToBlockMap[nd] = bptr;
    }

    std::cout << "Total blocks created is " << blocks.size() << std::endl;
}

void MinMovementFloorplanner::createLargeProblem( std::vector<Block*>& blocks, std::map<Node*,Block*>& nodeToBlockMap )
{
    // Creates blocks for all involved nodes.

    double rowHeight = m_arch->m_rows[0]->m_rowHeight;

    for( int i = 0; i < blocks.size(); i++ ) {
        delete blocks[i];
    }
	blocks.clear();
	nodeToBlockMap.clear();
    Block* bptr = 0;

    int count = 0;
    for( int i = 0; i < m_fixedNodes.size(); i++ ) {
        Node* nd = m_fixedNodes[i];

        double xmin = std::max( nd->getX() - 0.5 * nd->getWidth(), m_arch->m_xmin );
        double xmax = std::min( nd->getX() + 0.5 * nd->getWidth(), m_arch->m_xmax );
        double ymin = std::max( nd->getY() - 0.5 * nd->getHeight(), m_arch->m_ymin );
        double ymax = std::min( nd->getY() + 0.5 * nd->getHeight(), m_arch->m_ymax );

        if( xmax <= m_arch->m_xmin ) continue;
        if( xmin >= m_arch->m_xmax ) continue;
        if( ymax <= m_arch->m_ymin ) continue;
        if( ymin >= m_arch->m_ymax ) continue;

        bptr = new Block;
        bptr->SetFixed( true );
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = xmax-xmin;
        bptr->GetOrigDim( VGraph ) = ymax-ymin;
        bptr->GetOrigPos( HGraph ) = 0.5*(xmax+xmin);
        bptr->GetOrigPos( VGraph ) = 0.5*(ymax+ymin);

        bptr->GetDim( HGraph ) = xmax-xmin;
        bptr->GetDim( VGraph ) = ymax-ymin;
        bptr->GetPos( HGraph ) = 0.5*(xmax+xmin);
        bptr->GetPos( VGraph ) = 0.5*(ymax+ymin);

        blocks.push_back( bptr );
        ++count;
    }

    for( int i = 0; i < m_largeNodes.size(); i++ ) {
        Node* nd = m_largeNodes[i];

        bptr = new Block;
        bptr->SetFixed( false );
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = nd->getWidth();
        bptr->GetOrigDim( VGraph ) = nd->getHeight();
        bptr->GetOrigPos( HGraph ) = nd->getX();
        bptr->GetOrigPos( VGraph ) = nd->getY();

        bptr->GetDim( HGraph ) = nd->getWidth();
        bptr->GetDim( VGraph ) = nd->getHeight();
        bptr->GetPos( HGraph ) = nd->getX();
        bptr->GetPos( VGraph ) = nd->getY();

        blocks.push_back( bptr );
        ++count;

        nodeToBlockMap[nd] = bptr;
    }

    std::cout << "Total blocks created is " << blocks.size() << std::endl;
}







int MinMovementFloorplanner::collectBlocks(    
        std::vector<Block*>& blocks, 
        std::map<Node*,Block*>& nodeToBlockMap )
{
    // Collects the macros and other desireable cells; stores them in 'blocks'.

    double rowHeight = m_arch->m_rows[0]->m_rowHeight;

	blocks.clear();
	nodeToBlockMap.clear();
    Block* bptr = 0;

    std::map<Node*,std::vector<Network::Shape>*>::iterator it;
    int fixed = 0;
    int other = 0;
    int count = 0;
    int shape = 0;
    int filler = 0;

    // Architecture filler to account for gaps in rows...
    for( int i = 0; i < m_net->m_filler.size(); i++ ) {
        Node* nd = m_net->m_filler[i];

        double xmin = std::max( nd->getX() - 0.5 * nd->getWidth(), m_arch->m_xmin );
        double xmax = std::min( nd->getX() + 0.5 * nd->getWidth(), m_arch->m_xmax );
        double ymin = std::max( nd->getY() - 0.5 * nd->getHeight(), m_arch->m_ymin );
        double ymax = std::min( nd->getY() + 0.5 * nd->getHeight(), m_arch->m_ymax );

        if( xmax <= m_arch->m_xmin ) continue;
        if( xmin >= m_arch->m_xmax ) continue;
        if( ymax <= m_arch->m_ymin ) continue;
        if( ymin >= m_arch->m_ymax ) continue;

        bptr = new Block;
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = xmax-xmin;
        bptr->GetOrigDim( VGraph ) = ymax-ymin;
        bptr->GetOrigPos( HGraph ) = 0.5*(xmax+xmin);
        bptr->GetOrigPos( VGraph ) = 0.5*(ymax+ymin);

        bptr->GetDim( HGraph ) = xmax-xmin;
        bptr->GetDim( VGraph ) = ymax-ymin;
        bptr->GetPos( HGraph ) = 0.5*(xmax+xmin);
        bptr->GetPos( VGraph ) = 0.5*(ymax+ymin);

        bptr->SetFixed( true );

        blocks.push_back( bptr );

        ++count;
        ++fixed;
        ++filler;
    }

    // Network fixed, moveable and shapes...
    for( int i = 0; i < m_net->m_nodes.size(); i++ ) {
        Node* nd = &(m_net->m_nodes[i]);
        if( nd->getType() == NodeType_TERMINAL_NI ) {
            continue;
        }
        if( nd->getFixed() != NodeFixed_NOT_FIXED ) {
            if( m_net->m_shapes[nd->getId()].size() != 0 ) {
                for( int j = 0; j < m_net->m_shapes[nd->getId()].size(); j++ ) {
                    Node* shape = m_net->m_shapes[nd->getId()][j];

                    // If outside of the placement area, skip it.
                    double xmin = std::max( shape->getX() - 0.5 * shape->getWidth(), m_arch->m_xmin );
                    double xmax = std::min( shape->getX() + 0.5 * shape->getWidth(), m_arch->m_xmax );
                    double ymin = std::max( shape->getY() - 0.5 * shape->getHeight(), m_arch->m_ymin );
                    double ymax = std::min( shape->getY() + 0.5 * shape->getHeight(), m_arch->m_ymax );

                    if( xmax <= m_arch->m_xmin ) continue;
                    if( xmin >= m_arch->m_xmax ) continue;
                    if( ymax <= m_arch->m_ymin ) continue;
                    if( ymin >= m_arch->m_ymax ) continue;

                    bptr = new Block;
                    bptr->GetId() = count;

                    bptr->GetOrigDim( HGraph ) = xmax-xmin;
                    bptr->GetOrigDim( VGraph ) = ymax-ymin;
                    bptr->GetOrigPos( HGraph ) = 0.5*(xmax+xmin);
                    bptr->GetOrigPos( VGraph ) = 0.5*(ymax+ymin);

                    bptr->GetDim( HGraph ) = xmax-xmin;
                    bptr->GetDim( VGraph ) = ymax-ymin;
                    bptr->GetPos( HGraph ) = 0.5*(xmax+xmin);
                    bptr->GetPos( VGraph ) = 0.5*(ymax+ymin);

                    bptr->SetFixed( true );

                    blocks.push_back( bptr );

                    ++count;
                    ++fixed;
                    ++shape;
                }
            }
            else {
                // Use single fixed node...
        
                double xmin = std::max( nd->getX() - 0.5 * nd->getWidth(), m_arch->m_xmin );
                double xmax = std::min( nd->getX() + 0.5 * nd->getWidth(), m_arch->m_xmax );
                double ymin = std::max( nd->getY() - 0.5 * nd->getHeight(), m_arch->m_ymin );
                double ymax = std::min( nd->getY() + 0.5 * nd->getHeight(), m_arch->m_ymax );

                if( xmax <= m_arch->m_xmin ) continue;
                if( xmin >= m_arch->m_xmax ) continue;
                if( ymax <= m_arch->m_ymin ) continue;
                if( ymin >= m_arch->m_ymax ) continue;

                bptr = new Block;
                bptr->GetId() = count;

                bptr->GetOrigDim( HGraph ) = xmax-xmin;
                bptr->GetOrigDim( VGraph ) = ymax-ymin;
                bptr->GetOrigPos( HGraph ) = 0.5*(xmax+xmin);
                bptr->GetOrigPos( VGraph ) = 0.5*(ymax+ymin);

                bptr->GetDim( HGraph ) = xmax-xmin;
                bptr->GetDim( VGraph ) = ymax-ymin;
                bptr->GetPos( HGraph ) = 0.5*(xmax+xmin);
                bptr->GetPos( VGraph ) = 0.5*(ymax+ymin);

                bptr->SetFixed( true );

                blocks.push_back( bptr );

                ++count;
                ++fixed;
            }
            continue;
        }
        if( (nd->getHeight()-1.0e-3) >= rowHeight ) {
            // Use single moveable node...
            bptr = new Block;
            bptr->GetId() = count;

            bptr->GetOrigDim( HGraph ) = nd->getWidth();
            bptr->GetOrigDim( VGraph ) = nd->getHeight();
            bptr->GetOrigPos( HGraph ) = nd->getX();
            bptr->GetOrigPos( VGraph ) = nd->getY();

            bptr->GetDim( HGraph ) = nd->getWidth();
            bptr->GetDim( VGraph ) = nd->getHeight();
            bptr->GetPos( HGraph ) = nd->getX();
            bptr->GetPos( VGraph ) = nd->getY();

            bptr->SetFixed( false );

            blocks.push_back( bptr );

            ++count;
            ++other;

            std::cout << "Moveable node " << nd->getId() << " "
                 << "@ (" << nd->getX() << "," << nd->getY() << ")" << ", "
                 << "[" << nd->getWidth() << "," << nd->getHeight() << "]" << std::endl;

            nodeToBlockMap[nd] = bptr;
            continue;
        }
    }

    std::cout << "Fixed: " << fixed  << ", "
         << "Filler: " << filler << ", "
         << "Shapes: " << shape << ", "
         << "Moveable: " << other << ", "
         << "Total: " << count << std::endl;

    return other;
}

void MinMovementFloorplanner::initializeEpsilon()
{
    m_epsilon = std::numeric_limits<double>::max();
	for( int i = 0; i < m_blocks.size(); i++ ) {
        Block* tmp = m_blocks[i];

        m_epsilon = std::min( m_epsilon, tmp->GetDim( HGraph ) );
        m_epsilon = std::min( m_epsilon, tmp->GetDim( VGraph ) );
	}
	m_epsilon /= 1000.;
}

void MinMovementFloorplanner::initialize()
{
    // Setup some things; Can't use this routine for super large problems!
    initializeEpsilon();

	m_matrix[HGraph].resize( m_blocks.size() );
	m_matrix[VGraph].resize( m_blocks.size() );
	for( int i = 0; i < m_blocks.size(); i++ ) {
		m_matrix[HGraph][i].resize( m_blocks.size() );
		m_matrix[VGraph][i].resize( m_blocks.size() );
        std::fill( m_matrix[HGraph][i].begin(), m_matrix[HGraph][i].end(), false );
        std::fill( m_matrix[VGraph][i].begin(), m_matrix[VGraph][i].end(), false );
	}
	m_posInXSeqPair.resize( m_blocks.size() );
	m_posInYSeqPair.resize( m_blocks.size() );
	m_seqPair.Resize( m_blocks.size() );
}

void MinMovementFloorplanner::setNodePositions( std::map<Node*,Block*>& nodeToBlockMap )
{
    // This function sets the netlist's nodes to be the same as the blocks.
    for( std::map<Node*,Block*>::iterator it = nodeToBlockMap.begin();
            it != nodeToBlockMap.end();
            it++ ) 
    {
        Node* nd = it->first;
        Block* bptr = it->second;

        if( nd->getFixed() != NodeFixed_NOT_FIXED ) {
            continue;
        }
        nd->setX( bptr->GetPos(HGraph) );
        nd->setY( bptr->GetPos(VGraph) );
	}
}

void MinMovementFloorplanner::buildInitialSPComplex( void )
{
    // A fairly complex heuristic method for building an initial sequence pair from
    // a placement.  This method is much better at handling overlaps between cells,
    // and tests have shown that it results in far less initial cell movement than
    // other building strategies.  
    //
    // Works by building a constraint matrix and then making sure that there are no
    // cycles.

	for( int i = 0; i < m_blocks.size(); ++i ) {
		std::fill( m_matrix[HGraph][i].begin(), m_matrix[HGraph][i].end(), false );
		std::fill( m_matrix[VGraph][i].begin(), m_matrix[VGraph][i].end(), false );
	}

    // Setup the constraints.
    for( int i = 0; i < m_blocks.size(); i++ ) {
        Block* blk_i = m_blocks[i];

        for( int j = 0; j <= i; j++ ) {
            Block* blk_j = m_blocks[j];
            if( i == j ) {
				m_matrix[HGraph][i][j] = 1;
				m_matrix[VGraph][i][j] = 1;
                continue;
            }

            double iXStart = blk_i->GetPos( HGraph ) - 0.5 * blk_i->GetDim( HGraph );
            double iXEnd   = blk_i->GetPos( HGraph ) + 0.5 * blk_i->GetDim( HGraph );
            double jXStart = blk_j->GetPos( HGraph ) - 0.5 * blk_j->GetDim( HGraph );
            double jXEnd   = blk_j->GetPos( HGraph ) + 0.5 * blk_j->GetDim( HGraph );

            double iYStart = blk_i->GetPos( VGraph ) - 0.5 * blk_i->GetDim( VGraph );
            double iYEnd   = blk_i->GetPos( VGraph ) + 0.5 * blk_i->GetDim( VGraph );
            double jYStart = blk_j->GetPos( VGraph ) - 0.5 * blk_j->GetDim( VGraph );
            double jYEnd   = blk_j->GetPos( VGraph ) + 0.5 * blk_j->GetDim( VGraph );

			double horizOverlap = 0.0;
			double vertOverlap = 0.0;
			double vertOverlapDir = 0;
			double horizOverlapDir = 0;

			// Horizontal constraints
			if( jYStart < (iYStart + m_epsilon) && jYEnd < (iYEnd + m_epsilon) && jYEnd > (iYStart + m_epsilon) ) {
				vertOverlap = jYEnd - iYStart;	// Lower overlap
				vertOverlapDir = 0;
			} 
            else if( jYStart > (iYStart + m_epsilon) && jYEnd < (iYEnd + m_epsilon) ) {
				vertOverlap = jYEnd - jYStart;	// Inner overlap
				if( (iYEnd - jYEnd) > (jYStart - iYStart) ) {
					vertOverlapDir = 0;
				} else {
					vertOverlapDir = 1;
				}
			}
            else if( jYStart > (iYStart + m_epsilon) && jYStart < (iYEnd + m_epsilon) && jYEnd > (iYEnd + m_epsilon) ) {
				vertOverlap = iYEnd - jYStart;	// Upper overlap
				vertOverlapDir = 1;
			}
            else if( jYStart < (iYStart + m_epsilon) && jYEnd > (iYEnd + m_epsilon) ) {
				vertOverlap = iYEnd - iYStart;	// Outer overlap
				if( (jYEnd - iYEnd) > (iYStart - jYStart) ) {
					vertOverlapDir = 1;
				} else {
					vertOverlapDir = 0;
				}
			}
            else {
				m_matrix[HGraph][i][j] = 0;
			}

			// Vertical constraint
			if( jXStart < (iXStart + m_epsilon) && jXEnd < (iXEnd + m_epsilon) && jXEnd > (iXStart + m_epsilon) ) {
				horizOverlap = jXEnd - iXStart;	// Right overlap
				horizOverlapDir = 0;
			} 
            else if( jXStart > (iXStart + m_epsilon) && jXEnd < (iXEnd + m_epsilon) ) {
				horizOverlap = jXEnd - jXStart;	// Inner overlap
				if( (iXEnd - jXEnd) > (jXStart - iXStart) ) {
					horizOverlapDir = 0;
				} else {
					horizOverlapDir = 1;
				}
			} 
            else if( jXStart > (iXStart + m_epsilon) && jXStart < (iXEnd + m_epsilon) && jXEnd > (iXEnd + m_epsilon) ) {
				horizOverlap = iXEnd - jXStart;	// Left overlap
				horizOverlapDir = 1;
			}
            else if( jXStart < (iXStart + m_epsilon) && jXEnd > (iXEnd + m_epsilon) ) {
				horizOverlap = iXEnd - iXStart;	// Outer overlap
				if( jXEnd - iXEnd > iXStart - jXStart ) {
					horizOverlapDir = 1;
				} else {
					horizOverlapDir = 0;
				}
			} 
            else {
				m_matrix[VGraph][i][j] = 0;
			}

			if( vertOverlap > m_epsilon && horizOverlap <= m_epsilon ) {
				if( iXStart <= jXStart ) {
					m_matrix[HGraph][i][j] = 1;
				} 
                else {
					m_matrix[HGraph][j][i] = 1;
				}
			} 
            else if( horizOverlap > m_epsilon && vertOverlap <= m_epsilon ) {
				if( iYStart <= jYStart ) {
					m_matrix[VGraph][i][j] = 1;
				} 
                else {
					m_matrix[VGraph][j][i] = 1;
				}
			} 
            else if( horizOverlap > m_epsilon && vertOverlap > m_epsilon ) {
				// Overlapping case
				if( vertOverlap >= horizOverlap ) {
					if( horizOverlapDir == 1 ) {
						m_matrix[HGraph][i][j] = 1;
					} 
                    else {
						m_matrix[HGraph][j][i] = 1;
					}
				} 
                else {
					if( vertOverlapDir == 1 ) {
						m_matrix[VGraph][i][j] = 1;
					} 
                    else {
						m_matrix[VGraph][j][i] = 1;
					}
				}
			}
		}
	}

    // Compute the transitive closure of the graphs.
	transitiveClosure( m_matrix[HGraph] );
	transitiveClosure( m_matrix[VGraph] );

	// Find cycles and break them.   Cycles will appear if i->j and j->i in either the
    // horizontal or vertical graph due to the transitive closure computation.
	for( int i = 0; i < m_blocks.size(); i++ ) {
		Block* blk_i = m_blocks[i];

		for( int j = 0; j < i; ++j ) {
			Block* blk_j = m_blocks[j];

			int count = 0;
			if( m_matrix[HGraph][i][j] == 1 ) 	++count;
			if( m_matrix[HGraph][j][i] == 1 ) 	++count;
			if( m_matrix[VGraph][i][j] == 1 ) 	++count;
			if( m_matrix[VGraph][j][i] == 1 )	++count;

			if( count > 1 ) {
                // Either a cycle or a constraint in both graphs.  We cannot allow cycles and
                // we don't need constraints in both directions.  We try to be "smart" about 
                // this by breaking the more "difficult" (i.e., the larger) of the constraints.
				double distX = blk_i->GetDim(HGraph) + blk_j->GetDim(HGraph);
				double distY = blk_i->GetDim(VGraph) + blk_j->GetDim(VGraph);

				if( distX > (distY + m_epsilon) ) {
					m_matrix[HGraph][i][j] = 0;	// vert constraint
					m_matrix[HGraph][j][i] = 0;
				} 
                else if( distY > (distX + m_epsilon) ) {
					m_matrix[VGraph][i][j] = 0;	// horiz constraint
					m_matrix[VGraph][j][i] = 0;
				} 
                else {
					// Failing the largest dimension, we'll try to break the tie based on distance.
					distX = std::fabs( blk_i->GetPos(HGraph) - blk_j->GetPos(HGraph) );
					distY = std::fabs( blk_i->GetPos(VGraph) - blk_j->GetPos(VGraph) );

					if( distX > (distY + m_epsilon) ) {
						m_matrix[VGraph][i][j] = 0;	// horiz constraint
						m_matrix[VGraph][j][i] = 0;
					} 
                    else if( distY > (distX + m_epsilon) ) {
						m_matrix[HGraph][i][j] = 0;	// horiz constraint
						m_matrix[HGraph][j][i] = 0;
					} 
                    else {
						// Completely random.  (What else can we do?)
						if( (*m_rng)() % 2 == 0 ) {
							m_matrix[HGraph][i][j] = 0;	// vert constraint
							m_matrix[HGraph][j][i] = 0;
						} 
                        else {
							m_matrix[VGraph][i][j] = 0;	// horiz constraint
							m_matrix[VGraph][j][i] = 0;
						}
					}
				}
			}

			// Handle the case where there might be no constraint
			// between the blocks by artificially adding a
			// constraint.  This is done because *every* pair of
			// modules must have constraints between them.  
			if( m_matrix[HGraph][i][j] == 0 && m_matrix[HGraph][j][i] == 0 && 
				m_matrix[VGraph][i][j] == 0 && m_matrix[VGraph][j][i] == 0 ) {

				// We try to be "smart" about handling these
				// non-overlapping cells by placing the
				// constraint in the graph where it will do the
				// least harm.  This seems to work best by
				// creating the constraint in the FURTHEST
				// direction.
				double distX = std::fabs( blk_i->GetPos(HGraph) - blk_j->GetPos(HGraph) );
				double distY = std::fabs( blk_i->GetPos(VGraph) - blk_j->GetPos(VGraph) );

				if( distX > (distY + m_epsilon) ) {
					if( blk_i->GetPos( HGraph ) < blk_j->GetPos( HGraph ) ) {
						m_matrix[HGraph][i][j] = 1;
					} 
                    else {
						m_matrix[HGraph][j][i] = 1;
					}
				} else if( distY > (distX + m_epsilon) ) {
					if( blk_i->GetPos( VGraph ) < blk_j->GetPos( VGraph ) ) {
						m_matrix[VGraph][i][j] = 1;
					} 
                    else {
						m_matrix[VGraph][j][i] = 1;
					}
				} 
                else {
					// Failing the distance metric, let's
					// look at the sizes of the blocks and
					// try to insert in the direction with
					// the smallest dimensions.
					distX = blk_i->GetDim(HGraph) + blk_j->GetDim(HGraph);
					distY = blk_i->GetDim(VGraph) + blk_j->GetDim(VGraph);

					if( distX > (distY + m_epsilon) ) {
						if( blk_i->GetPos( VGraph ) < blk_j->GetPos( VGraph ) ) {
							m_matrix[VGraph][i][j] = 1;
						} else {
							m_matrix[VGraph][j][i] = 1;
						}
					} else if( distY > (distX + m_epsilon) ) {
						if( blk_i->GetPos( HGraph ) < blk_j->GetPos( HGraph ) ) {
							m_matrix[HGraph][i][j] = 1;
						} else {
							m_matrix[HGraph][j][i] = 1;
						}
					} else {
						// Completely random.  (What else can we do?)
						if( (*m_rng)() % 2 == 0 ) {
							if( blk_i->GetPos( VGraph ) < blk_j->GetPos( VGraph ) ) {
								m_matrix[VGraph][i][j] = 1;
							} else {
								m_matrix[VGraph][j][i] = 1;
							}
						} else {
							if( blk_i->GetPos( HGraph ) < blk_j->GetPos( HGraph ) ) {
								m_matrix[HGraph][i][j] = 1;
							} else {
								m_matrix[HGraph][j][i] = 1;
							}
						}
					}
				}
			}
		}
	}
	SortSeqPairUsingMatrix	sortX( &m_matrix[HGraph], &m_matrix[VGraph], true );
	SortSeqPairUsingMatrix	sortY( &m_matrix[HGraph], &m_matrix[VGraph], false );

    // Finally build the sequence pair.
	for( int i = 0; i < m_blocks.size(); ++i ) {
		m_seqPair.m_x[i] = i;
		m_seqPair.m_y[i] = i;
	}

	Utility::random_shuffle( m_seqPair.m_x.begin(), m_seqPair.m_x.end(), m_rng );
	Utility::random_shuffle( m_seqPair.m_y.begin(), m_seqPair.m_y.end(), m_rng );

	std::stable_sort( m_seqPair.m_x.begin(), m_seqPair.m_x.end(), sortX );
	std::stable_sort( m_seqPair.m_y.begin(), m_seqPair.m_y.end(), sortY );

	for( int i = 0; i < m_blocks.size(); ++i ) {
        m_posInXSeqPair[m_seqPair.m_x[i]] = i;
        m_posInYSeqPair[m_seqPair.m_y[i]] = i;
	}
}

void MinMovementFloorplanner::buildInitialSPSimple()
{
    // A simple method for building an initial sequence pair from a placement.

	std::vector<double>	posX( m_blocks.size() );
	std::vector<double>	posY( m_blocks.size() );

	for( int i = 0; i < m_blocks.size(); ++i ) {
		m_seqPair.m_x[i] = i;
		m_seqPair.m_y[i] = i;

        posX[i] = m_blocks[i]->GetPos( HGraph );
        posX[i] = m_blocks[i]->GetPos( VGraph );
	}
    SortSeqPairUsingPositions	posSorterX( &posX, &posY, true  );
    SortSeqPairUsingPositions	posSorterY( &posX, &posY, false );

	std::stable_sort( m_seqPair.m_x.begin(), m_seqPair.m_x.end(), posSorterX );
	std::stable_sort( m_seqPair.m_y.begin(), m_seqPair.m_y.end(), posSorterY );

    for( int i = 0; i < m_blocks.size(); i++ ) {
        m_posInXSeqPair[m_seqPair.m_x[i]] = i;
        m_posInYSeqPair[m_seqPair.m_y[i]] = i;
	}
}

void MinMovementFloorplanner::buildTCGfromSP( void )
{
    // Given a SP, construct a topological constraint graph (TCG).
    m_graph[HGraph]->Resize( m_blocks.size() );
    m_graph[VGraph]->Resize( m_blocks.size() );

    // 1) If two elements appear in order a..b in X and order a..b in Y,
    // then there is a horizontal arc (in H graph), going from "left to
    // right", from a to b.
    //
    // 2) Else, if two elements appear in order a..b in X and order b..a in
    // Y, then there is a vertical arc (in V graph), going from "bottom to
    // top" from b to a.
    for( int i = 0; i < m_blocks.size(); ++i ) { 
        for( int j = 0; j < i; ++j ) { 
            if     ( m_posInXSeqPair[i] < m_posInXSeqPair[j] && m_posInYSeqPair[i] < m_posInYSeqPair[j] ) { 
                m_graph[HGraph]->AddEdge( i, j );
			} 
            else if( m_posInXSeqPair[i] > m_posInXSeqPair[j] && m_posInYSeqPair[i] > m_posInYSeqPair[j] ) {
				m_graph[HGraph]->AddEdge( j, i );
			} 
            else if( m_posInXSeqPair[i] < m_posInXSeqPair[j] && m_posInYSeqPair[i] > m_posInYSeqPair[j] ) {
				m_graph[VGraph]->AddEdge( j, i );
			} 
            else if( m_posInXSeqPair[i] > m_posInXSeqPair[j] && m_posInYSeqPair[i] < m_posInYSeqPair[j] ) {
				m_graph[VGraph]->AddEdge( i, j );
			} 
            else 
            {
			}
		}
	}
}

void MinMovementFloorplanner::computeSlack( WhichGraph wg )
{
    FPGraph& gr = *m_graph[wg];
	gr.TopologicalSort( m_order );

    m_worstSlack[wg] = std::numeric_limits<double>::max();
    m_maxTarr[wg] = -std::numeric_limits<double>::max();
    m_minTreq[wg] = std::numeric_limits<double>::max();

    // Initialization...
    for( int i = 0; i < m_blocks.size(); i++ ) {
        Block* srcblk = m_blocks[i];
        Slack* srcslk = srcblk->GetSlack( wg );

		// Initialize blocks to the minimum value.  Note that fixed
		// cells act like POs -- their arrival times are set.
		if( ( ( wg == HGraph ) && ( srcblk->GetFixed() ) ) ||
			( ( wg == VGraph ) && ( srcblk->GetFixed() ) ) ) {

			srcslk->GetTarr() = srcblk->GetPos( wg ) - 0.5 * srcblk->GetDim( wg );
			srcslk->GetTreq() = srcblk->GetPos( wg ) - 0.5 * srcblk->GetDim( wg );
		} 
        else {
			srcslk->GetTarr() = m_Tmin[wg];
			srcslk->GetTreq() = m_Tmax[wg] - srcblk->GetDim( wg );
        }
    }

	// Forward traversal...
    for( int i = 0; i < m_order.size(); ++i ) {
        int idx = m_order[i];

        Block* srcblk = m_blocks[idx];
        Slack* srcslk = srcblk->GetSlack( wg );

        for( int j = 0; j < gr.GetNumOutEdges( idx ); j++ ) {
            int tgt = gr.GetOutEdgeDest( idx, j );

            Block* tgtblk = m_blocks[tgt];
			Slack* tgtslk = tgtblk->GetSlack( wg );

			// The arrival time at a fixed block doesn't change.
			if( ( ( wg == HGraph ) && ( tgtblk->GetFixed() ) ) ||
				( ( wg == VGraph ) && ( tgtblk->GetFixed() ) ) ) {
				continue;
			}
            // Placement starts at least here...
            tgtslk->GetTarr() = std::max( tgtslk->GetTarr(), srcslk->GetTarr() + srcblk->GetDim( wg ) );
        }
		m_maxTarr[wg] = std::max( m_maxTarr[wg], srcslk->GetTarr() );
    }
	m_maxTarr[wg] = std::max( m_maxTarr[wg] , m_Tmax[wg] );

	// Reverse traversal...
	for( int j = m_order.size() - 1; j >= 0; j-- ) {
		int idx = m_order[j];

		Block* srcblk = m_blocks[idx];
		Slack* srcslk = srcblk->GetSlack( wg );

		for( int i = 0; i < gr.GetNumInEdges( idx ); ++i ) {
			int tgt = gr.GetInEdgeSrc( idx, i );

			Block* tgtblk = m_blocks[tgt];
			Slack* tgtslk = tgtblk->GetSlack( wg );

			// The required time at a fixed block doesn't change.
			if( ( ( wg == HGraph ) && ( tgtblk->GetFixed() ) ) ||
				( ( wg == VGraph ) && ( tgtblk->GetFixed() ) ) ) {
				continue;
			}
            // Placement ends at least here...
			tgtslk->GetTreq() = std::min( tgtslk->GetTreq(), srcslk->GetTreq() - tgtblk->GetDim( wg ) );
		}
		m_minTreq[wg] = std::min( m_minTreq[wg], srcslk->GetTreq() );

		// Here, we compute the "worst slack" in the design.  This is
		// akin to performing a longest-path computation (because all
		// nodes on the longest path have the same [worst case] slack).  
		m_worstSlack[wg] = std::min( m_worstSlack[wg], srcslk->ComputeSlack() );
	}
	// Note that the minTreq should be either that computed, or the min dimension
	// of the chip (if smaller).
	m_minTreq[wg] = std::min( m_minTreq[wg], m_Tmin[wg] );
}

bool MinMovementFloorplanner::shiftFast( double& maxMovedX, double& amtMovedX )
{
    std::cout << "Fast removal of overlap." << std::endl;

    maxMovedX = 0.;
    amtMovedX = 0.;

    // XXX: To align with sites, I need to look at the rows.  I'm going to
    // get this information from the first row.  Not sure this is entirely
    // correct.
    double originX      = m_arch->m_rows[0]->m_subRowOrigin;
    double siteSpacing  = m_arch->m_rows[0]->m_siteSpacing;
    double rowHeight    = m_arch->m_rows[0]->m_rowHeight;

//    std::cout << "Origin is " << originX << ", Spacing is " << siteSpacing << std::endl;

    FPGraph& gr = *m_graph[HGraph];
    computeSlack( HGraph );

	// Forward traversal...
    for( int i = 0; i < m_order.size(); ++i ) 
    {
        int idx = m_order[i];

        Block* srcblk = m_blocks[idx];
        Slack* srcslk = srcblk->GetSlack( HGraph );
        if( !srcblk->GetFixed() )
        {
            double Tarr = srcslk->GetTarr();
            double Treq = srcslk->GetTreq();

            // Get the left edge of the block and make sure it is within range.
            double origX = srcblk->GetPos( HGraph );
            double pos = srcblk->GetPos( HGraph ) - 0.5 * srcblk->GetDim( HGraph );
            double llx = std::max( Tarr, std::min( Treq, pos ) );
            // Get the aligned spot and shift the cell.
            int ix = (int)((llx-originX)/siteSpacing + 0.5);
            double aligned = originX + ix * siteSpacing;
            if( std::fabs( aligned - llx ) > 1.0e-3 )
            {
                // Not site aligned.  
                if( aligned > Tarr-1.0e-3 && aligned < Treq+1.0e-3 )
                {
                    // Aligned site valid for cell.  But, consider shifting left so we know
                    // we will not add overlap on the right.
                    if( aligned > pos && originX + (ix-1)*siteSpacing > Tarr-1.0e-3 )
                    {
                        aligned = originX + (ix-1)*siteSpacing;
                    }
                    llx = aligned;
                }
            }
            srcblk->GetPos( HGraph ) = llx + 0.5 * srcblk->GetDim( HGraph );
            double dist = std::fabs( srcblk->GetPos( HGraph ) - origX );
            maxMovedX = std::max( maxMovedX, dist );
            amtMovedX += dist;
        }

        for( int j = 0; j < gr.GetNumOutEdges( idx ); j++ ) 
        {
            int tgt = gr.GetOutEdgeDest( idx, j );

            Block* tgtblk = m_blocks[tgt];
			Slack* tgtslk = tgtblk->GetSlack( HGraph );

			if( tgtblk->GetFixed() )
            {
                continue;
			}
            // Placement starts at least here...
            double lrx = srcblk->GetPos( HGraph ) + 0.5 * srcblk->GetDim( HGraph );
            tgtslk->GetTarr() = std::max( tgtslk->GetTarr(), lrx );
        }
    }

    return true;
}

void MinMovementFloorplanner::alignToSites( void )
{
    std::cout << "Aligning blocks to sites." << std::endl;

    // XXX: To align with sites, I need to look at the rows.  I'm going to
    // get this information from the first row.  Not sure this is entirely
    // correct.
    double originX      = m_arch->m_rows[0]->m_subRowOrigin;
    double siteSpacing  = m_arch->m_rows[0]->m_siteSpacing;
    double rowHeight    = m_arch->m_rows[0]->m_rowHeight;

//    std::cout << "Origin is " << originX << ", Spacing is " << siteSpacing << std::endl;

    FPGraph& gr = *m_graph[HGraph];
    computeSlack( HGraph );

	// Forward traversal...
    for( int i = 0; i < m_order.size(); ++i ) 
    {
        int idx = m_order[i];

        Block* srcblk = m_blocks[idx];
        Slack* srcslk = srcblk->GetSlack( HGraph );
        if( !srcblk->GetFixed() )
        {
            double Tarr = srcslk->GetTarr();
            double Treq = srcslk->GetTreq();

            // Get the left edge of the block and make sure it is within range.
            double pos = srcblk->GetPos( HGraph ) - 0.5 * srcblk->GetDim( HGraph );
            double llx = std::max( Tarr, std::min( Treq, pos ) );
            // Get the aligned spot and shift the cell.
            int ix = (int)((llx-originX)/siteSpacing + 0.5);
            double aligned = originX + ix * siteSpacing;
            if( std::fabs( aligned - llx ) > 1.0e-3 )
            {
                // Not site aligned.  
                if( aligned > Tarr-1.0e-3 && aligned < Treq+1.0e-3 )
                {
                    // Aligned site valid for cell.  But, consider shifting left so we know
                    // we will not add overlap on the right.
                    if( aligned > pos && originX + (ix-1)*siteSpacing > Tarr-1.0e-3 )
                    {
                        aligned = originX + (ix-1)*siteSpacing;
                    }
                    llx = aligned;
                }
            }
            srcblk->GetPos( HGraph ) = llx + 0.5 * srcblk->GetDim( HGraph );
        }

        for( int j = 0; j < gr.GetNumOutEdges( idx ); j++ ) 
        {
            int tgt = gr.GetOutEdgeDest( idx, j );

            Block* tgtblk = m_blocks[tgt];
			Slack* tgtslk = tgtblk->GetSlack( HGraph );

			if( tgtblk->GetFixed() )
            {
                continue;
			}
            // Placement starts at least here...
            tgtslk->GetTarr() = std::max( tgtslk->GetTarr(), srcblk->GetPos( HGraph ) + 0.5 * srcblk->GetDim( HGraph ) );
        }
    }
}

bool MinMovementFloorplanner::legalizeTCG( void )
{
    // Attempts to make the sequence pair feasible by eliminating critical paths.  The 
    // routine quits once it has found something legal (i.e., something that fits).

    int maxPass = ( m_params._fastMode ? 1 : 3 );

    Utility::Timer tm;

	m_order.reserve( m_blocks.size() );

	m_pathCountB[HGraph].resize( m_blocks.size() );
	m_pathCountB[VGraph].resize( m_blocks.size() );
	m_pathCountF[HGraph].resize( m_blocks.size() );
	m_pathCountF[VGraph].resize( m_blocks.size() );

    bool legal = false;
	for( int i = 0; i < maxPass; i++ ) {
        tm.start();
		legal = adjustEdgesByMovingOrReversing();
        tm.stop();
        m_timeMovingOrReversing += tm.usertime();
		if( legal ) {
            break;
        }

        tm.start();
		legal = adjustEdgesByInserting();
        tm.stop();
        m_timeInserting += tm.usertime();
		if( legal ) {
            break;
        }
	}
	return legal;
}

bool MinMovementFloorplanner::adjustEdgesByInserting( void )
{
    // Attempts moves in the sequence pair to make cells fit.  This operation is
    // equivalent to the "move" operation defined in Nag/Chaudhary, but has no
    // equivalent in the TCG papers.  The basic idea is to identify arcs that have
    // sufficient slack that a candidate cell could be moved into it, then actually
    // proceed to move said cell into the arc and test the effect on the placement.
    // This technique isn't usually as good as "adjustEdgesByMovingOrReversing",
    // but it can help.
	std::vector<Arc>	arcs;
	Block			*blkO;
	unsigned			numTotal, numAccept = 0, numReject = 0;
    char buf[1024];
    (void) buf;

	computeSlack( HGraph );
	computeSlack( VGraph );

    // For backing up the sequence pair is case of a rejection.
    std::vector<unsigned> backupX;
    std::vector<unsigned> backupY;
	std::vector<int> backupPosInXSeqPair;
    std::vector<int> backupPosInYSeqPair;
    // For backing up slacks in case of a rejection.
	std::vector<Slack> backupSlacksX( m_blocks.size() );
	std::vector<Slack> backupSlacksY( m_blocks.size() );

    // For costing benefit of a change.
    double worstSlackAft[2];
    double worstSlackBef[2];

    WhichGraph wg = HGraph;
    WhichGraph tgtwg;
    int stuck = 0;
    int maxInnerPass;
    int maxOuterPass = std::max( 20, std::min( (int)m_blocks.size(), 200 ) );
	for( int passOuter = 0; passOuter < maxOuterPass; ++passOuter ) {
		if( m_worstSlack[HGraph] >= 0. && m_worstSlack[VGraph] >= 0. ) {
            // Done...  Things fit.
			goto cleanup;
		}

		if( stuck > ( m_params._fastMode ? 1 : 3 ) ) {
			break;
		}

		// Choose a critical direction.
		if( m_worstSlack[HGraph] < 0. && m_worstSlack[VGraph] < 0. ) {
			wg = ( passOuter % 2 == 0 ) ? HGraph : VGraph;
		} else if( m_worstSlack[HGraph] < 0. ) { 
			wg = HGraph;
		} else if( m_worstSlack[VGraph] < 0. ) { 
			wg = VGraph;
		} else {
            assert( 0 );
			wg = ( passOuter % 2 == 0 ) ? HGraph : VGraph;
		}
		tgtwg = ( wg == HGraph ) ? VGraph : HGraph;

		// Find the best block to move.
		pathCounting( wg );
		blkO = getCandidateBlock( wg );
		if( blkO == NULL ) {
			++stuck;
			continue;
		}
		pathCounting( tgtwg );	// Necessary for the arc gathering.

		// NB: When we gather arcs, they must be reduction edges (not
		// transitive arcs), so we need to compute a transitive
		// reduction here.
        m_graph[HGraph]->TransitiveReduction();
        m_graph[VGraph]->TransitiveReduction();

		arcs.clear();
		arcs.reserve( m_graph[HGraph]->GetNumEdges() + m_graph[VGraph]->GetNumEdges() );

		// Find appealing candidate arcs in *both* directions -- doing
		// it in both seems to help the technique remove overlap more
		// quickly than if it had just used the current direction.
		getCandidateArcs( HGraph, blkO, arcs );
		getCandidateArcs( VGraph, blkO, arcs );

		if( arcs.empty() ) {
			++stuck;
			continue;
		}
		Utility::random_shuffle( arcs.begin(), arcs.end(), m_rng );
		std::sort( arcs.begin(), arcs.end(), SortArcs() );

		// Try changing the SP to get something that hopefully will
		// fit within the placement region better.
		if( m_params._fastMode ) {
			maxInnerPass = std::min( (int) arcs.size(), (int)(m_blocks.size() / 6) );
		} else {
			maxInnerPass = std::min( (int) arcs.size(), (int)(m_blocks.size() / 3) );
		}

		// We save our backup ("golden") copies of the graphs and the
		// sequence pairs here.  This saves us a LOT of time when we
		// reject moves, because we don't need to go through the
		// process of recomputing things -- we basically just restore
		// the previous state from backup.
		backupX = m_seqPair.m_x;
		backupY = m_seqPair.m_y;
		backupPosInXSeqPair = m_posInXSeqPair;
		backupPosInYSeqPair = m_posInYSeqPair;

		m_backupGraph[HGraph]->Copy( *m_graph[HGraph] );
		m_backupGraph[VGraph]->Copy( *m_graph[VGraph] );
		for( int i = 0; i < m_blocks.size(); i++ ) {
			backupSlacksX[i] = *(m_blocks[i]->GetSlack(HGraph));
			backupSlacksY[i] = *(m_blocks[i]->GetSlack(VGraph));
		}
		worstSlackBef[HGraph] = m_worstSlack[HGraph];
		worstSlackBef[VGraph] = m_worstSlack[VGraph];

		// Main loop ...
		for( int passInner = 0; passInner < maxInnerPass; ++passInner ) {
			doSPInsertion( blkO, arcs, passInner );
			buildTCGfromSP();

			computeSlack( HGraph );
			computeSlack( VGraph );
			worstSlackAft[HGraph] = m_worstSlack[HGraph];
			worstSlackAft[VGraph] = m_worstSlack[VGraph];

			// Compute cost of a move.  Basically, the change in slack.
			double metric = ( worstSlackAft[HGraph] < 0. ? worstSlackAft[HGraph] : 0. ) -
				 ( worstSlackBef[HGraph] < 0. ? worstSlackBef[HGraph] : 0. ) +
				 ( worstSlackAft[VGraph] < 0. ? worstSlackAft[VGraph] : 0. ) -
				 ( worstSlackBef[VGraph] < 0. ? worstSlackBef[VGraph] : 0. );

			if( metric <= 0. ) {
                // Things did not improve so restore the previous solution.
				m_seqPair.m_x = backupX;
				m_seqPair.m_y = backupY;
				m_posInXSeqPair = backupPosInXSeqPair;
				m_posInYSeqPair = backupPosInYSeqPair;
				m_graph[HGraph]->Copy( *(m_backupGraph[HGraph]) );
				m_graph[VGraph]->Copy( *(m_backupGraph[VGraph]) );
				for( int i = 0; i < m_blocks.size(); ++i ) {
					*(m_blocks[i]->GetSlack(HGraph)) = backupSlacksX[i];
					*(m_blocks[i]->GetSlack(VGraph)) = backupSlacksY[i];
				}
				m_worstSlack[HGraph] = worstSlackBef[HGraph];
				m_worstSlack[VGraph] = worstSlackBef[VGraph];
				++numReject;
			} 
            else {
				// Things did improve so make a new backup.
				backupX = m_seqPair.m_x;
				backupY = m_seqPair.m_y;
				backupPosInXSeqPair = m_posInXSeqPair;
				backupPosInYSeqPair = m_posInYSeqPair;
				m_backupGraph[HGraph]->Copy( *m_graph[HGraph] );
				m_backupGraph[VGraph]->Copy( *m_graph[VGraph] );
				for( int i = 0; i < m_blocks.size(); ++i ) {
					backupSlacksX[i] = *(m_blocks[i]->GetSlack(HGraph));
					backupSlacksY[i] = *(m_blocks[i]->GetSlack(VGraph));
				}
				worstSlackBef[HGraph] = m_worstSlack[HGraph];
				worstSlackBef[VGraph] = m_worstSlack[VGraph];
				stuck = 0;
				++numAccept;
				goto exitLoop;
			}
		}
		++stuck;
exitLoop:
		;
	}
cleanup:
	numTotal = numAccept + numReject;
    std::cout << "Inserting, accepts is " << numAccept << ", rejects is " << numReject << ", total is " << numTotal << std::endl;
    std::cout << "Horizontal slack is " << m_worstSlack[HGraph] << ", Vertical slack is " << m_worstSlack[VGraph] 
         << std::endl;
	return( m_worstSlack[HGraph] >= 0. && m_worstSlack[VGraph] >= 0. );
}

bool MinMovementFloorplanner::adjustEdgesByMovingOrReversing( void )
{
    // Adjusts the critical edges in the graph for the specified direction.  This
    // function implements the "Swap" in Nag/Chaudhary, a "move & reverse" (in TCG
    // parlance), and an "adjustment" in Cong's 2006 ASP-DAC paper.

	std::vector<Slack> backupSlacksX( m_blocks.size() );
	std::vector<Slack> backupSlacksY( m_blocks.size() );
	std::vector<int> backupPosInXSeqPair;
	std::vector<int> backupPosInYSeqPair;
	std::vector<unsigned> backupX;
	std::vector<unsigned> backupY;
	std::vector<EdgeSet>		edgeSets;
	double				metric;
	unsigned				numAccept = 0, numReject = 0, numTotal;
	unsigned                		passInner;
	unsigned				stuck = 0;
	double				worstSlackAfter[2];
	double				worstSlackBefore[2];
	WhichGraph			wg = HGraph;
    char buf[1024];

	computeSlack( HGraph );
	computeSlack( VGraph );

    int maxInnerPass = 0;
	int maxOuterPass = std::max( 20, std::min( (int)m_blocks.size(), 200 ) );
	for( int passOuter = 0; passOuter < maxOuterPass; ++passOuter ) {
		if( m_worstSlack[HGraph] >= 0. && m_worstSlack[VGraph] >= 0. ) {
			goto cleanup;
		}

		if( stuck > ( m_params._fastMode ? 2 : 3 ) ) {
			break;
		}

		// Choose a critical direction.
		if( m_worstSlack[HGraph] < 0. && m_worstSlack[VGraph] < 0. ) {
			wg = ( passOuter % 2 == 0 ) ? HGraph : VGraph;
		} else if( m_worstSlack[HGraph] < 0. ) { 
			wg = HGraph;
		} else if( m_worstSlack[VGraph] < 0. ) { 
			wg = VGraph;
		} else {
		}

		extractSubgraph( wg );
		determineEdgeSet( wg, edgeSets );

		if( edgeSets.empty() ) {
			++stuck;
			continue;
		}

		sprintf( &buf[0], "%d - Move/Reverse: Chosen direction: %s, worst slack H: %.2f, V: %.2f", 
				passOuter, wg == HGraph ? "(H)" : "(V)", 
				m_worstSlack[HGraph], m_worstSlack[VGraph] ); 

		if( m_params._fastMode ) { 
			maxInnerPass = std::min( (int) edgeSets.size(), (int)(m_blocks.size() / 4) );
		} else {
			maxInnerPass = std::min( (int) edgeSets.size(), (int) m_blocks.size() );
		}

		// We save our backup ("golden") copies of the graphs and the
		// sequence pairs here.  This saves us a LOT of time when we
		// reject moves, because we don't need to go through the
		// process of recomputing things -- we basically just restore
		// the previous state from backup.
		backupX = m_seqPair.m_x;
		backupY = m_seqPair.m_y;
		backupPosInXSeqPair = m_posInXSeqPair;
		backupPosInYSeqPair = m_posInYSeqPair;
		m_backupGraph[HGraph]->Copy( *m_graph[HGraph] );
		m_backupGraph[VGraph]->Copy( *m_graph[VGraph] );
		for( int i = 0; i < m_blocks.size(); ++i ) {
			backupSlacksX[i] = *(m_blocks[i]->GetSlack(HGraph));
			backupSlacksY[i] = *(m_blocks[i]->GetSlack(VGraph));
		}
		worstSlackBefore[HGraph] = m_worstSlack[HGraph];
		worstSlackBefore[VGraph] = m_worstSlack[VGraph];

		// Main loop ...
		for( passInner = 0; passInner < maxInnerPass; ++passInner ) {
			doSPMoveOrReversal( wg, edgeSets[passInner], (stuck % 2) == 0 );
			buildTCGfromSP();

			// Compute worst slacks ...
			computeSlack( HGraph );
			computeSlack( VGraph );
			worstSlackAfter[HGraph] = m_worstSlack[HGraph];
			worstSlackAfter[VGraph] = m_worstSlack[VGraph];

			// Compute a metric for the move.  We're interested in
			// improving the *net* change in slack, so we can make one
			// direction slightly worse, but the other will have to get
			// better to compensate.
			metric = ( worstSlackAfter[HGraph] < 0. ? worstSlackAfter[HGraph] : 0. ) -
				 ( worstSlackBefore[HGraph] < 0. ? worstSlackBefore[HGraph] : 0. ) +
				 ( worstSlackAfter[VGraph] < 0. ? worstSlackAfter[VGraph] : 0. ) -
				 ( worstSlackBefore[VGraph] < 0. ? worstSlackBefore[VGraph] : 0. );


			if( metric <= 0. ) {
				// The slacks got modified during the
				// computation, but since we're not keeping
				// them, we have to restore them from backup.
				m_seqPair.m_x = backupX;
				m_seqPair.m_y = backupY;
				m_posInXSeqPair = backupPosInXSeqPair;
				m_posInYSeqPair = backupPosInYSeqPair;
				m_graph[HGraph]->Copy( *(m_backupGraph[HGraph]) );
				m_graph[VGraph]->Copy( *(m_backupGraph[VGraph]) );
				for( int i = 0; i < m_blocks.size(); ++i ) {
					*(m_blocks[i]->GetSlack(HGraph)) = backupSlacksX[i];
					*(m_blocks[i]->GetSlack(VGraph)) = backupSlacksY[i];
				}
				m_worstSlack[HGraph] = worstSlackBefore[HGraph];
				m_worstSlack[VGraph] = worstSlackBefore[VGraph];
				++numReject;
			} else {
				// We have to update our backup copies now.
				backupX = m_seqPair.m_x;
				backupY = m_seqPair.m_y;
				backupPosInXSeqPair = m_posInXSeqPair;
				backupPosInYSeqPair = m_posInYSeqPair;
				m_backupGraph[HGraph]->Copy( *m_graph[HGraph] );
				m_backupGraph[VGraph]->Copy( *m_graph[VGraph] );
				for( int i = 0; i < m_blocks.size(); ++i ) {
					backupSlacksX[i] = *(m_blocks[i]->GetSlack(HGraph));
					backupSlacksY[i] = *(m_blocks[i]->GetSlack(VGraph));
				}
				worstSlackBefore[HGraph] = m_worstSlack[HGraph];
				worstSlackBefore[VGraph] = m_worstSlack[VGraph];
				stuck = 0;
				++numAccept;
				goto exitLoop;
			}
		}
		++stuck;
exitLoop:
		;
	}
cleanup:
	numTotal = numAccept + numReject;
    std::cout << "Adjusting, accepts is " << numAccept << ", rejects is " << numReject << ", total is " << numTotal << std::endl;
    std::cout << "Horizontal slack is " << m_worstSlack[HGraph] << ", Vertical slack is " << m_worstSlack[VGraph] 
         << std::endl;
	return( m_worstSlack[HGraph] >= 0. && m_worstSlack[VGraph] >= 0. );
}

void MinMovementFloorplanner::determineEdgeSet( WhichGraph wg,
	       std::vector<EdgeSet> &edgeSets )
{
    // This function works on the subgraph (i.e., the reduction edges) to determines a set of 
    // edges that are worthwhile to swap in the original (closure) graph.  
	std::map<double, std::list<unsigned> >	arrivalBin;
	unsigned					i, j;
	std::map<double, std::list<unsigned> >::iterator	ite;
	std::list<unsigned>::iterator		ite2;
	double					maxCrit;
	double					pcSlack, pcDiscount, pcCrit;
	std::vector<Block *>			&reverseMapping = _subgraphReverseMapping[wg];
	unsigned					src, dst;
	Block					*srcblk, *tgtblk;
	Slack					*srcslk, *tgtslk;
	FPGraph					&srcSubgraph = *_subgraph[wg];
	std::vector<Block *>			&srcSubgraphMapping = _subgraphReverseMapping[wg];
	EdgeSet					tmpEdgeSet;
	EdgePair				tmpEdgePair;

	edgeSets.clear();

	tmpEdgeSet._crit = 0.;

	// Loop through the subgraph.  "Bin" the nodes based on their arrival
	// times.
	for( i = 0; i < srcSubgraph.GetNumNodes(); ++i ) {
		srcblk = reverseMapping[i];
		srcslk = srcblk->GetSlack( wg );
		arrivalBin[srcslk->GetTarr()].push_back( i );
	}

	// For every arrival time, loop through the nodes, and find their input
	// (fanin) edges.  Add these edges to an edge set (they must all be
	// cut, at once).
	for( ite = arrivalBin.begin(); ite != arrivalBin.end(); ++ite ) {
		tmpEdgeSet._pairs.clear();
		for( ite2 = ite->second.begin(); ite2 != ite->second.end(); ++ite2 ) {
			dst = *ite2;
			tgtblk = reverseMapping[dst];
			tmpEdgePair._dst = tgtblk->GetId();
			for( i = 0; i < srcSubgraph.GetNumInEdges( dst ); ++i ) {
				src = srcSubgraph.GetInEdgeSrc( dst, i );
				srcblk = reverseMapping[src];

				// We should not have edges between two fixed cells.
				// (That's taken care of in the graph extraction.)
				if( ( ( wg == HGraph ) && 
					( srcblk->GetFixed() ) &&
					( tgtblk->GetFixed() ) )

					||
						
					( ( wg == VGraph ) && 
					( srcblk->GetFixed() ) &&
					( tgtblk->GetFixed() ) ) ) {

				}
				tmpEdgePair._src = srcblk->GetId();
				tmpEdgeSet._pairs.push_back( tmpEdgePair );
			}
		}

		if( !tmpEdgeSet._pairs.empty() ) {
			edgeSets.push_back( tmpEdgeSet );
		}
	}

	// We now have a collection of edge sets for all critical paths.  We
	// must cost these edge sets.  We begin by counting the paths in the
	// graph/subgraph.
	pathCounting( wg );

	_edgeCrit.clear();

	// Walk through vertices in the subgraph, and find critical arcs.
	// Here, we try to organize edges in such a fashion as to encourage
	// finding a good candidate swap early on.
	// 
	// NB:
	// 1) Swapping {A,B} in H creates a new arc {A,B} in V 
	// 2) Swapping {A,B} in V creates a new arc {B,A} in H
	for( i = 0; i < srcSubgraph.GetNumNodes(); ++i ) {
		srcblk = srcSubgraphMapping[i];
		srcslk = srcblk->GetSlack( wg );

		tmpEdgePair._src = srcblk->GetId();

		for( j = 0; j < srcSubgraph.GetNumOutEdges( i ); ++j ) {
			tgtblk = srcSubgraphMapping[ srcSubgraph.GetOutEdgeDest( i, j ) ];

			// We should not have edges between two fixed cells.
			// (That's taken care of in the graph extraction.)
			if( ( ( wg == HGraph ) && 
				( srcblk->GetFixed() ) &&
				( tgtblk->GetFixed() ) ) 
					
				||
					
				( ( wg == VGraph ) && 
				( srcblk->GetFixed() ) &&
				( tgtblk->GetFixed() ) ) ) {

			}
			tgtslk = tgtblk->GetSlack( wg );
			pcSlack = tgtslk->GetTreq() - srcslk->GetTarr() - srcblk->GetDim( wg );
			pcDiscount = DISCOUNT( pcSlack, m_maxTarr[wg] );
			pcCrit = m_pathCountF[wg][srcblk->GetId()] * m_pathCountB[wg][tgtblk->GetId()] * pcDiscount;

			tmpEdgePair._dst = tgtblk->GetId();
			_edgeCrit[tmpEdgePair] = pcCrit;
		}
	}

	// Annotate the edge sets with the criticalities that we just computed
	// NB: Searching in a map that doesn't already have an entry is defined
	// to return 0.  (This is exactly what we want, given that edges between
	// critical overlapping fixed obstacles should not be counted in the
	// first place.)
	for( i = 0; i < edgeSets.size(); ++i ) {
		maxCrit = -INFINITY;

		for( j = 0; j < edgeSets[i]._pairs.size(); ++j ) {
			srcblk = m_blocks[ edgeSets[i]._pairs[j]._src ];
			tgtblk = m_blocks[ edgeSets[i]._pairs[j]._dst ];

			tmpEdgePair._src = edgeSets[i]._pairs[j]._src;
			tmpEdgePair._dst = edgeSets[i]._pairs[j]._dst;
			maxCrit = std::max( maxCrit, _edgeCrit[tmpEdgePair] );
		}
		edgeSets[i]._crit = maxCrit;
	}
	Utility::random_shuffle( edgeSets.begin(), edgeSets.end(), m_rng );
	std::sort( edgeSets.begin(), edgeSets.end(), SortEdgeSet() );
}

bool MinMovementFloorplanner::doSPInsertion( Block *blkO, std::vector<Arc> &arcs, unsigned pass )
{
    // Pick an arc from among the best.  Then, move blkO between blkA and blkB in
    // both the Pset and the Nset, as per Section 3.2.2 of Nag & Chaudhary.

	Block					*blkA, *blkB;
        unsigned                                  i;
        std::vector<unsigned>::iterator           ite;
        std::deque<Node*>                       neighbours;
	unsigned					newLocForO = 0;
	unsigned					posBlkO, posBlkA, posBlkB;
	Arc					tmpArc;


	tmpArc = arcs[ pass ];
	blkA = m_blocks[ tmpArc._ep._src ];
	blkB = m_blocks[ tmpArc._ep._dst ];

	// Now move the cell in X.....
	posBlkO = m_posInXSeqPair[blkO->GetId()];
	posBlkA = m_posInXSeqPair[blkA->GetId()];
	posBlkB = m_posInXSeqPair[blkB->GetId()];

	ite = m_seqPair.m_x.begin() + posBlkO;
	m_seqPair.m_x.erase( ite );

	// Account for the erasure of blkO by modifying 'newLoc'.
	// NB: insert() occurs *before* the specified value, so we go +1 to go to its right.
	if( posBlkO < std::min( posBlkA, posBlkB ) ) {
		// NB: We go -1 to account for the erasure of posBlkO.
		newLocForO = std::min( posBlkA, posBlkB ) + 1 - 1 + 
			(unsigned)( (*m_rng)() ) % ( std::max( posBlkA, posBlkB ) - std::min( posBlkA, posBlkB ) );
	} else if( posBlkO > std::max( posBlkA, posBlkB ) ) {
		newLocForO = std::min( posBlkA, posBlkB ) + 1 + 
			(unsigned)( (*m_rng)() ) % ( std::max( posBlkA, posBlkB ) - std::min( posBlkA, posBlkB ) );
	} else if( posBlkO > std::min( posBlkA, posBlkB ) && posBlkO < std::max( posBlkA, posBlkB ) ) {
		newLocForO = std::min( posBlkA, posBlkB ) + 1 + 
			(unsigned)( (*m_rng)() ) % ( std::max( posBlkA, posBlkB ) - 1 - std::min( posBlkA, posBlkB ) );
	} else {
	}
	ite = m_seqPair.m_x.begin() + newLocForO;
	m_seqPair.m_x.insert( ite, blkO->GetId() );


	// Now move the cell in Y.....
	posBlkO = m_posInYSeqPair[blkO->GetId()];
	posBlkA = m_posInYSeqPair[blkA->GetId()];
	posBlkB = m_posInYSeqPair[blkB->GetId()];

	ite = m_seqPair.m_y.begin() + posBlkO;
	m_seqPair.m_y.erase( ite );

	// Account for the erasure of blkO by modifying 'newLoc'.
	// NB: insert() occurs *before* the specified value, so we go +1 to go to its right.
	if( posBlkO < std::min( posBlkA, posBlkB ) ) {
		// NB: We go -1 to account for the erasure of posBlkO.
		newLocForO = std::min( posBlkA, posBlkB ) + 1 - 1 + 
			(unsigned)( (*m_rng)() ) % ( std::max( posBlkA, posBlkB ) - std::min( posBlkA, posBlkB ) );
	} else if( posBlkO > std::max( posBlkA, posBlkB ) ) {
		newLocForO = std::min( posBlkA, posBlkB ) + 1 + 
			(unsigned)( (*m_rng)() ) % ( std::max( posBlkA, posBlkB ) - std::min( posBlkA, posBlkB ) );
	} else if( posBlkO > std::min( posBlkA, posBlkB ) && posBlkO < std::max( posBlkA, posBlkB ) ) {
		newLocForO = std::min( posBlkA, posBlkB ) + 1 + 
			(unsigned)( (*m_rng)() ) % ( std::max( posBlkA, posBlkB ) - 1 - std::min( posBlkA, posBlkB ) );
	} else {
	}
	ite = m_seqPair.m_y.begin() + newLocForO;
	m_seqPair.m_y.insert( ite, blkO->GetId() );
		

	// At this point, the sequence pairs have changed, which means
	// that we need to recompute the 'posInSeqPair' variables. 
	for( i = 0; i < m_blocks.size(); ++i ) {
		m_posInXSeqPair[m_seqPair.m_x[i]] = i;
		m_posInYSeqPair[m_seqPair.m_y[i]] = i;
	}
	return true;
}

void MinMovementFloorplanner::doSPMoveOrReversal( WhichGraph wg, 
					EdgeSet &es, 
	       				bool doReversal )
// ******************************************************************
// The edge set is the list of edges that we want to REMOVE from 'wg' (and,
// basically, add to 'tgtwg').  In other words, if we have an edge that goes
// from i -> j in H, we want it going from i -> j in V.
//
// If 'doReversal' is true, then an edge that goes i => j in H will go j => i
// in V.  This can possibly help to create more swapping opportunities.
{
	unsigned		j;
	unsigned		src, tgt;
	
	// Make all of the swaps in the edge set.  Here, we figure out how to
	// swap the edge from src => tgt in order to eliminate it from the
	// current graph direction.  Note that, as a result of swapping earlier
	// in the set, we might have eliminated it already, or we might have
	// botched an edge relationship.
	if( wg == HGraph ) {
		for( j = 0; j < es._pairs.size(); ++j ) {
			src = es._pairs[j]._src;
			tgt = es._pairs[j]._dst;

			// Sanity.

			if( m_posInXSeqPair[src] < m_posInXSeqPair[tgt] && m_posInYSeqPair[src] < m_posInYSeqPair[tgt] ) {
				if( !doReversal ) {
					// Make src => tgt in V.
					std::swap( m_seqPair.m_x[ m_posInXSeqPair[src] ], m_seqPair.m_x[ m_posInXSeqPair[tgt] ] );
					std::swap( m_posInXSeqPair[src], m_posInXSeqPair[tgt] );
				} else {
					// Make tgt => src in V.
					std::swap( m_seqPair.m_y[ m_posInYSeqPair[src] ], m_seqPair.m_y[m_posInYSeqPair[tgt] ] );
					std::swap( m_posInYSeqPair[src], m_posInYSeqPair[tgt] );
				}
			} else if( m_posInXSeqPair[src] > m_posInXSeqPair[tgt] && m_posInYSeqPair[src] > m_posInYSeqPair[tgt] ) {
				// As a result of a previous swap, we ended up
				// adding tgt => src as a constraint in H.  We
				// need to undo this.
				if( !doReversal ) {
					// Make src => tgt in V.
					std::swap( m_seqPair.m_y[ m_posInYSeqPair[src] ], m_seqPair.m_y[m_posInYSeqPair[tgt] ] );
					std::swap( m_posInYSeqPair[src], m_posInYSeqPair[tgt] );
				} else {
					// Make tgt => src in V.
					std::swap( m_seqPair.m_x[ m_posInXSeqPair[src] ], m_seqPair.m_x[ m_posInXSeqPair[tgt] ] );
					std::swap( m_posInXSeqPair[src], m_posInXSeqPair[tgt] );
				}
			} else if( m_posInXSeqPair[src] < m_posInXSeqPair[tgt] && m_posInYSeqPair[src] > m_posInYSeqPair[tgt] ) {
				// No edge in H.
			} else if( m_posInXSeqPair[src] > m_posInXSeqPair[tgt] && m_posInYSeqPair[src] < m_posInYSeqPair[tgt] ) {
				// No edge in H.
			} else {
			}
		}
	} else {
		for( j = 0; j < es._pairs.size(); ++j ) {
			src = es._pairs[j]._src;
			tgt = es._pairs[j]._dst;

			// Sanity.

			if( m_posInXSeqPair[src] < m_posInXSeqPair[tgt] && m_posInYSeqPair[src] < m_posInYSeqPair[tgt] ) {
				// No edge in V.
			} else if( m_posInXSeqPair[src] > m_posInXSeqPair[tgt] && m_posInYSeqPair[src] > m_posInYSeqPair[tgt] ) {
				// No edge in V.
			} else if( m_posInXSeqPair[src] < m_posInXSeqPair[tgt] && m_posInYSeqPair[src] > m_posInYSeqPair[tgt] ) {
				// As a result of a previous swap, we ended up
				// adding tgt => src as a constraint in V.  We
				// need to undo this.
				if( !doReversal ) {
					// Make src => tgt in H.
					std::swap( m_seqPair.m_y[ m_posInYSeqPair[src] ], m_seqPair.m_y[m_posInYSeqPair[tgt] ] );
					std::swap( m_posInYSeqPair[src], m_posInYSeqPair[tgt] );
				} else {
					// Make tgt => src in H.
					std::swap( m_seqPair.m_x[ m_posInXSeqPair[src] ], m_seqPair.m_x[ m_posInXSeqPair[tgt] ] );
					std::swap( m_posInXSeqPair[src], m_posInXSeqPair[tgt] );
				}
			} else if( m_posInXSeqPair[src] > m_posInXSeqPair[tgt] && m_posInYSeqPair[src] < m_posInYSeqPair[tgt] ) {
				if( !doReversal ) {
					// Make src => tgt in H.
					std::swap( m_seqPair.m_x[ m_posInXSeqPair[src] ], m_seqPair.m_x[ m_posInXSeqPair[tgt] ] );
					std::swap( m_posInXSeqPair[src], m_posInXSeqPair[tgt] );
				} else {
					// Make tgt => src in H.
					std::swap( m_seqPair.m_y[ m_posInYSeqPair[src] ], m_seqPair.m_y[m_posInYSeqPair[tgt] ] );
					std::swap( m_posInYSeqPair[src], m_posInYSeqPair[tgt] );
				}
			} else {
			}
		}
	}
}
void MinMovementFloorplanner::drawBlocks( 
            std::string str, 
            std::vector<Block*>& blocks )
{

    // Draws the current blocks in the problem.

    char buf[1024];
    (void) buf;
    static unsigned	counter = 0;
    static char filename[BUFFER_SIZE + 1];
    FILE* fp;
   	unsigned n;
	Block* blk;
    double x, y, h, w;

    double xMinBox = m_Tmin[HGraph];
    double xMaxBox = m_Tmax[HGraph];
    double yMinBox = m_Tmin[VGraph];
    double yMaxBox = m_Tmax[VGraph];

    snprintf( filename, BUFFER_SIZE, "%s.%d.gp", str.c_str(), counter++ );

   	if( ( fp = fopen( filename, "w" ) ) == 0 ) 
    {
        std::cout << "Error opening file " << filename << std::endl;
		return;
	}
	
	// Output the header.
	fprintf( fp, 	"# Use this file as a script for gnuplot\n"
			"set title \"# Blocks: %d (remember, there may be overlaps!\"\n"
			"set nokey\n"
            "set noxtics\n"
            "set noytics\n"
            "set nokey\n"
            "set terminal x11\n"
			"#Uncomment these two lines starting with \"set\"\n"
			"#to save an EPS file for inclusion into a latex document\n"
			"#set terminal postscript eps color solid 20\n"
			"#set output \"%s\"\n\n",
			(unsigned) blocks.size(), filename );

	// Constrain the x and y ranges.
	double		fracX, fracY;
	fracX = 0.01 * ( m_Tmax[HGraph] - m_Tmin[HGraph] );
	fracY = 0.01 * ( m_Tmax[HGraph] - m_Tmin[HGraph] );

	fprintf( fp, 	"set xrange[%f:%f]\n"
			"set yrange[%f:%f]\n",
			m_Tmin[HGraph] - fracX, m_Tmax[HGraph] + fracX,
			m_Tmin[VGraph] - fracY, m_Tmax[VGraph] + fracY );
	
	// Output the plot command.
	fprintf( fp, "plot[:][:] '-' w l\n" );

	// Display nodes.
    	for( n = 0; n < blocks.size(); ++n ) {
		blk = blocks[n];

	    	x = blk->GetPos( HGraph );
	    	y = blk->GetPos( VGraph );
	    	h = blk->GetDim( VGraph );
	    	w = blk->GetDim( HGraph );

            w *= 0.90;
            h *= 0.90;

		fprintf( fp, "\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "\n", 
				x - 0.5 * w, y - 0.5 * h,
				x - 0.5 * w, y + 0.5 * h,
				x + 0.5 * w, y + 0.5 * h,
				x + 0.5 * w, y - 0.5 * h,
				x - 0.5 * w, y - 0.5 * h );
    	}

	// Output the bounding box ...
	fprintf( fp, "\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "%lf %lf\n" "\n",
			xMinBox, yMinBox,
			xMinBox, yMaxBox,
			xMaxBox, yMaxBox,
			xMaxBox, yMinBox,
			xMinBox, yMinBox );

	// Cleanup.
	fclose( fp );
}


void MinMovementFloorplanner::extractSubgraph( WhichGraph wg )
// ***********************************************************
// Constructs the critical (<= 0 slack) transitively-reduced network for the
// specified direction.  
{
	Block				*blk;
	FPGraph				&gr = *(m_graph[wg]);
	unsigned				i, j;
	std::vector<bool>		isInSubgraph( m_blocks.size(), false );
	unsigned				nNodes = 0;
	Slack				*slk;
	FPGraph				&subgr = *(_subgraph[wg]);
	double				slack;
	unsigned				src, tgt;
	std::vector<int>		&subgrMapping = _subgraphMapping[wg];
	std::vector<Block *>		&subgrReverseMapping = _subgraphReverseMapping[wg];

	subgr.Clear();

	subgrMapping.resize( m_blocks.size() );

	subgrMapping.resize( m_blocks.size() );
	std::fill( subgrMapping.begin(), subgrMapping.end(), -1 );

	subgrReverseMapping.resize( m_blocks.size() );
	std::fill( subgrReverseMapping.begin(), subgrReverseMapping.end(), (Block *) NULL );

	// Find the critical nodes in the problem.  Note that we include fixed
	// cells, too, because it helps us to (later) pick up arcs between the
	// fixed and non-fixed cells.
	for( i = 0; i < m_blocks.size(); ++i ) {
		blk = m_blocks[i];
		slk = blk->GetSlack( wg );
		slack = slk->ComputeSlack();

		if( slack <= ( m_worstSlack[wg] + 1e-3 ) || 
			( ( wg == HGraph ) && ( blk->GetFixed() ) ) ||
			( ( wg == VGraph ) && ( blk->GetFixed() ) ) ) {

			subgrMapping[blk->GetId()] = nNodes;
			subgrReverseMapping[nNodes] = blk;
			++nNodes;
			isInSubgraph[blk->GetId()] = true;
		}
	}

	subgr.Resize( nNodes );

	// NB: When we go to adjust edges, during graph operations, we have to
	// work with *Reduction* edges.  Instead of computing reduction edges
	// every time that we make a move, I compute the transitive reduction
	// on the extracted subgraph.
	for( i = 0; i < m_blocks.size(); ++i ) {
		blk = m_blocks[i];

		if( isInSubgraph[blk->GetId()] ) {
			src = i;
				
			for( j = 0; j < gr.GetNumOutEdges( src ); ++j ) {
				tgt = gr.GetOutEdgeDest( i, j );

				if( isInSubgraph[tgt] ) {
					// Don't add edges between two fixed
					// cells.  (We don't bother trying to
					// swap such edges.)
					if( ( ( wg == HGraph ) && 
						( m_blocks[src]->GetFixed() ) &&
						( m_blocks[tgt]->GetFixed() ) )
						
						||
							
						( ( wg == VGraph ) && 
						( m_blocks[src]->GetFixed() ) &&
						( m_blocks[tgt]->GetFixed() ) ) ) {

						continue;
					}
					subgr.AddEdge( subgrMapping[src], subgrMapping[tgt] );
				}
			}
		}
	}
	subgr.TransitiveReduction();

	// Sanity.
}


bool MinMovementFloorplanner::getCandidateArcs( WhichGraph wg,
		Block *blkO, std::vector<Arc> &arcs )
// ***********************************************************
// Loop through the graph, and find arcs {A,B} which have large enough slack to
// fit blkO.
//
// NB: This function doesn't clear 'arcs', it just keeps adding to it.  So you
// could actually call this function with different directions, and get all
// arcs if you wanted to.
{
	Block					*blkA, *blkB;
        unsigned                                  i, j;
        std::vector<unsigned>::iterator           ite;
        std::deque<Node*>                       neighbours;
	double					pcSlack;
	double					pcDiscount, pcCrit;
	Slack					*srcslk, *tgtslk;
	Arc					tmpArc;

	tmpArc._wg = wg;

	for( i = 0; i < m_blocks.size(); ++i ) {
		blkA = m_blocks[i];
		if( blkA == blkO )	continue;

		srcslk = blkA->GetSlack( wg );
		tmpArc._ep._src = i;

		for( j = 0; j < m_graph[wg]->GetNumOutEdges( i ); ++j ) {
			blkB = m_blocks[ m_graph[wg]->GetOutEdgeDest( i, j ) ];
			if( blkB == blkO )	continue;

			tgtslk = blkB->GetSlack( wg );
			pcSlack = tgtslk->GetTreq() - srcslk->GetTarr() - blkA->GetDim( wg );

			if( pcSlack >= blkO->GetDim( wg ) ) {
				tmpArc._ep._dst = blkB->GetId();

				// Estimate the distance of this arc to blkO,
				// using the blocks' original positions as a
				// guide.  While this is no doubt
				// approximative, it is fairly fast and
				// probably not too bad quality-wise.
				tmpArc._dist = std::fabs( 0.5 * ( blkA->GetPos(HGraph) + blkB->GetPos(HGraph) ) - 
							blkO->GetPos(HGraph) ) +
						std::fabs( 0.5 * ( blkA->GetPos(VGraph) + blkB->GetPos(VGraph) ) -
							blkO->GetPos(VGraph) );

				pcDiscount = DISCOUNT( pcSlack, m_maxTarr[wg] );
				pcCrit = m_pathCountF[wg][blkA->GetId()] * m_pathCountB[wg][blkB->GetId()] * pcDiscount;

				tmpArc._crit = pcCrit;
				arcs.push_back( tmpArc );
			}
		}
	}
	if( arcs.empty() ) {
		return false;
	}
	return true;
}


MinMovementFloorplanner::Block *MinMovementFloorplanner::getCandidateBlock( WhichGraph wg, bool onlySingleHeight )
{
    // Chooses a "good" source block from among the most critical blocks in the placement.

    std::vector<BlockSet> blockSet;
        unsigned                                  i, j;
        std::vector<unsigned>::iterator           ite;
        std::deque<Node*>                       neighbours;
	double					pcDiscount, pcCrit = 0.;
	double					pcSlack;
	Block					*srcblk, *tgtblk;
	Slack					*srcslk, *tgtslk;
	BlockSet				tmpBlockSet;
    double singleRowHeight = m_arch->m_rows[0]->getH();
    int count = 0;

	// Get a sorted list of blocks in the placement which are close to critical.
	blockSet.reserve( m_blocks.size() );
	for( int i = 0; i < m_blocks.size(); ++i ) 
    {
		Block* srcblk = m_blocks[i];
		Slack* srcslk = srcblk->GetSlack( wg );

		// Skip over fixed cells.
        if( srcblk->GetFixed() ) 
        {
            continue;
		}


		if( srcslk->ComputeSlack() > (m_worstSlack[wg] + 1e-3) ) {
            continue;
        }

        ++count;

        // Consider skipping multi-height cells.
        if( onlySingleHeight )
        {
            Node* ndi = srcblk->GetNode();
            if( ndi != 0 )
            {
                int spanned = (int)(ndi->getHeight() / singleRowHeight + 0.5);
                if( spanned > 1 )
                {
                    continue;
                }
            }
        }

        // Get maximum weight over all critical edges leaving the block.
        double ap = -std::numeric_limits<double>::max();
		for( int j = 0; j < m_graph[wg]->GetNumOutEdges( i ); ++j ) {
			Block* tgtblk = m_blocks[ m_graph[wg]->GetOutEdgeDest( i, j ) ];
			Slack* tgtslk = tgtblk->GetSlack( wg );
			
            double slack = tgtslk->GetTreq() - srcslk->GetTarr() - srcblk->GetDim( wg );
            double discount = DISCOUNT( slack, m_maxTarr[wg] );

            ap = std::max( ap, m_pathCountF[wg][srcblk->GetId()] * m_pathCountB[wg][tgtblk->GetId()] * discount );
		}
		tmpBlockSet._blk = srcblk;
        // XXX: pcCrit not ever set??????????????????????
		tmpBlockSet._crit = pcCrit;
		blockSet.push_back( tmpBlockSet );
	}
	if( blockSet.empty() ) 
    {
        std::cout << "No candidate single height cells, " << count << " candidates otherwise." << std::endl;
		return NULL;
	}
	std::sort( blockSet.begin(), blockSet.end(), SortBlockSet() );

	// Experiments have shown that a choice among ~10 cells leads to pretty
	// decent answers.
	return blockSet[ unsigned( (*m_rng)() ) % ( std::min( (unsigned)blockSet.size(), (unsigned)10) ) ]._blk;
}

void MinMovementFloorplanner::pathCounting( WhichGraph wg )
{
    // Determines the criticality of arcs in the desired graph using the algorithm
    // from Tim Kong's 2004 paper.
	double			Fs, Bs;
	double			discount;
	FPGraph			&gr = *m_graph[wg];
        unsigned          	i;
	unsigned			srcidx, tgtidx;
	int			j;
	Block			*srcblk, *tgtblk;
	Slack			*srcslk, *tgtslk;

	gr.TopologicalSort( m_order );
	
	// Initialization ...
    for( int i = 0; i < m_blocks.size(); ++i ) {
		Block* srcblk = m_blocks[i];

		if( gr.GetNumInEdges( i ) == 0 ) {	// PI
			m_pathCountF[wg][i] = 1;
		} 
        else {
			m_pathCountF[wg][i] = 0;
		}

		if( gr.GetNumOutEdges( i ) == 0 ) {	// PO
			m_pathCountB[wg][i] = 1;
		} 
        else {
			m_pathCountB[wg][i] = 0;
		}
    }

	// Forward traversal ...
    for( int i = 0; i < m_order.size(); ++i ) {
		int tgtidx = m_order[i];
		Block* tgtblk = m_blocks[tgtidx];
		Slack* tgtslk = tgtblk->GetSlack( wg );

		for( int j = 0; j < gr.GetNumInEdges( tgtidx ); ++j ) {
			int srcidx = gr.GetInEdgeSrc( tgtidx, j );
			Block* srcblk = m_blocks[srcidx];
			Slack* srcslk = srcblk->GetSlack( wg );

			double Fs = tgtslk->GetTarr() - srcslk->GetTarr() - srcblk->GetDim( wg );
			double discount = DISCOUNT( Fs, m_maxTarr[wg] );
			m_pathCountF[wg][tgtidx] += discount * m_pathCountF[wg][srcidx];
		}
	}

	// Backward traversal ...
	for( int j = m_order.size() - 1; j >= 0; j-- ) {
		int srcidx = m_order[j];
		Block* srcblk = m_blocks[srcidx];
		Slack* srcslk = srcblk->GetSlack( wg );

		for( i = 0; i < gr.GetNumOutEdges( srcidx ); ++i ) {
			int tgtidx = gr.GetOutEdgeDest( srcidx, i );
			Block* tgtblk = m_blocks[tgtidx];
			Slack* tgtslk = tgtblk->GetSlack( wg );

			double Bs = tgtslk->GetTreq() - srcslk->GetTreq() - srcblk->GetDim( wg );
			double discount = DISCOUNT( Bs, m_maxTarr[wg] );
			m_pathCountB[wg][srcidx] += discount * m_pathCountB[wg][tgtidx];
		}
	}
}




/**
bool MinMovementFloorplanner::setupAndSolveLP( bool doResize,
		bool onlySolveForX, 
        double moveLimitX, double moveLimitY, 
        double& maxMovedX, double& maxMovedY,
		double& amtMovedX, double& amtMovedY,
        double& amtResized,
        bool observeGapsForX )
// ***********************************************************
// Sets up an LP similar to what is described in "A Robust Detailed
// Placement for Mixed-Size IC Designs" (by Cong and Xie).  
//
// If 'doResize' is true, then the modules will be resized to make the
// constraint graphs fit.  Note that the resizing is heavily penalized -- this
// code much prefers to move cells than resize them.  This setting should
// typically be 'true', even if constraint graphs are valid, to handle the case
// of numerical inaccuracy in the LP solver not quite recognizing
// perfectly-legal constraint graphs.
//
// If 'onlySolveForX' is true, then this code computes a shift only in the X
// direction, and NOT in Y.  (This would be done, for example, to preserve row
// assignments.)
//
// AAK: If 'observeGapsForX' is true, then the code will attempt to observe
// spacing constraints between cells in the X-direction.  Note that this 
// might result in resizing and an illegal placement (since resizing might
// introduce overlap).  XXX: The satisfaction of gaps could likely be handled
// much better.  For example, if the LP is not feasible with the gaps, then 
// we could allow resizing of the gaps (i.e., get a solution, but without all
// the gaps being satisfied).  I should think about making such an improvement.
//
// This function returns the amount moved in 'amtMoved' and the amount by which
// cells were resized in 'amtResized'.  (If the amount resized was numerically
// insigificant, then one could interpret the placement as being legal.)
//
// Returns true for success, false for failure (infeasibility breakdown).
//
// NB:
// - This function changes the graphs (by computing a transitive reduction).
//
// - This function is NOT intended for handling soft blocks.  Module resizing,
// for instance, does not take into account aspect ratios, and widths/heights
// are set to go from 1 unit up to their original size (but not larger).
// Significant rethinking of the linear program would be required to handle
// soft blocks here.
//
// - This function will abort on solving error.  It just shouldn't happen.
{
    maxMovedX = 0.;
    maxMovedY = 0.;

    amtMovedX = 0.;
    amtMovedY = 0.;

    amtResized = 0.;

    if( doResize ) {
        std::cout << "Resizing is enabled." << std::endl;
    }

	std::vector<double>	ar;
	std::vector<int>	ia, ja;
	unsigned			i, j;
	int			ret;
	Block			*srcblk, *tgtblk;
	double			val;

	unsigned 			LIMIT_DXI_LO, LIMIT_DXI_HI;
	unsigned 			LIMIT_DYI_LO, LIMIT_DYI_HI;
	unsigned 			LIMIT_XIP_LO, LIMIT_XIP_HI;
	unsigned 			LIMIT_YIP_LO, LIMIT_YIP_HI;
	unsigned 			LIMIT_DHEI_LO, LIMIT_DHEI_HI;
	unsigned 			LIMIT_DWID_LO, LIMIT_DWID_HI;
    (void)LIMIT_DWID_HI;

//    std::cout << "[" << m_Tmin[HGraph] << "," << m_Tmax[HGraph] << "]" 
//         << "-"
//         << "[" << m_Tmin[VGraph] << "," << m_Tmax[VGraph] << "]" 
//         << std::endl;

    moveLimitX = std::min( moveLimitX, COIN_DBL_MAX );
    moveLimitY = std::min( moveLimitY, COIN_DBL_MAX );

	// Setup the "limits" on the columns.
	LIMIT_DXI_LO = 0;
	LIMIT_DXI_HI = LIMIT_DXI_LO + m_blocks.size() - 1;
	LIMIT_DYI_LO = LIMIT_DXI_HI + 1;
	LIMIT_DYI_HI = LIMIT_DYI_LO + m_blocks.size() - 1;
	LIMIT_XIP_LO = LIMIT_DYI_HI + 1;
	LIMIT_XIP_HI = LIMIT_XIP_LO + m_blocks.size() - 1;
	LIMIT_YIP_LO = LIMIT_XIP_HI + 1;
	LIMIT_YIP_HI = LIMIT_YIP_LO + m_blocks.size() - 1;
	LIMIT_DHEI_LO = LIMIT_YIP_HI + 1;
	LIMIT_DHEI_HI = LIMIT_DHEI_LO + m_blocks.size() + 1;
	LIMIT_DWID_LO = LIMIT_DHEI_HI + 1;
	LIMIT_DWID_HI = LIMIT_DWID_LO + m_blocks.size() + 1;

	// Build the model.
	CoinModel	build;

	// Set the limits on columns.
	for( i = 0; i < m_blocks.size(); ++i ) 
    {
		srcblk = m_blocks[i];

		// Limits on the {dxi, dyi} variables.
		build.setContinuous( LIMIT_DXI_LO + i );

		if( !onlySolveForX ) 
        {
			build.setContinuous( LIMIT_DYI_LO + i );
		}

		if( srcblk->GetFixed() ) 
        {
			build.setColumnBounds( LIMIT_DXI_LO + i, 0., 0. );
		} 
        else 
        {
			build.setColumnBounds( LIMIT_DXI_LO + i, -moveLimitX, moveLimitX );
		}

		if( !onlySolveForX ) 
        {
			if( srcblk->GetFixed() ) 
            {
				build.setColumnBounds( LIMIT_DYI_LO + i, 0., 0. );
			} 
            else 
            {
				build.setColumnBounds( LIMIT_DYI_LO + i, -moveLimitY, moveLimitY );
			}
		}

		// Col constraint:  _tmin[HGraph] + 0.5 * width(i) <= xiprime <= _tmax[HGraph] - 0.5 * width(i)
		build.setContinuous( LIMIT_XIP_LO + i );
		if( srcblk->GetFixed() ) 
        {
			build.setColumnBounds( LIMIT_XIP_LO + i, srcblk->GetOrigPos(HGraph), srcblk->GetOrigPos(HGraph) );
		} 
        else 
        {
            double minpos = m_Tmin[HGraph] + 0.5 * srcblk->GetDim( HGraph );
            double maxpos = m_Tmax[HGraph] - 0.5 * srcblk->GetDim( HGraph );
			build.setColumnBounds( LIMIT_XIP_LO + i, minpos, maxpos );
		}
		// Col constraint:  0.5 * height(i) <= yiprime <= _tmax[VGraph] - 0.5 * height(i)
		if( !onlySolveForX ) 
        {
			build.setContinuous( LIMIT_YIP_LO + i );
			if( srcblk->GetFixed() ) 
            {
				build.setColumnBounds( LIMIT_YIP_LO + i, srcblk->GetOrigPos(VGraph), srcblk->GetOrigPos(VGraph) );
			}
            else 
            {
                double minpos = m_Tmin[VGraph] + 0.5 * srcblk->GetDim( VGraph );
                double maxpos = m_Tmax[VGraph] - 0.5 * srcblk->GetDim( VGraph );
				build.setColumnBounds( LIMIT_YIP_LO + i, minpos, maxpos );
			}
		}

		// Limits on the delta widths/heights of variables.  (Can only
		// shrink as much as the dimension of the cell.)
		if( doResize) 
        {
			build.setContinuous( LIMIT_DWID_LO + i );
			if( srcblk->GetFixed() ) 
            {
                // XXX: Let fixed blocks shrink too. 
				//build.setColumnBounds( LIMIT_DWID_LO + i, 0., 0. );
				build.setColumnBounds( LIMIT_DWID_LO + i, 0., srcblk->GetDim( HGraph ) );
			} 
            else 
            {
				build.setColumnBounds( LIMIT_DWID_LO + i, 0., srcblk->GetDim( HGraph ) );
			}

			if( !onlySolveForX ) 
            {
				build.setContinuous( LIMIT_DHEI_LO + i );
				if( srcblk->GetFixed() ) 
                {
                    // XXX: Let fixed blocks shrink too. 
					//build.setColumnBounds( LIMIT_DHEI_LO + i, 0., 0. );
				    build.setColumnBounds( LIMIT_DHEI_LO + i, 0., srcblk->GetDim( VGraph ) );
				} 
                else
                {
					build.setColumnBounds( LIMIT_DHEI_LO + i, 0., srcblk->GetDim( VGraph ) );
				}
			}
		}

		// Setup the objective function coefficients.
		// We *heavily* penalize the shrinking of cells.
		if( srcblk->GetFixed() ) 
        {
			build.setColumnObjective( LIMIT_DXI_LO + i, 0. );
			//if( doResize)	build.setColumnObjective( LIMIT_DWID_LO + i, 0. );
			if( doResize)	build.setColumnObjective( LIMIT_DWID_LO + i, 1e8 );
		}
        else 
        {
			build.setColumnObjective( LIMIT_DXI_LO + i, 1. );
			if( doResize)	build.setColumnObjective( LIMIT_DWID_LO + i, 1e8 );
		}

		if( !onlySolveForX ) 
        {
			if( srcblk->GetFixed() ) 
            {
				build.setColumnObjective( LIMIT_DYI_LO + i, 0. );
				//if( doResize)	build.setColumnObjective( LIMIT_DHEI_LO + i, 0. );
				if( doResize)	build.setColumnObjective( LIMIT_DHEI_LO + i, 1e8 );
			}
            else 
            {
				build.setColumnObjective( LIMIT_DYI_LO + i, 1. );
				if( doResize)	build.setColumnObjective( LIMIT_DHEI_LO + i, 1e8 );
			}
		}
	}

	// Add rows to the problem.
	for( i = 0; i < m_blocks.size(); ++i ) 
    {
		srcblk = m_blocks[i];

		// Constr 1A:	xi <= xiprime + dxi 
		if( !( srcblk->GetFixed() ) ) 
        {
      			int	colIndex[]	= { LIMIT_XIP_LO + i, LIMIT_DXI_LO + i };
      			double	elemValue[]	= { 1., 1. };
			build.addRow( 2, colIndex, elemValue, m_blocks[i]->GetOrigPos( HGraph ), COIN_DBL_MAX );
		}
        else 
        {
      			int	colIndex[]	= { LIMIT_XIP_LO + i};
      			double	elemValue[]	= { 1. };
			build.addRow( 1, colIndex, elemValue, m_blocks[i]->GetOrigPos( HGraph ), COIN_DBL_MAX );
		}

		// Constr 1B:	xiprime - dxi <= xi
		if( !( srcblk->GetFixed() ) ) 
        {
      			int	colIndex[]	= { LIMIT_XIP_LO + i, LIMIT_DXI_LO + i };
      			double	elemValue[]	= { 1., -1. };
			build.addRow( 2, colIndex, elemValue, -COIN_DBL_MAX, m_blocks[i]->GetOrigPos( HGraph ) );
		}
        else 
        {
      			int	colIndex[]	= { LIMIT_XIP_LO + i};
      			double	elemValue[]	= { 1. };
			build.addRow( 1, colIndex, elemValue, -COIN_DBL_MAX, m_blocks[i]->GetOrigPos( HGraph ) );
		}

		if( !onlySolveForX ) 
        {
			// Constr 2A:   yi <= yiprime + dyi
			if( !( srcblk->GetFixed() ) ) 
            {
				int	colIndex[]	= { LIMIT_YIP_LO + i, LIMIT_DYI_LO + i };
				double	elemValue[]	= { 1., 1. };
				build.addRow( 2, colIndex, elemValue, m_blocks[i]->GetOrigPos( VGraph ), COIN_DBL_MAX );
			} 
            else 
            {
				int	colIndex[]	= { LIMIT_YIP_LO + i };
				double	elemValue[]	= { 1. };
				build.addRow( 1, colIndex, elemValue, m_blocks[i]->GetOrigPos( VGraph ), COIN_DBL_MAX );
			}

			// Constr 2B:	yiprime - dyi <= yi
			if( !( srcblk->GetFixed() ) ) 
            {
				int	colIndex[]	= { LIMIT_YIP_LO + i, LIMIT_DYI_LO + i };
				double	elemValue[]	= { 1., -1. };
				build.addRow( 2, colIndex, elemValue, -COIN_DBL_MAX, m_blocks[i]->GetOrigPos( VGraph ) );
			}
            else 
            {
				int	colIndex[]	= { LIMIT_YIP_LO + i };
				double	elemValue[]	= { 1. };
				build.addRow( 1, colIndex, elemValue, -COIN_DBL_MAX, m_blocks[i]->GetOrigPos( VGraph ) );
			}
		}
	}
	// Constr 3: xjprime - xiprime >= 0.5 * width(i) + 0.5 * width(j) - 0.5 * dwid(i) - 0.5 * dwid(j) if e_{ij} in G_h
	//
	// NB1: no DWID variables if resizing is disabled)
	// NB2: there is no constraint between two fixed modules
    //
    // XXX: What if, for any reason, we have a constraint from a fixed node to a moveable node
    // which *FORCES* the moveable node to be outside of the placement area?  In this case,
    // the problem will never be solved!!!  I *think* the solution would be to simply ensure 
    // that any fixed nodes entirely outside of the placement area are ignored when collecting
    // blocks.  But, should we do something here anyway...
	for( i = 0; i < m_graph[HGraph]->GetNumNodes(); ++i ) 
    {
		srcblk = m_blocks[i];
		for( j = 0; j < m_graph[HGraph]->GetNumOutEdges( i ); ++j ) 
        {
			tgtblk = m_blocks[ m_graph[HGraph]->GetOutEdgeDest( i, j ) ];

			if( ( srcblk->GetFixed() ) &&
				( tgtblk->GetFixed() ) ) 
            {
				continue;
			}
			val = 0.5 * ( srcblk->GetDim( HGraph ) + tgtblk->GetDim( HGraph ) );

            // TRY TO SATISFY GAPS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // This is where gaps can be satisfied.  If these blocks are both cells, then
            // the spacing should be larger than 0.5*width(i)+0.5*width(j)+gap(i,j).  
            // I think it is possible that including this can result in an infeasible LP.
            // However, we can detect that upon return and do something about it.
            if( observeGapsForX )
            {
                Node* ndi = srcblk->GetNode();
                Node* ndj = tgtblk->GetNode();
                if( ndi != 0 && ndj != 0 )
                {
                    double gap = m_arch->get_cell_spacing( ndi->m_etr, ndj->m_etl );
                    val += gap;
                }
            }

			if( doResize ) 
            {
				int	colIndex[]	= { LIMIT_XIP_LO + tgtblk->GetId(),
								LIMIT_XIP_LO + srcblk->GetId(),
								LIMIT_DWID_LO + srcblk->GetId(),
								LIMIT_DWID_LO + tgtblk->GetId() };
				double	elemValue[]	= { 1., -1., 0.5, 0.5 };

				build.addRow( 4, colIndex, elemValue, val, COIN_DBL_MAX );
			} 
            else 
            {
				int	colIndex[]	= { LIMIT_XIP_LO + tgtblk->GetId(),
								LIMIT_XIP_LO + srcblk->GetId() };
				double	elemValue[]	= { 1., -1. };

				build.addRow( 2, colIndex, elemValue, val, COIN_DBL_MAX );
			}
		}
	}
	// Constr 4: yjprime - yiprime >= 0.5 * height(i) + 0.5 * height(j) - 0.5 * dhei(i) - 0.5 * dhei(j) if e_{ij} in G_v
	// 
	// NB1: no DHEI variables if resizing is disabled)
	// NB2: there is no constraint between two fixed modules

    // XXX: I've found a problem.   It could be the situation where moveable blocks are constrained to 
    // be above fixed blocks (and there could be a cascade effect; i.e., Moveable B above fixed A, and
    // then moveable C above moveable B - which means C must also be above A - etc.  This means the
    // bound variables on the moveables which keep them inside the placement area might not be satisfied.
    // How to fix this in the general case????????????????  This will cause the solver to fail.

	if( !onlySolveForX ) 
    {
		for( i = 0; i < m_graph[VGraph]->GetNumNodes(); ++i ) 
        {
			srcblk = m_blocks[i];
			for( j = 0; j < m_graph[VGraph]->GetNumOutEdges( i ); ++j ) 
            {
				tgtblk = m_blocks[ m_graph[VGraph]->GetOutEdgeDest( i, j ) ];

				if( ( srcblk->GetFixed() ) &&
					( tgtblk->GetFixed() ) ) 
                {
					continue;
				}
				val = 0.5 * ( srcblk->GetDim( VGraph ) + tgtblk->GetDim( VGraph ) );

				if( doResize ) 
                {
					int	colIndex[]	= { LIMIT_YIP_LO + tgtblk->GetId(),
									LIMIT_YIP_LO + srcblk->GetId(),
									LIMIT_DHEI_LO + srcblk->GetId(),
									LIMIT_DHEI_LO + tgtblk->GetId() };
					double	elemValue[]	= { 1., -1., 0.5, 0.5 };

					build.addRow( 4, colIndex, elemValue, val, COIN_DBL_MAX );

				} 
                else 
                {
					int	colIndex[]	= { LIMIT_YIP_LO + tgtblk->GetId(),
									LIMIT_YIP_LO + srcblk->GetId() };
					double	elemValue[]	= { 1., -1. };

					build.addRow( 2, colIndex, elemValue, val, COIN_DBL_MAX );
				}
			}
		}
	}
	
	// Setup & solve the problem.
	ClpSimplex	model;
	model.loadProblem( build );

	model.setOptimizationDirection( 1 );	// 1 == minimize.
	model.setLogLevel( 0 );
	model.messageHandler()->setLogLevel( 0 );

	ClpSolve solvectl;
	solvectl.setSolveType( ClpSolve::useDual );	// useDual usePrimal usePrimalorSprint 
	solvectl.setPresolveType( ClpSolve::presolveOn );
	ret = model.initialSolve( solvectl );

	if( ret == 0 ) 
    {
		;	// Solution achieved.
	} 
    else if( ret > 0 ) 
    {
        std::cout << "LP unsolvable." << std::endl;
#if 0
		// The following is *exceptionally* useful for identifying 
		// the constraint(s) that render an LP model invalid. 
		double	*debugger = model.infeasibilityRay();
		double	tmp;

		printf( "Infeasibility ray for DXI:\n" );
		for( i = 0; i < m_blocks.size(); ++i ) {
			tmp = debugger[ LIMIT_DXI_LO + i ];
			if( tmp < 0. ) {
				printf( "Idx: %d, val: %.15e ", i, tmp );
			}
		}
		printf( "\n" );
		printf( "Infeasibility ray for DYI:\n" );
		for( i = 0; i < m_blocks.size(); ++i ) {
			tmp = debugger[LIMIT_DYI_LO + i];
			if( tmp < 0. ) {
				printf( "Idx: %d, val: %.15e ", i, tmp );
			}
		}
		printf( "\n" );
		printf( "Infeasibility ray for XIP:\n" );
		for( i = 0; i < m_blocks.size(); ++i ) {
			tmp = debugger[LIMIT_XIP_LO + i];
			if( tmp < 0. ) {
				printf( "Idx: %d, val: %.15e ", i, tmp );
			}
		}
		printf( "\n" );
		printf( "Infeasibility ray for YIP:\n" );
		for( i = 0; i < m_blocks.size(); ++i ) {
			tmp = debugger[LIMIT_YIP_LO + i];
			if( tmp < 0. ) {
				printf( "Idx: %d, val: %.15e ", i, tmp );
			}
		}
		printf( "\n" );
		printf( "Infeasibility ray for DWID:\n" );
		for( i = 0; i < m_blocks.size(); ++i ) {
			tmp = debugger[LIMIT_DWID_LO + i];
			if( tmp < 0. ) {
				printf( "Idx: %d, val: %.15e ", i, tmp );
			}
		}
		printf( "\n" );
		printf( "Infeasibility ray for DHEI:\n" );
		for( i = 0; i < m_blocks.size(); ++i ) {
			tmp = debugger[LIMIT_DHEI_LO + i];
			if( tmp < 0. ) {
				printf( "Idx: %d, val: %.15e, %s, %.2lf %.2lf", i, tmp, (m_blocks[i]->GetFixed() ? "F" : "M"),
                        m_blocks[i]->GetOrigPos(VGraph), m_blocks[i]->GetDim(VGraph));
			}
		}
		printf( "\n" );
#endif

		// It is _possible_ that the LP is unsolvable for the following
		// reason: let's say that you have two fixed, overlapping
		// cells, A and C.  Let's say that a moveable cell, B, is
		// placed between these two fixed cells, and that constraint
		// edges get built s.t. A => B => C.  (Incidentally, one can
		// easily imagine that "B" represents more than just one cell.) 
		// If the edge adjustment couldn't purge this slack (e.g., by
		// removing this edge constraint), then this problem is
		// unsolvable.
		return false;
		
	}

	double	*columnPrimal = model.primalColumnSolution();

	// Compute the amount moved and resized.
	for( i = 0; i < m_blocks.size(); ++i ) 
    {
		amtMovedX += std::fabs( columnPrimal[ LIMIT_DXI_LO + i ] );
		if( !onlySolveForX ) {
			amtMovedY += std::fabs( columnPrimal[ LIMIT_DYI_LO + i ] );
		}

        maxMovedX = std::max( maxMovedX, std::fabs( columnPrimal[ LIMIT_DXI_LO + i ] ) );
        if( !onlySolveForX ) {
            maxMovedY = std::max( maxMovedY, std::fabs( columnPrimal[ LIMIT_DYI_LO + i ] ) );
        }

		if( doResize ) 
        {
			// Sanity.
            double dwid = (columnPrimal[LIMIT_DWID_LO + i] < 0.) ? 0. : columnPrimal[LIMIT_DWID_LO + i];
			amtResized += dwid;

            if( dwid < -1.0e-3 ) {
                std::cout << "Warning: block " << m_blocks[i]->GetId() << ", " << (m_blocks[i]->GetFixed() ? "" : "not") << " fixed, "
                     << "has resized width " << columnPrimal[LIMIT_DWID_LO+i]
                     << " (" << build.getColumnLower( LIMIT_DWID_LO+i ) << ",  " << build.getColumnUpper( LIMIT_DWID_LO+i ) << ")"
                     << std::endl;
            }

			if( !onlySolveForX ) 
            {
                double dhei = (columnPrimal[LIMIT_DHEI_LO + i] < 0.) ? 0. : columnPrimal[LIMIT_DHEI_LO + i];
				amtResized += dhei;
                if( columnPrimal[LIMIT_DHEI_LO + i ] < 0.0 ) {
                    std::cout << "Warning: block " << m_blocks[i]->GetId() << ", " << (m_blocks[i]->GetFixed() ? "" : "not") << " fixed, "
                         << "has resized height " << columnPrimal[LIMIT_DHEI_LO+i]
                         << " (" << build.getColumnLower( LIMIT_DHEI_LO+i ) << ",  " << build.getColumnUpper( LIMIT_DHEI_LO+i ) << ")"
                         << std::endl;
                }
			}
		}
	}

	// Update blocks' sizes and positions.
	for( i = 0; i < m_blocks.size(); ++i ) 
    {
		srcblk = m_blocks[i];

		srcblk->SetResized( false );
		srcblk->SetPlaced( true );

		if( srcblk->GetFixed() ) 
        {
			// Sanity checking...

            // We allow fixed block resizing...
            if( std::fabs( columnPrimal[ LIMIT_DXI_LO + i ] ) != 0 ) {
                std::cout << "Warning; fixed block moved; "
                     << columnPrimal[ LIMIT_DXI_LO + i ] << " vs. zero." 
                     << std::endl;
                std::cout << "LP solution numbers: "
                     << columnPrimal[ LIMIT_DXI_LO + i ] << ", "
                     << columnPrimal[ LIMIT_XIP_LO + i ] << ", "
                     << columnPrimal[ LIMIT_DWID_LO + i ] 
                     << std::endl;
            }
            if( srcblk->GetPos( HGraph ) != columnPrimal[ LIMIT_XIP_LO + i ] ) {
                std::cout << "Warning; fixed block " << m_blocks[i]->GetId() << " @ " << srcblk->GetPos( HGraph )
                     << " LP position not expected; "
                     << columnPrimal[ LIMIT_XIP_LO + i ]  
                     << " vs. " 
                     << srcblk->GetPos( HGraph ) 
                     << ", orig: " << srcblk->GetOrigPos( HGraph )
                     << std::endl;
                std::cout << "LP solution numbers: "
                     << columnPrimal[ LIMIT_DXI_LO + i ] 
                     << " (" << build.getColumnLower( LIMIT_DXI_LO+i ) << ",  " << build.getColumnUpper( LIMIT_DXI_LO+i ) << ")"
                     << ", "
                     << columnPrimal[ LIMIT_XIP_LO + i ]
                     << " (" << build.getColumnLower( LIMIT_XIP_LO+i ) << ",  " << build.getColumnUpper( LIMIT_XIP_LO+i ) << ")"
                     << ", "
                     << columnPrimal[ LIMIT_DWID_LO + i ] 
                     << " (" << build.getColumnLower( LIMIT_DWID_LO+i ) << ",  " << build.getColumnUpper( LIMIT_DWID_LO+i ) << ")"
                     << std::endl;
            }
            if( srcblk->GetPos( HGraph  ) != srcblk->GetOrigPos( HGraph ) ) {
                std::cout << "Warning: fixed block not at original position; " 
                     << srcblk->GetPos( HGraph ) << " vs. " << srcblk->GetOrigPos( HGraph ) << std::endl;
                std::cout << "LP solution numbers: "
                     << columnPrimal[ LIMIT_DXI_LO + i ] << ", "
                     << columnPrimal[ LIMIT_XIP_LO + i ] << ", "
                     << columnPrimal[ LIMIT_DWID_LO + i ] 
                     << std::endl;
            }

			if( doResize )
            {
                // Fixed block resizing is sometimes allowed...
            }
		}
        else 
        {
			srcblk->GetPos(HGraph) = columnPrimal[ LIMIT_XIP_LO + i ];

			if( doResize ) 
            {
				if( columnPrimal[LIMIT_DWID_LO + i] <= 1e-3 ) 
                {
					;
				} 
                else 
                {
					srcblk->GetDim(HGraph) -= columnPrimal[ LIMIT_DWID_LO + i ];
					srcblk->SetResized( true );
					srcblk->SetPlaced( false );
				}
			}
		}

		if( !onlySolveForX ) 
        {
			if( srcblk->GetFixed() ) 
            {
                if( std::fabs( columnPrimal[ LIMIT_DYI_LO + i ] ) != 0 ) {
                    std::cout << "Warning; fixed block moved;  "
                         << columnPrimal[ LIMIT_DXI_LO + i ] << " vs. zero." << std::endl;
                    std::cout << "LP solution numbers: "
                         << columnPrimal[ LIMIT_DYI_LO + i ] << ", "
                         << columnPrimal[ LIMIT_YIP_LO + i ] << ", "
                         << columnPrimal[ LIMIT_DHEI_LO + i ] 
                         << std::endl;
                }
                if( srcblk->GetPos( VGraph ) != columnPrimal[ LIMIT_YIP_LO + i ] ) {
                    std::cout << "Warning; fixed block " << m_blocks[i]->GetId() << " @ " << srcblk->GetPos( VGraph )
                         << " LP position not expected; "
                         << columnPrimal[ LIMIT_YIP_LO + i ]  
                         << " vs. " 
                         << srcblk->GetPos( VGraph ) 
                         << std::endl;
                    std::cout << "LP solution numbers: "
                         << columnPrimal[ LIMIT_DYI_LO + i ] << ", "
                         << columnPrimal[ LIMIT_YIP_LO + i ] << ", "
                         << columnPrimal[ LIMIT_DHEI_LO + i ] 
                         << std::endl;
                }
                if( srcblk->GetPos( VGraph  ) != srcblk->GetOrigPos( VGraph ) ) {
                    std::cout << "Warning: fixed block not at original position; " 
                         << srcblk->GetPos( VGraph ) << " vs. " << srcblk->GetOrigPos( VGraph ) << std::endl;
                    std::cout << "LP solution numbers: "
                         << columnPrimal[ LIMIT_DYI_LO + i ] << ", "
                         << columnPrimal[ LIMIT_YIP_LO + i ] << ", "
                         << columnPrimal[ LIMIT_DHEI_LO + i ] 
                         << std::endl;
                }

				if( doResize )
                {
                }
			} else
            {
				srcblk->GetPos(VGraph) = columnPrimal[ LIMIT_YIP_LO + i ];

				if( doResize ) 
                {
					if( columnPrimal[LIMIT_DHEI_LO + i] <= 1e-3 ) 
                    {
						;
					} 
                    else 
                    {
						srcblk->GetDim(VGraph) -= columnPrimal[ LIMIT_DHEI_LO + i ];
						srcblk->SetResized( true );
						srcblk->SetPlaced( false );
					}
				}
			}
		}
	}
	return true;
}
**/


void MinMovementFloorplanner::transitiveClosure( std::vector<std::vector<bool> > &matrix )
{
    // Computes the transitive closure on the specified matrix.

    const int size = matrix[0].size();
    std::vector<std::vector<bool> > adj = matrix; // Copy the matrix.
	std::vector<int> pre( size, -1 );

    for( int i = 0; i < size; i++ ) {
        std::fill( matrix[i].begin(), matrix[i].end(), false );
	}

	int count = 0;
	for( int i = 0; i < size; i++ ) {
		if( pre[i] == -1 ) {
            transitiveClosureInner( matrix, adj, i, pre, count );
		}
	}
}


void MinMovementFloorplanner::transitiveClosureInner( 
		std::vector<std::vector<bool> > &matrix,
		std::vector<std::vector<bool> > &adj,
		int v, std::vector<int> &pre, int &count )
{
    const int size = adj[0].size();

	unsigned 	u, i;

	pre[v] = count++;

    for( int u = 0; u < size; u++ ) {
		if( adj[v][u] ) {
			matrix[v][u] = true;

			if( pre[u] > pre[v] ) {
                continue;
            }

			if( pre[u] == -1 ) {
				transitiveClosureInner( matrix, adj, u, pre, count );
			}

			for( int i = 0; i < size; ++i ) {
				if( matrix[u][i] == 1 ) {
					matrix[v][i] = 1;
				}
			}
		}
	}
}




bool MinMovementFloorplanner::clump( double& amtMoved, double& amtResized )
{
    // Assuming that a horizontal constraint graph exists, performs clumping in the
    // X-direction to remove overlap.  
    //
    // XXX: Wrote this code to replace the LP solver code.  No reason, other than 
    // the fact that the LP solver might not be installed/available.  Also, this
    // is a bit of a hack.  In order to handle segment boundaries as well as other
    // fixed objects, I just assign them an extremely large weight.  Presumably,
    // this will prevent them from moving.  Finally, there could be a problem if
    // there is a negative slack which means we cannot resolve the overlap.  We
    // can still clump, but the result will not be feasible.  Alternatively, we
    // could resize a few blocks until the slack becomes >= 0.  This is something
    // to think about...

    double singleRowHeight = m_arch->m_rows[0]->getH();
    double siteWidth = m_arch->m_rows[0]->m_siteWidth;
    amtResized = 0.;
    amtMoved = 0.;

    computeSlack( HGraph );
    std::cout << "Clumping using constraint graph, "
        << "Horizontal slack is " << m_worstSlack[HGraph] << std::endl;

    // If worst slack is +ve, then this routine should succeed.
    bool retval = (m_worstSlack[HGraph] >= -1.0e-3) ? true : false;

    // Hmmm... If we have negative slack, this code is going to push cells 
    // off the edge.  I wonder if it is worthwhile to look at the single 
    // height cells and reduce their widths artificially in order to get the
    // slack to zero.  This will result in overlap, but at least everything
    // should be within the region.
    if( m_worstSlack[HGraph] <= -1.0e-3 )
    {
        // This resizing can be slow....
        std::cout << "Shrinking single height cells to reduce slack." << std::endl;

        std::set<Block*> resized;
        std::vector<Block*> candidates;
        int noimp = 0;
        int pass = 0;
        for(;;)
        {
            ++pass;
            std::cout << "Pass " << pass << ", Worst slack is now " << m_worstSlack[HGraph] << std::endl; 

            if( m_worstSlack[HGraph] >= 0.0 || (noimp >= 3 ) )
            {
                break;
            }

            double oldSlack = m_worstSlack[HGraph];

            candidates.clear();
            for( size_t i = 0; i < m_blocks.size(); i++ )
            {
                Block* bptr = m_blocks[i];
		        Slack* bslk = bptr->GetSlack( HGraph );
                if( bslk->ComputeSlack() < 0. )
                {
                    int spanned = (int)(bptr->GetDim( VGraph ) / singleRowHeight + 0.5);
                    if( spanned == 1 )
                    {
                        candidates.push_back( bptr );
                    }
                }
            }
		    Utility::random_shuffle( candidates.begin(), candidates.end(), m_rng );

            for( size_t i = 0; i < candidates.size() && m_worstSlack[HGraph] <= -1.0e-3 ; i++ )
            {
                Block* bptr = candidates[i];
                bptr->GetDim( HGraph ) = std::max( 0.0, bptr->GetDim( HGraph ) - siteWidth );
                resized.insert( bptr );

                // The following is going to slow things down so much since the slack 
                // is not done incrementally...  XXX: FIX ME...
                computeSlack( HGraph );
                if( m_worstSlack[HGraph] >= 0.0 )
                {
                    break;
                }
            }

            if( m_worstSlack[HGraph] <= oldSlack )
            {
                // Nothing is getting better...
                ++noimp;
            }
        }
        std::cout << "Blocks resized is " << resized.size() << ", "
            << "Slack is now " << m_worstSlack[HGraph] << std::endl;
        for( size_t i = 0; i < m_blocks.size(); i++ )
        {
            Block* bptr = m_blocks[i];
            if( std::fabs( bptr->GetDim( HGraph ) - bptr->GetOrigDim( HGraph ) ) > 1.0e-3 )
            {
                std::cout << "Block " << i << ", Resized from " 
                    << bptr->GetOrigDim( HGraph ) << " to "
                    << bptr->GetDim( HGraph )
                    << std::endl;
            }
        }
    }
    for( size_t i = 0; i < m_blocks.size(); i++ )
    {
        Block* bptr = m_blocks[i];
        double diff = std::fabs( bptr->GetDim( HGraph ) - bptr->GetOrigDim( HGraph ) );
        amtResized += diff;
    }

    // Sort the graph since we need to process blocks in proper order left -> right.
    m_order.erase( m_order.begin(), m_order.end() );
    m_graph[HGraph]->TopologicalSort( m_order );

    // Allocate a clump for each block.
    m_clumps.resize( m_blocks.size() );
    for( size_t i = 0; i < m_blocks.size(); i++ )
    {
        m_clumps[i] = new Clump;
    }


    //drawBlocks( "test", m_blocks );

    // Put each block into its own clump.
    for( size_t i = 0; i < m_blocks.size(); i++ )
    {
        Block* bptr = m_blocks[i];

        Clump* cptr = m_clumps[i];


        bptr->m_offset  = 0.;
        bptr->m_weight  = (bptr->GetFixed() ? 1.0e8 : 1.0);
        bptr->m_target  = bptr->GetOrigPos(HGraph) - 0.5 * bptr->GetOrigDim(HGraph);
        //bptr->m_dim     = bptr->GetOrigDim(HGraph);
        bptr->m_dim     = bptr->GetDim(HGraph); // To account for potential shrinking.
        bptr->m_clump   = cptr;

        cptr->m_id      = i;
        cptr->m_weight  = bptr->m_weight;
        cptr->m_wposn   = bptr->m_weight * (bptr->m_target - bptr->m_offset);
        cptr->m_posn    = cptr->m_wposn / cptr->m_weight;
        cptr->m_blocks.clear();
        cptr->m_blocks.push_back( bptr->GetId() );

        //std::cout << "Block " << bptr->GetId() << ", "
        //    << "Target: " << bptr->m_target << ", "
        //    << "Cluster posn: " << cptr->m_posn << std::endl;
    }

    //drawBlocks( "bclump", m_blocks );

    // Clump the blocks in order.
    for( size_t i = 0; i < m_order.size(); i++ )
    {
        Block* bptr = m_blocks[ m_order[i] ];
        mergeLeft( bptr->m_clump, HGraph );
    }

    // Debug.
    for( size_t i = 0; i < m_clumps.size(); i++ )
    {
        Clump* cptr = m_clumps[i];
        if( cptr->m_blocks.size() != 0 )
        {
            //std::cout << "Clump " << i << ", has " << cptr->m_blocks.size() << std::endl;
            for( size_t j = 0; j < cptr->m_blocks.size(); j++ )
            {
                int id = cptr->m_blocks[j];

                Block* bptr = m_blocks[id];
                double x = cptr->m_posn + bptr->m_offset + 0.5 * bptr->m_dim;
                double t = bptr->m_target + 0.5 * bptr->m_dim;

                //std::cout << "Block " << bptr->GetId() << " "
                //    << "@ (" << bptr->GetPos( HGraph ) << "," << bptr->GetPos( VGraph ) << ")" << ", "
                //    << "Width " << bptr->GetDim( HGraph ) << ", "
                //    << x << "," << t << std::endl;
            }
        }
    }

    // Extract new positions and compute cost.
    amtMoved = 0.;
    for( size_t i = 0; i < m_order.size(); i++ )
    {
        Block* bptr = m_blocks[ m_order[i] ];

        Clump* cptr = bptr->m_clump;

        double x = cptr->m_posn + bptr->m_offset + 0.5 * bptr->m_dim;
        double t = bptr->m_target + 0.5 * bptr->m_dim;
        double d = x - t;

        if( !bptr->GetFixed() )
        {
            //bptr->GetDim( HGraph ) = bptr->GetOrigDim( HGraph );
            bptr->GetPos( HGraph ) = x;

            // Only return the displacement of the non-fixed stuff.
            amtMoved += bptr->m_weight * (d * d);
        }
        else
        {
            // If a fixed block seemingly has moved, then we have failed.
            ;
        }
    }

    //drawBlocks( "aclump", m_blocks );
    
    // Cleanup.
    for( size_t i = 0; i < m_clumps.size(); i++ )
    {
        delete m_clumps[i];
    }
    m_clumps.clear();

    return retval;
}

void MinMovementFloorplanner::mergeLeft( Clump* r, WhichGraph wg )
{
    // Find most violated constraint and merge clumps if required.

    double dist = 0.;
    Clump* l = nullptr;
    while( getMostViolatedEdge( r, wg, l, dist ) == true ) 
    {
        // Yes, we need to merge clump r into clump l which, in turnm could require
        // more merges.

        // Move blocks from r to l.  
        for( size_t i = 0; i < r->m_blocks.size(); i++ ) 
        {
            Block* bptr = m_blocks[ r->m_blocks[i] ];
            bptr->m_offset += dist;
            bptr->m_clump = l; // Block now in this clump.
            l->m_blocks.push_back( r->m_blocks[i] ); // Block now in this clump.
        }
        // Can remove the blocks in r, I believe.
        r->m_blocks.clear();

        // Update position of clump l.
        l->m_wposn += r->m_wposn - dist * r->m_weight;
        l->m_weight += r->m_weight;
        l->m_posn = l->m_wposn / l->m_weight;

        // Since clump l changed position, we need to make it the new right clump
        // and see if there are more merges to the left.
        r = l;
    }
}

bool MinMovementFloorplanner::getMostViolatedEdge(
        Clump* r, WhichGraph wg, Clump*& l, double& dist )
{
    l = nullptr;
    double worst_v = std::numeric_limits<double>::max();
    dist = std::numeric_limits<double>::max();

    for( size_t i = 0; i < r->m_blocks.size(); i++ ) 
    {
        int dstidx = r->m_blocks[i];
        Block* dstblk = m_blocks[dstidx];

        for( int j = 0; j < m_graph[wg]->GetNumInEdges( dstidx ); j++ ) 
        {
            int srcidx = m_graph[wg]->GetInEdgeSrc( dstidx, j );
            Block* srcblk = m_blocks[srcidx];

            Clump* t = srcblk->m_clump;
            
            if( t == r ) {
                continue; // Internal edge...
            }

            double pdst = r->m_posn + dstblk->m_offset;
            double psrc = t->m_posn + srcblk->m_offset;
            double gap = srcblk->m_dim;

            double v = pdst - psrc - gap;
            if( v >= -1.0e-3 )
            {
                continue;
            }
            else 
            {
                if( v < worst_v ) 
                {
                    l = t;
                    worst_v = v;
                    dist = srcblk->m_offset + gap - dstblk->m_offset;
                }
            }
        }
    }

    return (l != 0) ? true : false;
}

bool MinMovementFloorplanner::test( DetailedMgr& mgr, bool onlyMulti ) 
{
    // !!! Requires segments for the region to be populated !!!
    mgr.resortSegments();
    for( size_t regId = 0; regId < m_arch->m_regions.size(); regId++ )
    {
        if( !test( mgr, regId, onlyMulti ) )
        {
            return false;
        }
    }
    return true;
}

bool MinMovementFloorplanner::test( DetailedMgr& mgr, int regId, bool onlyMulti ) 
{
    // !!! Requires segments for the region to be populated !!!

    // Simply builds a constraint graph for the corresponding region and then computes
    // the slack which is an indication of whether or not the region can be made 
    // legal by shifting cells in the X-direction.  
    // NOTES: 
    // - Does not account for gaps.
    // - Does not account for site alignment.

    mgr.resortSegments();

    m_net = mgr.getNetwork();
    m_arch = mgr.getArchitecture();

    double x, y, w, h;
    int l, r;

    // Collect all cells in the region.
    std::vector<Node*> cells;
    for( size_t i = 2; i < mgr.m_multiHeightCells.size(); i++ )
    {
        for( size_t n = 0; n < mgr.m_multiHeightCells[i].size(); n++ )
        {
            Node* ndi = mgr.m_multiHeightCells[i][n];
            if( ndi->getRegionId() != regId )
            {
                continue;
            }
            cells.push_back( ndi );
        }
    }
    if( !onlyMulti )
    {
        for( size_t i = 0; i < mgr.m_singleHeightCells.size(); i++ )
        {
            Node* ndi = mgr.m_singleHeightCells[i];
            if( ndi->getRegionId() != regId )
            {
                continue;
            }
            cells.push_back( ndi );
        }
    }

    if( cells.size() == 0 )
    {
        return true;
    }

    // Figure out the segments in which cells are found. 
    std::set<DetailedSeg*> segs;
    for( size_t i = 0; i < cells.size(); i++ )
    {
        Node* ndi = cells[i];
        segs.insert( mgr.m_reverseCellToSegs[ndi->getId()].begin(), mgr.m_reverseCellToSegs[ndi->getId()].end() );
    }

    // Create blocks for the cells.
    std::map<DetailedSeg*,Block*> segToRightBlockMap;
    std::map<DetailedSeg*,Block*> segToLeftBlockMap;
    std::map<Node*,Block*> nodeToBlockMap;
    std::vector<Block*> blocks;
    int count = 0;
    Block* bptr;
    for( size_t i = 0; i < cells.size(); i++ )
    {
        Node* ndi = cells[i];

        w = ndi->getWidth() ;
        h = ndi->getHeight();
        x = ndi->getX();
        y = ndi->getY();

        bptr = new Block;
        bptr->GetId() = count;
        bptr->SetNode( ndi );

        bptr->GetOrigDim( HGraph ) = w;
        bptr->GetOrigDim( VGraph ) = h;
        bptr->GetOrigPos( HGraph ) = x;
        bptr->GetOrigPos( VGraph ) = y;

        bptr->GetDim( HGraph ) = w;
        bptr->GetDim( VGraph ) = h;
        bptr->GetPos( HGraph ) = x;
        bptr->GetPos( VGraph ) = y;

        bptr->SetFixed( false );

        blocks.push_back( bptr );
        ++count;

        nodeToBlockMap[ndi] = bptr;
    }

    // Create fixed blocks for segments.  This includes all segments for the region
    // and any other segments in which an involved cell might be found.

    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        int rowId = segPtr->m_rowId;

        w = 0.;
        h = 0.;

        // Left.
        x = segPtr->m_xmin;
        y = m_arch->m_rows[rowId]->getY() + 0.5 * m_arch->m_rows[rowId]->getH();

        bptr = new Block;
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = w;
        bptr->GetOrigDim( VGraph ) = h;
        bptr->GetOrigPos( HGraph ) = x;
        bptr->GetOrigPos( VGraph ) = y;

        bptr->GetDim( HGraph ) = w;
        bptr->GetDim( VGraph ) = h;
        bptr->GetPos( HGraph ) = x;
        bptr->GetPos( VGraph ) = y;

        bptr->SetFixed( true );

        blocks.push_back( bptr );
        ++count;

        segToLeftBlockMap[segPtr] = bptr;

        // Right.
        x = segPtr->m_xmax;
        y = m_arch->m_rows[rowId]->getY() + 0.5 * m_arch->m_rows[rowId]->getH();

        bptr = new Block;
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = w;
        bptr->GetOrigDim( VGraph ) = h;
        bptr->GetOrigPos( HGraph ) = x;
        bptr->GetOrigPos( VGraph ) = y;

        bptr->GetDim( HGraph ) = w;
        bptr->GetDim( VGraph ) = h;
        bptr->GetPos( HGraph ) = x;
        bptr->GetPos( VGraph ) = y;

        bptr->SetFixed( true );

        blocks.push_back( bptr );
        ++count;

        segToRightBlockMap[segPtr] = bptr;
    }

    // Make sure required segments are sorted.
    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        mgr.resortSegment( segPtr );
    }

    // Create the horizontal constraint graph.
    m_Tmin[HGraph] = m_arch->m_xmin;
    m_Tmax[HGraph] = m_arch->m_xmax;
    m_Tmin[VGraph] = m_arch->m_ymin;
    m_Tmax[VGraph] = m_arch->m_ymax;

    // Turn the blocks into a member variable.
    m_blocks = blocks;

    // Compute a tolerance for certain checks.
    initializeEpsilon();

    // Construct HGraph using sorted segments.
    m_graph[HGraph]->Clear();
    m_graph[VGraph]->Clear();
    m_graph[HGraph]->Resize( m_blocks.size() );

    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        int segId = segPtr->m_segId;

        std::map<DetailedSeg*,Block*>::iterator it_l = segToLeftBlockMap.find( segPtr );
        if( segToLeftBlockMap.end() == it_l )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
        std::map<DetailedSeg*,Block*>::iterator it_r = segToRightBlockMap.find( segPtr );
        if( segToRightBlockMap.end() == it_r )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }

        int n = mgr.m_cellsInSeg[segId].size();
        if( n == 0 )
        {
            continue;
        }

        Block* blk_i = it_l->second;
        Block* blk_j = 0;
        for( size_t j = 0; j < mgr.m_cellsInSeg[segId].size(); j++ )
        {
            // Might have cells in this segment that are not part of the region.
            Node* ndj = mgr.m_cellsInSeg[segId][j];
            std::map<Node*,Block*>::iterator it_n = nodeToBlockMap.find( ndj );
            if( nodeToBlockMap.end() == it_n )
            {
                if( ndj->getRegionId() == regId && !onlyMulti )
                {
                    std::cout << "Error." << std::endl;
                    exit(-1);
                }
                continue;
            }
            blk_j = it_n->second;

            // Need an edge from blk_i -> blk_j.
            if( !m_graph[HGraph]->DoesEdgeExist( blk_i->GetId(), blk_j->GetId() ) ) 
            { 
                m_graph[HGraph]->AddEdge( blk_i->GetId(), blk_j->GetId() ); 
            }

            blk_i = blk_j;
        }
        blk_j = it_r->second;
        if( !m_graph[HGraph]->DoesEdgeExist( blk_i->GetId(), blk_j->GetId() ) ) 
        { 
            m_graph[HGraph]->AddEdge( blk_i->GetId(), blk_j->GetId() ); 
        }
    }
 
    computeSlack( HGraph );

    // Cleanup.
    for( size_t i = 0; i < blocks.size(); i++ )
    {
        delete blocks[i];
    }
    blocks.clear();
    cells.clear();
    segToRightBlockMap.clear();
    segToLeftBlockMap.clear();
    nodeToBlockMap.clear();

    if( m_worstSlack[HGraph] >= 0. - 1.0e-3 )
    {
        return true;
    }
    std::cout << "Testing, slack is " << m_worstSlack[HGraph] << std::endl;
    return false;
}

double MinMovementFloorplanner::repair_slack( 
            std::vector<std::vector<std::pair<int,int> > >& gr_out,
            std::vector<std::vector<std::pair<int,int> > >& gr_inp,
            std::vector<double>& Tarr, std::vector<double>& Treq 
            )
{
    std::vector<int> order;
    
    repair_order( gr_out, gr_inp, order );

    double minTreq = m_arch->m_xmax;
    double maxTarr = m_arch->m_xmin;
    double worst = std::numeric_limits<double>::max();
    for( size_t i = 0; i < order.size(); i++ )
    {
        int j = order[i];

        if( m_blocks[j]->GetFixed() )
        {
            Tarr[j] = m_blocks[j]->GetPos( HGraph ) - 0.5 * m_blocks[j]->GetDim( HGraph );
            Treq[j] = m_blocks[j]->GetPos( HGraph ) - 0.5 * m_blocks[j]->GetDim( HGraph );
        }
        else
        {
            Tarr[j] = m_arch->m_xmin;
            Treq[j] = m_arch->m_xmax - m_blocks[j]->GetDim( HGraph ) ;
        }
    }
    for( size_t i = 0; i < order.size(); i++ )
    {
        int j = order[i];
        for( size_t k = 0; k < gr_out[j].size(); k++ )
        {
            int l = gr_out[j][k].first;

            // j -> l.
            if( m_blocks[l]->GetFixed() )
            {
                continue;
            }
            Tarr[l] = std::max( Tarr[l], Tarr[j] + m_blocks[j]->GetDim( HGraph ) );
        }   
        maxTarr = std::max( maxTarr, Tarr[j] );
    }
    for( size_t i = order.size(); i > 0; ) 
    {
        int j = order[--i];
        if( m_blocks[j]->GetFixed() )
        {
            continue;
        }
        for( size_t k = 0; k < gr_out[j].size(); k++ )
        {
            int l = gr_out[j][k].first;

            // j -> k.
            Treq[j] = std::min( Treq[j], Treq[l] - m_blocks[j]->GetDim( HGraph ) );
        }
        minTreq = std::min( minTreq, Treq[j] );

        worst = std::min( worst, Treq[j] - Tarr[j] );
    }
    return worst;
}

void MinMovementFloorplanner::repair_order(
            std::vector<std::vector<std::pair<int,int> > >& gr_out,
            std::vector<std::vector<std::pair<int,int> > >& gr_inp,
            std::vector<int>& order
            )
{
    // Topologically order the local graph.

    order.erase( order.begin(), order.end() );

    std::vector<int> indeg;
    std::vector<int> visit;
    indeg.resize( m_blocks.size() );
    visit.resize( m_blocks.size() );
    std::fill( indeg.begin(), indeg.end(), 0 );
    std::fill( visit.begin(), visit.end(), 0 );
    for( size_t b = 0; b < m_blocks.size(); b++ )
    {
        int i = m_blocks[b]->GetId();

        for( size_t k = 0; k < gr_out[i].size(); k++ )
        {
            int j = gr_out[i][k].first;

            ++indeg[j];
        }
    }
    for( size_t b = 0; b < m_blocks.size(); b++ )
    {
        int i = m_blocks[b]->GetId();
        if( indeg[i] == 0 )
        {
            order.push_back( i );
            visit[i] = 1;
        }
    }
    for( size_t i = 0; i < order.size(); i++ )
    {
        int j = order[i];
        for( size_t k = 0; k < gr_out[j].size(); k++ )
        {
            int l = gr_out[j][k].first;

            // j -> l.
            --indeg[l];
            if( indeg[l] < 0 )
            {
                std::cout << "Error." << std::endl;
                exit(-1);
            }
            if( indeg[l] == 0 )
            {
                if( visit[l] == 1 )
                {
                    std::cout << "Error." << std::endl;
                    exit(-1);
                }
                visit[l] = 1;
                order.push_back( l );
            }
        }
    }
    if( order.size() != m_blocks.size() )
    {
        std::cout << "Error." << std::endl;
        exit(-1);
    }
//    std::cout << "Ordered" << std::endl;
}

void add_edge( std::vector<std::vector<std::pair<int,int> > >& gr_out,
               std::vector<std::vector<std::pair<int,int> > >& gr_inp,
               int src, int snk, int seg )
{
    std::vector<std::pair<int,int> >::iterator it;

    it = std::find( gr_out[src].begin(), gr_out[src].end(), std::make_pair( snk, seg ) );
    if( gr_out[src].end() != it )
    {
        // Edge already outgoing from src -> snk.  Confirm it is also incoming.
        it = std::find( gr_inp[snk].begin(), gr_inp[snk].end(), std::make_pair( src, seg ) );
        if( gr_inp[snk].end() == it )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
        return;
    }
    gr_out[src].push_back( std::make_pair( snk, seg ) );
    gr_inp[snk].push_back( std::make_pair( src, seg ) );
}

void rem_edge( std::vector<std::vector<std::pair<int,int> > >& gr_out,
               std::vector<std::vector<std::pair<int,int> > >& gr_inp,
               int src, int snk, int seg )
{
    std::vector<std::pair<int,int> >::iterator it;

    it = std::find( gr_out[src].begin(), gr_out[src].end(), std::make_pair( snk, seg ) );
    if( gr_out[src].end() == it )
    {
        // Expected edge not present.
        std::cout << "Error." << std::endl;
        exit(-1);
    }
    else
    {
        gr_out[src].erase( it );
    }

    it = std::find( gr_inp[snk].begin(), gr_inp[snk].end(), std::make_pair( src, seg ) );
    if( gr_inp[snk].end() == it )
    {
        // Expected edge not present.
        std::cout << "Error." << std::endl;
        exit(-1);
    }
    else
    {
        gr_inp[snk].erase( it );
    }
}

bool is_edge_present( std::vector<std::vector<std::pair<int,int> > >& gr_out,
                      std::vector<std::vector<std::pair<int,int> > >& gr_inp,
                      int src, int snk, int seg )
{
    std::vector<std::pair<int,int> >::iterator it;
    it = std::find( gr_out[src].begin(), gr_out[src].end(), std::make_pair( snk, seg ) );
    if( gr_out[src].end() == it )
    {
        // Not outgoing.
        it = std::find( gr_inp[snk].begin(), gr_inp[snk].end(), std::make_pair( src, seg ) );
        if( gr_inp[snk].end() == it )
        {
            // Not incoming.
            return false;
        }
        // Incoming, but not outgoing?  This is an error.
        std::cout << "Error." << std::endl;
        exit(-1);
    }
    return true;
}

struct compareCandidatesBySlack
{
    inline bool operator() ( const std::pair<int,double>& p, const std::pair<int,double>& q ) const
    {
        if( p.second == q.second )
            return p.first < q.first;
        return p.second < q.second;
    }
};

bool MinMovementFloorplanner::repair( DetailedMgr& mgr, int regId ) 
{
    // !!! Requires segments for the region to be populated !!!

    // Tries to move _only_ single height cells (which it can do with only the
    // horizontal constraint graph) to resolve negative slack issues.

    std::cout << "Repairing cells in region " << regId << " to eliminate overlap." << std::endl;

    m_net = mgr.getNetwork();
    m_arch = mgr.getArchitecture();

    double singleRowHeight = m_arch->m_rows[0]->getH();

    std::map<Block*,DetailedSeg*> blockToSegMap;
    std::map<Block*,Node*> blockToNodeMap;
    std::map<Node*,Block*> nodeToBlockMap;
    std::map<DetailedSeg*,Block*> segToLeftBlockMap;
    std::map<DetailedSeg*,Block*> segToRightBlockMap;
    std::vector<Block*> blocks;
    std::vector<Node*> cells;
    std::set<DetailedSeg*> segs;
    Block* bptr;
    double x, y, w, h;
    int count = 0;
    int l, r;
    std::map<Block*,DetailedSeg*>::iterator it_seg;
    std::map<Block*,Node*>::iterator it_cell;

    bool retval = true;

    // Easy to make things go region by region.  Just loop over the regions.
    // Then, we only consider multi-height cells with the proper region and
    // segments with the proper region.


    blockToNodeMap.clear();
    blockToSegMap.clear();
    nodeToBlockMap.clear();
    cells.clear();
    blocks.clear();
    segs.clear();

    // Collect all cells in the region.
    for( size_t i = 2; i < mgr.m_multiHeightCells.size(); i++ )
    {
        for( size_t n = 0; n < mgr.m_multiHeightCells[i].size(); n++ )
        {
            Node* ndi = mgr.m_multiHeightCells[i][n];
            if( ndi->getRegionId() != regId )
            {
                continue;
            }
            cells.push_back( ndi );
        }
    }
    for( size_t i = 0; i < mgr.m_singleHeightCells.size(); i++ )
    {
        Node* ndi = mgr.m_singleHeightCells[i];
        if( ndi->getRegionId() != regId )
        {
            continue;
        }
        cells.push_back( ndi );
    }

    if( cells.size() == 0 )
    {
        return true;
    }

    // Figure out the segments in which cells are found. 
    for( size_t i = 0; i < cells.size(); i++ )
    {
        Node* ndi = cells[i];
        segs.insert( mgr.m_reverseCellToSegs[ndi->getId()].begin(), mgr.m_reverseCellToSegs[ndi->getId()].end() );
    }

    // Create blocks for the cells.
    count = 0;
    for( size_t i = 0; i < cells.size(); i++ )
    {
        Node* ndi = cells[i];

        w = ndi->getWidth() ;
        h = ndi->getHeight();
        x = ndi->getX();
        y = ndi->getY();

        bptr = new Block;
        bptr->GetId() = count;
        bptr->SetNode( ndi );

        bptr->GetOrigDim( HGraph ) = w;
        bptr->GetOrigDim( VGraph ) = h;
        bptr->GetOrigPos( HGraph ) = x;
        bptr->GetOrigPos( VGraph ) = y;

        bptr->GetDim( HGraph ) = w;
        bptr->GetDim( VGraph ) = h;
        bptr->GetPos( HGraph ) = x;
        bptr->GetPos( VGraph ) = y;

        bptr->SetFixed( false );

        blocks.push_back( bptr );
        ++count;

        nodeToBlockMap[ndi] = bptr;
        blockToNodeMap[bptr] = ndi;
    }

    // Create fixed blocks for segments.  This includes all segments for the region
    // and any other segments in which an involved cell might be found.

    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        int rowId = segPtr->m_rowId;

        w = 0.;
        h = 0.;

        // Left.
        x = segPtr->m_xmin;
        y = m_arch->m_rows[rowId]->getY() + 0.5 * m_arch->m_rows[rowId]->getH();

        bptr = new Block;
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = w;
        bptr->GetOrigDim( VGraph ) = h;
        bptr->GetOrigPos( HGraph ) = x;
        bptr->GetOrigPos( VGraph ) = y;

        bptr->GetDim( HGraph ) = w;
        bptr->GetDim( VGraph ) = h;
        bptr->GetPos( HGraph ) = x;
        bptr->GetPos( VGraph ) = y;

        bptr->SetFixed( true );

        blocks.push_back( bptr );
        ++count;

        blockToSegMap[bptr] = segPtr;
        segToLeftBlockMap[segPtr] = bptr;

        // Right.
        x = segPtr->m_xmax;
        y = m_arch->m_rows[rowId]->getY() + 0.5 * m_arch->m_rows[rowId]->getH();

        bptr = new Block;
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = w;
        bptr->GetOrigDim( VGraph ) = h;
        bptr->GetOrigPos( HGraph ) = x;
        bptr->GetOrigPos( VGraph ) = y;

        bptr->GetDim( HGraph ) = w;
        bptr->GetDim( VGraph ) = h;
        bptr->GetPos( HGraph ) = x;
        bptr->GetPos( VGraph ) = y;

        bptr->SetFixed( true );

        blocks.push_back( bptr );
        ++count;

        blockToSegMap[bptr] = segPtr;
        segToRightBlockMap[segPtr] = bptr;
    }

    // Make sure required segments are sorted.
    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        mgr.resortSegment( segPtr );
    }

    // Create the horizontal constraint graph.
    std::cout << "Building local graph." << std::endl;

    // Turn the blocks into a member variable.
    m_blocks = blocks;

    std::vector<std::vector<std::pair<int,int> > > gr_out;
    std::vector<std::vector<std::pair<int,int> > > gr_inp;
    gr_out.resize( blocks.size() );
    gr_inp.resize( blocks.size() );

    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        int segId = segPtr->m_segId;

        std::map<DetailedSeg*,Block*>::iterator it_l = segToLeftBlockMap.find( segPtr );
        if( segToLeftBlockMap.end() == it_l )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
        std::map<DetailedSeg*,Block*>::iterator it_r = segToRightBlockMap.find( segPtr );
        if( segToRightBlockMap.end() == it_r )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }

        int n = mgr.m_cellsInSeg[segId].size();
        if( n == 0 )
        {
            continue;
        }

        Block* blk_i = it_l->second;
        Block* blk_j = 0;
        for( size_t j = 0; j < mgr.m_cellsInSeg[segId].size(); j++ )
        {
            // Might have cells in this segment that are not part of the region.
            Node* ndj = mgr.m_cellsInSeg[segId][j];
            std::map<Node*,Block*>::iterator it_n = nodeToBlockMap.find( ndj );
            if( nodeToBlockMap.end() == it_n )
            {
                if( ndj->getRegionId() == regId )
                {
                    std::cout << "Error." << std::endl;
                    exit(-1);
                }
                continue;
            }
            blk_j = it_n->second;

            add_edge( gr_out, gr_inp, blk_i->GetId(), blk_j->GetId(), segId );
            blk_i = blk_j;
        }
        blk_j = it_r->second;
        add_edge( gr_out, gr_inp, blk_i->GetId(), blk_j->GetId(), segId );
    }

    // Scan the order and compute Tarr and Treq.
    std::vector<double> Tarr;
    std::vector<double> Treq;
    Tarr.resize( m_blocks.size() );
    Treq.resize( m_blocks.size() );

    double worst = repair_slack( gr_out, gr_inp, Tarr, Treq );
    bool done = (worst >= 0.);
    std::cout << "Worst slack is " << worst << ", Done? " << (done?"Y":"N") << std::endl;

    // Use slack to guide the movement of single height cells to resolve issues.
    std::vector<std::pair<std::pair<int,int>,int> > gaps;
    std::vector<std::pair<int,double> > candidates;
    for( int pass = 1; pass <= 10 && !done; pass++ )
    {
        // Collect all single height cells with negative slack.
        candidates.clear();
        gaps.clear();
        for( size_t s = 0; s < mgr.m_segments.size(); s++ )
        {
            DetailedSeg* oldPtr = mgr.m_segments[s];
            // If not a segment we care about, skip it.
            if( !(oldPtr->m_regId == regId || segs.end() != segs.find( oldPtr )) )
            {
                continue;
            }
            int oldSegId = oldPtr->m_segId;
            for( size_t i = 0; i < mgr.m_cellsInSeg[oldSegId].size(); i++ )
            {
                Node* ndi = mgr.m_cellsInSeg[oldSegId][i];
                std::map<Node*,Block*>::iterator it = nodeToBlockMap.find( ndi );
                if( nodeToBlockMap.end() != it )
                {
                    int spanned = (int)(ndi->getHeight() / singleRowHeight + 0.5);
                    if( spanned == 1 )
                    {
                        Block* ptri = it->second;
                        double slack = Treq[ptri->GetId()] - Tarr[ptri->GetId()];
                        if( slack < 0.0 )
                        {
                            candidates.push_back( std::make_pair(ptri->GetId(), slack) );
                        }
                    }
                }
            }
        }
        std::stable_sort( candidates.begin(), candidates.end(), compareCandidatesBySlack() );

        // Look at all blocks and determine gaps.
        for( size_t b = 0; b < blocks.size(); b++ )
        {
            Block* ptri = blocks[b];

            int i = ptri->GetId();
            for( size_t e = 0; e < gr_out[i].size(); e++ )
            {
                int j = gr_out[i][e].first;
                int s = gr_out[i][e].second;
                Block* ptrj = blocks[j];
                double space = Treq[j] - ( Tarr[i] + ptri->GetDim( HGraph ) );
                if( space > 0 )
                {
                    gaps.push_back( std::make_pair(std::make_pair(i,j),s) );
                }
            }
        }

        std::cout << "Pass " << pass << " of repair, "
            << "Options is " << candidates.size() << ", Potential gaps is " << gaps.size() << std::endl;

        if( candidates.size() == 0 || gaps.size() == 0 )
        {
            break;
        }

        // Note that, as we go, candidates might change (i.e., not have negative slack
        // anymore) and the gaps might change (as we are moving things around).
        for( size_t c = 0; c < candidates.size() && !done; c++ )
        {
            int i = candidates[c].first;

            double slack = Treq[i] - Tarr[i];
            if( slack >= 0 )
            {
                continue;
            }

            //std::cout << "Candidate is " << i << ", Slack is " << candidates[c].second << ", " << slack << std::endl;

            // Look at gaps and pick the first one.
            for( size_t g = 0; g < gaps.size() && !done; g++ )
            {
                // XXX: As we move things around, certain gaps might disappear since
                // edges are changed...
                int j = gaps[g].first.first ;
                int k = gaps[g].first.second;
                int s = gaps[g].second;
                if( !is_edge_present( gr_out, gr_inp, j, k, s ) )
                {
                    continue;
                }
                double space1 = Treq[k] - (Tarr[j] + blocks[j]->GetDim( HGraph ));

                if( space1 >= blocks[i]->GetDim( HGraph ) )
                {
                    //std::cout << "Moving " 
                    //    << i << " @ " << "[" << blocks[i]->GetPos( HGraph ) << "," << blocks[i]->GetPos( VGraph ) << "]" << ", "
                    //    << "Width is " << blocks[i]->GetDim( HGraph ) << ", "
                    //    << "Between "
                    //    << j << " @ " << "[" << blocks[j]->GetPos( HGraph ) << "," << blocks[j]->GetPos( VGraph ) << "]" << " & "
                    //    << k << " @ " << "[" << blocks[k]->GetPos( HGraph ) << "," << blocks[k]->GetPos( VGraph ) << "]" << ", "
                    //    << "Space is " << space1 << std::endl;

                    for( size_t b = 0; b < blocks.size(); b++ )
                    {
                        Block* ptrb = blocks[b];
                        if( ptrb->GetFixed() )
                        {
                            continue;
                        }
                        ptrb->GetPos( HGraph ) = Tarr[ptrb->GetId()] + 0.5 * ptrb->GetDim( HGraph );
                    }

                    // Since the candidate is a single height cell, it should have only
                    // one incoming and one outgoing edge.
                    if( gr_inp[i].size() != 1 || gr_out[i].size() != 1 || gr_inp[i][0].second != gr_out[i][0].second )
                    {
                        std::cout << "Error." << std::endl;
                        exit(-1);
                    }

                    int m = gr_inp[i][0].first ;
                    int n = gr_out[i][0].first ;
                    int p = gr_inp[i][0].second;
            
                    // Currently: m -> i -> n in p and j -> k in s.  
                    // Want: m -> n in p and j -> i -> k in s.
                    rem_edge( gr_out, gr_inp, m, i, p );
                    rem_edge( gr_out, gr_inp, i, n, p );
                    rem_edge( gr_out, gr_inp, j, k, s );
                    add_edge( gr_out, gr_inp, m, n, p );
                    add_edge( gr_out, gr_inp, j, i, s );
                    add_edge( gr_out, gr_inp, i, k, s );

                    // Move the block and the cell, including segment assignment.  
                    // Need to actually move the cell...
                    mgr.removeCellFromSegment( blocks[i]->GetNode(), p );

                    double xx = 0.5 * (blocks[j]->GetPos( HGraph ) + blocks[k]->GetPos( HGraph ) );
                    int r = mgr.m_segments[s]->m_rowId;
                    double yy = m_arch->m_rows[r]->getY() + 0.5 * blocks[i]->GetDim( VGraph );
                    blocks[i]->GetPos( HGraph ) = xx;
                    blocks[i]->GetPos( VGraph ) = yy;
                    blocks[i]->GetNode()->setX( xx );
                    blocks[i]->GetNode()->setY( yy );

                    mgr.addCellToSegment( blocks[i]->GetNode(), s );

                    // Compute the new worst slack since the constraint graph
                    // has changed. 
                    worst = repair_slack( gr_out, gr_inp, Tarr, Treq );
                    done = (worst >= 0. );

                    //std::cout << "Worst slack is " << worst << std::endl;

                    // Since we've moved the candidate, we should have resolved its
                    // negative slack issue.  Things have changed so move on to the
                    // next candidate.
                    break;
                }

                // Try a swap???
            }
        }
        std::cout << "Pass, Ending worst slack is " << worst << std::endl;
    }


    worst = repair_slack( gr_out, gr_inp, Tarr, Treq );
    done = (worst >= 0. );


    // Update positions and resort segments.
    for( size_t b = 0; b < blocks.size(); b++ )
    {
        Block* ptrb = blocks[b];
        double lx = ptrb->GetPos( HGraph ) - 0.5 * ptrb->GetDim( HGraph );
        lx = std::max( Tarr[b], std::min( Treq[b], lx ) );
        ptrb->GetPos( HGraph ) = lx + 0.5 * ptrb->GetDim( HGraph );
        if( ptrb->GetNode() )
        {
            ptrb->GetNode()->setX( ptrb->GetPos( HGraph ) );
        }
        Tarr[b] = lx;
        for( size_t e = 0; e < gr_out[b].size(); e++ )
        {
            int j = gr_out[b][e].first ;
            Tarr[j] = std::max( Tarr[j], lx );
        }
    }

    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        mgr.resortSegment( segPtr );
    }

    // Cleanup.
    for( size_t i = 0; i < blocks.size(); i++ )
    {
        delete blocks[i];
    }
    blocks.clear();

    return done;
}

bool MinMovementFloorplanner::repair1( DetailedMgr& mgr, int regId ) 
{
    // !!! Requires segments for the region to be populated !!!

    // Removes single height cells and then attempts to place all the single
    // height cells based on a slack graph.

    std::cout << "Repairing cells in region " << regId << " to eliminate overlap." << std::endl;

    m_net = mgr.getNetwork();
    m_arch = mgr.getArchitecture();

    double singleRowHeight = m_arch->m_rows[0]->getH();

    std::map<Block*,DetailedSeg*> blockToSegMap;
    std::map<Block*,Node*> blockToNodeMap;
    std::map<Node*,Block*> nodeToBlockMap;
    std::map<DetailedSeg*,Block*> segToLeftBlockMap;
    std::map<DetailedSeg*,Block*> segToRightBlockMap;
    std::vector<Block*> blocks;
    std::vector<Node*> cells;
    std::set<DetailedSeg*> segs;
    Block* bptr;
    double x, y, w, h;
    int count = 0;
    int l, r;
    std::map<Block*,DetailedSeg*>::iterator it_seg;
    std::map<Block*,Node*>::iterator it_cell;

    bool retval = true;

    // Easy to make things go region by region.  Just loop over the regions.
    // Then, we only consider multi-height cells with the proper region and
    // segments with the proper region.


    blockToNodeMap.clear();
    blockToSegMap.clear();
    nodeToBlockMap.clear();
    cells.clear();
    blocks.clear();
    segs.clear();

    // Collect all cells in the region.
    for( size_t i = 2; i < mgr.m_multiHeightCells.size(); i++ )
    {
        for( size_t n = 0; n < mgr.m_multiHeightCells[i].size(); n++ )
        {
            Node* ndi = mgr.m_multiHeightCells[i][n];
            if( ndi->getRegionId() != regId )
            {
                continue;
            }
            cells.push_back( ndi );
        }
    }
    for( size_t i = 0; i < mgr.m_singleHeightCells.size(); i++ )
    {
        Node* ndi = mgr.m_singleHeightCells[i];
        if( ndi->getRegionId() != regId )
        {
            continue;
        }
        cells.push_back( ndi );
    }

    if( cells.size() == 0 )
    {
        return true;
    }

    // Figure out the segments in which cells are found. 
    for( size_t i = 0; i < cells.size(); i++ )
    {
        Node* ndi = cells[i];
        segs.insert( mgr.m_reverseCellToSegs[ndi->getId()].begin(), mgr.m_reverseCellToSegs[ndi->getId()].end() );
    }

    // Create blocks for the cells.
    count = 0;
    for( size_t i = 0; i < cells.size(); i++ )
    {
        Node* ndi = cells[i];

        w = ndi->getWidth() ;
        h = ndi->getHeight();
        x = ndi->getX();
        y = ndi->getY();

        bptr = new Block;
        bptr->GetId() = count;
        bptr->SetNode( ndi );

        bptr->GetOrigDim( HGraph ) = w;
        bptr->GetOrigDim( VGraph ) = h;
        bptr->GetOrigPos( HGraph ) = x;
        bptr->GetOrigPos( VGraph ) = y;

        bptr->GetDim( HGraph ) = w;
        bptr->GetDim( VGraph ) = h;
        bptr->GetPos( HGraph ) = x;
        bptr->GetPos( VGraph ) = y;

        bptr->SetFixed( false );

        blocks.push_back( bptr );
        ++count;

        nodeToBlockMap[ndi] = bptr;
        blockToNodeMap[bptr] = ndi;
    }

    // Create fixed blocks for segments.  This includes all segments for the region
    // and any other segments in which an involved cell might be found.

    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        int rowId = segPtr->m_rowId;

        w = 0.;
        h = 0.;

        // Left.
        x = segPtr->m_xmin;
        y = m_arch->m_rows[rowId]->getY() + 0.5 * m_arch->m_rows[rowId]->getH();

        bptr = new Block;
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = w;
        bptr->GetOrigDim( VGraph ) = h;
        bptr->GetOrigPos( HGraph ) = x;
        bptr->GetOrigPos( VGraph ) = y;

        bptr->GetDim( HGraph ) = w;
        bptr->GetDim( VGraph ) = h;
        bptr->GetPos( HGraph ) = x;
        bptr->GetPos( VGraph ) = y;

        bptr->SetFixed( true );

        blocks.push_back( bptr );
        ++count;

        blockToSegMap[bptr] = segPtr;
        segToLeftBlockMap[segPtr] = bptr;

        // Right.
        x = segPtr->m_xmax;
        y = m_arch->m_rows[rowId]->getY() + 0.5 * m_arch->m_rows[rowId]->getH();

        bptr = new Block;
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = w;
        bptr->GetOrigDim( VGraph ) = h;
        bptr->GetOrigPos( HGraph ) = x;
        bptr->GetOrigPos( VGraph ) = y;

        bptr->GetDim( HGraph ) = w;
        bptr->GetDim( VGraph ) = h;
        bptr->GetPos( HGraph ) = x;
        bptr->GetPos( VGraph ) = y;

        bptr->SetFixed( true );

        blocks.push_back( bptr );
        ++count;

        blockToSegMap[bptr] = segPtr;
        segToRightBlockMap[segPtr] = bptr;
    }

    // Make sure required segments are sorted.
    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        mgr.resortSegment( segPtr );
    }

    // Create the horizontal constraint graph.
    std::cout << "Building local graph." << std::endl;

    std::vector<std::vector<std::pair<int,int> > > gr_out;
    std::vector<std::vector<std::pair<int,int> > > gr_inp;
    gr_out.resize( blocks.size() );
    gr_inp.resize( blocks.size() );

    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        int segId = segPtr->m_segId;

        std::map<DetailedSeg*,Block*>::iterator it_l = segToLeftBlockMap.find( segPtr );
        if( segToLeftBlockMap.end() == it_l )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
        std::map<DetailedSeg*,Block*>::iterator it_r = segToRightBlockMap.find( segPtr );
        if( segToRightBlockMap.end() == it_r )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }

        int n = mgr.m_cellsInSeg[segId].size();
        if( n == 0 )
        {
            continue;
        }

        Block* blk_i = it_l->second;
        Block* blk_j = 0;
        for( size_t j = 0; j < mgr.m_cellsInSeg[segId].size(); j++ )
        {
            // Might have cells in this segment that are not part of the region.
            Node* ndj = mgr.m_cellsInSeg[segId][j];
            std::map<Node*,Block*>::iterator it_n = nodeToBlockMap.find( ndj );
            if( nodeToBlockMap.end() == it_n )
            {
                if( ndj->getRegionId() == regId )
                {
                    std::cout << "Error." << std::endl;
                    exit(-1);
                }
                continue;
            }
            blk_j = it_n->second;

            add_edge( gr_out, gr_inp, blk_i->GetId(), blk_j->GetId(), segId );
            blk_i = blk_j;
        }
        blk_j = it_r->second;
        add_edge( gr_out, gr_inp, blk_i->GetId(), blk_j->GetId(), segId );
    }

    // Scan the order and compute Tarr and Treq.
    std::vector<double> Tarr;
    std::vector<double> Treq;
    Tarr.resize( blocks.size() );
    Treq.resize( blocks.size() );

    double worst = repair_slack( gr_out, gr_inp, Tarr, Treq );
    bool done = (worst >= 0.);
    std::cout << "Worst slack is " << worst << ", Done? " << (done?"Y":"N") << std::endl;

    // Use slack to guide the movement of single height cells to resolve issues.
    std::vector<std::pair<std::pair<int,int>,int> > gaps;
    std::vector<std::pair<int,double> > candidates;

    // Collect all single height cells.  Further, remove them from their 
    // assigned segment and from the constraint graph.
    candidates.clear();
    for( size_t ix = 0; ix < cells.size(); ix++ )
    {
        Node* ndi = cells[ix];
        int spanned = (int)(ndi->getHeight() / singleRowHeight + 0.5);
        if( spanned != 1 )
        {
            continue;
        }
        std::map<Node*,Block*>::iterator it = nodeToBlockMap.find( ndi );
        if( nodeToBlockMap.end() == it )
        {
            continue;
        }
        Block* ptri = it->second;
        int i = ptri->GetId();

        // Remove edges.
        if( gr_inp[i].size() != 1 || gr_out[i].size() != 1 )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
        if( gr_inp[i][0].second != gr_out[i][0].second )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
        int m = gr_inp[i][0].first ;
        int n = gr_out[i][0].first ;
        int p = gr_inp[i][0].second;
        // Currently: m -> i -> n in p.  Want: m -> n in p.
        rem_edge( gr_out, gr_inp, m, i, p );
        rem_edge( gr_out, gr_inp, i, n, p );
        add_edge( gr_out, gr_inp, m, n, p );

        // Remove cell from segment.
        mgr.removeCellFromSegment( ndi, p );

        candidates.push_back( std::make_pair(ptri->GetId(), 0.0) );
    }
    std::cout << "Single height cells removed is " << candidates.size() << std::endl;

    // Compute slack.
    worst = repair_slack( gr_out, gr_inp, Tarr, Treq );
    std::cout << "Worst slack with single height cells removed is " << worst << std::endl;

    // Consider each candidate, find the closest edge which accommodates
    // the candidate and then update.  This is very time-consuming with
    // the way it is currently coded...
    for( size_t c = 0; c < candidates.size(); c++ )
    {
        Block* ptrc = blocks[candidates[c].first];
        double width = ptrc->GetDim( HGraph );
        double xc = ptrc->GetPos( HGraph );
        double yc = ptrc->GetPos( VGraph );

        double best = std::numeric_limits<double>::max();
        gaps.clear();
        for( size_t b = 0; b < blocks.size(); b++ )
        {
            Block* ptri = blocks[b];
            int i = ptri->GetId();
            for( size_t e = 0; e < gr_out[i].size(); e++ )
            {
                int j = gr_out[ptri->GetId()][e].first ;
                Block* ptrj = blocks[j];
                int s = gr_out[ptri->GetId()][e].second;

                double space = Treq[j] - ( Tarr[i] + ptri->GetDim( HGraph ) );
                if( space <= width-1.0e-3 )
                {
                    continue;
                }

                double xa = 0.5 * (ptri->GetPos(HGraph)+ptrj->GetPos(HGraph) );
                double ya = 0.5 * (ptri->GetPos(VGraph)+ptrj->GetPos(VGraph) );

                double dist = std::fabs(xa - xc) + std::fabs(ya - yc);
                if( dist < best )
                {
                    if( gaps.size() != 0 ) gaps.clear();
                    gaps.push_back( std::make_pair(std::make_pair(i,j),s) );
                    best = dist;
                }
            }
        }
        if( gaps.size() != 0 )
        {
            int j = gaps[0].first.first ;
            int k = gaps[0].first.second;
            int s = gaps[0].second;
            if( !is_edge_present( gr_out, gr_inp, j, k, s ) )
            {
                continue;
            }
            Block* ptrj = blocks[j];
            Block* ptrk = blocks[k];
            double xa = 0.5 * (ptrj->GetPos(HGraph)+ptrk->GetPos(HGraph) );
            double ya = 0.5 * (ptrj->GetPos(VGraph)+ptrk->GetPos(VGraph) );

            std::cout << "Assigning block " << ptrc->GetId() << " @ "
                << "(" << xc << "," << yc << ")" << ", "
                << "(" << xa << "," << ya << ")" << std::endl;

            // Currently: j -> k in s.  Want: j -> ptrc->GetId() -> k in s.
            rem_edge( gr_out, gr_inp, j, k, s );
            add_edge( gr_out, gr_inp, j, ptrc->GetId(), s );
            add_edge( gr_out, gr_inp, ptrc->GetId(), k, s );

            // Update cell and block position.
            int r = mgr.m_segments[s]->m_rowId;
            ya = m_arch->m_rows[r]->getY() + 0.5 * ptrc->GetDim( VGraph );
            ptrc->GetPos( HGraph ) = xa;
            ptrc->GetPos( VGraph ) = ya;
            ptrc->GetNode()->setX( xa );
            ptrc->GetNode()->setY( ya );

            // Update segment.
            mgr.addCellToSegment( ptrc->GetNode(), s );

            // Update slacks.
            worst = repair_slack( gr_out, gr_inp, Tarr, Treq );
        }
    }

    // Update positions and resort segments.
    for( size_t b = 0; b < blocks.size(); b++ )
    {
        Block* ptrb = blocks[b];
        double lx = ptrb->GetPos( HGraph ) - 0.5 * ptrb->GetDim( HGraph );
        lx = std::max( Tarr[b], std::min( Treq[b], lx ) );
        ptrb->GetPos( HGraph ) = lx + 0.5 * ptrb->GetDim( HGraph );
        if( ptrb->GetNode() )
        {
            ptrb->GetNode()->setX( ptrb->GetPos( HGraph ) );
        }
        Tarr[b] = lx;
        for( size_t e = 0; e < gr_out[b].size(); e++ )
        {
            int j = gr_out[b][e].first ;
            Tarr[j] = std::max( Tarr[j], lx );
        }
    }

    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        mgr.resortSegment( segPtr );
    }

    // Cleanup.
    for( size_t i = 0; i < blocks.size(); i++ )
    {
        delete blocks[i];
    }
    blocks.clear();

    return true;
}

bool MinMovementFloorplanner::shift( DetailedMgr& mgr, bool onlyMulti )
{
    bool retval = true;
    for( size_t regId = 0; regId < m_arch->m_regions.size(); regId++ )
    {
        if( !shift( mgr, regId, onlyMulti ) )
        {
            retval = false;
        }
    }
    return retval;
}

bool MinMovementFloorplanner::shift( DetailedMgr& mgr, int regId, bool onlyMulti ) 
{
    m_net = mgr.getNetwork();
    std::vector<std::pair<double,double> > pos;
    pos.resize( m_net->m_nodes.size() );
    for( size_t i = 0; i < m_net->m_nodes.size(); i++ )
    {
        Node* ndi = &(m_net->m_nodes[i]);
        pos[ndi->getId()] = std::make_pair( ndi->getX(), ndi->getY() );
        
    }
    return shift( mgr, regId, pos, onlyMulti );
}

bool MinMovementFloorplanner::shift( DetailedMgr& mgr, int regId, 
        std::vector<std::pair<double,double> >& targets,
        bool onlyMulti ) 
{
    // !!! Requires segments for the region to be populated !!!

    // Performs shifting of cells in the X-direction for the specified region in 
    // order to remove overlap and align to sites.  Might also be able to fix
    // specific (minor) problems with the placement feasibility.

    std::cout << "Shifting cells in region " << regId << " to reduce overlap." << std::endl;

    mgr.resortSegments();

    m_net = mgr.getNetwork();
    m_arch = mgr.getArchitecture();


    std::map<Block*,DetailedSeg*> blockToSegMap;
    std::map<Block*,Node*> blockToNodeMap;
    std::map<Node*,Block*> nodeToBlockMap;
    std::map<DetailedSeg*,Block*> segToLeftBlockMap;
    std::map<DetailedSeg*,Block*> segToRightBlockMap;
    std::vector<Block*> blocks;
    std::vector<Node*> cells;
    std::set<DetailedSeg*> segs;
    Block* bptr;
    double x, y, w, h;
    int count = 0;
    int l, r;
    std::map<Block*,DetailedSeg*>::iterator it_seg;
    std::map<Block*,Node*>::iterator it_cell;

    bool retval = true;

    // Easy to make things go region by region.  Just loop over the regions.
    // Then, we only consider multi-height cells with the proper region and
    // segments with the proper region.


    blockToNodeMap.clear();
    blockToSegMap.clear();
    nodeToBlockMap.clear();
    cells.clear();
    blocks.clear();
    segs.clear();

    // Collect all cells in the region.
    for( size_t i = 2; i < mgr.m_multiHeightCells.size(); i++ )
    {
        for( size_t n = 0; n < mgr.m_multiHeightCells[i].size(); n++ )
        {
            Node* ndi = mgr.m_multiHeightCells[i][n];
            if( ndi->getRegionId() != regId )
            {
                continue;
            }
            cells.push_back( ndi );
        }
    }
    if( !onlyMulti )
    {
        for( size_t i = 0; i < mgr.m_singleHeightCells.size(); i++ )
        {
            Node* ndi = mgr.m_singleHeightCells[i];
            if( ndi->getRegionId() != regId )
            {
                continue;
            }
            cells.push_back( ndi );
        }
    }

    if( cells.size() == 0 )
    {
        return true;
    }

    // Figure out the segments in which cells are found. 
    for( size_t i = 0; i < cells.size(); i++ )
    {
        Node* ndi = cells[i];
        segs.insert( mgr.m_reverseCellToSegs[ndi->getId()].begin(), mgr.m_reverseCellToSegs[ndi->getId()].end() );
    }

    // Create blocks for the cells.
    count = 0;
    for( size_t i = 0; i < cells.size(); i++ )
    {
        Node* ndi = cells[i];

        w = ndi->getWidth() ;
        h = ndi->getHeight();
        //x = ndi->getX();
        //y = ndi->getY();
        // Put the blocks at the targets.  Note that the constraints
        // are built from the information in the segments and that's
        // fine.  The set positions might not be in the same order.
        x = targets[ndi->getId()].first ;
        y = targets[ndi->getId()].second;

        bptr = new Block;
        bptr->GetId() = count;
        bptr->SetNode( ndi );

        bptr->GetOrigDim( HGraph ) = w;
        bptr->GetOrigDim( VGraph ) = h;
        bptr->GetOrigPos( HGraph ) = x;
        bptr->GetOrigPos( VGraph ) = y;

        bptr->GetDim( HGraph ) = w;
        bptr->GetDim( VGraph ) = h;
        bptr->GetPos( HGraph ) = x;
        bptr->GetPos( VGraph ) = y;

        bptr->SetFixed( false );

        blocks.push_back( bptr );
        ++count;

        nodeToBlockMap[ndi] = bptr;
        blockToNodeMap[bptr] = ndi;
    }

    // Create fixed blocks for segments.  This includes all segments for the region
    // and any other segments in which an involved cell might be found.

    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        int rowId = segPtr->m_rowId;

        w = 0.;
        h = 0.;

        // Left.
        x = segPtr->m_xmin;
        y = m_arch->m_rows[rowId]->getY() + 0.5 * m_arch->m_rows[rowId]->getH();

        bptr = new Block;
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = w;
        bptr->GetOrigDim( VGraph ) = h;
        bptr->GetOrigPos( HGraph ) = x;
        bptr->GetOrigPos( VGraph ) = y;

        bptr->GetDim( HGraph ) = w;
        bptr->GetDim( VGraph ) = h;
        bptr->GetPos( HGraph ) = x;
        bptr->GetPos( VGraph ) = y;

        bptr->SetFixed( true );

        blocks.push_back( bptr );
        ++count;

        blockToSegMap[bptr] = segPtr;
        segToLeftBlockMap[segPtr] = bptr;

        // Right.
        x = segPtr->m_xmax;
        y = m_arch->m_rows[rowId]->getY() + 0.5 * m_arch->m_rows[rowId]->getH();

        bptr = new Block;
        bptr->GetId() = count;

        bptr->GetOrigDim( HGraph ) = w;
        bptr->GetOrigDim( VGraph ) = h;
        bptr->GetOrigPos( HGraph ) = x;
        bptr->GetOrigPos( VGraph ) = y;

        bptr->GetDim( HGraph ) = w;
        bptr->GetDim( VGraph ) = h;
        bptr->GetPos( HGraph ) = x;
        bptr->GetPos( VGraph ) = y;

        bptr->SetFixed( true );

        blocks.push_back( bptr );
        ++count;

        blockToSegMap[bptr] = segPtr;
        segToRightBlockMap[segPtr] = bptr;
    }

    // Make sure required segments are sorted.
    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        mgr.resortSegment( segPtr );
    }

    // Create the horizontal constraint graph.
    m_Tmin[HGraph] = m_arch->m_xmin;
    m_Tmax[HGraph] = m_arch->m_xmax;
    m_Tmin[VGraph] = m_arch->m_ymin;
    m_Tmax[VGraph] = m_arch->m_ymax;

    // Turn the blocks into a member variable.
    m_blocks = blocks;

    // Compute a tolerance for certain checks.
    initializeEpsilon();

    // Construct HGraph using sorted segments.
    m_graph[HGraph]->Clear();
    m_graph[VGraph]->Clear();
    m_graph[HGraph]->Resize( m_blocks.size() );

    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        int segId = segPtr->m_segId;

        std::map<DetailedSeg*,Block*>::iterator it_l = segToLeftBlockMap.find( segPtr );
        if( segToLeftBlockMap.end() == it_l )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }
        std::map<DetailedSeg*,Block*>::iterator it_r = segToRightBlockMap.find( segPtr );
        if( segToRightBlockMap.end() == it_r )
        {
            std::cout << "Error." << std::endl;
            exit(-1);
        }

        int n = mgr.m_cellsInSeg[segId].size();
        if( n == 0 )
        {
            continue;
        }

        Block* blk_i = it_l->second;
        Block* blk_j = 0;
        for( size_t j = 0; j < mgr.m_cellsInSeg[segId].size(); j++ )
        {
            // Might have cells in this segment that are not part of the region.
            Node* ndj = mgr.m_cellsInSeg[segId][j];
            std::map<Node*,Block*>::iterator it_n = nodeToBlockMap.find( ndj );
            if( nodeToBlockMap.end() == it_n )
            {
                if( ndj->getRegionId() == regId && !onlyMulti )
                {
                    std::cout << "Error." << std::endl;
                    exit(-1);
                }
                continue;
            }
            blk_j = it_n->second;

            // Need an edge from blk_i -> blk_j.
            if( !m_graph[HGraph]->DoesEdgeExist( blk_i->GetId(), blk_j->GetId() ) ) 
            { 
                m_graph[HGraph]->AddEdge( blk_i->GetId(), blk_j->GetId() ); 
            }

            blk_i = blk_j;
        }
        blk_j = it_r->second;
        if( !m_graph[HGraph]->DoesEdgeExist( blk_i->GetId(), blk_j->GetId() ) ) 
        { 
            m_graph[HGraph]->AddEdge( blk_i->GetId(), blk_j->GetId() ); 
        }
    }
 
    computeSlack( HGraph );
    std::cout << "HGraph construction done, worst slack is " << m_worstSlack[HGraph] << ", "
        << "Nodes is " << m_graph[HGraph]->GetNumNodes() << ", "
        << "Edges is " << m_graph[HGraph]->GetNumEdges() << std::endl;

    // Setup and solve the LP to get the minimum shift.  I'm currently ignoring 
    // gap requirements...  I might have a bug so I am trying to find that...
    bool shift = true;
    computeSlack( HGraph );
    bool legal = m_worstSlack[HGraph] >= 0.;
    // The following call is very expensive in terms of memory so don't do it
    // right now until it is recoded.
    //m_graph[HGraph]->TransitiveReduction(); // Reduces the LP constraints.

    // Note that clumping updates the block locations, but does not update
    // cell positions.
    double amtResizedX = 0.;
    double amtMovedX = 0.;
    shift = clump( amtMovedX, amtResizedX );
    std::cout << "Clumping, total quadratic movement in X-direction is " << amtMovedX << ", "
        << "Resized is " << amtResizedX << std::endl;

    // Clumping does not update cell positions.
    {
        double disp = 0;
        for( size_t i = 0; i < cells.size(); i++ )
        {
            Node* ndi = cells[i];

            std::map<Node*,Block*>::iterator it = nodeToBlockMap.find( ndi );
            if( nodeToBlockMap.end() == it )
            {
                std::cout << "Error." << std::endl;
                exit(-1);
            }
            bptr = it->second;

            double oldX = ndi->getX();
            ndi->setX( bptr->GetPos( HGraph ) );

            disp += std::fabs( ndi->getX() - oldX );
        }
        std::cout << "Minimum shift for region " << regId << ", "
            << "Displacement is " << disp 
            << std::endl;
    }

    if( !shift )
    {
        // Shift failed.
        std::cout << "Minimum shift code failed for region " << regId << std::endl;

        retval = false;
    }
    else
    {
        // Shift passed.  
        std::cout << "Minimum shift code passed for region " << regId << std::endl;
    }

    if( 1 )
    {
        // Try to align to sites.
        alignToSites();

        double disp = 0.;
        for( size_t i = 0; i < cells.size(); i++ )
        {
            Node* ndi = cells[i];

            std::map<Node*,Block*>::iterator it = nodeToBlockMap.find( ndi );
            if( nodeToBlockMap.end() == it )
            {
                std::cout << "Error." << std::endl;
                exit(-1);
            }
            bptr = it->second;

            double oldX = ndi->getX();
            ndi->setX( bptr->GetPos( HGraph ) );

            disp += std::fabs( ndi->getX() - oldX );
        }
        std::cout << "Alignment for region " << regId << ", "
            << "Displacement is " << disp << ", "
            << std::endl;
    }

    // Make sure required segments are sorted.
    for( size_t s = 0; s < mgr.m_segments.size(); s++ )
    {
        DetailedSeg* segPtr = mgr.m_segments[s];

        if( !(segPtr->m_regId == regId || segs.end() != segs.find( segPtr )) )
        {
            continue;
        }

        mgr.resortSegment( segPtr );
    }

    // Cleanup.
    for( size_t i = 0; i < blocks.size(); i++ )
    {
        delete blocks[i];
    }
    blocks.clear();

    return retval;
}


} // namespace dpo
