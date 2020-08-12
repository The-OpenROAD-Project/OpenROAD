/**************************************************************************
***    
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2010 Regents of the University of Michigan,
***               Saurabh N. Adya, Jarrod A. Roy, David A. Papa and
***               Igor L. Markov
***
***  Contact author(s): abk@cs.ucsd.edu, imarkov@umich.edu
***  Original Affiliation:   UCLA, Computer Science Department,
***                          Los Angeles, CA 90095-1596 USA
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




//  Created : 10/09/97, Mike Oliver, VLSI CAD ABKGROUP UCLA 

#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "abkrand.h"
#include "abkMD5.h"

static const unsigned cryptMod    = 0xfffffffb; // greatest 32-bit prime
static const unsigned initVector  = 0xa90acaf7;
static const unsigned twoTo16     = 0x10000;
static const unsigned twoTo31     = 0x80000000;

static const char padding[]="I am I, Don Quixote, the Lord of La Mancha, my "
                      "destiny calls and I go.  And the wild winds of "
                      "fortune shall carry me onwards, o whithersoever "
                      "they blow";

static void numToStr(char *str,unsigned num)
    {
    str[0] = (num & 0xf) +1;
    num >>= 4;
    str[1] = (num & 0xf) +1;
    num >>= 4;
    str[2] = (num & 0xf) +1;
    num >>= 4;
    str[3] = (num & 0xf) +1;
    num >>= 4;
    str[4] = (num & 0xf) +1;
    num >>= 4;
    str[5] = (num & 0xf) +1;
    num >>= 4;
    str[6] = (num & 0xf) +1;
    num >>= 4;
    str[7] = (num & 0xf) +1;
    }

unsigned Tausworthe::_encryptWithSeed(unsigned clear)
    {
//    cout << "clear: " << clear << endl;
    unsigned initval = clear^_seed;
//    cout << "initval" << initval << endl;
    unsigned runval = initval;
    runval = _multmod(runval,runval);
//    cout << "runval-I" << runval << endl;
    runval = _multmod(runval,runval);
    runval = _multmod(runval,runval);
    runval = _multmod(runval,runval);
    runval = _multmod(runval,initval);
    return runval;

    }

inline unsigned trim(unsigned arg)
{ return (arg>=cryptMod)?(arg-cryptMod):arg;}


unsigned addMod(unsigned x, unsigned y)
{
    bool bigX=false,bigY=false;
    unsigned retval;
    if (x>=twoTo31) {x-=twoTo31;bigX=true;}
    if (y>=twoTo31) {y-=twoTo31;bigY=true;}
    if (bigX&&bigY)
    {
        retval = trim(trim(x+y)+5) ;//mod 2^32-5
    }
    else if (bigX||bigY)
    {
        if ((x+y)<twoTo31)
            retval = trim(x+y+twoTo31);
        else
            retval = trim(((x+y)-twoTo31)+5);
    }
    else
        retval = trim(x+y);
    return retval;
}

unsigned times5(unsigned x)
{
    unsigned x2=addMod(x,x);
    unsigned x4=addMod(x2,x2);
    return addMod(x,x4);
    /*
    unsigned lowerX = x &  0x0000ffff;
    unsigned upperX = x >> 16;
    unsigned upTimes5=upperX*5; //can't overflow or exceed cryptMod
    unsigned upTerm;
    if (upTimes5>=twoTo16)
        upTerm=(upTimes5-twoTo16)*twoTo16+5; //can't overflow or exceed cryptMod
    else
        upTerm=upTimes5*twoTo16; // can't overflow or exceed cryptMod
    unsigned lowTerm=lowerX*5; //can't overflow or exceed cryptMod
    return addMod(upTerm,lowTerm);
    */
}

unsigned timesTwoTo16(unsigned x)
{
    unsigned lowerX = x &  0x0000ffff;
    unsigned upperX = x >> 16;
    unsigned upTerm=upperX*5;
    unsigned lowTerm=lowerX*twoTo16;

    return addMod(upTerm,lowTerm);
}

