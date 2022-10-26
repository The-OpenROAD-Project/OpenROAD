import utl

# Definitions
DEFAULT_TARGET_DENSITY = 0.7


def is_pos_int(x):
    if x == None:
        return False
    elif isinstance(x, int) and x > 0 :
        return True
    else:
        utl.error(utl.GPL, 513, f"TypeError: {x} is not a postive integer")
    return False


def is_pos_float(x):
    if x == None:
        return False
    elif isinstance(x, float) and x > 0.0:
        return True
    else:
        utl.error(utl.GPL, 514, f"TypeError: {x} is not a positive float")
    return False


# no positional args here
def global_placement(design, *,
    skip_initial_place=False,
    skip_nesterov_place=False,
    timing_driven=False,
    routability_driven=False,
    incremental=False,
    force_cpu=False,
    skip_io=False,
    bin_grid_count=None,       # positive int
    density=None,              # target_density: 'uniform' or 0<d<1
    init_density_penalty=None, # positive float
    init_wirelength_coef=None, # positive float
    min_phi_coef=None,         # positive float
    max_phi_coef=None,         # positive float
    reference_hpwl=None,       # positive int
    overflow=None,             # overflow
    initial_place_max_iter=None,     # int > 0
    initial_place_max_fanout=None,   # initial_place_max_fanout
    routability_check_overflow=None, # routability_check_overflow
    routability_max_density=None,    # positive float
    routability_max_bloat_iter=None, # int
    routability_max_inflation_iter=None,      # int
    routability_target_rc_metric=None,        # routability_target_rc_metric
    routability_inflation_ratio_coef=None,    # routability_inflation_ratio_coef
    routability_max_inflation_ratio=None,     # routability_max_inflation_ratio
    routability_rc_coefficients=None,         # a list of four floats
    timing_driven_net_reweight_overflow=None, # list of ints
    timing_driven_net_weight_max=None,        # timing_driven_net_weight_max
    timing_driven_nets_percentage=None,       # timing_driven_nets_percentage
    pad_left=None, # int
    pad_right=None # int
):
    gpl = design.getReplace()

    if skip_initial_place:
          gpl.setInitialPlaceMaxIter(0)
    elif initial_place_max_iter != None:
        if is_pos_int(init_place_max_iter):
            gpl.setInitialPlaceMaxIter(init_place_max_iter)
        else:
            utl.error(utl.GPL, 501, "Bad init_place_max_iter (int, >0)")

    gpl.setForceCPU(force_cpu)

    gpl.setSkipIoMode(skip_io)
    if skip_io:
        gpl.setInitialPlaceMaxIter(0) 

    gpl.setTimingDrivenMode(timing_driven)

    if timing_driven:
        if design.evalTclString('get_libs -quiet "*"') == '':
            utl.error(utl.GPL, 502, "No liberty libraries found.")

        if skip_io:
            utl.error(utl.GPL, 503, "-skip_io will disable timing driven mode.")
            gpl.setTimingDrivenMode(False)

        if timing_driven_net_reweight_overflow != None:
            overflow_list = timing_driven_net_reweight_overflow
        else:
            overflow_list = [79, 64, 49, 29, 21, 15]
            
        for ov in overflow_list:
            gpl.addTimingNetWeightOverflow(ov)

        if timing_driven_net_weight_max != None:
            gpl.setTimingNetWeightMax(timing_driven_net_weight_max)

        if timing_driven_nets_percentage != None:
            design.evalTclString("rsz::set_worst_slack_nets_percent $timing_driven_nets_percentage")

    gpl.setRoutabilityDrivenMode(routability_driven)

    if routability_driven:
        utl.warn(utl.GPL, 504, "-skip_io will disable routability driven mode.")
        gpl.setRoutabilityDrivenMode(False)

    if is_pos_int(initial_place_max_fanout):
        gpl.setInitialPlaceMaxFanout(initial_place_max_fanout)
    elif initial_place_max_fanout != None:
        utl.error(utl.GPL, 505, "initial_place_max_fanout must be a positive integer")

    # Magic numbers
    uniform_mode = False

    if density != None:
        target_density = density
    else: 
        target_density = DEFAULT_TARGET_DENSITY

    if target_density == "uniform":
        uniform_mode = True
    elif is_pos_float(target_density):
        gpl.setTargetDensity(target_density)

    gpl.setUniformTargetDensityMode(uniform_mode)

    if is_pos_float(routability_max_density):
        gpl.setRoutabilityMaxDensity(routability_max_density)

    if is_pos_float(min_phi_coef):
        gpl.setMinPhiCoef(min_phi_coef)

    if is_pos_float(max_phi_coef):
        gpl.setMaxPhiCoef(max_phi_coef)

    if is_pos_float(init_density_penalty):
        gpl.setInitDensityPenalityFactor(init_density_penalty)

    if is_pos_float(init_wirelength_coef):
        gpl.setInitWireLengthCoef(init_wirelength_coef)

    if is_pos_float(reference_hpwl):
        gpl.setReferenceHpwl(reference_hpwl)

    if is_pos_int(bin_grid_count):
        gpl.setBinGridCntX(bin_grid_count)
        gpl.setBinGridCntY(bin_grid_count)

    if is_pos_float(overflow):
        gpl.setTargetOverflow(overflow)

    if is_pos_float(routability_check_overflow):
        gpl.setRoutabilityCheckOverflow(routability_check_overflow)

    if is_pos_float(routability_max_bloat_iter):
        gpl.setRoutabilityMaxBloatIter(routability_max_bloat_iter)

    if is_pos_float(routability_max_inflation_iter):
        gpl.setRoutabilityMaxInflationIter(routability_max_inflation_iter)

    if is_pos_float(routability_target_rc_metric):
        gpl.setRoutabilityTargetRcMetric(routability_target_rc_metric)

    if is_pos_float(routability_inflation_ratio_coef):
        gpl.setRoutabilityInflationRatioCoef(routability_inflation_ratio_coef)

    if is_pos_float(routability_max_inflation_ratio):
        gpl.setRoutabilityMaxInflationRatio(routability_max_inflation_ratio)

    if routability_rc_coefficients != None:
        k1, k2, k3, k4 = routability_rc_coefficients
        gpl.setRoutabilityRcCoefficients(k1, k2, k3, k4)

    # Now here is a section that is marked "temp code" in the tcl file

    if is_pos_int(pad_left):
        gpl.setPadLeft(pad_left)

    if is_pos_int(pad_right):
        gpl.setPadRight(pad_right)

    if len(design.getBlock().getRows()) > 0:  # db_has_rows
        if incremental:
            gpl.doIncrementalPlace()
        else:
            gpl.doInitialPlace()
            if not skip_nesterov_place:
                gpl.doNesterovPlace()

        gpl.reset()
        gpl.setDb(design.getTech().getDB())
        gpl.setLogger(design.getLogger())
        gpl.setGlobalRouter(design.getGlobalRouter())
        # no idea if this will work (actually, it doesn't)
        #rz = design.evalTclString("getOpenRoad()->getResizer()")
        #gpl.setResizer(rz)
    else:
        utl.error(utl.GPL, 506, "No rows defined in design. Use initialize_floorplan to add rows.")
