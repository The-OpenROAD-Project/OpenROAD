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

// Created by Igor Markov 980320

// CHANGES
// 980325  ilm  independent fix and blk files, AUX file I/O, stream I/O
// 980402  ilm  .are files are optional now
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/uofm_alloc.h>
#include <ABKCommon/pathDelims.h>
#include <Partitioning/partProb.h>
#include <Partitioning/partitionData.h>
#include <fstream>

using std::ofstream;
using std::ifstream;
using std::istream;
using std::cout;
using std::endl;
using std::setw;
using uofm::vector;

static uofm_bit_vector relativeCaps;
static vector<double> tolerances;
static vector<unsigned> tolType;

PartitioningProblem::PartitioningProblem(const char* baseFileName, Parameters params) : _params(params) {
        Timer tm;
        initToNULL();
        read(baseFileName);
        _postProcess();
        tm.stop();

        if (_params.verbosity.getForSysRes()) cout << "Init+Read+PostProcess for PartProblem took " << tm << endl;
}

PartitioningProblem::PartitioningProblem(const char* netDFileName, const char* areFileName, const char* blkFileName, const char* fixFileName, const char* solFileName, Parameters params) : _params(params) {
        Timer tm;
        initToNULL();
        bool success = readHG(netDFileName, areFileName);
        success |= read(blkFileName, fixFileName, solFileName);

        abkwarn(success, "Reading PartitioningProblem from files may have failed");
        _postProcess();
        tm.stop();
        if (_params.verbosity.getForSysRes()) cout << "Init+Read+PostProcess for PartProblem took " << tm << endl;
}

