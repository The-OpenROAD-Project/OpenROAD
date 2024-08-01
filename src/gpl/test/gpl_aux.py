import utl

# Definitions
DEFAULT_TARGET_DENSITY = 0.7


# Besides design, there are no positional args here. General strategy is that
# when an arg is None, we just skip setting it, otherwise we will set the
# parameter after a quick type check.
def global_placement(
    design,
    *,
    skip_initial_place=False,
    skip_nesterov_place=False,
    timing_driven=False,
    routability_driven=False,
    incremental=False,
    skip_io=False,
    bin_grid_count=None,  # positive int, default 0
    density=None,  # 'uniform' or 0.0 < d < 1.0  default 0.7
    init_density_penalty=None,  # positive float  default 0.00008
    init_wirelength_coef=None,  # positive float  default 0.25
    min_phi_coef=None,  # positive float  default 0.95
    max_phi_coef=None,  # positive float  default 1.05
    reference_hpwl=None,  # positive int    default 446000000
    overflow=None,  # positive float
    initial_place_max_iter=None,  # positive int, default 20
    initial_place_max_fanout=None,  # positive int, default 200
    routability_check_overflow=None,  # positive float
    routability_max_density=None,  # positive float  default  0.99
    routability_max_bloat_iter=None,  # positive int  default  1
    routability_max_inflation_iter=None,  # positive int  default 4
    routability_target_rc_metric=None,  # positive float
    routability_inflation_ratio_coef=None,  # positive float
    routability_max_inflation_ratio=None,  # positive float
    routability_rc_coefficients=None,  # a list of four floats
    timing_driven_net_reweight_overflow=None,  # list of ints
    timing_driven_net_weight_max=None,  # float
    timing_driven_nets_percentage=None,  # float
    pad_left=None,  # positive int
    pad_right=None,  # positive int
):
    gpl = design.getReplace()

    if skip_initial_place:
        gpl.setInitialPlaceMaxIter(0)
    elif is_pos_int(initial_place_max_iter):
        gpl.setInitialPlaceMaxIter(initial_place_max_iter)

    gpl.setSkipIoMode(skip_io)
    if skip_io:
        gpl.setInitialPlaceMaxIter(0)

    gpl.setTimingDrivenMode(timing_driven)

    if timing_driven:
        if design.evalTclString('get_libs -quiet "*"') == "":
            utl.error(utl.GPL, 502, "No liberty libraries found.")

        if skip_io:
            utl.warn(utl.GPL, 503, "-skip_io will disable timing driven mode.")
            gpl.setTimingDrivenMode(False)

        if timing_driven_net_reweight_overflow != None:
            overflow_list = timing_driven_net_reweight_overflow
        else:
            overflow_list = [79, 64, 49, 29, 21, 15]

        for ov in overflow_list:
            gpl.addTimingNetWeightOverflow(ov)

        if is_pos_float(timing_driven_net_weight_max):
            gpl.setTimingNetWeightMax(timing_driven_net_weight_max)

        if is_pos_float(timing_driven_nets_percentage):
            design.evalTclString(
                f"rsz::set_worst_slack_nets_percent {timing_driven_nets_percentage}"
            )

    gpl.setRoutabilityDrivenMode(routability_driven)

    if routability_driven:
        utl.warn(utl.GPL, 504, "-skip_io will disable routability driven mode.")
        gpl.setRoutabilityDrivenMode(False)

    if is_pos_int(initial_place_max_fanout):
        gpl.setInitialPlaceMaxFanout(initial_place_max_fanout)
    elif initial_place_max_fanout != None:
        utl.error(utl.GPL, 505, "initial_place_max_fanout must be a positive integer")

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
        gpl.setBinGridCnt(bin_grid_count, bin_grid_count)

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
            gpl.doIncrementalPlace(1)
        else:
            gpl.doInitialPlace()
            if not skip_nesterov_place:
                gpl.doNesterovPlace(1)
        gpl.reset()
    else:
        utl.error(
            utl.GPL,
            506,
            "No rows defined in design. Use initialize_floorplan to add rows.",
        )


def is_pos_int(x):
    if x == None:
        return False
    elif isinstance(x, int) and x > 0:
        return True
    else:
        utl.error(utl.GPL, 507, f"TypeError: {x} is not a postive integer")
    return False


def is_pos_float(x):
    if x == None:
        return False
    elif isinstance(x, float) and x > 0.0:
        return True
    else:
        utl.error(utl.GPL, 508, f"TypeError: {x} is not a positive float")
    return False
