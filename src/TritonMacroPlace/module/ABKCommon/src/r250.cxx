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

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif


#include <string.h>
#include "abkrand.h"

static unsigned seedbuf250[] = 
    {
    0xcd915fc1,
    0xc68c098b,
    0xa5fdbcd4,
    0x46b32e88,
    0xa3c240ad,
    0xa1b9359d,
    0xd1db4d2f,
    0x36ae291a,
    0x33f8d276,
    0x7837ee85,
    0xaa061dac,
    0xa51594da,
    0x60db3bc1,
    0x91708fe0,
    0x436e67f9,
    0x6c46828a,
    0x955763e8,
    0x66ca2847,
    0xf3395759,
    0xdb9ded31,
    0x5c2e45e9,
    0x3c317e49,
    0xc62733b0,
    0x845e2a41,
    0x675e1918,
    0xe0d70e01,
    0x3bf4c803,
    0xa5fc435a,
    0xa1bbfa00,
    0x991755e5,
    0x489463d5,
    0x3ee73348,
    0x6bad5126,
    0x6e57f78a,
    0xa4456d2f,
    0xdea15f78,
    0x2d62de5,
    0x84a5f847,
    0x2461c706,
    0x8f9219e,
    0xb80ee2d8,
    0xdb9fa3ca,
    0x944c38c4,
    0xb98ab182,
    0x37512ed9,
    0x56b03af5,
    0xc8500696,
    0xacacbf33,
    0x85f31c38,
    0xfd019787,
    0xf8a88c9f,
    0x2a0329af,
    0xfde7f912,
    0xde4202e6,
    0x61f5af5c,
    0x94d49ec0,
    0xd2142ab0,
    0xf67e8bf6,
    0xe2c06885,
    0x317d59e7,
    0xc24e1723,
    0xe6907f03,
    0x6d984e61,
    0xd7174935,
    0x6c3906b3,
    0x7f602d3c,
    0x67ad2b1b,
    0x28c22baf,
    0xe776d238,
    0x89fb7370,
    0x2afaa277,
    0xd40bb346,
    0x4ca34522,
    0xef70254a,
    0x1ad9139d,
    0x1db3d8dc,
    0x851fc680,
    0xf855aba3,
    0x9e9c8d3d,
    0x26f27055,
    0xce995c7,
    0x3d469c31,
    0xa630242c,
    0x32dfea0,
    0x31576b95,
    0x7f1b87c1,
    0x3d2171c0,
    0xf8442593,
    0x1c03235d,
    0xe97f3787,
    0x8a86e7e8,
    0xcd71cb5,
    0x3f46c23d,
    0x47bc1d50,
    0x45f57f66,
    0x99630cf1,
    0x17f2f162,
    0xb4512daa,
    0x1dc5d048,
    0x887d0239,
    0x591c6f89,
    0xae1cb420,
    0xaf215dc7,
    0x34971374,
    0xa49d9ac6,
    0x8320e7f6,
    0xb534dbe1,
    0x218f8c89,
    0x88ca36f7,
    0x63a0c137,
    0xee8c6298,
    0x2d9d40dd,
    0xb0491d59,
    0xd5b2000f,
    0xf3fcf4ff,
    0x99215be3,
    0x36af1baa,
    0x2bb8b92b,
    0xc3030bb7,
    0xff15889f,
    0x90e8e582,
    0x61cb7960,
    0xd82b0d2f,
    0x16f88ad6,
    0x28c227fc,
    0x2239d6d0,
    0xb1ed6e1e,
    0x4075860d,
    0x95941af9,
    0x8104c2dc,
    0xb1e8b104,
    0x234edc8,
    0x528141fa,
    0xd7b48be0,
    0x22d435ee,
    0x105af697,
    0x9cc862ff,
    0xddff9c2e,
    0xbe48f827,
    0xb58454b7,
    0xa8d16cd,
    0x31d9f629,
    0x5310d57b,
    0x6d3263b2,
    0x2a573c9,
    0x52063ebf,
    0xc4243249,
    0x75d4f694,
    0x3cfc29e9,
    0x363b02a3,
    0xd2c69bcf,
    0x502591ac,
    0x7b3fff1b,
    0x5ec3edac,
    0x446681c0,
    0x3dd07c23,
    0xe70115eb,
    0x34f124de,
    0x3dcbc4a7,
    0x29f89e00,
    0x65627684,
    0xd32ce164,
    0x69540e98,
    0xccd4a330,
    0xaa98f5df,
    0xfce46bdb,
    0x715111f7,
    0x49ef26dd,
    0xbb716c28,
    0xfb7dae64,
    0x5f54a62f,
    0x2cebc3bf,
    0x821338c2,
    0x839c5674,
    0xb1b45e56,
    0xdc4a9710,
    0x63520f73,
    0x93aa4708,
    0xb042377d,
    0x548be4a0,
    0xfde4354c,
    0xe8faae11,
    0x8977bee,
    0x9991d047,
    0x255162bb,
    0x814a27e5,
    0x6b40ec13,
    0x8da4247f,
    0x6e3d695b,
    0x7d6d2a5d,
    0x529515bd,
    0xdcfa3cbe,
    0x1f468c27,
    0x93b4c9a1,
    0x80a47137,
    0xfd54da2f,
    0xa78288aa,
    0xa4f6b6fe,
    0x5be88a85,
    0x64466ad0,
    0x51723a58,
    0x87021dfd,
    0x71b1c400,
    0x498973ef,
    0x90e29f2a,
    0x9a4530c2,
    0xa2134e14,
    0x46227037,
    0xde14539b,
    0x1a7971bf,
    0xf0ff0782,
    0x36db323,
    0x3eadda5c,
    0xaf947c9d,
    0x439f043c,
    0x171732a8,
    0xf475c3c5,
    0x2652e918,
    0x41779e84,
    0x29b72d38,
    0xddd31941,
    0xbba3f193,
    0xe01c808b,
    0xcb5fe5e4,
    0xb28ceff0,
    0x2e206285,
    0x1f19893,
    0xc64faa19,
    0xfae9e9b0,
    0x1bd790ed,
    0xd87d134a,
    0x4a5481f4,
    0x633aa9c5,
    0x10ead26b,
    0xb71567fb,
    0x9c2809ae,
    0x85113ddf,
    0x673988b3,
    0x1264267c,
    0x2aa5666d,
    0xb32f5929,
    0xb7d9dfa8,
    0xad248851,
    0x93c34edd,
    0x7820d04d,
    0x81a528dc,
    0xcbbc9a4f,
    0x6fa5b3a9,
    0x4d8427ad,
    0x6454efa5,
    };

static const unsigned bufferSize250   = 250;
static const unsigned tauswortheQ250  = 103;

RandomKernel250::RandomKernel250(unsigned seed,Verbosity verb)
                     :Tausworthe(bufferSize250,
                                     tauswortheQ250,
                                     seedbuf250,
                                     seed,verb)
    {
    }

RandomKernel250::RandomKernel250(const char *locIdent,
                        unsigned counterOverride,
                        Verbosity verb)
                     :Tausworthe(bufferSize250,
                                     tauswortheQ250,
                                     seedbuf250,
                                     locIdent,counterOverride,verb)
    {
    }