bool PartitioningProblem::read(const char* baseFileName) {
        Timer tm0;
        unsigned baseLength = strlen(baseFileName);
        ifstream auxFile(baseFileName);
        bool success = false;
        char* newNetD = NULL;
        char* newAre = NULL;

        char* newNodes = NULL;
        char* newNets = NULL;
        char* newWts = NULL;

        char* newBlk = NULL;
        char* newFix = NULL;
        char* newSol = NULL;

        char dir[255];
        char* auxFileDirEnd = strrchr(const_cast<char*>(baseFileName), pathDelim);
        if (auxFileDirEnd) {
                strncpy(dir, baseFileName, auxFileDirEnd - baseFileName + 1);
                dir[auxFileDirEnd - baseFileName + 1] = 0;
        } else
                strcpy(dir, "");

        if (auxFile) {
                bool found = false;
                int lineNo = 1;
                cout << "Reading " << baseFileName << endl;
                char oneLine[1023];
                char word1[100], word2[100];

                while (!found && !auxFile.eof()) {
                        auxFile >> eathash(lineNo) >> word1 >> noeol(lineNo) >> word2 >> noeol(lineNo);
                        abkfatal(!strcmp(word2, ":"),
                                 " Error in aux file: space-separated column "
                                 "expected");
                        if (!newstrcasecmp(word1, "CD")) {
                                auxFile >> word1;
                                auxFile >> needeol(lineNo++);
                                if (word1[0] == pathDelimWindows || word1[0] == pathDelimUnix)
                                        strcpy(dir, word1);
                                else
                                        strcat(dir, word1);
                                char fDel[2];
                                sprintf(fDel, "%c", pathDelim);
                                if (word1[strlen(word1) - 1] != pathDelimWindows || word1[0] == pathDelimUnix) strcat(dir, fDel);
                        } else if (!newstrcasecmp(word1, "PartProb")) {
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

                                        if (strstr(fileNames[j], ".nodes")) {
                                                newNodes = fileNames[j];
                                        } else if (strstr(fileNames[j], ".NODES")) {
                                                newNodes = fileNames[j];
                                        } else if (strstr(fileNames[j], ".nets")) {
                                                newNets = fileNames[j];
                                        } else if (strstr(fileNames[j], ".NETS")) {
                                                newNets = fileNames[j];
                                        } else if (strstr(fileNames[j], ".wts")) {
                                                newWts = fileNames[j];
                                        } else if (strstr(fileNames[j], ".WTS")) {
                                                newWts = fileNames[j];
                                        } else if (strstr(fileNames[j], ".net")) {
                                                newNetD = fileNames[j];
                                        } else if (strstr(fileNames[j], ".NET")) {
                                                newNetD = fileNames[j];
                                        } else if (strstr(fileNames[j], ".are")) {
                                                newAre = fileNames[j];
                                        } else if (strstr(fileNames[j], ".ARE")) {
                                                newAre = fileNames[j];
                                        } else if (strstr(fileNames[j], ".blk")) {
                                                newBlk = fileNames[j];
                                        } else if (strstr(fileNames[j], ".BLK")) {
                                                newBlk = fileNames[j];
                                        } else if (strstr(fileNames[j], ".fix")) {
                                                newFix = fileNames[j];
                                        } else if (strstr(fileNames[j], ".FIX")) {
                                                newFix = fileNames[j];
                                        } else if (strstr(fileNames[j], ".sol")) {
                                                newSol = fileNames[j];
                                        } else if (strstr(fileNames[j], ".SOL")) {
                                                newSol = fileNames[j];
                                        }
                                }

                                char* tmpNet = NULL;
                                char* tmpAre = NULL;
                                char* tmpNodes = NULL;
                                char* tmpNets = NULL;
                                char* tmpWts = NULL;
                                char* tmpBlk = NULL;
                                char* tmpFix = NULL;
                                char* tmpSol = NULL;

                                unsigned dirLen = strlen(dir);

                                if (newNetD != NULL)
                                        if (newNetD[0] != pathDelimWindows && newNetD[0] != pathDelimUnix) {
                                                tmpNet = new char[dirLen + strlen(newNetD) + 1];
                                                sprintf(tmpNet, "%s%s", dir, newNetD);
                                        } else {
                                                tmpNet = new char[strlen(newNetD) + 1];
                                                strcpy(tmpNet, newNetD);
                                        }

                                if (newAre != NULL)
                                        if (newAre[0] != pathDelimWindows && newAre[0] != pathDelimUnix) {
                                                tmpAre = new char[dirLen + strlen(newAre) + 1];
                                                sprintf(tmpAre, "%s%s", dir, newAre);
                                        } else {
                                                tmpAre = new char[strlen(newAre) + 1];
                                                strcpy(tmpAre, newAre);
                                        }

                                if (newNodes != NULL)
                                        if (newNodes[0] != pathDelimWindows && newNodes[0] != pathDelimUnix) {
                                                tmpNodes = new char[dirLen + strlen(newNodes) + 1];
                                                sprintf(tmpNodes, "%s%s", dir, newNodes);
                                        } else {
                                                tmpNodes = new char[strlen(newNodes) + 1];
                                                strcpy(tmpNodes, newNodes);
                                        }

                                if (newNets != NULL)
                                        if (newNets[0] != pathDelimWindows && newNets[0] != pathDelimUnix) {
                                                tmpNets = new char[dirLen + strlen(newNets) + 1];
                                                sprintf(tmpNets, "%s%s", dir, newNets);
                                        } else {
                                                tmpNets = new char[strlen(newNets) + 1];
                                                strcpy(tmpNets, newNets);
                                        }

                                if (newWts != NULL)
                                        if (newWts[0] != pathDelimWindows && newWts[0] != pathDelimUnix) {
                                                tmpWts = new char[dirLen + strlen(newWts) + 1];
                                                sprintf(tmpWts, "%s%s", dir, newWts);
                                        } else {
                                                tmpWts = new char[strlen(newWts) + 1];
                                                strcpy(tmpWts, newWts);
                                        }

                                if (newBlk != NULL)
                                        if (newBlk[0] != pathDelimWindows && newBlk[0] != pathDelimUnix) {
                                                tmpBlk = new char[dirLen + strlen(newBlk) + 1];
                                                sprintf(tmpBlk, "%s%s", dir, newBlk);
                                        } else {
                                                tmpBlk = new char[strlen(newBlk) + 1];
                                                strcpy(tmpBlk, newBlk);
                                        }

                                if (newFix != NULL)
                                        if (newFix[0] != pathDelimWindows && newFix[0] != pathDelimUnix) {
                                                tmpFix = new char[dirLen + strlen(newFix) + 1];
                                                sprintf(tmpFix, "%s%s", dir, newFix);
                                        } else {
                                                tmpFix = new char[strlen(newFix) + 1];
                                                strcpy(tmpFix, newFix);
                                        }

                                if (newSol != NULL)
                                        if (newSol[0] != pathDelimWindows && newSol[0] != pathDelimUnix) {
                                                tmpSol = new char[dirLen + strlen(newSol) + 1];
                                                sprintf(tmpSol, "%s%s", dir, newSol);
                                        } else {
                                                tmpSol = new char[strlen(newSol) + 1];
                                                strcpy(tmpSol, newSol);
                                        }

                                abkfatal(tmpNet == NULL || tmpNodes == NULL,
                                         "AUX file cannot contain both net[D] "
                                         "and Nodes files");

                                if (tmpNet) success = readHG(_abk_cpd(tmpNet), _abk_cpd(tmpAre));
                                if (tmpNodes) success = readHG(_abk_cpd(tmpNodes), _abk_cpd(tmpNets), _abk_cpd(tmpWts));

                                success |= read(_abk_cpd(tmpBlk), _abk_cpd(tmpFix), _abk_cpd(tmpSol));

                                if (tmpNet) delete[] tmpNet;
                                if (tmpAre) delete[] tmpAre;
                                if (tmpNodes) delete[] tmpNodes;
                                if (tmpNets) delete[] tmpNets;
                                if (tmpWts) delete[] tmpWts;
                                if (tmpBlk) delete[] tmpBlk;
                                if (tmpFix) delete[] tmpFix;
                                if (tmpSol) delete[] tmpSol;
                        }
                }
        } else {
                newNetD = new char[baseLength + 6];
                newAre = new char[baseLength + 5];
                newNodes = new char[baseLength + 7];
                newNets = new char[baseLength + 6];
                newWts = new char[baseLength + 5];
                newBlk = new char[baseLength + 5];
                newFix = new char[baseLength + 5];
                newSol = new char[baseLength + 5];
                strcpy(newNetD, baseFileName);
                strcat(newNetD, ".netD");
                strcpy(newAre, baseFileName);
                strcat(newAre, ".are");
                strcpy(newNodes, baseFileName);
                strcat(newNodes, ".nodes");
                strcpy(newNets, baseFileName);
                strcat(newNets, ".nets");
                strcpy(newWts, baseFileName);
                strcat(newWts, ".wts");
                strcpy(newBlk, baseFileName);
                strcat(newBlk, ".blk");
                strcpy(newFix, baseFileName);
                strcat(newFix, ".fix");
                strcpy(newSol, baseFileName);
                strcat(newSol, ".sol");

                ifstream netDStr(newNetD);
                ifstream nodesStr(newNodes);
                abkfatal2(netDStr || nodesStr, " No .aux, .nodes or .netD found: ", baseFileName);

                if (netDStr) {
                        netDStr.close();
                        nodesStr.close();
                        success = readHG(newNetD, newAre);
                } else {
                        netDStr.close();
                        nodesStr.close();
                        success = readHG(newNodes, newNets, newWts);
                }

                success |= read(newBlk, newFix, newSol);

                delete[] newNetD;
                delete[] newAre;
                delete[] newNodes;
                delete[] newNets;
                delete[] newWts;
                delete[] newBlk;
                delete[] newFix;
                delete[] newSol;
        }
        abkwarn(success, "Reading PartitioningProblem from files may have failed\n");
        tm0.stop();

        if (_params.verbosity.getForSysRes()) cout << "Reading PartitioningProblem took " << tm0 << endl;
        return true;
}

