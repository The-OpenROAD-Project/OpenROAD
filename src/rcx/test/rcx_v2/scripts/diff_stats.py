import os
import re
import argparse
import math
import xlsxwriter
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


def mkdir(path):
    if not os.path.exists(path):
        os.makedirs(path)


class Pattern:
    def __init__(self):
        self.cnt5 = 0
        self.cnt1 = 0
        self.stats = []

    def makeStats(targetPercent1, targetPercent5, capTable, sfile):
        cnt5 = 0
        cnt1 = 0
        stats = []
        stats.append(k)
        stats.append("Wire" + str(i))
        stats.append(totCnt)
        capTable.sort()
        for s in capTable:
            dd = s[0]
            if dd > targetPercent5 or dd < -targetPercent5:
                cnt5 += 1
            if dd > targetPercent1 or dd < -targetPercent1:
                cnt1 += 1

            line = " ".join(map(str, s))
            ofile.write(line + "\n")

        stats.append(targetPercent1)
        stats.append(cnt1)
        stats.append(round(100.0 * cnt1 / totCnt, 1))
        stats.append(targetPercent5)
        stats.append(cnt5)
        stats.append(round(100.0 * cnt5 / totCnt, 1))
        sline = " ".join(map(str, stats))
        sfile.write(sline + "\n")


parser = argparse.ArgumentParser(description="Patterns Error Distribution")
parser.add_argument(
    "-target_error_percent",
    type=float,
    default=3.0,
    help="Report on error greater than abs value, default=3.0",
)
# parser.add_argument('-target_error_percent_hi', type=float, default=5.0, help='Report on error greater than abs value, default=5.0')
parser.add_argument(
    "-corner", type=int, default=0, help="target extraction corner, default=0"
)
parser.add_argument(
    "-diff_spef_out_file",
    type=str,
    default="diff_spef.out",
    help="diff_spef.out file after running diff_spef command, default=diff_spef.out",
)
parser.add_argument(
    "-out_dir",
    type=str,
    default="stats_diff_spef",
    help="directory name for all reports, default=stats_diff_spef",
)
args = parser.parse_args()

dir_name = args.out_dir
if not os.path.exists(dir_name):
    os.mkdir(dir_name)
cntx = "cntx"
diag = "DU"
capDict = {"netCap": 1, "netRes": 2, "netGndCap": 3, "netCcap": 4}
wireDict = {"_1": 1, "_2": 2, "_3": 3, "_4": 4, "_5": 5}
patDict = {"O6": 1, "U6": 2, "OU6": 3, "R6": 4, "DU6": 5, "V2": 6}

f = open("diff_spef.out", "r")

pat = [0, 1, 2, 3, 4, 5, 6, 7, 8]
cap = [0, 1, 2, 3, 4, 5, 6, 7, 8]
for i in range(8):
    cap[i] = [0, 1, 2, 3, 4, 5, 6]
    pat[i] = [0, 1, 2, 3, 4, 5, 6]
    for j in range(6):
        cap[i][j] = []
        pat[i][j] = [0, 1, 2, 3, 4, 5, 6]
        for p in range(7):
            pat[i][j][p] = []

dbg0 = 0
cnt = 0
rows = []
for line in f:
    if line.find(cntx) != -1:
        continue
    if line.find("netCcap") != -1:
        continue
    if line.find("ccCap") != -1:
        continue
    if line.find("netGndCap") != -1:
        continue

    row = line.split()
    row[0] = float(row[0])
    rows.append(row)

    netName = row[len(row) - 1]
    nameSeg = netName.split("_")

    # Via patterns can have . or - as separators
    wireNum = 0
    if netName.startswith("V") and netName.find("V2") != -1:
        nameSeg = netName.split("-")
        if nameSeg[0] != "V2":
            nameSeg = netName.split(".")
        isVia = 1
        viaWire = nameSeg[len(nameSeg) - 1].split("W")
        wireNum = int(viaWire[1])
        if dbg0 > 0:
            print(netName, " ", nameSeg, viaWire, wireNum)
    else:
        wireNum = int(nameSeg[len(nameSeg) - 1])

    pattern = patDict[nameSeg[0]]
    if len(row) < 22:
        continue

    resKey = row[21]
    capType = row[19]
    cornerWord = row[21]
    if capType not in capDict.keys():
        capType = row[21]
        cornerWord = row[23]

    corner = int(cornerWord)
    if corner != args.corner:
        continue

    capIndex = capDict[capType]

    if dbg0 > 0:
        print("pattern=", pattern, nameSeg)
        print("wireNum", wireNum, "capIndex", capIndex, "pattern", pattern)

    pat[wireNum][capIndex][pattern].append(row)
    if pattern == "DU6":
        print(row)

    cap[wireNum][capIndex].append(row)
    cnt = cnt + 1

