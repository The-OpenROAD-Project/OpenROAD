


#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <vector>


namespace aak
{

class Graph
{
public:
    Graph( int v ):m_v(v)
    {
        m_adj.resize( m_v );

        m_color.resize( m_v );
        std::fill( m_color.begin(), m_color.end(), -1 );
        m_ncolors = 0;
    }
    virtual ~Graph()
    {
    }
    void addEdge( int u, int v )
    {
        m_adj[u].push_back(v);
        m_adj[v].push_back(u);
    }
    void removeDuplicates()
    {
        for( int i = 0; i < m_v; i++ )
        {
            std::sort( m_adj[i].begin(), m_adj[i].end() );
            m_adj[i].erase( std::unique( m_adj[i].begin(), m_adj[i].end() ), m_adj[i].end() );
        }
    }

    void greedyColoring()
    {
        m_color.resize( m_v );
        std::fill( m_color.begin(), m_color.end(), -1 );
        m_color[0] = 0;  // first node gets first color.

        m_ncolors = 1;

        std::vector<int> avail( m_v, -1 ); 

        // Do subsequent nodes.
        for( int v = 1; v < m_v; v++ )
        {
            // Determine which colors cannot be used.  Pick the smallest 
            // color which can be used.
            for( int i = 0; i < m_adj[v].size(); i++ )
            {
                int u = m_adj[v][i];
                if( m_color[u] != -1 )
                {
                    // Node "u" has a color.  So, it is not available to "v".
                    avail[m_color[u]] = v; // Marking "avail[color]" with a "v" means it is not available for node v.
                }
            }

            for( int cr = 0; cr < m_v; cr++ )
            {
                if( avail[cr] != v ) { m_color[v] = cr; m_ncolors = std::max( m_ncolors, cr+1 ); break; }
            }
        }
        std::cout << "N: " << m_v << ", Colors: " << m_ncolors << std::endl;

    }
public:
    int m_v;
    std::vector<std::vector<int> > m_adj;
    std::vector<int> m_color;
    int m_ncolors;

};

} // namespace aak

