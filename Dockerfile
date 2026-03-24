################################################################################
#                        Install dependencies for dev                          #
################################################################################

# https://github.com/moby/moby/issues/38379#issuecomment-448445652
ARG fromImage=ubuntu:24.04
ARG devImage=dev

FROM $fromImage AS dev

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Los_Angeles
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

ARG INSTALLER_ARGS=""

COPY etc/DependencyInstaller.sh /tmp/.
RUN <<EOF
set -e
/tmp/DependencyInstaller.sh -ci -base
/tmp/DependencyInstaller.sh -ci -common -save-deps-prefixes=/etc/openroad_deps_prefixes.txt $INSTALLER_ARGS
if echo "$fromImage" | grep -q "ubuntu"; then
    echo "fromImage contains 'ubuntu' — stripping section from libQt5Core.so"
    strip --remove-section=.note.ABI-tag /usr/lib/x86_64-linux-gnu/libQt5Core.so || true
else
    echo "Skipping strip command as fromImage does not contain 'ubuntu'"
fi
rm -f /tmp/DependencyInstaller.sh
EOF

################################################################################
#                         Build OpenROAD from source                           #
################################################################################

FROM $devImage AS builder

ARG compiler=gcc
ARG numThreads=NotSet
ARG orVersion=NotSet

RUN <<EOF
groupadd user --gid 9000
useradd --create-home --uid 9000 -g user --skel /etc/skel --shell /bin/bash user
EOF

USER user
WORKDIR /OpenROAD
COPY --chown=user:user . .
RUN <<EOF
# enable compiler for RHEL8
if [ -f /opt/rh/gcc-toolset-13/enable ]; then
    source /opt/rh/gcc-toolset-13/enable
fi
DEPS_ARGS=""
if [ -f /etc/openroad_deps_prefixes.txt ]; then
    DEPS_ARGS=$(cat /etc/openroad_deps_prefixes.txt)
fi
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DOPENROAD_VERSION=${orVersion} $DEPS_ARGS
if [ "$numThreads" = "NotSet" ]; then
    numThreads=$(nproc)
fi
cmake --build build -- -j ${numThreads}
EOF

COPY --chmod=775 --chown=user:user etc/docker-entrypoint.sh /usr/local/bin/.

################################################################################
#                                 Final Image                                  #
################################################################################

FROM $devImage AS final

COPY --from=builder /OpenROAD/build/bin/openroad /usr/bin/.
ENV OPENROAD_EXE=/usr/bin/openroad

RUN <<EOF
groupadd user --gid 9000
useradd --create-home --uid 9000 -g user --skel /etc/skel --shell /bin/bash user
EOF

USER user
WORKDIR /home/user

ENTRYPOINT [ "openroad" ]
