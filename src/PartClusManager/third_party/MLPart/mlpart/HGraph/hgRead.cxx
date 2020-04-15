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

// CHANGES
// ilm 293000  in addition to '\n', the parser needs to check for '\r'

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/abkcommon.h>
#include "newcasecmp.h"
#include <ABKCommon/pathDelims.h>
#include <Ctainers/bitBoard.h>
#include <HGraph/hgFixed.h>
#include <cstdio>
#include <string>
#include <sstream>

using uofm::stringstream;
using uofm::string;
using std::ifstream;
using std::pair;
using std::cout;
using std::cerr;
using std::endl;
using uofm::vector;

vector<pair<HGFNode*, char> > _USELESS_declaration_for_compilerBut_;
pair<HGFNode*, char> _USELESS_declaration_for_compilerBut_NUmber2_;

HGraphFixed::HGraphFixed(const char* Filename1, const char* Filename2, const char* Filename3, HGraphParameters param) : HGraphBase(param) {
        abkfatal(Filename1, "null char* for first filename in HGraph ctor");

        const char* auxFileName = 0;
        const char* netDFileName = 0;
        const char* areMFileName = 0;
        const char* wtsFileName = 0;
        const char* netsFileName = 0;
        const char* nodesFileName = 0;

        const char* files[3] = {Filename1, Filename2, Filename3};

        for (unsigned f = 0; f < 3; f++) {
                if (!files[f]) continue;

                if (strstr(files[f], ".aux"))
                        auxFileName = files[f];
                else if (strstr(files[f], ".wts"))
                        wtsFileName = files[f];
                else if (strstr(files[f], ".are"))
                        areMFileName = files[f];
                else if (strstr(files[f], ".nodes"))
                        nodesFileName = files[f];
                else if (strstr(files[f], ".netD"))
                        netDFileName = files[f];
                else if (strstr(files[f], ".nets"))
                        netsFileName = files[f];
                else if (strstr(files[f], ".net"))
                        netDFileName = files[f];
        }

        abkfatal(!((netDFileName || areMFileName) && (netsFileName || nodesFileName || wtsFileName)), "can't mix netD/areM and nets/nodes/wts");

        if (auxFileName)
                parseAux(auxFileName);
        else if (netDFileName) {
                readNetD(netDFileName);
                if (areMFileName)
                        readAreM(areMFileName);
                else {  // set unit weights
                        abkfatal(_numMultiWeights == 0, "there should be no weights here");
                        _numMultiWeights = _numTotalWeights = 1;
                        _multiWeights = vector<HGWeight>(_nodes.size(), 1.0);
                        cout << "are(M) file not found. loading unit weights "
                                "for all nodes" << endl;
                }
        } else if (nodesFileName) {
                abkfatal(netsFileName, "must have both a nodes and nets file");

                readNodes(nodesFileName);
                readNets(netsFileName);
                if (wtsFileName)
                        readWts(wtsFileName);
                else {  // set unit weights
                        abkfatal(_numMultiWeights == 0, "there should be no weights here");
                        _numMultiWeights = _numTotalWeights = 1;
                        _multiWeights = vector<HGWeight>(_nodes.size(), 1.0);
                }
        } else {
                cerr << "aux, netD or nets file not found.  ";
                cerr << "Filename arguments were:" << endl;
                if (Filename1) cout << Filename1 << endl;
                if (Filename2) cout << Filename2 << endl;
                if (Filename3) cout << Filename3 << endl;
        }
        finalize();
}