bool PartitioningProblem::saveAsNetDAre(const char* baseFileName) const {
        _hgraph->saveAsNetDAre(baseFileName);
        cout << " Done writing " << baseFileName << ".netD and " << baseFileName << ".areM" << endl;

        unsigned baseLength = strlen(baseFileName);
        char* newAux = new char[baseLength + 6];
        char* newBlk = new char[baseLength + 5];
        char* newFix = new char[baseLength + 5];
        char* newSol = new char[baseLength + 5];
        strcpy(newAux, baseFileName);
        strcat(newAux, ".aux");
        strcpy(newBlk, baseFileName);
        strcat(newBlk, ".blk");
        strcpy(newFix, baseFileName);
        strcat(newFix, ".fix");
        strcpy(newSol, baseFileName);
        strcat(newSol, ".sol");

        if (_terminalToBlock == NULL || _terminalToBlock->empty()) {
                delete[] newFix;
                newFix = NULL;
        }
        if (_partitions == NULL) {
                delete[] newBlk;
                delete[] newSol;
                newBlk = NULL, newSol = NULL;
        } else if (_bestSolnNum >= _solnBuffers->size() && newSol) {
                delete[] newSol;
                newSol = NULL;
        }

        bool success = save(newBlk, newFix, newSol, true);

        abkwarn(success, "Writing PartitioningProblem to files may have failed");
        if (success) {
                ofstream auxFile(newAux);
                abkfatal(newAux, " Can't open AUX file for writing");
                auxFile << "PartProb : " << baseFileName << ".netD " << baseFileName << ".areM ";
                if (newBlk) auxFile << newBlk << " ";
                if (newFix) auxFile << newFix << " ";
                if (newSol) auxFile << newSol << " ";
                auxFile << endl;
        }

        delete[] newAux;
        if (newBlk) delete[] newBlk;
        if (newFix) delete[] newFix;
        if (newSol) delete[] newSol;
        return success;
}

bool PartitioningProblem::saveAsNodesNets(const char* baseFileName) const {
        _hgraph->saveAsNodesNetsWts(baseFileName);

        cout << " Done writing " << baseFileName << ".nodes " << baseFileName << ".nets and " << baseFileName << ".wts" << endl;

        unsigned baseLength = strlen(baseFileName);
        char* newAux = new char[baseLength + 6];
        char* newBlk = new char[baseLength + 5];
        char* newFix = new char[baseLength + 5];
        char* newSol = new char[baseLength + 5];
        strcpy(newAux, baseFileName);
        strcat(newAux, ".aux");
        strcpy(newBlk, baseFileName);
        strcat(newBlk, ".blk");
        strcpy(newFix, baseFileName);
        strcat(newFix, ".fix");
        strcpy(newSol, baseFileName);
        strcat(newSol, ".sol");

        if (_terminalToBlock == NULL || _terminalToBlock->empty()) {
                delete[] newFix;
                newFix = NULL;
        }
        if (_partitions == NULL) {
                delete[] newBlk;
                delete[] newSol;
                newBlk = NULL, newSol = NULL;
        } else if (_bestSolnNum >= _solnBuffers->size() && newSol) {
                delete[] newSol;
                newSol = NULL;
        }

        bool success = save(newBlk, newFix, newSol, false);

        abkwarn(success, "Writing PartitioningProblem to files may have failed");
        if (success) {
                ofstream auxFile(newAux);
                abkfatal(newAux, " Can't open AUX file for writing");
                auxFile << "PartProb : " << baseFileName << ".nodes " << baseFileName << ".nets " << baseFileName << ".wts ";
                if (newBlk) auxFile << newBlk << " ";
                if (newFix) auxFile << newFix << " ";
                if (newSol) auxFile << newSol << " ";
                auxFile << endl;
        }

        delete[] newAux;
        if (newBlk) delete[] newBlk;
        if (newFix) delete[] newFix;
        if (newSol) delete[] newSol;
        return success;
}

