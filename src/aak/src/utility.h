
////////////////////////////////////////////////////////////////////////////////
// File: utility.h
////////////////////////////////////////////////////////////////////////////////


#pragma once


////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <iostream>
#include <vector>
// Needed for getrusage
#include <sys/time.h>  
#include <sys/resource.h> 
#include <unistd.h>
#include <boost/random/mersenne_twister.hpp>

#define USE_GETTIMEOFDAY

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////
// Typedefs.
////////////////////////////////////////////////////////////////////////////////
typedef boost::mt19937  Placer_RNG;


namespace aak
{
class Network;
class Node;
class Edge;
class Pin;
class Architecture;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class Utility
{
public:

    // Timer that uses getrusage to record time.
    class Timer
    {
    public:
        Timer() : m_tm1(0.0),m_tm2(0.0)
        { 
            ; 
        }

        inline double usertime( void ) const 
        { 
            return (m_tm2-m_tm1);
        } 
        
        inline void start() 
        { 
#ifdef USE_GETTIMEOFDAY
            m_tm1 = ( gettimeofday(&m_tv,  NULL) == 0 )
                        ? m_tv.tv_sec + 1.0e-6 * m_tv.tv_usec
                        : 0.0
                        ;
#else
            m_tm1 = ( getrusage( RUSAGE_SELF, &m_ru ) == 0 ) 
                        ? m_ru.ru_utime.tv_sec + 1.0e-6 * m_ru.ru_utime.tv_usec
                        : 0.0
                        ; 
#endif
        }

        inline void stop () 
        { 
#ifdef USE_GETTIMEOFDAY
            m_tm2 = ( gettimeofday(&m_tv,  NULL) == 0 )
                        ? m_tv.tv_sec + 1.0e-6 * m_tv.tv_usec
                        : 0.0
                        ;
#else
            m_tm2 = ( getrusage( RUSAGE_SELF, &m_ru ) == 0 ) 
                        ? m_ru.ru_utime.tv_sec + 1.0e-6 * m_ru.ru_utime.tv_usec
                        : 0.0
                        ; 
#endif
        }
    public:
#ifdef USE_GETTIMEOFDAY
        struct timeval m_tv;
#else
        struct rusage m_ru;
#endif
        double m_tm1;
        double m_tm2;
    };

    template <class RandomAccessIter> inline static void random_shuffle(
            RandomAccessIter first, RandomAccessIter last, 
            Placer_RNG* rng ) 
    {
        // This function implements the random_shuffle code from the STL, but 
        // uses our Boost-based random number generator to get much better 
        // random permutations.
        
        unsigned  randnum;

        if( first == last ) 
        {
            return; 
        } 
        for( RandomAccessIter i = first + 1; i != last; ++i ) 
        { 
            randnum = ( (unsigned)(*rng)() ) % ( i - first + 1 ); 
            std::iter_swap( i, first + randnum ); 
        } 
    }

    struct compare_blockages
    {
        inline bool operator()( std::pair<double,double> i1, std::pair<double,double> i2 ) const
        {
            if( i1.first == i2.first )
            {
                return i1.second < i2.second;
            }
            return i1.first < i2.first;
        }
    };

public:
    static double disp_l1( Network* nw, double& tot, double& max, double& avg );

    static double hpwl( Network* nw );
    static double hpwl( Network* nw, double& hpwlx, double& hpwly );
    static double hpwl( Network* nw, Edge* ed );
    static double hpwl( Network* nw, Edge*, double& hpwlx, double& hpwly );

    static double rsmt( Network* nw );
    static double rsmt( Network* nw, Edge* ed );

    static void check_connectivity( Network* );

    static double area( Network* nw, bool print = true );

    static unsigned count_num_ones( unsigned x )
    {
        x -= ((x >> 1) & 0x55555555);
        x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
        x = (((x >> 4) + x) & 0x0f0f0f0f);
        x += (x >> 8);
        x += (x >> 16);
        return(x & 0x0000003f);
    }

    static void map_shredded_to_original( Network* original, Network* shredded, std::vector<Node*>& reverseMap,
            std::vector<std::vector<Node*> >& forwardMap );


    static void get_row_blockages( Network* network, Architecture* arch, std::vector<Node*>& fixed, 
                        std::vector<std::vector<std::pair<double,double> > >& blockages );
    static void get_segments_from_blockages( Network* network, Architecture* arch, 
                        std::vector<std::vector<std::pair<double,double> > >& blockages,
                        std::vector<std::vector<std::pair<double,double> > >& intervals );

    static double compute_overlap( double xmin1, double  xmax1, double ymin1, double ymax1, double xmin2, double xmax2, double ymin2, double ymax2 );

    static bool setOrientation( Network* network, Node* node, unsigned newOri );

};

} // namespace aak


