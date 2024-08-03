import drt
import utl


# NOTE: currently no error checking is done on the inputs as it is
#       done for the tcl version of detailed_route. If we want to use this
#       as a basis for a Python api, we will have to add that here
def detailed_route(
    design,
    *,
    output_maze="",
    output_drc="",
    output_cmap="",
    output_guide_coverage="",
    db_process_node="",
    disable_via_gen=False,
    droute_end_iter=-1,
    via_in_pin_bottom_layer="",
    via_in_pin_top_layer="",
    or_seed=-1,
    or_k=0,
    bottom_routing_layer="",
    top_routing_layer="",
    verbose=1,
    distributed=False,
    remote_host=None,
    remote_port=None,
    shared_volume=None,
    cloud_size=None,
    clean_patches=False,
    no_pin_access=False,
    single_step_dr=False,
    min_access_points=-1,
    save_guide_updates=False
):
    router = design.getTritonRoute()
    params = drt.ParamStruct()
    params.outputMazeFile = output_maze
    params.outputDrcFile = output_drc
    params.outputCmapFile = output_cmap
    params.outputGuideCoverageFile = output_guide_coverage
    params.dbProcessNode = db_process_node
    params.enableViaGen = not disable_via_gen
    params.drouteEndIter = droute_end_iter
    params.viaInPinBottomLayer = via_in_pin_bottom_layer
    params.viaInPinTopLayer = via_in_pin_top_layer
    params.orSeed = or_seed
    params.orK = or_k
    params.bottomRoutingLayer = bottom_routing_layer
    params.topRoutingLayer = top_routing_layer
    params.verbose = verbose
    params.cleanPatches = clean_patches
    params.doPa = not no_pin_access
    params.singleStepDR = single_step_dr
    params.minAccessPoints = min_access_points
    params.saveGuideUpdates = save_guide_updates

    router.setParams(params)
    router.main()


def step_dr(
    design,
    size,
    offset,
    mazeEndIter,
    workerDRCCost,
    workerMarkerCost,
    workerFixedShapeCost,
    workerMarkerDecay,
    ripupMode,
    followGuide,
):
    router = design.getTritonRoute()
    router.stepDR(
        size,
        offset,
        mazeEndIter,
        workerDRCCost,
        workerMarkerCost,
        workerFixedShapeCost,
        workerMarkerDecay,
        ripupMode,
        followGuide,
    )


def step_end(design):
    router = design.getTritonRoute()
    router.endFR()