bool PartitioningProblem::save(const char* blkFileName, const char* fixFileName, const char* solFileName, bool saveAsNetD) const {
        unsigned k, j, numWeights = (*_totalWeight).size();
        vector<double> sumCapacities(numWeights, 0);

        if (_capacities == NULL) return true;

        unsigned numPart = (*_capacities).size();

        char buf[20], buf1[20];

        for (k = 0; k != numPart; k++) {
                for (j = 0; j != numWeights; j++) sumCapacities[j] += (*_capacities)[k][j];
        }

        if (blkFileName && _padBlocks && _partitions) {
                unsigned numPadBlocks = (*_padBlocks).size();

                cout << " Writing " << blkFileName << " ... " << endl;
                ofstream blkFile(blkFileName);
                abkfatal(blkFile, " Can't open .blk file for writing");

                blkFile << "UCLA blk 1.0 " << endl << TimeStamp() << User() << Platform() << endl;

                //  Write .blk file

                blkFile << "Regular partitions : " << numPart << endl << "Pad partitions : " << numPadBlocks << endl << "Relative capacities : ";
                for (k = 0; k != (*_totalWeight).size(); k++) blkFile << "yes ";
                blkFile << endl << "Capacity tolerances : ";

                const vector<double>& firstNodeCaps = (*_capacities)[0];
                const vector<double>& firstNodeMaxCaps = (*_maxCapacities)[0];
                vector<double> capTols(firstNodeCaps.size());
                for (k = 0; k != capTols.size(); k++) {
                        capTols[k] = 100.0 * fabs(firstNodeMaxCaps[k] - firstNodeCaps[k]) / firstNodeCaps[k];  // sumCapacities[k] ;
                        blkFile << capTols[k] << "%   ";
                }
                blkFile << endl;
                for (k = 0; k != _padBlocks->size(); k++) {
                        sprintf(buf, "pb%d", k);
                        sprintf(buf1, "%7s rect ", buf);
                        blkFile << buf1;
                        const Rectangle& rect = (*_padBlocks)[k];
                        if (rect.isEmpty())
                                blkFile << "        Empty ";
                        else {
                                blkFile << setw(9) << rect.xMin << " " << setw(9) << rect.yMin << " " << setw(9) << rect.xMax << " " << setw(9) << rect.yMax << " ";
                        }
                        //    blkFile << " : ";
                        blkFile << endl;
                }

                for (k = 0; k != numPart; k++) {
                        sprintf(buf, "b%d", k);
                        sprintf(buf1, "%7s rect ", buf);
                        blkFile << buf1;
                        const Rectangle& rect = (*_partitions)[k];
                        if (rect.isEmpty())
                                blkFile << "        Empty ";
                        else {
                                blkFile << setw(9) << rect.xMin << " " << setw(9) << rect.yMin << " " << setw(9) << rect.xMax << " " << setw(9) << rect.yMax << " ";
                        }
                        blkFile << " : ";
                        const vector<double>& caps = (*_capacities)[k];
                        for (j = 0; j != caps.size(); j++) {
                                blkFile << setw(5) << 100 * caps[j] / sumCapacities[j] << " ";
                                // compute cap tols and write them here if difft
                                // from capTols
                        }
                        blkFile << endl;
                }
        }

        //  Now write .fix file

        if (fixFileName && _terminalToBlock && !_terminalToBlock->empty()) {
                ofstream fixFile(fixFileName);
                abkfatal(fixFile, " Can't open .fix file for writing");
                cout << " Writing " << fixFileName << " ... " << endl;
                fixFile << "UCLA fix 1.0 " << endl << TimeStamp() << User() << Platform() << endl;

                PartitionIds freeToMove;
                freeToMove.setToAll(numPart);

                unsigned numFixedPads = 0;
                unsigned ttBlocks = _terminalToBlock->size();
                for (k = 0; k != _hgraph->getNumTerminals(); k++) {
                        if ((*_fixedConstr)[k] != freeToMove || (k < ttBlocks && (*_terminalToBlock)[k] != UINT_MAX)) numFixedPads++;
                }

                unsigned numFixedNonPads = 0;
                for (k = _hgraph->getNumTerminals(); k != _hgraph->getNumNodes(); k++) {
                        PartitionIds constrainedTo = (*_fixedConstr)[k];
                        if (constrainedTo != freeToMove && !constrainedTo.isEmpty()) numFixedNonPads++;
                }

                fixFile << "Regular partitions : " << (*_partitions).size() << endl << "Pad partitions : " << (*_padBlocks).size() << endl;
                fixFile << "Fixed Pads : " << numFixedPads << endl;
                fixFile << "Fixed NonPads : " << numFixedNonPads << endl;

                itHGFNodeGlobal n;
                for (n = _hgraph->terminalsBegin(); n != _hgraph->terminalsEnd(); n++) {
                        k = (*n)->getIndex();

                        unsigned pb = UINT_MAX;
                        if (k < ttBlocks) pb = (*_terminalToBlock)[k];
                        PartitionIds propagatedTo = (*_fixedConstr)[k];

                        if (pb != UINT_MAX || propagatedTo != freeToMove) {
                                if (saveAsNetD)
                                        fixFile << "p" << k + 1 << " : ";
                                else
                                        fixFile << setw(10) << _hgraph->getNodeNameByIndex((*n)->getIndex()) << " : ";
                                if (pb != UINT_MAX) fixFile << "pb" << pb << " ";
                                if (propagatedTo != freeToMove)
                                        for (j = 0; j != 8 * sizeof(PartitionIds); j++)
                                                if (propagatedTo[j]) fixFile << "b" << j << " ";
                                fixFile << endl;
                        }
                }

                for (; n != _hgraph->nodesEnd(); n++) {
                        k = (*n)->getIndex();

                        PartitionIds constrainedTo = (*_fixedConstr)[k];
                        if (constrainedTo != freeToMove) {
                                if (saveAsNetD)
                                        fixFile << "p" << k + 1 << " : ";
                                else
                                        fixFile << setw(10) << _hgraph->getNodeNameByIndex((*n)->getIndex()) << " : ";
                                if (constrainedTo != freeToMove && !constrainedTo.isEmpty())
                                        for (j = 0; j != 8 * sizeof(PartitionIds); j++)
                                                if (constrainedTo[j]) fixFile << "b" << j << " ";
                                fixFile << endl;
                        }
                }
        }

        if (solFileName && _bestSolnNum < _solnBuffers->size()) saveBestSol(solFileName);
        return true;
}

