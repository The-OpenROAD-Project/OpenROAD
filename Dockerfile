################################################################################
#                        Install dependencies for dev                          #
################################################################################

# https://github.com/moby/moby/issues/38379#issuecomment-448445652
ARG fromImage=ubuntu:26.04
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
# Installs bazelisk plus the libxml2 and X11/xcb runtime libraries the Bazel
# build needs, which on Ubuntu 26.04 include the libxml2.so.2 compatibility
# symlink without which the prebuilt LLVM lld cannot load.
/tmp/DependencyInstaller.sh -bazel $INSTALLER_ARGS
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
# Release builds pass the published version, for example
# --build-arg orVersion=2026-09-10. The build context has no .git directory,
# so the Bazel stamp cannot find the version by itself.
ARG orVersion=""

RUN <<EOF
set -e
groupadd user --gid 9000
useradd --create-home --uid 9000 -g user --skel /etc/skel --shell /bin/bash user
EOF

USER user
WORKDIR /OpenROAD
COPY --chown=user:user . .
# Keep Bazel's build cache out of the published builder image. `--target
# builder` is a published stage, so whatever this step leaves behind ships:
# etc/Build.sh also writes build/openroad_build.log, which is removed below.
#
# Callers may pass `--secret id=bazelrc,src=<file>` to inject a temporary
# user.bazelrc (loaded via `try-import %workspace%/user.bazelrc` in .bazelrc)
# for remote execution or authenticated remote caching without baking endpoints
# or credentials into image layers; omitting `--secret` is a no-op.
#
#   * `src` must be the repository-root `user.bazelrc`, which .dockerignore
#     excludes, or a file outside the build context. `COPY . .` above runs
#     before this step, so any other in-context file is copied into this
#     published stage first.
#   * Remote-execution callers must also replace .bazelrc's default HTTPS
#     `--remote_cache` with a gRPC endpoint, or clear it with
#     `--remote_cache=`. Bazel rejects an HTTP cache next to
#     `--remote_executor`.
#   * etc/Build.sh passes `--jobs=${numThreads}` on the command line, which
#     overrides `--jobs` in user.bazelrc. Pass `--build-arg numThreads=<N>` to
#     raise fan-out for remote execution.
#   * BuildKit keeps secret contents out of this step's cache key, so adding,
#     changing or removing `--secret` on its own can reuse a warm layer and
#     silently skip the requested configuration. Change a build argument that
#     this step reads, such as `numThreads`, or pass
#     `--no-cache-filter builder`.
RUN --mount=type=cache,target=/home/user/.cache,uid=9000,gid=9000 \
    --mount=type=secret,id=bazelrc,target=/OpenROAD/user.bazelrc,uid=9000,gid=9000,mode=0400,required=false \
    <<EOF
set -e
OPENROAD_VERSION="${orVersion}" \
    bash ./etc/Build.sh -prefix=/OpenROAD/install -threads=${numThreads}
test -x /OpenROAD/install/bin/openroad
rm -f bazel-OpenROAD bazel-bin bazel-out bazel-testlogs
rm -rf /OpenROAD/build
EOF

COPY --chmod=775 --chown=user:user etc/docker-entrypoint.sh /usr/local/bin/.

################################################################################
#                                 Final Image                                  #
################################################################################

FROM $devImage AS final

COPY --chown=root:root --from=builder /OpenROAD/install/ /usr/
ENV OPENROAD_EXE=/usr/bin/openroad

RUN <<EOF
set -e
groupadd user --gid 9000
useradd --create-home --uid 9000 -g user --skel /etc/skel --shell /bin/bash user
EOF

USER user
WORKDIR /home/user

ENTRYPOINT [ "openroad" ]
