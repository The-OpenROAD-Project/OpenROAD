#!/usr/bin/env bash
# Entrypoint for the Claude Code container.
# Creates a user matching the host UID/GID so that files created
# inside the container have the correct ownership on the host volume.
set -e

if [ -n "${HOST_UID}" ] && [ -n "${HOST_GID}" ]; then
    USER_NAME="claude-user"
    USER_HOME="/home/${USER_NAME}"

    # Create group (reuse existing if GID is taken)
    if ! getent group "${HOST_GID}" > /dev/null 2>&1; then
        groupadd --gid "${HOST_GID}" "${USER_NAME}"
    fi
    GROUP_NAME=$(getent group "${HOST_GID}" | cut -d: -f1)

    # Create user (reuse existing if UID is taken)
    if ! getent passwd "${HOST_UID}" > /dev/null 2>&1; then
        mkdir -p "${USER_HOME}"
        useradd \
            --uid "${HOST_UID}" \
            --gid "${HOST_GID}" \
            --groups sudo \
            --no-create-home \
            --home-dir "${USER_HOME}" \
            --shell /bin/bash \
            "${USER_NAME}"
    else
        USER_NAME=$(getent passwd "${HOST_UID}" | cut -d: -f1)
        USER_HOME=$(getent passwd "${HOST_UID}" | cut -d: -f6)
    fi

    # Passwordless sudo
    echo "${USER_NAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/claude-user
    chmod 0440 /etc/sudoers.d/claude-user

    export HOME="${USER_HOME}"
    chown "${HOST_UID}:${HOST_GID}" "${USER_HOME}"
    touch "${USER_HOME}/.hushlogin"

    exec setpriv \
        --reuid "${HOST_UID}" \
        --regid "${HOST_GID}" \
        --init-groups \
        -- "$@"
else
    # No UID/GID mapping requested; run as root
    exec "$@"
fi