bool PartitioningProblem::saveBestSol(const char* solFileName) const {
        unsigned bestSol = getBestSolnNum();
        ofstream solFile(solFileName);
        abkfatal(solFile, " Can't open .sol file for writing");
        cout << " Writing " << solFileName << " ... " << endl;
        solFile << "UCLA sol 1.0 " << endl << TimeStamp() << User() << Platform() << endl;

        unsigned numPart = (*_capacities).size();
        PartitionIds freeToMove;  //, nowhere;
        freeToMove.setToAll(numPart);

        unsigned j, k, numFixedPads = 0;

        for (k = 0; k != _hgraph->getNumTerminals(); k++) {
                if ((*_solnBuffers)[bestSol][k] != freeToMove) numFixedPads++;
        }

        unsigned numFixedNonPads = 0;
        for (k = _hgraph->getNumTerminals(); k != _hgraph->getNumNodes(); k++) {
                PartitionIds constrainedTo = (*_solnBuffers)[bestSol][k];
                if (constrainedTo != freeToMove) numFixedNonPads++;
        }

        solFile << "Regular partitions : " << (*_partitions).size() << endl << "Pad partitions : " << (*_padBlocks).size() << endl;
        solFile << "Fixed Pads : " << numFixedPads << endl;
        solFile << "Fixed NonPads : " << numFixedNonPads << endl;

        itHGFNodeGlobal n;
        for (n = _hgraph->terminalsBegin(); n != _hgraph->terminalsEnd(); n++) {
                k = (*n)->getIndex();

                PartitionIds propagatedTo = (*_solnBuffers)[bestSol][k];
                if (propagatedTo != freeToMove) {
                        solFile << setw(10) << _hgraph->getNodeNameByIndex((*n)->getIndex()) << " : ";
                        if (propagatedTo != freeToMove)
                                for (j = 0; j != 8 * sizeof(PartitionIds); j++)
                                        if (propagatedTo[j]) solFile << "b" << j << " ";
                        solFile << endl;
                }
        }

        for (; n != _hgraph->nodesEnd(); n++) {
                k = (*n)->getIndex();

                PartitionIds constrainedTo = (*_solnBuffers)[bestSol][k];
                if (constrainedTo != freeToMove) {
                        solFile << setw(10) << _hgraph->getNodeNameByIndex((*n)->getIndex()) << " : ";
                        if (constrainedTo != freeToMove)
                                for (j = 0; j != 8 * sizeof(PartitionIds); j++)
                                        if (constrainedTo[j]) solFile << "b" << j << " ";
                        solFile << endl;
                }
        }
        return true;
}

bool PartitioningProblem::readBLK(istream& blkFile) {
        abkfatal(_hgraph != 0, " _hgraph is not yet initialized in PartProb");
        abkfatal(blkFile, " Can't open .blk file for reading");

        unsigned k, j, numWeights = _hgraph->getNumWeights();

        vector<double> minNodeWeights(numWeights, DBL_MAX);
        for (itHGFNodeGlobal v = (*_hgraph).nodesBegin(); v != (*_hgraph).nodesEnd(); ++v) {
                for (k = 0; k != numWeights; k++) {
                        double w = _hgraph->getWeight((*v)->getIndex(), k);
                        if (w < minNodeWeights[k]) minNodeWeights[k] = w;
                }
        }

        int lineNo = 1;
        unsigned numPart = 0, numPadBlocks = 0;
        blkFile >> eathash(lineNo) >> needcaseword("UCLA", lineNo) >> needcaseword("blk", lineNo) >> skiptoeol;
        lineNo++;
        blkFile >> eathash(lineNo) >> needcaseword("Regular", lineNo) >> needcaseword("partitions", lineNo) >> needword(":", lineNo) >> my_isnumber(lineNo) >> numPart >> needeol(lineNo);

        _partitions = new vector<BBox>(numPart);
        PartitionIds freeToMove;
        freeToMove.setToAll(numPart);

        if (_fixedConstr == NULL) _fixedConstr = new Partitioning((*_hgraph).getNumNodes(), freeToMove);

        _capacities = new vector<vector<double> >(numPart, vector<double>(numWeights, -1));
        _maxCapacities = new vector<vector<double> >(numPart, vector<double>(numWeights, -1));
        _minCapacities = new vector<vector<double> >(numPart, vector<double>(numWeights, -1));
        blkFile >> eathash(lineNo) >> needcaseword("Pad", lineNo) >> needcaseword("partitions", lineNo) >> needword(":", lineNo) >> my_isnumber(lineNo) >> numPadBlocks >> needeol(lineNo);

        _padBlocks = new vector<BBox>(numPadBlocks);

        blkFile >> eathash(lineNo) >> needcaseword("Relative", lineNo) >> needcaseword("Capacities", lineNo) >> needword(":", lineNo);

        //    uofm_bit_vector relativeCaps(numWeights);
        relativeCaps = uofm_bit_vector(numWeights);
        for (k = 0; k != numWeights; k++) {
                char yesno[255];
                blkFile >> noeol(lineNo) >> isword(lineNo) >> yesno;
                if (newstrcasecmp(yesno, "yes") == 0 || newstrcasecmp(yesno, "y") == 0)
                        relativeCaps[k] = true;
                else if (newstrcasecmp(yesno, "no") == 0 || newstrcasecmp(yesno, "n") == 0)
                        relativeCaps[k] = false;
                else
                        abkfatal2(0, "'yes' or 'no' expected in line ", lineNo);
        }
        blkFile >> needeol(lineNo);

        blkFile >> eathash(lineNo) >> needcaseword("Capacity", lineNo) >> needcaseword("tolerances", lineNo) >> needword(":", lineNo);

        //    vector<double> tolerances(numWeights);
        tolerances = vector<double>(numWeights);
        //    vector<unsigned> tolType(numWeights);
        tolType = vector<unsigned>(numWeights);
        for (k = 0; k != numWeights; k++) {
                char* qualif = NULL;
                char toler[255];
                blkFile >> noeol(lineNo) >> my_isnumber(lineNo) >> toler;
                if ((qualif = strchr(toler, 'b')) != NULL) {
                        qualif[0] = '\0';
                        // tolerance in terms of min node width
                        tolType[k] = 2;
                        double fact = strtod(toler, &qualif);
                        abkfatal2(fact >= 1,
                                  " Capacity tolerance less than min node "
                                  "weight in line ",
                                  lineNo);
                        tolerances[k] = fact * minNodeWeights[k];
                        abkfatal2(qualif != toler, " Expected a number in line ", lineNo);
                } else if ((qualif = strchr(toler, '%')) != NULL) {
                        qualif[0] = '\0';
                        // tolerance in relative terms
                        tolType[k] = 0;
                        tolerances[k] = 0.01 * strtod(toler, &qualif);
                        abkfatal2(qualif != toler, " Expected a number in line ", lineNo);
                } else {
                        tolerances[k] = strtod(toler, &qualif);
                        tolType[k] = 1;
                        abkfatal2(qualif != toler, " Expected a number in line ", lineNo);
                        abkfatal2((qualif - toler) == int(strlen(toler)), " Trailing characters near tolerance in line ", lineNo);
                }
        }
        for (k = 0; k != numPart + numPadBlocks; k++) {
                char partId[256], rect[256];
                blkFile >> eathash(lineNo) >> isword(lineNo) >> partId >> noeol(lineNo) >> rect >> noeol(lineNo);

                abkfatal2(newstrcasecmp(rect, "rect") == 0, "'rect' expected in line", lineNo);
                double xMin, yMin, xMax, yMax;
                blkFile >> my_isnumber(lineNo) >> xMin >> my_isnumber(lineNo) >> yMin >> my_isnumber(lineNo) >> xMax >> my_isnumber(lineNo) >> yMax;
                BBox box(xMin, yMin, xMax, yMax);

                if (partId[0] == 'b') {
                        unsigned partNum = atoi(partId + 1);
                        abkfatal2(partNum >= 0 && partNum < numPart, " Bad partition Id in line ", lineNo);
                        (*_partitions)[partNum] = box;
                        blkFile >> needword(":", lineNo);
                        for (j = 0; j != numWeights; j++) {
                                blkFile >> noeol(lineNo) >> my_isnumber(lineNo) >> (*_capacities)[partNum][j];
                        }
                } else if (partId[0] == 'p' && partId[1] == 'b') {
                        unsigned padBlockNum = atoi(partId + 2);
                        abkfatal2(padBlockNum >= 0 && padBlockNum < numPadBlocks, " Bad pad block Id (too big a number) in line ", lineNo);
                        (*_padBlocks)[padBlockNum] = box;
                } else {
                        char buf[255];
                        sprintf(buf, "\n Got '%s' ", partId);
                        abkfatal3(0,
                                  "Partition Id starting with b or pb expected "
                                  "in line ",
                                  lineNo, buf);
                }
                blkFile >> needeol(lineNo);
        }
        return true;
}

