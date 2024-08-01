from openroad import Design, Tech
import stt_aux
import helpers

tech = Tech()
design = Design(tech)

alpha = 0.4
dup1 = [["dup1", 0], [["p0", 0, 0], ["p1", 10, 10], ["p2", 10, 20], ["p3", 10, 10]]]

stt_aux.report_pd_net(design, dup1, alpha)

dup2 = [["dup2", 2], [["p0", 29, 43], ["p1", 28, 45], ["p2", 28, 44]]]

stt_aux.report_pd_net(design, dup2, alpha)

dup3 = [["dup3", 2], [["p0", 100, 56], ["p1", 65, 37], ["p2", 65, 38]]]
stt_aux.report_pd_net(design, dup3, alpha)

# 2 duplicate points
dup4 = [["dup4", 0], [["p0", 10, 10], ["p1", 10, 10]]]
stt_aux.report_pd_net(design, dup4, alpha)


one = [["one", 0], [["p0", 10, 10]]]
stt_aux.report_pd_net(design, one, alpha)

# driver index changes by duplicate removal
dup5 = [
    ["dup5", 2],
    [["p0", 123, 209], ["p1", 123, 209], ["p2", 123, 215], ["p3", 122, 211]],
]
stt_aux.report_pd_net(design, dup5, 0.3)