sfile = open("diff_spef.stats", "w")
all_outliers_file = open("outliers.list", "w")

# targetPercent5=args.target_error_percent_hi
targetPercent1 = args.target_error_percent
for k in capDict.keys():  # netCap, resRes
    if k == "netCcap":
        continue
    if k == "netGndCap":
        continue

    capIndex = capDict[k]

    for i in range(6):  # wires 1,2,3,4,5

        totCnt = len(cap[i][capIndex])
        if totCnt == 0:
            continue
        dirPath = dir_name + "/" + k + "/" + str(i)
        mkdir(dirPath)
        outFile = dirPath + "/" + str(i) + "." + k
        outliersFile = outFile + ".outliers"
        # print('outFile=',outFile)
        ofile = open(outFile, "w")
        outfile = open(outliersFile, "w")
        # print(' wire ', i, ' capIndex ', capIndex)
        cap[i][capIndex].sort()
        cnt5 = 0
        cnt1 = 0
        stats = []
        stats.append(k)
        stats.append("Wire" + str(i))
        stats.append("totCnt")
        stats.append(totCnt)
        for s in cap[i][capIndex]:
            line = " ".join(map(str, s))
            dd = float(s[0])
            # if dd>targetPercent5 or dd<-targetPercent5: cnt5+=1
            if dd > targetPercent1 or dd < -targetPercent1:
                outfile.write(line + "\n")
                all_outliers_file.write(line + "\n")
                cnt1 += 1

            # print(' '.join(str(e) for e in s),'\n')
            ofile.write(line + "\n")

        stats.append("targetPercentError")
        stats.append(targetPercent1)
        stats.append("outlierCnt")
        stats.append(cnt1)
        stats.append("outlierPercent")
        stats.append(round(100.0 * cnt1 / totCnt, 1))

        sline = " ".join(map(str, stats))
        sfile.write(sline + "\n")
        print(stats)
        ofile.close()

dbg1 = 0
for p in patDict.keys():
    patIndex = patDict[p]

    wireList = []
    for wireNum in range(6):
        if wireNum == 0:
            continue

        wireList.append(wireNum)

        for resCap in capDict.keys():
            if resCap == "netCcap":
                continue
            if resCap == "netGndCap":
                continue

            capIndex = capDict[resCap]

            dirPath = dir_name + "/" + resCap + "/" + p
            mkdir(dirPath)

            outFile = dirPath + "/" + p + "." + str(wireNum) + "." + resCap
            outliersFile = (
                dirPath + "/" + p + "." + str(wireNum) + "." + resCap + ".outliers"
            )

            totCnt = len(pat[wireNum][capIndex][patIndex])
            if totCnt == 0:
                continue

            caps = pat[wireNum][capIndex][patIndex]
            caps.sort()

            if dbg1 > 0:
                print("outFile=", outFile)
                print("outLiers=", outliersFile)
                print(
                    "wireNum=",
                    wireNum,
                    "  capIndex=",
                    capIndex,
                    "  patIndex=",
                    patIndex,
                )
                for s in pat[wireNum][capIndex][patIndex]:
                    print(s)
                print(
                    "-------------------------------------------------------------------------------------------"
                )

            ofile = open(outFile, "w")
            lfile = open(outliersFile, "w")

            caps.sort()
            cnt5 = 0
            cnt1 = 0
            stats = []
            stats.append(resCap)
            stats.append(p)
            stats.append("Wire" + str(wireNum))
            stats.append("totCnt")
            stats.append(totCnt)
            for s in caps:
                line = " ".join(map(str, s))
                dd = float(s[0])
                if dd > targetPercent1 or dd < -targetPercent1:
                    lfile.write(line + "\n")
                    cnt1 += 1

                ofile.write(line + "\n")

            stats.append("targetPercentError")
            stats.append(targetPercent1)
            stats.append("outlierCnt")
            stats.append(cnt1)
            stats.append("outlierPercent")
            stats.append(round(100.0 * cnt1 / totCnt, 1))
            sline = " ".join(map(str, stats))
            sfile.write(sline + "\n")
            print(stats)
            ofile.close()
