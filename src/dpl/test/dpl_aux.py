import utl
import re
import typing as t


def detailed_placement(
    design,
    *,
    max_displacement: t.Optional[t.Union[int, t.List[int]]] = None,
    disallow_one_site_gaps: bool = False,
    report_file_name: str = "",
    suppress=False,
):
    if not max_displacement:
        max_disp_x = 0
        max_disp_y = 0
    elif isinstance(max_displacement, int):
        max_disp_x = max_displacement
        max_disp_y = max_displacement
    elif len(max_displacement) == 2:
        max_disp_x = max_displacement[0]
        max_disp_y = max_displacement[1]
    else:
        utl.error(utl.DPL, 101, "-max_displacement disp | [disp_x, disp_y]")

    dpl = design.getOpendp()

    if len(design.getBlock().getRows()) > 0:  # db_has_rows
        site = design.getBlock().getRows()[0].getSite()
        max_disp_x = int(design.micronToDBU(max_disp_x) / site.getWidth())
        max_disp_y = int(design.micronToDBU(max_disp_y) / site.getHeight())
        dpl.detailedPlacement(
            max_disp_x, max_disp_y, report_file_name, disallow_one_site_gaps
        )
        if not suppress:
            dpl.reportLegalizationStats()
    else:
        utl(
            utl.DPL,
            102,
            "No rows defined in design. Use initialize_floorplan to add rows.",
        )


# Note that the tcl flag -instance in the tcl version of this command is not supported
def set_placement_padding(design, *, masters=None, right=None, left=None, globl=False):
    gpl = design.getOpendp()

    if not is_pos_int(left):
        left = 0

    if not is_pos_int(right):
        right = 0

    if globl:
        gpl.setPaddingGlobal(left, right)

    elif masters != None:
        for m in masters:
            gpl.setPaddingMasters(master, left, right)

    else:
        utl.warn(utl.DPL, 103, "No padding target specified")


def filler_placement(design, prefix=None, masters=None, verbose=False):
    dpl = design.getOpendp()
    if prefix == None:
        prefix = "FILLER_"

    if masters == None:
        utl.error(utl.DPL, 104, "filler_placement requires masters to be set")

    filler_masters = get_masters(design, masters)
    if len(filler_masters) > 0:
        dpl.fillerPlacement(filler_masters, prefix, verbose)


def get_masters(design, names):
    """Takes a design and a list of regular expressions to match master names.
    NOTE: these are Python regular expressions"""
    matched = False
    masters = []
    db = design.getTech().getDB()

    for name in names:
        for lib in db.getLibs():
            for master in lib.getMasters():
                master_name = master.getConstName()
                if re.fullmatch(name, master_name) != None:
                    masters.append(master)
                    matched = True

    if not matched:
        utl.warn(utl.DPL, 105, f"{names} did not match any masters")

    return masters


# actually, this should be called is_non_negative_int
def is_pos_int(x):
    if x == None:
        return False
    elif isinstance(x, int) and x >= 0:
        return True
    else:
        utl.error(utl.DPL, 106, f"TypeError: {x} is not a postive integer")
    return False
