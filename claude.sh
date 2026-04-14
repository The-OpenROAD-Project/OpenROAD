#!/usr/bin/env bash
# Wrapper to run Claude Code inside a Docker container with
# --dangerously-skip-permissions.  The repo is volume-mounted so all
# edits are reflected on the host.
#
# Usage:
#   ./claude.sh --build          # build the Docker image (one-time)
#   ./claude.sh                  # start Claude Code interactively
#   ./claude.sh --shell          # get a bash shell instead
#   ./claude.sh -- -p "prompt"   # pass extra args to claude

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- Defaults (overridable via environment) ---
CLAUDE_IMAGE="${CLAUDE_IMAGE:-openroad/openroad-claude:latest}"
CONTAINER_NAME="${CONTAINER_NAME:-claude-openroad}"

# --- Argument parsing ---
SHELL_MODE=0
BUILD_IMAGE=0
CLAUDE_ARGS=()

usage() {
    cat <<EOF
Usage: $0 [OPTIONS] [-- CLAUDE_ARGS...]

Options:
    --build             Build the Docker image
    --shell             Start a bash shell instead of Claude Code
    --image NAME        Override Docker image (default: ${CLAUDE_IMAGE})
    --name NAME         Override container name (default: ${CONTAINER_NAME})
    -h, --help          Show this help

Environment:
    CLAUDE_IMAGE        Docker image override (same as --image).
    CONTAINER_NAME      Container name override (same as --name).
    ANTHROPIC_API_KEY   Optional. Passed through if set.

Examples:
    $0 --build                      # build the image
    $0                              # interactive Claude Code
    $0 -- -p "fix the failing test" # Claude with a prompt
    $0 --shell                      # bash inside the container
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build)     BUILD_IMAGE=1; shift ;;
        --shell)     SHELL_MODE=1; shift ;;
        --image)     CLAUDE_IMAGE="${2?--image requires a value}"; shift 2 ;;
        --name)      CONTAINER_NAME="${2?--name requires a value}"; shift 2 ;;
        -h|--help)   usage ;;
        --)          shift; CLAUDE_ARGS=("$@"); break ;;
        *)           CLAUDE_ARGS+=("$1"); shift ;;
    esac
done

# --- Build image (explicitly or automatically if missing) ---
_build_image() {
    echo "Building Claude Code Docker image: ${CLAUDE_IMAGE}"
    docker build \
        -f "${SCRIPT_DIR}/etc/Dockerfile.claude" \
        -t "${CLAUDE_IMAGE}" \
        "${SCRIPT_DIR}/etc"
    echo "Done.  Image: ${CLAUDE_IMAGE}"
}

if [[ "${BUILD_IMAGE}" -eq 1 ]]; then
    _build_image
    # If only --build was given, exit.
    if [[ "${SHELL_MODE}" -eq 0 ]] && [[ ${#CLAUDE_ARGS[@]} -eq 0 ]]; then
        exit 0
    fi
elif ! docker image inspect "${CLAUDE_IMAGE}" > /dev/null 2>&1; then
    echo "Image ${CLAUDE_IMAGE} not found. Building automatically..."
    _build_image
fi

# --- Determine the user's home inside the container ---
# The entrypoint creates /home/claude-user for the mapped user.
CONTAINER_HOME="/home/claude-user"

# --- Assemble docker run arguments ---
DOCKER_RUN_ARGS=(
    --rm
    --name "${CONTAINER_NAME}"
    -v "${SCRIPT_DIR}:/workspace"
    -e "HOST_UID=$(id -u)"
    -e "HOST_GID=$(id -g)"
    -w /workspace
)

# TTY/stdin handling for interactive, CI, and piped-input cases:
#   * terminal in + terminal out → -it   (normal interactive use)
#   * anything else              → -i    (pass stdin through; no TTY)
# Forcing -it unconditionally breaks non-interactive use
# ("cannot attach stdin to a TTY-enabled container"), and dropping -i
# would silently discard piped input like `cat prompt.txt | claude.sh ...`.
if [[ -t 0 && -t 1 ]]; then
    DOCKER_RUN_ARGS+=(-it)
else
    DOCKER_RUN_ARGS+=(-i)
fi

# Pass through optional environment variables
for var in ANTHROPIC_API_KEY ANTHROPIC_MODEL CLAUDE_CODE_MAX_TURNS \
           CLAUDE_CODE_USE_BEDROCK \
           AWS_ACCESS_KEY_ID AWS_SECRET_ACCESS_KEY AWS_REGION \
           GOOGLE_APPLICATION_CREDENTIALS; do
    if [[ -v "${var}" ]] && [[ -n "${!var}" ]]; then
        DOCKER_RUN_ARGS+=(-e "${var}=${!var}")
    fi
done

# SSH agent forwarding (for git push/pull over SSH)
if [[ -n "${SSH_AUTH_SOCK:-}" ]]; then
    DOCKER_RUN_ARGS+=(
        -v "${SSH_AUTH_SOCK}:/tmp/ssh-agent.sock"
        -e "SSH_AUTH_SOCK=/tmp/ssh-agent.sock"
    )
fi

# Host gitconfig (read-only, for user.name / user.email)
if [[ -f "${HOME}/.gitconfig" ]]; then
    DOCKER_RUN_ARGS+=(-v "${HOME}/.gitconfig:${CONTAINER_HOME}/.gitconfig:ro")
fi

# Persist Claude Code settings / history / memory across runs
CLAUDE_CONFIG_DIR="${HOME}/.claude"
mkdir -p "${CLAUDE_CONFIG_DIR}"
DOCKER_RUN_ARGS+=(-v "${CLAUDE_CONFIG_DIR}:${CONTAINER_HOME}/.claude")

# Claude Code config file (lives next to ~/.claude/, not inside it)
if [[ -f "${HOME}/.claude.json" ]]; then
    DOCKER_RUN_ARGS+=(-v "${HOME}/.claude.json:${CONTAINER_HOME}/.claude.json")
fi

# Persist Bazel cache (avoids re-downloading hundreds of MB each run)
BAZEL_CACHE_DIR="${BAZEL_CACHE_DIR:-${HOME}/.cache/bazel-claude}"
mkdir -p "${BAZEL_CACHE_DIR}"
DOCKER_RUN_ARGS+=(-v "${BAZEL_CACHE_DIR}:${CONTAINER_HOME}/.cache/bazel")

# --- Run ---
if [[ "${SHELL_MODE}" -eq 1 ]]; then
    exec docker run \
        "${DOCKER_RUN_ARGS[@]}" \
        "${CLAUDE_IMAGE}" \
        bash
else
    exec docker run \
        "${DOCKER_RUN_ARGS[@]}" \
        "${CLAUDE_IMAGE}" \
        claude --dangerously-skip-permissions "${CLAUDE_ARGS[@]}"
fi
