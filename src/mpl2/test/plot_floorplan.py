import os
import matplotlib.pyplot as plt

file_name = "./rtl_mp/final_floorplan.txt"

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


with open(file_name) as f:
    content = f.read().splitlines()
f.close()


outline_width = float(content[0].split()[-1])
outline_height = float(content[1].split()[-1])


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


plt.figure()
for i in range(len(cluster_list)):
    rectangle = plt.Rectangle((cluster_lx_list[i], cluster_ly_list[i]), cluster_ux_list[i] - cluster_lx_list[i],
                              cluster_uy_list[i] - cluster_ly_list[i], fc = "r", ec = "blue")
    plt.gca().add_patch(rectangle)


for i in range(len(macro_list)):
    rectangle = plt.Rectangle((macro_lx_list[i], macro_ly_list[i]), macro_ux_list[i] - macro_lx_list[i],
                              macro_uy_list[i] - macro_ly_list[i], fc = "yellow", ec = "blue")
    plt.gca().add_patch(rectangle)


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


















