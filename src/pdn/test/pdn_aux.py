import re
import utl
import pdn
import odb
from collections import defaultdict


#  In tcl land, this lives in OpenRoad.tcl. However, it seems to be only called
#  in the pdn regression tests so it is defined here instead.
def add_global_connection(
    design,
    *,
    net_name=None,
    inst_pattern=None,
    pin_pattern=None,
    power=False,
    ground=False,
    region=None,
):
    if net_name is None:
        utl.error(
            utl.PDN,
            1501,
            "The net option for the " + "add_global_connection command is required.",
        )

    if inst_pattern is None:
        inst_pattern = ".*"

    if pin_pattern is None:
        utl.error(
            utl.PDN,
            1502,
            "The pin_pattern option for the "
            + "add_global_connection command is required.",
        )

    net = design.getBlock().findNet(net_name)
    if net is None:
        net = odb.dbNet_create(design.getBlock(), net_name)

    if power and ground:
        utl.error(utl.PDN, 1551, "Only power or ground can be specified")
    elif power:
        net.setSpecial()
        net.setSigType("POWER")
    elif ground:
        net.setSpecial()
        net.setSigType("GROUND")

    # region = None
    if region is not None:
        region = design.getBlock().findRegion(region)
        if region is None:
            utl.error(utl.PDN, 1504, f"Region {region} not defined")

    design.getBlock().addGlobalConnect(region, inst_pattern, pin_pattern, net, True)


def check_design_state(design, cmd):
    if not bool(design.getBlock()):
        utl.error(utl.PDN, 1599, f"Design must be loaded before calling {cmd}.")


def set_voltage_domain(
    design,
    *,
    name=None,
    power=None,
    ground=None,
    region_name=None,
    secondary_power=None,
    switched_power_name=None,
):
    pdngen = design.getPdnGen()
    check_design_state(design, "set_voltage_domain")
    if design.getBlock() is None:
        utl.error(
            utl.PDN,
            1505,
            "Design must be loaded before calling " + "set_voltage_domain.",
        )

    if power is None:
        utl.error(utl.PDN, 1506, "The power argument is required.")
    else:
        pwr = design.getBlock().findNet(power)
        if pwr is None:
            utl.error(utl.PDN, 1507, f"Unable to find power net: {power}")

    if ground is None:
        utl.error(utl.PDN, 1508, "The ground argument is required.")
    else:
        gnd = design.getBlock().findNet(ground)
        if gnd is None:
            utl.error(utl.PDN, 1509, f"Unable to find ground net: {ground}")

    region = None
    if region_name is not None:
        region = design.getBlock().findRegion(region_name)
        if region is None:
            utl.error(utl.PDN, 1510, f"Unable to find region: {region}")

        if bool(name):
            name = name.capitalize() if name == "CORE" else name
        else:
            name = region.getName()

    secondary = []
    if secondary_power is not None:
        for snet in secondary_power.split():
            db_net = design.getBlock().findNet(snet)
            if db_net is None:
                utl.error(utl.PDN, 1511, f"Unable to find secondary power net: {snet}")
            else:
                secondary.append(db_net)

    switched_power = None
    if switched_power_name is not None:
        switched_power_net_name = switched_power_name
        switched_power = design.getBlock().findNet(switched_power_net_name)
        if switched_power is None:
            switched_power = odb.dbNet_create(
                design.getBlock(), switched_power_net_name
            )
            switched_power.setSpecial()
            switched_power.setSigType("POWER")
        else:
            if switched_power.getSigType() != "POWER":
                utl.error(
                    utl.PDN,
                    1512,
                    f"Net {switched_power_net_name} "
                    + "already exists in the design, but is of signal "
                    + "type {switched_power.getSigType()}.",
                )

    if region is None:
        if bool(name) and name.capitalize() != "Core":
            utl.warn(utl.PDN, 1513, 'Core voltage domain will be named "Core".')
        pdngen.setCoreDomain(pwr, switched_power, gnd, secondary)

    else:
        pdngen.makeRegionVoltageDomain(
            name, pwr, switched_power, gnd, secondary, region
        )


def get_voltage_domains(design, names):
    pdngen = design.getPdnGen()
    domains = []
    for name in names:
        domain = pdngen.findDomain(name)
        if domain is None:
            utl.error(utl.PDN, 1514, f"Unable to find {name} domain.")
        domains.append(domain)
    return domains


def get_layer(design, name):
    layer = design.getTech().getDB().getTech().findLayer(name)
    if layer is None:
        utl.error(utl.PDN, 1518, f"Unable to find '{name}' layer.")
    else:
        return layer


