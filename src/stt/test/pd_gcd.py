# prim-dikstra gcd
from openroad import Design, Tech
import helpers
import stt_aux

tech = Tech()
design = Design(tech)

nets = stt_aux.read_nets("gcd.nets")
alpha = 0.8

for net in nets:
    stt_aux.report_pd_net(design, net, alpha)