void HGraphFixed::parseAux(const char* baseFileName) {
        ifstream auxFile(baseFileName);
        char* newNetD = NULL;
        char* newAreM = NULL;
        char* newNets = NULL;
        char* newNodes = NULL;
        char* newWts = NULL;

        uofm::string dir;

        const char* auxFileDirEnd = strrchr(baseFileName, pathDelim);
        if (auxFileDirEnd) {
                string tmp = baseFileName;
                tmp.resize(auxFileDirEnd - baseFileName + 1);
                dir = tmp;
        }

        if (auxFile) {
                bool found = false;
                int lineno = 1;
                cout << "Reading " << baseFileName << endl;
                char oneLine[1023];
                char word1[100], word2[100];

                while (!found && !auxFile.eof()) {
                        auxFile >> eathash(lineno) >> word1 >> noeol(lineno) >> word2 >> eatblank >> noeol(lineno);
                        abkfatal2(!strcmp(word2, ":"),
                                  " Error in aux file: space-separated column "
                                  "expected. got:",
                                  word2);
                        if (!newstrcasecmp(word1, "CD")) {
                                auxFile >> word1;
                                auxFile >> needeol(lineno++);
                                if (word1[0] == pathDelimWindows || word1[0] == pathDelimUnix)
                                        dir = word1;
                                else
                                        dir += word1;
                                char fDel[2];
                                sprintf(fDel, "%c", pathDelim);
                                if (word1[strlen(word1) - 1] != pathDelimWindows || word1[0] == pathDelimUnix) dir += fDel;
                        } else if (!newstrcasecmp(word1, "HGraph") || !newstrcasecmp(word1, "HGraphWPins") || !newstrcasecmp(word1, "PartProb") || !newstrcasecmp(word1, "FPPROBLEM") || !newstrcasecmp(word1, "RowBasedPlacement"))  // used in
                                                                                                                                                                                                                       // RBPl
                                                                                                                                                                                                                       // ctor
                        {
                                found = true;
                                auxFile.getline(oneLine, 1023);
                                unsigned len = strlen(oneLine), fileNum = 0;
                                bool space = true;
                                char* fileNames[6] = {NULL, NULL, NULL, NULL, NULL, NULL};

                                unsigned j;
                                for (j = 0; j != len; j++) {
                                        if (isspace(oneLine[j])) {
                                                if (!space) oneLine[j] = '\0';
                                                space = true;
                                        } else if (space) {
                                                space = false;
                                                abkfatal(fileNum < 6,
                                                         " Too many filenames "
                                                         "in AUX file");
                                                fileNames[fileNum++] = oneLine + j;
                                        }
                                }

                                for (j = 0; j != fileNum; j++) {
                                        if (strstr(fileNames[j], ".nets"))
                                                newNets = fileNames[j];
                                        else if (strstr(fileNames[j], ".NETS"))
                                                newNets = fileNames[j];
                                        else if (strstr(fileNames[j], ".nodes"))
                                                newNodes = fileNames[j];
                                        else if (strstr(fileNames[j], ".NODES"))
                                                newNodes = fileNames[j];
                                        else if (strstr(fileNames[j], ".wts"))
                                                newWts = fileNames[j];
                                        else if (strstr(fileNames[j], ".WTS"))
                                                newWts = fileNames[j];
                                        else if (strstr(fileNames[j], ".net"))
                                                newNetD = fileNames[j];
                                        else if (strstr(fileNames[j], ".NET"))
                                                newNetD = fileNames[j];
                                        else if (strstr(fileNames[j], ".are"))
                                                newAreM = fileNames[j];
                                        else if (strstr(fileNames[j], ".ARE"))
                                                newAreM = fileNames[j];
                                }

                                abkfatal(!((newNetD || newAreM) && (newNets || newNodes || newWts)), "can't mix netD/areM and nets/nodes/wts");

                                char* tmpNetD = NULL;
                                char* tmpAreM = NULL;
                                char* tmpNets = NULL;
                                char* tmpNodes = NULL;
                                char* tmpWts = NULL;

                                unsigned dirLen = dir.size();

                                if (newNetD != NULL)
                                        if (newNetD[0] != pathDelimWindows && newNetD[0] != pathDelimUnix) {
                                                tmpNetD = new char[dirLen + strlen(newNetD) + 1];
                                                sprintf(tmpNetD, "%s%s", dir.c_str(), newNetD);
                                        } else {
                                                tmpNetD = new char[strlen(newNetD) + 1];
                                                strcat(tmpNetD, newNetD);
                                        }

                                if (newAreM != NULL)
                                        if (newAreM[0] != pathDelimWindows && newAreM[0] != pathDelimUnix) {
                                                tmpAreM = new char[dirLen + strlen(newAreM) + 1];
                                                sprintf(tmpAreM, "%s%s", dir.c_str(), newAreM);
                                        } else {
                                                tmpAreM = new char[strlen(newAreM) + 1];
                                                strcat(tmpAreM, newAreM);
                                        }

                                if (newWts != NULL)
                                        if (newWts[0] != pathDelimWindows && newWts[0] != pathDelimUnix) {
                                                tmpWts = new char[dirLen + strlen(newWts) + 1];
                                                sprintf(tmpWts, "%s%s", dir.c_str(), newWts);
                                        } else {
                                                tmpWts = new char[strlen(newWts) + 1];
                                                strcat(tmpWts, newWts);
                                        }

                                if (newNets != NULL)
                                        if (newNets[0] != pathDelimWindows && newNets[0] != pathDelimUnix) {
                                                tmpNets = new char[dirLen + strlen(newNets) + 1];
                                                sprintf(tmpNets, "%s%s", dir.c_str(), newNets);
                                        } else {
                                                tmpNets = new char[strlen(newNets) + 1];
                                                strcat(tmpNets, newNets);
                                        }

                                if (newNodes != NULL)
                                        if (newNodes[0] != pathDelimWindows && newNodes[0] != pathDelimUnix) {
                                                tmpNodes = new char[dirLen + strlen(newNodes) + 1];
                                                sprintf(tmpNodes, "%s%s", dir.c_str(), newNodes);
                                        } else {
                                                tmpNodes = new char[strlen(newNodes) + 1];
                                                strcat(tmpNodes, newNodes);
                                        }

                                if (tmpNetD) {
                                        readNetD(tmpNetD);
                                        if (tmpAreM)
                                                readAreM(tmpAreM);
                                        else {  // set unit weights
                                                abkfatal(_numMultiWeights == 0,
                                                         "there should be no "
                                                         "weights here");
                                                _numMultiWeights = _numTotalWeights = 1;
                                                _multiWeights = vector<HGWeight>(_nodes.size(), 1.0);
                                                cout << "are(M) file not "
                                                        "found. loading unit "
                                                     << "weights for all nodes" << endl;
                                        }
                                } else if (tmpNodes) {
                                        abkassert(tmpNets, "must have .nets file with .nodes");
                                        readNodes(tmpNodes);
                                        readNets(tmpNets);
                                        if (tmpWts)
                                                readWts(tmpWts);
                                        else {  // set unit weights
                                                abkfatal(_numMultiWeights == 0,
                                                         "there should be no "
                                                         "weights here");
                                                _numMultiWeights = _numTotalWeights = 1;
                                                _multiWeights = vector<HGWeight>(_nodes.size(), 1.0);
                                        }
                                } else
                                        abkfatal(0,
                                                 "found neither net[D] or "
                                                 "nodes files");

                                if (tmpNetD) delete[] tmpNetD;
                                if (tmpAreM) delete[] tmpAreM;
                                if (tmpWts) delete[] tmpWts;
                                if (tmpNets) delete[] tmpNets;
                                if (tmpNodes) delete[] tmpNodes;
                        }
                }
                cout << "Finished reading " << baseFileName << " and referenced files" << endl;
        } else
                abkfatal2(0, baseFileName, "  not found");
}

