###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2022, The Regents of the University of California
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################
import utl
import odb
import re


def place_pins(
    design,
    *,
    hor_layers=None,
    ver_layers=None,
    random_seed=None,
    random=False,
    corner_avoidance=None,
    min_distance=None,
    min_distance_in_tracks=False,
    exclude=None,
    group_pins=None,
):
    """Perform pin placement

    keyword arguments:
    hor_layers       --
    ver_layers       --
    random_seed      --
    random           --
    corner_avoidance --
    min_distance     --
    min_distance_in_tracks --
    exclude          --
    group_pins       -- A list of strings
    """
    dbTech = design.getTech().getDB().getTech()
    if dbTech == None:
        utl.error(utl.PPL, 331, "No technology found.")

    dbBlock = design.getBlock()
    if dbBlock == None:
        utl.error(utl.PPL, 332, "No block found.")

    db = design.getTech().getDB()
    blockages = []

    for inst in dbBlock.getInsts():
        if inst.isBlock():
            if not inst.isPlaced():
                utl.warn(utl.PPL, 315, f"Macro {inst.getName()} is not placed.")
            else:
                blockages.append(inst)

    utl.report(f"Found {len(blockages)} macro blocks.")

    seed = 42
    if random_seed != None:
        seed = random_seed

    params = design.getIOPlacer().getParameters()
    params.setRandSeed(seed)

    if hor_layers == None:
        utl.error(utl.PPL, 317, "hor_layers is required.")

    if ver_layers == None:
        utl.error(utl.PPL, 318, "ver_layers is required.")

    # set default interval_length from boundaries as 1u
    distance = 1
    if corner_avoidance != None:
        distance = corner_avoidance
        params.setCornerAvoidance(design.micronToDBU(distance))

    min_dist = 2
    if min_distance != None:
        min_dist = min_distance
        if min_distance_in_tracks:
            params.setMinDistance(min_dist)
        else:
            params.setMinDistance(design.micronToDBU(min_dist))
    else:
        utl.report(f"Using {min_dist} tracks default min distance between IO pins.")
        # setting min distance as 0u leads to the default min distance
        params.setMinDistance(0)

    # Confusing logic, but this just sets a boolean to interpret the value already
    # set as either a track count or dbu count
    params.setMinDistanceInTracks(min_distance_in_tracks)

    bterms_cnt = len(dbBlock.getBTerms())

    if bterms_cnt == 0:
        utl.error(utl.PPL, 319, "Design without pins.")

    num_tracks_y = 0
    for hor_layer_name in hor_layers.split():
        hor_layer = parse_layer_name(design, hor_layer_name)
        if not db_layer_has_hor_tracks(design, hor_layer):
            utl.error(
                utl.PPL,
                321,
                f"Horizontal routing tracks not found for layer {hor_layer_name}.",
            )

        if hor_layer.getDirection() != "HORIZONTAL":
            utl.error(
                utl.PPL,
                345,
                f"Layer {hor_layer_name} preferred direction is not horizontal.",
            )

        hor_track_grid = dbBlock.findTrackGrid(hor_layer)

        num_tracks_y = num_tracks_y + len(hor_track_grid.getGridY())
        design.getIOPlacer().addHorLayer(hor_layer)

    num_tracks_x = 0
    for ver_layer_name in ver_layers.split():
        ver_layer = parse_layer_name(design, ver_layer_name)
        if not db_layer_has_ver_tracks(design, ver_layer):
            utl.error(
                utl.PPL,
                323,
                f"Vertical routing tracks not found for layer {ver_layer_name}.",
            )

        if ver_layer.getDirection() != "VERTICAL":
            utl.error(
                utl.PPL,
                346,
                f"Layer {ver_layer_name} preferred direction is not vertical.",
            )

        ver_track_grid = dbBlock.findTrackGrid(ver_layer)

        num_tracks_x = num_tracks_x + len(ver_track_grid.getGridX())
        design.getIOPlacer().addVerLayer(ver_layer)

    num_slots = (2 * num_tracks_x + 2 * num_tracks_y) / min_dist

    if bterms_cnt > num_slots:
        utl.error(
            utl.PPL,
            324,
            f"Number of pins {bterms_cnt} exceeds max possible {num_slots}.",
        )

    if exclude != None:
        lef_units = dbTech.getLefUnits()
        for region in exclude:
            edge, interval = region.split(":")
            if not (edge in ["top", "bottom", "left", "right"]):
                utl.error(
                    utl.PPL,
                    326,
                    f"exclude: invalid syntax in {region}. Use (top|bottom|left|right):interval.",
                )
            edge_ = parse_edge(design, edge)

            if len(interval.split("-")) > 1:
                begin, end = interval.split("-")
                if begin == "*":
                    begin = get_edge_extreme(design, True, edge)
                else:
                    begin = int(begin)
                if end == "*":
                    end = get_edge_extreme(design, False, edge)
                else:
                    end = int(end)

                begin = begin * lef_units
                end = end * lef_units
                design.getIOPlacer().excludeInterval(edge_, begin, end)

            elif interval == "*":
                begin = get_edge_extreme(design, True, edge)
                end = get_edge_extreme(design, False, edge)
                design.getIOPlacer().excludeInterval(edge_, begin, end)

            else:
                utl.error(utl.PPL, 325, f"-exclude: {interval} is an invalid region.")

    if group_pins != None:
        group_idx = 0
        for group in group_pins:
            names = group.split()
            # suppress info printing for regression tests
            # utl.info(utl.PPL, 441, f"Pin group {group_idx}: [{group}]")
            pin_list = []
            for pin_name in names:
                db_bterm = dbBlock.findBTerm(pin_name)
                if db_bterm != None:
                    pin_list.append(db_bterm)
                else:
                    utl.warn(
                        utl.PPL, 343, f"Pin {pin_name} not found in group {group_idx}."
                    )

            dbBlock.addBTermGroup(pin_list, False)
            group_idx += 1

    design.getIOPlacer().runHungarianMatching(random)


