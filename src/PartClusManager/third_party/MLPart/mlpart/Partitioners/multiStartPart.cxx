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

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <iostream>
#include "multiStartPart.h"

using std::ostream;
using std::cout;
using std::endl;
using uofm::vector;

RandomRawUnsigned MultiStartPartitioner::_rng;

void PartitionerParams::initializeDefaults(void) {
        legalizeOnly = false;
        useClip = 0;
        useEarlyStop = false;
        maxHillHeightFactor = 0;
        maxHillWidth = 100;
        printHillStats = false;
        maxNumMoves = 0;
        minPassImprovement = 0;
        maxNumPasses = 0;
        relaxedTolerancePasses = 0;
        unCorking = 1;
        skipNetsLargerThan = UINT_MAX;
        skipLargestNets = 0;
        randomized = false;
        doFirstLIFOPass = false;
        allowCorkingNodes = false;
        allowIllegalMoves = false;
        tieBreakOnPins = false;
        wiggleTerms = false;
        saveMoveLog = false;
        useWts = true;
        mapBasedGC = false;
        hashBasedGC = false;
        noHashBasedGC = false;
        saveMem = false;
}

PartitionerParams::Parameters(Verbosity verbosity) : verb(verbosity) { initializeDefaults(); }

PartitionerParams::Parameters(int argc, const char* argv[]) : verb(argc, argv), moveMan(argc, argv), eval(argc, argv), solnGen(argc, argv) {
        initializeDefaults();

        BoolParam hashBasedGCFlag("hashBasedGC", argc, argv);
        BoolParam noHashBasedGCFlag("noHashBasedGC", argc, argv);
        //   BoolParam saveMemFlag("saveMem", argc, argv);
        BoolParam mapBasedGCFlag("mapBasedGC", argc, argv);
        BoolParam useWtsFlag("ignoreWts", argc, argv);
        BoolParam saveMoveLogFlag("saveMoveLog", argc, argv);
        BoolParam wiggleTermsFlag("noWiggleTerms", argc, argv);
        BoolParam legalizeOnlyFlag("legOnly", argc, argv);
        UnsignedParam useClipNum("useClip", argc, argv);
        BoolParam useClipFlag("useClip", argc, argv);
        DoubleParam maxHillHeight("maxHillHeightFactor", argc, argv);
        DoubleParam maxHillWidth_("maxHillWidth", argc, argv);
        BoolParam printHillStatsFlag("printHillStats", argc, argv);
        BoolParam helpRequest1("h", argc, argv);
        BoolParam helpRequest2("help", argc, argv);
        UnsignedParam maxNumMovesFlag("maxNumMoves", argc, argv);
        DoubleParam minPassImprovementFlag("minPassImprovement", argc, argv);
        UnsignedParam maxNumPassesFlag("maxNumPasses", argc, argv);
        UnsignedParam relaxedTolerancePassesFlag("relaxedTolerancePasses", argc, argv);
        BoolParam randomizedFlag("randomizedPart", argc, argv);
        IntParam unCorkingFlag("unCork", argc, argv);
        UnsignedParam skipNetsLargerThan_Flag("skipNetsLargerThan", argc, argv);
        DoubleParam skipLargestNets_Flag("skipLargestNets", argc, argv);
        BoolParam doFirstLIFOPass_Flag("doFirstLIFOPass", argc, argv);
        BoolParam allowCorkingNodes_Flag("allowCorkingNodes", argc, argv);
        BoolParam allowIllegalMoves_Flag("allowIllegalMoves", argc, argv);
        BoolParam tieBreakOnPins_Flag("tieBreakOnPins", argc, argv);

        if (helpRequest1.found() || helpRequest2.found()) {
                cout << "== Flat multistart partitioner parameters ==" << endl << " -useClip <0|1|2> \n"
                     << " -maxHillHeightFactor <double> \n"
                     << " -maxHillWidth        < 0<=double %%<=100 > \n"
                     << " -mapBasedGC, -hashBasedGC, -noHashBasedGC, "
                        "-ignoreWts \n"
                     << " -wiggleTerms, -legOnly, -printHillStats, "
                        "-saveMoveLog \n"
                     << " -maxNumMoves <unsigned> (0==infinity)[0] \n"
                     << " -minPassImprovement < 0<=double %% > \n"
                     << " -maxNumPasses <unsigned> [0]\n"
                     << " -relaxedTolerancePasses <unsigned> [0]\n"
                     << " -randomizedPart [false] \n"
                     << " -unCork    <int>  \n"
                     << " -skipNetsLargerThan <unsigned> \n"
                     << " -skipLargestNets    <0.0-100.0> \n"
                     << " -doFirstLIFOPass [false] \n"
                     << " -allowCorkingNodes [false] \n"
                     << " -allowIllegalMoves [false] \n"
                     << " -tieBreakOnPins    [false] " << endl;

                return;
        }

        hashBasedGC = hashBasedGCFlag.found();
        noHashBasedGC = noHashBasedGCFlag.found();
        //   saveMem       = saveMemFlag.found();
        mapBasedGC = mapBasedGCFlag.found();
        useWts = !useWtsFlag.found();
        saveMoveLog = saveMoveLogFlag.found();
        wiggleTerms = !wiggleTermsFlag.found();
        legalizeOnly = legalizeOnlyFlag.found();

        if (useClipNum.found())
                useClip = useClipNum;
        else if (useClipFlag.found())
                useClip = 1;
        if (useClip > 2) useClip = 1;

        if (maxHillHeight.found()) {
                useEarlyStop = true;
                maxHillHeightFactor = maxHillHeight;
        }

        if (maxHillWidth_.found()) {
                useEarlyStop = true;
                maxHillWidth = maxHillWidth_;
        }

        printHillStats = printHillStatsFlag.found();

        if (maxNumMovesFlag.found()) {
                maxNumMoves = maxNumMovesFlag;
                useEarlyStop = true;
        }

        if (maxNumPassesFlag.found()) maxNumPasses = maxNumPassesFlag;
        if (relaxedTolerancePassesFlag.found()) relaxedTolerancePasses = relaxedTolerancePassesFlag;
        if (minPassImprovementFlag.found()) minPassImprovement = minPassImprovementFlag;
        if (randomizedFlag.found()) randomized = randomizedFlag;
        if (unCorkingFlag.found()) unCorking = unCorkingFlag;
        if (skipNetsLargerThan_Flag.found()) skipNetsLargerThan = skipNetsLargerThan_Flag;
        if (skipLargestNets_Flag.found()) skipLargestNets = skipLargestNets_Flag;

        doFirstLIFOPass = doFirstLIFOPass_Flag.found();
        allowCorkingNodes = allowCorkingNodes_Flag.found();
        allowIllegalMoves = allowIllegalMoves_Flag.found();
        tieBreakOnPins = tieBreakOnPins_Flag.found();
}

