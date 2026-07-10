FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update \
    && apt-get -y install curl git

RUN curl -Lo bazelisk https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64 \
    && chmod +x bazelisk \
    && mv bazelisk /usr/local/bin/bazelisk

RUN install -m 0755 -d /etc/apt/keyrings \
    && curl -fsSL https://download.docker.com/linux/ubuntu/gpg \
        -o /etc/apt/keyrings/docker.asc \
    && chmod a+r /etc/apt/keyrings/docker.asc \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
        $(. /etc/os-release && echo "${VERSION_CODENAME}") stable" | \
        tee /etc/apt/sources.list.d/docker.list > /dev/null

RUN apt-get -y update \
    && apt-get -y install --no-install-recommends \
      bsdextrautils \
      build-essential \
      clang \
      containerd.io \
      docker-buildx-plugin \
      docker-ce \
      docker-ce-cli \
      groff \
      openjdk-21-jre-headless \
      pandoc \
      python3 \
      python3-yaml \
      time

# Runtime X11/xcb libraries for the Qt GUI build (--//:platform=gui).
# Qt itself is hermetic (qt-bazel prebuilts), but the linked openroad binary
# still resolves libX11/libxcb/libxkbcommon from the system at load time, so
# without these every test against the gui binary dies at ld.so with
# "libX11-xcb.so.1: cannot open shared object file". This is the runtime
# (non -dev) subset of etc/DependencyInstaller.sh's GUI xcb list.
RUN apt-get -y update \
    && apt-get -y install --no-install-recommends \
      libx11-6 \
      libx11-xcb1 \
      libsm6 \
      libice6 \
      libxcb1 \
      libxcb-icccm4 \
      libxcb-image0 \
      libxcb-keysyms1 \
      libxcb-randr0 \
      libxcb-render0 \
      libxcb-render-util0 \
      libxcb-shape0 \
      libxcb-shm0 \
      libxcb-sync1 \
      libxcb-util1 \
      libxcb-xfixes0 \
      libxcb-xkb1 \
      libdbus-1-3 \
      libfontconfig1 \
      libxkbcommon0 \
      libxkbcommon-x11-0

RUN groupadd -g 9000 user \
    && useradd -u 9000 -g 9000 -m -s /bin/bash user
USER user
