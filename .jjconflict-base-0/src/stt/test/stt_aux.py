import stt

FLUTE_ACCURACY = 3


def report_stt_net(design, net, alpha):
    print("Net", net[0][0], flush=True)
    drvr_index = net[0][1]
    pins = net[1]
    xs = [p[1] for p in pins]
    ys = [p[2] for p in pins]
    builder = design.getSteinerTreeBuilder()
    logger = design.getLogger()
    tree = builder.makeSteinerTree(xs, ys, drvr_index, alpha)
    stt.reportSteinerTree(tree, xs[drvr_index], ys[drvr_index], logger)


def report_flute_net(design, net):
    print("Net", net[0][0], flush=True)
    drvr_index = net[0][1]
    pins = net[1]
    xs = [p[1] for p in pins]
    ys = [p[2] for p in pins]
    logger = design.getLogger()
    builder = design.getSteinerTreeBuilder()
    tree = builder.flute(xs, ys, FLUTE_ACCURACY)
    stt.reportSteinerTree(tree, xs[drvr_index], ys[drvr_index], logger)


# Each net is
# {net_name pin_count drvr_index {pin_name x y}...}
def read_nets(filename):
    with open(filename, "r") as fd:
        lines = fd.readlines()
    nets = []
    idx = -1
    while idx < len(lines):
        idx += 1
        ignore, net_name, drvr_index = lines[idx].split()
        pins = []
        while True:
            idx += 1
            if idx >= len(lines) or lines[idx] == "\n":
                break

            pin_name, x, y = lines[idx].split()
            pins.append([pin_name, int(x), int(y)])

        net = [[net_name, int(drvr_index)], pins]
        nets.append(net)

    return nets


def report_pd_net(design, net, alpha):
    print("Net", net[0][0], flush=True)
    drvr_index = net[0][1]
    pins = net[1]
    xs = [p[1] for p in pins]
    ys = [p[2] for p in pins]
    logger = design.getLogger()
    tree = stt.primDijkstra(xs, ys, drvr_index, alpha, logger)
    stt.reportSteinerTree(tree, xs[drvr_index], ys[drvr_index], logger)
