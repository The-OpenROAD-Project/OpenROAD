Getting Started
===============

Get the tools
-------------

There are currently two options to get OpenROAD tools.

Option 1: download pre-build binaries
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We currently support pre-built binaries on CentOS 7. Please, refer to
the `releases page on
GitHub <https://github.com/The-OpenROAD-Project/OpenROAD-flow/releases>`__.

Option 2: build from sources
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

OpenROAD is divided into a number of tools that are orchestrated
together to achieve RTL-to-GDS. As of the current implementation, the
flow is divided into three stages:

1. Logic Synthesis: is performed by
   `yosys <https://github.com/The-OpenROAD-Project/yosys>`__.
2. Floorplanning through Detailed Routing: are performed by `OpenROAD
   App <https://github.com/The-OpenROAD-Project/OpenROAD>`__.

In order to integrate the flow steps,
`OpenROAD-flow <https://github.com/The-OpenROAD-Project/OpenROAD-flow>`__
repository includes the necessary scripts to build and test the flow.

**Prerequisites**

Build dependencies are documented in the Dockerfile of each tool. 1. See
`yosys
Dockerfile <https://github.com/The-OpenROAD-Project/yosys/blob/master/Dockerfile>`__
2. See `OpenROAD App
Dockerfile <https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/Dockerfile>`__

Before proceeding to the next step: 1. [recommended] Install
`Docker <https://docs.docker.com/install/linux/docker-ce/centos/>`__ on
your machine, OR 2. [bare-metal] Make sure that build dependencies for
all the tools are installed on your machine.

**Clone and build**

::

   git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD-flow
   ./build_openroad.sh

The build script will automatically use Docker builds if it finds
``docker`` command installed on the system.

Verify Installation
~~~~~~~~~~~~~~~~~~~

Setup environment:

1. ``./setup_env.sh`` if the build was done using Docker.
2. ``./setup_env_bare_metal_build.sh`` if the build was done on the
   bare-metal.

**Verify**

The following binaries should be available on your ``$PATH`` after
setting up the environment

-  ``yosys -h``
-  ``openroad -h``

Setting up the Flow
-------------------

1. Clone the repository

::

   git clone https://github.com/The-OpenROAD-Project/OpenROAD-flow
   cd OpenROAD-flow/flow

2. Setup your shell environment. The ``openroad`` app must be setup to
   implement designs or run tests. See setup instructions in the
   repository *Verify Installation* section above.

Designs
-------

Sample design configurations are available in the ``designs`` directory.
You can select a design using either of the following methods: 1. The
flow
`Makefile <https://github.com/The-OpenROAD-Project/OpenROAD-flow/blob/master/flow/Makefile>`__
contains a list of sample design configurations at the top of the file.
Uncomment the respective line to select the design 2. Specify the design
using the shell environment, e.g.
``make DESIGN_CONFIG=./designs/nangate45/swerv.mk`` or
``export DESIGN_CONFIG=./designs/nangate45/swerv.mk; make``

By default, the simple design gcd is selected. We recommend implementing
this design first to validate your flow and tool setup.

Adding a New Design
~~~~~~~~~~~~~~~~~~~

To add a new design, we recommend looking at the included designs for
examples of how to set one up.

.. warning::
   Please refer to the known issues and limitations `document
   <https://github.com/The-OpenROAD-Project/OpenROAD-flow/blob/openroad/flow/docs/Known%20Issues%20and%20Limitations.pdf>`__
   for information on conditioning your design/files for the flow. We are
   working to reduce the issues and limitations, but it will take time.

Platforms
---------

OpenROAD-flow supports Verilog to GDS for the following open platforms:
\* Nangate45 / FreePDK45

These platforms have a permissive license which allows us to
redistribute the PDK and OpenROAD platform-specific files. The platform
files and license(s) are located in ``platforms/{platform}``.

OpenROAD-flow also supports the following commercial platforms: \*
TSMC65LP \* GF14 (in progress)

The PDKs and platform-specific files for these kits cannot be provided
due to NDA restrictions. However, if you are able to access these
platforms, you can create the necessary platform-specific files
yourself.

Once the platform is setup. Create a new design configuration with
information about the design. See sample configurations in the
``design`` directory.

Adding a New Platform
~~~~~~~~~~~~~~~~~~~~~

At this time, we recommend looking at the
`Nangate45 <https://github.com/The-OpenROAD-Project/OpenROAD-flow/tree/openroad/flow/platforms/nangate45>`__
as an example of how to set up a new platform for OpenROAD-flow.

Implement the Design
--------------------

Run ``make`` to perform Verilog to GDS. The final output will be located
at ``flow/results/{platform}/{design_name}/6_final.gds``

Miscellaneous
-------------

tiny-tests - easy to add, single concern, single Verilog file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The tiny-tests are have been designed with two design goals in mind:

1. It should be trivial to add a new test: simply add a tiny standalone
   Verilog file to ``OpenROAD-flow/flow/designs/src/tiny-tests``
2. Each test should be as small and as standalone as possible and be a
   single concern test.

To run a test:

::

   make DESIGN_NAME=SmallPinCount DESIGN_CONFIG=`pwd`/designs/tiny-tests.mk

nangate45 smoke-test harness for top level Verilog designs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Drop your Verilog files into designs/src/harness
2. Start the workflow:

::

   make DESIGN_NAME=TopLevelName DESIGN_CONFIG=`pwd`/designs/harness.mk

TIP! Start with a small tiny submodule in your design with few pins
