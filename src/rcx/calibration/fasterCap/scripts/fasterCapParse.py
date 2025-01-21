# ///////////////////////////////////////////////////////////////////////////////
# // BSD 3-Clause License
# //
# // Copyright (c) 2024, Dimitris Fotakis
# // All rights reserved.
# //
# // Redistribution and use in source and binary forms, with or without
# // modification, are permitted provided that the following conditions are met:
# //
# // * Redistributions of source code must retain the above copyright notice, this
# //   list of conditions and the following disclaimer.
# //
# // * Redistributions in binary form must reproduce the above copyright notice,
# //   this list of conditions and the following disclaimer in the documentation
# //   and/or other materials provided with the distribution.
# //
# // * Neither the name of the copyright holder nor the names of its
# //   contributors may be used to endorse or promote products derived from
# //   this software without specific prior written permission.
# //
# // THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# // AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# // IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# // ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# // LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# // CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# // SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# // INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# // CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# // ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# // POSSIBILITY OF SUCH DAMAGE.

import os
import re
import argparse
import math
import xlsxwriter
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


def OpenFile(file_path, rw="r"):
    if rw == "r":
        if os.path.exists(file_path):
            # print(file_path)
            return open(file_path, rw)
        else:
            print(f"The file {file_path} does not exist.")
            exit()
    if rw == "w":
        return open(file_path, rw)


def getWSformat(sp):
    dist = "{:.4f}".format(sp)
    return dist


def getFF(cap):
    cc = "{:9.6f}".format(cap * 1e15)
    return cc


def getWS(word, SW, ii):
    # parsing W0.14_W0.14
    s1 = float(word.split("_")[ii].split(SW)[1])
    return s1


# g2_wire_M5_w0  -7.3632e-17 2.82679e-16 -1.01845e-16 -5.93445e-17 -5.83917e-17 -5.96175e-17 -1.01798e-16
# g3_wire_M4_w1  -1.41823e-16 -2.30241e-17 5.5345e-16 -2.76143e-16 -3.19856e-18 1.15872e-18 -2.70547e-20


def parseMatrixRow(row, met, matrixRowIndex, wireCnt, diagWireCnt, dbg=0):
    # process one row of cap matrix at a time
    # returns wire index last number of the first column
    # return diagonal cap value for the wire index
    # return total coupling left and right of diagonal
    # return 2 caps left and right after immediate left and right
    # return diag cap as the before last column

    capValues = row.split()
    ii = matrixRowIndex

    # g3_wire_M4_w1
    targetMetWord = "_M" + str(met) + "_"

    if dbg > 0:
        print("-------------------------------------------------")
        print(row)
    if dbg > 1:
        print("targetMetWord= ", targetMetWord)

    if targetMetWord not in capValues[0]:
        return []

    wireIndex = int(capValues[0].split("w")[2])
    if wireIndex == 0:
        return []

    preWireCnt = matrixRowIndex - wireIndex
    diagWireIndex = preWireCnt + wireCnt
    if dbg > 1:
        print("wire= ", wireIndex)
        print("preWireCnt= ", preWireCnt)
        print("diagWireIndex= ", diagWireIndex)

    # diagonal
    tot = float(capValues[ii])
    cc = 0.0
    if wireIndex > 1:
        cc += float(capValues[ii - 1])
    if wireIndex + 1 <= wireCnt:
        cc += float(capValues[ii + 1])

    cc2 = 0.0
    if wireIndex > 2:
        cc2 += float(capValues[ii - 2])
    if wireIndex + 2 <= wireCnt:
        cc2 += float(capValues[ii + 2])

    diagCC = []
    for i in range(1, diagWireCnt + 1):
        jj = i + diagWireIndex
        diagCC.append(-float(capValues[jj]))

    return [wireIndex, tot, -cc, -cc2, diagCC]