ostream& operator<<(ostream& os, const PartitionerParams& parms) {
        const char* tf[3] = {"false", "true", "half-true"};

        os << "   Partitioner Parameters: \n"
           << "  " << parms.verb << "   Move manager                    : " << parms.moveMan << "\n"
           << "   Partitioning evaluator          : " << parms.eval << "\n"
           << "   Solution generator              : " << parms.solnGen << "\n"
           << "   Legalize only                   : " << tf[parms.legalizeOnly] << "\n"
           << "   Use clip / early stop           : " << tf[parms.useClip] << "/" << tf[parms.useEarlyStop] << "\n"
           << "   Print hill stats                : " << tf[parms.printHillStats] << "\n";

        if (parms.useEarlyStop) {
                os << "   Max hill height factor          : " << parms.maxHillHeightFactor << "x\n"
                   << "   Max hill width                  : " << parms.maxHillWidth << "%\n"
                   << "   Max num moves                   : " << parms.maxNumMoves << endl;
        }
        os << "   Min pass improvement            : " << parms.minPassImprovement << "%\n"
           << "   Max num passes                  : " << parms.maxNumPasses << "\n"
           << "   Relaxed tolerance passes        : " << parms.relaxedTolerancePasses << "\n"
           << "   Randomized                      : " << tf[parms.randomized] << "\n"
           << "   Uncorking                       : " << parms.unCorking << "\n";
        if (parms.skipNetsLargerThan != UINT_MAX) os << "   Skip nets larger than           : " << parms.skipNetsLargerThan << "\n";
        if (parms.skipLargestNets != 0) os << "   Skip largest nets               : " << parms.skipLargestNets << "%\n";
        os << "   Do first LIFO pass              : " << tf[parms.doFirstLIFOPass] << "\n";
        os << "   Allow corking nodes             : " << tf[parms.allowCorkingNodes] << "\n";
        os << "   Allow illegal moves             : " << tf[parms.allowIllegalMoves] << "\n";
        os << "   Tie break on pin counts         : " << tf[parms.tieBreakOnPins] << "\n";
        os << "   Wiggle terminals                : " << tf[parms.wiggleTerms] << "\n";
        os << "   Save move log                   : " << tf[parms.saveMoveLog] << "\n";
        os << "   Use weights                     : " << tf[parms.useWts] << "\n";
        os << "   Use map based gain cont         : " << tf[parms.mapBasedGC] << "\n";
        os << "   Use hash based gain cont        : " << tf[parms.hashBasedGC] << "\n";
        os << "   Do not use hash based gain cont : " << tf[parms.noHashBasedGC] << "\n";
        return os;
}