void HGraphFixed::readNetD(const char* netDFileName) {
        ifstream netD(netDFileName);
        abkfatal2(netD, " Could not open ", netDFileName);

        char t;
        netD >> t;  // eat 1st 0
        unsigned numNets, numModules, padOffset, numWeights = 0;
        netD >> _numPins >> numNets >> numModules >> padOffset;
        if (padOffset == 0) padOffset = numModules - 1;
        _numTerminals = numModules - padOffset - 1;

        init(numModules, numWeights);

        for (unsigned n = 0; n < numModules; n++) {
                stringstream nameBuff;

                if (n < _numTerminals) {
                        nameBuff << "p" << n + 1;
                } else {
                        nameBuff << "a" << n - _numTerminals;
                }

                _nodeNames.push_back(nameBuff.str());
                _nodeNamesMap[nameBuff.str()] = n;
        }

        HGFEdge* edge = 0;
        int lineno = 6;
        char tmp[1024];
        vector<pair<HGFNode*, char> > netPins;

        unsigned numEdgesSeen = 0;

        while (netD >> tmp) {
                if (!*tmp) break;
                netD >> t;

                t = toupper(t);
                if (t == 'S') {
                        if (++numEdgesSeen > numNets) {
                                abkwarn3(0, "line ", lineno, ": extra edges in .netD file");
                                break;
                        }

                        if (netPins.size() > 1)  // at the start of a new net,
                                                 // construct the one that just finished
                        {
                                edge = addEdge();
                                for (unsigned i = 0; i < netPins.size(); i++) switch (netPins[i].second) {
                                                case 'O':
#ifdef SIGNAL_DIRECTIONS
                                                        addSrc(*netPins[i].first, *edge);
                                                        break;
#endif
                                                case 'I':
#ifdef SIGNAL_DIRECTIONS
                                                        addSnk(*netPins[i].first, *edge);
                                                        break;
#endif
                                                case 'B':
                                                case '1':
                                                case 'L':  // !!! why it happens???
                                                        addSrcSnk(*netPins[i].first, *edge);
                                                        break;
                                                default:  // error
                                                        abkfatal3(0, t,
                                                                  ": "
                                                                  "unexpected "
                                                                  "symbol "
                                                                  "encountered "
                                                                  "in line ",
                                                                  lineno);
                                        }
                        }

                        netPins.clear();
                }
                netD >> t;

                char origchar = t;
                t = toupper(t);

                if (netD)  // it may be that we just finished the file
                {
                        if (t != 'B' && t != 'I' && t != 'O' && t != '1') {
                                netD.putback(origchar);
                                abkfatal(netD, "could not putback to stream");
                                t = '1';
                        }
                }

                unsigned nodeIdx;
                if (tmp[0] == 'p') {
                        nodeIdx = static_cast<unsigned>(atoi(tmp + 1)) - 1;
                } else {
                        nodeIdx = static_cast<unsigned>(atoi(tmp + 1)) + _numTerminals;
                }

                HGFNode& node = getNodeByIdx(nodeIdx);

                //      if(_param.makeAllSrcSnk && (t=='O' || t=='I') ) t='B';

                netPins.push_back(pair<HGFNode*, char>(&node, t));
                lineno++;
        }

        if (netPins.size() > 1) {
                edge = addEdge();
                for (unsigned i = 0; i < netPins.size(); i++) switch (netPins[i].second) {
                                case 'O':
#ifdef SIGNAL_DIRECTIONS
                                        addSrc(*netPins[i].first, *edge);
                                        break;
#endif
                                case 'I':
#ifdef SIGNAL_DIRECTIONS
                                        addSnk(*netPins[i].first, *edge);
                                        break;
#endif
                                case 'B':
                                case '1':
                                case 'L':  // !!! why it happens???
                                        addSrcSnk(*netPins[i].first, *edge);
                                        break;
                                default:  // error
                                        abkfatal3(0, t,
                                                  ": unexpected symbol "
                                                  "encountered in line ",
                                                  lineno);
                        }
        }
        _netNames = vector<uofm::string>(numEdgesSeen, uofm::string());

        abkwarn3(numEdgesSeen == numNets, "missing ", numNets - numEdgesSeen, " nets in .netD file");
        {
                int ii = -1;
                netD >> eathash(ii);
                abkwarn(!netD, "trailing lines in .netD file");
        }
}

