import os
import matplotlib.pyplot as plt
from math import log
import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--directory", default = "./results/mp_test1", help = "result directory")
parser.add_argument("--net_threshold", default = -1, help = "nets with weight below net_threshold will not be displayed")
parser.add_argument("--top_n", default = 50, help = "only show the top_n nets")

args = parser.parse_args()
design_dir = args.directory
top_n = args.top_n
net_threshold = args.net_threshold

# users can set this threshold manually
# if it's not set, it will be updated automatically
# only highlight the top n critical edges
#top_n = 50
#net_threshold = -1
highlight_list = []
#design_dir = "./results/mp_test1"
file_name =  design_dir + "/root.fp.txt"
macro_map = {   }
terminal_map = {  }
with open(file_name) as f:
    content = f.read().splitlines()
f.close()

for line in content:
    items = line.split()
    macro_map[items[0]] = [float(items[1]), float(items[2]), float(items[3]), float(items[4])]
    terminal_map[items[0]] = [float(items[1]) + float(items[3]) / 2.0, float(items[2]) + float(items[4]) / 2.0]


file_name = design_dir + "/root.net.txt"
net_map = [  ]
with open(file_name) as f:
    content = f.read().splitlines()
f.close()

net_values = []
for line in content:
    items = line.split()
    net_map.append([items[0], items[1], float(items[2]) + 1])
    net_values.append(float(items[2]) + 1)

if (top_n > 0 and net_threshold <= 1 and len(net_values) > top_n):
  net_values.sort(reverse = True)
  net_threshold = net_values[top_n]
  print("Only highlight the top ", top_n, " edges")
  print("Reset net_threshold to ", net_threshold)


lx = 1e9
ly = 1e9
ux = 0.0
uy = 0.0

plt.figure()
for macro_name, size in macro_map.items():
    color = "r"
    if (macro_name in highlight_list):
        color = "yellow"
    rectangle = plt.Rectangle((size[0], size[1]), size[2], size[3], fc = color, ec = "blue")
    lx = min(lx, size[0])
    ly = min(ly, size[1])
    ux = max(ux, size[0] + size[2])
    uy = max(uy, size[1] + size[3])
    plt.gca().add_patch(rectangle)

for net in net_map:
    source = net[0]
    target = net[1]
    weight = net[2]
    x = []
    y = []
    x.append(terminal_map[source][0])
    x.append(terminal_map[target][0])
    y.append(terminal_map[source][1])
    y.append(terminal_map[target][1])
    if weight > net_threshold:
        plt.plot(x,y,'k', lw = log(weight))



x = []
y = []
x.append(lx)
y.append(ly)
x.append(ux)
y.append(ly)
plt.plot(x,y, '--k')

x = []
y = []
x.append(lx)
y.append(uy)
x.append(ux)
y.append(uy)
plt.plot(x,y, '--k')

x = []
y = []
x.append(lx)
y.append(ly)
x.append(lx)
y.append(uy)
plt.plot(x,y, '--k')


x = []
y = []
x.append(ux)
y.append(ly)
x.append(ux)
y.append(uy)
plt.plot(x,y, '--k')


plt.xlim(lx, ux)
plt.ylim(ly, uy)
plt.axis("scaled")
plt.show()








