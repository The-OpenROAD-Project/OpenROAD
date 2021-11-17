
//////////////////////////////////////////////////////////////////////////////// 
// File: seq_pair.h
// Description:
//   This file contains a representation of a sequence pair.
//////////////////////////////////////////////////////////////////////////////// 

//////////////////////////////////////////////////////////////////////////////// 
// Includes.
//////////////////////////////////////////////////////////////////////////////// 
#include "seq_pair.h"
#include "utility.h"




namespace aak
{

//////////////////////////////////////////////////////////////////////////////// 
// SP::SP:
//////////////////////////////////////////////////////////////////////////////// 
SP::SP()
{ 
    ;
}

//////////////////////////////////////////////////////////////////////////////// 
// SP::SP:
//////////////////////////////////////////////////////////////////////////////// 
SP::SP( unsigned n, Placer_RNG* rng ) 
{ 
    m_x.resize(n); 
    m_y.resize(n); 
    for( unsigned i = 0 ; i < n ; i++) 
    { 
        m_x[i] = i; 
        m_y[i] = i; 
    } 
    Utility::random_shuffle( m_x.begin(), m_x.end(), rng ); 
    Utility::random_shuffle( m_y.begin(), m_y.end(), rng ); 
}

//////////////////////////////////////////////////////////////////////////////// 
// SP::SP:
//////////////////////////////////////////////////////////////////////////////// 
SP::SP( std::vector<unsigned> &x, std::vector<unsigned> &y ): 
    m_x(x), 
    m_y(y) 
{ 
    ; 
}


//////////////////////////////////////////////////////////////////////////////// 
// SPEval::SPEval:
//////////////////////////////////////////////////////////////////////////////// 
SPEval::SPEval( std::vector<double>& widths, std::vector<double>& heights ):
    m_widths(widths),
	m_heights(heights),
	m_xSize(0.0),
	m_ySize(0.0)
{
    unsigned n = widths.size();

    m_xLocFor.resize(n); 
    m_yLocFor.resize(n);

    m_xLocRev.resize(n); 
    m_yLocRev.resize(n);

    m_xSlacks.resize(n); 
    m_ySlacks.resize(n);

    m_x.resize(n); 
    m_y.resize(n);
    m_match.resize(n); 
    m_l.resize(n);
}

//////////////////////////////////////////////////////////////////////////////// 
// SPEval::Evaluate:
//////////////////////////////////////////////////////////////////////////////// 
void SPEval::Evaluate( std::vector<unsigned>& x, std::vector<unsigned>& y )
{
    // Evaluates the longest common subsequence in X and Y.
    m_x = x;
    m_y = y;
    m_xSize = evalX();
    m_ySize = evalY();
}

//////////////////////////////////////////////////////////////////////////////// 
// SPEval::EvaluateReverse:
//////////////////////////////////////////////////////////////////////////////// 
void SPEval::EvaluateReverse( std::vector<unsigned>& x, std::vector<unsigned> &y )
{
    m_x = x;
    m_y = y; 
    std::reverse( m_x.begin(),m_x.end() ); 
    std::reverse( m_y.begin(),m_y.end() );
    evalReverseX();
    evalReverseY();
    std::reverse( m_x.begin(),m_x.end() );
    std::reverse( m_y.begin(),m_y.end() );

    const unsigned n = m_x.size();
    for( unsigned i = 0; i < n; i++) 
    { 
        m_xLocRev[i] = m_xSize - m_xLocRev[i] - m_widths[i] ;
        m_yLocRev[i] = m_ySize - m_yLocRev[i] - m_heights[i];
	}
}

//////////////////////////////////////////////////////////////////////////////// 
// SPEval::EvaluateSlacks:
//////////////////////////////////////////////////////////////////////////////// 
void SPEval::EvaluateSlacks( std::vector<unsigned>& x, std::vector<unsigned>& y )
{
    // Updates slack information for the sequence pair.
	m_x = x;
	m_y = y;

	Evaluate(x,y);
	EvaluateReverse(x,y);
	const unsigned n = x.size();
	for( unsigned i = 0; i < n; i++) 
    {
		m_xSlacks[i] = m_xLocRev[i] - m_xLocFor[i];
		m_ySlacks[i] = m_yLocRev[i] - m_yLocFor[i];
	}
}

//////////////////////////////////////////////////////////////////////////////// 
// SPEval::EvaluateSlacks:
//////////////////////////////////////////////////////////////////////////////// 
double SPEval::computeLCS( 
        const std::vector<unsigned>& x, 
        const std::vector<unsigned>& y, 
        const std::vector<double>& weights, 
        std::vector<unsigned>& match, 
        std::vector<double>& pos, 
        std::vector<double>& len )
{
    const unsigned 	n = x.size();

    unsigned i, j, p;
    double t;

	for( i = 0; i < n; i++) 
    {
		match[y[i]] = i;
		len[i] = 0;
	}
	for( i = 0; i < n; i++) 
    {
		p = match[x[i]];

		pos[x[i]] = len[p];
		t = pos[x[i]] + weights[x[i]]; 
		for( j = p; j < n; j++) 
        {
			if( t > len[j]) 
            {
				len[j] = t;
			}
            else 
            {
				break;
			}
		}
	}
	return len[n-1];
}

//////////////////////////////////////////////////////////////////////////////// 
// SPEval::evalX:
//////////////////////////////////////////////////////////////////////////////// 
double SPEval::evalX( void )
{
    std::fill( m_match.begin(), m_match.end(), 0 );
    double retval = computeLCS( m_x,m_y,m_widths,m_match,m_xLocFor,m_l );
    return retval;
}

//////////////////////////////////////////////////////////////////////////////// 
// SPEval::evalY:
//////////////////////////////////////////////////////////////////////////////// 
double SPEval::evalY( void )
{
	std::fill( m_match.begin(), m_match.end(), 0 );
	std::reverse( m_x.begin(), m_x.end());
	double retval = computeLCS( m_x,m_y,m_heights,m_match,m_yLocFor,m_l );
    std::reverse( m_x.begin(),m_x.end() );
	return retval;
}

//////////////////////////////////////////////////////////////////////////////// 
// SPEval::evalReverseX:
//////////////////////////////////////////////////////////////////////////////// 
double SPEval::evalReverseX( void )
{
	std::fill( m_match.begin(), m_match.end(), 0 );
	double retval = computeLCS( m_x,m_y,m_widths,m_match,m_xLocRev,m_l );
	return retval;
}

//////////////////////////////////////////////////////////////////////////////// 
// SPEval::evalReverseY:
//////////////////////////////////////////////////////////////////////////////// 
double SPEval::evalReverseY( void )
{
	std::fill( m_match.begin(), m_match.end(), 0 );
	std::reverse( m_x.begin(),m_x.end() );
	double retval = computeLCS( m_x,m_y,m_heights,m_match,m_yLocRev,m_l );
	std::reverse( m_x.begin(),m_x.end() );
	return retval;
}

} // namespace aak
