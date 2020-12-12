# Using the logging infrastructure

In order to ensure consistent messaging from the openroad application we have adopted spdlog as our logging infrastructure.  We have a thin wrapper on top for extensibility.  Whenever a message needs to be issued you will use one of the logging functions in the ‘ord’ namespace.

All output from OpenROAD tools should be directed through the logging API so that redirection, file logging and execution control flow is handled consistently.

## Message Types

### Report
Reports are tool output in the form of a report to the user. Examples are timing paths, or power analysis results. Tool reports that use ‘printf’ or c++ streams should use the report message API instead.
Information
Information messages may be used for reporting metrics, quality of results, or program status to the user. Any messages which indicate runtime problems, such as potential faulty input or other internal program issues, should be issued at a higher status level.

Example messages for this status level:
Number of input ports: 47
Running optimization iteration 2
Current cell site utilization: 57.1567%

### Warning
Warnings should be used for indicating atypical runtime conditions that may affect quality, but not correctness of the output. Any conditions that affect correctness should be issued at a higher status level.

Example warning messages:
* Core area utilization is greater than 90%. The generated cell placement may not be routable.
* 14 outputs are not constrained for max capacitance.
* Pin ‘A[0]’ on instance ‘mem01’ does not contain antenna information and will not be checked for antenna violations.

### Error
Error messages should be used for indicating correctness problems where the program is able to work around the issue or ignore the problem. Problems which prevent execution from continuing should be issued at a higher status level.

Example error messages:
* Invalid selection: net ‘test0’ does not exist in the design.
* Cell placement cannot be run before floorplanning.
* Argument ‘max_routing_layer’ expects an integer value from 1 to 10.

### Critical
Critical messages should be used for indicating correctness problems that the program is not able to work around or ignore, and require immediate exiting of the function (ie, throw an error).

Example critical messages:
* Database ‘chip’ has been corrupted and is not recoverable.
* Unable to allocate heap memory for array ‘vertexIndices’. The required memory size may exceed host machine limits.
* Assertion failed: ‘nodeVisited == false’ on line 122 of example.cpp. Please file a Github issue and attach a testcase.

## Coding
Each status message requires:
* The three letter tool ID
* The message ID
* The message string
* Optionally, additional arguments to fill in placeholders in the message string

Reporting is simply printing and does not require a tool or message ID. The tool ID comes from a fixed enumeration of all the tools in the system. This enumeration is in Logger.h. New abbreviations should be added after discussion with the system architects. The abbreviation matches the c++ namespace for the tool.

Message IDs are integers. They are expected to be unique for each tool.  This has the benefit that a message can be mapped to the source code unambiguously even if the text is not unique.  Maintaining this invariant is the tool owner’s responsibility. To ensure that the IDs are unique each tool should maintain a file named ‘messages.txt’ in the top level tool directory listing the message IDs along with the format string. When code that uses a message ID is removed the ID should be retired by removing it from ‘messages.txt’.

Spdlog comes with the fmt library which supports message formatting in a python / c++20 like style.

The message string should not include the tool ID or message ID which will automatically be prepended.  A trailing new line will automatically be added so messages should not end with one.  Messages should be written as complete sentences and end in a period. Multi-line messages may contain embedded new lines.

Some examples:

* logger->report(“Path startpoint: {}”, startpoint);
* logger->error(ODB, 25, “Unable to open LEF file {}.”, file_name);
* logger->info(DRT, 42, “Routed {} nets in {:3.2f}s.”, net_count, elapsed_time);

Tcl functions for reporting messages are defined in the OpenRoad swig file OpenRoad.i. The message is simply a Tcl string (no c++20 formatting). The logger for Tcl functions The above examples in Tcl are shown below.

* ord::report “Path startpoint: $startpoint”
* ord::error ODB 25 “Unable to open LEF file $file_name.”
* ord::info DRT 42 “Routed $net_count nets in [format %3.2f $elapsed_time].”

ord::report should be used instead of ‘puts’ so that all output is logged.

Calls the the previously defined functions ord::warn and ord::error with a single message argument report with tool ID “UKN” and message ID 0000.

Tools '#include openroad/Logger.h' that defines the logger API. Logger functions require an instance of the Logger class as the first argument. The Logger instance is owned by the OpenRoad instance. Each tool should retrieve the logger instance in the tool init function called after the tool make function by the OpenRoad application.

Every tool swig file must include src/Exception.i so that errors thrown by ord::error are caught at the Tcl command level. Use the following swig command before %inline.

'''
%include "../../Exception.i"
'''

The logger functions are shown below.

'''
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
'''

## Converting to Logger

The error functions in 'include/openroad/Error.hh' should no longer be included or used.
Use the corresponding logger functions.

All uses of the tcl functions ord::error and ord::warn should be updated to pass in a Tool ID and message ID. For compatibility these are defaulted to 'UKN' and '0000' until they are updated.

There is no reason to 'puts' (ie, print) errors in regression tests that are caught.
The logger prints the error now.

The resizer and opendp have been updated to use the Logger if you need examples of how to initialize and use it.

Regression tests should not have any "UKN-0000" messages in their ok files. A simple grep should indicate that you still have pending calls to pre-logger error/warn functions.
`

The cmake file for the tool must also be updated to include spdlog in the link libraries so it can find the header files if they are not in the normal system directories. dfm is an example of this problem; it has an ancient version of spdlog in '/usr/include/spdlog'. Use module to instal spdlog 1.8.1 on dfm and check your build there.

```
target_link_libraries(<library_target>
  PUBLIC
  spdlog::spdlog
  )
```
