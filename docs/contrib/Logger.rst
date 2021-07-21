Using the Logging Infrastructure
================================

OpenROAD uses `spdlog`_ as part of logging infrastructure in order to
ensure a clear, consistent messaging and complete messaging interface. A
wrapper formats the prefix in the recommended messaging style and limit. A
message format is as follows:

.. code-block:: text

   <tool id> <message id> : <Message body>.

For Example,

.. code-block:: text

   [INFO ODB-0127] Reading DEF file: ./results/asap7/aes/base/4_cts.def

All output from OpenROAD tools should be directed through the logging API
to ensure that redirection, file logging and execution control flow is
handled consistently. This also includes messages from a third party tool.
Use the 'ord' message ID for third party tools.

The logging infrastructure also supports generating a `JSON`_ file
containing design metrics (e.g. area, slack). This output is directed to
a user specified file. The OpenROAD application has a ``-metrics`` command
line argument to specify the file.

Handling Messages
------------------

OpenROAD supports multiple levels of severity for message outputs as:
Critical, Error, Warning, Information, Debug. These are supported by
automatic calls to the logger which will then prefix the appropriate severity
type to the message.

Messaging Guidelines
--------------------

In addition to the proper use of message types, follow the guidelines below
to compose messages for clarity, consistency and other guidelines:

Grammar
~~~~~~~

Start with a capital letter and end with period besides well-known
exceptions. Use capital letters for file formats and tool names. e.g., LEF,
DEF, SPICE.

After the first word's capitalization, do not use capital letters (aside
from obvious exceptions - RSMT, hCut, etc.

Do not use exclamations. Severity must be communicated by message severity
and clear implied or explicit action.

Avoid long, verbose messages. Use commas to list and separate clauses
in messages.

Spellcheck all messages using American English spellings.

Use ellipsis ``...`` only to indicate a pause, as when some tool is running
or being initialized.

Abbreviations and Shortcuts
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use single-word versions when well-accepted / well-understood by users and
developers. Examples: ``stdcell, cutline, wirelength, stdcell, flipchip,
padring, bondpad, wirebond, libcell, viarule etc``.

Do not abbreviate English words, expand for the sake of clarity.

========= =======
Incorrect Correct
========= =======
Num, #    Number
Tot.      Total
========= =======

Use acceptable, well-understood abbreviations for brevity.
Examples: ``db, tech, lib, inst, term, params, etc``.

Avoid contractions of action words:

==============   =======
Incorrect        Correct
==============   =======
Can't, Can not   Cannot
Don't            Do not
==============   =======

Actionability Messages should have clear, implied or an explicit action
that is necessary for flow continuation or quality of results.

.. code-block:: text

   Correct:
   A value for core_area must specified in the footprint specification, or in the environment variable CORE_AREA

Clarity Messages must be clear and complete so as to communicate necessary
and sufficient information and action. Elaborate variable, option, parameter
numbers.

.. code-block:: text

   Correct:
   Unrecognized argument $arg, should be one of -pitch, -bump_pin_name, -spacing_to_edge, -cell_name, -bumps_per_tile, -rdl_layer, -rdl_width, -rdl_spacing.

Specify objects clearly in the local context:

.. code-block:: text

   Correct:
   cutWithin is smaller than cutSpacing for ADJACENTCUTS on layer {}, please check your rule definition.

   Incomplete:
   Warning: {} does not have viaDef aligned with layer.

Make any assumptions or use of default values explicit:

.. code-block:: text

   Correct:
   No net slacks found.
   Timing-driven mode disabled.

   Incomplete, missing default:
   Utilization exceeds 100%.

Use simple language, avoid repetitions:

.. code-block:: text

   Correct:
   Missing orientation for cell $cell_ref

   Incorrect:
   No orientation available for orientation of $cell_ref.

Message Types
~~~~~~~~~~~~~

OpenROAD supports the following levels of severity through the logger:
Report, Debug, Information, Error, Warning and Critical.

Report
++++++

Reports are tool output in the form of a report to the user. Examples
include timing paths or power analysis results.

Example Report message:

.. code-block:: text

    Path startpoint: $startpoint

Debug
+++++