def getWireCnt(capMatrix, mets, dbg=0):
    # returns wireCnt of target met=met and cnt of diagonal wires
    # skipping the context conductors
    # traverses all the rows of the cap matrix looking for patterns like _M2_
    diag = mets[3]
    met = mets[0]
    metUnder = mets[1]
    metOver = mets[2]

    # _M3_
    targetMetWord = "_M" + str(met) + "_"
    targetDiagMetWord = ""
    if diag:
        targetDiagMetWord = "_M" + str(metOver) + "_"

    wireCnt = 0
    diagWireCnt = 0
    for i in range(len(capMatrix)):
        name = capMatrix[i].split()[0]
        if dbg > 0:
            print(name)

        if targetMetWord in name:
            wireCnt = wireCnt + 1
        if diag and targetDiagMetWord in name:
            diagWireCnt = diagWireCnt + 1
            continue
    if dbg > 0:
        print(targetMetWord, targetDiagMetWord, [wireCnt, diagWireCnt])

    return [wireCnt, diagWireCnt]


def getPatternName(word):
    # returns last 4 subwords of the Input Pattern Name

    n = len(word)
    name = ""
    for i in range(6, 1, -1):
        name += word[n - i]
        name += "/"
    return name


def getMets(word):
    # returns metal indices from the pattern
    diag = 0
    met = 0
    metOver = 0
    metUnder = 0

    over = word.split("o")
    # print(over)
    if len(over) > 1:
        met = over[0].split("M")[1]
        mu = over[1].split("u")
        metUnder = mu[0].split("M")[1]
        if len(mu) > 1:
            metOver = mu[1].split("M")[1]
    else:
        under = word.split("u")
        # Diag M1duM3
        if "d" in under[0]:
            diag = 1
            met = under[0].split("d")[0].split("M")[1]
        else:
            met = under[0].split("M")[1]

        metOver = under[1].split("M")[1]

    return [met, metUnder, metOver, diag]


