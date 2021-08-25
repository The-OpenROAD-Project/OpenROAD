# Build from sources using Docker

## Prerequisites

For this method you only need to install
[Docker](https://docs.docker.com/engine/install) on your machine.

---

**WARNING**

The `build_openroad.sh` will use the host number of CPUs to compile `openroad`.

Please check your Docker daemon setup to make sure all host CPUs are
available.  If you are not sure, you can check with the command below. If
the output number is different from the number of CPUs from your machine,
then is recommended that you restrict the number of CPUs used by the scripts
(see instructions below).

``` shell
docker run <IMAGE> nproc
# <IMAGE> can be any commonly used OS, e.g., 'centos:centos7'
docker run centos:centos7 nproc
```

You can restrict the number of CPUs with the `-t|--threads N` argument:

``` shell
./build_openroad.sh --threads N
```

---

## Clone and Build

``` shell
git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts
cd OpenROAD-flow-scripts
./build_openroad.sh
```

## Verify Installation

The binaries are only available from inside the Docker container, thus to
start one use:

``` shell
docker run -it -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd)/flow/platforms:/OpenROAD-flow-scripts/flow/platforms:ro openroad/flow-scripts
```

Then, inside docker:

``` shell
source ./setup_env.sh
yosys -help
openroad -help
cd flow
make
exit
```
