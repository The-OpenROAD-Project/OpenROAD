import utl
from string import Template
import rmp
import os

# So, getting back objects from evalTclString is not supported and we
# end up with this... These lib pins appear to be Liberty lib pins,
# and it doesn't look like I can use odb to get these objects.
# Must wait until sta is wrapped?
#
# To be used with this substition dict, where portname is a string
# {'tielohi_port': portname, 'tie' : 'hi'} or
# {'tielohi_port': portname, 'tie' : 'lo'}
lohitemp = Template(
    """set lohiport $tielohi_port
      if { ![sta::is_object $$lohiport] } {
        set lohiport [sta::get_lib_pins $tielohi_port]
        if { [llength $$lohiport] > 1 } {
          # multiple libraries match the lib port arg; use any
          set lohiport [lindex $$lohiport 0]
        }
      }
      if { $$lohiport != "" } {
        rmp::set_tie${tie}_port_cmd $$lohiport
      }
    """
)


def set_tiehi(design, tiehi_port):
    if tiehi_port == None:
        utl.error(utl.RMP, 301, "Must specify a tiehi_port")
    tieHiport = design.evalTclString(
        lohitemp.substitute({"tielohi_port": tiehi_port, "tie": "hi"})
    )


def set_tielo(design, tielo_port):
    if tielo_port == None:
        utl.error(utl.RMP, 302, "Must specify a tielo_port")
    tieLoport = design.evalTclString(
        lohitemp.substitute({"tielohi_port": tielo_port, "tie": "lo"})
    )


def restructure(
    design,
    *,
    liberty_file_name="",
    target="area",
    slack_threshold=0.0,
    depth_threshold=16,
    workdir_name=".",
    tielo_port=None,
    tiehi_port=None,
    abc_logfile=""
):
    os.makedirs(workdir_name, exist_ok=True)
    rst = design.getRestructure()
    set_tielo(design, tielo_port)
    set_tiehi(design, tiehi_port)
    rst.setMode(target)
    rst.run(
        liberty_file_name, slack_threshold, depth_threshold, workdir_name, abc_logfile
    )


def create_blif(design, *, hicell="", hiport="", locell="", loport=""):
    logger = design.getLogger()
    sta = design.getTech().getSta()
    return rmp.Blif(logger, sta, locell, loport, hicell, hiport)


def blif_read(design, blif, filename):
    return blif.readBlif(filename, design.getBlock())