bool PartitioningProblem::readFIX(istream& fixFile) {
        abkfatal(_hgraph != 0, " _hgraph is not yet initialized in PartProb");
        abkfatal(fixFile, " Can't open .fix file for reading");

        unsigned k, numWeights = _hgraph->getNumWeights();
        unsigned numTerminals = _hgraph->getNumTerminals();

        abkfatal(_terminalToBlock == 0, " _terminalToBlock is already init'd");
        _terminalToBlock = new vector<unsigned>(numTerminals, UINT_MAX);

        int lineNo = 1;
        unsigned numPart = 0, numPadBlocks = 0, numFixedPads = 0, numFixedNonPads = 0;

        fixFile >> eathash(lineNo);
        fixFile >> needcaseword("UCLA", lineNo) >> needcaseword("fix", lineNo);
        fixFile >> skiptoeol;
        lineNo++;

        fixFile >> eathash(lineNo);
        fixFile >> needcaseword("Regular", lineNo) >> needcaseword("partitions", lineNo) >> needword(":", lineNo) >> my_isnumber(lineNo) >> numPart >> needeol(lineNo);

        abkfatal(numPart > 1, " Need at least two regular partitions");
        abkfatal(numPart <= 32, " Can't have more than 32 regular partitions");

        if (_partitions != NULL) {
                abkfatal3((*_partitions).size() == numPart, "Mismatch with #partitions from blk file: line ", lineNo, "\n");
        }

        if (_capacities != NULL) {
                abkfatal2((*_capacities).size() == numPart, "Mismatch with #partitions from blk file: line ", lineNo);
        } else
                _capacities = new vector<vector<double> >(numPart);

        fixFile >> eathash(lineNo) >> needcaseword("Pad", lineNo) >> needcaseword("partitions", lineNo) >> needword(":", lineNo) >> my_isnumber(lineNo) >> numPadBlocks >> needeol(lineNo);

        fixFile >> eathash(lineNo) >> needcaseword("Fixed", lineNo) >> needcaseword("Pads", lineNo) >> needword(":", lineNo) >> my_isnumber(lineNo) >> numFixedPads >> needeol(lineNo);
        fixFile >> eathash(lineNo) >> needcaseword("Fixed", lineNo) >> needcaseword("NonPads", lineNo) >> needword(":", lineNo) >> my_isnumber(lineNo) >> numFixedNonPads >> needeol(lineNo);

        PartitionIds freeToMove;
        freeToMove.setToAll(numPart);
        if (_fixedConstr == NULL) {
                _fixedConstr = new Partitioning((*_hgraph).getNumNodes(), freeToMove);
        }

        for (k = 0; k != numFixedPads + numFixedNonPads; k++) {
                char nodeId[256];
                fixFile >> eathash(lineNo) >> isword(lineNo) >> nodeId >> noeol(lineNo) >> needword(":", lineNo) >> noeol(lineNo);

                const HGFNode& node = _hgraph->getNodeByName(nodeId);

                if (!_hgraph->isTerminal(node.getIndex())) {
                        PartitionIds constr;

                        while (fixFile.peek() != '\n' && fixFile.peek() != '\r') {
                                char partId[256];
                                fixFile >> isword(lineNo) >> partId >> eatblank;
                                abkfatal3(partId[0] == 'b',
                                          " NonPad node can be constrained to "
                                          "regular partitions only ",
                                          "in line ", lineNo);
                                unsigned partNum = atoi(partId + 1);
                                abkfatal2(partNum >= 0 && partNum < numPart, "Bad partition Id in line ", lineNo);
                                constr.addToPart(partNum);
                        }
                        if (constr.isEmpty()) constr = freeToMove;
                        (*_fixedConstr)[node.getIndex()] = constr;
                } else {
                        PartitionIds constr;

                        while (fixFile.peek() != '\n' && fixFile.peek() != '\r') {
                                char partId[256];
                                fixFile >> isword(lineNo) >> partId >> eatblank;
                                if (partId[0] == 'b') {
                                        unsigned partNum = atoi(partId + 1);
                                        abkfatal2(partNum >= 0 && partNum < numPart,
                                                  "Partition Id out of range in "
                                                  "line ",
                                                  lineNo);
                                        constr.addToPart(partNum);
                                } else if (partId[0] == 'p' && partId[1] == 'b') {
                                        unsigned blockNum = atoi(partId + 2);
                                        abkfatal2(blockNum >= 0 && blockNum < numPadBlocks, "Block Id out of range in line ", lineNo);
                                        if ((*_terminalToBlock)[node.getIndex()] != UINT_MAX) {
                                                char errMess[256];
                                                sprintf(errMess,
                                                        " Pad %s is "
                                                        "constrained to >1 pad "
                                                        "block in line %d\n",
                                                        nodeId, lineNo);
                                                abkfatal(0, errMess);
                                        }
                                        (*_terminalToBlock)[node.getIndex()] = blockNum;
                                        for (unsigned kw = 0; kw != numWeights; kw++) _hgraph->setWeight(node.getIndex(), 0.0, kw);
                                } else {
                                        char buf[255];
                                        sprintf(buf, ".\n Got %s ", partId);
                                        abkfatal3(0,
                                                  " Block or partition Id "
                                                  "expected in line ",
                                                  lineNo, buf);
                                }
                        }
                        if (constr.isEmpty()) constr = freeToMove;
                        (*_fixedConstr)[node.getIndex()] = constr;
                }

                fixFile >> needeol(lineNo);
        }
        return true;
}