void HGraphFixed::readAreM(const char* areMFileName) {
        ifstream areM(areMFileName);
        abkfatal2(areM, " Could not open ", areMFileName);

        char cName[1024];  // cell name
        areM >> cName;     // skip cell name this time....
        areM >> cName;
        _numMultiWeights = 0;

        while (_numMultiWeights <= 100 && isdigit(cName[0])) {
                areM >> cName;
                _numMultiWeights++;
        }
        _numTotalWeights = _numMultiWeights;
        areM.close();
        areM.open(areMFileName);
        abkfatal(_numMultiWeights < 100, "error: there appear to be > 100 weights");
        abkfatal(_multiWeights.size() == 0, "multiWeights should be empty");

        _multiWeights = vector<HGWeight>(getNumNodes() * _numMultiWeights);

        int lineno = 0;

        while (areM >> cName) {
                if (!*cName) break;

                unsigned nodeIdx;
                if (cName[0] == 'p') {
                        nodeIdx = static_cast<unsigned>(atoi(cName + 1)) - 1;
                } else {
                        nodeIdx = static_cast<unsigned>(atoi(cName + 1)) + _numTerminals;
                }

                HGFNode& node = getNodeByIdx(nodeIdx);
                unsigned wtNum = 0;
                double nextWt;
                while (!areM.eof() && areM.peek() != '\n' && areM.peek() != '\r') {
                        areM >> eatblank >> nextWt >> eatblank;
                        setWeight(node.getIndex(), nextWt, wtNum++);
                }
                lineno++;
        }
        areM.close();
}