def has_grid(design, name):
    return len(design.getPdnGen().findGrid(name)) > 0


def get_obstructions(design, obstructions):
    if obstructions == []:
        return []
    layers = []
    for l in obstructions:
        layers.append(get_layer(design, l))
    return layers


def define_pdn_grid_real(
    design,
    *,
    name=None,
    pins=[],
    obstructions=[],
    power_control=None,
    voltage_domains=None,
    power_switch_cell=None,
    starts_with_power=False,
    grid_over_pg_pins=False,
    grid_over_boundary=False,
    power_control_network="STAR",
):  # (STAR|DAISY)]}
    pdngen = design.getPdnGen()
    if bool(voltage_domains):
        domains = get_voltage_domains(design, voltage_domains)
    else:
        domains = [pdngen.findDomain("Core")]

    if name is None:
        utl.error(
            utl.PDN, 1516, "'name' is a required parameter for define_pdn_grid_real"
        )

    if has_grid(design, name):
        utl.error(utl.PDN, 1517, f"Grid named {name} already defined.")

    pin_layers = []
    if bool(pins):
        for pin in pins:
            pin_layers.append(get_layer(design, pin))

    if bool(obstructions):
        obstructions = get_obstructions(design, obstructions)

    power_cell = None
    if power_switch_cell is not None:
        power_cell = pdngen.findSwitchedPowerCell(power_switch_cell)
        if power_cell is None:
            utl.error(
                utl.PDN,
                1519,
                f"Switched power cell {power_switch_cell} is not defined.",
            )

        if not bool(power_control):
            utl.error(
                utl.PDN,
                1520,
                "'power_control' must be specified with 'power_switch_cell'",
            )
        else:
            power_control = design.getBlock().findNet(power_control)
            if power_control is None:
                utl.error(
                    utl.PDN, 1521, f"Unable to find power control net: {power_control}"
                )

    starts_with = pdn.POWER if starts_with_power else pdn.GROUND
    for domain in domains:
        pdngen.makeCoreGrid(
            domain,
            name,
            starts_with,
            pin_layers,
            obstructions,
            power_cell,
            power_control,
            power_control_network,
        )


def define_pdn_grid_macro(
    design,
    *,
    name="",
    voltage_domains=None,
    orient=[],
    instances=[],  # no regex's supported in this version
    cells=None,
    halo=[0, 0, 0, 0],
    starts_with_power=False,  # only POWER or GROUND, no GRID
    obstructions=[],  # string list of layers
    grid_over_pg_pins=False,
    grid_over_boundary=False,
    default_grid=False,
    is_bump=False,
):
    pdngen = design.getPdnGen()
    pg_pins_to_boundary = True
    if grid_over_pg_pins and grid_over_boundary:
        utl.error(
            utl.PDN,
            1522,
            "Options 'grid_over_pg_pins' and 'grid_over_boundary' "
            + "are mutually exclusive.",
        )
    elif grid_over_pg_pins:
        pg_pins_to_boundary = False

    exclusive_keys = 0
    if bool(instances):
        exclusive_keys += 1

    if bool(cells):
        exclusive_keys += 1

    if default_grid:
        exclusive_keys += 1

    if exclusive_keys > 1:
        utl.error(
            utl.PDN,
            1523,
            "Options 'instances', 'cells', and 'default_grid' are mutually exclusive.",
        )
    elif exclusive_keys < 1:
        utl.error(
            utl.PDN,
            1524,
            "One of either 'instances', 'cells', or 'default_grid' must be specified.",
        )

    if default_grid:
        # set default pattern to .*
        cells = ".*"

    if name == "":
        utl.error(
            utl.PDN, 1524, "'name' is a required parameter for define_pdn_grid_macro"
        )

    if has_grid(design, name):
        utl.error(utl.PDN, 1525, f"Grid named {name} already defined.")

    halo = [design.micronToDBU(h) for h in halo]

    if bool(voltage_domains):
        domains = get_voltage_domains(design, voltage_domains)
    else:
        domains = [pdngen.findDomain("Core")]

    obst_list = get_obstructions(design, obstructions)
    orient_list = get_orientations(orient)

    starts_with = pdn.POWER if starts_with_power else pdn.GROUND

    if bool(instances):
        insts = []
        for inst_name in instances:
            inst = design.getBlock().findInst(inst_name)
            if bool(inst):
                insts.append(inst)
            else:
                utl.error(utl.PDN, 1526, f"Unable to find instance: {inst_name}")

        for inst in insts:
            # must match orientation, if provided
            if not bool(orient_list) or inst.getOrient() in orient_list:
                for domain in domains:
                    pdngen.makeInstanceGrid(
                        domain,
                        name,
                        starts_with,
                        inst,
                        halo,
                        pg_pins_to_boundary,
                        default_grid,
                        obst_list,
                        is_bump,
                    )

    else:
        cells = []
        for cell_pattern in cells:
            for cell in get_masters(design, cell_pattern):
                # only add blocks
                if cell.isBlock():
                    cells.append(cell)

        cells = list(set(cells)).sort(key=lambda x: x.getName())
        for cell in cells:
            for inst in design.getBlock.getInsts():
                # inst must match cells
                if inst.getMaster() == cell:
                    # must match orientation, if provided
                    if not bool(orient_list) or inst.getOrient() in orient_list:
                        for domain in domains:
                            pdn.make_instance_grid(
                                domain,
                                name,
                                starts_with,
                                inst,
                                halo,
                                pg_pins_to_boundary,
                                default_grid,
                                obst_list,
                                is_bump,
                            )