bool PartitioningProblem::readSOL(istream& solFile) {
        abkfatal(_hgraph != 0, " _hgraph is not yet initialized in PartProb");
        abkfatal(solFile, " Can't open .sol file for reading");

        unsigned k;

        int lineNo = 1;
        unsigned numPart = 0, numPadBlocks = 0, numFixedPads = 0, numFixedNonPads = 0;

        solFile >> eathash(lineNo);
        solFile >> needcaseword("UCLA", lineNo) >> needcaseword("sol", lineNo);
        solFile >> skiptoeol;
        lineNo++;

        solFile >> eathash(lineNo);
        solFile >> needcaseword("Regular", lineNo) >> needcaseword("partitions", lineNo) >> needword(":", lineNo) >> my_isnumber(lineNo) >> numPart >> needeol(lineNo);

        abkfatal(numPart > 1, " Need at least two regular partitions");
        abkfatal(numPart <= 32, " Can't have more than 32 regular partitions");
        PartitionIds freeToMove, nowhere;
        freeToMove.setToAll(numPart);

        if (!_solnBuffers) {
                _solnBuffers = new PartitioningBuffer(_hgraph->getNumNodes(), 1);
                unsigned kk = 0;
                for (; kk != _hgraph->getNumTerminals(); ++kk) (*_solnBuffers)[0][kk] = nowhere;
                for (; kk != _hgraph->getNumNodes(); ++kk) (*_solnBuffers)[0][kk] = freeToMove;
        }

        if (_partitions != NULL) {
                abkfatal3((*_partitions).size() == numPart, "Mismatch with #partitions from blk file: line ", lineNo, "\n");
        }

        if (_fixedConstr) {
                if (_fixedConstr == NULL) _fixedConstr = new Partitioning((*_hgraph).getNumNodes(), freeToMove);
        }

        if (_capacities != NULL) {
                abkfatal2((*_capacities).size() == numPart, "Mismatch with #partitions from blk file: line ", lineNo);
        } else
                _capacities = new vector<vector<double> >(numPart);

        solFile >> eathash(lineNo) >> needcaseword("Pad", lineNo) >> needcaseword("partitions", lineNo) >> needword(":", lineNo) >> my_isnumber(lineNo) >> numPadBlocks >> needeol(lineNo);

        solFile >> eathash(lineNo) >> needcaseword("Fixed", lineNo) >> needcaseword("Pads", lineNo) >> needword(":", lineNo) >> my_isnumber(lineNo) >> numFixedPads >> needeol(lineNo);
        solFile >> eathash(lineNo) >> needcaseword("Fixed", lineNo) >> needcaseword("NonPads", lineNo) >> needword(":", lineNo) >> my_isnumber(lineNo) >> numFixedNonPads >> needeol(lineNo);

        for (k = 0; k != numFixedPads + numFixedNonPads; k++) {
                char nodeId[256];
                solFile >> eathash(lineNo) >> isword(lineNo) >> nodeId >> noeol(lineNo) >> needword(":", lineNo) >> noeol(lineNo);

                PartitionIds constr;
                const HGFNode& node = _hgraph->getNodeByName(nodeId);
                if (!_hgraph->isTerminal(node.getIndex())) {
                        while (solFile.peek() != '\n' && solFile.peek() != '\r') {
                                char partId[256];
                                solFile >> isword(lineNo) >> partId >> eatblank;
                                abkfatal3(partId[0] == 'b',
                                          " NonPad node can be constrained to "
                                          "regular partitions only ",
                                          "in line ", lineNo);
                                unsigned partNum = atoi(partId + 1);
                                abkfatal2(partNum >= 0 && partNum < numPart, "Bad partition Id in line ", lineNo);
                                constr.addToPart(partNum);
                        }
                } else {
                        while (solFile.peek() != '\n' && solFile.peek() != '\r') {
                                char partId[256];
                                solFile >> isword(lineNo) >> partId >> eatblank;
                                if (partId[0] == 'b') {
                                        unsigned partNum = atoi(partId + 1);
                                        abkfatal2(partNum >= 0 && partNum < numPart,
                                                  "Partition Id out of range in "
                                                  "line ",
                                                  lineNo);
                                        constr.addToPart(partNum);
                                } else if (partId[0] == 'p' && partId[1] == 'b') {
                                        // ignore
                                } else {
                                        char buf[255];
                                        sprintf(buf, ".\n Got %s ", partId);
                                        abkfatal3(0,
                                                  " Block or partition Id "
                                                  "expected in line ",
                                                  lineNo, buf);
                                }
                        }
                }

                if (!constr.isEmpty())
                        (*_solnBuffers)[0][node.getIndex()] = constr;
                else
                        (*_solnBuffers)[0][node.getIndex()] = freeToMove;

                solFile >> needeol(lineNo);
        }
        for (k = 1; k < _solnBuffers->size(); k++) (*_solnBuffers)[k] = (*_solnBuffers)[0];
        return true;
}

