/**************************************************************************
***
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
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

//! author="Igor Markov"

/*  The basic Pseudo-random Number Generator
  This class should be a model for all other pseudo-random
  generators (not necessarily >number< generators)

CHANGES
  970821 ilm   rid class RandomNumberGenerator of two [ambiguous] conversion
                   (unsigned and double). Instead derived two new classes
                   RandomUnsigned and RandmDouble which have the same
                   interface and respective conversions.
  970821 ilm   all private members/methods of RNG made protected
                   so that it is good as base class for RandomGenerators
  970821 ilm   added unsigned RNG::operator() (unsigned N) const to
                   RandomUnsigned for STL conformance
  971009 mro   extensive changes including making all end-user RNG classes
                   templates, making the internal random engine r250/r1279,
               adding RandomRawUnsigned, RandomRawDouble, RandomNormal
  971113 mro   added seed logging
  971114 mro   added SeedCleaner class, got rid of "Old" kernel
  971215 mro   moved SeedCleaner and SeedHandler classes to abkseed.h
  990617 mro   extensive expansion of seed-handling mechanism:  now
                  allows tripartite seeds for locally independent
                  RNGs.  Also improved selection of initial nondeterministic
                  seed and added support for verbosity.
*/
//! CHANGES="abkrand.h 970821 ilm   rid class RandomNumberGenerator of two
//[ambiguous] conversion (unsigned and double). Instead derived two new classes
// RandomUnsigned and RandmDouble which have the same interface and respective
// conversions.  970821 ilm   all private members/methods of RNG made protected
// so that it is good as base class for RandomGenerators 970821 ilm   added
// unsigned RNG::operator() (unsigned N) const to RandomUnsigned for STL
// conformance 971009 mro   extensive changes including making all end-user RNG
// classes templates, making the internal random engine r250/r1279, adding
// RandomRawUnsigned, RandomRawDouble, RandomNormal 971113 mro   added seed
// logging 971114 mro   added SeedCleaner class, got rid of "Old" kernel 971215
// mro   moved SeedCleaner and SeedHandler classes to abkseed.h   990617 mro
// extensive expansion of seed-handling mechanism:  now allows tripartite seeds
// for locally independent RNGs.  Also improved selection of initial
// nondeterministic seed and added support for verbosity. "

#ifndef _ABKRAND_H_
#define _ABKRAND_H_

#include "abkseed.h"
#include "abktimer.h"
#include "abkassert.h"
#include "verbosity.h"
#include <iostream>
#include <cmath>
#include <ABKCommon/uofm_alloc.h>

/* These classes are actually typedefed in the bottom of this class;
 statements are rather verbose, therefore not included here  -ILM

class RandomNumberGenerator;
class RandomUnsigned;
class RandomDouble;

typedef RandomNumberGenerator RNG;

 IMPORTANT:

 All random "kernel" classes MUST be derived from RandomRoot
*/

//: All random "kernel" classes MUST be derived from RandomRoot
class RandomRoot {
       private:
        SeedHandler _handler;

       protected:
        unsigned _seed;
        Verbosity _verb;
        RandomRoot(unsigned seed = UINT_MAX, Verbosity verb = Verbosity("silent")) : _handler(seed), _verb(verb) {
                _seed = _handler._seed;
                if (verb.getForMajStats()) std::cout << "_seed = " << _seed << std::endl;
        }

        RandomRoot(const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent"));

       public:
        static const double randTick;  //    = 1/(0xffffffff + 1.0); // 2^-32
        unsigned getSeed() const {
                abkfatal(!_handler._isSeedMultipartite,
                         "can't use getSeed() "
                         "with multipartite seed");
                return _seed;
        }

        static unsigned getExternalSeed();
        const char *getLocIdent() const;
        unsigned getCounter() const;

        const Verbosity &getVerbosity() const { return _verb; }
};

//:  Currently implemented are based on the r250 (or r1279)
//   Tausworthe shift-register algorithm.  The periods are enormously
//   longer than the 32-bit linear congruential RNGs we were using.
//   Short periods can be a problem even if you don't exhaust them,
//   because you have a significant chance that the sequences produced
//   by two different seeds will overlap in the global sequence of
//   the RNG, and therefore not be independent.
class Tausworthe : public RandomRoot {
       private:
        unsigned _encryptWithSeed(unsigned clear);
        // the reason for the function is to be able
        // to "encrypt" a buffer value in a way dependent on
        // the seed, so that the initial state of the buffer
        // depends very chaotically on the seed.
        // returns (clear XOR _seed)^17 mod cryptMod
        void _encryptBufWithSeed();

