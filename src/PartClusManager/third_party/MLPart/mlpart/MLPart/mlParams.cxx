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

//  Adapted by Igor Markov from an earlier prototype by Andrew Caldwell
//  March 3, 1998

//  Reworked for standalone ML by Igor Markov and Andy Caldwell, March 30, 1998

// CHANGES
//  980912 aec version4.0 with ClusteredHGraph - taken from baseMLPart.cxx
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "ABKCommon/abkcommon.h"
#include "baseMLPart.h"
#include "Partitioning/partitioning.h"
#include <iomanip>

using std::ostream;
using std::cout;
using std::endl;

#ifdef _MSC_VER
#pragma warning(disable : 4800)
#endif

MLPartParams::Parameters(int argc, const char *argv[]) : PartitionerParams(argc, argv), savePartProb(NeverSave), flatPartitioner(argc, argv), useBBonTop(false), partFuzziness(0), runsPerClTree(1), solnPoolOnTopLevel(3), toleranceMultiple(2), toleranceAlpha(0), useTwoPartCalls(TOPONLY), netThreshold(0), Vcycling(Exclusive), timeLimit(0), expPrint2Costs(false), clusterToTerminals(false), seedTopLvlSoln(false), pruningPercent(10000), pruningPoint(30), maxNumPassesAtBottom(0), maxPassesAfterTopLevels(2), vcNumFailures(1), vcImproveRatio(1), vc1ClusterRatio(2), vc1FirstLevel(200), vc1LevelGrowth(2), vc2ClusterRatio(1.3), vc2FirstLevel(200), vc2LevelGrowth(2), useFMPartPlus(false), lastVCycleStartThreshold(DBL_MAX), lastVCycleImproveThreshold(2. / 300.), clParams(argc, argv) {
        // meta-options
        BoolParam ispd_("ispd05", argc, argv);
        UnsignedParam tryharder_("tryHarder", argc, argv);
        UnsignedParam faster_("faster", argc, argv);

        if (ispd_.found()) {
                // nothing here just yet
        }

        if (tryharder_.found()) {
                //     runsPerClTree = 10;
        }

        if (faster_.found()) {
        }

        DoubleParam maxHillHeightOption("maxHillHeightFactor", argc, argv);
        UnsignedParam useClipOption("useClip", argc, argv);

        if (!maxHillHeightOption.found())  // default in ML is 2.0
        {
                useEarlyStop = true;
                maxHillHeightFactor = 2.0;
        }

        if (!useClipOption.found()) useClip = 0;

        BoolParam helpRequest1("h", argc, argv);
        BoolParam helpRequest2("help", argc, argv);

        if (helpRequest1.found() || helpRequest2.found()) {
                cout << "== Multilevel partitioner parameters == \n"
                     << " -savePartProb   NeverSave   | AtAllLastLevels | "
                        "AtAllLevels\n"
                     << "         AtFirstLevelOfFirst | AtLastLevelOfFirst "
                     << " | AtAllLevelsOfFirst \n"
                     << " -useBBonTop                          \n"
                     << " -partFuzziness       <double 0-100%> \n"
                     << " -mlRunsPerClustering <unsigned> [1]\n"
                     << " -solnPoolOnTopLevel  <unsigned> [3] (to init. each "
                        "soln. thread)\n"
                     << " -toleranceMultiple   <double >= 1.00> [2] \n"
                     << " -toleranceAlpha      <double 0.0 <= A <= 1.00> [0] \n"
                     << " -useTwoPartCalls {ALL | TOPONLY | ALLBUTLAST | NEVER "
                        "} \n"
                     << " -mlNetThreshold       <unsigned>  (0 for \"none\")\n"
                     << " -mlClusterToTerminals <bool> \n"
                     << " -seedTopLvlSoln     <bool> [false] \n"
                     << "\n   Vcycling Parameters: \n"
                     << " -mlVcycling          "
                        "<NoVcycles|Exclusive|Relaxed|Relaxed2|Comprehensive|"
                        "Initial>\n"
                     << " -vcNumFailures       <unsigned [1/2]> \n"
                     << " -vcImproveRatio      <double   >= 1.0> \n"
                     << " -vc1ClusterRatio     <double   >1.0> \n"
                     << " -vc1FirstLevel       <unsigned>   \n"
                     << " -vc1LevelGrowth      <double   >1.0>  \n"
                     << " -vc2ClusterRatio     <double   >1.0> \n"
                     << " -vc2FirstLevel       <unsigned> \n"
                     << " -vc2LevelGrowth      <double   >1.0> \n"
                     << " -useFMPartPlus       <bool> \n"
                     << " -lastVCycleImproveThreshold  <double 0-100%> \n\n"
                     << " -mlPruningPercent    <unsigned %>     [0]  \n"
                     << " -mlPruningPoint      <double %>       [30%]\n"
                     << " -mlTimeLimit         <double sec> \n";
                return;
        }

        StringParam savePartProb_("savePartProb", argc, argv);
        BoolParam doFlat_("doFlat", argc, argv);
        DoubleParam partFuzziness_("partFuzziness", argc, argv);
        BoolParam useBBonTop_("useBBonTop", argc, argv);
        UnsignedParam runsPerClTree_("mlRunsPerClustering", argc, argv);
        UnsignedParam solnPoolOnTopLevel_("solnPoolOnTopLevel", argc, argv);
        DoubleParam toleranceMultiple_("toleranceMultiple", argc, argv);
        DoubleParam toleranceAlpha_("toleranceAlpha", argc, argv);
        StringParam useTwoPartCalls_("useTwoPartCalls", argc, argv);
        StringParam Vcycling_("mlVcycling", argc, argv);
        UnsignedParam netThreshold_("mlNetThreshold", argc, argv);
        BoolParam timeLimit_("mlTimeLimit", argc, argv);
        UnsignedParam clusterToTerminals_("mlClusterToTerminals", argc, argv);
        UnsignedParam pruningPercent_("mlPruningPercent", argc, argv);
        DoubleParam pruningPoint_("mlPruningPoint", argc, argv);
        UnsignedParam maxNumPassesAtBottom_("maxNumPassesAtBottom", argc, argv);
        UnsignedParam maxPassesAfterTopLevels_("maxPassesAfterTopLevels", argc, argv);

        BoolParam seedTopLvlSoln_("seedTopLvlSoln", argc, argv);

        UnsignedParam vcNumFailures_("vcNumFailures", argc, argv);
        DoubleParam vcImproveRatio_("vcImproveRatio", argc, argv);

        DoubleParam vc1ClusterRatio_("vc1ClusterRatio", argc, argv);
        UnsignedParam vc1FirstLevel_("vc1FirstLevel", argc, argv);
        DoubleParam vc1LevelGrowth_("vc1LevelGrowth", argc, argv);

        DoubleParam vc2ClusterRatio_("vc2ClusterRatio", argc, argv);
        UnsignedParam vc2FirstLevel_("vc2FirstLevel", argc, argv);
        DoubleParam vc2LevelGrowth_("vc2LevelGrowth", argc, argv);

        BoolParam useFMPartPlus_("useFMPartPlus", argc, argv);

        DoubleParam lastVCycleImproveThreshold_("lastVCycleImproveThreshold", argc, argv);

#define SEARCH_FOR1(NAME) \
        if (!newstrcasecmp(savePartProb_, #NAME)) savePartProb = NAME;

        if (savePartProb_.found()) {
                SEARCH_FOR1(NeverSave)
                else SEARCH_FOR1(AtAllLastLevels) else SEARCH_FOR1(AtAllLevels) else SEARCH_FOR1(AtFirstLevelOfFirst) else SEARCH_FOR1(AtLastLevelOfFirst) else abkfatal3(0,
                                                                                                                                                                          " -savePartProb "
                                                                                                                                                                          "followed by an unkown "
                                                                                                                                                                          "mode \"",
                                                                                                                                                                          savePartProb_, "\" in command line\n");
        }

#define SEARCH_FOR2(NAME) \
        if (!newstrcasecmp(Vcycling_, #NAME)) Vcycling = NAME;

        if (Vcycling_.found()) {
                SEARCH_FOR2(NoVcycles)
                else SEARCH_FOR2(Exclusive) else SEARCH_FOR2(Relaxed2) else SEARCH_FOR2(Relaxed) else SEARCH_FOR2(Comprehensive) else SEARCH_FOR2(Initial) else abkfatal3(0,
                                                                                                                                                                          " -"
                                                                                                                                                                          "ml"
                                                                                                                                                                          "Vc"
                                                                                                                                                                          "yc"
                                                                                                                                                                          "li"
                                                                                                                                                                          "ng"
                                                                                                                                                                          " f"
                                                                                                                                                                          "ol"
                                                                                                                                                                          "lo"
                                                                                                                                                                          "we"
                                                                                                                                                                          "d "
                                                                                                                                                                          "by"
                                                                                                                                                                          " a"
                                                                                                                                                                          "n "
                                                                                                                                                                          "un"
                                                                                                                                                                          "ko"
                                                                                                                                                                          "wn"
                                                                                                                                                                          " m"
                                                                                                                                                                          "od"
                                                                                                                                                                          "e "
                                                                                                                                                                          "\"",
                                                                                                                                                                          Vcycling_,
                                                                                                                                                                          "\""
                                                                                                                                                                          " i"
                                                                                                                                                                          "n "
                                                                                                                                                                          "co"
                                                                                                                                                                          "mm"
                                                                                                                                                                          "an"
                                                                                                                                                                          "d "
                                                                                                                                                                          "li"
                                                                                                                                                                          "ne"
                                                                                                                                                                          "\n");
        }

        if (doFlat_.found()) clParams.sizeOfTop = 0;
        if (useBBonTop_.found()) useBBonTop = true;
        if (partFuzziness_.found()) partFuzziness = partFuzziness_;

        if (runsPerClTree_.found()) runsPerClTree = runsPerClTree_;
        if (solnPoolOnTopLevel_.found()) solnPoolOnTopLevel = solnPoolOnTopLevel_;
        if (toleranceMultiple_.found()) toleranceMultiple = toleranceMultiple_;
        if (toleranceAlpha_.found()) toleranceAlpha = toleranceAlpha_;

        if (useTwoPartCalls_.found()) {
                if (!newstrcasecmp(useTwoPartCalls_, "ALL"))
                        useTwoPartCalls = ALL;
                else if (!newstrcasecmp(useTwoPartCalls_, "TOPONLY"))
                        useTwoPartCalls = TOPONLY;
                else if (!newstrcasecmp(useTwoPartCalls_, "ALLBUTLAST"))
                        useTwoPartCalls = ALLBUTLAST;
                else if (!newstrcasecmp(useTwoPartCalls_, "NEVER"))
                        useTwoPartCalls = NEVER;
                else
                        abkfatal(0, "unknown option for useTwoPartCalls");
        }

        if (netThreshold_.found()) netThreshold = netThreshold_;
        if (timeLimit_.found()) timeLimit = timeLimit_;

        if (clusterToTerminals_.found()) clusterToTerminals = clusterToTerminals_;
        if (pruningPercent_.found()) pruningPercent = pruningPercent_;
        if (pruningPoint_.found()) pruningPoint = pruningPoint_;

        if (maxNumPassesAtBottom_.found()) maxNumPassesAtBottom = maxNumPassesAtBottom_;

        if (maxPassesAfterTopLevels_.found()) maxPassesAfterTopLevels = maxPassesAfterTopLevels_;

        seedTopLvlSoln = seedTopLvlSoln_.found();

        if (vcNumFailures_.found()) vcNumFailures = vcNumFailures_;
        if (vcImproveRatio_.found()) vcImproveRatio = vcImproveRatio_;

        if (vc1ClusterRatio_.found()) vc1ClusterRatio = vc1ClusterRatio_;
        if (vc1FirstLevel_.found()) vc1FirstLevel = vc1FirstLevel_;
        if (vc1LevelGrowth_.found()) vc1LevelGrowth = vc1LevelGrowth_;

        if (vc2ClusterRatio_.found()) vc2ClusterRatio = vc2ClusterRatio_;
        if (vc2FirstLevel_.found()) vc2FirstLevel = vc2FirstLevel_;
        if (vc2LevelGrowth_.found()) vc2LevelGrowth = vc2LevelGrowth_;

        abkfatal(vcNumFailures == 1 || vcNumFailures == 2, "vcNumFailures must == 1 or 2");
        abkfatal(vcImproveRatio >= 1.0, "vc[1/2]ImproveRatio must be >= 1.0");

        abkfatal(vc1ClusterRatio > 1.0 && vc2ClusterRatio > 1.0, "vc[1/2]ClusterRatio must be > 1.0");
        abkfatal(vc1LevelGrowth > 1.0 && vc2LevelGrowth > 1.0, "vc[1/2]LevelGrowth must be > 1.0");

        if (useFMPartPlus_.found()) useFMPartPlus = true;

        if (lastVCycleImproveThreshold_.found()) lastVCycleImproveThreshold = lastVCycleImproveThreshold_ / 100.;
}

ostream &operator<<(ostream &os, const MLPartParams &parms) {
        const char *VcyclingTypes[6] = {"NoVcycles", "Exclusive", "Relaxed", "Relaxed2", "Comprehensive", "Initial"};
        const char *tf[2] = {"false", "true"};
        const char *twoPartStr[4] = {"All", "TopOnly", "AllButLast", "Never"};

        cout << " Multi-Level Partitioner Parameters : " << endl << static_cast<PartitionerParams>(parms) << " Flat partitioner              " << parms.flatPartitioner << "\n"
             << " Use B&B at top level          " << tf[parms.useBBonTop] << "\n"
             << " Partition fuzziness:          " << parms.partFuzziness << "\n"
             << " Runs per clustering:          " << parms.runsPerClTree << "\n"
             << " Solution pool on top level    " << parms.solnPoolOnTopLevel << "\n"
             << " Tolerance Multiple:           " << parms.toleranceMultiple << "\n"
             << " Tolerance Alpha:              " << parms.toleranceAlpha << "\n"
             << " Use Two Partitioner Calls     " << twoPartStr[parms.useTwoPartCalls] << "\n"
             << " Max num passes at bottom:     " << parms.maxNumPassesAtBottom << "\n"
             << " Max passes after top levels:  " << parms.maxPassesAfterTopLevels << "\n"
             << " Cluster to terminals:         " << tf[parms.clusterToTerminals] << "\n"
             << " Seed Top Level Solution:      " << tf[parms.seedTopLvlSoln] << "\n"
             << " V-cycling type:               " << VcyclingTypes[parms.Vcycling] << "\n"
             << " vcNumFailures:                " << parms.vcNumFailures << "\n"
             << " vcImproveRatio:               " << parms.vcImproveRatio << "\n"
             << " vc1ClusterRatio:              " << parms.vc1ClusterRatio << "\n"
             << " vc1FirstLevel:                " << parms.vc1FirstLevel << "\n"
             << " vc1LevelGrowth:               " << parms.vc1LevelGrowth << "\n"
             << " vc2ClusterRatio:              " << parms.vc2ClusterRatio << "\n"
             << " vc2FirstLevel:                " << parms.vc2FirstLevel << "\n"
             << " vc2LevelGrowth:               " << parms.vc2LevelGrowth << "\n"
             << " useFMPartPlus:                " << tf[parms.useFMPartPlus] << "\n"
             << " Pruning Percentage:           ";
        if (parms.pruningPercent >= 1000)
                cout << "No Pruning \n";
        else
                cout << parms.pruningPercent << "\n";
        cout << " Pruning Point:               " << parms.pruningPoint << "%\n"
             << " Net threshold (max net size): " << parms.netThreshold << "\n"
             << " Time limit (sec):             " << parms.timeLimit << "\n"
             << " Clustering Parameters \n" << parms.clParams << endl;

        cout << "solnGen " << parms.solnGen << endl;
        return os;
}
