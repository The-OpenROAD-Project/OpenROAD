import os
import re
import argparse
import math
import xlsxwriter
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


class DiffPlot:
    def Plot3x3(self, xyBuckets, xyBuckets2, wl, units, suptitle):
        cnt = len(xyBuckets)
        cnt2 = len(xyBuckets2)
        print(" bucket count= ", cnt)
        rowCnt = 3
        colCnt = 3
        # Create a 3x3 grid of subplots
        fig, axs = plt.subplots(
            rowCnt, colCnt, figsize=(10, 10)
        )  # Adjust figsize as needed
        fig.suptitle(suptitle + "\n", fontsize=16)
        # Loop through the subplots and plot the data
        i = -1
        j = -1
        for k in range(cnt):
            if k % colCnt == 0:
                i = i + 1
                j = 0

            e = xyBuckets[k]
            x = e[0]
            y = e[1]
            y1 = []
            if cnt2 > 0:
                y1 = xyBuckets2[k][1]

            count = e[4]

            label = e[3] + " " + units + "  NetCnt:" + str(count)

            if cnt2 > 0:
                axs[i, j].plot(x, y, label="Now")
                axs[i, j].plot(x, y1, label="Before")
            else:
                axs[i, j].plot(x, y)
            axs[i, j].set_title(label)
            axs[i, j].set_xlabel("Error %")
            axs[i, j].set_ylabel("Count")
            j = j + 1

        # Adjust layout to prevent overlapping titles
        plt.tight_layout()

        # Show the plot
        plt.show()

    def PlotPercentError(self, xyBuckets):

        # Create a scatter plot
        # plt.figure(figsize=(8, 8))  # Adjust the figure size as needed

        for e in xyBuckets:
            x = e[0]
            y = e[1]
            label = e[2] + "_WL_" + str(e[3])
            # label='_WL_' + str(e[3])
            print(x)
            print(y)
            print(label)
            plt.plot(x, y, label=label)

        # Add labels and a legend
        plt.xlabel("Dist")
        plt.ylabel("Error%")
        plt.legend()

        # Add a title
        plt.title("NetCap Error Distribution")

        # Display the plot
        plt.grid(True)
        plt.show()


class Bucket:
    def __init__(_, start, n, step, dbg=0):
        _.dbg = dbg
        _.start = start
        _.last = n
        _.step = step
        _.value = []
        _.cnt = []
        for i in range(_.last):
            v = _.start + i * step
            _.value.append(v)
            _.cnt.append(0)

    def GetValues(_):
        return _.value

    def GetCounts(_):
        return _.cnt

    def GetPercent(_, totCnt):
        percent = []
        for ii in range(_.last):
            if totCnt == 0:
                percent.append(0)
            else:
                percVal = round(100.0 * _.cnt[ii] / totCnt, 1)
                percent.append(percVal)
        return percent

    def GetXY(_, percent, cnt, totCnt, X, Y):
        for ii in range(_.last - 1):
            percVal = round(100.0 * cnt[ii] / totCnt, 1)
            X.append(_.value[ii])
            Y.append(percVal)

    def GetRange(_, low, hi):
        # print("last=  ", _.last)
        totCnt = sum(_.cnt)
        cnt = 0
        for ii in range(_.last - 1):
            # totCnt= totCnt + _.cnt[ii]
            if not (_.value[ii] >= low and _.value[ii] < hi):
                continue
            print(ii, " ", _.value[ii], " ", _.value[ii + 1], " cnt=", _.cnt[ii])
            cnt = cnt + _.cnt[ii]
        percVal = 0
        if totCnt > 0:
            percVal = round(100.0 * cnt / totCnt, 1)

        print(percVal, " range: ", low, "-", hi, " rangeCnt= ", cnt, " Total= ", totCnt)
        rng = [percVal, low, hi, cnt, totCnt]
        print(rng)

    def PrintAll(_):
        print("last=  ", _.last)
        for ii in range(_.last - 1):
            print(ii, " ", _.value[ii], " ", _.value[ii + 1], " cnt=", _.cnt[ii])
        ii = ii + 1
        print(ii, " ", _.value[ii], " ", _.value[ii], " cnt=", _.cnt[ii])

    def Print(_, dd, ii):
        if ii == _.last - 1:
            print(dd, " ", _.value[ii], " ", _.value[ii], " ", ii, " cnt=", _.cnt[ii])
        else:
            print(
                dd, " ", _.value[ii], " ", _.value[ii + 1], " ", ii, " cnt=", _.cnt[ii]
            )

    def Update(_, dd, ii):
        _.cnt[ii] = _.cnt[ii] + 1
        if _.dbg > 0:
            _.Print(dd, ii)

    def Add(_, dd):
        if _.dbg > 0:
            print(dd)

        if dd <= _.value[0]:
            _.Update(dd, 0)
            return 0

        for ii in range(_.last - 1):
            if dd >= _.value[ii] and dd < _.value[ii + 1]:
                _.Update(dd, ii)
                return ii
        n = _.last - 1
        _.Update(dd, n)
        return n

    def AddAll(_, rows, col):
        for e in rows:
            _.Add(e[col])

    def PrintBucket(self, percent, cnt, totCnt):
        for ii in range(len(percent) - 1):
            percVal = round(100.0 * cnt[ii] / totCnt, 1)
            # print (percent[ii], percent[ii+1], " ", cnt[ii], "  ", percVal)
            print(
                " %4d " % percent[ii],
                " %4d " % percent[ii + 1],
                " %6d " % cnt[ii],
                " %5.1f " % percVal,
            )

    def GetBucketXY(self, percent, cnt, totCnt, X, Y):
        for ii in range(len(percent) - 1):
            percVal = round(100.0 * cnt[ii] / totCnt, 1)
            X.append(percent[ii])
            Y.append(percVal)
            # print (' %4d ' % percent[ii], ' %4d ' % percent[ii+1], ' %6d ' %cnt[ii], ' %5.1f ' %percVal)