MultiStartPartitioner::MultiStartPartitioner(PartitioningProblem& problem, const Parameters& params) : _params(params), _problem(problem), _aveCost(DBL_MAX), _bestCost(UINT_MAX), _stdDev(DBL_MAX), _numLegalSolns(UINT_MAX), _maxHillHeightSeen(1), _maxGoodHillHeightSeen(1), _maxHillHeightSeenThisPass(1), _maxHillWidthSeen(0), _maxGoodHillWidthSeen(0), _maxHillWidthSeenThisPass(0) {
        if (_params.verb.getForMajStats() > 2) {
                cout << "** Statistics for the hypergraph to be partitioned: " << endl;
                const HGraphFixed& hgraph = _problem.getHGraph();
                if (hgraph.getNumEdges() == 0) {
                        abkwarn(0,
                                "No Edges in Hypergraph to be partitioned. Not "
                                "printing Statistics \n");
                } else {
                        hgraph.printEdgeSizeStats();
                        hgraph.printNodeWtStats();
                        hgraph.printNodeDegreeStats();
                        cout << endl;
                }
        }

        PartitioningBuffer& buf = _problem.getSolnBuffers();
        _solutions = vector<PartitioningSolution*>(buf.size(), static_cast<PartitioningSolution*>(NULL));

        _beginUsedSoln = buf.beginUsedSoln();
        _endUsedSoln = buf.endUsedSoln();

        unsigned _nParts = _problem.getNumPartitions();

        for (unsigned sol = _beginUsedSoln; sol != _endUsedSoln; sol++) _solutions[sol] = new PartitioningSolution(buf[sol], _problem.getHGraph(), _nParts, UINT_MAX);
}

MultiStartPartitioner::~MultiStartPartitioner() {
        unsigned sz = _solutions.size();
        for (unsigned i = 0; i != sz; i++)
                if (_solutions[i]) delete _solutions[i];
}