def add_pdn_stripe(
    design,
    *,
    grid="",
    layer=None,
    width=0,
    pitch=0,
    spacing=0,
    offset=0,
    starts_with=None,
    number_of_straps=0,
    nets=None,
    followpins=False,
    extend_to_core_ring=False,
    extend_to_boundary=False,
    snap_to_grid=False,
):
    pdngen = design.getPdnGen()

    if layer is None:
        utl.error(utl.PDN, 1527, "The 'layer' argument is required.")

    if not followpins:
        if width == 0:
            utl.error(
                utl.PDN,
                1528,
                "The 'width' argument is required when followpins is false.",
            )

        if pitch == 0:
            utl.error(
                utl.PDN,
                1529,
                "The 'pitch' argument is required when followpins is false.",
            )
    nets_list = []
    if bool(nets):
        for net_name in nets:
            net = design.getBlock.findNet(net_name)
            if net == None:
                utl.error(utl.PDN, 1530, f"Unable to find net {net_name}.")
            nets_list.append(net)

    layer = get_layer(design, layer)
    width = design.micronToDBU(width)
    pitch = design.micronToDBU(pitch)
    spacing = design.micronToDBU(spacing)
    offset = design.micronToDBU(offset)

    extend = pdn.CORE
    if extend_to_core_ring and extend_to_boundary:
        utl.error(
            utl.PDN,
            1531,
            "Options 'extend_to_core_ring' and "
            + "'extend_to_boundary' are mutually exclusive.",
        )
    elif extend_to_core_ring:
        extend = pdn.RINGS
    elif extend_to_boundary:
        extend = pdn.BOUNDARY

    if followpins:
        for g in pdngen.findGrid(grid):
            pdngen.makeFollowpin(g, layer, width, extend)

    else:
        if not bool(starts_with):
            starts_with = pdn.GRID
        elif starts_with.upper() == "POWER":
            starts_with = pdn.POWER
        elif starts_with.upper() == "GROUND":
            starts_with = pdn.GROUND
        else:
            utl.error(utl.PDN, 607, "Invalid starts_with. Must be POWER or GROUND")

        for g in pdngen.findGrid(grid):
            pdngen.makeStrap(
                g,
                layer,
                width,
                spacing,
                pitch,
                offset,
                number_of_straps,
                snap_to_grid,
                starts_with,
                extend,
                nets_list,
            )


valid_orientations = ["R0", "R90", "R180", "R270", "MX", "MY", "MXR90", "MYR90"]
lef_orientations = {
    "N": "R0",
    "FN": "MY",
    "S": "R180",
    "FS": "MX",
    "E": "R270",
    "FE": "MYR90",
    "W": "R90",
    "FW": "MXR90",
}


def get_orientations(orientations):
    if orientations == []:
        return []
    checked_orientations = []
    for orient in orientations:
        if orient in valid_orientations:
            checked_orientations.append(orient)
        elif orient in lef_orientations:
            checked_orientations.append(lef_orientations[orient])
        else:
            vld = " ".join(valid_orientations)
            utl.error(
                utl.PDN,
                1036,
                f"Invalid orientation {orient} specified, " + f"must be one of {vld}",
            )

    return checked_orientations


