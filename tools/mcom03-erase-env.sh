#!/bin/bash
# SPDX-License-Identifier: GPL-2.0+
# Copyright 2023 RnD Center "ELVEES", JSC

set -o errexit
set -o pipefail

function help {
    echo "Description:
    Utility to erase the MCom-03 SBL or U-Boot environment.
    /etc/fw_env.config is used to detect U-Boot environment variables location.
    /etc/fw_env_sbl.config is used to detect SBL environment variables location.
Usage:
    $(basename "$0") [OPTION] NAME
Arguments:
    NAME        Name of environment to be erased:
                    uboot - U-Boot environment
                    sbl   - SBL environment
Options:
    -h          print this message and exit"
}

while getopts "h" arg; do
    case "$arg" in
    h)
        help
        exit 0
    ;;
    *)
        echo "Incorrect option"
        help
        exit 1
    ;;
    esac
done
if [[ "$#" -ne 1 ]]; then
    echo "Wrong number of arguments"
    help
    exit 1
fi

NAME=$1

if [[ "$NAME" == "uboot" ]]; then
    ENV_CONFIG=/etc/fw_env.config
elif [[ "$NAME" == "sbl" ]]; then
    ENV_CONFIG=/etc/fw_env_sbl.config
else
    echo "Unsupported environment"
    help
    exit 1
fi

# Get environment parameters from a non-empty and non-commented line in the
# config file
CONF_STR="$(grep -vE '(^[[:space:]]*#|^[[:space:]]*$)' $ENV_CONFIG | head -n1)"
DEV=$(echo "$CONF_STR" | awk '{print $1}')
OFFSET=$(echo "$CONF_STR" | awk '{print $2}')
SIZE=$(echo "$CONF_STR" | awk '{print $3}')

# TODO: Add lock check
TEMPFILES=$(mktemp -d)
trap 'rm -rf $TEMPFILES' EXIT
trap 'echo "$LOG" >&2' ERR

# Get real offset if provided one is negative
FULL_SIZE=$(mtd_debug info "$DEV" | grep -o "mtd.size = [0-9]*")
FULL_SIZE=${FULL_SIZE##* }

if (( OFFSET < 0 )); then
    OFFSET=$(( FULL_SIZE + OFFSET ))
fi

# Round up to mtd.erase_size
BLOCK_SIZE=$(mtd_debug info "$DEV" | grep -o "mtd.erasesize = [0-9]*")
BLOCK_SIZE=${BLOCK_SIZE##* }

# Check that environment size is aligned to BLOCK_SIZE
ALIGNED_SIZE=$(( (SIZE + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE ))
if [[ $SIZE -ne $ALIGNED_SIZE ]]; then
    echo "Environment size has to be aligned to '$BLOCK_SIZE'"
    exit 1
fi

# Check that environment offset is aligned to BLOCK_SIZE
ALIGNED_OFFSET=$(( (OFFSET + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE ))
if [[ $OFFSET -ne $ALIGNED_OFFSET ]]; then
    echo "Environment offset has to be aligned to '$BLOCK_SIZE'"
    exit 1
fi

echo "Erasing environment. Do not terminate process! ..."
OLD_ENV=$TEMPFILES/old.env
LOG=$(mtd_debug read "$DEV" "$OFFSET" "$SIZE" "$OLD_ENV" 2>&1)
LOG=$(mtd_debug erase "$DEV" "$OFFSET" "$SIZE" 2>&1)

echo "Verifying ..."
ERASED_ENV=$TEMPFILES/erased.env
LOG=$(mtd_debug read "$DEV" "$OFFSET" "$SIZE" "$ERASED_ENV" 2>&1)
if ! cmp "$OLD_ENV" "$ERASED_ENV" &> /dev/null; then
    echo "Erased successful"
else
    echo "Erase failed" >&2
    exit 1
fi