def place_pin(
    design,
    pin_name=None,
    layer=None,
    location=None,
    pin_size=None,
    force_to_die_boundary=False,
    placed_status=False,
):
    x = design.micronToDBU(location[0])
    y = design.micronToDBU(location[1])
    width = design.micronToDBU(pin_size[0])
    height = design.micronToDBU(pin_size[1])
    pin = parse_pin_names(design, pin_name)
    lay = parse_layer_name(design, layer)
    design.getIOPlacer().placePin(
        pin[0], lay, x, y, width, height, force_to_die_boundary, placed_status
    )


def parse_layer_name(design, layer_name):
    mt = design.getTech().getDB().getTech()
    if mt == None:
        utl.error(utl.PPL, 350, "No technology has been read.")
    tech_layer = mt.findLayer(layer_name)
    if tech_layer == None:
        utl.error(utl.PPL, 351, f"Layer {layer_name} not found.")
    return tech_layer


def db_layer_has_tracks(design, layer, *, horz=False):
    if layer == None:
        return False
    block = design.getBlock()
    trackGrid = block.findTrackGrid(layer)
    if trackGrid == None:
        return False
    if horz:
        return trackGrid.getNumGridPatternsY() > 0
    else:
        return trackGrid.getNumGridPatternsX() > 0


def db_layer_has_hor_tracks(design, layer):
    return db_layer_has_tracks(design, layer, horz=True)


def db_layer_has_ver_tracks(design, layer):
    return db_layer_has_tracks(design, layer, horz=False)


def parse_edge(design, edge):
    if not (edge in ["top", "bottom", "left", "right"]):
        utl.error(
            utl.PPL, 327, f"{edge} is an invalid edge. Use top, bottom, left or right."
        )
    return design.getIOPlacer().getEdge(edge)


