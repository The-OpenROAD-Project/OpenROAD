import utl
from enum import Enum


def get_layer_idx(design, layer_name):
    """Given a layer name and a technology, return the layer index"""
    mt = design.getTech().getDB().getTech()
    if mt == None:
        utl.error(utl.GRT, 500, "No technology has been read.")
    tlayer = mt.findLayer(layer_name)
    if tlayer == None:
        utl.error(utl.GRT, 501, f"Layer {layer_name} not found.")
    idx = tlayer.getRoutingLevel()
    return idx


def get_layer_range(design, layer_range):
    l1, l2 = layer_range.split("-")
    idx1 = get_layer_idx(design, l1)
    idx2 = get_layer_idx(design, l2)
    if idx1 > idx2:
        utl.error(utl.GRT, 502, "Input format to define layer range is min-max.")
    return idx1, idx2


# This also assumes that routing layer indicies are contiguous
def check_routing_layer_by_index(design, layer_idx):
    """Check that the layer index 'layer_idx' is in bounds of the defined layers"""
    rtech = design.getTech().getDB().getTech()
    if rtech == None:
        utl.error(utl.GRT, 503, "No technology has been read.")
    if layer_idx < 0:
        utl.error(utl.GRT, 504, "Layer index must be non negative.")

    # this is actually the max index, where they start at 0, not the total
    # number of routing layers
    max_routing_layer = rtech.getRoutingLayerCount()
    tech_layer = rtech.findRoutingLayer(layer_idx)

    # NOTE: the tcl version uses 1 instead of 0, which is incorrect.
    min_tech_layer = rtech.findRoutingLayer(0)
    max_tech_layer = rtech.findRoutingLayer(max_routing_layer)

    if layer_idx > max_routing_layer:
        utl.error(
            utl.GRT,
            505,
            f"Layer {tech_layer.getConstName()} is greater "
            f"than the max routing layer "
            f"({max_tech_layer.getConstName()}).",
        )

    if layer_idx < 0:
        utl.error(
            utl.GRT,
            506,
            f"Layer {tech_layer.getConstName()} is less "
            f"than the min routing layer "
            f"({min_tech_layer.getConstName()}).",
        )


def set_global_routing_layer_adjustment(design, layer, adj):
    """In design "design", set the global layer adjustment for "layer".
    layer can be a named layer or range specified as layer1-layer2,
    where layer1 < layer2. The adjustment "adj" must be > 0.0"""
    gr = design.getGlobalRouter()

    if adj <= 0.0:
        utl.error(utl.GRT, 507, "Layer adjustment must be positive")

    if layer == "*":
        gr.setAdjustment(adj)

    elif layer.rfind("-") > 0:
        first_layer, last_layer = get_layer_range(design, layer)
        for l in range(first_layer, last_layer + 1):
            check_routing_layer_by_index(design, l)
            gr.addLayerAdjustment(l, adj)

    else:
        layer_idx = get_layer_idx(design, layer)
        check_routing_layer_by_index(design, layer_idx)
        gr.addLayerAdjustment(layer_idx, adj)


class Orient(Enum):
    HORZ = 0
    VERT = 1


def layer_has_tracks(design, layer, *, orient=Orient.HORZ):
    if layer == None:
        return False

    if design.getBlock() == None:
        utl.error(utl.GRT, 991, "Design has no block")

    trackGrid = design.getBlock().findTrackGrid(layer)

    if trackGrid == None:
        return False

    if orient == Orient.HORZ:
        return trackGrid.getNumGridPatternsY() > 0
    elif orient == Orient.VERT:
        return trackGrid.getNumGridPatternsX() > 0
    else:
        utl.error(utl.GRT, 508, f"Unknown routing orientation: {orient}")


def define_layer_range(design, layers):
    lmin, lmax = get_layer_range(design, layers)
    check_routing_layer_by_index(design, lmin)
    check_routing_layer_by_index(design, lmax)

    design.getGlobalRouter().setMinRoutingLayer(lmin)
    design.getGlobalRouter().setMaxRoutingLayer(lmax)

    rtech = design.getTech().getDB().getTech()
    for layer in range(lmin, lmax + 1):
        db_layer = rtech.findRoutingLayer(layer)
        if not layer_has_tracks(
            design, db_layer, orient=Orient.HORZ
        ) and not layer_has_tracks(design, db_layer, orient=Orient.VERT):
            layer_name = db_layer.getName()
            utl.error(utl.GRT, 509, f"Missing track structure for layer {layer_name}.")


def define_clock_layer_range(design, layers):
    mincl, maxcl = get_layer_range(design, layers)
    check_routing_layer_by_index(design, mincl)
    check_routing_layer_by_index(design, maxcl)

    if mincl < maxcl:
        design.getGlobalRouter().setMinLayerForClock(mincl)
        design.getGlobalRouter().setMaxLayerForClock(maxcl)
    else:
        utl.error(
            utl.GRT,
            510,
            "In setting clock layers, min routing layer is greater than max routing layer.",
        )


# Force keyword only args
def set_routing_layers(design, *, signal=None, clock=None):
    if signal != None:
        define_layer_range(design, signal)

    if clock != None:
        define_clock_layer_range(design, clock)


def create_ndr(design):
    pass


# The -all_clocks flag is not (yet) supported
def assign_ndr(design, *, ndrName="", netName=""):
    ndr = design.getBlock().findNonDefaultRule(ndrName)
    if ndr == None:
        utl.error(utl.GRT, 511, f"No NDR named {ndrName} found.")

    if netName != "":
        net = design.getBlock().findNet(netName)
        if net == None:
            utl.error(utl.GRT, 512, f"No net named {netName} found.")
        net.setNonDefaultRule(ndr)
    else:
        utl.error(utl.GRT, 513, f"No net name specified for ndr.")
