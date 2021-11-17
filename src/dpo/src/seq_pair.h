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



//////////////////////////////////////////////////////////////////////////////// 
// File: seq_pair.h
// Description:
//   This file contains a representation of a sequence pair.
//////////////////////////////////////////////////////////////////////////////// 
 

#pragma once


//////////////////////////////////////////////////////////////////////////////// 
// Includes.
//////////////////////////////////////////////////////////////////////////////// 
#include <cmath>
#include <vector>
#include "utility.h"



namespace dpo
{

//////////////////////////////////////////////////////////////////////////////// 
// Classes.
//////////////////////////////////////////////////////////////////////////////// 
class SP 
{
// Representation of a sequence pair.
public:
	SP();
    SP( unsigned n, Placer_RNG* rng );
	SP( std::vector<unsigned> &x, std::vector<unsigned> &y );
    virtual ~SP()
    {
        ;
    }

    void Resize( unsigned n ) 
    { 
        m_x.resize( n ); 
        m_y.resize( n ); 
    }
	unsigned size( void ) 
    { 
        return m_x.size(); 
    }

public:
    std::vector<unsigned> m_x;
    std::vector<unsigned> m_y;
};

class SPEval 
{
// Methods for evaluating LCS and slacks given a sequence pair.
public:
    SPEval( std::vector<double>&, std::vector<double>& );
    virtual ~SPEval() 
    {
        ;
    }

    void Evaluate( std::vector<unsigned>&, std::vector<unsigned> & );
    void EvaluateReverse( std::vector<unsigned>&, std::vector<unsigned>& );
    void EvaluateSlacks( std::vector<unsigned>&, std::vector<unsigned>& );

protected:
    double computeLCS( const std::vector<unsigned>&, 
                const std::vector<unsigned>&, 
                const std::vector<double>&, 
                std::vector<unsigned>&, 
                std::vector<double>&, 
                std::vector<double>& );
    double evalX( void );
    double evalY( void );
    double evalReverseX( void );
    double evalReverseY( void );

public:
    std::vector<double> m_xLocFor;
    std::vector<double> m_yLocFor;

    std::vector<double> m_xLocRev;
    std::vector<double> m_yLocRev;

    std::vector<double> m_xSlacks;
    std::vector<double> m_ySlacks;

    std::vector<double> m_widths;
    std::vector<double> m_heights;

    std::vector<unsigned> m_x;
    std::vector<unsigned> m_y;
    std::vector<unsigned> m_match;
    std::vector<double> m_l;

    double m_xSize;
    double m_ySize;
};

struct SlackPair
{
    double m_v;
    unsigned m_i;
};

struct SortSlacks
{
    inline bool operator()( const SlackPair& pt1, const SlackPair&  pt2 ) const
	{
        return( pt1.m_v < pt2.m_v);
    }
};

struct SortSeqPairUsingPositions
{
public:
    SortSeqPairUsingPositions( 
            std::vector<double>* posX, std::vector<double>* posY, 
            bool sortingInX ): 
        m_posX( posX ),
        m_posY( posY ),
        m_dir( sortingInX )
	{
		;
	}

    inline bool operator()( unsigned i, unsigned j ) 
    { 
        double	vali, valj; 
        
        if( m_dir )
        { 
            vali = (*m_posX)[i] + (*m_posY)[i]; 
            valj = (*m_posX)[j] + (*m_posY)[j]; 
            return vali < valj; 
        } 
        else
        { 
            vali = (*m_posY)[i] - (*m_posX)[i]; 
            valj = (*m_posY)[j] - (*m_posX)[j]; 
            return vali < valj; 
        } 
    }

protected:
    std::vector<double>* m_posX;
    std::vector<double>* m_posY;
    bool m_dir;		// true == x, false == y
};


struct SortSeqPairUsingMatrix
{
public:
	SortSeqPairUsingMatrix( 
            std::vector< std::vector<bool> >* matrixH, std::vector< std::vector<bool> >* matrixV, 
            bool sortingInX ) : 
        m_matrixH( matrixH ), 
        m_matrixV( matrixV ), 
        m_dir( sortingInX )
	{
		;
	}

	inline bool operator()( unsigned i, unsigned j )
	{ 
        if( m_dir ) 
        { 
            if(      (*m_matrixH)[i][j] == 1 ) 
            { 
                return 1; 
            } 
            else if( (*m_matrixH)[j][i] == 1 ) 
            { 
                return 0; 
            } 
            else if( (*m_matrixV)[j][i] == 1 ) 
            {
				return 1;
			}
            else if( (*m_matrixV)[i][j] == 1 ) 
            {
				return 0;
			} 
            else 
            {
				return i < j;
			}
		} 
        else 
        {
			if(      (*m_matrixH)[i][j] == 1 ) 
            {
				return 1;
			}
            else if( (*m_matrixH)[j][i] == 1 ) 
            {
				return 0;
			}
            else if( (*m_matrixV)[j][i] == 1 ) 
            {
				return 0;
			}
            else if( (*m_matrixV)[i][j] == 1 ) 
            {
				return 1;
			}
            else 
            {
				return i < j;
			}
		}
	}

protected:
    std::vector< std::vector<bool> >* m_matrixH;
    std::vector< std::vector<bool> >* m_matrixV;
    bool m_dir;		// true == x, false == y
};

} // namespace dpo