def readFasterCapOutPutLog(in_file, out_file, warnFP, emptyFP, statsFP, dbg):
    # reads the log file and parses the cap values for all conductors from the last valid iteration

    if dbg > 1:
        print(in_file)

    iterCnt = 0
    dimension = 0
    capMatrixCompleted = 0
    lastIterationIndex = 0
    iteration = []
    rows = []
    patternName = ""
    spacings = []
    widths = []
    mets = []
    patternType = ""

    mbytes = 0
    secs = 0

    line_cnt = 0
    f = OpenFile(in_file)
    for line in f:
        line_cnt = line_cnt + 1
        # parsing OverUnder5/M4oM3uM5/W0.14_W0.14/S0.14_S0.14/wires.lst
        if "Input file:" in line:
            file_line = line.split()
            full_pattern_file = file_line[2]

            # old flow word= file_line[2].split('/')
            word = in_file.split("/")
            n = len(word)
            if dbg > 0:
                print("Parsing Pattern: ", file_line[2])

            patternName = getPatternName(word)
            spacings = [getWS(word[n - 2], "S", 0), getWS(word[n - 2], "S", 1)]
            widths = [getWS(word[n - 3], "W", 0), getWS(word[n - 3], "W", 1)]
            len_in_widths = int(word[n - 2].split("L")[1])
            mets = getMets(word[n - 4])
            patternType = word[n - 5]
            if dbg > 1:
                print(patternName, spacings, widths, mets, len_in_widths)
            continue

        if "Iteration number" in line:
            iterCnt = iterCnt + 1
            rows = []
            continue

        if "Dimension" in line:
            capMatrixCompleted = 0
            dimension = int(line.split()[1])
            continue

        # Total allocated memory: 1439880 kilobytes
        # Total time: 375.292633s (0 days, 0 hours, 6 mins, 15 s)
        if "Weighted Frobenius" in line:
            capMatrixCompleted = 1

        if "Total allocated memory" in line:
            mbytes = int(line.split()[3]) / 1000

        if "Total time" in line:
            secs = int(line.split()[2].split("s")[0].split(".")[0])

        if dimension > 0:
            capMatrixCompleted = 0
            rows.append(line)
            dimension = dimension - 1
            if dimension == 0:
                iteration.append(rows)
            continue

    warning = ""
    if line_cnt == 0:
        warning = "Warning: Empty File " + in_file
        emptyFP.write(in_file + "\n")
        return [0, line_cnt, warning]

    if len(iteration) == 0:
        # warning= 'Warning: No Cap Matrix in File ' + in_file
        warning = "Warning: No Cap Matrix in File " + full_pattern_file
        warnFP.write(warning + "\n")
        return [0, line_cnt, warning]

    if dbg > 2:
        print("last iteration:")
        print(iteration)

    warning1 = ""
    warning = ""
    if capMatrixCompleted > 0:
        lastIterationIndex = len(iteration) - 1
    else:
        if dbg > 0:
            print("Warning -- iterations= ", iterCnt, len(iteration))
            print(iteration[lastIterationIndex])

        # warning1='Incomplete Last Iteration ' + in_file
        warning1 = "Incomplete Last Iteration " + full_pattern_file
        warnFP.write(warning1 + "\n")

        warning = "Incomplete"
        lastIterationIndex = lastIterationIndex - 2
        if lastIterationIndex < 0 or len(iteration[lastIterationIndex]) < dimension:
            if dbg > 0:
                print(
                    "Warning: Before last iteration: ",
                    lastIterationIndex,
                    " Incomplete",
                )
            # warning1='Incomplete Before Last Iteration ' + in_file
            warning1 = "Incomplete Before Last Iteration " + full_pattern_file
            warnFP.write(warning1 + "\n")
            return [0, line_cnt, warning1]

    wireCnt = getWireCnt(iteration[0], mets, dbg)
    met = mets[0]

    # parse last complete capacitance matrix
    initCapRowIndex = 0
    ii = 0
    for row in iteration[lastIterationIndex]:
        ii = ii + 1
        # print(row)
        caps = parseMatrixRow(row, met, ii, wireCnt[0], wireCnt[1])
        if len(caps) == 0:
            continue
        if dbg > 0:
            print("Caps = ", caps)

        diagUnder = " Under "
        diagCaps = ""
        if mets[3] > 0:
            diagUnder = " DiagUnder "
            dcc = getFF(caps[4][0])
            diagCaps = (
                " DiagDist "
                + str(spacings[1])
                + " DiagWidth "
                + str(widths[1])
                + " DiagCC "
                + dcc
            )

        dist = getWSformat(spacings[0])
        width1 = getWSformat(widths[0])
        out_line = (
            "Metal "
            + str(mets[0])
            + " Over "
            + str(mets[1])
            + diagUnder
            + str(mets[2])
            + "  Dist "
            + dist
            + " Width "
            + width1
        )
        cc = getFF(caps[2])
        fr = getFF(caps[1] - caps[2])
        tc = getFF(caps[1])
        cc2 = getFF(caps[3])
        full_net_name = patternName + "wire_" + str(caps[0])

        # print(out_line, "CC", cc, "FR", fr , 'TC', tc, 'CC2', cc2, ' ', diagCaps, full_net_name, run_stats, warning)
        out_file.write(
            out_line
            + "  LEN "
            + str(len_in_widths)
            + "  CC "
            + cc
            + "  FR "
            + fr
            + "  TC "
            + tc
            + "  CC2 "
            + cc2
            + "  "
            + diagCaps
            + " "
            + full_net_name
            + " "
            + warning
        )
        out_file.write("\n")
    run_stats = (
        str(mbytes)
        + " MB "
        + str(secs)
        + " secs "
        + str(len(iteration))
        + " iterations"
    )
    statsFP.write(run_stats + " " + full_pattern_file + "\n")
    return [1, line_cnt, warning1]