Debug messages are only of use to tool developers and not to end users.
These messages are not shown unless explicitly enabled.

Information
+++++++++++

Information messages may be used for reporting metrics, quality of
results, or program status to the user. Any message which indicates
runtime problems, such as potential faulty input or other internal
program issues, should be issued at a higher status level.

Example Information messages:

.. code-block:: text

   Number of input ports: 47
   Running optimization iteration 2
   Current cell site utilization: 57.1567%

Warning
+++++++

Warnings should be used for indicating atypical runtime conditions that
may affect quality, but not correctness of the output. Any conditions
that affect correctness should be issued at a higher status level.

Example Warning messages:

.. code-block:: text

   Core area utilization is greater than 90%. The generated cell placement may not be routable.

   14 outputs are not constrained for max capacitance.

   Pin 'A[0]' on instance 'mem01' does not contain antenna information and will not be checked for antenna violations.

Error
+++++

Error messages should be used for indicating correctness problems.
Problems with command arguments are a good example of errors. Errors
exit the current command by throwing an exception that can be caught in a Tcl
command script. Errors that occur while reading a command file stop
executing the script commands.

Example Error messages:

.. code-block:: text

   Invalid selection: net 'test0' does not exist in the design.

   Cell placement cannot be run before floorplanning.

   Argument 'max_routing_layer' expects an integer value from 1 to 10.

Critical
++++++++

Critical messages should be used for indicating correctness problems
that the program is not able to work around or ignore, and require
immediate exiting of the program (abort).

Example Critical messages:

.. code-block:: text

   Database 'chip' has been corrupted and is not recoverable.

   Unable to allocate heap memory for array 'vertexIndices'. The required memory size may exceed host machine limits.

   Assertion failed: 'nodeVisited == false' on line 122 of example.cpp. Please file a Github issue and attach a testcase.

Coding
------

Each status message requires:

* The three letter tool ID
* The message ID
* The message string
* Optionally, additional arguments to fill in placeholders in the message string

Reporting is simply printing and does not require a tool or message ID.
The tool ID comes from a fixed enumeration of all the tools in the
system. This enumeration is in ``Logger.h``. New abbreviations should be
added after discussion with the system architects. The abbreviation matches
the c++ namespace for the tool.

Message IDs are integers. They are expected to be unique for each tool.
This has the benefit that a message can be mapped to the source code
unambiguously even if the text is not unique. Maintaining this invariant is
the tool owner's responsibility. To ensure that the IDs are unique each
tool should maintain a file named 'messages.txt' in the top level tool
directory listing the message IDs along with the format string. When code
that uses a message ID is removed the ID should be retired by removing it
from 'messages.txt'. See the utility ``etc/find_messages.py`` to scan
a tool directory and write a ``messages.txt`` file.

Spdlog comes with the ``fmt`` library which supports
message formatting in a python / `C++20 like style`_.

The message string should not include the tool ID or message ID which will
automatically be prepended. A trailing new line will automatically be added
so messages should not end with one. Messages should be written as complete
sentences and end in a period. Multi-line messages may contain embedded
new lines.

Some examples:

.. code-block:: c++

   logger->report("Path startpoint: {}", startpoint);
   logger->error(ODB, 25, "Unable to open LEF file {}.", file_name);
   logger->info(DRT, 42, "Routed {} nets in {:3.2f}s.", net_count, elapsed_time);

Tcl functions for reporting messages are defined in the OpenROAD swig file
``OpenRoad.i``. The message is simply a Tcl string (no C++20 formatting). The
logger for Tcl functions The above examples in Tcl are shown below.

.. code-block:: c++

   utl::report "Path startpoint: $startpoint"
   utl::error ODB 25 "Unable to open LEF file $file_name."
   utl::info DRT 42 "Routed $net_count nets in [format %3.2f $elapsed_time]."

``utl::report`` should be used instead of 'puts' so that all output
is logged.

Calls to the Tcl functions ``utl::warn`` and ``utl::error`` with a single
message argument report with tool ``ID UKN`` and message ``ID 0000``.

