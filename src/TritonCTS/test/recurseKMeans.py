#!/usr/bin/env python3
import pandas as pd
from sklearn.cluster import KMeans
from sklearn import preprocessing
import argparse

cmdLineParser = argparse.ArgumentParser()
cmdLineParser.add_argument('-i', action='store', dest='inpCsvFile', type=str, required=True,
                            help='Input Csv File With Node Locations')
cmdLineParser.add_argument('-o', action='store', dest='outFile', type=str, required=True,
                            help='Output File With Cluster Id for each Node')
cmdLineParser.add_argument('-s', action='store', dest='clusterSize', default=30, type=int,
                            help='Size of Each Cluster')
cmdLineParser.add_argument('-d', action='store', dest='clusterDiameter', default=0.27, type=float,
                            help='Max Diameter of Each Cluster')

def clusterNodes(csvFile:str, clusterSize:int=30, clusterDiameter:float=0.27)->pd.DataFrame :
    retDf = pd.read_csv(csvFile)
    retDf["Label"]="0"
    # Define the aggregation calculations
    aggregations = {
        # Bounds
        'NodeLocX': ['max', 'min'],
        'NodeLocY': ['max', 'min'],
        'Label' : 'count'
    }
    labelEncoder = preprocessing.LabelEncoder()
    nodeBounds = None
    print(f"Clustering Data Frame of DF of Size : {len(retDf)}, DataFrame Head : \n{retDf.head()}")
    while True :
        nodeBounds = retDf.groupby(by="Label").agg(aggregations)
        nodeBounds.columns = ["xMax", "xMin", "yMax", "yMin", "NodeCount"]
        nodeBounds['xSpan'] = nodeBounds.xMax - nodeBounds.xMin
        nodeBounds['ySpan'] = nodeBounds.yMax - nodeBounds.yMin
        bigClusters = nodeBounds[(nodeBounds.NodeCount > clusterSize) |\
                                 (nodeBounds.xSpan > clusterDiameter) |\
                                 (nodeBounds.ySpan > clusterDiameter)].index.values
        if bigClusters.size == 0 :
            break
        for clusterId in bigClusters :
            nodeIds = retDf[retDf.Label == clusterId].index.values
            X = retDf.loc[nodeIds, ['NodeLocX', 'NodeLocY']].values
            kmeans = KMeans(n_clusters=2, random_state=0).fit(X)
            retDf.loc[nodeIds, 'Label'] += kmeans.labels_.astype(str)

    labelEncoder.fit(retDf.Label)
    retDf['ClusterId'] = labelEncoder.transform(retDf.Label)
    return (retDf, nodeBounds)

if __name__ == "__main__" :
    cmdArgs = cmdLineParser.parse_args()
    clusterDf, boundsDf = clusterNodes(cmdArgs.inpCsvFile, cmdArgs.clusterSize, cmdArgs.clusterDiameter)
    clusterDf.sort_values(by="ClusterId")['ClusterId'].to_csv(cmdArgs.outFile, header=False, index=True, sep=" ")
    clusterDf.to_csv(f"{cmdArgs.inpCsvFile.split('.')[0]}_clusterData.csv", index=False)
    boundsDf.to_csv(f"{cmdArgs.inpCsvFile.split('.')[0]}_boundData.csv", index=False)
    #clusterDf.ClusterId.to_csv(cmdArgs.outFile, header=False, index=False)