void HGraphFixed::readNodes(const char* nodesFileName) {
        ifstream nodes(nodesFileName);
        abkfatal2(nodes, " Could not open ", nodesFileName);
        cout << " Reading " << nodesFileName << " ... " << endl;

        int lineno = 1;
        nodes >> needcaseword("UCLA") >> needcaseword("nodes") >> needword("1.0") >> skiptoeol >> eathash(lineno);

        unsigned numNodes;
        nodes >> needcaseword("NumNodes") >> needword(":") >> my_isnumber(lineno) >> numNodes >> skiptoeol;
        lineno++;

        nodes >> needcaseword("NumTerminals") >> needword(":") >> my_isnumber(lineno) >> _numTerminals >> skiptoeol;
        lineno++;

        init(numNodes, 0);

        // char cName[1024];
        string cName;
        vector<string> termNodeNames;
        vector<string> nonTermNodeNames;
        termNodeNames.reserve(_numTerminals);
        nonTermNodeNames.reserve(numNodes);

        nodes >> cName;
        for (unsigned nId = 0; nId < numNodes; nId++) {
                if (cName[0] == 0) {
                        cerr << "Expected " << numNodes << " nodes, of them " << _numTerminals << " terminals " << endl;
                        cerr << "Found only " << nId << " (through line " << lineno << ") " << endl;
                        if (termNodeNames.size()) cerr << "Last terminal name seen: " << termNodeNames.back() << ", ";
                        if (nonTermNodeNames.size()) cerr << "last non-terminal name seen: " << nonTermNodeNames.back();
                        cerr << endl;
                        abkfatal(0, "Not enough nodeNames in .nodes file");
                }

                string newNodeName(cName);

                bool isTerminal = false;

                nodes >> eatblank;

                while (!nodes.eof() && nodes.peek() != '\n' && nodes.peek() != '\r') {
                        nodes >> cName;
                        if (!newstrcasecmp(cName.c_str(), "terminal")) {
                                isTerminal = true;
                                nodes >> skiptoeol;
                                break;
                        } else
                                nodes >> eatblank;
                }

                if (isTerminal)
                        termNodeNames.push_back(newNodeName);
                else
                        nonTermNodeNames.push_back(newNodeName);

                nodes >> skiptoeol;
                lineno++;
                nodes >> eathash(lineno);

                if (!nodes.eof())
                        nodes >> cName;
                else
                        cName[0] = 0;
        }

        if (_numTerminals != termNodeNames.size()) {
                cerr << "Expected " << _numTerminals << " terminals " << endl;
                cerr << "Found only " << termNodeNames.size() << " (through line " << lineno << ") " << endl;
                if (termNodeNames.size()) cerr << "Last terminal name seen: " << termNodeNames.back() << ", ";
                if (nonTermNodeNames.size()) cerr << "last non-terminal name seen: " << nonTermNodeNames.back();
                cerr << endl;
                abkfatal(0, "Did not find the expected number of terminals");
        }

        unsigned t;
        for (t = 0; t < termNodeNames.size(); t++) {
                // char * thisName = new char[termNodeNames[t].size()];
                // strcpy(thisName,termNodeNames[t].c_str());
                string thisName = termNodeNames[t];
                _nodeNames.push_back(thisName);
                _nodeNamesMap[thisName] = t;
        }
        for (unsigned n = 0; n < nonTermNodeNames.size(); n++) {
                string thisName = nonTermNodeNames[n];
                _nodeNames.push_back(thisName);
                _nodeNamesMap[thisName] = t + n;
        }

        nodes.close();
}