class DiffFilter:
    def __init__(_, resCapType, name, wl, excludeName, corner, dbg=0):
        _.dbg = dbg
        _.row_type = resCapType
        _.name = name
        _.value_type = wl
        _.exclude = excludeName
        _.corner = corner

    def Filter(_, row, result):
        n = len(row)
        netName = row[n - 1]

        if not _.name.find("All") >= 0 and not netName.find(_.name) >= 0:
            return True

        wlen = 0
        resCap = ""
        if row[21].find("netRes") >= 0:
            wlen = int(row[18])
            resCap = row[21]
            corner = int(row[23])
        elif row[19].find("netCap") >= 0:
            wlen = int(row[14])
            resCap = row[19]
            corner = int(row[21])
        else:
            return True

        if corner != _.corner:
            return True

        if not resCap.find(_.row_type) >= 0:
            return True

        e = float(row[0])
        v = float(row[1])
        refVal = float(row[6])

        if _.value_type == 1:
            result.append(v)
        else:
            result.append(wlen)

        result.append(e)
        result.append(v)
        result.append(refVal)
        result.append(wlen)
        result.append(resCap)
        result.append(netName)

        if _.dbg > 0:
            print("FILTER= ", result)

        return False


class Diff:
    def __init__(_, n, dbg=0):
        _.dbg = dbg
        _.rowBuckets = []
        _.cnt = n
        for ii in range(n):
            _.rowBuckets.append(ii)
            _.rowBuckets[ii] = []

    def Write(_, prefix, dir_name, X):
        if not os.path.exists(dir_name):
            os.mkdir(dir_name)
        for ii in range(_.cnt):
            outFile = dir_name + "/" + prefix + "." + str(X[ii]) + "." + str(ii)
            print("outFile=", outFile)
            ofile = open(outFile, "w")
            _.rowBuckets[ii].sort()
            for b in _.rowBuckets[ii]:
                line = " ".join(map(str, b))
                # line= str(dd) + ' ' + str(adjustPercent) + ' ' + str(avg) + ' ' + line
                ofile.write(line + "\n")
            ofile.close()

    # def Buckets(_, rows, capType, WL_Val, buckets) :
    def Buckets(_, rows, row_filter, buckets):
        for row in rows:
            if _.dbg > 0:
                print(row)

            res = []
            if row_filter.Filter(row, res):
                continue

            n = buckets.Add(res[0])  #

            #   n= buckets.Add(row[6])
            # if WL_Val==1 :
            # n= buckets.Add(row[6])

            # else :
            # if capType.find('netRes')!=-1:
            # wlen= int(row[18])
            # else :
            # wlen= int(row[14])

            # n= buckets.Add(wlen)

            _.rowBuckets[n].append(row)
            # print(_.rowBuckets[n])
        return _.rowBuckets

    def ReadMinMax(_, fileName, row_filter, WL, Val, adjust_percent=1):
        f = open(fileName, "r")

        cnt = 0
        rows = []
        for line in f:
            cnt = cnt + 1
            # Split on any whitespace (including tab characters)
            row = line.split()

            # bug!!!
            if row[2] == "ccCap":
                continue

            if _.dbg > 0:
                print(row)
                print("cnt=", cnt)

            # Convert strings to numeric values:
            percent_err = float(row[0])
            refVal = float(row[6])
            v = float(row[1])
            if adjust_percent > 1 and refVal > 0:
                percent_err = 100.0 * (adjust_percent * v - refVal) / refVal
                # print(row)
                # print (percent_err, row[0], v, refVal)

            row[0] = percent_err
            result = []
            if row_filter.Filter(row, result):
                continue

            wlen = result[0]

            if WL[0] > wlen:
                WL[0] = wlen
            if WL[1] < wlen:
                WL[1] = wlen

            row[6] = refVal
            if Val[0] > refVal:
                Val[0] = refVal
            if Val[1] < refVal:
                Val[1] = refVal

            rows.append(row)
        return rows

    def MakePlotBuckets(_, b100, capType):

        # b100.PrintAll()
        xyBuckets = []
        total = 0
        print("bucket values: ", b100.GetValues())
        for ii in range(b100.last):
            # b= bucket_rows[ii]
            b = _.rowBuckets[ii]
            totCnt = len(b)
            total = total + totCnt
            print("Bucket: ", ii, " Count: ", len(b))
            p30 = Bucket(-30, 12, 5)
            p30.AddAll(b, 0)
            p30.PrintAll()
            p30.GetRange(-10, 10)
            X = p30.GetValues()
            Y = p30.GetCounts()
            percent = p30.GetPercent(totCnt)
            valueRange = "<=" + str(b100.value[ii])
            if ii < b100.last - 1:
                valueRange = str(b100.value[ii]) + "-" + str(b100.value[ii + 1])
            elif ii > 0:
                valueRange = ">=" + str(b100.value[ii])

            xyBuckets.append([p30.value, p30.cnt, capType, valueRange, totCnt])
            # print(xyBuckets)
        print(" Total Nets = ", total)
        return xyBuckets

    def Read(_, fileName, tgt_capType, capTypeDict, WL, Val):

        cntx = "cntx"
        diag = "DU"
        capDict = {"netCap": 1, "netRes": 2, "netGndCap": 3, "netCcap": 4}
        # lenDict={0 : 0, 1000 : 1, 2000 : 2, 5000 : 3, 10000 : 4, 20000 : 5, 50000 : 6, 100000 : 7}
        lenDict = {0: 0, 5000: 1, 10000: 2, 20000: 3, 50000: 4, 100000: 5, 200000: 6}

        f = open(fileName, "r")

        pat = [0, 1, 2, 3, 4, 5]
        cap = [0, 1, 2, 3, 4, 5]
        for i in range(6):
            cap[i] = [0, 1, 2, 3, 4, 5, 6, 7, 8]
            pat[i] = [0, 1, 2, 3, 4, 5, 6, 7, 8]
            for j in range(8):
                cap[i][j] = []
                pat[i][j] = []

        dbg1 = 0
        cnt = 0
        rows = []
        for line in f:
            # Split on any whitespace (including tab characters)
            row = line.split()
            if row[2] == "ccCap":
                continue
            if dbg1 > 0:
                print(row)
                print("cnt=", cnt)
            # Convert strings to numeric values:
            row[0] = float(row[0])
            resKey = row[21]
            capType = row[19]
            if dbg1 > 0:
                print("cnt=", cnt, "res ", resKey, " cap ", capType)
                print(capDict)
            wlen = 0
            if capType not in capDict.keys():
                capType = row[21]
                wlen = int(row[18])
            else:
                wlen = int(row[14])

            # if wlen==0 : continue

            capIndex = capDict[capType]

            if wlen == 0:
                lenIndex = 0
            elif wlen <= 5000:
                lenIndex = lenDict[5000]
            elif wlen <= 10000:
                lenIndex = lenDict[10000]
            elif wlen <= 20000:
                lenIndex = lenDict[20000]
            elif wlen <= 50000:
                lenIndex = lenDict[50000]
            elif wlen <= 100000:
                lenIndex = lenDict[100000]
            else:
                lenIndex = lenDict[200000]

            if dbg1 > 0:
                print(row)
                print("capType ", capType, "  wlen ", wlen, " lenIndex ", lenIndex)

            # row[2] = int(row[2])
            # Append to our list of lists:
            rows.append(row)

            netName = row[len(row) - 1]
            # if netName.find(cntx)!=-1:
            # continue

            # print(netName, ' wireNum ', wireNum, ' captype ', capType, ' ', resKey, ' capIndex ', capIndex, '\n')

            cap[capIndex][lenIndex].append(row)
            # print(cap[capIndex][lenIndex])
            # print('\n')
            cnt = cnt + 1


