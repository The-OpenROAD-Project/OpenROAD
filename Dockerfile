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
/tmp/DependencyInstaller.sh -ci -base $INSTALLER_ARGS
if echo "$fromImage" | grep -q "ubuntu"; then
    echo "fromImage contains 'ubuntu' — stripping section from libQt5Core.so"
    strip --remove-section=.note.ABI-tag /usr/lib/x86_64-linux-gnu/libQt5Core.so || true
else
    echo "Skipping strip command as fromImage does not contain 'ubuntu'"
fi
if command -v apt-get >/dev/null 2>&1; then
    rm -rf /var/lib/apt/lists/*
fi
rm -f /tmp/DependencyInstaller.sh
EOF

################################################################################
#                         Build OpenROAD from source                           #
################################################################################

FROM $devImage AS builder

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
bazelisk run //:cmake
cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=/OpenROAD/deps/toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPENROAD_VERSION=${orVersion}
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

COPY --from=builder /OpenROAD/build/bin/openroad /usr/local/openroad/bin/openroad
COPY --from=builder /OpenROAD/deps/lib/tcl9.0 /usr/local/openroad/lib/tcl9.0
COPY --from=builder /OpenROAD/deps/python /usr/local/openroad/python

# The binary links deps/python/lib/libpython via an absolute build RPATH and
# needs the bundled Tcl/Python homes; a wrapper keeps that environment scoped
# to openroad instead of leaking into the image (the base image's own python3
# and tclsh must keep working).
RUN <<EOF
printf '%s\n' \
  '#!/bin/sh' \
  'export TCL_LIBRARY=/usr/local/openroad/lib/tcl9.0' \
  'export PYTHONHOME=/usr/local/openroad/python' \
  'export LD_LIBRARY_PATH=/usr/local/openroad/python/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}' \
  'exec /usr/local/openroad/bin/openroad "$@"' > /usr/bin/openroad
chmod 755 /usr/bin/openroad
EOF

ENV OPENROAD_EXE=/usr/bin/openroad

RUN <<EOF
groupadd user --gid 9000
useradd --create-home --uid 9000 -g user --skel /etc/skel --shell /bin/bash user
EOF

USER user
WORKDIR /home/user

ENTRYPOINT [ "openroad" ]