unsigned Tausworthe::_multmod(unsigned x,unsigned y)
{
    unsigned lowerY = y &  0x0000ffff;
    unsigned upperY = y >> 16;
    unsigned lowerX = x &  0x0000ffff;
    unsigned upperX = x >> 16;

    unsigned upperProd = trim(upperX*upperY);
    unsigned upperTerm = trim(times5(upperProd)); //contrib to sum mod 2^32-5
    unsigned midProd1 = trim(lowerY*upperX);
    unsigned midTerm1 = trim(timesTwoTo16(midProd1));
    unsigned midProd2 = trim(upperY*lowerX);
    unsigned midTerm2 = trim(timesTwoTo16(midProd2));
    unsigned lowerProd = trim(lowerY*lowerX);

    return addMod(upperTerm,addMod(midTerm1,addMod(midTerm2,lowerProd)));

}

void Tausworthe::_encryptBufWithSeed()
    {
    unsigned i;
    unsigned mask = 0xffffffff;
    unsigned setbit = 0x1;
    unsigned IVlocal = _encryptWithSeed(initVector);
//  cout << "IVlocal" << IVlocal << endl;
    memcpy(_buffer,_preloads,_bufferSize*sizeof(unsigned));

    //Having preloaded the buffer with the noise above, we
    //now "encrypt" it in cipher-feedback mode.  "Encrypt" is
    //in quotes because it's not intended to be secure and isn't
    //*quite* a permutation (although there's a decryption method
    //that would work 99.999% of the time).

    //The purpose of this is so that related seeds will not
    //produce results that depend on each other in any obvious way.

    for (i=0;i<_bufferSize;i++)
        {
        _buffer[i] ^= IVlocal;
        IVlocal = _encryptWithSeed(_buffer[i]);
        }

    //Now we make sure the 32 columns of bits are linearly
    //independent.  This is kind of overkill as the chance
    //of linear dependence is only 1 in 2^218, but it doesn't
    //really cost anything and everybody does it, so why not.

    for (i=0;i<sizeof(unsigned)*8;i++)
        {
        _buffer[i] &= mask;
        _buffer[i] |= setbit;
        mask <<= 1;
        setbit <<= 1;
        }

    _cursor=0;
    }

void Tausworthe::_encryptBufMultipartiteSeed()
    {
    unsigned i;
    unsigned mask = 0xffffffff;
    unsigned setbit = 0x1;
    MD5 hashPool;
    unsigned extSeed=getExternalSeed();
    unsigned counter=getCounter();

    char numstr[9];
    numstr[8]=0;
    const char *numstrPtr=reinterpret_cast<const char*>(numstr);
    const char *padPtr=reinterpret_cast<const char*>(padding);

    numToStr(numstr,extSeed);
    hashPool.update(numstrPtr,false);
    numToStr(numstr,counter);
    hashPool.update(numstrPtr,false);

    hashPool.update(getLocIdent(),false);
    hashPool.update(padPtr,false); //make sure we *do* MD5 transforms

    unsigned IVlocal = hashPool.getWord(0);

    memcpy(_buffer,_preloads,_bufferSize*sizeof(unsigned));

    //Having preloaded the buffer with the noise above, we
    //now "encrypt" it in cipher-feedback mode.

    //The purpose of this is so that related seeds will not
    //produce results that depend on each other in any obvious way.

    for (i=0;i<_bufferSize;i++)
        {
        _buffer[i] ^= IVlocal;
        numToStr(numstr,_buffer[i]);
        hashPool.update(numstrPtr,false);
        hashPool.update(padPtr,false);
        IVlocal = hashPool.getWord(0);
        }

    //Now we make sure the 32 columns of bits are linearly
    //independent.  This is kind of overkill as the chance
    //of linear dependence is only 1 in 2^218, but it doesn't
    //really cost anything and everybody does it, so why not.

    for (i=0;i<sizeof(unsigned)*8;i++)
        {
        _buffer[i] &= mask;
        _buffer[i] |= setbit;
        mask <<= 1;
        setbit <<= 1;
        }

    _cursor=0;

    }