def get_edge_extreme(design, begin, edge):
    dbBlock = design.getBlock()
    die_area = dbBlock.getDieArea()
    if begin:
        if edge == "top" or edge == "bottom":
            extreme = die_area.xMin()
        elif edge == "left" or edge == "right":
            extreme = die_area.yMin()
        else:
            utl.error(
                utl.PPL,
                329,
                "Invalid edge {edge}. Should be one of top, bottom, left, right.",
            )
    else:
        if edge == "top" or edge == "bottom":
            extreme = die_area.xMax()
        elif edge == "left" or edge == "right":
            extreme = die_area.yMax()
        else:
            utl.error(
                utl.PPL,
                330,
                "Invalid edge {edge}. Should be one of top, bottom, left, right.",
            )

    return extreme


def clear_io_pin_constraints(design):
    design.getIOPlacer().clearConstraints()


def set_io_pin_constraint(
    design,
    *,
    direction=None,
    pin_names=None,  # single string list of pins
    region=None,
    group=False,
    order=False,
):
    """Set the region constraints for pins according to the pin direction or the pin name

    keyword arguments:
    direction -- "input" | "output" | "inout" | "feedthru"
    pin_names -- string list of pins to constrain, can contain regex.
                 Note that we need to escape regex characters for exact matching,
                 ie, use "rqst\\[23\\]" instead of "rqst[23]"
    region    -- region constraint, e.g. "top:*" or "left:1.2-3.4"
                 "up" takes an area spec, ie "up:10 10 300 300" or specify
                 entire area with "up:*"
    group     -- places together on the die boundary the pin list defined
                 in pin_names
    order     -- places the pin group ordered in ascending x/y position
    """
    dbTech = design.getTech().getDB().getTech()
    dbBlock = design.getBlock()
    lef_units = dbTech.getLefUnits()
    edge = None
    interval = None
    if region != None:
        edge, interval = region.split(":")
    else:
        region = ""

    if edge in ["top", "bottom", "left", "right"]:
        if len(interval.split("-")) > 1:
            begin, end = interval.split("-")
            if begin == "*":
                begin = get_edge_extreme(design, True, edge)
            else:
                begin = design.micronsToDBU(begin)

            if end == "*":
                end = get_edge_extreme(design, False, edge)
            else:
                end = design.micronToDBU(end)
        elif interval == "*":
            begin = get_edge_extreme(design, True, edge)
            end = get_edge_extreme(design, False, edge)
        else:
            utl.error(utl.PPL, 399, "Unrecognized region specification")

        if direction != None and pin_names != None:
            utl.error(
                utl.PPL,
                316,
                "Both 'direction' and 'pin_names' constraints not allowed.",
            )

        constraint_region = dbBlock.findConstraintRegion(edge, begin, end)

        if direction != None:
            # utl.info(utl.PPL, 349, f"Restrict {direction} pins to region " +
            #          f"{design.micronToDBU(begin)}-{design.micronToDBU(end)}, " +
            #          f"in the {edge} edge.")
            dbBlock.addBTermConstraintByDirection(direction, constraint_region)

        if pin_names != None:
            pin_list = parse_pin_names(design, pin_names)
            dbBlock.addBTermsToConstraint(pin_list, constraint_region)

    elif bool(re.fullmatch("(up):(.*)", region)):
        # no walrus in < 3.8
        mo = re.fullmatch("(up):(.*)", region)
        if mo.group(2) == "*":
            top_grid_region = dbBlock.getBTermTopLayerGridRegion()
            if top_grid_region.isRect():
                region_rect = top_grid_region.getEnclosingRect()
                llx = region_rect.xMin()
                lly = region_rect.yMin()
                urx = region_rect.xMax()
                ury = region_rect.yMax()

        elif len(mo.group(2).split()) == 4:
            llx, lly, urx, ury = mo.group(2).split()
            llx = design.micronToDBU(float(llx))
            lly = design.micronToDBU(float(lly))
            urx = design.micronToDBU(float(urx))
            ury = design.micronToDBU(float(ury))
        else:
            utl.error(
                utl.PPL, 359, 'Box at top layer must have 4 values "llx lly urx ury".'
            )

        if pin_names != None:
            pin_list = parse_pin_names(design, pin_names)
            # utl.info(utl.PPL, 399, "Restrict pins to ... on layer ...")
            rect = odb.Rect(llx, lly, urx, ury)
            dbBlock.addBTermsToConstraint(pin_list, rect)

    elif group:
        if pin_names != None:
            names = pin_names.split()
            pin_list = []
            for pin_name in names:
                db_bterm = dbBlock.findBTerm(pin_name)
                if db_bterm != None:
                    pin_list.append(db_bterm)
                else:
                    utl.warn(
                        utl.PPL, 500, f"Group pin {pin_name} not found in the design."
                    )

            dbBlock.addBTermGroup(pin_list, order)

    else:
        utl.warn(utl.PPL, 373, f"Constraint with region {region} has an invalid edge.")