bool PartitioningProblem::readHG(const char* f1, const char* f2, const char* f3) {
        _hgraph = new HGraphFixed(f1, f2, f3, _params);
        cout << " Done reading HGraph" << endl;
        return true;
}

bool PartitioningProblem::read(const char* blkFileName, const char* fixFileName, const char* solFileName) {
        abkfatal(_hgraph != NULL, "Must call readHGraph first");
        abkfatal(blkFileName, "Missing .blk file");

        _ownsData = true;
        _costs = vector<double>(1, DBL_MAX);
        _violations = vector<double>(1, DBL_MAX);
        _imbalances = vector<double>(1, DBL_MAX);

        unsigned numWeights = _hgraph->getNumWeights();

        itHGFNodeGlobalMutable v = _hgraph->nodesBegin();

        if (blkFileName) {
                cout << " Reading " << blkFileName << " ... " << endl;
                ifstream ifs(blkFileName);
                readBLK(ifs);
        }

        if (fixFileName) {
                cout << " Reading " << fixFileName << " ... " << endl;
                ifstream ifs(fixFileName);
                readFIX(ifs);
        } else  // still need _terminalToBlock
        {
                abkwarn(_hgraph->getNumTerminals() == 0,
                        "Terminals specified in .net or .netD file, but "
                        "no .fix file given.\n  Making all nodes nonterminal");

                _hgraph->clearTerminals();  // set num terminals to 0
                abkfatal(_terminalToBlock == 0, " _terminalToBlock is already init'd");
                _terminalToBlock = new vector<unsigned>(_hgraph->getNumTerminals(), UINT_MAX);
        }
        // count the total weight of the nodes in the problem
        _totalWeight = new vector<double>(numWeights, 0.0);
        for (v = _hgraph->nodesBegin(); v != _hgraph->nodesEnd(); v++) {
                for (unsigned k = 0; k != numWeights; k++) (*_totalWeight)[k] += _hgraph->getWeight((*v)->getIndex(), k);
        }

        unsigned k, j, numPart = _capacities->size();

        for (k = 0; k != numWeights; k++) {
                for (j = 0; j != numPart; j++) {
                        if (relativeCaps[k]) (*_capacities)[j][k] *= 0.01 * (*_totalWeight)[k];
                        switch (tolType[k]) {
                                case 0:
                                        (*_maxCapacities)[j][k] = (*_capacities)[j][k] * (1 + tolerances[k]);
                                        (*_minCapacities)[j][k] = (*_capacities)[j][k] * (1 - tolerances[k]);
                                        break;
                                case 1:
                                case 2:
                                        (*_maxCapacities)[j][k] = (*_capacities)[j][k] + tolerances[k];
                                        (*_minCapacities)[j][k] = (*_capacities)[j][k] - tolerances[k];
                                        break;
                                default:
                                        abkfatal(0, "Unknown balance tolerance type");
                        }
                }
        }
        PartitionIds freeToMove, nowhere;
        freeToMove.setToAll(numPart);
        if (_capacities) {
                _solnBuffers = new PartitioningBuffer(_hgraph->getNumNodes(), 1);
                for (k = 0; k != _hgraph->getNumTerminals(); k++) (*_solnBuffers)[0][k] = nowhere;
                for (; k != _hgraph->getNumNodes(); k++) (*_solnBuffers)[0][k] = freeToMove;
        }

        if (solFileName) {
                cout << " Reading " << solFileName << " ... " << endl;
                ifstream ifs(solFileName);
                readSOL(ifs);
        }

        if (!_solnBuffers) {
                _solnBuffers = new PartitioningBuffer(_hgraph->getNumNodes(), 1);
                for (k = 0; k != _hgraph->getNumTerminals(); k++) (*_solnBuffers)[0][k] = nowhere;
                for (; k != _hgraph->getNumNodes(); k++) (*_solnBuffers)[0][k] = freeToMove;
        }

        return true;
}