void MultiStartPartitioner::runMultiStart() {

        Timer basePartTimer;

        PartitioningBuffer& buf = _problem.getSolnBuffers();
        unsigned solnBegin = buf.beginUsedSoln(), solnEnd = buf.endUsedSoln();
        unsigned numStarts = solnEnd - solnBegin;

        if (_params.verb.getForActions() > 0) {
                cout << endl;
                cout << " Running " << numStarts << " multi-start";
                if (numStarts != 1) cout << 's';
                cout << endl;
        }

        for (unsigned soln = solnBegin; soln != solnEnd; soln++) doOne(soln);

        vector<double>& costs = const_cast<vector<double>&>(_problem.getCosts());
        vector<double>& viols = const_cast<vector<double>&>(_problem.getViolations());
        vector<double>& imbals = const_cast<vector<double>&>(_problem.getImbalances());

        for (unsigned sol = solnBegin; sol != solnEnd; sol++) {
                // make sure all the info is up-to-date
                _solutions[sol]->calculatePartData(_problem.getHGraph());
                costs[sol] = _solutions[sol]->cost;
                viols[sol] = _solutions[sol]->violation;
                imbals[sol] = _solutions[sol]->imbalance;
        }

        setBestSolAndAverageCost();

        if (_params.verb.getForMajStats() > 0) {
                cout << " Legal solutions : " << _numLegalSolns;
                if (_numLegalSolns == numStarts)
                        cout << " (all) " << endl;
                else
                        cout << " (out of " << numStarts << ") " << endl;
                cout << " Best" << (_numLegalSolns && _numLegalSolns != numStarts ? " legal" : "") << " solution:  " << _solutions[_bestSolnNum]->cost << " ";
                cout << " Average:  " << _aveCost << " StdDev: " << _stdDev << endl;

                unsigned k;
                cout << " Areas: ";
                for (k = 0; k < _solutions[_bestSolnNum]->partArea.size(); k++) cout << _solutions[_bestSolnNum]->partArea[k][0] << "  ";

                cout << endl << " Pins: ";
                for (k = 0; k < _solutions[_bestSolnNum]->partPins.size(); k++) cout << _solutions[_bestSolnNum]->partPins[k] << "  ";
                cout << endl << endl;
        }

        basePartTimer.stop();
        if (_params.verb.getForSysRes() > 3) cout << _solutions.size() << " flat starts took " << basePartTimer << endl;

        _userTime = basePartTimer.getUserTime();
}