parser = argparse.ArgumentParser(
    description="Diff_Spef_out Total Net Resistance ot Capacitance Error Distribution"
)
parser.add_argument(
    "-rc_value",
    type=str,
    default="netCap",
    help="values should be netCap or netRes, default=netCap",
)
parser.add_argument(
    "-WL_val",
    type=int,
    default=1,
    help="bucket based ditsribution based on 0: WireLoad or 1:Value, default=1",
)
parser.add_argument(
    "-corner", type=int, default=0, help="target extraction corner, default=0"
)
# parser.add_argument('buckets')
parser.add_argument(
    "-name",
    type=str,
    default="All",
    help="all rows or Substring of net Name, default=All",
)
parser.add_argument(
    "-suptitle",
    type=str,
    default="Error Distribution",
    help="Title of Graph, default=Error Distribution",
)
parser.add_argument(
    "-diff_spef_out_file",
    type=str,
    default="diff_spef.out",
    help="diff_spef.out file after running diff_spef command, default=diff_spef.out",
)
parser.add_argument(
    "-diff_spef_out_file_2",
    type=str,
    default="",
    help="comparison diff_spef.out file after running diff_spef command, default=empty",
)
args = parser.parse_args()

WL_flag = int(args.WL_val)

resCap = args.rc_value
corner = args.corner

