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
      build-essential \
      clang \
      containerd.io \
      docker-buildx-plugin \
      docker-ce \
      docker-ce-cli \
      openjdk-21-jre-headless \
      python3 \
      python3-yaml \
      time

RUN groupadd -g 9000 user \
    && useradd -u 9000 -g 9000 -m -s /bin/bash user
USER user