void MultiStartPartitioner::setBestSolAndAverageCost() {
        _aveCost = 0.0;
        _stdDev = 0.0;
        vector<double> forStdDev;
        _bestCost = UINT_MAX;
        _bestSolnNum = UINT_MAX;

        double totalCostOfLegalSol = 0.0;
        _numLegalSolns = 0;
        double smallestOverFill = DBL_MAX;
        abkfatal(_solutions.size(), " No solutions available ");

        const vector<vector<double> >& maxAreas = _problem.getMaxCapacities();
        // vector<vector<double> >&  minAreas=   _problem.getMinCapacities();
        // vector<vector<double> >&  targetAreas=_problem.getCapacities();

        PartitioningBuffer& buf = _problem.getSolnBuffers();
        unsigned solnBegin = buf.beginUsedSoln();
        unsigned solnEnd = buf.endUsedSoln();
        vector<double>& imbals = const_cast<vector<double>&>(_problem.getImbalances());

        if (_params.verb.getForMajStats() > 0) cout << " MultiStart costs : ";
        for (unsigned s = solnBegin; s != solnEnd; s++) {
                _aveCost += _solutions[s]->cost;
                forStdDev.push_back(_solutions[s]->cost);
                if (_params.verb.getForMajStats() > 0) cout << _solutions[s]->cost << "  ";
                double overfill = 0.;
                for (unsigned k = 0; k < _solutions[s]->partArea.size(); k++) {
                        if (greaterThanDouble(_solutions[s]->partArea[k][0], maxAreas[k][0])) {
                                double partOverfill = _solutions[s]->partArea[k][0] - maxAreas[k][0];
                                if (greaterThanDouble(partOverfill, overfill)) overfill = partOverfill;
                        }
                }

                if (equalDouble(overfill, 0.)) {
                        totalCostOfLegalSol += _solutions[s]->cost;
                        if (_numLegalSolns == 0  // i.e. this is the first legal
                                                 // solution seen
                            ||
                            lessThanDouble(_solutions[s]->cost, _bestCost))  // i.e. it's the best
                        {
                                _bestCost = _solutions[s]->cost;
                                _bestSolnNum = s;
                        } else if (equalDouble(_solutions[s]->cost, _bestCost) && lessThanDouble(imbals[s], imbals[_bestSolnNum]) || (equalDouble(imbals[s], imbals[_bestSolnNum]) && lessThanDouble(_solutions[s]->pinImbalance, _solutions[_bestSolnNum]->pinImbalance))) {
                                _bestCost = _solutions[s]->cost;
                                _bestSolnNum = s;
                        }
                        _numLegalSolns++;
                } else if (_numLegalSolns == 0)
                    // if  there were no legal solns, seek best illegal
                {
                        if (lessThanDouble(overfill, smallestOverFill))  // i.e. it's not
                                                                         // as violating
                                                                         // as others
                        {
                                smallestOverFill = overfill;
                                _bestCost = _solutions[s]->cost;
                                _bestSolnNum = s;
                        }
                }
        }

        if (_params.verb.getForMajStats() > 0) cout << endl;

        abkfatal(_bestSolnNum < UINT_MAX, " Failed to find best solution ");
        _problem.setBestSolnNum(_bestSolnNum);
        if (_numLegalSolns)
                _aveCost = totalCostOfLegalSol / _numLegalSolns;
        else
                _aveCost /= (solnEnd - solnBegin);
        for (unsigned k = 0; k != forStdDev.size(); k++) _stdDev += square(_aveCost - forStdDev[k]);
        _stdDev = sqrt(_stdDev / forStdDev.size());

        for (int ctr = _solutions[_bestSolnNum]->partPins.size() - 1; ctr >= 0; --ctr) {
                _solutions[_bestSolnNum]->partPins[ctr] = 0;
        }

        Partitioning& part = _solutions[_bestSolnNum]->part;

        const HGraphFixed& hg = _problem.getHGraph();
        unsigned nParts = _solutions[_bestSolnNum]->numPartitions;
        for (unsigned n = 0; n != hg.getNumNodes(); n++) {
                unsigned deg = hg.getNodeByIdx(n).getDegree();
                for (int pt = nParts - 1; pt >= 0; --pt) {
                        if (part[n].isInPart(pt)) _solutions[_bestSolnNum]->partPins[pt] += deg;
                }
        }

        // Now set pin balances of the best solution
}

MultiStartPartitioner::Parameters::Parameters(const Parameters& params)
    : verb(params.verb),
      moveMan(params.moveMan),
      eval(params.eval),
      solnGen(params.solnGen),
      legalizeOnly(params.legalizeOnly),
      useClip(params.useClip),
      useEarlyStop(params.useEarlyStop),
      maxHillHeightFactor(params.maxHillHeightFactor),
      maxHillWidth(params.maxHillWidth),
      printHillStats(params.printHillStats),
      maxNumMoves(params.maxNumMoves),
      minPassImprovement(params.minPassImprovement),
      maxNumPasses(params.maxNumPasses),
      relaxedTolerancePasses(params.relaxedTolerancePasses),
      unCorking(params.unCorking),
      skipNetsLargerThan(params.skipNetsLargerThan),
      skipLargestNets(params.skipLargestNets),
      randomized(params.randomized),
      doFirstLIFOPass(params.doFirstLIFOPass),
      allowCorkingNodes(params.allowCorkingNodes),
      allowIllegalMoves(params.allowIllegalMoves),
      tieBreakOnPins(params.tieBreakOnPins),
      wiggleTerms(params.wiggleTerms),
      saveMoveLog(params.saveMoveLog),
      useWts(params.useWts),
      mapBasedGC(params.mapBasedGC),
      hashBasedGC(params.hashBasedGC),
      noHashBasedGC(params.noHashBasedGC),
      saveMem(params.saveMem) {}