Tools ``#include utl/Logger.h`` that defines the logger API. The Logger
instance is owned by the OpenROAD instance. Each tool should retrieve
the logger instance in the tool init function called after the tool make
function by the OpenROAD application.

Every tool swig file must include src/Exception.i so that errors thrown
by ``utl::error`` are caught at the Tcl command level. Use the following swig
command before ``%inline``.

.. code-block:: swig

   %include "../../Exception.i"

The logger functions are shown below.

.. code-block:: c++

   Logger::report(const std::string& message,
                  const Args&... args)
   Logger::info(ToolId tool,
                int id,
                const std::string& message,
                const Args&... args)
   Logger::warn(ToolId tool,
                int id,
                const std::string& message,
                const Args&... args)
   Logger::error(ToolId tool,
                 int id,
                 const std::string& message,
                 const Args&... args)
   Logger::critical(ToolId tool,
                    int id,
                    const std::string& message,
                    const Args&... args)

The corresponding Tcl functions are shown below.

.. code-block:: tcl

   utl::report message
   utl::info tool id message
   utl::warn tool id message
   utl::error tool id message
   utl::critical tool id message

Although there is a ``utl::critical`` function, it is really difficult to
imagine any circumstances that would justify aborting execution of the
application in a tcl function.

Debug Messages
~~~~~~~~~~~~~~

The debug message have a different programming model. As they are most
often *not* issued the concern is to avoid slowing down normal
execution. For this reason such messages are issued by using the
debugPrint macro. This macro will avoid evaluating its arguments if they
are not going to be printed. The API is:

.. code-block:: c++

   debugPrint(logger, tool, group, level, message, ...);

The ``debug()`` method of the Logger class should not be called directly. No
message id is used as these messages are not intended for end users. The
level is printed as the message id in the output.

The argument types are as for the info/warn/error/ciritical messages.
The one additional argument is group which is a ``const char*``. Its
purposes is to allow the enabling of subsets of messages within one
tool.

Debug messages are enabled with the tcl command:
``set_debug_level <tool> <group> <level>``

Metrics
-------

The metrics logging uses a more restricted API since JSON only supports
specific types. There are a set of overloaded methods of the form:

.. code-block:: c++

   metric(ToolId tool,
          const std::string_view metric,
          <type> value)

where ``<type>`` can be ``int, double, string, or bool``. This will result in
the generated JSON:

.. code-block:: text

   "<tool>-<metric>" : value

String values will be enclosed in double-quotes automatically.

Converting to Logger
--------------------

The error functions in ``include/openroad/Error.hh`` should no longer be
included or used. Use the corresponding logger functions.

All uses of the tcl functions ord::error and ord::warn should be updated
call the ``utl::error/warn`` with a Tool ID and message ID. For
compatibility these are defaulted to ``UKN`` and ``0000`` until they are
updated.

Regression tests should not have any ``UKN-0000`` messages in their ok
files. A simple grep should indicate that you still have pending calls
to pre-logger error/warn functions.

The cmake file for the tool must also be updated to include spdlog in
the link libraries so it can find the header files if they are not in
the normal system directories.


.. note::

   At UCSD, dfm is an example of this problem; it has an ancient version
   of spdlog in '/usr/include/spdlog'. Use `module` to install spdlog 1.8.1
   on dfm and check your build there.

.. code-block:: cmake

   target_link_libraries(<library_target>
     PUBLIC
       utl
   )

================ =================
Tool             message/namespace
================ =================
antenna_checker  ant
dbSta            sta
FastRoute        grt
finale           fin
flute3           stt
gui              gui
ICeWall          pad
init_fp          ifp
ioPlacer         ppl
OpenDB           odb
opendp           dpl
OpenRCX          rcx
OpenROAD         ord
OpenSTA          sta
PartMgr          par
pdngen           pdn
PDNSim           psm
replace          gpl
resizer          rsz
tapcell          tap
TritonCTS        cts
TritonMacroPlace mpl
TritonRoute      drt
utility          utl
================ =================

.. _`spdlog`: https://isocpp.org/blog/2014/11/spdlog
.. _`JSON`: https://www.json.org
.. _`C++20 like style`: https://en.cppreference.com/w/cpp/utility/format/formatter#Standard_format_specification
