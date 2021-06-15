#! /home/kshan/anaconda2/bin/python

import matplotlib
matplotlib.use("TKAGG")
import pandas as pd
import sys
import matplotlib.gridspec as gridspec
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.colorbar import ColorbarBase, make_axes_gridspec
from matplotlib import cm
import matplotlib.cm as cmx
import matplotlib.colors as colors
import matplotlib.ticker as mtick
from matplotlib.ticker import FuncFormatter
import numpy as np
 
#the magic of 3D arrow
from matplotlib.patches import FancyArrowPatch
from mpl_toolkits.mplot3d import proj3d
#the magic of 3D shading 
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

def plotTree(data, fn, fanout, title):
  plt.figure()
  ax = plt.subplot()
  font = {'family': 'serif',
        'color':  'darkred',
        'weight': 'normal',
        'size': 5,
        }
  for i in range(len(data[:,0])):
    if (i == 0):
      plt.plot(data[i,0], data[i,1], 'ko', c='r', label="source")
    elif (i == 1):
      plt.plot(data[i,0], data[i,1], 'ko', c='b', label="sinks")
    elif (i < fanout):
      plt.plot(data[i,0], data[i,1], 'ko', c='b')
    elif (i == int(fanout)):
      plt.plot(data[i,0], data[i,1], 'ko', c='g', label="Steiner")
    else:
      plt.plot(data[i,0], data[i,1], 'ko', c='g')
    #plt.text(data[i,0]+1, data[i,1]+1, i, fontdict=font);
  
  for i in range(len(data[:,0])):
    if (i == 0):
      continue
    x1 = data[i,0]
    x2 = data[data[i,2],0]
    y1 = data[i,1]
    y2 = data[data[i,2],1]
    plt.plot([x1, x2], [y1, y2], c='y', linewidth=2.0)
  plt.legend(loc='upper left', bbox_to_anchor=(1,1))
  axis_font = {'fontname':'Arial', 'size':'7'}
  plt.tight_layout(pad=8)
  for label in (ax.get_xticklabels() + ax.get_yticklabels()):
      label.set_fontname('Arial')
      label.set_fontsize(8)
  plt.xlabel("x", **axis_font)
  plt.ylabel("y", **axis_font)
  plt.suptitle(title, fontsize=10)
  plt.savefig(fn)
  plt.close("all")


def readTree(filename, idx):
  fin = open(filename, 'r')
  s = 'n%d' % (idx)
  flag = False
  cnt = 0;
  for line in fin:
    if ("Net" in line.split(' ')):
      cnt = cnt + 1
    if (cnt == idx):
      break

  for line in fin:
    if(len(line.split(' ')) < 2):
      break
    X = float(line.split(' ')[1])
    Y = float(line.split(' ')[2]) 
    pid = int(line.split(' ')[3])
    print(line)
    if (flag == False):
      data = np.array([X, Y, pid])
      flag = True
    else:
      data = np.vstack([data, np.array([X, Y, pid])])

  fin.close()

  return data

def parseData(filename):
  fin  = open(filename, 'r')
  cnt = 0;
  for line in fin:
    if ('Graph' in line.split(' ')):
      cnt = cnt + 1
  fin.close();

  data = np.zeros((cnt, 4))
  fin  = open(filename, 'r')
  line = fin.readline()
  cnt = 0
  for line in fin:
    if ('Graph' in line.split(' ')):
      WL = float(line.split(' ')[3])
      MWL = float(line.split(' ')[5])
      Light = float(line.split(' ')[7])
      Shallow = float(line.split(' ')[9])
      data[cnt] = np.asarray([WL, MWL, Light, Shallow]) 
      cnt = cnt + 1
  fin.close()
  return data

#alpha = sys.argv[1]
#esp = sys.argv[2]
#fanout = sys.argv[3]
#idx = int(sys.argv[4])
#
#sol1 = "PD_opt/PD_opt_p%s_alpha_%s_out" % (fanout,alpha)
#sol2 = "SALT/SALT_p%s_eps_%s_out" % (fanout,esp)
#
#PDRev = parseData(sol1)
##PDRev = parseData("./SB1_4_new_daf")
#SALT = parseData(sol2)
#
#n1 = "PD_opt/PD_opt_p%s_alpha_%s" % (fanout,alpha)

title = "PD"
fanout = 20
data = readTree(title + ".txt", 1)
fn = title + ".png"
plotTree(data, fn, fanout, title)

title = "PD_II"
data = readTree(title + ".txt", 1)
fn = title + ".png"
plotTree(data, fn, fanout, title)

title = "HVW"
data = readTree(title + ".txt", 1)
fn = title + ".png"
plotTree(data, fn, fanout, title)

title = "DAS"
data = readTree(title + ".txt", 1)
fn = title + ".png"
plotTree(data, fn, fanout, title)

