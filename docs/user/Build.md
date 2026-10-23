# Installing OpenROAD

OpenROAD is built with [Bazel](Bazel.md).
Install the [prerequisites](#prerequisites), then build and install with GUI support:

``` shell
bazelisk run --//:platform=gui //:install
```

See [Bazel](Bazel.md) for testing, profiling and build configurations.

## Clone Repository

The first step, independent of the build method, is to download the repository:

``` shell
git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD.git
cd OpenROAD
```

OpenROAD git submodules (cloned by the `--recursive` flag) are located in `src/`.

```{note}
There are several methods for obtaining OpenROAD (in order of recommendation):
prebuilt binaries, docker images, and building from source with Bazel.
```

## Build and install with Bazel

### Prerequisites

Install `bazelisk` and the system libraries that the Bazel build needs.
Bazel is the default build system of `DependencyInstaller.sh`:

``` shell
sudo ./etc/DependencyInstaller.sh
```

Add `-no-gui` to skip the GUI-only libraries when you build the CLI-only binary.
Add `-bazel-dev` to install the Bazel developer tools, such as `buildifier`.

```{note}
`DependencyInstaller.sh` installs `bazelisk` to `/usr/local/bin` by default.
To put it in `~/.local/bin`, split the steps:

    sudo ./etc/DependencyInstaller.sh -base
    ./etc/DependencyInstaller.sh -common -local
```

On macOS, do not use `sudo`.
`./etc/DependencyInstaller.sh` installs `bazelisk` with Homebrew.
Install the Xcode command line tools first with `xcode-select --install`.

You can also install [bazelisk](https://bazel.build/install/bazelisk) yourself.
`bazelisk` reads the `.bazelversion` file and downloads the matching Bazel.

### Build and install with the GUI

Build OpenROAD with GUI support and install into `../install/OpenROAD/bin`:

    bazelisk run --//:platform=gui //:install

To install to a custom location, e.g. `/tmp/myinstall`:

    bazelisk run --//:platform=gui //:install -- /tmp/myinstall

To produce an `openroad.tar` file with the install files:

    bazelisk build --//:platform=gui //:tarfile

The tarfile is located at `bazel-bin/packaging/openroad.tar`.

The install process installs the binary `openroad` and the runfile directory
`openroad.runfiles`, which contains runtime data needed by the binary.
It also installs the man pages into `share/openroad/man`.

### Build the CLI-only binary

The default platform is `cli`, which does not build the GUI.
Install the prerequisites with `-bazel -no-gui`, then run:

    bazelisk run --//:platform=cli //:install

To only build the binary, without installing it:

    bazelisk build :openroad

The binary is `bazel-bin/openroad`.
For a GUI build, add `--//:platform=gui` to the command.

### Build options

Pass these options to `bazelisk build` or `bazelisk run`.

| Option                         | Purpose                                      |
|--------------------------------|----------------------------------------------|
| `-c opt`                       | Optimized build. This is the default.        |
| `-c dbg`                       | Debug build.                                 |
| `--cxxopt=<flag>`              | Additional compiler flags.                   |
| `--config=release`             | Embed the git version string.                |
| `--jobs=<N>`                   | Limit the number of parallel build jobs.     |
| `//:install -- <dir>`          | Install into `<dir>`.                        |

Bazel downloads its own toolchain and libraries, so you do not need to point it to the Tcl or `zlib` libraries.

To embed the real git version string, add `--config=release`:

    bazelisk run --config=release --//:platform=gui //:install

### Using `etc/Build.sh`

`./etc/Build.sh` is a wrapper that builds and installs with Bazel.
It always builds with `--config=release`.
Run `./etc/Build.sh -help` for all options.

| `Build.sh` flag              | Bazel option                              |
|------------------------------|-------------------------------------------|
| (default)                    | `--//:platform=gui`                       |
| `-no-gui`                    | `--//:platform=cli`                       |
| `-gpu`                       | `--config=gpu`                            |
| `-threads=<N>`               | `--jobs=<N>`                              |
| `-prefix=<DIR>`              | `//:install -- <DIR>`                     |
| `-local`                     | `//:install -- ~/.local`                  |

``` shell
./etc/Build.sh -no-gui -prefix=/tmp/myinstall
```

Every `./etc/Build.sh` run writes `build/openroad_build.log`.
Attach it when you file an issue in the "Relevant log output" section of the
[issue forms](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/issues/new/choose).
A direct `bazelisk` run does not write a log file.

The other flags in the `-help` output configure the legacy build and have no effect here.
The exceptions are `-clean` and `-dir`, which only remove and name the `build/` directory that holds the log.

### Man pages

Build the man pages with:

    bazelisk build //docs:man_pages

`//:install` installs them as well.

### Link time optimization (LTO)

The default build is `-c opt`, which enables optimization without LTO.
`--config=opt` adds ThinLTO, and `--config=release` includes `--config=opt`.
See [Bazel](Bazel.md#lto) for details.

### Build with Address Sanitizer

Run the tests with the `asan` config:

    bazelisk test --config=asan --test_tag_filters=-py src/...

Address Sanitizer adds instrumentation for detecting memory errors.
OpenROAD runs slower and uses more RAM with it.
See [Bazel](Bazel.md) for details.

### System wide OpenROAD install

```{warning}
Only use the following instructions if you know what you are doing.
```

Install into `/usr/local`:

``` shell
bazelisk run --//:platform=gui //:install -- /usr/local
```

or:

``` shell
./etc/Build.sh -prefix=/usr/local
```

The install script writes below the given directory, so your user must be able to write to it.
The `DESTDIR` variable has no Bazel equivalent.

## Build with Prebuilt Binaries

Courtesy of [Precision Innovations](https://precisioninno.com/), there are prebuilt binaries
of OpenROAD with self-contained dependencies released on a regular basis.
Download them from [OpenROAD VaultLink](https://vaultlink.precisioninno.com/).

Each release has one `.deb` package per supported Ubuntu version.
Install the one that matches your system.
For example, on Ubuntu 26.04:

```shell
sudo apt install ./openroad_<version>_amd64-ubuntu-26.04.deb
```

Check the installation:

```shell
openroad -version
```

```{note}
Older releases are on the
[Precision Innovations GitHub releases page](https://github.com/Precision-Innovations/OpenROAD/releases).
```

## Build with Docker

### Prerequisites

- For this method you only need to install
[Docker](https://docs.docker.com/engine/install) on your machine.
- Ensure that you have sufficient memory allocated to the Virtual Machine (VM)
as per our system [requirements](../index.md#system-requirements). Refer to
this [Docker guide](https://docs.docker.com/config/containers/resource_constraints/) for setting CPU cores and memory limits.

### Installation

Use a Docker image of a supported OS and install OpenROAD in it
with the prebuilt binaries.
Start a container in interactive mode:

```shell
docker run -it ubuntu:26.04
```

Inside the container, download the `.deb` package for the image's Ubuntu version
from [OpenROAD VaultLink](https://vaultlink.precisioninno.com/).
Then install it as described in [Build with Prebuilt Binaries](#build-with-prebuilt-binaries).

The deprecated CMake build, which will be removed on Nov. 1, 2026, is described in [Building with CMake](Build-CMake.md).
