# Build from sources using Docker

## Prerequisites

For this method you only need to install
[Docker](https://docs.docker.com/engine/install) on your machine.

## Clone and Build

``` shell
$ git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts
$ cd OpenROAD-flow-scripts
$ ./build_openroad.sh
```

## Verify Installation

The binaries are only available from inside the Docker container, thus to
start one use:

``` shell
$ docker run -it -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd)/flow/platforms:/OpenROAD-flow-scripts/flow/platforms:ro openroad/flow-scripts
```

Then, inside docker:

``` shell
$ source ./setup_env.sh
$ yosys -help
$ openroad -help
$ cd flow
$ make
$ exit
```