def main():
    arg_parser = argparse.ArgumentParser(description="Parser of a FasterCap file")
    arg_parser.add_argument(
        "-in_list_file",
        type=str,
        default="",
        help="Input Filename with list of input file paths, default= ",
    )
    arg_parser.add_argument(
        "-in_file",
        type=str,
        default="wires.log",
        help="Input Filename, default=wires.log",
    )
    arg_parser.add_argument(
        "-out_file",
        type=str,
        default="pattern.caps",
        help="Output Filename, default=pattern.caps",
    )
    arg_parser.add_argument(
        "-wire", type=int, default=3, help="target wire number, default=3"
    )
    arg_parser.add_argument("-dbg", type=int, default=0, help="debug level, default=0")

    args = arg_parser.parse_args()

    outFP = OpenFile(args.out_file, "w")
    warnFP = OpenFile("warnings", "w")
    emptyFP = OpenFile("empty_files", "w")
    statsFP = OpenFile("run_stats", "w")
    file_cnt = 0
    incompleteCnt = 0
    empty_file_cnt = 0
    successCnt = 0

    dbg = args.dbg
    # Single file
    if len(args.in_list_file) == 0:
        readFasterCapOutPutLog(args.in_file, outFP, warnFP, emptyFP, statsFP, dbg)
        file_cnt += 1

        if retCode[0] == 1:
            successCnt += 1
            if "Incomplete" in retCode[2]:
                incompleteCnt += 1

        if retCode[0] == 0 and "Empty" in retCode[2]:
            empty_file_cnt += 1
        if retCode[0] == 0 and "Incomplete" in retCode[2]:
            incompleteCnt += 1
        exit()

    # list of files
    f = OpenFile(args.in_list_file)
    for file_line in f:
        retCode = readFasterCapOutPutLog(
            file_line.split()[0], outFP, warnFP, emptyFP, statsFP, dbg
        )
        file_cnt += 1

        if retCode[0] == 1:
            successCnt += 1
            if "Incomplete" in retCode[2]:
                incompleteCnt += 1

        if retCode[0] == 0 and "Empty" in retCode[2]:
            empty_file_cnt += 1
        if retCode[0] == 0 and "Incomplete" in retCode[2]:
            incompleteCnt += 1

    outFP.close()
    warnFP.close()
    emptyFP.close()
    statsFP.close()

    print(file_cnt, " Files Parsed")
    print(empty_file_cnt, " Files are Empty -- look at file:empty_files")
    print(incompleteCnt, " Files were incomplete -- look at file: warnings")


if __name__ == "__main__":
    main()

# Dimension 7 x 7
# g1_wire_M3_w0  6.9136e-16 -5.40629e-17 -1.68603e-16 -1.02291e-16 -9.64771e-17 -1.05795e-16 -1.6943e-16
# g2_wire_M5_w0  -7.3632e-17 2.82679e-16 -1.01845e-16 -5.93445e-17 -5.83917e-17 -5.96175e-17 -1.01798e-16
# g3_wire_M4_w1  -1.41823e-16 -2.30241e-17 5.5345e-16 -2.76143e-16 -3.19856e-18 1.15872e-18 -2.70547e-20
# g4_wire_M4_w2  -9.1805e-17 -1.36862e-17 -2.73845e-16 7.20986e-16 -2.7526e-16 -4.54052e-18 1.38176e-18
# g5_wire_M4_w3  -9.05258e-17 -1.32927e-17 -3.24811e-18 -2.75761e-16 7.2318e-16 -2.80613e-16 -3.72233e-18
# g6_wire_M4_w4  -9.13211e-17 -1.37111e-17 1.56117e-18 -4.38767e-18 -2.81118e-16 7.2663e-16 -2.7262e-16
# g7_wire_M4_w5  -1.4171e-16 -2.30185e-17 -3.0492e-20 8.26274e-19 -5.05703e-18 -2.73595e-16 5.535e-16

# Solve statistics:
# Total allocated memory: 4950569 kilobytes
# Total time: 2578.236084s (0 days, 0 hours, 42 mins, 58 s)
