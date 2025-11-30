# OpenROAD MCP Server

This directory contains an [MCP (Model Context
Protocol)](https://modelcontextprotocol.io/) server that provides an interface
to OpenROAD. It allows AI agents to interact directly with the OpenROAD flow,
run commands, and inspect design data.

## Overview

The server exposes a set of tools that wrap standard OpenROAD Tcl commands and Python APIs. This enables:
- Loading designs (LEF, DEF, Verilog).
- Running timing analysis (WNS, TNS, detailed checks).
- Inspecting the design database (cells, properties).
- Executing arbitrary Tcl commands.

## Prerequisites

- OpenROAD installed and built in your workspace.
- Bazel build system.

## How to Run the Server

To start the MCP server, use the following Bazel command from the root of the OpenROAD repository:

```bash
bazel run //openroad/mcp:server
```

This will start the server using the HTTP transport on `http://127.0.0.1:8000`
(default). Only clients on localhost are allowed to make calls to the MCP
server.

The server itself is an instance of OpenROAD. When you start the server, this is
equivalent to starting OpenROAD with an empty database. If at any point you need to
re-load a modified design or you want to rerun a script from zero, you will need
to either ask your agent to use the `reset` tool or restart the server.

## Using with Gemini CLI

### Install

Add the following config to your Gemini's CLI settings:

```json
  "mcpServers": {
    "openroad": {
      "httpUrl": "http://127.0.0.1:8000/mcp",
      "trust": true
    }
  }
```

This file is usually inside your $HOME/.gemini folder. Other agents should
have a similar config step.

### Usage

It is recommended that you run your agent in the PWD of your project. The agent
will automatically change the OpenROAD working directory to your project directory, or you can ask it to do so
if needed. 


### Examples

Full example is available [here](https://gist.github.com/fgaray/5e944d829201a41fc0610073d822407f)

```
> Change openroad's pwd into this current folder. Source my_design.tcl and
perform a timing analysis.
```

```
> Explain why the current loaded design is not meeting timing.
```

```
> Do a timing analysis of the SS, TT, and FF corners of my design.
```

## Available Tools

The server exposes the following tools to the AI agent:

- **Core Control:**
  - `run_tcl`: Execute any OpenROAD Tcl command.
  - `source`: Source and run a Tcl file.
  - `cd`, `pwd`: Manage the current working directory of the OpenROAD instance.
  - `reset`: Reset the OpenROAD database.

- **Design Loading:**
  - `read_lef`: Read LEF/Tech/Lib files.
  - `read_def`: Read DEF files.
  - `read_verilog`: Read Verilog files.

- **Inspection:**
  - `get_cells`: Search for cell instances by name pattern.
  - `get_property`: Query specific properties of cells, nets, pins, etc.

- **Timing Analysis:**
  - `report_wns`: Report Worst Negative Slack.
  - `report_tns`: Report Total Negative Slack.
  - `report_checks`: Perform detailed timing path analysis.

## Development

The server implementation is located in `server.py`. It uses the `fastmcp` library to define tools and handle requests.