void HGraphFixed::readNets(const char* netsFileName) {
        ifstream nets(netsFileName);
        abkfatal2(nets, " Could not open ", netsFileName);
        cout << " Reading " << netsFileName << " ... " << endl;

        int lineno = 1;
        nets >> needcaseword("UCLA") >> needcaseword("nets") >> needcaseword("1.0") >> skiptoeol;
        nets >> eathash(lineno);

        unsigned expectedNumPins = _numPins = 0;
        char cName[1024];
        nets >> cName;
        if (!strcmp(cName, "NumNets")) {
                nets >> skiptoeol;
                nets >> needcaseword("NumPins") >> needword(":") >> my_isnumber(lineno) >> expectedNumPins >> skiptoeol;
        } else if (!strcmp(cName, "NumPins"))
                nets >> needword(":") >> my_isnumber(lineno) >> expectedNumPins >> skiptoeol;

#ifdef SIGNAL_DIRECTIONS
        _srcs.reserve(1 + (expectedNumPins / 3));
        _snks.reserve(1 + (expectedNumPins / 3));
        _srcSnks.reserve(1 + (expectedNumPins / 3));
#else
        _srcSnks.reserve(expectedNumPins);
#endif

        BitBoard nodesSeen(_nodes.size());
        char tmpNetName[510];

        unsigned netDegree;
        char dir;
        bool makingEdge;
        while (_numPins < expectedNumPins) {
                if (nets.eof()) {
                        cerr << "Expected " << expectedNumPins << " got " << _numPins << endl;
                        abkfatal(0, "mal-formed nets file (incorrect # pins)");
                }
                nets >> needcaseword("NetDegree") >> needword(":") >> my_isnumber(lineno) >> netDegree >> eatblank;
                char c = nets.peek();
                if (c != '\n' && c != '\r')
                        nets >> tmpNetName >> skiptoeol;
                else
                        tmpNetName[0] = 0;
                //      cout << "Net " << _netNames.size() << " name " <<
                // tmpNetName << endl;
                HGFEdge* edge;
                if (netDegree > 1) {
                        edge = addEdge();
                        if (tmpNetName[0] != 0) {
                                char* newName = new char[strlen(tmpNetName) + 1];
                                strcpy(newName, tmpNetName);
                                _netNames.push_back(newName);
                                _netNamesMap[newName] = edge->getIndex();
                        } else
                                _netNames.push_back(string());
                        abkassert(edge->getWeight() > 0, "constructed a 0-weight edge");
                        makingEdge = true;
                } else {
                        cerr << " Net of degree 1 found ";
                        if (strlen(tmpNetName)) cerr << " named " << tmpNetName;
                        abkfatal(0, "This parser does not allow nets of degree < 2");
                        makingEdge = false;
                }

                nodesSeen.clear();

                for (unsigned n = 0; n < netDegree; n++) {
                        if (!makingEdge) {
                                nets >> skiptoeol;
                                _numPins++;
                                continue;
                        }

                        nets >> cName;
                        abkfatal(strcmp(cName, "NetDegree"), "did not find expected number of node names");
                        abkfatal2(haveSuchNode(cName), "no such node: ", cName);
                        HGFNode& node = getNodeByName(cName);
                        unsigned nId = node.getIndex();

                        if (nodesSeen.isBitSet(nId)) {
                                _numPins++;
                                nets >> skiptoeol;
                                nets >> eathash(lineno);
                                lineno++;
                                continue;
                        }

                        nets >> dir >> skiptoeol;
                        dir = toupper(dir);
                        //   if(_param.makeAllSrcSnk && (dir == 'O' || dir ==
                        // 'I')) dir='B';

                        switch (dir) {
                                case('I') :
#ifdef SIGNAL_DIRECTIONS
                                        addSnk(node, *edge);
                                        break;
#endif
                                case('O') :
#ifdef SIGNAL_DIRECTIONS
                                        addSrc(node, *edge);
                                        break;
#endif
                                case('B') :
                                        addSrcSnk(node, *edge);
                                        break;
                                default:  // error
                                        abkfatal3(0, dir, ": unexpected symbol in line ", lineno);
                        };
                        _numPins++;
                        lineno++;
                        nets >> eathash(lineno);
                        nodesSeen.setBit(nId);

                        if (_param.verb.getForActions() > 10) cout << "Parsed net " << edge->getIndex() << " degree " << netDegree << endl;
                }
                nets >> eathash(lineno);
        }

        nets.close();
}

