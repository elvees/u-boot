#!/bin/bash
# SPDX-License-Identifier: GPL-2.0+
# Copyright 2019-2022 RnD Center "ELVEES", JSC

set -o errexit
set -o pipefail

function help {
    echo "Description:
    Flash the MCom-02 U-Boot image to the SPI from OS running on the board.
    /etc/fw_env.config is used to detect U-Boot environment variables location.
    mtd-utils are required.
Usage:
    $(basename "$0") [OPTION] IMAGE
Arguments:
    IMAGE      U-Boot image file
Options:
    -r         restore U-Boot environment
    -h         print this message"
}

function read_uboot_version {
    head $DEV -c 65536 | strings | grep 'U-Boot SPL'
}

while getopts "hr" arg; do
    case "$arg" in
    h)
        help
        exit 0
    ;;
    r)
        RESTORE_ENV=true
    ;;
    *)
        echo "Incorrect option"
        help
        exit 1
    ;;
    esac
done
shift $((OPTIND-1))
IMAGE=$1
if [[ -z "$IMAGE" ]]; then
    echo "U-Boot image is not specified"
    help
    exit 1
fi
shift
if [ ! -z "$@" ]; then
    echo "Too much arguments"
    help
    exit 1
fi
if [ ! -f "$IMAGE" ]; then
    echo "File '$IMAGE' not found"
    exit 1
fi

# Get environment parameters from a non-empty and non-commented line in the
# config file
ENV_CONFIG=/etc/fw_env.config
CONF_STR="$(grep -vE '(^[[:space:]]*#|^[[:space:]]*$)' $ENV_CONFIG | head -n1)"
readonly DEV=$(echo "$CONF_STR" | awk '{print $1}')
readonly ENV_OFFSET=$(echo "$CONF_STR" | awk '{print $2}')
readonly ENV_SIZE=$(echo "$CONF_STR" | awk '{print $3}')

# TODO: Add lock check
readonly TEMPFILES=$(mktemp -d)
trap 'rm -rf $TEMPFILES' 0
trap 'echo "$LOG" >&2' ERR

if [[ "$RESTORE_ENV" != "true" ]]; then
    # Prepare new image with environment block taken from device
    echo "Saving old environment variables..."
    TMP_IMAGE=$TEMPFILES/tmp.image
    cp "$IMAGE" "$TMP_IMAGE"
    ENV_BLOCK=$TEMPFILES/env
    LOG=$(mtd_debug read "$DEV" "$ENV_OFFSET" "$ENV_SIZE" "$ENV_BLOCK" 2>&1)
    dd if="$ENV_BLOCK" of="$TMP_IMAGE" bs=$(( 16#${ENV_SIZE:2} )) \
        seek=$(( ENV_OFFSET/ENV_SIZE )) conv=notrunc &> /dev/null
    IMAGE=$TMP_IMAGE
fi

readonly IMAGE_SIZE=$(wc -c "$IMAGE" | awk '{print $1}')
# Round up to mtd.erase_size
BLOCK_SIZE=$(mtd_debug info "$DEV" | grep -o "mtd.erasesize = [0-9]*")
BLOCK_SIZE=${BLOCK_SIZE##* }
# shellcheck disable=SC2017
readonly ALIGNED_IMAGE_SIZE=$(( (IMAGE_SIZE + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE ))

echo "Flashing firmware. Do not terminate process!"
echo "Erasing old firmware..."
echo "U-Boot version on device: $(read_uboot_version)"
LOG=$(mtd_debug erase "$DEV" 0 "$ALIGNED_IMAGE_SIZE" 2>&1)
echo "Writing firmware..."
LOG=$(mtd_debug write "$DEV" 0 "$IMAGE_SIZE" "$IMAGE" 2>&1)

echo "Verifying firmware..."
echo "U-Boot version on device: $(read_uboot_version)"
CHECK_IMAGE=$TEMPFILES/check.image
LOG=$(mtd_debug read "$DEV" 0 "$IMAGE_SIZE" "$CHECK_IMAGE" 2>&1)
if cmp "$IMAGE" "$CHECK_IMAGE" &> /dev/null; then
    echo "Flashing successful"
else
    echo "Flashing failed" >&2
    exit 1
fi
