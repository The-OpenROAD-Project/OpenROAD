################################################################################
# Base image
################################################################################

ARG fromImage=ubuntu:24.04
FROM ${fromImage} AS dev

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

WORKDIR /tmp

COPY etc/DependencyInstaller.sh .

RUN chmod +x DependencyInstaller.sh && \
    ./DependencyInstaller.sh -ci -base && \
    ./DependencyInstaller.sh -ci -common && \
    rm DependencyInstaller.sh

################################################################################
# Build OpenROAD
################################################################################

FROM dev AS builder

ARG orVersion=dev

RUN groupadd -g 9000 user && \
    useradd -m -u 9000 -g user -s /bin/bash user

USER user
WORKDIR /OpenROAD

COPY --chown=user:user . .

RUN cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPENROAD_VERSION=${orVersion} && \
    cmake --build build -j$(nproc)

################################################################################
# Final runtime image
################################################################################

FROM ${fromImage}

RUN apt-get update && apt-get install -y \
    libtcl8.6 \
    libqt5core5t64 \
    libqt5gui5t64 \
    libqt5widgets5t64 \
    libqt5charts5 \
    libyaml-cpp0.8 \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd -g 9000 user && \
    useradd -m -u 9000 -g user -s /bin/bash user

COPY --from=builder /OpenROAD/build/bin/openroad /usr/bin/openroad

ENV OPENROAD_EXE=/usr/bin/openroad

USER user
WORKDIR /home/user

ENTRYPOINT ["openroad"]