from fastmcp import FastMCP
import openroad.odb as odb
from openroad import Tech, Design
import os
import sys
import tempfile
from typing import Literal

# Create an MCP server
mcp = FastMCP(
    name = "OpenROAD",
    instructions = """
      This server provides an instance of OpenROAD.
      OpenROAD's unified application implementing an RTL-to-GDS Flow. Documentation at https://openroad.readthedocs.io/en/latest/
      This MCP server expose several tools that you can use to interact with the current instance.
      You don't need to write tcl files, you can direclty use run_tcl to send commands to openroad.
      Check with the command "pwd" to know in what path you are running OpenROAD. Use "cd" to change directories to be able to read files correctly.
    """
)

tech = Tech()
design = Design(tech)

def eval_tcl(command):
  # Create a temporary file to capture stdout
  with tempfile.TemporaryFile(mode='w+') as tmp:
      # Flush stdout to ensure order
      sys.stdout.flush()
      
      # Save original stdout
      original_stdout_fd = os.dup(1)
      
      try:
          # Redirect stdout to the temporary file
          os.dup2(tmp.fileno(), 1)
          
          # Run the command
          tcl_result = design.evalTclString(command)
      finally:
          # Restore stdout
          os.dup2(original_stdout_fd, 1)
          os.close(original_stdout_fd)
          
      # Read the captured output
      tmp.seek(0)
      stdout_output = tmp.read()
      
  if stdout_output:
      return stdout_output
  return tcl_result

@mcp.tool
def source(filename: str) -> str:
  """Sources a tcl file and runs it into OpenROAD

  Args:
    filename: The absolute path to the tcl file to source and run

  Returns
    The status of the command.
  """

  return eval_tcl("source %s" %filename)

@mcp.tool
def cd(path: str) -> str:
  """Changes the current folder where OpenROAD is running

  Args:
    path: The new path were OpenROAD will run from

  Returns
    The status of the command.
  """

  return eval_tcl("cd %s" %path)

@mcp.tool
def pwd() -> str:
  """Returns the current path where OpenROAD is running

  Returns
    The path where OpenROAD is running
  """

  return eval_tcl("pwd")


@mcp.tool
def run_tcl(command: str) -> str:
  """Runs any tcl command available on OpenROAD

  Args:
    command: A tcl command to run on this OpenROAD instance

  Returns:
    The text response of the command.
  """
  return eval_tcl(command)

@mcp.tool
def read_lef(filename: str, tech: bool, library: bool) -> str:
  """Reads Library Exchange Format (.lef) files or tech files or library (.lib) files

  Args:
    filename: The absolute path to the file to read.
    tech: boolean if we are reading a tech file
    library: boolean if we are reading a library file

  Returns
    The status of the command.

  """

  if tech:
    return eval_tcl("read_lef -tech %s" %filename)
  if library:
    return eval_tcl("read_lef -library %s" %filename)
  return eval_tcl("read_lef %s" %filename)


@mcp.tool
def read_def(filename: str) -> str:
  """Read Design Exchange Format (.def) files.

  Args:
    filename: The absolute path to the file to read.

  Returns
    The status of the command.
  """

  return eval_tcl("read_def %s" %filename)


@mcp.tool
def read_verilog(filename: str) -> str:
  """Read Verilog (.v) input file.

  Args:
    filename: The absolute path to the verilog file to read.

  Returns
    The status of the command.
  """

  return eval_tcl("read_verilog %s" %filename)

@mcp.tool()
def get_property(
    object_name: str,
    object_type: Literal["cell", "net", "pin", "port", "lib_cell"],
    property_name: str
) -> str:
    """
    Queries a specific property of a design object in the OpenROAD database.
    
    Use this to inspect specific details of the design, such as the area of a cell,
    the capacitance of a net, or the reference name of an instance.

    Args:
        object_name: The specific name of the object (e.g., "u123", "clk_net", "u123/A").
        object_type: The category of the object. Used to find the object in the DB.
                     - 'cell': Design instances (e.g., u123).
                     - 'net': Wires/Nets (e.g., net_a).
                     - 'pin': Pins on instances (e.g., u123/A).
                     - 'port': Top-level Input/Output ports.
                     - 'lib_cell': The master library cell (e.g., sky130_fd_sc_hd__nand2_1).
        property_name: The attribute to retrieve. Common properties include:
                       - For cells: "area", "ref_name", "is_sequential"
                       - For nets: "capacitance", "weight"
                       - For pins: "direction", "arrival_window"
    """
    
    selector_map = {
        "cell": "get_cells",
        "net": "get_nets",
        "pin": "get_pins",
        "port": "get_ports",
        "lib_cell": "get_lib_cells"
    }
    
    selector_cmd = selector_map.get(object_type)
    
    tcl_command = f"get_property [{selector_cmd} {{{object_name}}}] {property_name}"
    return eval_tcl(tcl_command)