        void _encryptBufMultipartiteSeed();

        unsigned _multmod(unsigned x, unsigned y);
        // returns x*y mod cryptMod
        unsigned _bufferSize;
        unsigned _tauswortheQ;
        mutable unsigned *_buffer;
        mutable unsigned _cursor;

       protected:
        Tausworthe(unsigned bufSize, unsigned Q, unsigned *preloads, unsigned seed = UINT_MAX, Verbosity verb = Verbosity("silent")) : RandomRoot(seed, verb), _bufferSize(bufSize), _tauswortheQ(Q), _preloads(preloads) {
                _buffer = new unsigned[bufSize];
                for (unsigned i = 0; i != bufSize; i++) _buffer[i] = 0;
                _encryptBufWithSeed();
        }

        Tausworthe(unsigned bufSize, unsigned Q, unsigned *preloads, const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent")) : RandomRoot(locIdent, counterOverride, verb), _bufferSize(bufSize), _tauswortheQ(Q), _preloads(preloads) {
                _buffer = new unsigned[bufSize];
                for (unsigned i = 0; i != bufSize; i++) _buffer[i] = 0;
                _encryptBufMultipartiteSeed();
        }

        ~Tausworthe() { delete[] _buffer; }
        inline unsigned _getRawUnsigned() {
                // the function is the genuine "raw" engine for the RNG
                {
                        unsigned retval;
                        int otherPoint = _cursor - _tauswortheQ;
                        if (otherPoint < 0) otherPoint += _bufferSize;

#if defined(__SUNPRO_CC)
                        //|| defined(__GNUC__)
                        /* SUN compiler doesn't use "mutable" */
                        retval = const_cast<unsigned *>(_buffer)[const_cast<Tausworthe *>(this)->_cursor] ^= const_cast<unsigned *>(_buffer)[otherPoint];
                        if (++(const_cast<Tausworthe *>(this)->_cursor) >= _bufferSize) const_cast<Tausworthe *>(this)->_cursor = 0;

#else
                        retval = _buffer[_cursor] ^= _buffer[otherPoint];
                        if (++_cursor >= _bufferSize) _cursor = 0;
#endif

                        return retval;
                }
        }
        double _getRawDouble() { return randTick * _getRawUnsigned(); }
        unsigned *_preloads;
};

//: Existing engine classes: RandomKernel250 is derived from Tausworthe.
//    This is not necessary for future engine classes, and indeed
//    if your generator is not Tausworthe-style then please
//    don't derive it from Tausworthe as that would be confusing.
class RandomKernel250 : public Tausworthe {

       protected:
        RandomKernel250(unsigned seed = UINT_MAX, Verbosity verb = Verbosity("silent"));

        RandomKernel250(const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent"));
};
//: The other existing engine class derived form Tausworthe.
class RandomKernel1279 : public Tausworthe {

       protected:
        RandomKernel1279(unsigned seed = UINT_MAX, Verbosity verb = Verbosity("silent"));

        RandomKernel1279(const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent"));
};

//: Base template class of RandomUnsignT, RandomDoubleT, RandomRawUnsignT
//  RandomRawDoubleT and so on
template <class RK>
class RandomNumberGeneratorT : public RK {
       protected:
        double _dLowerB, _dUpperB, _dDelta;
        unsigned _lowerB, _upperB, _delta;

        unsigned _getUnsignedRand();
        double _getDoubleRand();

       public:
        RandomNumberGeneratorT(double _lowerBdry, double _upperBdry, unsigned seedN = UINT_MAX, Verbosity verb = Verbosity("silent")) : RK(seedN, verb), _dLowerB(_lowerBdry), _dUpperB(_upperBdry), _dDelta(_upperBdry - _lowerBdry), _lowerB(unsigned(_lowerBdry)), _upperB(unsigned(_upperBdry)) {
                abkfatal(_lowerBdry < _upperBdry, " Invalid range for random number generator");
                abkfatal(_dDelta > 1e-4, " Range too small for random number generator ");
                _delta = unsigned(_dDelta);
                if (_dDelta < 1) _delta = 1;
        }

