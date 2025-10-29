#!/bin/bash
if [ ! -z ${CUSTOM_USER+x} ]; then
        if [ -z ${CUSTOM_USER_ID+x} ] || [ -z ${CUSTOM_GROUP_ID+x} ]; then
                echo "You need to set CUSTOM_USER_ID and CUSTOM_GROUP_ID"
                exit 1
        fi
        echo "Starting with USER=${CUSTOM_USER} UID=${CUSTOM_USER_ID} GID=${CUSTOM_GROUP_ID}"
        groupadd --gid "${CUSTOM_GROUP_ID}" "${CUSTOM_USER}"
        useradd --groups sudo -g "${CUSTOM_USER}" \
                --uid "${CUSTOM_USER_ID}" \
                -m "${CUSTOM_USER}"
        chown "${CUSTOM_USER}:${CUSTOM_USER}" -R /OpenROAD
        exec setpriv \
                --reuid "${CUSTOM_USER_ID}" \
                --regid "${CUSTOM_GROUP_ID}" \
                --clear-groups \
                -- "$@"
else
        echo "Starting with default user"
        exec "$@"
fi
