#include "pinCache.h"
#include "Nets.h"
#include "Nodes.h"
#include <sstream>
using std::stringstream;

namespace parquetfp
{
    unsigned PinOffsetCache::instCount = 0;

    PinOffsetCache::PinOffsetCache(bool active):
        //_active(active),
        _active(false),
        _pinOffsetsSetup(false),hits(0),misses(0),computes(0),inst(instCount++)
    {
        if(_active)
            cout<<"Constructing an active pin cache, inst: "<<inst<<endl;
        else
            cout<<"Constructing an inactive pin cache, inst: "<<inst<<endl;
    }
    PinOffsetCache::PinOffsetCache(const PinOffsetCache& orig):
      _active(orig._active),
      _pinOffsetsSetup(orig._pinOffsetsSetup),
      _computedPinOffset(orig._computedPinOffset),
      _pinOffsetx(orig._pinOffsetx),
      _pinOffsety(orig._pinOffsety),
      _netOffsetIntoPinArray(orig._netOffsetIntoPinArray),
      _nodeHard(orig._nodeHard),
      hits(0),
      misses(0),
      computes(0),
      inst(instCount++)
    {
        if(_active)
            cout<<"Copy Constructing an active pin cache, inst: "<<inst<<endl;
        else
            cout<<"Copy Constructing an inactive pin cache, inst: "<<inst<<endl;
    }
 
    PinOffsetCache& PinOffsetCache::operator=(const PinOffsetCache& rhs)
    {
        if(_active)
            cout<<"operator = an active pin cache, inst: "<<inst<<endl;
        else
            cout<<"operator = an inactive pin cache, inst: "<<inst<<endl;
        _active=rhs._active;
        _pinOffsetsSetup=rhs._pinOffsetsSetup;
        hits=rhs.hits;
        misses=rhs.misses;
        computes=rhs.computes;
        inst=instCount++;
        _computedPinOffset.clear();
        _computedPinOffset.insert(_computedPinOffset.end(), rhs._computedPinOffset.begin(), rhs._computedPinOffset.end());
        _pinOffsetx.clear();
        _pinOffsetx.insert(_pinOffsetx.end(), rhs._pinOffsetx.begin(), rhs._pinOffsetx.end());
        _pinOffsety.clear();
        _pinOffsety.insert(_pinOffsety.end(), rhs._pinOffsety.begin(), rhs._pinOffsety.end());
        _netOffsetIntoPinArray.clear();
        _netOffsetIntoPinArray.insert(_netOffsetIntoPinArray.end(), rhs._netOffsetIntoPinArray.begin(), rhs._netOffsetIntoPinArray.end());
        _nodeHard.clear();
        _nodeHard.insert(_nodeHard.end(), rhs._nodeHard.begin(), rhs._nodeHard.end());
        return *this;
    }

}