def add_pdn_connect(
    design,
    *,
    grid="",
    layers=None,
    cut_pitch=[0, 0],
    fixed_vias=[],
    max_rows=0,
    max_columns=0,
    ongrid=[],  # list of layer names that should be on grid?
    split_cuts={},  # dictionary of layer name to pitch
    dont_use_vias="",
):
    pdngen = design.getPdnGen()
    check_design_state(design, "add_pdn_connect")

    if layers is None:
        utl.error(utl.PDN, 1537, "The 'layers' argument is required.")
    elif len(layers) != 2:
        utl.error(utl.PDN, 1538, "The layers argument must contain two layers.")

    l0 = get_layer(design, layers[0])
    l1 = get_layer(design, layers[1])

    if len(cut_pitch) != 2:
        utl.error(utl.PDN, 1539, "The cut_pitch argument must contain two enties.")

    cut_pitch = [design.micronToDBU(l) for l in cut_pitch]

    fixed_generate_vias = []
    fixed_tech_vias = []
    for via in fixed_vias:
        tech_via = design.getTech().getDB().getTech().findVia(via)
        generate_via = design.getTech().getDB().getTech().findViaGenerateRule(via)
        if tech_via is None and generate_via is None:
            utl.error(utl.PDN, 1540, f"Unable to find via: {via}")
        if bool(tech_via):
            fixed_tech_vias.append(tech_via)
        if bool(generate_via):
            fixed_generate_vias.append(generate_via)

    ongrid_list = [get_layer(l) for l in ongrid]

    split_cuts_layers = [get_layer(l) for l in split_cuts.keys()]
    split_cuts_pitches = [design.micronToDBU(x) for x in split_cuts.values()]
    split_cuts_dict = dict(zip(split_cuts_layers, split_cuts_pitches))

    for g in pdngen.findGrid(grid):
        pdngen.makeConnect(
            g,
            l0,
            l1,
            cut_pitch[0],
            cut_pitch[1],
            fixed_generate_vias,
            fixed_tech_vias,
            max_rows,
            max_columns,
            ongrid_list,
            split_cuts_dict,
            dont_use_vias,
        )


def pdngen_db(design, failed_via_report="", skip_trim=False, dont_add_pins=False):
    pdngen = design.getPdnGen()
    trim = not skip_trim
    add_pins = not dont_add_pins

    pdngen.checkSetup()
    pdngen.buildGrids(trim)
    pdngen.writeToDb(add_pins, failed_via_report)
    pdngen.resetShapes()


def add_pdn_ring(
    design,
    *,
    nets=[],
    layers=[],  # list of size 2
    widths=[],  # list of size 2
    spacings=[],  # list of size 2
    grid="",
    pad_offsets=[],  # array of size 4
    core_offsets=[],  # array of size 4
    connect_to_pad_layers=None,
    starts_with=None,
    add_connect=False,
    extend_to_boundary=False,
    connect_to_pads=False,
):
    pdngen = design.getPdnGen()
    check_design_state(design, "add_pdn_ring")

    if not bool(layers):
        utl.error(utl.PDN, 1541, "The layers argument is required.")

    if len(layers) != 2:
        utl.error(utl.PDN, 1542, "'layers' must be a list of size 2")

    if not bool(widths) or len(widths) != 2:
        utl.error(utl.PDN, 1543, "'widths' is a required list of size two.")
    widths = [design.micronToDBU(w) for w in widths]

    if not bool(spacings) or len(spacings) != 2:
        utl.error(utl.PDN, 1544, "'spacings' is a required list of size two.")
    elif bool(spacings):
        spacings = [design.micronToDBU(s) for s in spacings]

    if bool(core_offsets) and bool(pad_offsets):
        utl.error(
            utl.PDN,
            1545,
            "Only one of 'pad_offsets' or 'core_offsets' can be specified.",
        )
    elif not bool(core_offsets) and not bool(pad_offsets):
        utl.error(
            utl.PDN, 1546, "One of 'pad_offsets' or 'core_offsets' must be specified."
        )

    if bool(core_offsets):
        if len(core_offsets) != 4:
            utl.error(utl.PDN, 1559, "'core_offsets must have a length of 4")
        core_offsets = [design.micronToDBU(o) for o in core_offsets]
    else:
        core_offsets = [0, 0, 0, 0]

    if bool(pad_offsets):
        if len(pad_offset) != 4:
            utl.error(utl.PDN, 1560, "'pad_offsets must have a length of 4")
        pad_offsets = [design.micronToDBU(p) for p in pad_offsets]
    else:
        pad_offsets = [0, 0, 0, 0]

    if not bool(starts_with):
        starts_with = pdn.GRID
    elif starts_with.upper() == "POWER":
        starts_with = pdn.POWER
    elif starts_with.upper() == "GROUND":
        starts_with = pdn.GROUND
    else:
        utl.error(
            utl.PDN, 608, "Invalid starts_with. Must be unspecified or POWER or GROUND"
        )

    if extend_to_boundary and connect_to_pads:
        utl.error(
            utl.PDN,
            1547,
            "Only one of 'pad_offsets' or 'core_offsets' can be specified.",
        )

    nets_list = []
    if bool(nets):
        for net_name in nets:
            net = design.getBlock().findNet(net_name)
            if net == None:
                utl.error(utl.PAD, 1548, f"Unable to find net {net_name}.")
            nets_list.append(net)

    l0 = get_layer(design, layers[0])
    l1 = get_layer(design, layers[1])

    connect_to_pad_layers = []
    if bool(connect_to_pads):
        if not bool(connect_to_pad_layers):
            for layer in design.getTech().getDB().getTech().getLayers():
                if layer.getType() == "ROUTING":
                    connect_to_pad_layers.append(layer)
        else:
            connect_to_pad_layers = [
                get_layer(design, l) for l in connect_to_pad_layers
            ]

    for g in pdngen.findGrid(grid):
        pdngen.makeRing(
            g,
            l0,
            widths[0],
            spacings[0],
            l1,
            widths[1],
            spacings[1],
            starts_with,
            core_offsets,
            pad_offsets,
            extend_to_boundary,
            connect_to_pad_layers,
            nets_list,
        )

    if bool(add_connect):
        add_pdn_connect(design, grid, layers)


