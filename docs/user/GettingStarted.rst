Getting Started
===============

Get the tools
-------------

There are currently two options to get OpenROAD tools.

Option 1: download pre-build binaries
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We currently support pre-built binaries on CentOS 7. Please, refer to the `releases page on GitHub`_.

Option 2: build from sources
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

OpenROAD is divided into a number of tools that are orchestrated
together to achieve RTL-to-GDS. As of the current implementation, the
flow is divided into three stages:

1. Logic Synthesis: is performed by `yosys`_.
2. Floorplanning through Detailed Routing: are performed by `OpenROAD App`_.

In order to integrate the flow steps, `OpenROAD-flow`_ repository includes
the necessary scripts to build and test the flow.

**Prerequisites**

Build dependencies can be installed with `etc/DependencyInstaller.sh` and
for yosys they are documented in the `yosys Dockerfile`_.

Before proceeding to the next step:
1. [recommended] Install `Docker`_ on your machine, OR
2. [bare-metal] Make sure that build dependencies for all the tools are installed on your machine.

**Clone and build**

.. code-block:: shell

   $ git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD-flow
   $ ./build_openroad.sh

The build script will automatically use Docker builds if it finds
``docker`` command installed on the system.

Verify Installation
~~~~~~~~~~~~~~~~~~~

Setup environment: ``source ./setup_env.sh``

**Verify**

The following binaries should be available on your ``$PATH`` after
setting up the environment

-  ``yosys -h``
-  ``openroad -h``

Setting up the Flow
-------------------

1. Clone the repository

.. code-block:: shell

   git clone https://github.com/The-OpenROAD-Project/OpenROAD-flow
   cd OpenROAD-flow/flow

2. Setup your shell environment. The ``openroad`` app must be setup to
   implement designs or run tests. See setup instructions in the
   repository *Verify Installation* section above.

Designs
-------

Sample design configurations are available in the ``designs`` directory.
You can select a design using either of the following methods:

1. The flow `Makefile`_ contains a list of sample design configurations at
the top of the file.  Uncomment the respective line to select the design
2. Specify the design using the shell environment, e.g.
``make DESIGN_CONFIG=./designs/nangate45/swerv/config.mk`` or
``export DESIGN_CONFIG=./designs/nangate45/swerv/config.mk ; make``

By default, the simple design gcd is selected. We recommend implementing
this design first to validate your flow and tool setup.

Adding a New Design
~~~~~~~~~~~~~~~~~~~

To add a new design, we recommend looking at the included designs for
examples of how to set one up.

Platforms
---------

OpenROAD-flow supports Verilog to GDS for the following open platforms:
Nangate45 / FreePDK45

These platforms have a permissive license which allows us to
redistribute the PDK and OpenROAD platform-specific files. The platform
files and license(s) are located in ``platforms/{platform}``.

OpenROAD-flow also supports the following commercial platforms: TSMC65LP /
GF14 (in progress)

The PDKs and platform-specific files for these kits cannot be provided
due to NDA restrictions. However, if you are able to access these
platforms, you can create the necessary platform-specific files
yourself.

Once the platform is setup. Create a new design configuration with
information about the design. See sample configurations in the
``design`` directory.

Adding a New Platform
~~~~~~~~~~~~~~~~~~~~~

At this time, we recommend looking at the `Nangate45`_ as an example of
how to set up a new platform for OpenROAD-flow.

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

.. code-block:: shell

   make DESIGN_NAME=SmallPinCount DESIGN_CONFIG=`pwd`/designs/tiny-tests.mk

nangate45 smoke-test harness for top level Verilog designs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Drop your Verilog files into designs/src/harness
2. Start the workflow:

.. code-block:: shell

   make DESIGN_NAME=TopLevelName DESIGN_CONFIG=`pwd`/designs/harness.mk


.. note::
   TIP! Start with a small tiny submodule in your design with few pins

.. _`yosys`: https://github.com/The-OpenROAD-Project/yosys
.. _`releases page on GitHub`: https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/releases
.. _`OpenROAD App`: https://github.com/The-OpenROAD-Project/OpenROAD
.. _`OpenROAD-flow`: https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts
.. _`yosys Dockerfile`: https://github.com/The-OpenROAD-Project/yosys/blob/master/Dockerfile
.. _`Docker`: https://docs.docker.com/engine/install
.. _`Makefile`: https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/blob/master/flow/Makefile
.. _`Nangate45`: https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/tree/master/flow/platforms/nangate45
