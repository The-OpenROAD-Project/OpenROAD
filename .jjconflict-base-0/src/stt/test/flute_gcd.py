# flute gcd
from openroad import Design, Tech
import helpers
import stt_aux

tech = Tech()
design = Design(tech)

nets = stt_aux.read_nets("gcd.nets")

for net in nets:
    stt_aux.report_flute_net(design, net)