def is_pos_float(x):
    if x == None:
        return False
    elif isinstance(x, float) and x > 0.0:
        return True
    else:
        utl.error(utl.PPL, 378, f"TypeError: {x} is not a positive float")
    return False


# This does support regex in port names, but they are Python regex's not tcl regex's
# so that, for example, we must use ".*" instead of "*" and "req_msg\[15\]" instead
# of "req_msg[15]". Always returns a list, even for a single name.
def parse_pin_names(design, names):
    if bool(re.findall(" ", names)):
        cre = re.compile("|".join(names.split()))
    else:
        cre = re.compile(names)

    bterms = design.getBlock().getBTerms()
    pin_list = []
    for bterm in bterms:
        if bool(cre.match(bterm.getName())):
            pin_list.append(bterm)
    return pin_list


def define_pin_shape_pattern(
    design,
    layer_name=None,
    x_step=None,
    y_step=None,
    region=None,
    size=None,
    pin_keepout=None,
):
    """Defines a pin placement grid on the specified layer

    keyword arguments:
    layer_name  -- Defines a single top-most routing layer
    x_step      -- Define the distance (in microns) between each valid position
                   on the grid, in the x direction
    y_step      -- Ditto for y direction
    region      -- Defines the "llx lly urx ury" region of the placement grid
                   in microns
    pin_keepout -- defines the boundary (in microns) around existing routing
                   obstructions that the pins should avoid. This defaults to the
                   layer minimum spacing.
    """
    if layer_name != None:
        layer = parse_layer_name(design, layer_name)
        if layer == None:
            utl.error(utl.PPL, 352, f"Routing layer not found for name {layer_name}.")
    else:
        utl.error(utl.PPL, 353, "layer_name is required.")

    if x_step != None and y_step != None:
        x_step = design.micronToDBU(x_step)
        y_step = design.micronToDBU(y_step)
    else:
        utl.error(utl.PPL, 354, "x_step and y_step are required.")

    if region != None:
        if region == "*":
            dbBlock = design.getBlock()
            die_area = dbBlock.getDieArea()
            llx = die_area.xMin()
            lly = die_area.yMin()
            urx = die_area.xMax()
            ury = die_area.yMax()

        elif len(region.split()) == 4:
            (
                llx,
                lly,
                urx,
                ury,
            ) = region.split()
            llx = design.micronToDBU(float(llx))
            lly = design.micronToDBU(float(lly))
            urx = design.micronToDBU(float(urx))
            ury = design.micronToDBU(float(ury))

        else:
            utl.error(
                utl.PPL, 363, f"region should be a list of 4 values. It is {region}."
            )

        rect = odb.Rect(llx, lly, urx, ury)
    else:
        utl.error(utl.PPL, 355, "region is required.")

    if size != None:
        if len(size) != 2:
            utl.error(utl.PPL, 356, f"size should be list of 2 values. It is {size}")

        width = design.micronToDBU(size[0])
        height = design.micronToDBU(size[1])
    else:
        utl.error(utl.PPL, 357, "size is required.")

    if is_pos_float(pin_keepout):
        keepout = design.micronToDBU(pin_keepout)
    else:
        max_dim = max(width, height)
        keepout = (
            design.getTech().getDB().getTech().findLayer(layer_name).getSpacing(max_dim)
        )

    dbBlock = design.getBlock()
    odb.set_bterm_top_layer_grid(
        dbBlock, layer, x_step, y_step, rect, width, height, keepout
    )