        RandomNumberGeneratorT(double _lowerBdry, double _upperBdry, const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent")) : RK(locIdent, counterOverride, verb), _dLowerB(_lowerBdry), _dUpperB(_upperBdry), _dDelta(_upperBdry - _lowerBdry), _lowerB(unsigned(_lowerBdry)), _upperB(unsigned(_upperBdry)) {
                abkfatal(_lowerBdry < _upperBdry, " Invalid range for random number generator");
                abkfatal(_dDelta > 1e-4, " Range too small for random number generator ");
                _delta = unsigned(_dDelta);
                if (_dDelta < 1) _delta = 1;
        }

        unsigned operator()(unsigned N) { return RK::_getRawUnsigned() % N; }
};

//: To override the seed , do something like RandomUnsigned ru(0,19,257);
//    where 0 is the lower limit (inclusive), 19 the upper limit (strict),
//    and 257 the seed. (So it goes from 0 to 18 inclusive.)
//    If you want the seed set by the system clock, or by seeds.in, use
//    RandomUnsigned ru(0,19);
//  To make a local independent RNG object, use the *second* ctor
//    below, passing a local identifier.  Suggestion:  If your
//    RNG is called _rng and is a member of MyClass, pass
//    the string "MyClass::_rng".  If it's a local variable
//    called rng in the method MyClass::myMethod(), pass
//    the string "MyClass::myMethod(),_rng".  In this way
//    you will get the same string of random values independently
//    of how many RNG objects have been created in other people's
//    code (it will keep track of the number of RNG objects created
//    specifically with *your* string.
//  Just as you can override the seed in the older ctor, you can
//    override the counter with the ctor supporting local independence.
//    This may be useful for debugging if you need to reproduce exactly
//    what happened in a particular segment of code.
template <class RK>
class RandomUnsignedT : public RandomNumberGeneratorT<RK> {
       public:
        RandomUnsignedT(double _lowerBdry, double _upperBdry, unsigned seedN = UINT_MAX, Verbosity verb = Verbosity("silent")) : RandomNumberGeneratorT<RK>(_lowerBdry, _upperBdry, seedN, verb) {}

        RandomUnsignedT(double _lowerBdry, double _upperBdry, const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent")) : RandomNumberGeneratorT<RK>(_lowerBdry, _upperBdry, locIdent, counterOverride, verb) {}
        operator unsigned() { return RandomNumberGeneratorT<RK>::_getUnsignedRand(); }
};

//: Everything is exactly analogous to RandomUnsigned as above,
//    replacing "unsigned" by "double" in the first two parameters
//    to the constructor, and in that you cast to double to get
//    the random value.
template <class RK>
class RandomDoubleT : public RandomNumberGeneratorT<RK> {
       public:
        RandomDoubleT(double _lowerBdry, double _upperBdry, unsigned seedN = UINT_MAX, Verbosity verb = Verbosity("silent")) : RandomNumberGeneratorT<RK>(_lowerBdry, _upperBdry, seedN, verb) {};

        RandomDoubleT(double _lowerBdry, double _upperBdry, const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent")) : RandomNumberGeneratorT<RK>(_lowerBdry, _upperBdry, locIdent, counterOverride, verb) {};
        operator double() { return RandomNumberGeneratorT<RK>::_getDoubleRand(); }
};

/*
//: The next two "raw" classes do not accept ranges.
// RandomRawUnsigned takes as its range all possible unsigned values.
// RandomRawDouble returns a value in [0,1) with 32 significant bits.  They
should
// be slightly faster than RandomUnsigned and RandomDouble, which
// adjust for ranges.
*/
template <class RK>
class RandomRawUnsignedT_impl : public RK {
       public:
        RandomRawUnsignedT_impl(unsigned seedN = UINT_MAX, Verbosity verb = Verbosity("silent")) : RK(seedN, verb) {};

        RandomRawUnsignedT_impl(const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent")) : RK(locIdent, counterOverride, verb) {};

        operator unsigned() { return RK::_getRawUnsigned(); }
        unsigned operator()(unsigned N) { return RK::_getRawUnsigned() % N; }
};

template <class RK>
class RandomRawUnsignedT {