capDict = {"netCap": 1, "netRes": 2, "netGndCap": 3, "netCcap": 4}

row_filter = DiffFilter(resCap, args.name, WL_flag, "", corner, 0)

units = "fF"
# b100= Bucket(0, 9, 10, 0)
# b2= Bucket(0, 9, 10, 0)
b100 = Bucket(0, 9, 10, 0)
b2 = Bucket(0, 9, 10, 0)
if WL_flag == 0:
    b100 = Bucket(0, 9, 10000, 0)
    b2 = Bucket(0, 9, 10000, 0)
    units = "nm"

diff = Diff(b100.last, 0)
WL = [1000000000, 0]
Val = [1000000000, 0]

rows = diff.ReadMinMax(args.diff_spef_out_file, row_filter, WL, Val, 1.000001)

print("WL minMax ", WL)
print("Val minMax ", Val)
# print(rows)

bucket_rows = diff.Buckets(rows, row_filter, b100)
diff.Write("netCap", "stats", b100.value)
xyBuckets = diff.MakePlotBuckets(b100, "netCap")
# print(" Total Nets = ", total)
print("xyBuckets --------------------- ")
print(xyBuckets)
print(" --------------------- ")

# second diff.out

diff2 = Diff(b2.last, 0)
WL = [1000000000, 0]
Val = [1000000000, 0]

xyBuckets2 = []
if len(args.diff_spef_out_file_2) > 0:
    rows = diff2.ReadMinMax(args.diff_spef_out_file_2, row_filter, WL, Val, 1.000001)
    # print("WL minMax ", WL)
    # print("Val minMax ", Val)
    # print(rows)

    bucket_rows2 = diff2.Buckets(rows, row_filter, b2)
    diff2.Write("netCap", "stats2", b2.value)
    xyBuckets2 = diff2.MakePlotBuckets(b2, "netCap")
    # print(" Total Nets = ", total)
    print(xyBuckets2)

diffPlot = DiffPlot()
diffPlot.Plot3x3(xyBuckets, xyBuckets2, WL_flag, units, args.suptitle)
exit(0)
