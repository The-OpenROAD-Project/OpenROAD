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
# The dev image serves both build systems, so it carries the Bazel dependency
# set too: bazelisk plus the libxml2 and X11/xcb runtime libraries the Bazel
# build needs, which on Ubuntu 26.04 include the libxml2.so.2 compatibility
# symlink without which the prebuilt LLVM lld cannot load.
/tmp/DependencyInstaller.sh -bazel
/tmp/DependencyInstaller.sh -ci -common -save-deps-prefixes=/etc/openroad_deps_prefixes.txt $INSTALLER_ARGS
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

RUN <<EOF
groupadd user --gid 9000
useradd --create-home --uid 9000 -g user --skel /etc/skel --shell /bin/bash user
EOF

USER user
WORKDIR /OpenROAD
COPY --chown=user:user . .
# Keep Bazel's build cache out of the published builder image.
RUN --mount=type=cache,target=/home/user/.cache,uid=9000,gid=9000 <<EOF
bash ./etc/Build.sh -prefix=/OpenROAD/install -threads=${numThreads}
# Preserve the path used by builder-image consumers.
mkdir -p build/bin
ln -s ../../install/bin/openroad build/bin/openroad
rm -f bazel-OpenROAD bazel-bin bazel-out bazel-testlogs
EOF

COPY --chmod=775 --chown=user:user etc/docker-entrypoint.sh /usr/local/bin/.

################################################################################
#                                 Final Image                                  #
################################################################################

FROM $devImage AS final

COPY --chown=root:root --from=builder /OpenROAD/install/ /usr/
ENV OPENROAD_EXE=/usr/bin/openroad

RUN <<EOF
groupadd user --gid 9000
useradd --create-home --uid 9000 -g user --skel /etc/skel --shell /bin/bash user
EOF

USER user
WORKDIR /home/user

ENTRYPOINT [ "openroad" ]