        RandomRawUnsignedT_impl<RK> *_impl;
        uofm::string _locIdent;
        unsigned _seedN, _counterOverride;
        Verbosity _verb;
        bool _haveLoc;

        void create(void) {
                if (_haveLoc)
                        _impl = new RandomRawUnsignedT_impl<RK>(_locIdent.c_str(), _counterOverride, _verb);
                else
                        _impl = new RandomRawUnsignedT_impl<RK>(_seedN, _verb);
        }

       public:
        RandomRawUnsignedT(unsigned seedN = UINT_MAX, Verbosity verb = Verbosity("silent")) : _impl(NULL), _seedN(seedN), _verb(verb), _haveLoc(false) {}

        RandomRawUnsignedT(const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent")) : _impl(NULL), _locIdent(locIdent), _counterOverride(counterOverride), _verb(verb), _haveLoc(true) {}

        ~RandomRawUnsignedT() { delete _impl; }

        operator unsigned() {
                if (_impl == NULL) create();
                return static_cast<unsigned>(*_impl);
        }

        unsigned operator()(unsigned N) {
                if (_impl == NULL) create();
                return _impl->operator()(N);
        }

        unsigned getSeed() {
                if (_impl == NULL) create();
                return _impl->getSeed();
        }
};

//:
template <class RK>
class RandomRawDoubleT : public RK {
       public:
        RandomRawDoubleT(unsigned seedN = UINT_MAX, Verbosity verb = Verbosity("silent")) : RK(seedN, verb) {};

        RandomRawDoubleT(const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent")) : RK(locIdent, counterOverride, verb) {};

        operator double() { return RK::_getRawDouble(); }
        unsigned operator()(unsigned N) { return RK::_getRawUnsigned() % N; }
};

//:
//    The constructor of class accepts two values (mean, standard deviation)
//    or three (overriding the seed):
//
//    RandomNormal rn(4.0,2.0);
//    RandomNormal rn(4.0,2.0,14);
//
//    The values of rn will be normally distributed (Gaussian, bell curve)
//    with mean 4.0 and standard deviation 2.0.
template <class RK>
class RandomNormalT : public RK {
       private:
        double _mu;
        double _sigma;
        mutable bool _cacheFull;
        mutable double _cachedValue;

       public:
        RandomNormalT<RK>(double mean, double stdDev, unsigned seed = UINT_MAX, Verbosity verb = Verbosity("silent")) : RK(seed, verb), _mu(mean), _sigma(stdDev), _cacheFull(false), _cachedValue(0.0) {}

        RandomNormalT<RK>(double mean, double stdDev, const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent")) : RK(locIdent, counterOverride, verb), _mu(mean), _sigma(stdDev), _cacheFull(false), _cachedValue(0.0) {}

        operator double();
};

// This class generates pairs of binormal deviates
// (see "Numerical Recipes in C", second edition, section 14.5)
// with specified means, standard deviations, and correlations.
// Each time the getPair() method is called, a new pair
// of deviates is returned.  The "first" element of the
// pair has mean equal to "mean1" and standard deviation
// equal to "stdDev1"; mutatatis mutandis for the "second"
// element.

template <class RK>
class RandomNormCorrPairsT {
       private:
        RandomNormalT<RK> _norm;
        const double _mu1, _mu2;
        const double _sigma1, _sigma2;
        const double _rho;
        const double _c;  // we'll use x1+_c*x2 and x1-_c*x2,
                          // where x1, x2 are indep. unit normal deviates

        const double _a1, _a2, _b1, _b2;  // intermed values useful to precalc

       public:
        RandomNormCorrPairsT<RK>(double mean1, double stdDev1, double mean2, double stdDev2, double correlation, unsigned seed = UINT_MAX, Verbosity verb = Verbosity("silent")) : _norm(0, 1, seed, verb), _mu1(mean1), _mu2(mean2), _sigma1(stdDev1), _sigma2(stdDev2), _rho(correlation), _c(sqrt((1 - _rho) / (1 + _rho))), _a1(_sigma1 / sqrt(1 + _c * _c)), _a2(_sigma2 / sqrt(1 + _c * _c)), _b1(_sigma1 * _c / sqrt(1 + _c * _c)), _b2(_sigma2 * _c / sqrt(1 + _c * _c)) {}

