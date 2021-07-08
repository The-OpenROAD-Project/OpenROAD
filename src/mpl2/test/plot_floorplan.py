import os
import matplotlib.pyplot as plt
from math import log


file_name = "./results/mp_test1/final_floorplan.txt"
net_file = "./results/mp_test1/mp_test1.net"

net_threshold = 0

cluster_list = []
cluster_lx_list = []
cluster_ly_list = []
cluster_ux_list = []
cluster_uy_list = []

macro_list = []
macro_lx_list = []
macro_ly_list = []
macro_ux_list = []
macro_uy_list = []

cluster_dict = {}


with open(file_name) as f:
    content = f.read().splitlines()
f.close()


outline_width = float(content[0].split()[-1])
outline_height = float(content[1].split()[-1])

terminal_dict = {}

terminal_dict["LM"] = [0, outline_height / 2.0]
terminal_dict["RM"] = [outline_width, outline_height / 2.0]
terminal_dict["BM"] = [outline_width / 2.0, 0]
terminal_dict["TM"] = [outline_width / 2.0, outline_height]
terminal_dict["LL"] = [0, outline_height / 6.0]
terminal_dict["RL"] = [outline_width, outline_height / 6.0]
terminal_dict["BL"] = [outline_width / 6.0, 0.0]
terminal_dict["TL"] = [outline_width / 6.0, outline_height]
terminal_dict["LU"] = [0, outline_height * 5.0 / 6.0]
terminal_dict["RU"] = [outline_width, outline_height * 5.0 / 6.0]
terminal_dict["BU"] = [outline_width * 5.0 / 6.0, 0.0]
terminal_dict["TU"] = [outline_width * 5.0 / 6.0, outline_height]




i = 2
while(i < len(content)):
    words = content[i].split()
    if(len(words) == 0):
        break
    else:
        cluster_list.append(words[0])
        cluster_lx_list.append(float(words[1]))
        cluster_ly_list.append(float(words[2]))
        cluster_ux_list.append(float(words[3]))
        cluster_uy_list.append(float(words[4]))
        cluster_dict[words[0]] = [(float(words[1]) + float(words[3])) / 2.0,  (float(words[2]) + float(words[4])) / 2.0]



    i = i + 1


i = i + 1
while(i < len(content)):
    words = content[i].split()
    macro_list.append(words[0])
    macro_lx_list.append(float(words[1]))
    macro_ly_list.append(float(words[2]))
    macro_ux_list.append(float(words[3]))
    macro_uy_list.append(float(words[4]))
    i = i + 1




net_list = []

with open(net_file) as f:
    content = f.read().splitlines()
f.close()

for line in content:
    items = line.split()
    if(len(items) > 1):
        source = items[1]
        for j in range(2, len(items), 2):
            target = items[j]
            weight = float(items[j+1])
            net_list.append([source, target, weight])


plt.figure()
for i in range(len(cluster_list)):
    rectangle = plt.Rectangle((cluster_lx_list[i], cluster_ly_list[i]), cluster_ux_list[i] - cluster_lx_list[i],
                              cluster_uy_list[i] - cluster_ly_list[i], fc = "r", ec = "blue")
    plt.gca().add_patch(rectangle)


for i in range(len(macro_list)):
    rectangle = plt.Rectangle((macro_lx_list[i], macro_ly_list[i]), macro_ux_list[i] - macro_lx_list[i],
                              macro_uy_list[i] - macro_ly_list[i], fc = "yellow", ec = "blue")
    plt.gca().add_patch(rectangle)



for i in range(len(net_list)):
    source = net_list[i][0]
    target = net_list[i][1]
    weight = net_list[i][2]

    x = []
    y = []

    if source in cluster_dict:
        x.append(cluster_dict[source][0])
        y.append(cluster_dict[source][1])
    else:
        x.append(terminal_dict[source][0])
        y.append(terminal_dict[source][1])

    if target in cluster_list:
        x.append(cluster_dict[target][0])
        y.append(cluster_dict[target][1])
    else:
        x.append(terminal_dict[target][0])
        y.append(terminal_dict[target][1])

    if weight > net_threshold:
        plt.plot(x,y,'k', lw = log(weight))





x = []
y = []
x.append(0)
y.append(0)
x.append(outline_width)
y.append(0)
plt.plot(x,y, '--k')

x = []
y = []
x.append(0)
y.append(0)
x.append(0)
y.append(outline_height)
plt.plot(x,y, '--k')

x = []
y = []
x.append(0)
y.append(outline_height)
x.append(outline_width)
y.append(outline_height)
plt.plot(x,y, '--k')

x = []
y = []
x.append(outline_width)
y.append(0)
x.append(outline_width)
y.append(outline_height)
plt.plot(x,y, '--k')


plt.xlim(0, outline_width)
plt.ylim(0, outline_height)
plt.axis("scaled")
plt.show()


