def define_power_switch_cell(
    design,
    *,
    name=None,
    control=None,
    acknowledge=None,
    power_switchable=None,
    power=None,
    ground=None,
):
    pdngen = design.getPdnGen()
    check_design_state(design, "define_power_switch_cell")

    if not bool(name):
        utl.error(utl.PDN, 31183, "The 'name' argument is required.")
    else:
        master = design.getTech().getDB().findMaster(name)
        if not bool(master):
            utl.error(
                utl.PDN, 31046, f"Unable to find power switch cell master: {name}"
            )

    if not bool(control):
        utl.error(utl.PDN, 31184, "The 'control' argument is required.")
    else:
        control = master.findMTerm(control)
        if not bool(control):
            utl.error(utl.PDN, 31055, f"Unable to find {control} on {master}")

    if not bool(acknowledge):
        utl.error(utl.PDN, 31199, "The 'acknowledge' argument is required.")
    else:
        acknowledge = master.findMTerm(acknowledge)
        if not bool(acknowledge):
            util.error(utl.PDN, 31056, f"Unable to find {acknowledge} on {master}")

    if not bool(power_switchable):
        utl.error(utl.PDN, 31186, "The 'power_switchable' argument is required.")
    else:
        power_switchable = master.findMTerm(power_switchable)
        if not bool(power_switchable):
            utl.error(utl.PDN, 31057, f"Unable to find {power_switchable} on {master}")

    if not bool(power):
        utl.error(utl.PDN, 31187, "The 'power' argument is required.")
    else:
        power = master.findMTerm(power)
        if not bool(power):
            utl.error(utl.PDN, 31188, f"Unable to find {power} on {master}")

    if not bool(ground):
        utl.error(utl.PDN, 31188, "The 'ground' argument is required.")
    else:
        ground = master.findMTerm(ground)
        if not bool(ground):
            print(utl.PDN, 31188, f"Unable to find {ground} on {master}")

    pdngen.makeSwitchedPowerCell(
        master, control, acknowledge, power_switchable, power, ground
    )


def define_pdn_grid_existing(design, *, name="existing_grid", obstructions=[]):
    pdngen = design.getPdnGen()
    if bool(obstructions):
        obstructions = get_obstructions(obstructions)

    pdngen.makeExistingGrid(name, obstructions)


def repair_pdn_vias(design, *, net=None, all=False):
    pdngen = design.getPdnGen()
    if bool(net) and all:
        utl.error(utl.PDN, 31191, "Cannot use both 'net' and 'all' arguments.")

    if not bool(net) and not all:
        utl.error(utl.PDN, 31192, "Must use either 'net' or 'all' arguments.")

    nets = []
    if bool(net):
        net = design.getBlock().findNet(net)
        if net is None:
            utl.error(utl.PDN, 31190, f"Unable to find net: {net}")
        nets.append(net)

    if all:
        for net in design.getBlock().getNets():
            if net.getSigType() == "POWER" or net.getSigType() == "GROUND":
                nets.append(net)

    pdngen.repairVias(nets)