@mcp.tool()
def get_cells(
    pattern: str = "*",
    hierarchical: bool = False,
    limit: int = 20
) -> str:
    """
    Searches for cell instances in the design matching a specific name pattern.
    Returns a list of instance names (e.g., "u_core/u_alu/u_adder").

    Use this to explore the design hierarchy or find specific modules.

    Args:
        pattern: The wildcard pattern to match (e.g., "u_cpu*", "*alu*", or "u_regs/*").
                 Default is "*" (all cells in current scope).
        hierarchical: If True, searches recursively through the hierarchy.
                      If False, searches only the top level (or current scope).
        limit: The maximum number of results to return. Critical for preventing
               context overflow on large designs. Default is 20.

    Returns:
        str: A space-separated string containing the full hierarchical names of the
             matching cell instances. If no cells are found, returns an empty string.
    """

    # The -quiet flag prevents errors if no cells are found
    flags = "-quiet"
    if hierarchical:
        flags += " -hierarchical"

    # We cannot simply run 'get_cells' because it returns an opaque pointer.
    # We must loop through the collection and extract names.
    tcl_command = f"""
    set collection [get_cells {flags} {{{pattern}}}]
    set count 0
    set result_list []

    foreach_in_collection c $collection {{
        if {{ $count >= {limit} }} {{
            break
        }}
        # Extract the human-readable name of the cell
        lappend result_list [get_object_name $c]
        incr count
    }}

    # Return the list as a string
    puts $result_list
    """

    return eval_tcl(tcl_command)

@mcp.tool
def reset() -> str:
  """
  Resets OpenROAD dropping the database.
  WARNING: You will need to start from the begining loading all the libraries and designs again.
  Use this command if you need to change something and OpenROAD does not allow you to do it after loading a design like re-defining corners.
  """
  tcl_command """
  set db [::ord::get_db]
  $db clear
  dbDatabase_destroy $db
  set db [dbDatabase_create]
  """
  return eval_tcl(tcl_command)


# Timing tools
@mcp.tool
def report_wns() -> str:
  """
  Calculates and reports the Worst Negative Slack (WNS) for the currently
  loaded Verilog design in OpenROAD.

  WNS is the critical metric for timing closure; a negative value indicates
  that the design does not meet frequency constraints.
  """
  return eval_tcl("report_wns")

@mcp.tool()
def report_tns(digits: int = 2) -> str:
  """
  Calculates and reports the Total Negative Slack (TNS) for the currently
  loaded Verilog design in OpenROAD.

  TNS is the sum of all negative slack across all failing endpoints in the
  design. While WNS (Worst Negative Slack) indicates the speed limit, TNS
  indicates the *magnitude* of the failing paths. A large TNS value suggests
  many paths are failing and the design may need significant architectural
  changes or different optimization strategies.
  """
  return eval_tcl("report_tns")

@mcp.tool()
def report_checks(
    path_delay: Literal["min", "max", "min_max"] = "max",
    group_count: int = 1,
    detailed: bool = False
) -> str:
    """
    Performs detailed timing path analysis (report_checks) on the current design.
    Returns the specific startpoints, endpoints, and logic gates associated with
    timing violations.

    Args:
        path_delay: The type of analysis to perform.
            - 'max': Setup analysis (detects paths that are too slow).
            - 'min': Hold analysis (detects paths that are too fast).
            - 'min_max': Reports both.
        group_count: The number of worst paths to report. Default is 1 (the single worst path).
        detailed: If True, expands clock network propagation and adds fields for slew,
                  capacitance, and input pins. useful for deep debugging.
    """

    cmd_parts = ["report_checks"]

    cmd_parts.append(f"-path_delay {path_delay}")
    cmd_parts.append(f"-group_count {group_count}")

    if detailed:
        cmd_parts.append("-format full_clock_expanded")
        cmd_parts.append("-fields {slew cap input_pins nets fanout}")
    else:
        cmd_parts.append("-format full")

    tcl_command = " ".join(cmd_parts)

    return eval_tcl(tcl_command)


if __name__ == "__main__":
    print(os.getcwd())
    # Run the server
    mcp.run(transport="http", host="127.0.0.1", port=8000)