void HGraphFixed::readWts(const char* wtsFileName) {
        ifstream wts(wtsFileName);
        abkfatal2(wts, " Could not open ", wtsFileName);
        cout << " Reading " << wtsFileName << " ... " << endl;

        int lineno = 1;

        wts >> needcaseword("UCLA") >> needcaseword("wts") >> needword("1.0") >> skiptoeol;
        wts >> eathash(lineno);

        _numMultiWeights = 0;
        char cName[1024];  // cell name

        if (!wts.eof()) {
                wts >> cName;  // skip cell name this time....
                if (!wts.eof()) {
                        wts >> cName;
                        while (!wts.eof() && _numMultiWeights <= 100 && isdigit(cName[0])) {
                                wts >> cName;
                                _numMultiWeights++;
                        }
                }
        }

        _numTotalWeights = _numMultiWeights;
        wts.close();
        abkfatal(_numMultiWeights < 100,
                 "error: there appear to be > 100 weights OR cell names start "
                 "with numbers");
        abkfatal(_multiWeights.size() == 0, "multiWeights should be empty");

        if (_numTotalWeights == 0) {  // set unit weights
                _numMultiWeights = _numTotalWeights = 1;
                _multiWeights = vector<HGWeight>(_nodes.size(), 1.0);
                return;
        }

        _multiWeights = vector<HGWeight>(getNumNodes() * _numMultiWeights);

        // wts.open(wtsFileName);
        lineno = 0;

        ifstream wts2(wtsFileName);
        abkfatal2(wts2, " Could not open ", wtsFileName);

        wts2 >> needcaseword("UCLA") >> needcaseword("wts") >> needword("1.0") >> skiptoeol;
        wts2 >> eathash(lineno);

        int NodeNetNameWarningCount = 0;

        int numWarning = 5;
        int warningCount4Nets = 0;

        while (!wts2.eof() && wts2 >> eathash(lineno) >> cName) {
                if (!wts2.eof()) {
                        // cerr<<"Loop 1 iter got name: "<<cName<<endl;
                        if (cName[0] == 0 || strlen(cName) == 0) break;
                        if (haveSuchNode(cName) && haveSuchNet(cName) && NodeNetNameWarningCount < 10) {
                                NodeNetNameWarningCount++;
                                string msg = "Node and Net with the same name \"" + string(cName) + "\",\n\t .wts preference given to Net.\n";
                                if (NodeNetNameWarningCount == 10)
                                        msg +=
                                            "This warning has occurred 10 "
                                            "times, and will not be "
                                            "repeated.\n";
                                abkwarn(0, msg.c_str());
                        }

                        if (haveSuchNet(cName)) {
                                HGFEdge& net = getNetByName(cName);
                                double nextWt;
                                // following change by sadya. what is actually
                                // correct?
                                //           wts2>>eatblank>>nextWt>>needeol(lineno);
                                wts2 >> eatblank >> nextWt >> eatblank;
                                // added by JFLU: bound the netweight by 1000
                                if (nextWt > 1000) {
                                        if (warningCount4Nets < numWarning) {
                                                stringstream msg;
                                                msg << "Net weight greater "
                                                       "than 1000, it is being "
                                                       "set to 1000" << endl;
                                                msg << "\tOriginal Weight: " << nextWt << endl;
                                                abkwarn(0, msg.str().c_str());
                                        }
                                        if (warningCount4Nets == numWarning) {
                                                abkwarn(0,
                                                        "More nets have weight "
                                                        "greater than 1000, "
                                                        "further warnings are "
                                                        "being suppressed.");
                                        }
                                        ++warningCount4Nets;
                                        nextWt = 1000;
                                }
                                net.setWeight(nextWt);
                        } else if (haveSuchNode(cName)) {
                                HGFNode& node = getNodeByName(cName);
                                unsigned wtNum = 0;
                                double nextWt;
                                while (!wts2.eof() && wts2.peek() != '\n' && wts2.peek() != '\r') {
                                        wts2 >> eatblank >> nextWt >> eatblank;
                                        setWeight(node.getIndex(), nextWt, wtNum++);
                                }
                        }
                        lineno++;
                        cName[0] = 0;
                }
        }

        wts2.close();
}