        RandomNormCorrPairsT<RK>(double mean1, double stdDev1, double mean2, double stdDev2, double correlation, const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent")) : _norm(0, 1, locIdent, counterOverride, verb), _mu1(mean1), _mu2(mean2), _sigma1(stdDev1), _sigma2(stdDev2), _rho(correlation), _c(sqrt((1 - _rho) / (1 + _rho))), _a1(_sigma1 / sqrt(1 + _c * _c)), _a2(_sigma2 / sqrt(1 + _c * _c)), _b1(_sigma1 * _c / sqrt(1 + _c * _c)), _b2(_sigma2 * _c / sqrt(1 + _c * _c)) {}

        std::pair<double, double> getPair();
};

// This RNG gives multinormally-distributed X_0, X_1,...,X_{n-1}
// with specified pairwise correlations.
template <class RK>
class RandomNormCorrTuplesT {
       private:
        // helper function; _rho_ij includes only upper-triangular elements
        // i.e. _rho_ij[i][j] is not really rho_ij but
        // rather rho_{i,i+j+1}
        // return value is success
        bool _findBasis(const uofm::vector<uofm::vector<double> > &_rho_ij, uofm::vector<uofm::vector<double> > &_v_ij);

        const unsigned _n;  // number of variates
        uofm::vector<RandomNormalT<RK> *> _norm_j;
        uofm::vector<double> _y_j;  // scratch space; fill with normal values
        // then transform with _v_ij
        const uofm::vector<double> _mu_i;
        const uofm::vector<double> _sigma_i;
        uofm::vector<uofm::vector<double> > _v_ij;

        bool _bad;  // will be set if _findBasis() fails

       public:
        RandomNormCorrTuplesT<RK>(const uofm::vector<double> &means, const uofm::vector<double> &stdDevs,

                                  // Note:  corrs[i][j] is the desired
                                  // correlation between X_i and
                                  // X_{i+j+1} (i.e. only upper-triangular
                                  // elements are included
                                  const uofm::vector<uofm::vector<double> > &corrs, const char *locIdent, unsigned counterOverride = UINT_MAX, Verbosity verb = Verbosity("silent"));

        ~RandomNormCorrTuplesT();

        void getTuple(uofm::vector<double> &tuple);
        bool bad() const { return _bad; }
};
/* -------------------------   IMPLEMENTATIONS  ----------------------- */
// I don't seem to be able to get any compiler to instantiate these
// methods from a template database.  I don't understand why.
// If it's figured out, then the following "#include" can be
// conditioned on the compiler
// mro 011029

#include "abkrand_templ.cxx"

typedef RandomDoubleT<RandomKernel250> RandomDouble;
typedef RandomDoubleT<RandomKernel1279> RandomDouble1279;
typedef RandomUnsignedT<RandomKernel250> RandomUnsigned;
typedef RandomUnsignedT<RandomKernel1279> RandomUnsigned1279;
typedef RandomRawDoubleT<RandomKernel250> RandomRawDouble;
typedef RandomRawDoubleT<RandomKernel1279> RandomRawDouble1279;
typedef RandomRawUnsignedT<RandomKernel250> RandomRawUnsigned;
typedef RandomRawUnsignedT<RandomKernel1279> RandomRawUnsigned1279;
typedef RandomNormalT<RandomKernel250> RandomNormal;
typedef RandomNormalT<RandomKernel1279> RandomNormal1279;
typedef RandomNumberGeneratorT<RandomKernel250> RandomNumberGenerator;
typedef RandomNumberGeneratorT<RandomKernel1279> RandomNumberGenerator1279;
typedef RandomNumberGenerator RNG;
typedef RandomNumberGenerator1279 RNG1279;
typedef RandomNormCorrPairsT<RandomKernel250> RandomNormCorrPairs;
typedef RandomNormCorrPairsT<RandomKernel1279> RandomNormCorrPairs1279;
typedef RandomNormCorrTuplesT<RandomKernel250> RandomNormCorrTuples;
typedef RandomNormCorrTuplesT<RandomKernel1279> RandomNormCorrTuples1279;

/*typedef RandomRawUnsignedT<RandomKernelOld> RandomRawUnsignedOld; */
#endif
