/**************************************************************************
***
*** Copyright (c) 2000-2006 Regents of the University of Michigan,
***               Saurabh N. Adya, Hayward Chan, Jarrod A. Roy
***               and Igor L. Markov
***
***  Contact author(s): sadya@umich.edu, imarkov@umich.edu
***  Original Affiliation:   University of Michigan, EECS Dept.
***                          Ann Arbor, MI 48109-2122 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation
***  the rights to use, copy, modify, merge, publish, distribute, sublicense,
***  and/or sell copies of the Software, and to permit persons to whom the
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/


#ifndef _PINCACHE_H_
#define _PINCACHE_H_

#include "Nets.h"
#include "Net.h"
#include "Node.h"
#include "Nodes.h"
#include <sstream>
#include <string>

namespace parquetfp
{
    class PinOffsetCache
    {
        public:
            PinOffsetCache(bool active);
            PinOffsetCache(const PinOffsetCache& orig);
            PinOffsetCache& operator=(const PinOffsetCache& orig);
            void resize(const Nodes* nodes, const Nets* nets);
            bool isActive(void) const { return _active; }

            void computePinOffset(const Node& node, const Net& net, const pin& curpin, float& x, float& y) const;
            void getPinOffset(const Node& node, const Net& net, unsigned pinOnNetIdx, float& x, float& y);
            void invalidate(const Net& net, unsigned pinOnNetIdx);
            unsigned getNumHits(void) const { return hits; }
            unsigned getNumMisses(void) const { return misses; }
            unsigned getNumComputes(void) const { return computes; }
        private: 
            bool _active; //not active means always compute when asked
            bool _pinOffsetsSetup; 
            std::vector<bool> _computedPinOffset;
            std::vector< float > _pinOffsetx;
            std::vector< float > _pinOffsety;
            std::vector<unsigned> _netOffsetIntoPinArray;
            std::vector<bool> _nodeHard;
            
            mutable unsigned hits;
            mutable unsigned misses;
            mutable unsigned computes;

            static unsigned instCount;
            unsigned inst;
    };

    inline void PinOffsetCache::resize(const Nodes* nodes, const Nets* nets)
    {
        if(!_active) return;

        abkfatal(nodes, "Trying to resize a cache with a NULL nodes pointer");
        abkfatal(nets, "Trying to resize a cache with a NULL nets pointer");

        unsigned numPins = nets->getNumPins();
        _pinOffsetx.resize(numPins);
        _pinOffsety.resize(numPins);
        _computedPinOffset.resize(numPins);

        unsigned numNets = nets->getNumNets();
        _netOffsetIntoPinArray.resize(numNets);
        unsigned accum = 0;
        for(unsigned n = 0; n<numNets; ++n)
        {
            _netOffsetIntoPinArray[n] = accum; 
            accum += nets->getNet(n).getDegree();
        }

        unsigned numNodes = nodes->getNumNodes();
        unsigned numTot = numNodes + nodes->getNumTerminals();
        _nodeHard.resize(numTot);
        for(unsigned i = 0; i < numNodes; ++i)
            _nodeHard[i]=nodes->getNode(i).isHard();
        for(unsigned i = numNodes; i < numTot; ++i)
            _nodeHard[i]=false;

        _pinOffsetsSetup=true;

        unsigned numHard = 0;
        for(unsigned i=0; i < numNodes; ++i)
            numHard += _nodeHard[i]?1:0;
        cout<<"Pin cache "<<inst<<" says: Num Nodes: "<<numNodes<<
            " Num Terms: "<<(numTot-numNodes)<<
            " Num Pins: "<<(numPins)<<
            " Num Hard:  "<<numHard<<
            " Setup:  "<<_pinOffsetsSetup<<endl;

    }

    inline void PinOffsetCache::invalidate(const Net& net, unsigned pinOnNetIdx)
    {
        if(!_active) return;

        if(!_pinOffsetsSetup)
        {
          std::stringstream ss;
          std::string msg("Trying to invalidate a pin offset before cache is set up");
          ss<<" in pin cache "<<inst<<endl;   
          msg+=ss.str();
          abkfatal(_pinOffsetsSetup, msg.c_str());
        }
        _computedPinOffset[_netOffsetIntoPinArray[net.getIndex()]+pinOnNetIdx]=false;
    }

    inline void PinOffsetCache::computePinOffset(const Node& node, const Net& net, const pin& curpin, float& x, float& y) const
    {
        //++computes;
        float width = node.getWidth();
        float height = node.getHeight();
        if(node.allPinsAtCenter)
        {
            x = 0.5*width;
            y = 0.5*height;
        }
        else
        {
            x = 0.5*width + (curpin.getXOffset()*width);
            y = 0.5*height + (curpin.getYOffset()*height);
        }
        return;
    }

    inline void PinOffsetCache::getPinOffset(const Node& node, const Net& net, unsigned pinOnNetIdx, float& x, float& y) 
    {
        if(!_active) { computePinOffset(node, net, net.getPin(pinOnNetIdx), x, y); return; }

        //abkfatal(_pinOffsetsSetup,"Trying to get pin offset before cache is set up");

        unsigned pinIdx = _netOffsetIntoPinArray[net.getIndex()]+pinOnNetIdx;
        const pin& curpin = net.getPin(pinOnNetIdx);
        if( _nodeHard[node.getIndex()] && _computedPinOffset[pinIdx] )
        {
            //cout<<"Cache hit!"<<endl;
            //++hits;
            x = _pinOffsetx[pinIdx];
            y = _pinOffsety[pinIdx];
            return; 
        }
        else
        {
            //cout<<"Cache miss!"<<endl;
            //++misses;
            computePinOffset(node, net, curpin, x, y);
            _pinOffsetx[pinIdx] = x;
            _pinOffsety[pinIdx] = y;
            _computedPinOffset[pinIdx] = true;
            return;
        }
    }

}

#endif



